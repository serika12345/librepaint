/*
 *  SPDX-FileCopyrightText: 2025 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoSvgTextShapeOutlineHelper.h"
#include "KisHandleStyle.h"
#include "kis_icon_utils.h"

#include <KoCanvasBase.h>
#include <KoCanvasResourceProvider.h>

#include <KoSvgTextShape.h>
#include <KisHandlePainterHelper.h>
#include <KoShapeManager.h>
#include <KoSelection.h>
#include <QApplication>
#include <QPalette>
#include <KoColorDisplayRendererInterface.h>
#include <QtGlobal>
#include <qhashfunctions.h>
#include <qimage.h>
#include <qline.h>
#include <qlist.h>
#include <qpainterpath.h>
#include <qpen.h>
#include <qpolygon.h>

const int BUTTON_ICON_SIZE = 16;
const int BUTTON_PADDING = 4;
const int BUTTON_CORNER_ROUND = 1;
const QString ICON_EXIT = "object-ungroup-calligra";
const QString ICON_ENTER = "object-group-calligra";

struct KoSvgTextShapeOutlineHelper::Private {
    Private(KoCanvasBase *canvasBase): canvas(canvasBase) {
    }
    KoCanvasBase *canvas;
    int handleRadius = 7;
    int decorationThickness = 1;

    bool drawBoundingRect = true;
    bool drawTextWrappingArea = false;
    bool drawOutline = false;
    bool textWrappingAreasHovered = false;

    KoSvgTextShape *getTextModeShape() {
        return dynamic_cast<KoSvgTextShape*>(canvas->currentShapeManagerOwnerShape());
    }

    KoSvgTextShape *getPotentialTextShape(const QPointF &point) {
        Q_FOREACH(KoShape*shape, canvas->shapeManager()->selection()->selectedEditableShapes()) {
            KoSvgTextShape *text = dynamic_cast<KoSvgTextShape*>(shape);
            if (drawButton(text)) {
                if (getButtonRectCorrected(text->boundingRect()).contains(point)) {
                    return text;
                }
            }
        }
        return nullptr;
    }

    KoViewConverter *converter() const {
        return canvas->viewConverter();
    }

    QRectF getButtonRect(QRectF base) {
        const int buttonSize = BUTTON_ICON_SIZE + (2* BUTTON_PADDING);
        return QRectF(base.topRight(), QSizeF(buttonSize, buttonSize));
    }
    QRectF getButtonRectCorrected(QRectF base) {
        return converter()->viewToDocument().mapRect(getButtonRect(converter()->documentToView().mapRect(base)));
    }

    bool drawButton(KoSvgTextShape *text) {
        return text && !text->internalShapeManager()->shapes().isEmpty();
    }

    KoColorDisplayRendererInterface *renderInterface() {
        return canvas->displayRendererInterface();
    }
};

KoSvgTextShapeOutlineHelper::KoSvgTextShapeOutlineHelper(KoCanvasBase *canvas)
    : d(new Private(canvas))
{
}

KoSvgTextShapeOutlineHelper::~KoSvgTextShapeOutlineHelper()
{

}

QList<QLineF> getTextAreaOrderArrows(QList<QPainterPath> areas) {
    QList<QLineF> lines;
    if (areas.size() <= 1) return lines;
    for (int i = 1; i < areas.size(); i++) {
        const QPainterPath previous = areas.at(i-1);
        const QPainterPath next = areas.at(i);
        const bool overlap = previous.intersects(next);
        QLineF arrow(previous.boundingRect().center(), next.boundingRect().center());
        if (!overlap) {

            Q_FOREACH (QPolygonF p, previous.toSubpathPolygons()) {
                if (p.size() == 1) continue;
                for (int j = 1; j < p.size(); j++) {
                    QLineF l2(p.at(j-1), p.at(j));
                    QPointF intersect;
                    if (l2.intersects(arrow, &intersect) == QLineF::BoundedIntersection) {
                        arrow.setP1(intersect);
                        break;
                    }
                }
            }
            Q_FOREACH (QPolygonF p, next.toSubpathPolygons()) {
                if (p.size() == 1) continue;
                for (int j = 1; j < p.size(); j++) {
                    QLineF l2(p.at(j-1), p.at(j));
                    QPointF intersect;
                    if (l2.intersects(arrow, &intersect) == QLineF::BoundedIntersection) {
                        arrow.setP2(intersect);
                        break;
                    }
                }
            }

        }
        lines.append(arrow);
    }
    return lines;
}

void KoSvgTextShapeOutlineHelper::paintTextShape(QPainter *painter, const KoViewConverter &converter,
                                                 KoSvgTextShape *text, bool contourModeActive) {
    painter->save();
    KisHandlePalette handlePalette = d->renderInterface()->handlePaletteForDisplayColorSpace();
    KisHandlePainterHelper helper =
            KoShape::createHandlePainterHelperView(painter, text, converter, d->handleRadius, d->decorationThickness);
    helper.setHandleStyle(KisHandleStyle::secondarySelection(handlePalette));
    if (contourModeActive) {
        if (d->drawOutline) {

            Q_FOREACH(KoShape *shape, text->internalShapeManager()->shapes()) {
                QPainterPath p = shape->transformation().map(shape->outline());
                helper.drawPath(p);
            }
            QList<QPainterPath> areas;
            Q_FOREACH(const KoShape *shape, text->shapesInside()) {
                areas.append(shape->transformation().map(shape->outline()));
            }
            Q_FOREACH(QLineF arrow, getTextAreaOrderArrows(areas)) {
                helper.drawGradientArrow(arrow.p1(), arrow.p2(), 1.5 * d->handleRadius);
            }
        }
        if (d->drawBoundingRect) {
            QPainterPath rect;
            rect.addRect(text->outlineRect());
            helper.drawPath(rect);
        }
    }
    if (d->drawTextWrappingArea) {
        if (d->textWrappingAreasHovered) {
            helper.setHandleStyle(KisHandleStyle::partiallyHighlightedPrimaryHandles(handlePalette));
        }
        QList<QPainterPath> areas = text->textWrappingAreas();
        Q_FOREACH(QLineF arrow, getTextAreaOrderArrows(areas)) {
            helper.drawGradientArrow(arrow.p1(), arrow.p2(), 1.5 * d->handleRadius);
        }
        Q_FOREACH(const QPainterPath path, areas) {
            helper.drawPath(path);
        }
    }
    painter->restore();

    painter->save();
    QPalette systemPalette = d->renderInterface()->systemPaletteForDisplayColorSpace();
    QIcon icon = contourModeActive? KisIconUtils::loadIcon(ICON_EXIT): KisIconUtils::loadIcon(ICON_ENTER);
    QImage pm = d->renderInterface()->convertImageToDisplayColorSpace(icon.pixmap(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE).toImage());
    painter->setBrush(contourModeActive? systemPalette.highlight(): systemPalette.button());
    QPen pen;
    pen.setColor(contourModeActive? systemPalette.highlightedText().color(): systemPalette.buttonText().color());
    pen.setCosmetic(true);
    pen.setWidthF(d->decorationThickness);
    painter->setPen(pen);
    const QRectF buttonRect = d->getButtonRect(converter.documentToView().mapRect(text->boundingRect()));
    painter->drawRoundedRect(buttonRect, BUTTON_CORNER_ROUND, BUTTON_CORNER_ROUND);
    painter->drawImage(buttonRect.topLeft()+QPointF(BUTTON_PADDING, BUTTON_PADDING), pm);
    painter->restore();
}

void KoSvgTextShapeOutlineHelper::paint(QPainter *painter, const KoViewConverter &converter)
{
    KoSvgTextShape *text = d->getTextModeShape();
    if (text) {
        paintTextShape(painter, converter, text, true);
    } else {
        Q_FOREACH(KoShape* shape, d->canvas->shapeManager()->selection()->selectedEditableShapes()) {
            text = dynamic_cast<KoSvgTextShape*>(shape);
            if (d->drawButton(text)) {
                paintTextShape(painter, converter, text, false);
            }
        }
    }
}

QRectF KoSvgTextShapeOutlineHelper::decorationRect()
{
    QRectF decorationRect;
    KoSvgTextShape *text = d->getTextModeShape();
    if (text) {
        QRectF base = text->boundingRect();
        base |= d->getButtonRectCorrected(base);
        decorationRect = base;
    } else {
        Q_FOREACH(KoShape* shape, d->canvas->shapeManager()->selection()->selectedEditableShapes()) {
            text = dynamic_cast<KoSvgTextShape*>(shape);
            if (d->drawButton(text)) {
                QRectF base = text->boundingRect();
                base |= d->getButtonRectCorrected(base);
                decorationRect |= base;
            }
        }
    }
    return decorationRect;
}

void KoSvgTextShapeOutlineHelper::setDrawBoundingRect(bool enable)
{
    d->drawBoundingRect = enable;
}

bool KoSvgTextShapeOutlineHelper::drawBoundingRect() const
{
    return d->drawBoundingRect;
}

void KoSvgTextShapeOutlineHelper::setDrawTextWrappingArea(bool enable)
{
    d->drawTextWrappingArea = enable;
}

void KoSvgTextShapeOutlineHelper::setDrawShapeOutlines(bool enable)
{
    d->drawOutline = enable;
}

bool KoSvgTextShapeOutlineHelper::drawShapeOutlines() const
{
    return d->drawOutline;
}

void KoSvgTextShapeOutlineHelper::setHandleRadius(int radius)
{
    d->handleRadius = radius;
}

void KoSvgTextShapeOutlineHelper::setDecorationThickness(int thickness)
{
    d->decorationThickness = thickness;
}

KoSvgTextShape *KoSvgTextShapeOutlineHelper::contourModeButtonHovered(const QPointF &point)
{
    KoSvgTextShape *text = d->getTextModeShape();
    if (text) {
        if (d->getButtonRect(text->boundingRect()).contains(point)) {
            return text;
        }
    }
    return d->getPotentialTextShape(point);
}

bool KoSvgTextShapeOutlineHelper::updateTextContourMode()
{
    KoSvgTextShape *text = d->getTextModeShape();
    if (text && !d->drawButton(text)) {
        toggleTextContourMode(nullptr);
        return true;
    }
    return false;
}

void KoSvgTextShapeOutlineHelper::toggleTextContourMode(KoSvgTextShape *shape)
{
    if (d->canvas) {
        if (shape == d->canvas->currentShapeManagerOwnerShape()) {
            d->canvas->setCurrentShapeManagerOwnerShape(nullptr);
        } else {
            d->canvas->setCurrentShapeManagerOwnerShape(shape);
        }
    }
}

void KoSvgTextShapeOutlineHelper::setTextAreasHovered(bool enabled)
{
    d->textWrappingAreasHovered = enabled;
}
