/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2013 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#define GL_GLEXT_PROTOTYPES

#include <QEvent>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QResizeEvent>
#include <QSurface>
#include <QTimer>
#include <QWindow>

#include "opengl/kis_opengl_canvas2.h"
#include "opengl/KisOpenGLCanvasRenderer.h"
#include "opengl/KisOpenGLSync.h"
#include "opengl/kis_opengl_canvas_debugger.h"

#include "canvas/kis_canvas2.h"
#include <kis_image.h>
#include <canvas/kis_canvas_resource_provider.h>
#include "application/kis_config.h"
#include "kis_config_notifier.h"
#include "kis_debug.h"
#include <application/ui/workspace/KisViewManager.h>
#include "KisRepaintDebugger.h"

#include "KisOpenGLModeProber.h"
#include "KisOpenGLContextSwitchLock.h"
#include <application/ui/orchestration/KisPlatformPluginInterfaceFactory.h>
#include <canvas/KisDisplayConfig.h>

#include "config-qt-patches-present.h"

static bool OPENGL_SUCCESS = false;

class KisOpenGLCanvas2::CanvasBridge
    : public KisOpenGLCanvasRenderer::CanvasBridge
{
    friend class KisOpenGLCanvas2;
    explicit CanvasBridge(KisOpenGLCanvas2 *canvas)
        : m_canvas(canvas)
    {}
    ~CanvasBridge() override = default;
    Q_DISABLE_COPY(CanvasBridge)
    KisOpenGLCanvas2 *m_canvas;
protected:
    KisCanvas2 *canvas() const override {
        return m_canvas->canvas();
    }
    QOpenGLContext *openglContext() const override {
        return m_canvas->context();
    }
    qreal devicePixelRatioF() const override {
        return m_canvas->devicePixelRatioF();
    }
    KisCoordinatesConverter *coordinatesConverter() const override {
        return m_canvas->coordinatesConverter();
    }
    QColor borderColor() const override {
        return m_canvas->borderColor();
    }
    GLenum internalTextureFormat() const override {
        return m_canvas->textureFormat();
    }
};

struct KisOpenGLCanvas2::Private
{
public:
    ~Private() {
        delete renderer;
    }

    boost::optional<QRect> updateRect;
#if KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH
    bool shouldSkipRenderingPass = false;
#endif
    QRect canvasImageDirtyRect;
    KisOpenGLCanvasRenderer *renderer;
    QScopedPointer<KisOpenGLSync> glSyncObject;
    KisRepaintDebugger repaintDbg;
#ifdef Q_OS_IOS
    bool applicationActive {true};
    bool foregroundRefreshPending {false};
    bool foregroundRefreshScheduled {false};
    bool resizeDeferred {false};
    bool rendererInitializationDeferred {false};
    bool rendererInitialized {false};
    bool imageRefetchRequired {false};
    bool rendererConfigChangeDeferred {false};
    bool displayFilterChangeDeferred {false};
    bool imageColorSpaceChangeDeferred {false};
    bool contextUnavailableWarningPrinted {false};
    int contextRetryDelayMs {16};
    QSize deferredResizeOldSize;
    QSize deferredImageSize;
    QSharedPointer<KisDisplayFilter> deferredDisplayFilter;
    boost::optional<KisDisplayConfig> deferredDisplayConfig;
#endif
};

KisOpenGLCanvas2::KisOpenGLCanvas2(KisCanvas2 *canvas,
                                   KisCoordinatesConverter *coordinatesConverter,
                                   QWidget *parent,
                                   KisImageWSP image,
                                   const KisDisplayConfig &displayConfig,
                                   QSharedPointer<KisDisplayFilter> displayFilter,
                                   BitDepthMode bitDepthRequest)
    : QOpenGLWidget(parent)
    , KisCanvasWidgetBase(canvas, coordinatesConverter)
    , d(new Private())
{
    setProperty("krita_skip_srgb_surface_manager_assignment", true);

    KisConfig cfg(false);
    cfg.setCanvasState("OPENGL_STARTED");

    d->renderer = new KisOpenGLCanvasRenderer(new CanvasBridge(this), image, displayConfig, displayFilter);

    connect(d->renderer->openGLImageTextures().data(),
            SIGNAL(sigShowFloatingMessage(QString, int, bool)),
            SLOT(slotShowFloatingMessage(QString, int, bool)));

    setAcceptDrops(true);
    setAutoFillBackground(false);

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground, true);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_AcceptTouchEvents, false);
#else
    setAttribute(Qt::WA_AcceptTouchEvents, true);
#endif
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);

    const bool osManagedSurfacePresent = KisPlatformPluginInterfaceFactory::instance()->surfaceColorManagedByOS();
    bool useNativeSurfaceForCanvas = osManagedSurfacePresent && cfg.enableCanvasSurfaceColorSpaceManagement();
    if (qEnvironmentVariableIsSet("KRITA_USE_NATIVE_CANVAS_SURFACE")) {
        useNativeSurfaceForCanvas = qEnvironmentVariableIntValue("KRITA_USE_NATIVE_CANVAS_SURFACE");
        qDebug() << "FPS-DEBUG: LibrePaint canvas mode is overridden:" << (useNativeSurfaceForCanvas ? "native surface" : "legacy mode") << useNativeSurfaceForCanvas << qEnvironmentVariableIsSet("KRITA_USE_NATIVE_CANVAS_SURFACE");
    }

    if (useNativeSurfaceForCanvas) {
        setAttribute(Qt::WA_NativeWindow, true);
    }

    setUpdateBehavior(PartialUpdate);

    // we should make sure the texture doesn't have alpha channel,
    // otherwise blending will not work correctly.
    if (KisOpenGLModeProber::instance()->useHDRMode()) {
        setTextureFormat(GL_RGBA16F);
    } else {
        if (bitDepthRequest == BitDepthMode::Depth10Bit) {
            if (QSurfaceFormat::defaultFormat().redBufferSize() < 10) {
                warnOpenGL <<
                    "WARNING: KisOpenGLCanvas2 was created with a 10-bit surface, "
                    "while the global surface format is still set to 8-bit. Expect "
                    "color banding to appear";
            }
            setTextureFormat(GL_RGB10_A2);
        } else {
            /**
             * When in pure OpenGL mode, the canvas surface will have alpha
             * channel. Therefore, if our canvas blending algorithm produces
             * semi-transparent pixels (and it does), then Krita window itself
             * will become transparent. Which is not good.
             *
             * In Angle mode, GL_RGB8 is not available (and the transparence effect
             * doesn't exist at all).
             */
            if (!KisOpenGL::hasOpenGLES()) {
                setTextureFormat(GL_RGB8);
            }
        }
    }

    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    connect(KisConfigNotifier::instance(), SIGNAL(pixelGridModeChanged()), SLOT(slotPixelGridModeChanged()));

    connect(canvas->viewManager()->canvasResourceProvider(), SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(slotUpdateCursorColor()));
    connect(canvas->viewManager()->canvasResourceProvider(), SIGNAL(sigPaintOpPresetChanged(KisPaintOpPresetSP)), SLOT(slotUpdateCursorColor()));

#ifdef Q_OS_IOS
    d->applicationActive = !qGuiApp ||
        QGuiApplication::applicationState() == Qt::ApplicationActive;

    if (qGuiApp) {
        connect(qGuiApp,
                &QGuiApplication::applicationStateChanged,
                this,
                [this](Qt::ApplicationState state) {
                    d->applicationActive = state == Qt::ApplicationActive;

                    if (!d->applicationActive) {
                        d->foregroundRefreshPending = true;
                        d->imageRefetchRequired = true;
                        d->contextRetryDelayMs = 16;
                        d->contextUnavailableWarningPrinted = false;
                        return;
                    }

                    d->foregroundRefreshPending = true;

                    // Qt may emit ApplicationActive before the iOS platform
                    // backing surface is usable again. Defer all FBO work by
                    // at least one event-loop turn and verify makeCurrent()
                    // before replaying it.
                    scheduleIOSForegroundRefresh();
                });
    }
#endif

    slotConfigChanged();
    slotPixelGridModeChanged();
    cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
}

KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
    /**
     * Since we delete openGL resources, we should make sure the
     * context is initialized properly before they are deleted.
     * Otherwise resources from some other (current) context may be
     * deleted due to resource id aliasing.
     *
     * The main symptom of resources being deleted from wrong context,
     * the canvas being locked/backened-out after some other document
     * is closed.
     */

    makeCurrent();

    delete d;

    doneCurrent();
}

void KisOpenGLCanvas2::setDisplayFilter(QSharedPointer<KisDisplayFilter> displayFilter)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->deferredDisplayFilter = displayFilter;
        d->displayFilterChangeDeferred = true;
        d->imageRefetchRequired = true;
        return;
    }
#endif

    KisOpenGLContextSwitchLockSkipOnQt5 contextLock(this);
    d->renderer->setDisplayFilter(displayFilter);
}

void KisOpenGLCanvas2::notifyImageColorSpaceChanged(const KoColorSpace *cs)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->imageColorSpaceChangeDeferred = true;
        d->imageRefetchRequired = true;
        return;
    }
#endif

    KisOpenGLContextSwitchLockSkipOnQt5 contextLock(this);
    d->renderer->notifyImageColorSpaceChanged(cs);
}

void KisOpenGLCanvas2::setWrapAroundViewingMode(bool value)
{
    d->renderer->setWrapAroundViewingMode(value);
    update();
}

bool KisOpenGLCanvas2::wrapAroundViewingMode() const
{
    return d->renderer->wrapAroundViewingMode();
}

void KisOpenGLCanvas2::setWrapAroundViewingModeAxis(WrapAroundAxis value)
{
    d->renderer->setWrapAroundViewingModeAxis(value);
    update();
}

WrapAroundAxis KisOpenGLCanvas2::wrapAroundViewingModeAxis() const
{
    return d->renderer->wrapAroundViewingModeAxis();
}

bool KisOpenGLCanvas2::event(QEvent *e)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        switch (e->type()) {
        case QEvent::Show:
        case QEvent::DevicePixelRatioChange:
            // QOpenGLWidget::event() may initialize or recreate FBOs for
            // these events before the virtual show/resize handlers run.
            // Preserve QWidget's non-GL state transition and fold the GL
            // portion into the verified foreground resize replay.
            d->resizeDeferred = true;
            d->foregroundRefreshPending = true;
            d->imageRefetchRequired = true;
            return QWidget::event(e);
        default:
            break;
        }
    }
#endif

    return QOpenGLWidget::event(e);
}

void KisOpenGLCanvas2::initializeGL()
{
#ifdef Q_OS_IOS
    if (!iosApplicationIsActive()) {
        d->rendererInitializationDeferred = true;
        d->foregroundRefreshPending = true;
        return;
    }

    // updateConfig() normally runs before the first initializeGL(). Preserve
    // that ordering when the canvas was constructed while iOS was inactive.
    applyIOSDeferredRendererChanges();
#endif

    d->renderer->initializeGL();
    KisOpenGLSync::init(context());

#ifdef Q_OS_IOS
    d->rendererInitialized = true;
    d->rendererInitializationDeferred = false;
    // initializeGL() uploads the complete current projection, including any
    // CPU-side changes whose individual uploads were skipped while inactive.
    d->imageRefetchRequired = false;
#endif
}

void KisOpenGLCanvas2::resizeGL(int width, int height)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->resizeDeferred = true;
        d->foregroundRefreshPending = true;
        return;
    }
#endif

    d->renderer->resizeGL(width, height);
    d->canvasImageDirtyRect = QRect(0, 0, width, height);
}

void KisOpenGLCanvas2::paintGL()
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->foregroundRefreshPending = true;
        return;
    }
#endif

#if KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH
    if (d->shouldSkipRenderingPass) {
        return;
    }
#endif

    const QRect updateRect = d->updateRect ? *d->updateRect : QRect();

    if (!OPENGL_SUCCESS) {
        KisConfig cfg(false);
        cfg.writeEntry("canvasState", "OPENGL_PAINT_STARTED");
    }

    KisOpenglCanvasDebugger::instance()->notifyPaintRequested();
    QRect canvasImageDirtyRect = d->canvasImageDirtyRect & rect();
    d->canvasImageDirtyRect = QRect();
    d->renderer->paintCanvasOnly(canvasImageDirtyRect, updateRect);
    {
        QPainter gc(this);
        if (!updateRect.isEmpty()) {
            gc.setClipRect(updateRect);
        }

        QRect decorationsBoundingRect = coordinatesConverter()->imageRectInWidgetPixels().toAlignedRect();

        if (!updateRect.isEmpty()) {
            decorationsBoundingRect &= updateRect;
        }

        drawDecorations(gc, decorationsBoundingRect);
    }

    d->repaintDbg.paint(this, updateRect.isEmpty() ? rect() : updateRect);

    // We create the glFenceSync object here instead of in KisOpenGLRenderer,
    // because the glFenceSync object should be created after all render
    // commands in a frame, not just the OpenGL canvas itself. Putting it
    // outside of KisOpenGLRenderer allows the canvas widget to do extra
    // rendering, which a QtQuick2-based canvas will need.
    d->glSyncObject.reset(new KisOpenGLSync());

    if (!OPENGL_SUCCESS) {
        KisConfig cfg(false);
        cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
        OPENGL_SUCCESS = true;
    }
}

void KisOpenGLCanvas2::paintEvent(QPaintEvent *e)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->foregroundRefreshPending = true;
        e->accept();
        return;
    }
#endif

    KIS_SAFE_ASSERT_RECOVER_RETURN(!d->updateRect);

    if (qFuzzyCompare(devicePixelRatioF(), qRound(devicePixelRatioF()))) {
        /**
         * Enable partial updates **only** for the case when we have
         * integer scaling. There is a bug in Qt that causes artifacts
         * otherwise:
         *
         * See https://bugs.kde.org/show_bug.cgi?id=441216
         */
        d->updateRect = e->rect();
    } else {
        d->updateRect = this->rect();
    }

#if KRITA_QT_HAS_UPDATE_COMPRESSION_PATCH
    /**
     * When using Qt with a proper update paint event compression, then we don't
     * need to implement our own one in KisCanvas2, instead we should just skip
     * frames in paintEvent(), when the previous frame hasn't completed yet.
     */
    if (isBusy()) {
        //qWarning() << "WARNING: paint event delivered with the canvas non-ready, rescheduling...";
        d->shouldSkipRenderingPass = true;
        QOpenGLWidget::paintEvent(e);
        d->shouldSkipRenderingPass = false;
        QTimer::singleShot(0, this,
            [this, updateRect = *d->updateRect] () {
                if (updateRect.isEmpty()) {
                    this->update();
                } else {
                    this->update(updateRect);
                }
            });
    } else
#endif
    {
        QOpenGLWidget::paintEvent(e);
    }

    d->updateRect = boost::none;
}

void KisOpenGLCanvas2::resizeEvent(QResizeEvent *e)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        if (!d->resizeDeferred) {
            d->deferredResizeOldSize = e->oldSize();
        }
        d->resizeDeferred = true;
        d->foregroundRefreshPending = true;
        e->accept();
        return;
    }
#endif

    QOpenGLWidget::resizeEvent(e);
}

#ifdef Q_OS_IOS
bool KisOpenGLCanvas2::iosApplicationIsActive() const
{
    return d->applicationActive &&
        (!qGuiApp || QGuiApplication::applicationState() == Qt::ApplicationActive);
}

bool KisOpenGLCanvas2::iosOpenGLWorkIsAllowed() const
{
    return iosApplicationIsActive() && !d->foregroundRefreshPending;
}

void KisOpenGLCanvas2::scheduleIOSForegroundRefresh(int delayMs)
{
    if (!iosApplicationIsActive() || d->foregroundRefreshScheduled) {
        return;
    }

    d->foregroundRefreshScheduled = true;
    QTimer::singleShot(delayMs, this, [this] {
        d->foregroundRefreshScheduled = false;
        replayIOSDeferredOpenGLWork();
    });
}

void KisOpenGLCanvas2::applyIOSDeferredRendererChanges()
{
    if (d->rendererConfigChangeDeferred) {
        d->rendererConfigChangeDeferred = false;
        d->renderer->updateConfig();
    }

    if (d->deferredDisplayConfig) {
        const KisDisplayConfig config = *d->deferredDisplayConfig;
        d->deferredDisplayConfig = boost::none;
        d->renderer->setDisplayConfig(config);
    }

    if (d->displayFilterChangeDeferred) {
        const QSharedPointer<KisDisplayFilter> displayFilter = d->deferredDisplayFilter;
        d->deferredDisplayFilter.clear();
        d->displayFilterChangeDeferred = false;
        d->renderer->setDisplayFilter(displayFilter);
    }

    if (d->imageColorSpaceChangeDeferred) {
        d->imageColorSpaceChangeDeferred = false;
        const KisImageSP image = canvas()->image();
        if (image) {
            d->renderer->notifyImageColorSpaceChanged(image->colorSpace());
        }
    }

    if (d->deferredImageSize.isValid()) {
        const QSize imageSize = d->deferredImageSize;
        d->deferredImageSize = QSize();
        d->renderer->finishResizingImage(imageSize.width(), imageSize.height());
    }
}

void KisOpenGLCanvas2::replayIOSDeferredOpenGLWork()
{
    if (!iosApplicationIsActive()) {
        return;
    }

    // Keep all normal paint and projection paths closed until the context
    // preflight and any deferred resize have finished.
    d->foregroundRefreshPending = true;

    QOpenGLContext * const widgetContext = context();
    QOpenGLContext * const previousContext = QOpenGLContext::currentContext();
    QSurface * const previousSurface = previousContext ? previousContext->surface() : nullptr;

    if (widgetContext) {
        if (!widgetContext->isValid()) {
            if (!d->contextUnavailableWarningPrinted) {
                qWarning() << "iPadOS canvas foreground restore is waiting for a valid OpenGL context";
                d->contextUnavailableWarningPrinted = true;
            }

            const int retryDelayMs = d->contextRetryDelayMs;
            d->contextRetryDelayMs = qMin(d->contextRetryDelayMs * 2, 1000);
            scheduleIOSForegroundRefresh(retryDelayMs);
            return;
        }

        makeCurrent();
        if (QOpenGLContext::currentContext() != widgetContext) {
            if (previousContext && previousSurface &&
                QOpenGLContext::currentContext() != previousContext) {
                previousContext->makeCurrent(previousSurface);
            }

            if (!d->contextUnavailableWarningPrinted) {
                qWarning() << "iPadOS canvas foreground restore is waiting for its OpenGL surface";
                d->contextUnavailableWarningPrinted = true;
            }

            const int retryDelayMs = d->contextRetryDelayMs;
            d->contextRetryDelayMs = qMin(d->contextRetryDelayMs * 2, 1000);
            scheduleIOSForegroundRefresh(retryDelayMs);
            return;
        }
    } else if (d->rendererInitialized) {
        if (!d->contextUnavailableWarningPrinted) {
            qWarning() << "iPadOS canvas foreground restore is waiting for its OpenGL context";
            d->contextUnavailableWarningPrinted = true;
        }

        const int retryDelayMs = d->contextRetryDelayMs;
        d->contextRetryDelayMs = qMin(d->contextRetryDelayMs * 2, 1000);
        scheduleIOSForegroundRefresh(retryDelayMs);
        return;
    }

    d->contextRetryDelayMs = 16;
    d->contextUnavailableWarningPrinted = false;
    d->foregroundRefreshPending = false;

    if (widgetContext) {
        // QOpenGLWidget may have called initializeGL() while the application
        // was transitioning out of Active. Its internal initialization has
        // completed, so finish Krita's renderer initialization explicitly now
        // that the context is current and legal to use.
        if (d->rendererInitializationDeferred && !d->rendererInitialized) {
            applyIOSDeferredRendererChanges();
            d->renderer->initializeGL();
            KisOpenGLSync::init(widgetContext);
            d->rendererInitialized = true;
            d->rendererInitializationDeferred = false;
        } else {
            applyIOSDeferredRendererChanges();
        }
    }

    if (d->resizeDeferred) {
        const QSize oldSize = d->deferredResizeOldSize;
        d->deferredResizeOldSize = QSize();
        d->resizeDeferred = false;

        QResizeEvent deferredEvent(size(), oldSize);
        QOpenGLWidget::resizeEvent(&deferredEvent);
    }

    if (widgetContext && previousContext != widgetContext) {
        if (previousContext && previousSurface) {
            previousContext->makeCurrent(previousSurface);
        } else if (QOpenGLContext::currentContext() == widgetContext) {
            doneCurrent();
        }
    }

    d->canvasImageDirtyRect = rect();

    if (d->imageRefetchRequired) {
        d->imageRefetchRequired = false;
        canvas()->refetchDataFromImage();
    }

    canvas()->updateCanvas();
    update();
}
#endif

void KisOpenGLCanvas2::paintToolOutline(const KisOptimizedBrushOutline &path, int thickness)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        return;
    }
#endif

    /**
     * paintToolOutline() is called from drawDecorations(), which has clipping
     * set only for QPainter-based painting; here we paint in native mode, so we
     * should care about clipping manually
     *
     * `d->updateRect` might be empty in case the fractional DPI workaround
     * is active.
     */
    const QRect updateRect = d->updateRect ? *d->updateRect : QRect();

    d->renderer->paintToolOutline(path, updateRect, thickness);
}

bool KisOpenGLCanvas2::isBusy() const
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        return false;
    }
#endif

    const bool isBusyStatus = d->glSyncObject && !d->glSyncObject->isSignaled();
    KisOpenglCanvasDebugger::instance()->notifySyncStatus(isBusyStatus);
    return isBusyStatus;
}

void KisOpenGLCanvas2::setLodResetInProgress(bool value)
{
    d->renderer->setLodResetInProgress(value);
}

void KisOpenGLCanvas2::slotConfigChanged()
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->rendererConfigChangeDeferred = true;
        notifyConfigChanged();
        return;
    }
#endif

    d->renderer->updateConfig();

    notifyConfigChanged();
}

void KisOpenGLCanvas2::slotPixelGridModeChanged()
{
    d->renderer->updatePixelGridMode();

    update();
}

void KisOpenGLCanvas2::slotUpdateCursorColor()
{
    d->renderer->updateCursorColor();
}

void KisOpenGLCanvas2::slotShowFloatingMessage(const QString &message, int timeout, bool priority)
{
    canvas()->imageView()->showFloatingMessage(message, QIcon(), timeout, priority ? KisFloatingMessage::High : KisFloatingMessage::Medium);
}

QVariant KisOpenGLCanvas2::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return processInputMethodQuery(query);
}

void KisOpenGLCanvas2::inputMethodEvent(QInputMethodEvent *event)
{
    processInputMethodEvent(event);
}

void KisOpenGLCanvas2::focusInEvent(QFocusEvent *event)
{
    processFocusInEvent(event);
}

void KisOpenGLCanvas2::focusOutEvent(QFocusEvent *event)
{
    processFocusOutEvent(event);
}

void KisOpenGLCanvas2::hideEvent(QHideEvent *e)
{
    QOpenGLWidget::hideEvent(e);
    notifyDecorationsWindowMinimized(true);
}

void KisOpenGLCanvas2::showEvent(QShowEvent *e)
{
    QOpenGLWidget::showEvent(e);
    notifyDecorationsWindowMinimized(false);
}

void KisOpenGLCanvas2::setDisplayConfig(const KisDisplayConfig &config)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->deferredDisplayConfig = config;
        d->imageRefetchRequired = true;
        return;
    }
#endif

    KisOpenGLContextSwitchLockSkipOnQt5 contextLock(this);
    d->renderer->setDisplayConfig(config);
}

void KisOpenGLCanvas2::channelSelectionChanged(const QBitArray &channelFlags)
{
    d->renderer->channelSelectionChanged(channelFlags);
}


void KisOpenGLCanvas2::finishResizingImage(qint32 w, qint32 h)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        d->deferredImageSize = QSize(w, h);
        d->imageRefetchRequired = true;
        return;
    }
#endif

    KisOpenGLContextSwitchLockSkipOnQt5 contextLock(this);
    d->renderer->finishResizingImage(w, h);
}

KisUpdateInfoSP KisOpenGLCanvas2::startUpdateCanvasProjection(const QRect & rc)
{
    return d->renderer->startUpdateCanvasProjection(rc);
}


QRect KisOpenGLCanvas2::updateCanvasProjection(KisUpdateInfoSP info)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        // The projection itself remains authoritative. Drop only this GPU
        // upload and rebuild all texture data once foreground GL is legal.
        d->imageRefetchRequired = true;
        return rect();
    }
#endif

    return d->renderer->updateCanvasProjection(info);
}

QVector<QRect> KisOpenGLCanvas2::updateCanvasProjection(const QVector<KisUpdateInfoSP> &infoObjects)
{
#ifdef Q_OS_IOS
    if (!iosOpenGLWorkIsAllowed()) {
        // Do not construct the context-switch lock while inactive; it calls
        // QOpenGLWidget::makeCurrent() before dispatching individual updates.
        return KisCanvasWidgetBase::updateCanvasProjection(infoObjects);
    }
#endif

    KisOpenGLContextSwitchLockSkipOnQt5 contextLock(this);
    return KisCanvasWidgetBase::updateCanvasProjection(infoObjects);
}

void KisOpenGLCanvas2::updateCanvasImage(const QRect &imageUpdateRect)
{
    d->canvasImageDirtyRect |= imageUpdateRect;
    update(imageUpdateRect);
}

void KisOpenGLCanvas2::updateCanvasDecorations(const QRect &decoUpdateRect)
{
    update(decoUpdateRect);
}
bool KisOpenGLCanvas2::callFocusNextPrevChild(bool next)
{
    return focusNextPrevChild(next);
}

KisOpenGLImageTexturesSP KisOpenGLCanvas2::openGLImageTextures() const
{
    return d->renderer->openGLImageTextures();
}

KisOpenGLCanvas2::BitDepthMode KisOpenGLCanvas2::currentBitDepthMode() const 
{
    return
        textureFormat() == GL_RGB10_A2 &&
        format().redBufferSize() == 10 &&
        format().greenBufferSize() == 10 &&
        format().blueBufferSize() == 10 ?
            BitDepthMode::Depth10Bit :
            BitDepthMode::Depth8Bit;
}

QString KisOpenGLCanvas2::currentBitDepthUserReport() const {
    QString report;
    QDebug str(&report);

    str << "Texture Format: " << Qt::hex << Qt::showbase << textureFormat() << Qt::reset;

    switch (textureFormat()) {
        case GL_RGB10_A2:
            str << " (" << "GL_RGB10_A2" << ")";
            break;
// not available in openGLES
#ifdef GL_RGB10
            case GL_RGB10:
            str << " (" << "GL_RGB10" << ")";
            break;
#endif /* GL_RGB10 */
// not available in openGLES
#ifdef GL_RGB12
        case GL_RGB12:
            str << " (" << "GL_RGB12" << ")";
            break;
#endif /* GL_RGB12 */
// not available in openGLES
#ifdef GL_RGBA16
        case GL_RGBA16:
            str << " (" << "GL_RGBA16" << ")";
            break;
#endif /* GL_RGBA16 */
// not available in openGLES
#ifdef GL_RGB16
        case GL_RGB16:
            str << " (" << "GL_RGB16" << ")";
            break;
#endif /* GL_RGB16 */
        case GL_RGBA16F:
            str << " (" << "GL_RGBA16F" << ")";
            break;
        case GL_RGB8:
            str << " (" << "GL_RGB8" << ")";
            break;
        case GL_RGBA8:
            str << " (" << "GL_RGBA8" << ")";
            break;
        default:
            str << " (" << "<unknown>" << ")";
            break;
    }
    str << Qt::endl;

    str << "FBO Buffer Size: "
        << "R: " << format().redBufferSize() << " "
        << "G: " << format().greenBufferSize() << " "
        << "B: " << format().blueBufferSize() << " "
        << "A: " << format().alphaBufferSize() << Qt::endl;

    QWindow *win = windowHandle();
    if (win) {
        str << "Window Buffer Size: "
        << "R: " << win->format().redBufferSize() << " "
        << "G: " << win->format().greenBufferSize() << " "
        << "B: " << win->format().blueBufferSize() << " "
        << "A: " << win->format().alphaBufferSize() << Qt::endl;
    }

    if (win) {
        str << "Global Buffer Size: "
        << "R: " << QSurfaceFormat::defaultFormat().redBufferSize() << " "
        << "G: " << QSurfaceFormat::defaultFormat().greenBufferSize() << " "
        << "B: " << QSurfaceFormat::defaultFormat().blueBufferSize() << " "
        << "A: " << QSurfaceFormat::defaultFormat().alphaBufferSize() << Qt::endl;
    }

    return report;
}
