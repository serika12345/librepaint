/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QTest>
#include <QTimer>
#include <QWidget>
#include <QIcon>
#include <QScopeGuard>
#include <QWindow>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QDockWidget>
#include <QMdiSubWindow>
#include <QOpenGLWidget>
#include <QStandardPaths>
#include <application/kis_config.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/wgpu/WgpuCanvas.h>
#include <canvas/kis_zoom_manager.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <brushengine/kis_paintop_preset.h>
#include <KisGlobalResourcesInterface.h>
#include <actions/input/KisApplicationInputActions.h>
#include <input/ui/kis_input_profile_manager.h>
#include <document/KisDocument.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <opengl/kis_opengl.h>
#include <klocalizedstring.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <KisResourceModelProvider.h>
#include <kis_transaction.h>
#include <kundo2stack.h>
#include <nodes/kis_node_manager.h>
#include <testui.h>

void compareCanvasPerformance(KisView &view, KisDocument &document);

class WgpuCanvasIntegrationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void documentProjectionReachesNativeSurface();
};

void WgpuCanvasIntegrationTest::documentProjectionReachesNativeSurface()
{
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")
        || QGuiApplication::platformName() == QStringLiteral("minimal")) {
        QSKIP("The document GPU presentation contract requires a native windowing session");
    }
    Q_INIT_RESOURCE(krita);
    QIcon::setThemeName(QStringLiteral("breeze"));
    KLocalizedString::setApplicationDomain("krita");
    KisInputProfileManager::instance()->setActions(createApplicationInputActions());
    KoResourcePaths::addAssetDir(QStringLiteral("data"), QStringLiteral(KRITA_SOURCE_DIR "/krita/data"));
    KoResourcePaths::addAssetDir(QStringLiteral("kis_actions"), QStringLiteral(KRITA_SOURCE_DIR "/krita"));
    const QString comparison = qEnvironmentVariable("LIBREPAINT_CANVAS_PERFORMANCE");
    QVERIFY(comparison.isEmpty() || comparison == QStringLiteral("qpainter")
            || comparison == QStringLiteral("opengl") || comparison == QStringLiteral("wgpu"));
    const int previousFpsLimit = KisImageConfig(true).fpsLimit();
    if (!comparison.isEmpty()) KisImageConfig(false).setFpsLimit(100);
    auto restoreFpsLimit = qScopeGuard([comparison, previousFpsLimit] {
        if (!comparison.isEmpty()) KisImageConfig(false).setFpsLimit(previousFpsLimit);
    });
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                              + QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const QVariant previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    auto restoreRenderer = qScopeGuard([&displaySettings, previousRenderer] {
        if (previousRenderer.isValid()) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
    });
    if (!comparison.isEmpty()) {
        displaySettings.setValue(QStringLiteral("OpenGLRenderer"), comparison == QStringLiteral("opengl") ? "auto" : "none");
        displaySettings.sync();
        KisConfig(false).writeEntry("LineSmoothingType", 0);
        KisConfig(false).setNewOutlineStyle(OUTLINE_NONE);
    }
    const QByteArray previousBackend = qgetenv("LIBREPAINT_CANVAS_BACKEND");
    if (comparison.isEmpty() || comparison == QStringLiteral("wgpu")) qputenv("LIBREPAINT_CANVAS_BACKEND", "wgpu");
    else qunsetenv("LIBREPAINT_CANVAS_BACKEND");
    auto restoreBackend = qScopeGuard([previousBackend] {
        if (previousBackend.isNull()) qunsetenv("LIBREPAINT_CANVAS_BACKEND");
        else qputenv("LIBREPAINT_CANVAS_BACKEND", previousBackend);
    });
    QPointer<KisDocument> document(KisPart::instance()->createDocument());
    auto releaseTestDocument = qScopeGuard([&document] { delete document.data(); });
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    const int requestedDimension = qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_DIMENSION");
    const int dimension = comparison.isEmpty() ? 64 : requestedDimension > 0 ? requestedDimension : 1024;
    QVERIFY(document->newImage(QStringLiteral("GPU projection contract"), dimension, dimension, cs, KoColor(Qt::white, cs),
                               KisConfig::RASTER_LAYER, 1, QString(), 96));
    KisNodeSP paintingLayer = document->image()->rootLayer()->firstChild();
    const int layerCount = qMax(1, qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_LAYERS"));
    KisGroupLayerSP staticGroup;
    if(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_STATIC_CACHE") && layerCount>1) {
        staticGroup = new KisGroupLayer(document->image(),QStringLiteral("Static composite cache"),255);
        document->image()->addNode(staticGroup,document->image()->rootLayer());
    }
    for (int i = 1; i < layerCount; ++i) {
        KisPaintLayerSP extra = new KisPaintLayer(document->image(), QStringLiteral("Dense layer %1").arg(i), 8, cs);
        extra->paintDevice()->fill(document->image()->bounds(), KoColor(QColor(40+i*17%180, 30+i*31%190, 20+i*43%200), cs));
        document->image()->addNode(extra, staticGroup ? staticGroup : document->image()->rootLayer());
        extra->setDirty(document->image()->bounds());
    }
    document->image()->waitForDone();
    const QString bundle = QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/RGBA_brushes.bundle");
    // Import notifications require the resource models to exist before loading.
    for (const QString &type : KisResourceLoaderRegistry::instance()->resourceTypes()) {
        KisResourceModelProvider::resourceModel(type);
    }
    QVERIFY(KisResourceLocator::instance()->addStorage(bundle, QSharedPointer<KisResourceStorage>::create(bundle)));
    QVector<QPointer<QOpenGLWidget>> detachedGlWidgets;
    auto releaseDetachedGl = qScopeGuard([&] { for(const auto &widget : detachedGlWidgets) if(widget) delete widget.data(); });
    std::unique_ptr<KisMainWindow> mainWindow(KisPart::instance()->createMainWindow());
    if (!comparison.isEmpty()) {
        mainWindow->setWindowFlag(Qt::WindowStaysOnTopHint);
        for (auto *dock : mainWindow->findChildren<QDockWidget *>()) dock->hide();
    }
    auto discardTestDocument = qScopeGuard([&document] {
        if (document) document->setModified(false);
    });
    KisView *view = mainWindow->newView(document.data());
    mainWindow->viewManager()->setCurrentView(view);
    mainWindow->viewManager()->nodeManager()->slotNonUiActivatedNode(paintingLayer);
    if(qEnvironmentVariableIntValue("LIBREPAINT_WGPU_PLAIN_UI")) {
        for(auto *widget : mainWindow->findChildren<QOpenGLWidget *>()) {
            widget->hide(); widget->setParent(nullptr); detachedGlWidgets.append(widget);
        }
    }
    QVERIFY(KisInputProfileManager::instance()->currentProfile());
    // Workspace restoration is unrelated to this fixed document/viewport.
    for (auto *timer : mainWindow->findChildren<QTimer *>()) {
        if (timer->isSingleShot() && timer->interval() == 1000) timer->stop();
    }
    mainWindow->resize(800, 600);
    mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(mainWindow->windowHandle()));
    if (!comparison.isEmpty()) {
        for (auto *dock : mainWindow->findChildren<QDockWidget *>()) dock->hide();
        mainWindow->resize(qMax(1000, qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_VIEW_WIDTH")+120),
                           qMax(800, qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_VIEW_HEIGHT")+120));
        auto *subWindow = qobject_cast<QMdiSubWindow *>(view->parentWidget());
        QVERIFY(subWindow);
        subWindow->showMaximized();
        compareCanvasPerformance(*view, *document);
        return;
    }
    auto *canvas = dynamic_cast<Krita::Canvas::WgpuCanvas *>(view->canvasBase()->canvasWidget());
    QVERIFY2(canvas, "The experimental GPU canvas was not selected for the document");
    QTRY_VERIFY_WITH_TIMEOUT(canvas->submittedFrames() > 0, 5000);
    QVERIFY2(canvas->presentationError().isEmpty(), qPrintable(canvas->presentationError()));
    QCOMPARE(canvas->capturedFrame(), canvas->composedFrame().convertToFormat(QImage::Format_RGBA8888));
    const QImage initial = canvas->capturedFrame();
    auto layer = dynamic_cast<KisPaintLayer *>(document->image()->rootLayer()->firstChild().data());
    QVERIFY(layer);
    const QRect dirty(12, 15, 9, 7);
    KisTransaction transaction(layer->paintDevice());
    layer->paintDevice()->fill(dirty, KoColor(QColor(200, 30, 50), cs));
    transaction.commit(document->image()->undoAdapter());
    layer->setDirty(dirty);
    document->image()->waitForDone();
    QTRY_VERIFY_WITH_TIMEOUT(canvas->capturedFrame() != initial, 5000);
    const QImage changed = canvas->capturedFrame();
    QVERIFY(changed != initial);
    QCOMPARE(changed, canvas->composedFrame().convertToFormat(QImage::Format_RGBA8888));

    document->undoStack()->undo();
    document->image()->waitForDone();
    QTRY_COMPARE_WITH_TIMEOUT(canvas->capturedFrame(), initial, 5000);
    document->undoStack()->redo();
    document->image()->waitForDone();
    QTRY_COMPARE_WITH_TIMEOUT(canvas->capturedFrame(), changed, 5000);

    view->zoomManager()->slotZoomIn();
    QTRY_VERIFY_WITH_TIMEOUT(canvas->capturedFrame() != changed, 5000);
    QCOMPARE(canvas->capturedFrame(), canvas->composedFrame().convertToFormat(QImage::Format_RGBA8888));
    const QSize previousSize = canvas->composedFrame().size();
    view->canvasController()->setFixedSize(view->canvasController()->size() + QSize(32, 32));
    QTRY_VERIFY_WITH_TIMEOUT(canvas->composedFrame().size() != previousSize, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(canvas->capturedFrame(), canvas->composedFrame().convertToFormat(QImage::Format_RGBA8888), 5000);
    const QImage resized = canvas->capturedFrame();
    mainWindow->hide();
    QApplication::processEvents();
    mainWindow->show();
    QTRY_COMPARE_WITH_TIMEOUT(canvas->capturedFrame(), resized, 5000);

    QWindow *nativeWindow = canvas->windowHandle();
    QVERIFY(nativeWindow);
    mainWindow->viewManager()->setCurrentView(view);
    canvas->setFocus(Qt::OtherFocusReason);
    auto preset = KisPaintOpPresetSP::create(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/autobrush_300px.kpp"));
    QVERIFY(preset->load(KisGlobalResourcesInterface::instance()));
    view->resourceProvider()->setPaintOpPreset(preset);
    view->resourceProvider()->setSize(10.0);
    view->resourceProvider()->setFGColor(KoColor(Qt::black, cs));
    const QImage beforeStroke = layer->paintDevice()->convertToQImage(cs->profile(), 0, 0, 64, 64);
    const QPoint start = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(20, 20)).toPoint();
    const QPoint end = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(44, 44)).toPoint();
    QFocusEvent focusIn(QEvent::FocusIn, Qt::OtherFocusReason);
    QCoreApplication::sendEvent(canvas, &focusIn);
    QEnterEvent enter(start, start, canvas->mapToGlobal(start));
    QCoreApplication::sendEvent(nativeWindow, &enter);
    for (auto type : {QEvent::MouseButtonPress, QEvent::MouseMove, QEvent::MouseButtonRelease}) {
        const QPoint local = type == QEvent::MouseButtonPress ? start : end;
        QMouseEvent event(type, local, canvas->mapToGlobal(local),
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(nativeWindow, &event);
    }
    document->image()->waitForDone();
    QTRY_VERIFY_WITH_TIMEOUT(layer->paintDevice()->convertToQImage(cs->profile(), 0, 0, 64, 64) != beforeStroke, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(canvas->capturedFrame(), canvas->composedFrame().convertToFormat(QImage::Format_RGBA8888), 5000);
    // Focus loss must cancel a held color-sampling modifier before tool detachment.
    QTest::keyPress(nativeWindow, Qt::Key_Control);
    QFocusEvent focusOut(QEvent::FocusOut, Qt::OtherFocusReason);
    QCoreApplication::sendEvent(canvas, &focusOut);
    QApplication::processEvents();
    document->setModified(false);
    view->closeView();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
    mainWindow->hide();
    QApplication::processEvents();
    if (document) document->setModified(false);
    mainWindow.reset();
    delete document.data();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
}

int main(int argc, char *argv[])
{
    qputenv("LANGUAGE", "en");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    qputenv("QT_LOGGING_RULES", "");
    QStandardPaths::setTestModeEnabled(true);
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
    qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
    if (!qEnvironmentVariableIsEmpty("LIBREPAINT_CANVAS_PERFORMANCE")) {
        // Use the product's surface selection before constructing QApplication.
        const auto config = KisOpenGL::selectSurfaceConfig(KisOpenGL::RendererDesktopGL,
            KisConfig::BT709_G22, KisConfig::CanvasSurfaceBitDepthMode::Depth8Bit, false);
        KisOpenGL::setDefaultSurfaceConfig(config);
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
        if(qEnvironmentVariableIntValue("LIBREPAINT_WGPU_RASTER_UI")) qunsetenv("QT_WIDGETS_RHI");
        else qputenv("QT_WIDGETS_RHI", "1");
        qputenv("QT_WIDGETS_RHI_BACKEND", "opengl");
        qputenv("QSG_RHI_BACKEND", "opengl");
#endif
    } else {
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
    }
    QApplication app(argc, argv);
    if (qEnvironmentVariableIntValue("LIBREPAINT_WGPU_FORCE_RASTER")) app.setAttribute(Qt::AA_ForceRasterWidgets, true);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    registerResources();
    WgpuCanvasIntegrationTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "WgpuCanvasIntegrationTest.moc"
