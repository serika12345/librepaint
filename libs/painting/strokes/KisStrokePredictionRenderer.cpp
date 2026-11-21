/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionRenderer.h"

#include <QBitArray>

#include <KisLatencyProbe.h>

#include "kis_distance_information.h"
#include "kis_paint_device.h"
#include "kis_paint_information.h"
#include "kis_painter.h"
#include "kis_paintop_preset.h"
#include "kis_paintop_settings.h"
#include "kis_resources_snapshot.h"
#include "kis_stroke_random_source.h"

struct Q_DECL_HIDDEN KisStrokePredictionRenderer::Private
{
    Private(KisResourcesSnapshotSP _resources,
            KisPaintDeviceSP _source,
            KisSelectionSP _selection,
            bool _indirectPainting,
            const QString &_indirectPaintingCompositeOp)
        : resources(_resources),
          source(_source),
          selection(_selection),
          indirectPainting(_indirectPainting),
          indirectPaintingCompositeOp(_indirectPaintingCompositeOp)
    {
    }

    KisResourcesSnapshotSP resources;
    KisPaintDeviceSP source;
    KisSelectionSP selection;
    const bool indirectPainting;
    const QString indirectPaintingCompositeOp;

    KisPaintDeviceSP devices[2];
    QScopedPointer<KisPainter> painters[2];
    int nextIndex = 0;
};

KisStrokePredictionRenderer::KisStrokePredictionRenderer(KisResourcesSnapshotSP resources,
                                                         KisPaintDeviceSP source,
                                                         KisSelectionSP selection,
                                                         bool indirectPainting,
                                                         const QString &indirectPaintingCompositeOp)
    : m_d(new Private(resources, source, selection, indirectPainting, indirectPaintingCompositeOp))
{
    for (int i = 0; i < 2; i++) {
        KisPaintDeviceSP device = new KisPaintDevice(source->colorSpace());
        device->setDefaultBounds(source->defaultBounds());

        KisPainter *painter = new KisPainter();
        painter->begin(device, !indirectPainting ? selection : nullptr);
        resources->setupPainter(painter);

        if (indirectPainting) {
            painter->setCompositeOpId(indirectPaintingCompositeOp);
            painter->setOpacityToUnit();
            painter->setChannelFlags(QBitArray());
        }

        // The painter keeps its own KisFakeRunnableStrokeJobsExecutor, so the
        // brush dab jobs are executed in place and render() returns after all
        // dabs have been written.
        m_d->devices[i] = device;
        m_d->painters[i].reset(painter);
    }
}

KisStrokePredictionRenderer::~KisStrokePredictionRenderer()
{
    for (int i = 0; i < 2; i++) {
        if (m_d->painters[i]) {
            m_d->painters[i]->end();
        }
    }
}

KisStrokePredictionRenderer::Result
KisStrokePredictionRenderer::render(const KisPaintInformation &lastReal,
                                    const KisDistanceInformation &distance,
                                    const KisStrokeRandomSource &random,
                                    const QVector<QPointF> &points,
                                    const QVector<qreal> &dtMs)
{
    if (points.isEmpty()) {
        return Result();
    }

    const qreal brushSize = m_d->resources->currentPaintOpPreset()->settings()->paintOpSize();
    const qreal margin = brushSize + 2.0;

    qreal minX = lastReal.pos().x();
    qreal maxX = minX;
    qreal minY = lastReal.pos().y();
    qreal maxY = minY;

    for (const QPointF &point : points) {
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }

    const QRectF bounds = QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).adjusted(-margin, -margin, margin, margin);
    const QRect rect = bounds.toAlignedRect();

    KisLatencyProbe::mark("prediction_render_begin", points.last().x(), points.last().y(), points.size());

    const int index = m_d->nextIndex;
    m_d->nextIndex = (m_d->nextIndex + 1) % 2;

    KisPaintDeviceSP device = m_d->devices[index];
    KisPainter *painter = m_d->painters[index].data();

    KisPainter::copyAreaOptimized(rect.topLeft(), m_d->source, device, rect);

    KisDistanceInformation distanceCopy(distance);
    KisStrokeRandomSource randomCopy(random);
    KisRandomSourceSP rnd = randomCopy.source();
    KisPerStrokeRandomSourceSP strokeRnd = randomCopy.perStrokeSource();

    const int startDabSeqNo = distanceCopy.currentDabSeqNo();

    KisPaintInformation previous = lastReal;

    for (int i = 0; i < points.size(); i++) {
        const qreal dt = i < dtMs.size() ? dtMs[i] : 0.0;

        KisPaintInformation pi(points[i],
                               lastReal.pressure(),
                               lastReal.xTilt(),
                               lastReal.yTilt(),
                               lastReal.rotation(),
                               lastReal.tangentialPressure(),
                               lastReal.perspective(),
                               lastReal.currentTime() + dt,
                               lastReal.drawingSpeed());

        pi.setCanvasRotation(lastReal.canvasRotation());
        pi.setCanvasMirroredH(lastReal.canvasMirroredH());
        pi.setCanvasMirroredV(lastReal.canvasMirroredV());
        pi.setTiltDirectionOffset(lastReal.tiltDirectionOffset());
        pi.setRandomSource(rnd);
        pi.setPerStrokeRandomSource(strokeRnd);

        painter->paintLine(previous, pi, &distanceCopy);
        previous = pi;
    }

    const int dabCount = distanceCopy.currentDabSeqNo() - startDabSeqNo;

    const QVector<QRect> dirtyRects = painter->takeDirtyRegion();
    QRect dirtyBounds;
    for (const QRect &dirtyRect : dirtyRects) {
        dirtyBounds |= dirtyRect;
    }

    if (!dirtyBounds.isEmpty() && !rect.contains(dirtyBounds)) {
        KisLatencyProbe::mark("prediction_render_clip", 0.0, 0.0, 1);
    }

    KisLatencyProbe::mark("prediction_render_end", rect.width(), rect.height(), dabCount);

    Result result;
    result.device = device;
    result.rect = rect;
    result.dabCount = dabCount;
    return result;
}
