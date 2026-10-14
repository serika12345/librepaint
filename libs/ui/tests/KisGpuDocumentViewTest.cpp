/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QDockWidget>
#include <QEnterEvent>
#include <QListView>
#include <QItemSelectionModel>
#include <QPointer>
#include <QSignalSpy>
#include <QStatusBar>
#include <QScopeGuard>
#include <KLocalizedString>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <input/ui/kis_input_profile_manager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <KoToolManager.h>
#include <KoToolBase.h>
#include <KoColorSpaceRegistry.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_canvas2.h>
#include <kundo2stack.h>
#include <brushengine/kis_paintop_registry.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <exception>
#include <memory>

class KisGpuDocumentViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(bundlePath);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(
            KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(preset);
        preset->setName(QStringLiteral("GPU document view fixture"));
        preset->setFilename(QStringLiteral("gpu-document-view-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
        m_preset = preset;
    }
    void presentsEditsAndReleasesTheClosedTab();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled();
private:
    KisPaintOpPresetSP m_preset;
};

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data()
{
    QTest::addColumn<bool>("exhaustInput");
    QTest::addColumn<bool>("closeAfterError");
    QTest::newRow("paint-and-cancel") << false << false;
    QTest::newRow("input-limit") << true << false;
    QTest::newRow("close-on-input-limit") << true << true;
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled()
{
    QFETCH(bool, exhaustInput);
    QFETCH(bool, closeAfterError);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Limits limits;
    if (exhaustInput) limits.samplesPerStroke = 4;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64), limits);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU freehand input")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (documentLifetime) documentLifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (documentLifetime) {
            documentLifetime->setModified(false);
            auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
            if (close) close->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    window->statusBar()->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    QVERIFY(profiles->currentProfile());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", 0));
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
    if (closeAfterError) {
        connect(brushTool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message.isEmpty()) return;
            documentLifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    view->canvasBase()->canvasWidget()->setFocus();
    QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    const auto *converter = view->canvasBase()->coordinatesConverter();
    const auto first = converter->imageToWidget(QPointF(16, 16)).toPoint();
    const auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));
    QEnterEvent entered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mouseMove(native.data(), first);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a stroke" : qPrintable(errors.first().first().toString()));
    QTRY_COMPARE(execution.published(owner->handle()), owner->operations().preview());
    if (!exhaustInput) {
        resources->setSize(16);
        resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    }
    if (exhaustInput) {
        for (int step = 1; step <= 8 && owner->operations().hasActiveStroke(); ++step)
            QTest::mouseMove(native.data(), first + (last - first) * step / 8);
    } else {
        QTest::mouseMove(native.data(), last);
    }
    QTest::qWait(10);
    if (closeAfterError) {
        QVERIFY(!documentLifetime);
        QVERIFY(!native);
        QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QStringList reasons;
        for (const auto &notification : errors)
            if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
        QCOMPARE(reasons.size(), 1);
        return;
    }
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QVERIFY(!owner->operations().hasActiveStroke());
    if (exhaustInput) {
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!documentLifetime->isModified());
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(window->statusBar()->currentMessage(), errors.first().first().toString());
        QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
        return;
    }
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto painted = owner->operations().snapshot();
    QTRY_COMPARE(execution.published(owner->handle()), painted);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(45, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("00000000"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().undoCount(), 0);
    documentLifetime->undoStack()->redo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setSize(8);
    resources->setEraserMode(true);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    QVERIFY(owner->operations().hasActiveStroke());
    resources->setEraserMode(false);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("00000000"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto cancelled = converter->imageToWidget(QPointF(48, 16)).toPoint();
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    QVERIFY(owner->operations().hasActiveStroke());
    QTRY_COMPARE(execution.published(owner->handle()), owner->operations().preview());
    view->image()->requestStrokeCancellation();
    QVERIFY(!owner->operations().hasActiveStroke());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    pressed = false;
    QCOMPARE(owner->operations().snapshot(), painted);
    QCOMPARE(owner->operations().undoCount(), 1);
    QTRY_COMPARE(execution.published(owner->handle()), painted);
}

void KisGpuDocumentViewTest::presentsEditsAndReleasesTheClosedTab()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto handle = owner->handle();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU document view")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    KisView *view = nullptr;
    try {
        view = window->newView(document.get());
    } catch (const std::exception &exception) {
        qWarning("GPU view initialization failed: %s", exception.what());
        throw;
    }
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (documentLifetime) {
            documentLifetime->setModified(false);
            if (window) {
                auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
                if (close) close->trigger();
            }
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(view->canvasBase()->canvasWidget());
    QTRY_COMPARE(execution.published(handle), owner->operations().preview());
    auto *overview = window->findChild<QDockWidget*>(QStringLiteral("OverviewDocker"));
    QVERIFY(overview);
    QWidget *preview = nullptr;
    for (auto *child : overview->findChildren<QWidget*>())
        if (QLatin1String(child->metaObject()->className()) == QLatin1String("OverviewWidget")) preview = child;
    QVERIFY(preview);
    QVERIFY(!preview->isEnabled());
    QVERIFY(!preview->toolTip().isEmpty());
    auto *undoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    auto *redoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_redo"));
    QVERIFY(undoAction);
    QVERIFY(redoAction);
    auto *history = documentLifetime->undoStack();
    auto *historyDock = window->findChild<QDockWidget*>(QStringLiteral("History"));
    QVERIFY(historyDock);
    auto *historyView = qobject_cast<QListView*>(historyDock->widget());
    QVERIFY(historyView);
    QCOMPARE(historyView->model()->rowCount(), 1);
    QSignalSpy historyChanges(history, &KUndo2Stack::indexChanged);
    QCOMPARE(history->count(), 0);
    const auto initial = owner->operations().snapshot();
    const auto firstStroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QCOMPARE(owner->appendSamples(firstStroke, {{{24, 24}, 1}}), Gpu::Result::Success);
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), initial);
    QCOMPARE(history->count(), 0);
    window->resize(1000, 700);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("1181f1ff")), Gpu::Result::Success);
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QCOMPARE(history->count(), 1);
    QCOMPARE(history->index(), 1);
    QCOMPARE(historyView->model()->rowCount(), 2);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 1);
    QVERIFY(history->canUndo());
    QVERIFY(!history->canRedo());
    QVERIFY(!history->isClean());
    QVERIFY(history->command(0));
    QVERIFY(!history->command(0)->text().toString().isEmpty());
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(history->index(), 0);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 0);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QTRY_VERIFY(redoAction->isEnabled());
    redoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 1);
    QVERIFY(!history->isClean());
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f1ff"));
    QCOMPARE(historyChanges.size(), 3);
    historyChanges.clear();
    historyView->selectionModel()->setCurrentIndex(historyView->model()->index(0, 0), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(historyChanges.size(), 1);
    history->setIndex(1);
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{16, 16}, 1}}), Gpu::Result::Success);
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), saved);
    QCOMPARE(history->count(), 1);
    QTRY_COMPARE(execution.published(handle), saved);
    QPointer<KisView> retainedView(view);
    QPointer<QWidget> retainedCanvas(view->canvasBase()->canvasWidget());
    documentLifetime->setModified(false);
    auto *closeAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
    QVERIFY(closeAction);
    closeAction->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!retainedView);
    QTRY_VERIFY(!retainedCanvas);
    QTRY_VERIFY(!documentLifetime);
    const auto lastPresented = execution.published(handle);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("010203ff")), Gpu::Result::Success);
    QApplication::processEvents();
    QCOMPARE(execution.published(handle), lastPresented);
    window->hide();
    window.reset();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

KISTEST_MAIN(KisGpuDocumentViewTest)
#include "KisGpuDocumentViewTest.moc"
