/*
 *  kis_tool_freehand.cc - part of Krita
 *
 *  SPDX-FileCopyrightText: 2003-2007 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2004 Bart Coppens <kde@bartcoppens.be>
 *  SPDX-FileCopyrightText: 2007, 2008, 2010 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2009 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_freehand.h"
#include <klocalizedstring.h>
#include <QPainter>
#include <QRect>
#include <QThreadPool>
#include <QThread>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QtConcurrentRun>
#include <kis_coordinates_converter.h>
#include "canvas/kis_canvas_widget_base.h"
#include "canvas/KisDisplayConfig.h"
#include <color/kis_display_color_transform.h>


#include <functional>
#include <cmath>
#include <kis_icon.h>
#include <KoCanvasBase.h>
#include <KoCanvasResourcesIds.h>
#include <KoPointerEvent.h>
#include <KoViewConverter.h>
#include <KoCanvasController.h>

//pop up palette
#include <canvas/kis_canvas_resource_provider.h>

// Krita/image
#include <kis_image.h>
#include <kis_figure_painting_stroke.h>
#include <kis_painter.h>
#include <brushengine/kis_paintop.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/KisOptimizedBrushOutline.h>


// Krita/ui
#include "KisPerStrokeRandomSource.h"
#include "canvas/kis_abstract_perspective_grid.h"
#include "application/kis_config.h"
#include "kis_assert.h"
#include "kis_config_notifier.h"
#include "kis_global.h"
#include <kis_layer.h>
#include "kis_image_config.h"
#include "canvas/kis_canvas2.h"
#include "canvas/kis_display_color_converter.h"
#include "KisQuickShapePreview.h"
#include "kis_cursor.h"
#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_painting_assistants_decoration.h>
#include <qcursor.h>
#include <QtGlobal>
#include <qguiapplication.h>
#include <qhashfunctions.h>
#include <qnumeric.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qpointer.h>
#include <qset.h>
#include "kis_painting_information_builder_adapters.h"
#include "kis_quick_shape.h"
#include "kis_random_source.h"
#include "kis_tool.h"
#include "kis_tool_freehand_helper.h"
#include "kis_tool_paint.h"
#include "kis_tool_utils.h"
#include "kis_types.h"
#include "kundo2magicstring.h"

using namespace std::placeholders; // For _1 placeholder

namespace {

// How long the pointer must stay inside the hold slop before the gesture is
// treated as a request for a snapped shape.
const int kQuickShapeHoldDelayMs = 500;

// Pointer movement that restarts the hold delay, measured in canvas widget
// pixels and converted to image pixels for the current zoom.
const qreal kQuickShapeHoldSlopWidgetPixels = 3.0;

}


KisToolFreehand::KisToolFreehand(KoCanvasBase * canvas, const QCursor & cursor,
                                 const KUndo2MagicString &transactionText, bool useSavedSmoothing)
    : KisToolPaint(canvas, cursor),
      m_brushResizeCompressor(200, std::bind(&KisToolFreehand::slotDoResizeBrush, this, _1)),
      m_quickShapeTracker(new KisQuickShapeTracker()),
      m_quickShapePreview(new KisQuickShapePreview())
{

    setSupportOutline(true);

    m_quickShapeHoldTimer.setSingleShot(true);
    connect(&m_quickShapeHoldTimer, SIGNAL(timeout()), SLOT(slotQuickShapeHoldTimeout()));
    connect(&m_quickShapeSampleWatcher, &QFutureWatcher<std::tuple<QImage, QLineF, qreal>>::finished,
            this, &KisToolFreehand::finishQuickShapePreviewPreparation);

    updateMaskSyntheticEventsFromTouch();
    connect(KisConfigNotifier::instance(), SIGNAL(touchPaintingChanged()),
            SLOT(updateMaskSyntheticEventsFromTouch()));

    m_infoBuilder = new KisToolFreehandPaintingInformationBuilder(this);
    m_helper = new KisToolFreehandHelper(m_infoBuilder, canvas->resourceManager(), transactionText,
                                         new KisSmoothingOptions(useSavedSmoothing));

    connect(m_helper, SIGNAL(requestExplicitUpdateOutline()), SLOT(explicitUpdateOutline()));

    connect(qobject_cast<KisCanvas2*>(canvas)->viewManager(), SIGNAL(brushOutlineToggled()), SLOT(explicitUpdateOutline()));

    KisCanvasResourceProvider *provider = qobject_cast<KisCanvas2*>(canvas)->viewManager()->canvasResourceProvider();

    connect(provider, SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(explicitUpdateOutline()));
    connect(provider, SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(resetCursorStyle()));
    connect(provider, SIGNAL(sigPaintOpPresetChanged(KisPaintOpPresetSP)), SLOT(explicitUpdateOutline()));
    connect(provider, SIGNAL(sigPaintOpPresetChanged(KisPaintOpPresetSP)), SLOT(resetCursorStyle()));
}

KisToolFreehand::~KisToolFreehand()
{
    resetQuickShapeTracking();
    delete m_helper;
    delete m_infoBuilder;
}

void KisToolFreehand::mouseMoveEvent(KoPointerEvent *event)
{
    KisToolPaint::mouseMoveEvent(event);
    m_helper->cursorMoved(convertToPixelCoord(event));
}

KisSmoothingOptionsSP KisToolFreehand::smoothingOptions() const
{
    return m_helper->smoothingOptions();
}

void KisToolFreehand::resetCursorStyle()
{
    KisConfig cfg(true);

    bool useSeparateEraserCursor = cfg.separateEraserCursor() && isEraser();

    switch (useSeparateEraserCursor ? cfg.eraserCursorStyle() : cfg.newCursorStyle()) {
    case CURSOR_STYLE_NO_CURSOR:
        useCursor(KisCursor::blankCursor());
        break;
    case CURSOR_STYLE_POINTER:
        useCursor(KisCursor::arrowCursor());
        break;
    case CURSOR_STYLE_SMALL_ROUND:
        useCursor(KisCursor::roundCursor());
        break;
    case CURSOR_STYLE_CROSSHAIR:
        useCursor(KisCursor::crossCursor());
        break;
    case CURSOR_STYLE_TRIANGLE_RIGHTHANDED:
        useCursor(KisCursor::triangleRightHandedCursor());
        break;
    case CURSOR_STYLE_TRIANGLE_LEFTHANDED:
        useCursor(KisCursor::triangleLeftHandedCursor());
        break;
    case CURSOR_STYLE_BLACK_PIXEL:
        useCursor(KisCursor::pixelBlackCursor());
        break;
    case CURSOR_STYLE_WHITE_PIXEL:
        useCursor(KisCursor::pixelWhiteCursor());
        break;
    case CURSOR_STYLE_ERASER:
        useCursor(KisCursor::eraserCursor());
        break;
    case CURSOR_STYLE_TOOLICON:
    default:
        KisToolPaint::resetCursorStyle();
        break;
    }
}

KisPaintingInformationBuilder* KisToolFreehand::paintingInformationBuilder() const
{
    return m_infoBuilder;
}

void KisToolFreehand::resetHelper(KisToolFreehandHelper *helper)
{
    delete m_helper;
    m_helper = helper;
}

bool KisToolFreehand::supportsPaintingAssistants() const
{
    return true;
}

int KisToolFreehand::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP|KisTool::FLAG_USES_CUSTOM_PRESET
           |KisTool::FLAG_USES_CUSTOM_SIZE;
}

void KisToolFreehand::activate(const QSet<KoShape*> &shapes)
{
    KisToolPaint::activate(shapes);
}

void KisToolFreehand::deactivate()
{
    if (mode() == PAINT_MODE) {
        if (m_quickShapeActive) {
            abortQuickShape();
        } else {
            endStroke();
        }
        setMode(KisTool::HOVER_MODE);
    }
    KisToolPaint::deactivate();
}

void KisToolFreehand::initStroke(KoPointerEvent *event)
{
    resetQuickShapeTracking();

    const QPointF pixelCoords = convertToPixelCoord(event);

    m_helper->initPaint(event,
                        pixelCoords,
                        image(),
                        currentNode(),
                        image().data());

    m_quickShapeTracker->setHoldSlop(kQuickShapeHoldSlopWidgetPixels / canvasZoom());
    m_quickShapeTracker->begin(pixelCoords);
    m_quickShapeHoldTimer.start(kQuickShapeHoldDelayMs);
}

void KisToolFreehand::doStroke(KoPointerEvent *event)
{
    m_helper->paintEvent(event);
}

void KisToolFreehand::endStroke()
{
    resetQuickShapeTracking();

    m_helper->endPaint();
    bool paintOpIgnoredEvent = currentPaintOpPreset()->settings()->mouseReleaseEvent();
    Q_UNUSED(paintOpIgnoredEvent);
}

bool KisToolFreehand::primaryActionSupportsHiResEvents() const
{
    return true;
}

void KisToolFreehand::beginPrimaryAction(KoPointerEvent *event)
{
    // FIXME: workaround for the Duplicate Op
    trySampleByPaintOp(event, SampleFgImage);

    requestUpdateOutline(event->point, event);

    NodePaintAbility paintability = nodePaintAbility();
    // XXX: move this to KisTool and make it work properly for clone layers: for clone layers, the shape paint tools don't work either
    if (!nodeEditable() || paintability != PAINT) {
        if (paintability == KisToolPaint::VECTOR || paintability == KisToolPaint::CLONE){
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("The brush tool cannot paint on this layer.  Please select a paint layer or mask.");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }
        else if (paintability == MYPAINTBRUSH_UNPAINTABLE) {
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("The MyPaint Brush Engine is not available for this colorspace");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }
        event->ignore();

        return;
    }

    KIS_SAFE_ASSERT_RECOVER_RETURN(!m_helper->isRunning());

    setMode(KisTool::PAINT_MODE);

    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
    if (canvas2) {
        canvas2->viewManager()->disableControls();
    }

    initStroke(event);
}

void KisToolFreehand::continuePrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);

    requestUpdateOutline(event->point, event);

    if (m_quickShapeActive) {
        /**
         * The recognized shape has replaced the freehand stroke. The pointer
         * stays pressed so that the user can move the line endpoint or resize
         * and rotate the ellipse, request a circle with a second touch point, and release to
         * commit the shape.
         */
        if (m_quickShapeTracker->adjustTo(convertToPixelCoord(event))) {
            updateQuickShapePreview();
        }
        return;
    }

    if (m_quickShapeTracker->isTracking()
        && m_quickShapeTracker->extend(convertToPixelCoord(event))) {
        m_quickShapeHoldTimer.start(kQuickShapeHoldDelayMs);
    }

    /**
     * Actual painting
     */
    doStroke(event);
}

void KisToolFreehand::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);

    if (m_quickShapeActive) {
        if (m_quickShapeTracker->shape().type() == KisQuickShape::Line
            && m_quickShapeTracker->adjustTo(convertToPixelCoord(event))) {
            updateQuickShapePreview();
        }
        commitQuickShape();
    } else {
        endStroke();
    }

    if (m_assistant && static_cast<KisCanvas2*>(canvas())->paintingAssistantsDecoration()) {
        static_cast<KisCanvas2*>(canvas())->paintingAssistantsDecoration()->endStroke();
    }

    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
    if (canvas2) {
        canvas2->viewManager()->enableControls();
    }

    setMode(KisTool::HOVER_MODE);
}

bool KisToolFreehand::trySampleByPaintOp(KoPointerEvent *event, AlternateAction action)
{
    if (action != SampleFgNode && action != SampleFgImage) return false;

    /**
     * FIXME: we need some better way to implement modifiers
     * for a paintop level. This method is used in DuplicateOp only!
     */
    QPointF pos = adjustPosition(event->point, event->point);
    qreal perspective = calculatePerspective(pos);
    if (!currentPaintOpPreset()) {
        return false;
    }
    KisPaintInformation info(convertToPixelCoord(event->point),
                             m_infoBuilder->pressureToCurve(event->pressure()),
                             event->xTilt(), event->yTilt(),
                             event->rotation(),
                             event->tangentialPressure(),
                             perspective, 0, 0);
    info.setRandomSource(new KisRandomSource());
    info.setPerStrokeRandomSource(new KisPerStrokeRandomSource());

    bool paintOpIgnoredEvent = currentPaintOpPreset()->settings()->mousePressEvent(info,
                                                                                   event->modifiers(),
                                                                                   currentNode());
    // DuplicateOP during the sampling of new source point (origin)
    // is the only paintop that returns "false" here
    return !paintOpIgnoredEvent;
}

void KisToolFreehand::activateAlternateAction(AlternateAction action)
{
    if (action != ChangeSize && action != ChangeSizeSnap) {
        KisToolPaint::activateAlternateAction(action);
        return;
    }

    useCursor(KisCursor::blankCursor());
    setOutlineVisible(true);
}

void KisToolFreehand::deactivateAlternateAction(AlternateAction action)
{
    if (action != ChangeSize && action != ChangeSizeSnap) {
        KisToolPaint::deactivateAlternateAction(action);
        return;
    }

    resetCursorStyle();
    setOutlineVisible(false);
}

void KisToolFreehand::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (trySampleByPaintOp(event, action)) {
        m_paintopBasedSamplingInAction = true;
        return;
    }

    if (action != ChangeSize && action != ChangeSizeSnap) {
        KisToolPaint::beginAlternateAction(event, action);
        return;
    }

    setMode(GESTURE_MODE);
    m_initialGestureDocPoint = event->point;
    m_initialGestureGlobalPoint = event->globalPos();

    m_lastDocumentPoint = event->point;
    m_lastPaintOpSize = currentPaintOpPreset()->settings()->paintOpSize();

    m_beginAlternateActionEvent = event->deepCopyEvent();
    requestUpdateOutline(m_initialGestureDocPoint, &m_beginAlternateActionEvent->event);
}

void KisToolFreehand::continueAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (trySampleByPaintOp(event, action) || m_paintopBasedSamplingInAction) return;

    if (action != ChangeSize && action != ChangeSizeSnap) {
        KisToolPaint::continueAlternateAction(event, action);
        return;
    }

    QPointF lastWidgetPosition = convertDocumentToWidget(m_lastDocumentPoint);
    QPointF actualWidgetPosition = convertDocumentToWidget(event->point);

    QPointF offset = actualWidgetPosition - lastWidgetPosition;

    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN(canvas2);
    QRect screenRect = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    qreal scaleX = 0;
    qreal scaleY = 0;
    canvas2->coordinatesConverter()->imageScale(&scaleX, &scaleY);

    const qreal maxBrushSize = KisImageConfig(true).maxBrushSize();
    const qreal effectiveMaxDragSize = 0.5 * screenRect.width();
    const qreal effectiveMaxBrushSize = qMin(maxBrushSize, effectiveMaxDragSize / scaleX);

    const qreal scaleCoeff = effectiveMaxBrushSize / effectiveMaxDragSize;
    const qreal sizeDiff = scaleCoeff * offset.x() ;

    if (qAbs(sizeDiff) > 0.01) {
        KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();

        qreal newSize = m_lastPaintOpSize + sizeDiff;

        if (action == ChangeSizeSnap) {
            newSize = qMax(qRound(newSize), 1);
        }

        newSize = qBound(0.01, newSize, maxBrushSize);

        settings->setPaintOpSize(newSize);

        requestUpdateOutline(
            m_initialGestureDocPoint,
            m_beginAlternateActionEvent.has_value() ? &m_beginAlternateActionEvent->event : nullptr);
        //m_brushResizeCompressor.start(newSize);

        m_lastDocumentPoint = event->point;
        m_lastPaintOpSize = newSize;
    }
}

void KisToolFreehand::endAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if (trySampleByPaintOp(event, action) || m_paintopBasedSamplingInAction) {
        m_paintopBasedSamplingInAction = false;
        return;
    }

    if (action != ChangeSize && action != ChangeSizeSnap) {
        KisToolPaint::endAlternateAction(event, action);
        return;
    }

    KisToolUtils::setCursorPos(m_initialGestureGlobalPoint);
    requestUpdateOutline(m_initialGestureDocPoint, 0);

    setMode(HOVER_MODE);

    m_beginAlternateActionEvent.reset();
}

bool KisToolFreehand::wantsAutoScroll() const
{
    return false;
}

void KisToolFreehand::setAssistant(bool assistant)
{
    m_assistant = assistant;
}

void KisToolFreehand::setOnlyOneAssistantSnap(bool assistant)
{
    m_only_one_assistant = assistant;
}

void KisToolFreehand::setSnapEraser(bool assistant)
{
    m_eraser_snapping = assistant;
}

void KisToolFreehand::slotDoResizeBrush(qreal newSize)
{
    KisPaintOpSettingsSP settings = currentPaintOpPreset()->settings();

    settings->setPaintOpSize(newSize);
    requestUpdateOutline(m_initialGestureDocPoint, 0);

}

QPointF KisToolFreehand::adjustPosition(const QPointF& point, const QPointF& strokeBegin)
{
    if (m_assistant && static_cast<KisCanvas2*>(canvas())->paintingAssistantsDecoration()) {
        KisCanvas2* c = static_cast<KisCanvas2*>(canvas());
        c->paintingAssistantsDecoration()->setOnlyOneAssistantSnap(m_only_one_assistant);
        c->paintingAssistantsDecoration()->setEraserSnap(m_eraser_snapping);
        QPointF ap = c->paintingAssistantsDecoration()->adjustPosition(point, strokeBegin);
        QPointF fp = (1.0 - m_magnetism) * point + m_magnetism * ap;
        // Report the final position back to the assistant so the guides
        // can follow the brush
        c->paintingAssistantsDecoration()->setAdjustedBrushPosition(fp);
        return fp;
    }
    return point;
}

qreal KisToolFreehand::calculatePerspective(const QPointF &documentPoint)
{
    qreal perspective = 1.0;
    Q_FOREACH (const KisPaintingAssistantSP assistant, static_cast<KisCanvas2*>(canvas())->paintingAssistantsDecoration()->assistants()) {
        QPointer<KisAbstractPerspectiveGrid> grid = dynamic_cast<KisAbstractPerspectiveGrid*>(assistant.data());
        if (grid && grid->isActive() && grid->contains(documentPoint)) {
            perspective = grid->distance(documentPoint);
            break;
        }
    }
    return perspective;
}

void KisToolFreehand::updateMaskSyntheticEventsFromTouch()
{
    setMaskSyntheticEvents(KisConfig(true).disableTouchOnCanvas());
}

void KisToolFreehand::touchDuringStroke(const QPointF &documentPoint)
{
    Q_UNUSED(documentPoint);

    if (!m_quickShapeActive) {
        return;
    }

    if (!m_quickShapeTracker->toggleCircle()) {
        return;
    }

    updateQuickShapePreview();
}

void KisToolFreehand::slotQuickShapeHoldTimeout()
{
    if (m_quickShapeActive || !m_helper->isRunning()) {
        return;
    }

    if (!m_quickShapeTracker->recognize()) {
        return;
    }

    m_quickShapeActive = true;
    m_quickShapePaintInformation = m_helper->currentPaintInformation();
    m_quickShapeResources = m_helper->currentResourcesSnapshot();
    m_quickShapeRandomSeed = int(QRandomGenerator::global()->generate());

    /**
     * Drop the freehand stroke so that the snapped shape replaces it instead
     * of being painted over it.
     */
    m_helper->cancelPaint();

    updateQuickShapePreview();
    prepareQuickShapePreview();
}

void KisToolFreehand::paint(QPainter &gc, const KoViewConverter &converter)
{
    const auto *canvas2 = static_cast<KisCanvas2 *>(canvas());
    gc.save();
    gc.setTransform(QTransform());
    m_quickShapePreview->paint(gc, canvas2->coordinatesConverter()->imageToWidgetTransform(),
                               canvas2->canvasWidget()->rect());
    gc.restore();

    KisToolPaint::paint(gc, converter);
}

void KisToolFreehand::updateQuickShapePreview()
{
    if (m_quickShapeActive) {
        updateQuickShapeCanvas(m_quickShapePreview->update(m_quickShapeTracker->shape()));
    } else {
        updateQuickShapeCanvas(m_quickShapePreview->clear());
    }
}

void KisToolFreehand::updateQuickShapeCanvas(const QRectF &dirty)
{
    if (!dirty.isEmpty()) {
        const qreal margin = 8.0 / canvasZoom();
        canvas()->updateCanvas(convertToPt(dirty.adjusted(-margin, -margin, margin, margin)));
    }
}

void KisToolFreehand::prepareQuickShapePreview()
{
    if (!m_quickShapeActive || m_quickShapeSamplePending) return;
    // Prepare one small brush material after cancellation. Shape edits only
    // update the display owner; they never enter this preparation path.
    if (!m_quickShapeResources->image()->isIdle()) {
        const quint64 generation = m_quickShapeGeneration;
        QTimer::singleShot(16, this, [this, generation] {
            if (generation == m_quickShapeGeneration) prepareQuickShapePreview();
        });
        return;
    }
    m_quickShapeSampleGeneration = m_quickShapeGeneration;
    m_quickShapeSamplePending = true;
    const auto resources = m_quickShapeResources;
    const auto information = m_quickShapePaintInformation;
    const int seed = m_quickShapeRandomSeed;
    const auto displayConfig = static_cast<KisCanvas2 *>(canvas())->displayColorConverter()->multiSurfaceDisplayConfig();
    m_quickShapeSampleWatcher.setFuture(QtConcurrent::run([resources, information, seed, displayConfig] {
        const auto sample = KisFigurePaintingStroke::createPreviewSample(*resources, information, seed);
        KisDisplayColorTransform transform;
        transform.setInputColorSpace(sample.device->colorSpace());
        transform.setDisplayConfiguration(displayConfig.uiProfile, displayConfig.canvasProfile,
                                          displayConfig.intent, displayConfig.conversionFlags);
        return std::make_tuple(transform.convertImageToDisplayColorSpace(sample.device, sample.bounds),
                               sample.line, sample.imageUnitsPerPixel);
    }));
}

void KisToolFreehand::finishQuickShapePreviewPreparation()
{
    m_quickShapeSamplePending = false;
    if (!m_quickShapeActive) return;
    if (m_quickShapeSampleGeneration != m_quickShapeGeneration) {
        prepareQuickShapePreview();
        return;
    }
    const auto sample = m_quickShapeSampleWatcher.result();
    updateQuickShapeCanvas(m_quickShapePreview->setStrokeSample(std::get<0>(sample), std::get<1>(sample),
                                                               std::get<2>(sample)));
}

void KisToolFreehand::retireQuickShapePreview(quint64 generation)
{
    if (generation != m_quickShapeGeneration || m_quickShapeActive) return;
    if (!image()->isIdle()) {
        QTimer::singleShot(16, this, [this, generation] { retireQuickShapePreview(generation); });
        return;
    }
    updateQuickShapeCanvas(m_quickShapePreview->clear());
}

void KisToolFreehand::commitQuickShape()
{
    const QPainterPath path = m_quickShapePreview->path();
    m_quickShapeActive = false;
    m_quickShapeHoldTimer.stop();

    if (!path.isEmpty()) {
        KisFigurePaintingStroke stroke(kundo2_i18n("Draw Shape"), *m_quickShapeResources, m_quickShapeRandomSeed);
        stroke.paintStrokePath(path, m_quickShapePaintInformation);
    }

    m_quickShapeTracker->reset();
    m_quickShapeResources.clear();
    retireQuickShapePreview(m_quickShapeGeneration);
}

void KisToolFreehand::abortQuickShape()
{
    m_quickShapeActive = false;
    updateQuickShapePreview();
    resetQuickShapeTracking();
}

void KisToolFreehand::resetQuickShapeTracking()
{
    m_quickShapeActive = false;
    m_quickShapeHoldTimer.stop();
    ++m_quickShapeGeneration;
    updateQuickShapeCanvas(m_quickShapePreview->clear());
    m_quickShapeTracker->reset();
    m_quickShapeResources.clear();
    m_quickShapePaintInformation = KisPaintInformation();
}

qreal KisToolFreehand::canvasZoom() const
{
    KoCanvasResourceProvider *provider = canvas()->resourceManager();
    if (!provider) {
        return 1.0;
    }

    const qreal zoom = provider->resource(KoCanvasResource::EffectiveZoom).toReal();
    return zoom > 0.01 ? zoom : 1.0;
}

void KisToolFreehand::explicitUpdateOutline()
{
    requestUpdateOutline(m_outlineDocPoint, 0);
}

KisOptimizedBrushOutline KisToolFreehand::getOutlinePath(const QPointF &documentPos,
                                             const KoPointerEvent *event,
                                             KisPaintOpSettings::OutlineMode outlineMode)
{
    if (currentPaintOpPreset())
        return m_helper->paintOpOutline(convertToPixelCoord(documentPos),
                                        event,
                                        currentPaintOpPreset()->settings(),
                                        outlineMode);
    else
        return KisOptimizedBrushOutline();
}
