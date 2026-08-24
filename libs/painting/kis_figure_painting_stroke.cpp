/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_figure_painting_stroke.h>

#include <KoCanvasResourceProvider.h>

#include "kis_resources_snapshot.h"
#include <kis_distance_information.h>
#include "kis_image.h"
#include "kis_painter.h"
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/freehand_stroke.h>
#include "KisAsynchronousStrokeUpdateHelper.h"


KisFigurePaintingStroke::KisFigurePaintingStroke(
    const KUndo2MagicString &name,
    KisImageWSP image,
    KisNodeSP currentNode,
    KoCanvasResourceProvider *resourceManager,
    KisFigurePaintingOptions::StrokeStyle strokeStyle,
    KisFigurePaintingOptions::FillStyle fillStyle,
    QTransform fillTransform)
{
    m_strokesFacade = image.data();

    m_resources =
        new KisResourcesSnapshot(image,
                                 currentNode,
                                 resourceManager->canvasResourcesInterface());

    setupPaintStyles(m_resources, strokeStyle, fillStyle, fillTransform);

    KisFreehandStrokeInfo *strokeInfo = new KisFreehandStrokeInfo();

    KisStrokeStrategy *stroke =
        new FreehandStrokeStrategy(m_resources, strokeInfo, name);

    m_strokeId = m_strokesFacade->startStroke(stroke);
}

void KisFigurePaintingStroke::setupPaintStyles(
    KisResourcesSnapshotSP resources,
    KisFigurePaintingOptions::StrokeStyle strokeStyle,
    KisFigurePaintingOptions::FillStyle fillStyle,
    QTransform fillTransform)
{
    using namespace KisFigurePaintingOptions;

    const KoColor fgColor = resources->currentFgColor();
    const KoColor bgColor = resources->currentBgColor();

    switch (strokeStyle) {
    case StrokeStyleNone:
        resources->setStrokeStyle(KisPainter::StrokeStyleNone);
        break;
    case StrokeStyleForeground:
        resources->setStrokeStyle(KisPainter::StrokeStyleBrush);
        break;
    case StrokeStyleBackground:
        resources->setStrokeStyle(KisPainter::StrokeStyleBrush);

        resources->setFGColorOverride(bgColor);
        resources->setBGColorOverride(fgColor);

        if (fillStyle == FillStyleForegroundColor) {
            fillStyle = FillStyleBackgroundColor;
        } else if (fillStyle == FillStyleBackgroundColor) {
            fillStyle = FillStyleForegroundColor;
        }

        break;
    };

    switch (fillStyle) {
    case FillStyleForegroundColor:
        resources->setFillStyle(KisPainter::FillStyleForegroundColor);
        break;
    case FillStyleBackgroundColor:
        resources->setFillStyle(KisPainter::FillStyleBackgroundColor);
        break;
    case FillStylePattern:
        resources->setFillStyle(KisPainter::FillStylePattern);
        break;
    case FillStyleNone:
        resources->setFillStyle(KisPainter::FillStyleNone);
        break;
    }

    resources->setFillTransform(fillTransform);
}

KisFigurePaintingStroke::~KisFigurePaintingStroke()
{
    m_strokesFacade->addJob(m_strokeId,
        new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
    m_strokesFacade->endStroke(m_strokeId);
}

void KisFigurePaintingStroke::paintLine(const KisPaintInformation &pi0,
                                        const KisPaintInformation &pi1)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         pi0, pi1));
}

void KisFigurePaintingStroke::paintPolyline(const vQPointF &points)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::POLYLINE,
                                         points));
}

void KisFigurePaintingStroke::paintPolygon(const vQPointF &points)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::POLYGON,
                                         points));
}

void KisFigurePaintingStroke::paintRect(const QRectF &rect)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::RECT,
                                         rect));
}

void KisFigurePaintingStroke::paintEllipse(const QRectF &rect)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::ELLIPSE,
                                         rect));
}

void KisFigurePaintingStroke::paintPainterPath(const QPainterPath &path)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::PAINTER_PATH,
                                         path));
}

void KisFigurePaintingStroke::setFGColorOverride(const KoColor &color)
{
    m_resources->setFGColorOverride(color);
}

void KisFigurePaintingStroke::setBGColorOverride(const KoColor &color)
{
    m_resources->setBGColorOverride(color);
}

void KisFigurePaintingStroke::setSelectionOverride(KisSelectionSP m_selection)
{
    m_resources->setSelectionOverride(m_selection);
}

void KisFigurePaintingStroke::setBrush(const KisPaintOpPresetSP &brush)
{
    m_resources->setBrush(brush);
}

void KisFigurePaintingStroke::paintPainterPathQPen(const QPainterPath path,
                                                   const QPen &pen,
                                                   const KoColor &color)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::QPAINTER_PATH,
                                         path, pen, color));
}

void KisFigurePaintingStroke::paintPainterPathQPenFill(
    const QPainterPath path,
    const QPen &pen,
    const KoColor &color)
{
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::QPAINTER_PATH_FILL,
                                         path, pen, color));
}
