/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
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
#include <kis_paint_device.h>
#include <kis_properties_configuration.h>
#include <metadata/KoDocumentInfo.h>
#include <nodes/kis_node_manager.h>
#include <kis_tool.h>
#include <kundo2stack.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <memory>

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
        QTest::newRow("ordinary-commit") << false << false << QStringLiteral("drag");
        QTest::newRow("ordinary-cancel") << false << true << QStringLiteral("drag");
        QTest::newRow("gpu-commit") << true << false << QStringLiteral("drag");
        QTest::newRow("gpu-cancel") << true << true << QStringLiteral("drag");
        QTest::newRow("ordinary-keyboard") << false << false << QStringLiteral("keyboard");
        QTest::newRow("gpu-keyboard") << true << false << QStringLiteral("keyboard");
        QTest::newRow("ordinary-spin") << false << false << QStringLiteral("spin");
        QTest::newRow("gpu-spin") << true << false << QStringLiteral("spin");
        QTest::newRow("ordinary-undo-redo") << false << false << QStringLiteral("undo-redo");
        QTest::newRow("gpu-undo-redo") << true << false << QStringLiteral("undo-redo");
        QTest::newRow("gpu-close-during-preview") << true << true << QStringLiteral("close");
        QTest::newRow("gpu-cancel-during-preview-notification") << true << true << QStringLiteral("cancel-notify");
    }
    void movementPreservesPreviewCancellationHistoryAndSaving();
private:
    KisPaintOpPresetSP m_preset;
};

void KisMoveToolDocumentTest::movementPreservesPreviewCancellationHistoryAndSaving()
{
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
    QTRY_COMPARE(pixel(first + QPoint(40, 0)), direct);
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

KISTEST_MAIN(KisMoveToolDocumentTest)
#include "KisMoveToolDocumentTest.moc"
