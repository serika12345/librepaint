/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_figure_painting_stroke.h>

#include <KoCanvasResourceProvider.h>

#include "KisFigurePaintingOptions.h"
#include "kis_resources_snapshot.h"
#include <kis_distance_information.h>
#include "kis_image.h"
#include "kis_painter.h"
#include <qtransform.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/freehand_stroke.h>
#include "KisAsynchronousStrokeUpdateHelper.h"
#include "kis_stroke_strategy.h"
#include "kis_types.h"
#include "kis_node.h"
#include "kis_paint_device.h"
#include "kis_undo_stores.h"
#include "kundo2magicstring.h"
#include <KisFakeRunnableStrokeJobsExecutor.h>
#include <KisRunnableStrokeJobData.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_random_source.h>
#include <brushengine/kis_stroke_random_source.h>
#include <memory>
#include <strokes/KisMaskedFreehandStrokePainter.h>
#include <strokes/KisMaskingBrushRenderer.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <brushengine/kis_paintop.h>
#include <KoCompositeOpIds.h>


KisFigurePaintingStroke::KisFigurePaintingStroke(
    const KUndo2MagicString &name,
    KisImageWSP image,
    KisNodeSP currentNode,
    KoCanvasResourceProvider *resourceManager,
    KisFigurePaintingOptions::StrokeStyle strokeStyle,
    KisFigurePaintingOptions::FillStyle fillStyle,
    QTransform fillTransform)
{
    m_resources = new KisResourcesSnapshot(image, currentNode,
                                           resourceManager->canvasResourcesInterface());
    setupPaintStyles(m_resources, strokeStyle, fillStyle, fillTransform);
    startStroke(name);
}

KisFigurePaintingStroke::KisFigurePaintingStroke(
    const KUndo2MagicString &name,
    const KisResourcesSnapshot &resources,
    std::optional<int> dabRandomSeed)
{
    m_resources = new KisResourcesSnapshot(resources);
    startStroke(name, dabRandomSeed);
}

void KisFigurePaintingStroke::startStroke(const KUndo2MagicString &name, std::optional<int> dabRandomSeed)
{
    m_strokesFacade = m_resources->image().data();
    KisFreehandStrokeInfo *strokeInfo = new KisFreehandStrokeInfo();

    KisStrokeStrategy *stroke = dabRandomSeed
        ? new FreehandStrokeStrategy(m_resources, strokeInfo, name, FreehandStrokeStrategy::None, *dabRandomSeed)
        : new FreehandStrokeStrategy(m_resources, strokeInfo, name);

    m_strokeId = m_strokesFacade->startStroke(stroke);
}

KisFigurePaintingStroke::PreviewSample KisFigurePaintingStroke::createPreviewSample(
    const KisResourcesSnapshot &resources, const KisPaintInformation &information, int randomSeed)
{
    KisResourcesSnapshot captured(resources);
    const qreal originalSize = qMax(qreal(1.0), captured.currentPaintOpPreset()->settings()->paintOpSize());
    const qreal size = qMin(originalSize, qreal(64));
    captured.currentPaintOpPreset()->settings()->setPaintOpSize(size);
    KisPaintDeviceSP device = new KisPaintDevice(resources.currentNode()->paintDevice()->colorSpace());
    KisSelectionSP clip = new KisSelection();
    clip->pixelSelection()->select(QRect(0, 0, 256, 256));
    clip->updateProjection();
    KisFakeRunnableStrokeJobsExecutor executor;
    const bool indirect = captured.needsIndirectPainting();
    std::unique_ptr<KisMaskingBrushRenderer> masking;
    if (captured.needsMaskingBrushRendering()) {
        masking.reset(new KisMaskingBrushRenderer(device,
            captured.currentPaintOpPreset()->settings()->maskingBrushCompositeOp()));
    }
    KisFreehandStrokeInfo strokeInfo;
    KisFreehandStrokeInfo maskInfo;
    strokeInfo.painter->begin(masking ? masking->strokeDevice() : device, clip);
    strokeInfo.painter->setRunnableStrokeJobsInterface(&executor);
    captured.setupPainter(strokeInfo.painter);
    // The sample captures the brush material. Document selection, mirroring,
    // channel locks and compositing are applied once by the released stroke.
    strokeInfo.painter->setMirrorInformation(QPointF(), false, false);
    strokeInfo.painter->setChannelFlags(QBitArray());
    strokeInfo.painter->setCompositeOpId(indirect ? captured.indirectPaintingCompositeOp() : COMPOSITE_OVER);
    if (indirect) strokeInfo.painter->setOpacityToUnit();
    if (masking) {
        maskInfo.painter->begin(masking->maskDevice(), clip);
        maskInfo.painter->setRunnableStrokeJobsInterface(&executor);
        captured.setupMaskingBrushPainter(maskInfo.painter);
        maskInfo.painter->setMirrorInformation(QPointF(), false, false);
    }
    KisMaskedFreehandStrokePainter painter(&strokeInfo, masking ? &maskInfo : nullptr);
    KisPaintInformation start(information), end(information);
    KisStrokeRandomSource randomSources(randomSeed);
    KisRandomSourceSP random = randomSources.source();
    KisPerStrokeRandomSourceSP perStroke = randomSources.perStrokeSource();
    start.setPos(QPointF(64, 128));
    end.setPos(QPointF(192, 128));
    start.setRandomSource(random); end.setRandomSource(random);
    start.setPerStrokeRandomSource(perStroke); end.setPerStrokeRandomSource(perStroke);
    painter.paintLine(start, end);
    bool pending;
    do {
        QVector<KisRunnableStrokeJobData *> jobs;
        pending = painter.doAsynchronousUpdate(jobs).second;
        strokeInfo.painter->runnableStrokeJobsInterface()->addRunnableJobs(jobs);
    } while (pending);
    if (masking) masking->updateProjection(QRect(0, 0, 256, 256));
    const QRect pixels = device->exactBounds();
    const int radius = qBound(2, qMax(128 - pixels.top(), pixels.bottom() - 128) + 2, 63);
    const QRect crop(64 - radius, 128 - radius, 128 + 2 * radius + 1, 2 * radius + 1);
    KisPaintDeviceSP sample = new KisPaintDevice(device->colorSpace());
    KisPainter composite(sample);
    if (indirect) composite.setOpacityF(captured.opacity());
    composite.bitBlt(QPoint(), device, crop);
    return {sample, QRect(QPoint(), crop.size()),
            QLineF(radius, radius, radius + 128, radius), originalSize / size};
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

void KisFigurePaintingStroke::paintStrokePath(const QPainterPath &path, const KisPaintInformation &information)
{
    KisPaintInformation previous = information;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element element = path.elementAt(i);
        if (element.type == QPainterPath::MoveToElement) {
            previous.setPos(QPointF(element.x, element.y));
        } else if (element.type == QPainterPath::LineToElement) {
            KisPaintInformation next = information;
            next.setPos(QPointF(element.x, element.y));
            paintLine(previous, next);
            previous = next;
        } else if (element.type == QPainterPath::CurveToElement) {
            const QPainterPath::Element control2 = path.elementAt(i + 1);
            const QPainterPath::Element end = path.elementAt(i + 2);
            KisPaintInformation next = information;
            next.setPos(QPointF(end.x, end.y));
            m_strokesFacade->addJob(m_strokeId,
                new FreehandStrokeStrategy::Data(0, previous,
                    QPointF(element.x, element.y), QPointF(control2.x, control2.y), next));
            previous = next;
            i += 2;
        }
    }
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
