/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <KisPaintingEngineTestScope.h>
#include <QApplication>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QIcon>
#include <QMouseEvent>
#include <QPointer>
#include <QRadioButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QDomDocument>
#include <quazip.h>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoPointerEvent.h>
#include <KoSnapGuide.h>
#include <KoToolManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_preset.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuPaintDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <input/ui/kis_input_profile_manager.h>
#include <kis_canvas2.h>
#include <kis_config.h>
#include <kis_image.h>
#include <kis_node.h>
#include <commands/kis_node_property_list_command.h>
#include <kis_layer_properties_icons.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_default_bounds.h>
#include <KisRegion.h>
#include <KoColorProfile.h>
#include <KoCompositeOpRegistry.h>
#include <cstring>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <nodes/kis_node_manager.h>
#include <kis_tool.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <algorithm>
#include <memory>

namespace {
QByteArray exchangeRgbaBgra(QByteArray bytes, int pixelBytes)
{
    const int channelBytes = pixelBytes / 4;
    for (qsizetype pixel = 0; pixel < bytes.size(); pixel += pixelBytes)
        for (int byte = 0; byte < channelBytes; ++byte)
            std::swap(bytes[pixel + byte], bytes[pixel + 2 * channelBytes + byte]);
    return bytes;
}

QByteArray nativeSelectedMove(KisImageSP image, const QByteArray &source, QRect sourceRect, QPoint position,
                              KisSelectionSP selection, QPoint delta, QRect outputRect)
{
    const auto *cs = image->colorSpace();
    KisPaintDeviceSP base = new KisPaintDevice(cs);
    base->setDefaultBounds(new KisDefaultBounds(image));
    base->setX(position.x()); base->setY(position.y());
    const auto bgra = exchangeRgbaBgra(source, cs->pixelSize());
    base->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), sourceRect);
    KisPaintDeviceSP captured = new KisPaintDevice(cs);
    captured->setDefaultBounds(new KisDefaultBounds(image));
    {
        KisPainter painter(captured);
        painter.setSelection(selection);
        const auto copy = selection->selectedRect();
        painter.bitBlt(copy.topLeft(), base, copy);
    }
    base->clearSelection(selection);
    captured->setX(delta.x()); captured->setY(delta.y());
    {
        KisPainter painter(base);
        for (const auto &rect : captured->region().rects()) painter.bitBlt(rect.topLeft(), captured, rect);
    }
    QByteArray result(outputRect.width() * outputRect.height() * cs->pixelSize(), '\0');
    base->readBytes(reinterpret_cast<quint8 *>(result.data()), outputRect);
    return exchangeRgbaBgra(result, cs->pixelSize());
}
}

class KisMoveToolDocumentTest : public QObject
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
        const QString path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        m_preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(m_preset);
        m_preset->setName(QStringLiteral("Move tool document fixture"));
        m_preset->setFilename(QStringLiteral("move-tool-document-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        m_preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(m_preset));
    }
    void movementPreservesPreviewCancellationHistoryAndSaving_data()
    {
        QTest::addColumn<bool>("gpu");
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<QString>("input");
        QTest::newRow("gpu-commit") << true << false << QStringLiteral("drag");
        QTest::newRow("gpu-cancel-notify") << true << true << QStringLiteral("cancel-notify");
    }
    void movementPreservesPreviewCancellationHistoryAndSaving();
    void layerPickingForestAndMasksKeepHistoryAndKra_data();
    void layerPickingForestAndMasksKeepHistoryAndKra();
    void selectedPixelsKeepNativePreviewHistoryAndKra_data();
    void selectedPixelsKeepNativePreviewHistoryAndKra();
private:
    KisPaintOpPresetSP m_preset;
};

void KisMoveToolDocumentTest::movementPreservesPreviewCancellationHistoryAndSaving()
{
    const TestUtil::PaintingEngineScope cpuReference(false);
    QFETCH(bool, gpu);
    QFETCH(bool, cancel);
    QFETCH(QString, input);
    namespace Gpu = LibrePaint::Image::Gpu;
    const QPoint initial(7, -3), first(12, 9), delta(13, -7), moved = first + delta;
    const auto bytes = QByteArray::fromHex("112233ff445566006677888899aabbff");
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const QUuid uuid(QStringLiteral("{81828384-8586-8788-898a-8b8c8d8e8f90}"));
    KisConfig(true).disableOpenGL();
    KConfigGroup movement = KSharedConfig::openConfig()->group("KritaTransform/KisToolMove");
    movement.writeEntry("moveToolStep", 1);
    movement.writeEntry("moveToolScale", 10);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        Gpu::RasterImage raster;
        raster.size = {64, 64};
        raster.layers = {{QStringLiteral("Move target"), true, {}, {}}};
        raster.layers.first().position = initial;
        raster.layers.first().uuid = uuid;
        owner = Gpu::PaintImageSP::create(execution, raster);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Move tool")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Move tool"), 64, 64, cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        document->image()->root()->firstChild()->paintDevice()->moveTo(initial);
    }
    KisImageSP image = document->image();
    auto layer = image->root()->firstChild();
    if (!gpu) layer->setUuid(uuid);
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(bytes.constData()), QRect(first, QSize(2, 2)));
    const auto pixel = [&](QPoint point) {
        QByteArray result(16, '\0');
        layer->paintDevice()->readBytes(reinterpret_cast<quint8*>(result.data()), QRect(point, QSize(2, 2)));
        return result;
    };
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<KisDocument> continued;
    QPointer<KisTool> tool;
    QMetaObject::Connection interruption;
    const auto cleanup = qScopeGuard([&] {
        QObject::disconnect(interruption);
        if (tool) tool->requestStrokeCancellation();
        if (continued) continued->setModified(false);
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
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto toolId = QStringLiteral("KritaTransform/KisToolMove");
    try {
        KoToolManager::instance()->switchToolRequested(toolId);
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Move Tool activation failed: ") + QString::fromUtf8(error.what())));
    }
    tool = qobject_cast<KisTool*>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
    QVERIFY(tool);
    QRadioButton *selected = nullptr;
    for (QWidget *panel : tool->optionWidgets()) {
        if (auto *button = panel->findChild<QRadioButton*>(QStringLiteral("radioSelectedLayer"))) selected = button;
    }
    QVERIFY(selected);
    selected->setChecked(true);
    view->canvasBase()->snapGuide()->enableSnapping(false);
    lifetime->undoStack()->clear();
    lifetime->setModified(false);
    QSignalSpy handlesReady(tool, SIGNAL(moveInNewPosition(QPoint)));
    QVERIFY(handlesReady.isValid());
    QVERIFY(QMetaObject::invokeMethod(tool, "slotSelectionChanged"));
    QTRY_VERIFY(!handlesReady.isEmpty());
    const auto converter = view->canvasBase()->coordinatesConverter();
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    KoPointerEvent begin(&press, converter->imageToDocument(first));
    QMouseEvent motion(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    KoPointerEvent preview(&motion, converter->imageToDocument(moved));
    bool closing = false;
    if (input == QStringLiteral("close") || input == QStringLiteral("cancel-notify")) {
        interruption = connect(owner.data(), &Gpu::PaintImage::changed, tool.data(), [&] {
            if (closing) return;
            closing = true;
            if (input == QStringLiteral("cancel-notify")) {
                tool->requestStrokeCancellation();
                return;
            }
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    try {
        if (input == QStringLiteral("keyboard")) {
            for (int i = 0; i < delta.x(); ++i) QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteRight"));
            for (int i = 0; i < -delta.y(); ++i) QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteUp"));
        } else if (input == QStringLiteral("spin")) {
            QVERIFY(QMetaObject::invokeMethod(tool, "moveBySpinX", Q_ARG(int, moved.x())));
            QTRY_COMPARE(layer->paintDevice()->offset().x(), initial.x() + delta.x());
            QTRY_VERIFY(handlesReady.size() > 1);
            QVERIFY(QMetaObject::invokeMethod(tool, "moveBySpinY", Q_ARG(int, moved.y())));
            QVERIFY(QMetaObject::invokeMethod(tool, "commitChanges"));
        } else {
            tool->beginPrimaryAction(&begin);
            tool->continuePrimaryAction(&preview);
        }
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Move Tool failed: ") + QString::fromUtf8(error.what())));
    }
    if (input == QStringLiteral("close")) {
        QVERIFY(closing);
        QTRY_VERIFY(!tool);
        QVERIFY(!owner->operations().hasActiveStroke());
        return;
    }
    if (input == QStringLiteral("cancel-notify")) {
        QVERIFY(closing);
        QVERIFY(tool);
        QVERIFY(!owner->operations().hasActiveStroke());
        QCOMPARE(pixel(first), bytes);
        QCOMPARE(lifetime->undoStack()->count(), 0);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        KoPointerEvent end(&release, converter->imageToDocument(moved));
        tool->endPrimaryAction(&end);
        QCOMPARE(handlesReady.last().first().toPoint(), first);
        QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteRight"));
        QCOMPARE(pixel(first + QPoint(1, 0)), bytes);
        QCOMPARE(handlesReady.last().first().toPoint(), first + QPoint(1, 0));
        tool->requestStrokeCancellation();
        QCOMPARE(pixel(first), bytes);
        return;
    }
    QTRY_COMPARE(pixel(moved), bytes);
    QCOMPARE(pixel(first), QByteArray(16, '\0'));
    if (cancel) {
        tool->requestStrokeCancellation();
        image->waitForDone();
        QTRY_COMPARE(pixel(first), bytes);
        QCOMPARE(layer->paintDevice()->offset(), initial);
        QCOMPARE(lifetime->undoStack()->count(), 0);
        return;
    }
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    KoPointerEvent end(&release, converter->imageToDocument(moved));
    if (input == QStringLiteral("drag") || input == QStringLiteral("undo-redo")) tool->endPrimaryAction(&end);
    if (input == QStringLiteral("undo-redo")) {
        const auto previous = moved - QPoint(1, 0);
        QVERIFY(QMetaObject::invokeMethod(tool, "moveBySpinX", Q_ARG(int, previous.x())));
        QTRY_COMPARE(pixel(previous), bytes);
        QVERIFY(QMetaObject::invokeMethod(tool, "commitChanges"));
        QVERIFY(QMetaObject::invokeMethod(tool, "moveBySpinX", Q_ARG(int, moved.x())));
        QTRY_COMPARE(pixel(moved), bytes);
        QVERIFY(QMetaObject::invokeMethod(tool, "commitChanges"));
        if (gpu) {
            QCOMPARE(owner->execution().beginReadbackFrame().code, Gpu::ExecutionCode::Success);
            tool->requestUndoDuringStroke();
            QCOMPARE(owner->execution().cancelFrame().code, Gpu::ExecutionCode::Success);
            QCOMPARE(pixel(moved), bytes);
        }
        tool->requestUndoDuringStroke();
        if (gpu) QVERIFY(owner->operations().hasActiveStroke());
        QTRY_COMPARE(pixel(previous), bytes);
        if (gpu) {
            QCOMPARE(owner->execution().beginReadbackFrame().code, Gpu::ExecutionCode::Success);
            tool->requestRedoDuringStroke();
            QCOMPARE(owner->execution().cancelFrame().code, Gpu::ExecutionCode::Success);
            QCOMPARE(pixel(previous), bytes);
        }
        tool->requestRedoDuringStroke();
        QTRY_COMPARE(pixel(moved), bytes);
    }
    tool->requestStrokeEnd();
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 1);
    QCOMPARE(layer->paintDevice()->offset(), initial + delta);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(pixel(first), bytes);
    QCOMPARE(layer->paintDevice()->offset(), initial);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(pixel(moved), bytes);
    QCOMPARE(layer->paintDevice()->offset(), initial + delta);
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            output.setAutoRemove(false);
            qInfo() << "Move save artifacts retained at" << output.path();
        }
    });
    const auto path = output.filePath(QStringLiteral("moved.kra"));
    const auto saveTime = QDateTime::fromString(QStringLiteral("2001-02-03T04:05:06Z"), Qt::ISODate);
    lifetime->documentInfo()->setAboutInfo("creation-date", QStringLiteral("2000-01-02T03:04:05Z"));
    KisPropertiesConfigurationSP saveConfiguration(new KisPropertiesConfiguration);
    saveConfiguration->setProperty("documentSaveTime", saveTime);
    lifetime->setFileBatchMode(true);
    QSignalSpy saved(lifetime, &KisDocument::sigSavingFinished);
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false, saveConfiguration));
    saveConfiguration->setProperty("documentSaveTime", QStringLiteral("invalid"));
    QTRY_COMPARE(saved.size(), 1);
    saveConfiguration->setProperty("documentSaveTime", saveTime);
    QVERIFY(!lifetime->isSaving());
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY(reopened->openPath(path, KisDocument::DontAddToRecent));
    QCOMPARE(reopened->documentInfo()->aboutInfo("date"), saveTime.toString(Qt::ISODate));
    QCOMPARE(reopened->documentInfo()->aboutInfo("creation-date"), QStringLiteral("2000-01-02T03:04:05Z"));
    QuaZip archive(path);
    QVERIFY(archive.open(QuaZip::mdUnzip));
    int entries = 0;
    for (bool more = archive.goToFirstFile(); more; more = archive.goToNextFile()) {
        QuaZipFileInfo64 info;
        QVERIFY(archive.getCurrentFileInfo(&info));
        QCOMPARE(info.dateTime.date(), QDate(1980, 1, 1));
        QCOMPARE(info.dateTime.time(), QTime(0, 0));
        QCOMPARE(info.versionCreated >> 8, 3);
        ++entries;
    }
    QVERIFY(entries > 0);
    QCOMPARE(archive.getZipError(), 0);
    archive.close();
    const auto restored = reopened->image()->root()->firstChild()->paintDevice();
    QCOMPARE(restored->offset(), initial + delta);
    QByteArray result(16, '\0');
    restored->readBytes(reinterpret_cast<quint8*>(result.data()), QRect(moved, QSize(2, 2)));
    QCOMPARE(result, bytes);
    if (input == QStringLiteral("drag")) {
        QFile original(path);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const auto originalFile = original.readAll();
        original.close();
        const auto date = lifetime->documentInfo()->aboutInfo("date");
        const auto cycles = lifetime->documentInfo()->aboutInfo("editing-cycles");
        const auto history = lifetime->undoStack()->index();
        const auto modified = lifetime->isModified();
        const auto filename = lifetime->path();
        QSignalSpy failed(lifetime, &KisDocument::statusBarMessage);
        KisPropertiesConfigurationSP invalid(new KisPropertiesConfiguration);
        invalid->setProperty("documentSaveTime", QStringLiteral("invalid"));
        lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false, invalid);
        QTRY_VERIFY(!failed.isEmpty());
        QCOMPARE(saved.size(), 1);
        QVERIFY(!lifetime->isSaving());
        QCOMPARE(lifetime->documentInfo()->aboutInfo("date"), date);
        QCOMPARE(lifetime->documentInfo()->aboutInfo("editing-cycles"), cycles);
        QCOMPARE(lifetime->undoStack()->index(), history);
        QCOMPARE(lifetime->isModified(), modified);
        QCOMPARE(lifetime->path(), filename);
        QCOMPARE(pixel(moved), bytes);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), originalFile);
    }
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto png = output.filePath(QStringLiteral("moved.png"));
    QVERIFY2(lifetime->exportDocumentSync(png, QByteArrayLiteral("image/png"), configuration), qPrintable(lifetime->errorMessage()));
    QByteArray projection(64 * 64 * 4, '\0');
    image->projection()->readBytes(reinterpret_cast<quint8*>(projection.data()), QRect(0, 0, 64, 64));
    const auto decoded = QImage(png).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(decoded.size(), QSize(64, 64));
    auto rgba = projection;
    for (qsizetype i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
    for (int y = 0; y < 64; ++y)
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(decoded.constScanLine(y)), 64 * 4), rgba.mid(y * 64 * 4, 64 * 4));
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto prefix = QStringLiteral("move-") + (gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + input;
        QByteArray state;
        QDataStream stream(&state, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_15);
        stream << layer->paintDevice()->offset() << uuid << layer->exactBounds();
        const auto write = [&](QString suffix, const QByteArray &data) {
            QFile file(QDir(results).filePath(prefix + suffix));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(QStringLiteral(".bgra"), projection));
        QVERIFY(write(QStringLiteral(".state"), state));
        QFile kraInput(path), pngInput(png);
        QVERIFY(kraInput.open(QIODevice::ReadOnly));
        QVERIFY(pngInput.open(QIODevice::ReadOnly));
        QVERIFY(write(QStringLiteral(".kra"), kraInput.readAll()));
        QVERIFY(write(QStringLiteral(".png"), pngInput.readAll()));
    }
    const auto direct = QByteArray::fromHex("123456ff789abc00def01288345678ff");
    const QPoint directPoint = moved + QPoint(40, 0);
    try {
        layer->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(direct.constData()), QRect(directPoint, QSize(2, 2)));
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Direct pixel write after movement failed: ") + QString::fromUtf8(error.what())));
    }
    QCOMPARE(pixel(directPoint), direct);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(pixel(first + QPoint(40, 0)), gpu ? QByteArray(16, char(0)) : direct);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(pixel(directPoint), direct);

    if (gpu) {
        namespace Files = LibrePaint::Document::Gpu;
        auto loaded = Files::PaintDocument::openKra(owner->execution(), path);
        QVERIFY2(loaded.code == Files::FileCode::Success, qPrintable(loaded.diagnostic));
        reopened->setCurrentImage(KisImage::fromGpuImage(loaded.document->image(), image->objectName()));
    }
    auto *continuedView = window->newView(reopened.get());
    QVERIFY(continuedView);
    continued = reopened.release();
    window->viewManager()->setCurrentView(continuedView);
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool*>(KoToolManager::instance()->toolById(continuedView->canvasBase(), toolId));
    QVERIFY(tool);
    continuedView->canvasBase()->snapGuide()->enableSnapping(false);
    auto continuedLayer = continued->image()->root()->firstChild();
    QTRY_COMPARE(window->viewManager()->document(), continued.data());
    QTRY_VERIFY(continued->shapeForNode(continuedLayer));
    window->viewManager()->nodeManager()->slotUiActivatedNode(continuedLayer);
    window->viewManager()->nodeManager()->slotSetSelectedNodes({continuedLayer});
    QSignalSpy continuedHandles(tool, SIGNAL(moveInNewPosition(QPoint)));
    QVERIFY(QMetaObject::invokeMethod(tool, "slotSelectionChanged"));
    QTRY_VERIFY(!continuedHandles.isEmpty());
    const QPoint extra(1, 1), edited = moved + extra;
    QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteRight"));
    QTRY_COMPARE(continuedLayer->paintDevice()->offset(), initial + delta + QPoint(1, 0));
    QVERIFY(QMetaObject::invokeMethod(tool, "slotMoveDiscreteDown"));
    QTRY_COMPARE(continuedLayer->paintDevice()->offset(), initial + delta + extra);
    tool->requestStrokeEnd();
    continued->image()->waitForDone();
    QTRY_COMPARE(continued->undoStack()->count(), 1);
    QCOMPARE(continuedLayer->paintDevice()->offset(), initial + delta + extra);
    continuedLayer->paintDevice()->readBytes(reinterpret_cast<quint8*>(result.data()), QRect(edited, QSize(2, 2)));
    QCOMPARE(result, bytes);
    const auto editedPath = output.filePath(QStringLiteral("edited.kra"));
    QSignalSpy resaved(continued, &KisDocument::sigSavingFinished);
    saveConfiguration->setProperty("documentSaveTime", saveTime.addSecs(3600));
    QVERIFY2(continued->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false, saveConfiguration), qPrintable(continued->errorMessage()));
    QTRY_COMPARE(resaved.size(), 1);
    std::unique_ptr<KisDocument> finalDocument(KisPart::instance()->createDocument());
    finalDocument->setFileBatchMode(true);
    QVERIFY(finalDocument->openPath(editedPath, KisDocument::DontAddToRecent));
    QCOMPARE(finalDocument->documentInfo()->aboutInfo("date"), saveTime.addSecs(3600).toString(Qt::ISODate));
    QCOMPARE(finalDocument->documentInfo()->aboutInfo("creation-date"), QStringLiteral("2000-01-02T03:04:05Z"));
    const auto finalLayer = finalDocument->image()->root()->firstChild();
    QCOMPARE(finalLayer->paintDevice()->offset(), initial + delta + extra);
    finalLayer->paintDevice()->readBytes(reinterpret_cast<quint8*>(result.data()), QRect(edited, QSize(2, 2)));
    QCOMPARE(result, bytes);
    const auto editedPng = output.filePath(QStringLiteral("edited.png"));
    QVERIFY(continued->exportDocumentSync(editedPng, QByteArrayLiteral("image/png"), configuration));
    const auto editedProjection = QImage(editedPng).convertToFormat(QImage::Format_RGBA8888);
    auto expected = rgba;
    expected.fill('\0');
    for (int y = 0; y < 63; ++y)
        expected.replace(((y + 1) * 64 + 1) * 4, 63 * 4, rgba.mid(y * 64 * 4, 63 * 4));
    QCOMPARE(editedProjection.size(), QSize(64, 64));
    for (int y = 0; y < 64; ++y)
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(editedProjection.constScanLine(y)), 64 * 4), expected.mid(y * 64 * 4, 64 * 4));
    if (!results.isEmpty()) {
        const auto prefix = QStringLiteral("move-") + (gpu ? QStringLiteral("gpu-") : QStringLiteral("ordinary-")) + input;
        const auto copy = [&](QString source, QString suffix) {
            return QFile::copy(source, QDir(results).filePath(prefix + suffix));
        };
        QVERIFY(copy(editedPath, QStringLiteral(".edited.kra")));
        QVERIFY(copy(editedPng, QStringLiteral(".edited.png")));
    }
}

void KisMoveToolDocumentTest::layerPickingForestAndMasksKeepHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("painted");
    QTest::addColumn<QString>("mode");
    QTest::addColumn<bool>("groupOwner");
    for (bool words : {false, true}) for (bool painted : {false, true})
        for (const auto &mode : {"forest", "pick-layer", "pick-group", "mask", "protected-mask"})
            QTest::newRow(qPrintable(QStringLiteral("u%1-%2%3").arg(words ? 16 : 8).arg(QString::fromLatin1(mode))
                .arg(painted ? QStringLiteral("-painted") : QString())))
                << words << painted << QString::fromLatin1(mode) << false;
    for (bool words : {false, true}) for (bool painted : {false, true}) for (const auto &mode : {"forest", "mask"})
        QTest::newRow(qPrintable(QStringLiteral("u%1-%2-group-mask%3").arg(words ? 16 : 8).arg(QString::fromLatin1(mode))
            .arg(painted ? QStringLiteral("-painted") : QString())))
            << words << painted << QString::fromLatin1(mode) << true;

}

void KisMoveToolDocumentTest::layerPickingForestAndMasksKeepHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, painted);
    QFETCH(QString, mode);
    QFETCH(bool, groupOwner);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    Gpu::RasterImage raster;
    raster.size = {64, 64}; raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.colorProfile = cs->profile()->rawData();
    const int pixelBytes = Gpu::pixelByteCount(raster.format);
    const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
    const auto pixel = Gpu::rgbaBytes(color, raster.format);
    QByteArray tile(Gpu::RgbaTile::byteCount(raster.format), '\0');
    const QRect patch(24, 24, 4, 4);
    for (int y = patch.top(); y <= patch.bottom(); ++y) for (int x = patch.left(); x <= patch.right(); ++x)
        std::memcpy(tile.data() + (y * 256 + x) * pixelBytes, pixel.constData(), pixelBytes);
    Gpu::RasterLayer group{QStringLiteral("Group"), true, QUuid::createUuid(), {}};
    group.kind = Gpu::LayerKind::Group; group.position = {3, 5};
    Gpu::RasterLayer child{QStringLiteral("Child"), true, QUuid::createUuid(), {{{0, 0}, tile, raster.format}}};
    child.parentUuid = group.uuid; child.position = {7, -3};
    child.transparencyMask = Gpu::SelectionMask::fromAlphaCoverage(patch, QByteArray(16, char(128)), QRect(0, 0, 64, 64));
    Gpu::RasterLayer hidden = child;
    hidden.name = QStringLiteral("Hidden child"); hidden.uuid = QUuid::createUuid(); hidden.visible = false; hidden.transparencyMask.clear();
    Gpu::RasterLayer locked = hidden;
    locked.name = QStringLiteral("Protected child"); locked.uuid = QUuid::createUuid(); locked.visible = true; locked.userLocked = true;
    Gpu::RasterLayer other = hidden;
    other.name = QStringLiteral("Other selected layer"); other.uuid = QUuid::createUuid(); other.visible = true;
    other.parentUuid = {}; other.tiles.clear(); other.position = {-5, 2};
    if (groupOwner) {
        group.transparencyMask = child.transparencyMask;
        child.transparencyMask.clear();
    }
    const auto maskOwnerUuid = groupOwner ? group.uuid : child.uuid;
    const Gpu::LayerId maskOwnerId = groupOwner ? 1 : 2;
    raster.layers = {group, child, hidden, locked, other};
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    if (painted) {
        Gpu::Brush white; white.diameter = 2; white.rgba = {255, 255, 255, 255};
        const auto stroke = owner->beginTransparencyMaskStroke(maskOwnerId, white, 17); QVERIFY(!stroke.isNull());
        QCOMPARE(owner->appendSamples(stroke, {{{25, 25}, 1}}), Gpu::Result::Success);
        QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
        QVERIFY(owner->layerTransparencyMask(maskOwnerId)->isGpuGenerated());
    }
    const auto sourceMaskRegion = owner->layerTransparencyMask(maskOwnerId)->bounds();
    const auto originalMask = execution->readSelectionPixels(owner->layerTransparencyMask(maskOwnerId), sourceMaskRegion,
        sourceMaskRegion.width() * sourceMaskRegion.height());
    QCOMPARE(originalMask.code, Gpu::ExecutionCode::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Move layer forest"), cs));
    const auto find = [&](KisNodeSP root, const QUuid &uuid, const auto &self) -> KisNodeSP {
        if (root->uuid() == uuid) return root;
        for (auto node = root->firstChild(); node; node = node->nextSibling())
            if (auto found = self(node, uuid, self)) return found;
        return {};
    };
    const auto groupNode = find(document->image()->root(), group.uuid, find);
    const auto childNode = find(document->image()->root(), child.uuid, find);
    const auto otherNode = find(document->image()->root(), other.uuid, find);
    QVERIFY(groupNode); QVERIFY(childNode); QVERIFY(otherNode);
    const auto findMask = [](KisNodeSP parent) -> KisNodeSP {
        for (auto node = parent->firstChild(); node; node = node->nextSibling())
            if (node->inherits("KisTransparencyMask")) return node;
        return {};
    };
    const auto maskNode = findMask(groupOwner ? groupNode : childNode); QVERIFY(maskNode);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release()), continued;
    QPointer<KisTool> tool;
    const auto cleanup = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (lifetime) lifetime->setModified(false);
        if (continued) continued->setModified(false);
        window.reset(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget *>()) dock->hide();
    window->show(); window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    auto *manager = window->viewManager()->nodeManager();
    const auto active = mode == QStringLiteral("mask") ? maskNode :
                        mode == QStringLiteral("protected-mask") ? childNode : otherNode;
    manager->slotUiActivatedNode(active);
    manager->slotSetSelectedNodes(mode == QStringLiteral("forest") ? KisNodeList{groupNode, otherNode} : KisNodeList{active});
    const auto toolId = QStringLiteral("KritaTransform/KisToolMove");
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
    const auto buttonName = mode == QStringLiteral("pick-layer") ? QStringLiteral("radioFirstLayer") :
                            mode == QStringLiteral("pick-group") ? QStringLiteral("radioGroup") : QStringLiteral("radioSelectedLayer");
    QRadioButton *button = nullptr;
    for (QWidget *panel : tool->optionWidgets()) if (auto *found = panel->findChild<QRadioButton *>(buttonName)) button = found;
    QVERIFY2(button, qPrintable(buttonName)); button->setChecked(true);
    view->canvasBase()->snapGuide()->enableSnapping(false);
    if (mode == QStringLiteral("protected-mask")) {
        const auto mask = childNode->firstChild();
        auto properties = mask->sectionModelProperties();
        for (auto &property : properties) if (property.id == KisLayerPropertiesIcons::locked.id()) property.state = true;
        KisNodePropertyListCommand::setNodePropertiesAutoUndo(mask, lifetime->image(), properties);
        QVERIFY(mask->userLocked());
        lifetime->undoStack()->undo(); QVERIFY(!mask->userLocked());
        lifetime->undoStack()->redo(); QVERIFY(mask->userLocked());
        Gpu::LayerId maskedLayer = 0;
        for (const auto &layer : owner->operations().snapshot()->layers) if (layer.uuid == child.uuid) maskedLayer = layer.id;
        QVERIFY(maskedLayer);
        Gpu::Brush brush; brush.diameter = 8;
        brush.rgba = {0, 0, 0, quint16(Gpu::maximumComponent(raster.format))};
        QVERIFY(owner->beginTransparencyMaskStroke(maskedLayer, brush, 7).isNull());
        QVERIFY(owner->beginLayerTranslation(QList<Gpu::LayerTranslationTarget>{{maskedLayer, false, true}}).isNull());
    }
    const int initialHistory = owner->operations().undoCount();
    const auto before = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    const QPoint start(25, 25), delta(13, -7);
    const auto converter = view->canvasBase()->coordinatesConverter();
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent motion(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    KoPointerEvent begin(&press, converter->imageToDocument(start)), move(&motion, converter->imageToDocument(start + delta)),
        end(&release, converter->imageToDocument(start + delta));
    const bool forest = mode == QStringLiteral("forest") || mode == QStringLiteral("pick-group");
    const bool pixels = mode != QStringLiteral("mask");
    const auto byUuid = [](const Gpu::Snapshot &version, const QUuid &uuid) -> const Gpu::Layer * {
        for (const auto &layer : version->layers) if (layer.uuid == uuid) return &layer;
        return nullptr;
    };
    const auto movesPixels = [&](const QUuid &uuid) {
        return (uuid == child.uuid && pixels) || (forest && (uuid == group.uuid || uuid == hidden.uuid)) ||
            (uuid == other.uuid && mode == QStringLiteral("forest"));
    };
    const auto check = [&](const Gpu::Snapshot &version, bool moved) {
        if (version->layers.size() != before->layers.size()) return false;
        for (const auto &original : before->layers) {
            const auto layer = byUuid(version, original.uuid);
            if (!layer || layer->position != original.position + (moved && movesPixels(original.uuid) ? delta : QPoint()) ||
                layer->kind != original.kind || layer->parentUuid != original.parentUuid ||
                layer->visible != original.visible || layer->userLocked != original.userLocked) return false;
        }
        const auto maskedLayer = byUuid(version, maskOwnerUuid);
        return maskedLayer && maskedLayer->transparencyMask &&
            maskedLayer->transparencyMaskLocked == (mode == QStringLiteral("protected-mask")) && maskedLayer->transparencyMask->bounds() ==
            sourceMaskRegion.translated(moved && mode != QStringLiteral("protected-mask") ? delta : QPoint());
    };
    tool->beginPrimaryAction(&begin); QVERIFY(owner->operations().hasActiveStroke()); tool->continuePrimaryAction(&move);
    if (!check(owner->operations().preview(), true)) {
        for (const auto &candidate : owner->operations().preview()->layers)
            qDebug() << candidate.name << candidate.position << candidate.parentUuid << candidate.visible << candidate.userLocked
                     << (candidate.transparencyMask ? candidate.transparencyMask->bounds() : QRect());
        qDebug() << "original mask bounds" << byUuid(before, maskOwnerUuid)->transparencyMask->bounds()
                 << "preview is canonical" << (owner->operations().preview() == before);
    }
    QVERIFY(check(owner->operations().preview(), true));
    const auto maskPreview = byUuid(owner->operations().preview(), maskOwnerUuid)->transparencyMask;
    const auto maskRegion = sourceMaskRegion.translated(mode != QStringLiteral("protected-mask") ? delta : QPoint());
    QCOMPARE(execution->readSelectionPixels(maskPreview, maskRegion, originalMask.bytes.size()).bytes, originalMask.bytes);
    QCOMPARE(owner->operations().snapshot(), before);
    tool->requestStrokeCancellation(); QVERIFY(check(owner->operations().snapshot(), false));
    QCOMPARE(lifetime->undoStack()->count(), initialHistory); QVERIFY(!lifetime->isModified());
    tool->beginPrimaryAction(&begin); tool->continuePrimaryAction(&move); tool->endPrimaryAction(&end); tool->requestStrokeEnd();
    QVERIFY(check(owner->operations().snapshot(), true)); QCOMPARE(lifetime->undoStack()->count(), initialHistory + 1);
    const auto childId = before->layers[1].id;
    QCOMPARE(owner->readRgbaPixels(childId, patch.translated(pixels ? delta : QPoint()), 16 * pixelBytes).bytes, pixel.repeated(16));
    lifetime->undoStack()->undo(); QVERIFY(check(owner->operations().snapshot(), false));
    lifetime->undoStack()->redo(); QVERIFY(check(owner->operations().snapshot(), true));
    QTemporaryDir files; QVERIFY(files.isValid());
    const auto path = files.filePath(QStringLiteral("moved-forest.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument()); reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto next = reopened->image()->gpuImage(); QVERIFY(next);
    QVERIFY(check(next->operations().snapshot(), true));
    QCOMPARE(execution->readSelectionPixels(byUuid(next->operations().snapshot(), maskOwnerUuid)->transparencyMask,
                                            maskRegion, originalMask.bytes.size()).bytes, originalMask.bytes);
    QCOMPARE(next->readRgbaPixels(byUuid(next->operations().snapshot(), child.uuid)->id, patch.translated(pixels ? delta : QPoint()),
                                  16 * pixelBytes).bytes, pixel.repeated(16));
    const auto saved = next->operations().snapshot();
    auto *nextView = window->newView(reopened.get()); QVERIFY(nextView);
    continued = reopened.release();
    window->viewManager()->setCurrentView(nextView);
    const auto nextChild = find(continued->image()->root(), child.uuid, find);
    const auto nextGroup = find(continued->image()->root(), group.uuid, find);
    const auto nextOther = find(continued->image()->root(), other.uuid, find);
    QVERIFY(nextChild); QVERIFY(nextGroup); QVERIFY(nextOther);
    const auto nextActive = mode == QStringLiteral("mask") ? findMask(groupOwner ? nextGroup : nextChild) :
                            mode == QStringLiteral("protected-mask") ? nextChild : nextOther;
    manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(nextActive);
    manager->slotSetSelectedNodes(mode == QStringLiteral("forest") ? KisNodeList{nextGroup, nextOther} : KisNodeList{nextActive});
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(nextView->canvasBase(), toolId)); QVERIFY(tool);
    for (QWidget *panel : tool->optionWidgets())
        if (auto *found = panel->findChild<QRadioButton *>(buttonName)) found->setChecked(true);
    nextView->canvasBase()->snapGuide()->enableSnapping(false);
    const QPoint additional(-3, 5), nextStart = start + (pixels ? delta : QPoint());
    const auto nextConverter = nextView->canvasBase()->coordinatesConverter();
    KoPointerEvent nextBegin(&press, nextConverter->imageToDocument(nextStart));
    KoPointerEvent nextMove(&motion, nextConverter->imageToDocument(nextStart + additional));
    KoPointerEvent nextEnd(&release, nextConverter->imageToDocument(nextStart + additional));
    tool->beginPrimaryAction(&nextBegin); QVERIFY(next->operations().hasActiveStroke());
    tool->continuePrimaryAction(&nextMove); tool->endPrimaryAction(&nextEnd); tool->requestStrokeEnd();
    const auto edited = next->operations().snapshot();
    for (const auto &original : saved->layers) {
        const auto layer = byUuid(edited, original.uuid); QVERIFY(layer);
        QCOMPARE(layer->position, original.position + (movesPixels(original.uuid) ? additional : QPoint()));
    }
    const auto editedChild = byUuid(edited, child.uuid), savedChild = byUuid(saved, child.uuid);
    QVERIFY(editedChild); QVERIFY(savedChild);
    QCOMPARE(byUuid(edited, maskOwnerUuid)->transparencyMask->bounds(), byUuid(saved, maskOwnerUuid)->transparencyMask->bounds().translated(
        mode != QStringLiteral("protected-mask") ? additional : QPoint()));
    QCOMPARE(next->readRgbaPixels(editedChild->id, patch.translated(pixels ? delta + additional : QPoint()),
                                  16 * pixelBytes).bytes, pixel.repeated(16));
    continued->undoStack()->undo();
    QVERIFY(check(next->operations().snapshot(), true));
}

void KisMoveToolDocumentTest::selectedPixelsKeepNativePreviewHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("kind");
    QTest::addColumn<bool>("inverted");
    QTest::addColumn<bool>("cancel");
    for (bool words : {false, true}) for (int kind = 0; kind < 4; ++kind)
        for (bool inverted : {false, true}) for (bool cancel : {false, true})
            QTest::newRow(qPrintable(QStringLiteral("u%1-%2-%3-%4").arg(words ? 16 : 8).arg(kind)
                .arg(inverted ? QStringLiteral("outside") : QStringLiteral("inside"))
                .arg(cancel ? QStringLiteral("cancel") : QStringLiteral("commit")))) << words << kind << inverted << cancel;
}

void KisMoveToolDocumentTest::selectedPixelsKeepNativePreviewHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(int, kind);
    QFETCH(bool, inverted);
    QFETCH(bool, cancel);
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    constexpr int size = 64;
    const int pixelBytes = Gpu::pixelByteCount(format);
    QByteArray source(size * size * pixelBytes, '\0');
    QByteArray tile(Gpu::RgbaTile::byteCount(format), '\0');
    const quint16 wordAlphas[]{0, 1, 127, 128, 32768, 65535};
    const quint16 byteAlphas[]{0, 1, 127, 128, 254, 255};
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
        const auto maximum = words ? 65536 : 256;
        const Gpu::RgbaPixel color{quint16((4353 + x * 131 + y * 17) % maximum),
            quint16((33282 + x * 37 + y * 259) % maximum), quint16((61955 + x * 311 + y * 13) % maximum),
            (words ? wordAlphas : byteAlphas)[(x + y) % 6]};
        const auto bytes = Gpu::rgbaBytes(color, format);
        std::memcpy(source.data() + (y * size + x) * pixelBytes, bytes.constData(), pixelBytes);
        std::memcpy(tile.data() + (y * 256 + x) * pixelBytes, bytes.constData(), pixelBytes);
    }
    const QPoint initialPosition(7, -3), start(24, 24), delta(13, -7);
    Gpu::RasterImage raster;
    raster.size = {size, size}; raster.format = format; raster.colorProfile = cs->profile()->rawData();
    Gpu::RasterLayer properties{QStringLiteral("Moved selection"), true,
        QUuid(QStringLiteral("{96878889-8a8b-8c8d-8e8f-909192939495}")), {{{0, 0}, tile, format}}};
    properties.position = initialPosition;
    raster.layers = {properties};
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto layerId = owner->operations().activeLayer();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Move selected pixels"), cs));
    const QRect selected(16, 16, 24, 24), frame(-16, -16, 96, 96);
    auto image = KisImageSP(document->image());
    if (kind == 0) QVERIFY(image->setGpuSelectionRect(selected, inverted));
    else if (kind == 1) QVERIFY(image->setGpuSelectionEllipse(selected, inverted));
    else {
        QByteArray alpha(selected.width() * selected.height(), '\0');
        const quint8 values[]{0, 1, 127, 128, 254, 255};
        for (int y = 0; y < selected.height(); ++y) for (int x = 0; x < selected.width(); ++x)
            alpha[y * selected.width() + x] = char(values[(x + 2 * y) % 6]);
        const auto mask = Gpu::SelectionMask::fromAlphaCoverage(selected, alpha, QRect(0, 0, size, size));
        QVERIFY(mask);
        Gpu::SelectionMaskSP selection = mask;
        if (kind == 3) {
            Gpu::RasterImage reference; reference.size = {size, size};
            reference.layers = {{QStringLiteral("Colour selection reference"), true, {}, {}}};
            reference.layers.first().defaultPixel = {255, 255, 255, 255};
            Gpu::PaintImage producer(*execution, reference);
            Gpu::RegionFillSettings settings; settings.seed = {0, 0}; settings.threshold = 1;
            QCOMPARE(producer.selectRegion(1, settings, Gpu::SelectionCombination::Replace), Gpu::Result::Success);
            QCOMPARE(producer.combineSelectionMask(mask, Gpu::SelectionCombination::Intersect), Gpu::Result::Success);
            selection = producer.selectionMask(); QVERIFY(selection && selection->isGpuGenerated());
        }
        QVERIFY(image->setGpuSelectionMask(selection, inverted));
    }
    KisSelectionSP frozenSelection = new KisSelection(*image->globalSelection());
    if (kind == 3) {
        // The CPU reference consumes explicit fixture pixels, while the
        // application keeps the generated selection's read-only GPU view.
        QByteArray coverage(size * size, '\0');
        image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), 0, 0, size, size);
        QByteArray expectedInput(size * size, inverted ? char(255) : char(0));
        const quint8 levels[]{0, 1, 127, 128, 254, 255};
        for (int y = selected.top(); y <= selected.bottom(); ++y) for (int x = selected.left(); x <= selected.right(); ++x) {
            const auto value = levels[(x - selected.x() + 2 * (y - selected.y())) % 6];
            expectedInput[y * size + x] = char(inverted ? 255 - value : value);
        }
        QCOMPARE(coverage, expectedInput);
        frozenSelection = new KisSelection();
        frozenSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(expectedInput.constData()), 0, 0, size, size);
        frozenSelection->updateProjection();
    }
    const auto expected = nativeSelectedMove(image, source, QRect(0, 0, size, size), initialPosition,
                                             frozenSelection, delta, frame);
    const auto original = owner->readRgbaPixels(layerId, frame, frame.width() * frame.height() * pixelBytes).bytes;
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release()), continued;
    QPointer<KisTool> tool;
    const auto cleanup = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (continued) continued->setModified(false);
        if (lifetime) lifetime->setModified(false);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget *>()) dock->hide();
    window->show();
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    const auto toolId = QStringLiteral("KritaTransform/KisToolMove");
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId));
    QVERIFY(tool);
    for (QWidget *panel : tool->optionWidgets())
        if (auto *button = panel->findChild<QRadioButton *>(QStringLiteral("radioSelectedLayer"))) button->setChecked(true);
    view->canvasBase()->snapGuide()->enableSnapping(false);
    const int initialHistory = owner->operations().undoCount();
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    const auto before = owner->operations().snapshot();
    const auto converter = view->canvasBase()->coordinatesConverter();
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent motion(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    KoPointerEvent begin(&press, converter->imageToDocument(start));
    KoPointerEvent move(&motion, converter->imageToDocument(start + delta));
    KoPointerEvent end(&release, converter->imageToDocument(start + delta));
    tool->beginPrimaryAction(&begin);
    QVERIFY(owner->operations().hasActiveStroke());
    tool->continuePrimaryAction(&move);
    const auto preview = owner->operations().preview();
    const auto actual = execution->readRgbaPixels(owner->handle(), preview, frame, layerId, original.size());
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    const auto output = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!output.isEmpty()) {
        const auto path = QDir(output).filePath(QStringLiteral("selected-move-") + QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(path));
        for (const auto &entry : {qMakePair(QStringLiteral("actual.rgba"), actual.bytes), qMakePair(QStringLiteral("expected.rgba"), expected)}) {
            QFile file(QDir(path).filePath(entry.first)); QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), entry.second.size());
        }
    }
    QCOMPARE(actual.bytes, expected);
    QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(preview->layers.first().position, initialPosition);
    QCOMPARE(preview->selection, selected.translated(delta));
    if (cancel) {
        tool->requestStrokeCancellation();
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->readRgbaPixels(layerId, frame, original.size()).bytes, original);
        QCOMPARE(image->gpuSelectionRect(), selected);
        QVERIFY(!lifetime->isModified());
        QCOMPARE(lifetime->undoStack()->count(), initialHistory);
        return;
    }
    tool->endPrimaryAction(&end);
    tool->requestStrokeEnd();
    QCOMPARE(lifetime->undoStack()->count(), initialHistory + 1);
    QCOMPARE(owner->operations().historyAction(initialHistory + 1), Gpu::HistoryAction::MoveSelectionPixels);
    QCOMPARE(owner->readRgbaPixels(layerId, frame, original.size()).bytes, expected);
    lifetime->undoStack()->undo();
    QCOMPARE(owner->readRgbaPixels(layerId, frame, original.size()).bytes, original);
    lifetime->undoStack()->redo();
    QCOMPARE(owner->readRgbaPixels(layerId, frame, original.size()).bytes, expected);
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const auto path = files.filePath(QStringLiteral("moved-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    std::unique_ptr<KisDocument> reopened(KisPart::instance()->createDocument());
    reopened->setFileBatchMode(true);
    QVERIFY2(reopened->openPath(path, KisDocument::DontAddToRecent), qPrintable(reopened->errorMessage()));
    const auto next = reopened->image()->gpuImage();
    QVERIFY(next);
    QCOMPARE(next->operations().snapshot()->format, format);
    QCOMPARE(next->operations().snapshot()->layers.first().position, initialPosition);
    QCOMPARE(next->readRgbaPixels(next->operations().activeLayer(), frame, original.size()).bytes, expected);
    const QPoint additional(-3, 5);
    KisSelectionSP nextSelection = new KisSelection(*reopened->image()->globalSelection());
    const auto expectedNext = nativeSelectedMove(reopened->image(), expected, frame, initialPosition, nextSelection, additional, frame);
    auto *nextView = window->newView(reopened.get());
    QVERIFY(nextView);
    continued = reopened.release();
    window->viewManager()->setCurrentView(nextView);
    KoToolManager::instance()->switchToolRequested(toolId);
    tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(nextView->canvasBase(), toolId));
    QVERIFY(tool);
    nextView->canvasBase()->snapGuide()->enableSnapping(false);
    const auto nextConverter = nextView->canvasBase()->coordinatesConverter();
    KoPointerEvent nextBegin(&press, nextConverter->imageToDocument(start));
    KoPointerEvent nextMove(&motion, nextConverter->imageToDocument(start + additional));
    KoPointerEvent nextEnd(&release, nextConverter->imageToDocument(start + additional));
    tool->beginPrimaryAction(&nextBegin); tool->continuePrimaryAction(&nextMove); tool->endPrimaryAction(&nextEnd); tool->requestStrokeEnd();
    QCOMPARE(next->readRgbaPixels(next->operations().activeLayer(), frame, original.size()).bytes, expectedNext);
    continued->undoStack()->undo();
    QCOMPARE(next->readRgbaPixels(next->operations().activeLayer(), frame, original.size()).bytes, expected);
}

KISTEST_MAIN(KisMoveToolDocumentTest)
#include "KisMoveToolDocumentTest.moc"
