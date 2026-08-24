/*
 *  SPDX-FileCopyrightText: 2000 John Califf <jcaliff@compuzone.net>
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2004 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2007 Sven Langkamp <sven.langkamp@gmail.com>
 *  SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QAction>
#include <QPainterPath>

#include <KoPointerEvent.h>
#include <KoShapeController.h>
#include <KoViewConverter.h>
#include <KisViewManager.h>
#include <KoCanvasBase.h>
#include <kis_icon.h>
#include <kis_canvas2.h>
#include <input/kis_input_manager.h>

#include "KisToolOutlineBase.h"
#include "input/KisInputActionGroup.h"

KisToolOutlineBase::KisToolOutlineBase(KoCanvasBase * canvas, ToolType type, const QCursor & cursor)
    : KisToolShape(canvas, cursor)
    , m_type(type)
{}

KisToolOutlineBase::~KisToolOutlineBase()
{}

void KisToolOutlineBase::keyPressEvent(QKeyEvent *event)
{
    // Allow to enter continued mode only if we started drawing the shape
    if (mode() == PAINT_MODE && event->key() == Qt::Key_Control) {
        m_outlineInteraction.enableContinuedMode();
        installBlockActionGuard();
    }
    KisToolShape::keyPressEvent(event);
}

void KisToolOutlineBase::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control ||
        !(event->modifiers() & Qt::ControlModifier)) {
        m_outlineInteraction.disableContinuedMode();
        if (mode() != PAINT_MODE) {
            endStroke();
        }
    }
    KisToolShape::keyReleaseEvent(event);
}

void KisToolOutlineBase::mouseMoveEvent(KoPointerEvent *event)
{
    if (m_outlineInteraction.isContinuedMode() && mode() != PAINT_MODE) {
        updateContinuedMode();
        m_outlineInteraction.updateCursor(convertToPixelCoordAndSnap(event));
    } else {
        m_outlineInteraction.updateCursor(convertToPixelCoord(event));
    }
    if (mode() == PAINT_MODE) {
        KisToolShape::requestUpdateOutline(event->point, event);
    }

    KisToolShape::mouseMoveEvent(event);
}

void KisToolOutlineBase::activate(const QSet<KoShape *> &shapes)
{
    KisToolShape::activate(shapes);
    connect(action("undo_polygon_selection"), SIGNAL(triggered()), SLOT(undoLastPoint()), Qt::UniqueConnection);

    KisInputManager *inputManager = (static_cast<KisCanvas2*>(canvas()))->globalInputManager();
    if (inputManager) {
        inputManager->attachPriorityEventFilter(this);
    }
}

void KisToolOutlineBase::deactivate()
{
    cancelStroke();
    
    KisCanvas2 * kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    KIS_ASSERT_RECOVER_RETURN(kisCanvas);
    kisCanvas->updateCanvas();

    m_outlineInteraction.disableContinuedMode();

    KisInputManager *inputManager = kisCanvas->globalInputManager();
    if (inputManager) {
        inputManager->detachPriorityEventFilter(this);
    }

    KisToolShape::deactivate();
}

KisPopupWidgetInterface* KisToolOutlineBase::popupWidget()
{
    return !m_outlineInteraction.points().isEmpty() || m_type == SELECT
        ? nullptr
        : KisToolShape::popupWidget();
}

// Install an event filter to catch right-click events.
// The simplest way to accommodate the popup palette binding.
bool KisToolOutlineBase::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (m_outlineInteraction.points().isEmpty()) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            undoLastPoint();
            return true;
        }
    } else if (event->type() == QEvent::TabletPress) {
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);
        if (tabletEvent->button() == Qt::RightButton) {
            undoLastPoint();
            return true;
        }
    }
    return false;
}

void KisToolOutlineBase::undoLastPoint()
{
    if (m_outlineInteraction.canUndoPoint()) {
        // Initialize with the dragging segment's rect
        QRectF updateRect = dragBoundingRect();

        const QVector<QPointF> &points = m_outlineInteraction.points();
        // Add the rect for the last segment
        const QRectF lastSegmentRect =
            pixelToView(QRectF(points.last(), points.at(points.size() - 2)).normalized())
            .adjusted(-FEEDBACK_LINE_WIDTH, -FEEDBACK_LINE_WIDTH, FEEDBACK_LINE_WIDTH, FEEDBACK_LINE_WIDTH);
        updateRect = updateRect.united(lastSegmentRect);

        m_outlineInteraction.undoPoint();

        // Add the new dragging segment's rect
        updateRect = updateRect.united(dragBoundingRect());
        updateCanvasViewRect(updateRect);
    }
}

void KisToolOutlineBase::beginPrimaryAction(KoPointerEvent *event)
{
    NodePaintAbility paintability = nodePaintAbility();
    if ((m_type == PAINT && (!nodeEditable() || paintability == UNPAINTABLE || paintability  == KisToolPaint::CLONE || paintability == KisToolPaint::MYPAINTBRUSH_UNPAINTABLE)) || (m_type == SELECT && !selectionEditable())) {

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

    const bool continuedMode = m_outlineInteraction.isContinuedMode();
    const QPointF position = continuedMode
        ? convertToPixelCoordAndSnap(event)
        : convertToPixelCoord(event);
    if (m_outlineInteraction.beginInput(position)) {
        beginShape();
    }
}

void KisToolOutlineBase::continuePrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);

    m_outlineInteraction.addPoint(convertToPixelCoord(event));
    updateFeedback();
}

void KisToolOutlineBase::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    setMode(KisTool::HOVER_MODE);
    m_outlineInteraction.endInput();
    if (!m_outlineInteraction.isContinuedMode()) {
        // If the event was not originated by the user releasing the button
        // (for example due to the canvas loosing focus), then we just cancel
        // the operation. This prevents some issues with shapes being added
        // after the image was closed while the shape was being made
        if (event->spontaneous()) {
            endStroke();
        } else {
            cancelStroke();
        }
        event->accept();
    }
}

void KisToolOutlineBase::paint(QPainter& gc, const KoViewConverter &converter)
{
    const QVector<QPointF> &points = m_outlineInteraction.points();
    if ((mode() == KisTool::PAINT_MODE || m_outlineInteraction.isContinuedMode())
        && !points.isEmpty()) {
        QPainterPath outline;
        outline.moveTo(pixelToView(points.first()));
        for (qint32 i = 1; i < points.size(); ++i) {
            outline.lineTo(pixelToView(points[i]));
        }
        if (m_outlineInteraction.isContinuedMode() && mode() != KisTool::PAINT_MODE) {
            outline.lineTo(pixelToView(m_outlineInteraction.cursorPosition()));
        }
        paintToolOutline(&gc, outline);
    }

    KisToolShape::paint(gc, converter);
}

void KisToolOutlineBase::updateFeedback()
{
    const QVector<QPointF> &points = m_outlineInteraction.points();
    if (points.count() > 1) {
        qint32 lastPointIndex = points.count() - 1;

        QRectF updateRect = QRectF(points[lastPointIndex - 1], points[lastPointIndex]).normalized();
        updateRect = kisGrowRect(updateRect, FEEDBACK_LINE_WIDTH);

        updateCanvasPixelRect(updateRect);
    }
}

QRectF KisToolOutlineBase::dragBoundingRect()
{
    QRectF updateRect = pixelToView(
        QRectF(m_outlineInteraction.points().last(),
               m_outlineInteraction.cursorPosition()).normalized());
    updateRect = kisGrowRect(updateRect, FEEDBACK_LINE_WIDTH);
    return updateRect;
}

void KisToolOutlineBase::updateContinuedMode()
{
    if (!m_outlineInteraction.points().isEmpty()) {
        updateCanvasViewRect(dragBoundingRect());
    }
}

bool KisToolOutlineBase::hasUserInteractionRunning() const
{
    return m_outlineInteraction.isActive();
}

void KisToolOutlineBase::endStroke()
{
    if (!hasUserInteractionRunning()) {
        return;
    }
    uninstallBlockActionGuard();
    setMode(KisTool::HOVER_MODE);

    finishOutline(m_outlineInteraction.finish());
    endShape();
}

void KisToolOutlineBase::cancelStroke()
{
    if (!hasUserInteractionRunning()) {
        return;
    }
    uninstallBlockActionGuard();
    setMode(KisTool::HOVER_MODE);

    m_outlineInteraction.cancel();
    endShape();
}

void KisToolOutlineBase::requestStrokeEnd()
{
    endStroke();
}

void KisToolOutlineBase::requestStrokeCancellation()
{
    cancelStroke();
}

void KisToolOutlineBase::installBlockActionGuard()
{
    if (m_blockModifyingActionsGuard)
        return;
    m_blockModifyingActionsGuard.reset(new KisInputActionGroupsMaskGuard(
        static_cast<KisCanvas2*>(canvas())->inputActionGroupsMaskInterface(),
                                 ViewTransformActionGroup | ToolInvoactionActionGroup
                                ));
}

void KisToolOutlineBase::uninstallBlockActionGuard()
{
    m_blockModifyingActionsGuard.reset();
}
