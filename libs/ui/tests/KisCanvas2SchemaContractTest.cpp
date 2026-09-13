/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <functional>
#include <type_traits>

#include "canvas/KisDecorationsManager.h"
#include "canvas/KoCanvasControllerWidget.h"
#include "canvas/kis_canvas2.h"
#include "canvas/kis_canvas_controller.h"
#include "nodes/kis_node_selection_adapter.h"

#define ASSERT_CANVAS_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvas2::method)), signature>)
#define ASSERT_CANVAS_CONTROLLER_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvasController::method)), signature>)
#define ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCanvasControllerWidget::method)), signature>)

namespace
{
class CanvasControllerWidgetProbe final : public KoCanvasControllerWidget
{
public:
    using KoCanvasControllerWidget::KoCanvasControllerWidget;

    void ensureVisibleDoc(const QRectF &, bool) override;
    void zoomIn(const KoViewTransformStillPoint &) override;
    void zoomIn() override;
    void zoomOut(const KoViewTransformStillPoint &) override;
    void zoomOut() override;
    void setPreferredCenter(const QPointF &) override;
    QPointF preferredCenter() const override;
    void resetScrollBars() override;
    QPointF currentCursorPosition() const override;
    KoZoomState zoomState() const override;
    KisCanvasState canvasState() const override;

protected:
    void updateCanvasOffsetInternal(const QPointF &) override;
    void updateCanvasWidgetSizeInternal(const QSize &, qreal) override;
    void updateCanvasZoomInternal(KoZoomMode::Mode,
                                  qreal,
                                  qreal,
                                  qreal,
                                  const std::optional<KoViewTransformStillPoint> &) override;
    void zoomToInternal(const QRect &) override;
};
} // namespace

class KisCanvas2SchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasTypeConstructionAndBaseSchemaRemainStable();
    void decorationsManagerSchemaRemainStable();
    void nodeSelectionAdapterSchemaRemainStable();
    void shapeToolAndInputSignaturesRemainStable();
    void imageRenderingAndColorStateSignaturesRemainStable();
    void notificationAndCanvasUpdateSignaturesRemainStable();
    void controllerStateAndLifetimeSignaturesRemainStable();
    void canvasControllerTypeConstructionAndEventSchemaRemainStable();
    void canvasControllerCenterZoomAndSynchronizationSchemaRemainStable();
    void canvasControllerStatePersistenceAndResolutionSchemaRemainStable();
    void canvasControllerMirrorAndRotationSchemaRemainStable();
    void canvasControllerModeAndNotificationSchemaRemainStable();
    void controllerWidgetTypeLifetimeAndStateSchemaRemainStable();
    void controllerWidgetCanvasAndToolSignaturesRemainStable();
    void controllerWidgetZoomSignaturesRemainStable();
    void controllerWidgetPanSignaturesRemainStable();
    void controllerWidgetScrollSignaturesRemainStable();
};

void KisCanvas2SchemaContractTest::canvasTypeConstructionAndBaseSchemaRemainStable()
{
    using Canvas = KisCanvas2;

    static_assert(std::is_class_v<Canvas>);
    static_assert(std::is_base_of_v<KoCanvasBase, Canvas>);
    static_assert(std::is_base_of_v<KisToolCanvas, Canvas>);
    static_assert(std::is_constructible_v<Canvas,
                                          KisCoordinatesConverter *,
                                          KoCanvasResourceProvider *,
                                          KisMainWindow *,
                                          KisView *,
                                          KoShapeControllerBase *>);
    static_assert(std::has_virtual_destructor_v<Canvas>);
    ASSERT_CANVAS_SIGNATURE(disconnectCanvasObserver, void (Canvas::*)(QObject *));
    ASSERT_CANVAS_SIGNATURE(canvasIsOpenGL, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(openGLFilterMode, KisOpenGL::FilterMode (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(gridSize, void (Canvas::*)(QPointF *, QSizeF *) const);
    ASSERT_CANVAS_SIGNATURE(snapToGrid, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(addCommand, void (Canvas::*)(KUndo2Command *));
    ASSERT_CANVAS_SIGNATURE(documentOrigin, QPoint (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(documentOffset, QPoint (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(shapeManager, KoShapeManager * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(selectedShapesProxy, KoSelectedShapesProxy * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(globalShapeManager, KoShapeManager * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(currentShapeManagerOwnerShape, KoShape * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setCurrentShapeManagerOwnerShape, void (Canvas::*)(KoShape *));
    ASSERT_CANVAS_SIGNATURE(localShapeManager, KoShapeManager * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(updateCanvas, void (Canvas::*)(const QRectF &));
    ASSERT_CANVAS_SIGNATURE(coordinatesConverter, const KisCoordinatesConverter *(Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(viewConverter, const KoViewConverter *(Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(viewConverter, KoViewConverter * (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(displayRendererInterface, KoColorDisplayRendererInterface * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(canvasWidget, QWidget * (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(canvasWidget, const QWidget *(Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(unit, KoUnit (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(toolProxy, KoToolProxy * (Canvas::*)() const);

    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::decorationsManagerSchemaRemainStable()
{
    using Manager = KisDecorationsManager;
    using SetView = void (Manager::*)(QPointer<KisView>);
    using Setup = void (Manager::*)(KisActionManager *);

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(&Manager::setView), SetView>);
    static_assert(std::is_same_v<decltype(&Manager::setup), Setup>);

    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::nodeSelectionAdapterSchemaRemainStable()
{
    using Adapter = KisNodeSelectionAdapter;

    static_assert(std::is_class_v<Adapter>);
    static_assert(std::is_constructible_v<Adapter, KisNodeManager *>);
    static_assert(std::is_destructible_v<Adapter>);
    static_assert(std::is_same_v<decltype(&Adapter::activeNode), KisNodeSP (Adapter::*)() const>);
    static_assert(std::is_same_v<decltype(&Adapter::setActiveNode), void (Adapter::*)(KisNodeSP)>);

    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::shapeToolAndInputSignaturesRemainStable()
{
    using Canvas = KisCanvas2;

    ASSERT_CANVAS_SIGNATURE(currentImage, KisImageWSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(currentSelectionForTool, KisSelectionSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(selectedNodesForTool, KisNodeList (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(attachPriorityEventFilterForTool, void (Canvas::*)(QObject *, int));
    ASSERT_CANVAS_SIGNATURE(detachPriorityEventFilterForTool, void (Canvas::*)(QObject *));
    ASSERT_CANVAS_SIGNATURE(requestStrokeEndForTool, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(requestStrokeCancellationForTool, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(blockUntilOperationsFinishedForTool, bool (Canvas::*)(KisImageSP));
    ASSERT_CANVAS_SIGNATURE(blockUntilOperationsFinishedForToolForced, void (Canvas::*)(KisImageSP));
    ASSERT_CANVAS_SIGNATURE(selectionEditableForTool, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(selectionModifierMappingSwapsCtrlAndAltForTool, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(moveSelectionCursorForTool, QCursor (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(showToolMessage, void (Canvas::*)(const QString &, const QString &));
    ASSERT_CANVAS_SIGNATURE(drawToolOutline, void (Canvas::*)(QPainter *, const KisOptimizedBrushOutline &, int));
    ASSERT_CANVAS_SIGNATURE(toolConfigNotifier, QObject * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(globalInputManager, KisInputManager * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(paintingAssistantsDecoration, KisPaintingAssistantsDecorationSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(activeToolSupportsPaintingAssistants, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(referenceImagesDecoration, KisReferenceImagesDecorationSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setInputEventFilterConnection, void (Canvas::*)(std::function<void(QObject *, bool, int)>));
    ASSERT_CANVAS_SIGNATURE(setInputCanvasWidgetChangedCallback, void (Canvas::*)(std::function<void()>));
    ASSERT_CANVAS_SIGNATURE(inputActionGroupsMaskInterface,
                            KisInputActionGroupsMaskInterface::SharedInterface (Canvas::*)());

    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::imageRenderingAndColorStateSignaturesRemainStable()
{
    using Canvas = KisCanvas2;

    ASSERT_CANVAS_SIGNATURE(image, KisImageWSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(viewManager, KisViewManager * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(imageView, QPointer<KisView> (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(addDecoration, void (Canvas::*)(KisCanvasDecorationSP));
    ASSERT_CANVAS_SIGNATURE(decoration, KisCanvasDecorationSP (Canvas::*)(const QString &) const);
    ASSERT_CANVAS_SIGNATURE(setDisplayFilter, void (Canvas::*)(QSharedPointer<KisDisplayFilter>));
    ASSERT_CANVAS_SIGNATURE(displayFilter, QSharedPointer<KisDisplayFilter> (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(displayColorConverter, KisDisplayColorConverter * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(exposureGammaCorrectionInterface,
                            KisExposureGammaCorrectionInterface * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(fetchProofingOptions, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(updateProofingState, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(proofingConfiguration, KisProofingConfigurationSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setProofingConfigUpdated, void (Canvas::*)(bool));
    ASSERT_CANVAS_SIGNATURE(proofingConfigUpdated, bool (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(setCursor, void (Canvas::*)(const QCursor &));
    ASSERT_CANVAS_SIGNATURE(frameCache, KisAnimationFrameCacheSP (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(animationState, KisCanvasAnimationState * (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(refetchDataFromImage, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(regionOfInterest, QRect (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setRenderingLimit, void (Canvas::*)(const QRect &));
    ASSERT_CANVAS_SIGNATURE(renderingLimit, QRect (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(popupPalette, KisPopupPalette * (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(colorManagementReport, QString (Canvas::*)() const);

    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::notificationAndCanvasUpdateSignaturesRemainStable()
{
    using Canvas = KisCanvas2;
    using NoArgument = void (Canvas::*)();

    ASSERT_CANVAS_SIGNATURE(sigCanvasEngineChanged, NoArgument);
    ASSERT_CANVAS_SIGNATURE(sigCanvasCacheUpdated, NoArgument);
    ASSERT_CANVAS_SIGNATURE(sigContinueResizeImage, void (Canvas::*)(qint32, qint32));
    ASSERT_CANVAS_SIGNATURE(sigCanvasStateChanged, NoArgument);
    ASSERT_CANVAS_SIGNATURE(sigViewImageSizeChanged, void (Canvas::*)(const QPointF &, const QPointF &));
    ASSERT_CANVAS_SIGNATURE(sigViewImageResolutionChanged, NoArgument);
    ASSERT_CANVAS_SIGNATURE(sigViewNodeAddedAsync, void (Canvas::*)(KisNodeSP, KisNodeAdditionFlags));
    ASSERT_CANVAS_SIGNATURE(sigViewNodeRemovedAsync, void (Canvas::*)(KisNodeSP));
    ASSERT_CANVAS_SIGNATURE(sigViewImageColorSpaceChanged, void (Canvas::*)(const KoColorSpace *));
    ASSERT_CANVAS_SIGNATURE(sigViewImageProfileChanged, void (Canvas::*)(const KoColorProfile *));
    ASSERT_CANVAS_SIGNATURE(updateCanvasRequested, void (Canvas::*)(const QRect &));
    ASSERT_CANVAS_SIGNATURE(sigRegionOfInterestChanged, void (Canvas::*)(const QRect &));
    ASSERT_CANVAS_SIGNATURE(updateCanvas, NoArgument);
    ASSERT_CANVAS_SIGNATURE(updateCanvasProjection, void (Canvas::*)(const QRectF &));
    ASSERT_CANVAS_SIGNATURE(updateCanvasDecorations, NoArgument);
    ASSERT_CANVAS_SIGNATURE(updateCanvasDecorations, void (Canvas::*)(const QRectF &));
    ASSERT_CANVAS_SIGNATURE(updateCanvasToolOutlineDoc, void (Canvas::*)(const QRectF &));
    ASSERT_CANVAS_SIGNATURE(updateCanvasToolOutlineWdg, void (Canvas::*)(const QRect &));
    ASSERT_CANVAS_SIGNATURE(updateCanvasScene, NoArgument);
    ASSERT_CANVAS_SIGNATURE(startResizingImage, NoArgument);
    ASSERT_CANVAS_SIGNATURE(finishResizingImage, void (Canvas::*)(qint32, qint32));
    ASSERT_CANVAS_SIGNATURE(rotationAngle, qreal (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(xAxisMirrored, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(yAxisMirrored, bool (Canvas::*)() const);

    QVERIFY(true);
}

// clang-format off
void KisCanvas2SchemaContractTest::controllerStateAndLifetimeSignaturesRemainStable()
{
    using Canvas = KisCanvas2;
    using NoArgument = void (Canvas::*)();

    ASSERT_CANVAS_SIGNATURE(slotSoftProofing, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotGamutCheck, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotChangeGlobalProofingConfig, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotChangeProofingConfig, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotPopupPaletteRequestedZoomChange, void (Canvas::*)(int));
    ASSERT_CANVAS_SIGNATURE(channelSelectionChanged, NoArgument);
    ASSERT_CANVAS_SIGNATURE(startUpdateInPatches, void (Canvas::*)(const QRect &));
    ASSERT_CANVAS_SIGNATURE(slotTrySwitchShapeManager, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotConfigChanged, NoArgument);
    ASSERT_CANVAS_SIGNATURE(slotScreenChanged, void (Canvas::*)(QScreen *));
    ASSERT_CANVAS_SIGNATURE(setWrapAroundViewingMode, void (Canvas::*)(bool));
    ASSERT_CANVAS_SIGNATURE(wrapAroundViewingMode, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setWrapAroundViewingModeAxis, void (Canvas::*)(WrapAroundAxis));
    ASSERT_CANVAS_SIGNATURE(wrapAroundViewingModeAxis, WrapAroundAxis (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setLodPreferredInCanvas, void (Canvas::*)(bool));
    ASSERT_CANVAS_SIGNATURE(lodPreferredInCanvas, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(initializeImage, NoArgument);
    ASSERT_CANVAS_SIGNATURE(disconnectImage, NoArgument);
    ASSERT_CANVAS_SIGNATURE(connectViewImageSignals, NoArgument);
    ASSERT_CANVAS_SIGNATURE(prepareImageForDisplay, NoArgument);
    ASSERT_CANVAS_SIGNATURE(imageUsesFloatingPointColorDepth, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(handleColorDrop, QString (Canvas::*)(QDropEvent *, KisViewManager *, const KisNodeSP &, const QPoint &, QIcon *));
    ASSERT_CANVAS_SIGNATURE(setFavoriteResourceManager, void (Canvas::*)(KisFavoriteResourceManager *));

    QVERIFY(true);
}
// clang-format on

void KisCanvas2SchemaContractTest::canvasControllerTypeConstructionAndEventSchemaRemainStable()
{
    using Controller = KisCanvasController;
    static_assert(std::is_class_v<Controller>);
    static_assert(std::is_base_of_v<KoCanvasControllerWidget, Controller>);
    static_assert(std::is_constructible_v<Controller, QPointer<KisView>, KoCanvasSupervisor *, KisKActionCollection *>);
    static_assert(std::has_virtual_destructor_v<Controller>);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(activate, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(canvasState, KisCanvasState (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(currentCursorPosition, QPointF (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(ensureVisibleDoc, void (Controller::*)(const QRectF &, bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(eventFilter, bool (Controller::*)(QObject *, QEvent *));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(keyPressEvent, void (Controller::*)(QKeyEvent *));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setCanvas, void (Controller::*)(KoCanvasBase *));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(wheelEvent, void (Controller::*)(QWheelEvent *));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomState, KoZoomState (Controller::*)() const);
    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::canvasControllerCenterZoomAndSynchronizationSchemaRemainStable()
{
    using Controller = KisCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(preferredCenter, QPointF (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setPreferredCenter, void (Controller::*)(const QPointF &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(syncOnReferencesChange, void (Controller::*)(const QRectF &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(syncOnImageResolutionChange, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(syncOnImageSizeChange, void (Controller::*)(const QPointF &, const QPointF &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(
        rotateCanvas,
        void (Controller::*)(qreal, const std::optional<KoViewTransformStillPoint> &, bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomIn, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomIn, void (Controller::*)(const KoViewTransformStillPoint &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomOut, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomOut, void (Controller::*)(const KoViewTransformStillPoint &));
    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::canvasControllerStatePersistenceAndResolutionSchemaRemainStable()
{
    using Controller = KisCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(effectiveCanvasResolutionX, qreal (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(effectiveCanvasResolutionY, qreal (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(levelOfDetailMode, bool (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(resetScrollBars, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(restoreCanvasState, void (Controller::*)(const KisPropertiesConfiguration &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(saveCanvasState, void (Controller::*)(KisPropertiesConfiguration &) const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(updateScreenResolution, void (Controller::*)(QWidget *));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(usePrintResolutionMode, bool (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(wrapAroundMode, bool (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(wrapAroundModeAxis, WrapAroundAxis (Controller::*)() const);
    QVERIFY(true);
}

void KisCanvas2SchemaContractTest::canvasControllerMirrorAndRotationSchemaRemainStable()
{
    using Controller = KisCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(beginCanvasRotation, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(endCanvasRotation, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(mirrorCanvas, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(mirrorCanvasAroundCanvas, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(mirrorCanvasAroundCursor, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(resetCanvasRotation, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(rotateCanvas, void (Controller::*)(qreal));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(rotateCanvasLeft15, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(rotateCanvasRight15, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(rotation, qreal (Controller::*)() const);
    QVERIFY(true);
}

// clang-format off
void KisCanvas2SchemaContractTest::canvasControllerModeAndNotificationSchemaRemainStable()
{
    using Controller = KisCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(documentSizeChanged, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setUsePrintResolutionMode, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(sigUsePrintResolutionModeChanged, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotSetWrapAroundModeAxis, void (Controller::*)(WrapAroundAxis));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotSetWrapAroundModeAxisH, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotSetWrapAroundModeAxisHV, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotSetWrapAroundModeAxisV, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotToggleLevelOfDetailMode, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotTogglePixelGrid, void (Controller::*)(bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(slotToggleWrapAroundMode, void (Controller::*)(bool));
    QVERIFY(true);
}
// clang-format on

void KisCanvas2SchemaContractTest::controllerWidgetTypeLifetimeAndStateSchemaRemainStable()
{
    using Widget = KoCanvasControllerWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QAbstractScrollArea, Widget>);
    static_assert(std::is_base_of_v<KoCanvasController, Widget>);
    static_assert(std::is_abstract_v<Widget>);
    static_assert(
        std::is_constructible_v<CanvasControllerWidgetProbe, KisKActionCollection *, KoCanvasSupervisor *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(canvasState, KisCanvasState (Widget::*)() const);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(priv, Widget::Private * (Widget::*)());
}

void KisCanvas2SchemaContractTest::controllerWidgetCanvasAndToolSignaturesRemainStable()
{
    using Widget = KoCanvasControllerWidget;

    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(activate, void (Widget::*)());
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(setCanvas, void (Widget::*)(KoCanvasBase *));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(canvas, KoCanvasBase * (Widget::*)() const);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(changeCanvasWidget, void (Widget::*)(QWidget *));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(setToolOptionWidgets, void (Widget::*)(const QList<QPointer<QWidget>> &));
}

void KisCanvas2SchemaContractTest::controllerWidgetZoomSignaturesRemainStable()
{
    using Widget = KoCanvasControllerWidget;

    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(zoomTo, void (Widget::*)(const QRect &));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(setZoom, void (Widget::*)(KoZoomMode::Mode, qreal));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(setZoom, void (Widget::*)(KoZoomMode::Mode, qreal, qreal, qreal));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(
        setZoom,
        void (Widget::*)(KoZoomMode::Mode, qreal, const KoViewTransformStillPoint &));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(
        setZoom,
        void (Widget::*)(KoZoomMode::Mode, qreal, qreal, qreal, const std::optional<KoViewTransformStillPoint> &));
}

void KisCanvas2SchemaContractTest::controllerWidgetPanSignaturesRemainStable()
{
    using Widget = KoCanvasControllerWidget;
    using NoArgument = void (Widget::*)();

    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(pan, void (Widget::*)(const QPoint &));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(panUp, NoArgument);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(panDown, NoArgument);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(panLeft, NoArgument);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(panRight, NoArgument);
}

void KisCanvas2SchemaContractTest::controllerWidgetScrollSignaturesRemainStable()
{
    using Widget = KoCanvasControllerWidget;

    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(scrollContentsBy, void (Widget::*)(int, int));
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(scrollBarValue, QPoint (Widget::*)() const);
    ASSERT_CANVAS_CONTROLLER_WIDGET_SIGNATURE(setScrollBarValue, void (Widget::*)(const QPoint &));
}

QTEST_GUILESS_MAIN(KisCanvas2SchemaContractTest)

#include "KisCanvas2SchemaContractTest.moc"
