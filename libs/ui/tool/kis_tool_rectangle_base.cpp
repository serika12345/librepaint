/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_tool_rectangle_base.h"

#include <QPainterPath>

#include "KisViewManager.h"
#include "kis_canvas2.h"
#include <KisOptionCollectionWidget.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoPointerEvent.h>
#include <KoViewConverter.h>
#include <input/ui/kis_extended_modifiers_mapper.h>
#include <kis_icon.h>

#include "kis_rectangle_constraint_widget.h"

KisToolRectangleBase::KisToolRectangleBase(KoCanvasBase * canvas, KisToolRectangleBase::ToolType type, const QCursor & cursor)
    : KisToolShape(canvas, cursor)
    , m_type(type)
    , m_roundCornersX(0)
    , m_roundCornersY(0)
{
}


QList<QPointer<QWidget> > KisToolRectangleBase::createOptionWidgets()
{
    QList<QPointer<QWidget>> widgetsList = KisToolShape::createOptionWidgets();

    KisRectangleConstraintWidget *widget =
        new KisRectangleConstraintWidget(toolId(), showRoundCornersGUI());
    connect(widget,
            &KisRectangleConstraintWidget::constraintsChanged,
            this,
            &KisToolRectangleBase::constraintsChanged);
    connect(widget,
            &KisRectangleConstraintWidget::roundCornersChanged,
            this,
            &KisToolRectangleBase::roundCornersChanged);
    connect(this,
            &KisToolRectangleBase::rectangleChanged,
            widget,
            &KisRectangleConstraintWidget::setRectangle);
    connect(this,
            &KisToolRectangleBase::sigRequestReloadConfig,
            widget,
            &KisRectangleConstraintWidget::reloadConfig);
    widget->reloadConfig();

    if (widgetsList.size() > 0
        && dynamic_cast<KisOptionCollectionWidget *>(
            widgetsList.first().data())) {
        KisOptionCollectionWidget *baseOptions =
            dynamic_cast<KisOptionCollectionWidget *>(
                widgetsList.first().data());
        KisOptionCollectionWidgetWithHeader *sectionRectangle =
            new KisOptionCollectionWidgetWithHeader(widget->windowTitle());
        sectionRectangle->appendWidget("rectangleConstraintWidget", widget);
        baseOptions->appendWidget("sectionRectangle", sectionRectangle);
    } else {
        widget->setContentsMargins(10, 10, 10, 10);
        widgetsList.append(widget);
    }

    return widgetsList;
}

void KisToolRectangleBase::constraintsChanged(bool forceRatio, bool forceWidth, bool forceHeight, float ratio, float width, float height)
{
    m_rectangleInteraction.setConstraints(forceRatio,
                                          forceWidth,
                                          forceHeight,
                                          ratio,
                                          width,
                                          height);
}

void KisToolRectangleBase::roundCornersChanged(int rx, int ry)
{
    m_roundCornersX = rx;
    m_roundCornersY = ry;
}

void KisToolRectangleBase::showSize()
{
    KisCanvas2 *kisCanvas =dynamic_cast<KisCanvas2*>(canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN(kisCanvas);
    const QRectF rectangle = createRect(m_rectangleInteraction.start(),
                                        m_rectangleInteraction.end());
    kisCanvas->viewManager()->showFloatingMessage(i18n("Width: %1 px\nHeight: %2 px"
                                                       , rectangle.width()
                                                       , rectangle.height()), QIcon(), 1000
                                                       , KisFloatingMessage::High,  Qt::AlignLeft | Qt::TextWordWrap | Qt::AlignVCenter);

}
void KisToolRectangleBase::paint(QPainter& gc, const KoViewConverter &converter)
{
    if(mode() == KisTool::PAINT_MODE) {
        paintRectangle(gc, createRect(m_rectangleInteraction.start(),
                                      m_rectangleInteraction.end()));
    }

    KisToolPaint::paint(gc, converter);
}

void KisToolRectangleBase::activate(const QSet<KoShape *> &shapes)
{
    KisToolShape::activate(shapes);

    Q_EMIT sigRequestReloadConfig();
}

void KisToolRectangleBase::deactivate()
{
    cancelStroke();
    KisToolShape::deactivate();
}

void KisToolRectangleBase::keyPressEvent(QKeyEvent *event) {
    const Qt::Key key = KisExtendedModifiersMapper::workaroundShiftAltMetaHell(event);

    if (key == Qt::Key_Control) {
        m_rectangleInteraction.setModifier(Qt::ControlModifier, true);
    } else if (key == Qt::Key_Shift) {
        m_rectangleInteraction.setModifier(Qt::ShiftModifier, true);
    } else if (key == Qt::Key_Alt) {
        m_rectangleInteraction.setModifier(Qt::AltModifier, true);
    }

    KisToolShape::keyPressEvent(event);
}

void KisToolRectangleBase::keyReleaseEvent(QKeyEvent *event) {
    const Qt::Key key = KisExtendedModifiersMapper::workaroundShiftAltMetaHell(event);

    if (key == Qt::Key_Control) {
        m_rectangleInteraction.setModifier(Qt::ControlModifier, false);
    } else if (key == Qt::Key_Shift) {
        m_rectangleInteraction.setModifier(Qt::ShiftModifier, false);
    } else if (key == Qt::Key_Alt) {
        m_rectangleInteraction.setModifier(Qt::AltModifier, false);
    }

    KisToolShape::keyReleaseEvent(event);
}

void KisToolRectangleBase::beginPrimaryAction(KoPointerEvent *event)
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
    beginShape();

    QPointF pos = convertToPixelCoordAndSnap(event, QPointF(), false);
    m_rectangleInteraction.begin(pos);
    showSize();
    event->accept();
}

void KisToolRectangleBase::continuePrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);

    const QPointF pos = convertToPixelCoordAndSnap(event, QPointF(), false);
    m_rectangleInteraction.update(pos);

    if (!m_rectangleInteraction.isTranslating()) {
        showSize();
    } else {
        KisCanvas2 *kisCanvas =dynamic_cast<KisCanvas2*>(canvas());
        KIS_ASSERT(kisCanvas);
        const QPointF start = m_rectangleInteraction.start();
        kisCanvas->viewManager()->showFloatingMessage(i18n("X: %1 px\nY: %2 px"
                                                           , QString::number(start.x(), 'f', 1)
                                                           , QString::number(start.y(), 'f', 1)), QIcon(), 1000
                                                           , KisFloatingMessage::High,  Qt::AlignLeft | Qt::TextWordWrap | Qt::AlignVCenter);
    }
    updateArea();

    KisToolPaint::requestUpdateOutline(event->point, event);
}

void KisToolRectangleBase::endPrimaryAction(KoPointerEvent *event)
{
    CHECK_MODE_SANITY_OR_RETURN(KisTool::PAINT_MODE);
    // If the event was not originated by the user releasing the button
    // (for example due to the canvas loosing focus), then we just cancel the
    // operation. This prevents some issues with shapes being added after
    // the image was closed while the shape was being made
    if (event->spontaneous()) {
        endStroke();
    } else {
        cancelStroke();
    }
    event->accept();
}

void KisToolRectangleBase::requestStrokeEnd()
{
    if (mode() != KisTool::PAINT_MODE) {
        return;
    }
    endStroke();
}

void KisToolRectangleBase::requestStrokeCancellation()
{
    if (mode() != KisTool::PAINT_MODE) {
        return;
    }
    cancelStroke();
}

void KisToolRectangleBase::endStroke()
{
    setMode(KisTool::HOVER_MODE);
    updateArea();
    finishRect(createRect(m_rectangleInteraction.start(),
                          m_rectangleInteraction.end()),
               m_roundCornersX,
               m_roundCornersY);
    endShape();
}

void KisToolRectangleBase::cancelStroke()
{
    setMode(KisTool::HOVER_MODE);
    updateArea();
    endShape();
}

QRectF KisToolRectangleBase::createRect(const QPointF &start, const QPointF &end)
{
    return m_rectangleInteraction.rectangle(start, end);
}

bool KisToolRectangleBase::showRoundCornersGUI() const
{
    return true;
}

void KisToolRectangleBase::paintRectangle(QPainter &gc, const QRectF &imageRect)
{
    KIS_ASSERT_RECOVER_RETURN(canvas());

    const QRect viewRect = pixelToView(imageRect).toAlignedRect();

    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN(kritaCanvas);

    const KisCoordinatesConverter *converter = kritaCanvas->coordinatesConverter();
    const qreal roundCornersX = converter->effectiveZoom() * m_roundCornersX;
    const qreal roundCornersY = converter->effectiveZoom() * m_roundCornersY;

    QPainterPath path;
    if (m_roundCornersX > 0 || m_roundCornersY > 0) {
        path.addRoundedRect(viewRect,
                            roundCornersX, roundCornersY);
    } else {
        path.addRect(viewRect);
    }

    getRotatedPath(path, viewRect.center(), getRotationAngle());
    path.addPath(drawX(pixelToView(dragStart())));
    path.addPath(drawX(pixelToView(dragCenter())));
    paintToolOutline(&gc, path);
}

void KisToolRectangleBase::updateArea() {
    const QRectF bound = createRect(m_rectangleInteraction.start(),
                                    m_rectangleInteraction.end());

    canvas()->updateCanvas(convertToPt(bound).adjusted(-100, -100, +200, +200));

    Q_EMIT rectangleChanged(bound);
}

qreal KisToolRectangleBase::getRotationAngle() {
    return m_rectangleInteraction.rotationAngle();
}

QPointF KisToolRectangleBase::dragStart() const
{
    return m_rectangleInteraction.start();
}

QPointF KisToolRectangleBase::dragCenter() const
{
    return m_rectangleInteraction.center();
}

QPainterPath KisToolRectangleBase::drawX(const QPointF &pt) {
    QPainterPath path;
    path.moveTo(QPointF(pt.x() - 5.0, pt.y() - 5.0)); path.lineTo(QPointF(pt.x() + 5.0, pt.y() + 5.0));
    path.moveTo(QPointF(pt.x() - 5.0, pt.y() + 5.0)); path.lineTo(QPointF(pt.x() + 5.0, pt.y() - 5.0));
    return path;
}

void KisToolRectangleBase::getRotatedPath(QPainterPath &path, const QPointF &center, const qreal &angle) {
    QTransform t;
    t.translate(center.x(), center.y());
    t.rotateRadians(angle);
    t.translate(-center.x(), -center.y());

    path = t.map(path);
}
