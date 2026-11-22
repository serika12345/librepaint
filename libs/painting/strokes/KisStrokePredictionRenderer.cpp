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
#include "kis_paintop.h"
#include "KisRenderedDab.h"
#include "kis_paintop_preset.h"
#include "kis_paintop_settings.h"
#include "KisRunnableStrokeJobData.h"
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

        // The painter keeps its own KisFakeRunnableStrokeJobsExecutor, but the
        // pixel brush dabs are written only when the jobs returned by
        // doAsynchronousUpdate are run, so render() runs those jobs before it
        // returns.
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
                                    const QVector<qreal> &dtMs,
                                    KisPaintOp *realPaintOp)
{
    if (points.isEmpty()) {
        return Result();
    }

    const qreal paintOpSize = m_d->resources->currentPaintOpPreset()->settings()->paintOpSize();
    const qreal margin = paintOpSize + 4.0;

    auto renderedBounds = [&] (const QList<KisRenderedDab> &dabs) {
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

        QRectF bounds(QPointF(minX, minY), QPointF(maxX, maxY));

        for (const KisRenderedDab &dab : dabs) {
            bounds |= QRectF(dab.realBounds());
        }

        return bounds.adjusted(-margin, -margin, margin, margin).toAlignedRect();
    };

    KisLatencyProbe::mark("prediction_render_begin", points.last().x(), points.last().y(), points.size());

    const int index = m_d->nextIndex;
    m_d->nextIndex = (m_d->nextIndex + 1) % 2;

    KisPaintDeviceSP device = m_d->devices[index];
    KisPainter *painter = m_d->painters[index].data();
    KisPaintDeviceSP source = m_d->source;

    int seqBefore = -1;
    QList<KisRenderedDab> pendingDabs;
    if (realPaintOp) {
        pendingDabs = realPaintOp->renderPendingDabsForPreview(&seqBefore);
    }

    QRect rect = renderedBounds(pendingDabs);

    int dabCount = 0;
    int redrawCount = 0;

    while (true) {
        KisPainter::copyAreaOptimized(rect.topLeft(), source, device, rect);

        if (realPaintOp) {
            int seqAfter = -1;
            QList<KisRenderedDab> afterDabs = realPaintOp->renderPendingDabsForPreview(&seqAfter);

            if (seqAfter != seqBefore) {
                pendingDabs = afterDabs;
                rect = renderedBounds(pendingDabs);
                KisPainter::copyAreaOptimized(rect.topLeft(), source, device, rect);

                int seqRedrawn = -1;
                realPaintOp->renderPendingDabsForPreview(&seqRedrawn);

                if (seqRedrawn != seqAfter) {
                    KisLatencyProbe::mark("prediction_pending_race", 0, 0, 1);
                }

                seqBefore = seqAfter;
            }
        }

        if (realPaintOp && !pendingDabs.isEmpty()) {
            painter->bltFixed(rect, pendingDabs);
        }

        KisDistanceInformation distanceCopy(distance);
        KisRandomSourceSP rnd(new KisRandomSource(*random.source()));
        KisPerStrokeRandomSourceSP strokeRnd(new KisPerStrokeRandomSource(*random.perStrokeSource()));

        const int startDabSeqNo = distanceCopy.currentDabSeqNo();

        KisPaintInformation previous = lastReal;
        // KisPaintInformation::mix inherits the random sources from the starting
        // information, so the start of the prediction needs the copies as well.
        previous.setRandomSource(rnd);
        previous.setPerStrokeRandomSource(strokeRnd);

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

        dabCount = distanceCopy.currentDabSeqNo() - startDabSeqNo;

        if (painter->paintOp()) {
            int flushCount = 0;

            while (true) {
                QVector<KisRunnableStrokeJobData*> jobs;
                const bool needsMoreUpdates =
                    painter->paintOp()->doAsynchronousUpdate(jobs).second;

                for (KisRunnableStrokeJobData *job : jobs) {
                    job->run();
                    delete job;
                }

                if (jobs.isEmpty() && !needsMoreUpdates) {
                    break;
                }

                if (++flushCount >= 64) {
                    KisLatencyProbe::mark("prediction_render_flush_limit", 0, 0, 64);
                    break;
                }
            }
        }

        const QVector<QRect> dirtyRects = painter->takeDirtyRegion();
        QRect dirtyBounds;
        for (const QRect &dirtyRect : dirtyRects) {
            dirtyBounds |= dirtyRect;
        }
        for (const KisRenderedDab &dab : pendingDabs) {
            dirtyBounds |= dab.realBounds();
        }

        if (dirtyBounds.isEmpty() || rect.contains(dirtyBounds)) {
            break;
        }

        if (redrawCount >= 1) {
            KisLatencyProbe::mark("prediction_render_clip", 0.0, 0.0, 1);
            break;
        }

        redrawCount++;
        rect |= dirtyBounds;
    }

    KisLatencyProbe::mark("prediction_pending_dabs", 0, 0, pendingDabs.size());
    KisLatencyProbe::mark("prediction_render_end", rect.width(), rect.height(), dabCount);

    Result result;
    result.device = device;
    result.rect = rect;
    result.dabCount = dabCount;
    return result;
}
