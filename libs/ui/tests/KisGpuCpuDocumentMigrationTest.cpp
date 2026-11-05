/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testui.h>
#include <KisGpuPixelTolerance.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <canvas/kis_canvas2.h>
#include <opengl/kis_opengl.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_transaction.h>
#include <kis_crop_saved_extra_data.h>
#include <metadata/KoDocumentInfo.h>
#include <KoColorProfile.h>
#include <KoResourcePaths.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KisGlobalResourcesInterface.h>
#include <kis_paintop_registry.h>
#include <kis_paintop_preset.h>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QPointer>
#include <QScopeGuard>
#include <QSettings>
#include <QDir>
#include <QFileInfo>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
QByteArray pixels(KisPaintDeviceSP device, QRect rect)
{
    QByteArray bytes(qsizetype(rect.width()) * rect.height() * device->pixelSize(), Qt::Uninitialized);
    device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), rect);
    return bytes;
}
}

class KisGpuCpuDocumentMigrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        const auto prefix = qEnvironmentVariable("KIS_TEST_PREFIX_PATH");
        const auto staged = QDir(prefix).filePath(QStringLiteral("share/krita/actions"));
        KoResourcePaths::addAssetDir(QStringLiteral("kis_actions"),
            !prefix.isEmpty() && QFileInfo::exists(QDir(staged).filePath(QStringLiteral("krita.action"))) ?
            staged : QStringLiteral(KRITA_SOURCE_DIR "/krita"));
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        const auto path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(preset);
        preset->setName(QStringLiteral("CPU migration fixture"));
        preset->setFilename(QStringLiteral("cpu-migration-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
    }
    void documentKeepsFilesAndHistory_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8-native-history-save-and-autosave") << false;
        QTest::newRow("u16-redo-capacity-and-busy-rejection") << true;
    }
    void documentKeepsFilesAndHistory();
};

void KisGpuCpuDocumentMigrationTest::documentKeepsFilesAndHistory()
{
    QFETCH(bool, boundary);
    QSettings display(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                      QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const auto rendererKey = QStringLiteral("OpenGLRenderer");
    const bool hadRenderer = display.contains(rendererKey);
    const auto renderer = display.value(rendererKey);
    const auto restoreRenderer = qScopeGuard([&] {
        if (hadRenderer) display.setValue(rendererKey, renderer);
        else display.remove(rendererKey);
        display.sync();
    });
    KisConfig(true).disableOpenGL();
    constexpr qsizetype capacity = 64 * 1024 * 1024;
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const int depth = boundary ? 2 : 1;
    const QRect edited(252, 3, 13, 19);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = {270, 48};
    raster.format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer layer;
    layer.uuid = QUuid::createUuid();
    layer.name = QStringLiteral("Original paint");
    raster.layers = {layer};
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    QPointer<KisDocument> document(KisPart::instance()->createDocument());
    const auto deleteDocument = qScopeGuard([&] { delete document.data(); });
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("CPU document adoption"), space);
    document->setCurrentImage(image);
    image->setResolution(1.5, 2.5);
    document->documentInfo()->setAboutInfo(QStringLiteral("title"), QStringLiteral("Retained title"));
    document->setAssistantsGlobalColor(QColor(17, 63, 129, 201));
    const auto id = owner->operations().activeLayer();
    KisPaintDeviceSP seed = new KisPaintDevice(space);
    seed->fill(edited, KoColor(QColor(37, 143, 211, 173), space));
    // GPU writes use RGBA order, while the public native device stores BGRA.
    const auto native = pixels(seed, edited);
    QByteArray canonical = native;
    for (qsizetype i = 0; i < canonical.size(); i += 4 * depth)
        for (int byte = 0; byte < depth; ++byte) std::swap(canonical[i + byte], canonical[i + 2 * depth + byte]);
    QCOMPARE(owner->replaceRgbaPixels(id, edited, canonical), Gpu::Result::Success);
    QVERIFY(image->setGpuSelectionRect({250, 1, 17, 27}, boundary));
    if (boundary) {
        QCOMPARE(owner->resizeCanvas({4, -2, 280, 52}, false), Gpu::Result::Success);
        QCOMPARE(owner->cropLayer(id, edited.adjusted(1, 1, -1, -1)), Gpu::Result::Success);
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto originalPath = directory.filePath(QStringLiteral("original.kra"));
    QVERIFY(document->saveAs(originalPath, KisDocument::nativeFormatMimeType(), false));
    document->waitForSavingToComplete();
    QVERIFY(!document->isModified());
    QCOMPARE(owner->renameLayer(id, QStringLiteral("GPU name")), Gpu::Result::Success);
    QCOMPARE(owner->setLayerOpacity(id, 197), Gpu::Result::Success);
    if (boundary) QCOMPARE(owner->undo(), Gpu::Result::Success);
    else {
        QVERIFY(document->saveAs(originalPath, KisDocument::nativeFormatMimeType(), false));
        document->waitForSavingToComplete();
    }
    const auto version = owner->operations().snapshot();
    const auto root = image->rootLayer();
    auto node = root->firstChild();
    auto paint = node->paintDevice();
    const auto selection = image->globalSelection();
    const auto selectionBytes = pixels(selection->pixelSelection(), image->bounds());
    const auto beforePixels = pixels(paint, image->bounds());
    const auto projection = pixels(image->projection(), image->bounds());
    auto *stack = document->undoStack();
    const int historyIndex = stack->index(), historyCount = stack->count(), savedIndex = stack->cleanIndex();
    QStringList historyLabels;
    for (int i = 0; i < historyCount; ++i) historyLabels.append(stack->text(i));
    QCOMPARE(document->isModified(), boundary);
    std::unique_ptr<KisMainWindow> window;
    KisView *view = nullptr;
    QPointer<QWidget> previousCanvas;
    if (!boundary) {
        window.reset(KisPart::instance()->createMainWindow());
        view = window->newView(document.data());
        QVERIFY(view);
        window->viewManager()->setCurrentView(view);
        previousCanvas = view->canvasBase()->canvasWidget();
    }
    const auto closeWindow = qScopeGuard([&] {
        if (document) document->setModified(false);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    QSignalSpy engineChanged(image.data(), &KisImage::sigImageEngineChanged);
    if (boundary) {
        QVERIFY(!document->migrateToCpu(1));
        QCOMPARE(image->gpuImage(), owner);
        QCOMPARE(owner->operations().snapshot(), version);
        QCOMPARE(stack->index(), historyIndex);
        QCOMPARE(stack->count(), historyCount);
        QCOMPARE(stack->cleanIndex(), savedIndex);
        QCOMPARE(image->globalSelection(), selection);
        QCOMPARE(document->path(), originalPath);
        QVERIFY(document->isModified());
        QCOMPARE(engineChanged.size(), 0);
        image->barrierLock();
        qsizetype currentBytes = 0;
        QVERIFY(image->prepareCpuMigration(capacity, &currentBytes));
        image->unlock();
        QVERIFY(!document->migrateToCpu(currentBytes));
        QCOMPARE(image->gpuImage(), owner);
        QCOMPARE(owner->operations().snapshot(), version);
        const auto stroke = owner->beginStroke(id, Gpu::Brush{}, 0);
        QVERIFY(!stroke.isNull());
        QVERIFY(!document->migrateToCpu(capacity));
        QCOMPARE(owner->operations().activeOperation(), stroke);
        QCOMPARE(owner->cancelStroke(stroke), Gpu::Result::Success);
        QVERIFY(QMetaObject::invokeMethod(document.data(), "slotAutoSave", Qt::DirectConnection));
        QVERIFY(document->isSaving());
        QVERIFY(!document->migrateToCpu(capacity));
        document->waitForSavingToComplete();
        QCOMPARE(image->gpuImage(), owner);
    }
    QVERIFY(document->migrateToCpu(capacity));
    QVERIFY(!image->gpuImage());
    QVERIFY(!paint->prepareCpuMigration(capacity));
    QVERIFY(document->migrateToCpu(capacity));
    QCOMPARE(engineChanged.size(), 1);
    QCOMPARE(document->image().data(), image.data());
    QCOMPARE(image->rootLayer(), root);
    QCOMPARE(root->firstChild(), node);
    QCOMPARE(node->paintDevice(), paint);
    QCOMPARE(image->globalSelection(), selection);
    QCOMPARE(pixels(selection->pixelSelection(), image->bounds()), selectionBytes);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, image->bounds()), beforePixels, depth);
    QCOMPARE(document->path(), originalPath);
    QCOMPARE(document->localFilePath(), originalPath);
    QCOMPARE(document->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Retained title"));
    QCOMPARE(document->assistantsGlobalColor(), QColor(17, 63, 129, 201));
    QCOMPARE(image->xRes(), 1.5);
    QCOMPARE(image->yRes(), 2.5);
    QCOMPARE(stack->index(), historyIndex);
    QCOMPARE(stack->count(), historyCount);
    QCOMPARE(stack->cleanIndex(), savedIndex);
    QCOMPARE(document->isModified(), boundary);
    if (view) {
        QVERIFY(view->canvasBase()->canvasWidget() != previousCanvas.data());
        QCOMPARE(view->image().data(), image.data());
        QTRY_COMPARE(view->currentNode(), node);
    }
    for (int i = 0; i < historyCount; ++i) QCOMPARE(stack->text(i), historyLabels[i]);
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(image->projection(), image->bounds()), projection, depth);

    stack->setIndex(historyCount);
    image->waitForDone();
    QCOMPARE(image->root()->firstChild()->name(), QStringLiteral("GPU name"));
    QCOMPARE(image->root()->firstChild()->opacity(), quint8(197));
    stack->setIndex(savedIndex);
    image->waitForDone();
    QVERIFY(!document->isModified());
    QCOMPARE(image->root()->firstChild()->name(), boundary ? layer.name : QStringLiteral("GPU name"));
    QCOMPARE(document->preActivatedNode(), image->root()->firstChild());
    if (boundary) {
        const auto *crop = dynamic_cast<const KisCropSavedExtraData *>(stack->command(savedIndex - 1)->extraData());
        QVERIFY(crop);
        QCOMPARE(crop->cropNode(), image->root()->firstChild());
        QCOMPARE(crop->cropRect(), edited.adjusted(1, 1, -1, -1));
    }
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(image->root()->firstChild()->paintDevice(), image->bounds()), beforePixels, depth);
    stack->setIndex(0);
    image->waitForDone();
    QVERIFY(!image->globalSelection());
    QCOMPARE(pixels(image->root()->firstChild()->paintDevice(), edited), QByteArray(native.size(), '\0'));
    stack->setIndex(historyIndex);
    image->waitForDone();
    QCOMPARE(image->rootLayer(), root);
    QCOMPARE(root->firstChild(), node);
    QCOMPARE(node->paintDevice(), paint);
    QCOMPARE(image->globalSelection(), selection);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, image->bounds()), beforePixels, depth);
    QCOMPARE(owner->operations().snapshot(), version);

    image->barrierLock();
    KisTransaction transaction(kundo2_i18n("Native edit"), paint);
    paint->fill(edited, KoColor(QColor(191, 71, 13, 239), space));
    transaction.commit(image->undoAdapter());
    node->setDirty(edited);
    image->unlock();
    image->waitForDone();
    const auto afterPixels = pixels(paint, image->bounds());
    QVERIFY(!pixelStepDifference(afterPixels, beforePixels, depth).isEmpty());
    stack->undo();
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, image->bounds()), beforePixels, depth);
    stack->redo();
    image->waitForDone();
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(paint, image->bounds()), afterPixels, depth);
    const auto migratedPath = directory.filePath(QStringLiteral("native.kra"));
    QVERIFY(document->saveAs(migratedPath, KisDocument::nativeFormatMimeType(), false));
    document->waitForSavingToComplete();
    QVERIFY(!document->isModified());
    QScopedPointer<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(migratedPath, KisDocument::DontAddToRecent | KisDocument::UseCpuEngine));
    QCOMPARE(reopened->image()->root()->firstChild()->uuid(), node->uuid());
    QCOMPARE(reopened->image()->root()->firstChild()->name(), node->name());
    QCOMPARE(reopened->image()->root()->firstChild()->opacity(), node->opacity());
    QCOMPARE(reopened->documentInfo()->aboutInfo(QStringLiteral("title")), QStringLiteral("Retained title"));
    QCOMPARE(reopened->assistantsGlobalColor(), QColor(17, 63, 129, 201));
    QCOMPARE(reopened->image()->xRes(), 1.5);
    QCOMPARE(reopened->image()->yRes(), 2.5);
    QCOMPARE(reopened->image()->profile()->rawData(), space->profile()->rawData());
    QVERIFY(reopened->image()->globalSelection());
    QCOMPARE(pixels(reopened->image()->globalSelection()->pixelSelection(), image->bounds()), selectionBytes);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(reopened->image()->root()->firstChild()->paintDevice(), image->bounds()), afterPixels, depth);

    stack->undo();
    image->waitForDone();
    QVERIFY(document->isModified());
    QSignalSpy checkpoint(document.data(), &KisDocument::sigRecoveryAutoSaveFinished);
    QCOMPARE(document->requestRecoveryAutoSave(), KisDocument::RecoveryAutoSaveStartResult::Started);
    QTRY_COMPARE(checkpoint.size(), 1);
    QVERIFY(checkpoint[0][1].toBool());
    QVERIFY(document->isModified());
    QCOMPARE(document->path(), migratedPath);
    QCOMPARE(stack->index(), historyIndex);
    QCOMPARE(stack->cleanIndex(), historyIndex + 1);
    QVERIFY(reopened->openPath(checkpoint[0][0].toString(), KisDocument::DontAddToRecent | KisDocument::UseCpuEngine));
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(pixels(reopened->image()->root()->firstChild()->paintDevice(), image->bounds()), beforePixels, depth);
    QCOMPARE(owner->operations().snapshot(), version);
}

KISTEST_MAIN(KisGpuCpuDocumentMigrationTest)
#include "KisGpuCpuDocumentMigrationTest.moc"
