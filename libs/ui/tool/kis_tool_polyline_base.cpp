/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */


#include <QAction>
#include <QPainterPath>

#include <KoPointerEvent.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoViewConverter.h>
#include <input/ui/kis_input_manager.h>

#include "kis_tool_polyline_base.h"
#include "kis_canvas2.h"
#include <kis_canvas_resource_provider.h>
#include <KisViewManager.h>
#include <kis_action.h>
#include <kactioncollection.h>
#include <kis_icon.h>

#include "kis_action_registry.h"

#define SNAPPING_THRESHOLD 10
#define SNAPPING_HANDLE_RADIUS 8
#define PREVIEW_LINE_WIDTH 1

KisToolPolylineBase::KisToolPolylineBase(KoCanvasBase * canvas,  KisToolPolylineBase::ToolType type, const QCursor & cursor)
    : KisToolShape(canvas, cursor),
      m_type(type)
{
    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas);

    connect(kritaCanvas->viewManager()->canvasResourceProvider(), SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(resetCursorStyle()));
}


void KisToolPolylineBase::activate(const QSet<KoShape *> &shapes)
{
    KisToolShape::activate(shapes);
    connect(action("undo_polygon_selection"), SIGNAL(triggered()), SLOT(undoSelectionOrCancel()), Qt::UniqueConnection);

    KisInputManager *inputManager = (static_cast<KisCanvas2*>(canvas()))->globalInputManager();
    if (inputManager) {
        inputManager->attachPriorityEventFilter(this);
    }
}

void KisToolPolylineBase::deactivate()
{
    disconnect(action("undo_polygon_selection"), 0, this, 0);
    cancelStroke();

    KisInputManager *inputManager = (static_cast<KisCanvas2*>(canvas()))->globalInputManager();
    if (inputManager) {
        inputManager->detachPriorityEventFilter(this);
    }

    KisToolShape::deactivate();
}

void KisToolPolylineBase::requestStrokeEnd()
{
    endStroke();
}

void KisToolPolylineBase::requestStrokeCancellation()
{
    cancelStroke();
}

KisPopupWidgetInterface* KisToolPolylineBase::popupWidget()
{
    return m_polylineInteraction.isActive() || m_type == SELECT
        ? nullptr
        : KisToolShape::popupWidget();
}

// Install an event filter to catch right-click events.
// The simplest way to accommodate the popup palette binding.
bool KisToolPolylineBase::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (!m_polylineInteraction.isActive()) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            undoSelectionOrCancel();
            return true;
        }
    } else if (event->type() == QEvent::TabletPress) {
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);
        if (tabletEvent->button() == Qt::RightButton) {
            undoSelectionOrCancel();
            return true;
        }
    }
    return false;
}

void KisToolPolylineBase::beginPrimaryAction(KoPointerEvent *event)
{
    Q_UNUSED(event);
    NodePaintAbility paintability = nodePaintAbility();
    if ((m_type == PAINT && (!nodeEditable() || paintability == UNPAINTABLE || paintability  == KisToolPaint::CLONE || paintability == KisToolPaint::MYPAINTBRUSH_UNPAINTABLE)) ||
        (m_type == SELECT && !selectionEditable())) {

        if (paintability == KisToolPaint::CLONE){
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("This tool cannot paint on clone layers.  Please select a paint or vector layer or mask.");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }

        if (paintability == KisToolPaint::MYPAINTBRUSH_UNPAINTABLE) {
            KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
            QString message = i18n("The MyPaint Brush Engine is not available for this colorspace");
            kiscanvas->viewManager()->showFloatingMessage(message, koIcon("object-locked"));
        }

        event->ignore();
        return;
    }

    setMode(KisTool::PAINT_MODE);

    if (m_polylineInteraction.isActive()
        && m_polylineInteraction.closeSnappingActive()) {
        endStroke(true);
    } else {
        beginShape();
        m_polylineInteraction.begin();
    }
}

void KisToolPolylineBase::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    setMode(KisTool::HOVER_MODE);

    if (m_polylineInteraction.isActive()) {
        m_polylineInteraction.addPoint(convertToPixelCoordAndSnap(event));
    }
}

void KisToolPolylineBase::beginPrimaryDoubleClickAction(KoPointerEvent *event)
{
    endStroke();

    // this action will have no continuation
    event->ignore();
}

void KisToolPolylineBase::beginAlternateAction(KoPointerEvent *event, AlternateAction action)
{
    if ((action != ChangeSize && action != ChangeSizeSnap)
        || !m_polylineInteraction.isActive()) {
        KisToolPaint::beginAlternateAction(event, action);
    }

    endStroke(m_polylineInteraction.closeSnappingActive());
}

void KisToolPolylineBase::mouseMoveEvent(KoPointerEvent *event)
{
    if (m_polylineInteraction.isActive()
        && !m_polylineInteraction.points().empty()) {
        // erase old lines on canvas
        QRectF updateRect = dragBoundingRect();
        // get current mouse position
        const QPointF dragEnd = convertToPixelCoordAndSnap(event);

        const QPointF basePoint = pixelToView(
            m_polylineInteraction.points().first());
        const bool closeSnappingActive =
            m_polylineInteraction.points().size() > 1
            && (basePoint - pixelToView(dragEnd)).manhattanLength()
                < SNAPPING_THRESHOLD;
        m_polylineInteraction.updateCursor(dragEnd, closeSnappingActive);

        // draw new lines on canvas
        updateRect |= dragBoundingRect();
        updateCanvasViewRect(updateRect);

        updateCanvasViewRect(QRectF(basePoint, 2 * QSize(SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH, SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH)).translated(-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH,-SNAPPING_HANDLE_RADIUS + PREVIEW_LINE_WIDTH));
        KisToolPaint::requestUpdateOutline(event->point, event);
    } else {
        KisToolPaint::mouseMoveEvent(event);
    }
}

void KisToolPolylineBase::undoSelection()
{
    if (m_polylineInteraction.isActive()) {
        // Initialize with the dragging segment's rect
        QRectF updateRect = dragBoundingRect();

        if (m_polylineInteraction.canUndoPoint()) {
            const QVector<QPointF> &points = m_polylineInteraction.points();
            // Add the rect for the last segment
            const QRectF lastSegmentRect =
                pixelToView(QRectF(points.last(),
                                   points.at(points.size() - 2)).normalized())
                .adjusted(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
            updateRect = updateRect.united(lastSegmentRect);

            m_polylineInteraction.undoPoint();
        }

        // Add the new dragging segment's rect
        updateRect = updateRect.united(dragBoundingRect());
        updateCanvasViewRect(updateRect);
    }
}

void KisToolPolylineBase::undoSelectionOrCancel()
{
    if (m_polylineInteraction.canUndoPoint()) {
        undoSelection();
    } else {
        cancelStroke();
    }
}

void KisToolPolylineBase::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    if (!canvas() || !currentImage())
        return;

    QPointF start, end;
    QPointF startPos;
    QPointF endPos;

    QPainterPath path;
    const QVector<QPointF> &points = m_polylineInteraction.points();
    if (m_polylineInteraction.isActive() && !points.empty()) {
        startPos = pixelToView(m_polylineInteraction.dragStart());
        endPos = pixelToView(m_polylineInteraction.dragEnd());
        path.moveTo(startPos);
        path.lineTo(endPos);
    }

    for (auto it = points.cbegin(); it != points.cend(); ++it) {

        if (it == points.cbegin()) {
            start = (*it);
        } else {
            end = (*it);

            startPos = pixelToView(start);
            endPos = pixelToView(end);
            path.moveTo(startPos);
            path.lineTo(endPos);
            start = end;
        }
    }

    if (m_polylineInteraction.closeSnappingActive()) {
        QPointF basePoint = pixelToView(points.first());
        path.addEllipse(basePoint, SNAPPING_HANDLE_RADIUS, SNAPPING_HANDLE_RADIUS);
    }

    paintToolOutline(&gc, path);
    KisToolPaint::paint(gc,converter);
}

void KisToolPolylineBase::updateArea()
{
    updateCanvasPixelRect(image()->bounds());
}

void KisToolPolylineBase::endStroke(bool closePolyline)
{
    if (!m_polylineInteraction.isActive()) return;

    const QVector<QPointF> points =
        m_polylineInteraction.finish(closePolyline);
    if (points.count() > 1) {
        finishPolyline(points);
    }
    updateArea();
    endShape();
}

void KisToolPolylineBase::cancelStroke()
{
    if (!m_polylineInteraction.isActive()) return;

    m_polylineInteraction.cancel();
    updateArea();
    endShape();
}

QRectF KisToolPolylineBase::dragBoundingRect()
{
    QRectF rect = pixelToView(
        QRectF(m_polylineInteraction.dragStart(),
               m_polylineInteraction.dragEnd()).normalized());
    rect.adjust(-PREVIEW_LINE_WIDTH, -PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH, PREVIEW_LINE_WIDTH);
    return rect;
}
