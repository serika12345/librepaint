/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <KisGpuPixelTolerance.h>
#include <QDockWidget>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QIcon>
#include <QPointer>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <KLocalizedString>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <KoCanvasResourcesIds.h>
#include <KoPointerEvent.h>
#include <KoToolManager.h>
#include <KisAsyncColorSamplerHelper.h>
#include <actions/input/KisApplicationInputActions.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_input_profile_manager.h>
#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_transparency_mask.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_config.h>
#include <kis_tool.h>
#include <kis_tool_utils.h>
#include <nodes/kis_node_manager.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <memory>

class KisGpuColorSamplingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        QString path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        const auto prefix = qEnvironmentVariable("KIS_TEST_PREFIX_PATH");
        const auto staged = QDir(prefix).filePath(QStringLiteral("bin/data/test1.bundle"));
        if (!prefix.isEmpty() && QFileInfo(staged).isFile()) path = staged;
        const auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        m_preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(m_preset);
        m_preset->setName(QStringLiteral("GPU color sampling fixture"));
        m_preset->setFilename(QStringLiteral("gpu-color-sampling-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        m_preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(m_preset));
    }
    void samplesLayerAndProjectionWithoutEditing_data()
    {
        QTest::addColumn<bool>("currentLayer");
        QTest::addColumn<bool>("foreground");
        QTest::addColumn<QString>("transition");
        QTest::addColumn<int>("alpha");
        QTest::addColumn<bool>("gpu");
        QTest::addColumn<int>("blend");
        QTest::addColumn<int>("radius");
        QTest::newRow("layer-foreground") << true << true << QString() << 255 << true << 100 << 1;
        QTest::newRow("destroy-on-raw-color") << true << true << QStringLiteral("destroy") << 255 << true << 100 << 1;
    }
    void samplesLayerAndProjectionWithoutEditing();
    void samplesU16DocumentColorWithoutQuantizing();
    void samplesMaskedLayerThroughHistoryAndReopen_data()
    {
        QTest::addColumn<bool>("words");
        QTest::addColumn<int>("maskDefault");
        // #89 A1b-2: retain both masked-sampling regressions through save/reopen.
        QTest::newRow("u8-mask0") << false << 0;
        QTest::newRow("u16-mask17") << true << 17;
    }
    void samplesMaskedLayerThroughHistoryAndReopen();
private:
    KisPaintOpPresetSP m_preset;
};

void KisGpuColorSamplingTest::samplesMaskedLayerThroughHistoryAndReopen()
{
    // The ordinary current-layer sampler reads the physical node projection,
    // including mask changes and history, without applying parent opacity.
    QFETCH(bool, words);
    QFETCH(int, maskDefault);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("source.kra"));
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), 32, 16, cs, "Masked sampling");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 85);
        layer->paintDevice()->fill({0, 0, 32, 16}, KoColor(QColor(241, 37, 89, 219), cs));
        layer->paintDevice()->fill({12, 9, 1, 1}, KoColor(QColor(17, 129, 233, 255), cs));
        QVERIFY(image->addNode(layer));
        KisSelectionSP selection = new KisSelection;
        const quint8 defaultCoverage = maskDefault;
        selection->pixelSelection()->setDefaultPixel(KoColor(&defaultCoverage, KoColorSpaceRegistry::instance()->alpha8()));
        selection->pixelSelection()->select({0, 0, 32, 16}, 128);
        const quint8 coverage[]{0, 1, 128, 255};
        selection->pixelSelection()->writeBytes(coverage, {10, 9, 4, 1});
        KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Coverage"));
        mask->setSelection(selection); QVERIFY(image->addNode(mask, layer));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false));
        document->waitForSavingToComplete();
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            const auto artifact = QDir(results).filePath(QStringLiteral("masked-sampling-%1-source.kra")
                .arg(QString::fromLatin1(QTest::currentDataTag())));
            if (!QFileInfo::exists(artifact)) QVERIFY(QFile::copy(baseline, artifact));
        }
    }
    QList<KoColor> reference;
    for (const bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        QByteArray savedPixels, savedProjection, savedCoverage;
        int stage = 0;
        for (const bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent),
                qPrintable(QStringLiteral("gpu=%1 reopened=%2: %3").arg(gpu).arg(reopened).arg(document->errorMessage())));
            auto image = KisImageSP(document->image()); image->waitForDone();
            QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = image->root()->firstChild(); QVERIFY(layer);
            const auto mask = layer->firstChild(); QVERIFY(mask);
            std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
            auto *view = window->newView(document.get()); QVERIFY(view);
            QPointer<KisDocument> lifetime(document.release());
            const auto cleanup = qScopeGuard([&] {
                if (lifetime) {
                    lifetime->setModified(false);
                    window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
                }
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
            window->viewManager()->setCurrentView(view);
            window->show();
            auto *manager = window->viewManager()->nodeManager();
            window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
            manager->slotNonUiActivatedNode(layer);
            QTRY_COMPARE(view->currentNode(), layer);
            KisAsyncColorSamplerHelper helper(view->canvasBase());
            QSignalSpy raw(&helper, &KisAsyncColorSamplerHelper::sigRawColorSelected);
            const auto sample = [&] {
                manager->slotNonUiActivatedNode(layer);
                QTRY_COMPARE(view->currentNode(), layer);
                const auto history = lifetime->undoStack()->index();
                const auto fixed = image->gpuImage() ? image->gpuImage()->operations().snapshot() : LibrePaint::Image::Gpu::Snapshot{};
                for (int radius : {1, 3}) {
                    // Restoring a mask also restores the selected node asynchronously.
                    // Select the paint layer for each independent sampling action.
                    image->waitForDone();
                    QApplication::processEvents();
                    manager->slotUiActivatedNode(layer);
                    manager->slotSetSelectedNodes({layer});
                    QTRY_COMPARE(view->currentNode(), layer);
                    helper.activate(true, true);
                    helper.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), radius, 100);
                    helper.endAction(); helper.deactivate();
                    QTRY_COMPARE(raw.size(), 1);
                    const auto color = qvariant_cast<KoColor>(raw.takeFirst().first());
                    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                    if (!results.isEmpty()) {
                        QFile output(QDir(results).filePath(QStringLiteral("masked-sampling-%1-%2-%3.bgra")
                            .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("native")).arg(stage)));
                        QVERIFY(output.open(QIODevice::WriteOnly));
                        QCOMPARE(output.write(reinterpret_cast<const char *>(color.data()), color.colorSpace()->pixelSize()),
                            qint64(color.colorSpace()->pixelSize()));
                    }
                    if (!gpu) reference.append(color);
                    else {
                        auto expected = reference.value(stage);
                        auto actual = color;
                        expected.convertTo(cs); actual.convertTo(cs);
                        const auto difference = pixelStepDifference(
                            QByteArray(reinterpret_cast<const char *>(actual.data()), cs->pixelSize()),
                            QByteArray(reinterpret_cast<const char *>(expected.data()), cs->pixelSize()), words ? 2 : 1);
                        QVERIFY2(difference.isEmpty(), qPrintable(difference));
                    }
                    ++stage;
                }
                QCOMPARE(lifetime->undoStack()->index(), history);
                if (fixed) QCOMPARE(image->gpuImage()->operations().snapshot(), fixed);
            };
            sample();
            if (!reopened) {
                manager->slotUiActivatedNode(mask); manager->slotSetSelectedNodes({mask}); manager->removeNode();
                image->waitForDone(); QVERIFY(!layer->firstChild()); sample();
                lifetime->undoStack()->undo(); image->waitForDone(); QVERIFY(layer->firstChild()); sample();
                lifetime->undoStack()->redo(); image->waitForDone(); QVERIFY(!layer->firstChild()); sample();
                lifetime->undoStack()->undo(); image->waitForDone(); QVERIFY(layer->firstChild());
            } else {
                const KoColor edit(QColor(233, 81, 19, 255), cs);
                layer->paintDevice()->writeBytes(edit.data(), {12, 9, 1, 1});
                layer->setDirty(image->bounds()); image->waitForDone(); sample();
            }
            QVERIFY(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false));
            lifetime->waitForSavingToComplete();
            if (gpu && reopened) {
                savedPixels.resize(32 * 16 * cs->pixelSize()); savedProjection.resize(savedPixels.size());
                savedCoverage.resize(32 * 16);
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(savedPixels.data()), {0, 0, 32, 16});
                layer->projection()->readBytes(reinterpret_cast<quint8 *>(savedProjection.data()), {0, 0, 32, 16});
                const auto maskView = qobject_cast<KisTransparencyMask *>(layer->firstChild().data()); QVERIFY(maskView);
                maskView->selection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(savedCoverage.data()), {0, 0, 32, 16});
            }
        }
        if (gpu) {
            // The existing native reader must recover a GPU save's mask,
            // including its global default, and exact editable paint pixels.
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(saved, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto decoded = KisImageSP(native->image()); decoded->waitForDone();
            QVERIFY(!decoded->gpuImage());
            auto layer = decoded->root()->firstChild(); QVERIFY(layer);
            const auto mask = qobject_cast<KisTransparencyMask *>(layer->firstChild().data()); QVERIFY(mask);
            QCOMPARE(mask->selection()->pixelSelection()->defaultPixel().opacityU8(), quint8(maskDefault));
            QByteArray pixels(savedPixels.size(), '\0'), projection(savedProjection.size(), '\0'), coverage(savedCoverage.size(), '\0');
            layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), {0, 0, 32, 16});
            layer->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), {0, 0, 32, 16});
            mask->selection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), {0, 0, 32, 16});
            QCOMPARE(pixels, savedPixels); QCOMPARE(projection, savedProjection); QCOMPARE(coverage, savedCoverage);
        }
    }
}

void KisGpuColorSamplingTest::samplesU16DocumentColorWithoutQuantizing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = {32, 16};
    raster.format = Gpu::PixelFormat::Rgba16;
    raster.layers.append({QStringLiteral("Sample"), true, {}, {}, {0xabcd, 0x2468, 0x1357, 65535}});
    const auto cs = KoColorSpaceRegistry::instance()->rgb16();
    raster.colorProfile = cs->profile()->rawData();
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const quint16 bgra[] = {0x1357, 0x2468, 0xabcd, 65535};
    const KoColor expected(reinterpret_cast<const quint8 *>(bgra), cs);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("U16 sampling"), cs));
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    QTRY_VERIFY(view->currentNode());
    const auto version = owner->operations().snapshot();
    KisAsyncColorSamplerHelper helper(view->canvasBase());
    QSignalSpy raw(&helper, &KisAsyncColorSamplerHelper::sigRawColorSelected);
    for (const bool layer : {true, false}) {
        helper.activate(layer, true);
        helper.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), 1, 100);
        helper.endAction();
        helper.deactivate();
        QTRY_COMPARE(raw.size(), 1);
        QCOMPARE(qvariant_cast<KoColor>(raw.takeFirst().first()), expected);
        QCOMPARE(owner->operations().snapshot(), version);
        QCOMPARE(lifetime->undoStack()->count(), 0);
    }

}

void KisGpuColorSamplingTest::samplesLayerAndProjectionWithoutEditing()
{
    QFETCH(bool, currentLayer);
    QFETCH(bool, foreground);
    QFETCH(QString, transition);
    QFETCH(int, alpha);
    QFETCH(bool, gpu);
    QFETCH(int, blend);
    QFETCH(int, radius);
    const bool averaging = transition.contains(QStringLiteral("average"));
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = {64, 48};
    raster.activeLayer = 1;
    raster.layers = {{QStringLiteral("Destination"), true, {}, {}, {129, 129, 129, 255}},
                     {QStringLiteral("Source"), true, {}, {}, {126, 126, 126, 255}}};
    raster.layers[1].opacity = 85;
    raster.layers[1].defaultPixel[3] = quint8(alpha);
    raster.layers[0].uuid = QUuid(QStringLiteral("{d3000001-1122-3344-5566-778899aabbcc}"));
    raster.layers[1].uuid = QUuid(QStringLiteral("{d3000002-1122-3344-5566-778899aabbcc}"));
    if (averaging) {
        raster.layers[1].defaultPixel = {255, 0, 0, 255};
        raster.layers[1].opacity = 255;
    }
    const auto owner = gpu ? Gpu::PaintImageSP::create(execution, raster) : Gpu::PaintImageSP();
    if (averaging) {
        QCOMPARE(owner->replaceRgbaPixels(owner->operations().snapshot()->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("0000ffff")), Gpu::Result::Success);
    }
    if (transition == QStringLiteral("colorful")) {
        QCOMPARE(owner->replaceRgbaPixels(owner->operations().snapshot()->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("1561c9ff")), Gpu::Result::Success);
    }
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image;
    KisConfig(true).disableOpenGL();
    if (gpu) image = KisImage::fromGpuImage(owner, QStringLiteral("Color sampling"));
    else {
        image = new KisImage(document->createUndoStore(), 64, 48, cs, "Color sampling");
        KisPaintLayerSP bottom = new KisPaintLayer(image, QStringLiteral("Destination"), 255);
        KisPaintLayerSP top = new KisPaintLayer(image, QStringLiteral("Source"), 85);
        bottom->setUuid(raster.layers[0].uuid);
        top->setUuid(raster.layers[1].uuid);
        bottom->paintDevice()->fill({0, 0, 64, 48}, KoColor(QColor(129, 129, 129), cs));
        top->paintDevice()->fill({0, 0, 64, 48}, KoColor(QColor(126, 126, 126, alpha), cs));
        QVERIFY(image->addNode(bottom));
        QVERIFY(image->addNode(top, image->root(), bottom));
        image->refreshGraphAsync();
        image->waitForDone();
    }
    document->setCurrentImage(image);
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto cleanup = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    QTRY_VERIFY(!windowReady.isEmpty());
    if (!gpu) window->viewManager()->nodeManager()->slotNonUiActivatedNode(image->root()->lastChild());
    QTRY_COMPARE(view->currentNode()->uuid(), raster.layers[1].uuid);
    const auto previousSpace = transition == QStringLiteral("rgb16")
        ? KoColorSpaceRegistry::instance()->rgb16(cs->profile()) : cs;
    QVERIFY(previousSpace);
    const KoColor oldForeground(QColor(5, 10, 20), previousSpace), oldBackground(QColor(200, 210, 220), cs);
    resources->setFGColor(oldForeground);
    resources->setBGColor(oldBackground);
    lifetime->setModified(false);
    const auto before = owner ? owner->operations().snapshot() : Gpu::Snapshot();
    const auto pixels = [&] {
        QByteArray bytes(64 * 48 * 4, '\0');
        image->projection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), {0, 0, 64, 48});
        return bytes;
    };
    auto expectedPixels = pixels();
    auto expectedVersion = before;
    auto history = lifetime->undoStack()->count();
    bool modified = false;
    if (transition.startsWith(QStringLiteral("tool"))) {
        KisToolUtils::ColorSamplerConfig previous;
        previous.load();
        KisToolUtils::ColorSamplerConfig config;
        config.radius = radius;
        config.blend = blend;
        config.sampleMerged = true;
        config.toForegroundColor = true;
        config.save();
        const auto restore = qScopeGuard([&] {
            KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
            previous.save();
        });
        view->canvasBase()->resourceManager()->setResource(KoCanvasResource::UsingOtherColor, false);
        const auto toolId = QStringLiteral("KritaSelected/KisToolColorSampler");
        KoToolManager::instance()->switchToolRequested(toolId);
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
        auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
        QVERIFY(tool);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9));
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        KoPointerEvent start(&press, point);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        KoPointerEvent end(&release, point);
        try {
            tool->beginPrimaryAction(&start);
            tool->endPrimaryAction(&end);
        } catch (const std::exception &failure) {
            QFAIL(failure.what());
        }
        const QColor sampled = averaging ? (blend == 50 ? QColor(116, 5, 24) : QColor(227, 0, 28))
            : (blend == 50 ? QColor(66, 69, 74) : QColor(128, 128, 128));
        QTRY_COMPARE(resources->fgColor(), KoColor(sampled, cs));
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(pixels(), expectedPixels);
        QCOMPARE(lifetime->undoStack()->count(), history);
        QVERIFY(!lifetime->isModified());
        return;
    }
    auto helper = std::make_unique<KisAsyncColorSamplerHelper>(view->canvasBase());
    if (transition == QStringLiteral("preview")) helper->setUpdateGlobalColor(false);
    QSignalSpy raw(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected);
    QSignalSpy selected(helper.get(), &KisAsyncColorSamplerHelper::sigColorSelected);
    QSignalSpy finished(helper.get(), &KisAsyncColorSamplerHelper::sigFinalColorSelected);
    bool restarted = false;
    if (transition == QStringLiteral("destroy")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] { helper.reset(); });
    } else if (transition == QStringLiteral("restart")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] {
            if (restarted) return;
            restarted = true;
            helper->activate(false, foreground);
            helper->startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), radius, blend);
            helper->endAction();
            helper->deactivate();
        });
    } else if (transition == QStringLiteral("close-raw")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] { owner->close(); });
    } else if (transition == QStringLiteral("replace-raw")) {
        connect(helper.get(), &KisAsyncColorSamplerHelper::sigRawColorSelected, helper.get(), [&] {
            Gpu::RasterImage replacement;
            replacement.size = raster.size;
            replacement.layers = {{QStringLiteral("Replacement"), true, {}, {}, {42, 42, 42, 255}}};
            const auto changed = Gpu::PaintImageSP::create(execution, replacement);
            lifetime->setCurrentImage(KisImage::fromGpuImage(changed, QStringLiteral("Replacement")));
        });
    }
    helper->activate(currentLayer, foreground);
    QString error;
    try {
        helper->startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), radius, blend);
        QCOMPARE(raw.size(), 0);
        if (transition == QStringLiteral("palette")) resources->setFGColor(KoColor(QColor(230, 1, 40), cs));
        if (transition == QStringLiteral("edit")) {
            QCOMPARE(owner->replaceRgbaPixels(before->layers[1].id, {12, 9, 1, 1}, QByteArray::fromHex("fafafaff")), Gpu::Result::Success);
            expectedVersion = owner->operations().snapshot();
            history = lifetime->undoStack()->count();
            modified = lifetime->isModified();
            expectedPixels = pixels();
        }

        helper->endAction();
    } catch (const std::exception &exception) {
        error = QString::fromUtf8(exception.what());
    }
    helper->deactivate();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    if (transition == QStringLiteral("transparent")) {
        KisAsyncColorSamplerHelper barrier(view->canvasBase());
        barrier.setUpdateGlobalColor(false);
        QSignalSpy ready(&barrier, &KisAsyncColorSamplerHelper::sigFinalColorSelected);
        barrier.activate(false, false);
        barrier.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), 1, 100);
        barrier.endAction();
        barrier.deactivate();
        QTRY_COMPARE(ready.size(), 1);
        QCOMPARE(qvariant_cast<KoColor>(ready.first().first()).toQColor(), QColor(129, 129, 129));
        QCOMPARE(raw.size(), 0);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(resources->fgColor(), oldForeground);
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(pixels(), expectedPixels);
        QCOMPARE(owner->operations().snapshot(), before);
        return;
    }
    if (transition == QStringLiteral("replace")) {
        Gpu::RasterImage replacement;
        replacement.size = raster.size;
        replacement.layers = {{QStringLiteral("Replacement"), true, {}, {}, {42, 42, 42, 255}}};
        const auto changed = Gpu::PaintImageSP::create(execution, replacement);
        lifetime->setCurrentImage(KisImage::fromGpuImage(changed, QStringLiteral("Replacement")));
        KisAsyncColorSamplerHelper latest(view->canvasBase());
        latest.setUpdateGlobalColor(false);
        QSignalSpy ready(&latest, &KisAsyncColorSamplerHelper::sigFinalColorSelected);
        latest.activate(false, false);
        latest.startAction(view->canvasBase()->coordinatesConverter()->imageToDocument(QPointF(12, 9)), 1, 100);
        latest.endAction();
        latest.deactivate();
        QTRY_COMPARE(ready.size(), 1);
        QCOMPARE(qvariant_cast<KoColor>(ready.first().first()).toQColor(), QColor(42, 42, 42));
        QCOMPARE(raw.size(), 0);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(resources->fgColor(), oldForeground);
        QCOMPARE(resources->bgColor(), oldBackground);
        QCOMPARE(owner->operations().snapshot(), before);
        return;
    }

    if (transition == QStringLiteral("close-raw")) {
        QTRY_COMPARE(raw.size(), 1);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(owner->handle(), 0);
        return;
    }
    if (transition == QStringLiteral("destroy") || transition == QStringLiteral("replace-raw")) {
        QTRY_COMPARE(raw.size(), 1);
        if (transition == QStringLiteral("destroy")) QVERIFY(!helper);
        QCOMPARE(raw.size(), 1);
        QCOMPARE(selected.size(), 0);
        QCOMPARE(finished.size(), 0);
    } else {
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(raw.size(), restarted ? 2 : 1);
        QCOMPARE(selected.size(), 1);
    }
    QColor expected = currentLayer && !restarted ? QColor(126, 126, 126) : QColor(128, 128, 128);
    if (transition == QStringLiteral("colorful")) expected = QColor(21, 97, 201);
    if (averaging) expected = radius == 900 ? QColor(255, 0, 0) : QColor(227, 0, 28);
    auto rawExpected = expected;
    if (currentLayer && !restarted) rawExpected.setAlpha(alpha);
    if (blend < 100) {
        const auto previous = (foreground ? oldForeground : oldBackground).toQColor();
        const int sampledWeight = blend * 255 / 100;
        const int previousWeight = 255 - sampledWeight;
        const int totalAlpha = previous.alpha() * previousWeight + rawExpected.alpha() * sampledWeight;
        const auto channel = [&](int oldValue, int sampledValue) {
            return (oldValue * previous.alpha() * previousWeight + sampledValue * rawExpected.alpha() * sampledWeight + totalAlpha / 2) / totalAlpha;
        };
        rawExpected = QColor(channel(previous.red(), rawExpected.red()), channel(previous.green(), rawExpected.green()),
                             channel(previous.blue(), rawExpected.blue()), (totalAlpha + 127) / 255);
        expected = rawExpected;
        expected.setAlpha(255);
    }
    QCOMPARE(qvariant_cast<KoColor>(raw.last().first()).toQColor(), rawExpected);
    if (!selected.isEmpty()) QCOMPARE(qvariant_cast<KoColor>(selected.first().first()).toQColor(), expected);
    if (!finished.isEmpty()) QCOMPARE(qvariant_cast<KoColor>(finished.first().first()).toQColor(), rawExpected);
    QCOMPARE(resources->fgColor(), foreground && transition != QStringLiteral("preview") ? KoColor(expected, cs) : oldForeground);
    QCOMPARE(resources->bgColor(), foreground || transition == QStringLiteral("preview") ? oldBackground : KoColor(expected, cs));
    if (owner) QCOMPARE(owner->operations().snapshot(), expectedVersion);
    QCOMPARE(pixels(), expectedPixels);
    if (transition != QStringLiteral("replace-raw")) {
        QCOMPARE(lifetime->undoStack()->count(), history);
        QCOMPARE(lifetime->isModified(), modified);
    }
}

KISTEST_MAIN(KisGpuColorSamplingTest)
#include "KisGpuColorSamplingTest.moc"
