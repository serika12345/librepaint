/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <functional>
#include <type_traits>

#include "canvas/kis_canvas2.h"

#define ASSERT_CANVAS_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvas2::method)), signature>)

class KisCanvas2SchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasTypeConstructionAndBaseSchemaRemainStable();
    void shapeToolAndInputSignaturesRemainStable();
    void imageRenderingAndColorStateSignaturesRemainStable();
    void notificationAndCanvasUpdateSignaturesRemainStable();
    void controllerStateAndLifetimeSignaturesRemainStable();
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

QTEST_GUILESS_MAIN(KisCanvas2SchemaContractTest)

#include "KisCanvas2SchemaContractTest.moc"
