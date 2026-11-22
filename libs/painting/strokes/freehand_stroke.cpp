/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "freehand_stroke.h"

#include <QElapsedTimer>
#include <KisLatencyProbe.h>
#include "KisPerStrokeRandomSource.h"
#include "KisQStringListFwd.h"
#include "kis_assert.h"
#include "kis_debug.h"
#include "kis_default_bounds_base.h"
#include "kis_painter.h"
#include "kis_painter_based_stroke_strategy.h"
#include "kis_paintop.h"

#include "kis_random_source.h"
#include "kis_resources_snapshot.h"
#include "kis_simple_stroke_strategy.h"
#include "kis_stroke_job_strategy.h"
#include "kis_stroke_strategy.h"
#include "kis_update_time_monitor.h"

#include <brushengine/kis_stroke_random_source.h>
#include <KisRunnableStrokeJobsInterface.h>
#include <KisRunnableStrokeJobUtils.h>
#include "FreehandStrokeRunnableJobDataWithUpdate.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include "KisStrokeEfficiencyMeasurer.h"
#include <KisStrokeSpeedMonitor.h>
#include <numeric>
#include <QtGlobal>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/KisMaskedFreehandStrokePainter.h>
#include <strokes/KisStrokePredictionRenderer.h>
#include <tuple>

#include "brushengine/kis_paintop_utils.h"
#include "KisAsynchronousStrokeUpdateHelper.h"
#include "kundo2magicstring.h"
#include "kis_paint_layer.h"
#include "kis_default_bounds.h"

struct FreehandStrokeStrategy::Private
{
    Private(KisResourcesSnapshotSP _resources)
        : resources(_resources),
          needsAsynchronousUpdates(_resources->presetNeedsAsynchronousUpdates())
    {
        if (needsAsynchronousUpdates) {
            timeSinceLastUpdate.start();
        }
    }

    Private(KisResourcesSnapshotSP _resources, int dabRandomSeed)
        : randomSource(dabRandomSeed)
        , resources(_resources)
        , needsAsynchronousUpdates(_resources->presetNeedsAsynchronousUpdates())
    {
        if (needsAsynchronousUpdates) {
            timeSinceLastUpdate.start();
        }
    }

    Private(const Private &rhs)
        : randomSource(rhs.randomSource),
          resources(rhs.resources),
          needsAsynchronousUpdates(rhs.needsAsynchronousUpdates)
    {
        if (needsAsynchronousUpdates) {
            timeSinceLastUpdate.start();
        }
        efficiencyMeasurer.setEnabled(rhs.efficiencyMeasurer.isEnabled());
    }

    KisStrokeRandomSource randomSource;
    KisResourcesSnapshotSP resources;

    KisStrokeEfficiencyMeasurer efficiencyMeasurer;

    QElapsedTimer timeSinceLastUpdate;
    int currentUpdatePeriod = 40;

    const bool needsAsynchronousUpdates = false;
    std::mutex updateEntryMutex;

    bool supportsPrediction = false;

    std::unique_ptr<KisStrokePredictionRenderer> predictionRenderer;
    bool indirectPrediction = false;

    KisPaintInformation lastPaintedInfo;
    bool hasLastPaintedInfo = false;

    QVector<QPointF> predictionPoints;
    QVector<qreal> predictionDtMs;
    std::atomic<bool> predictionChanged {false};

    std::mutex predictionRenderMutex;
    QRect lastPredictionRect;

    std::function<void(KisPaintDeviceSP, const QPointF &, const QPointF &)>
        predictionProjectionReaderForTest;
};

FreehandStrokeStrategy::FreehandStrokeStrategy(KisResourcesSnapshotSP resources,
                                               KisFreehandStrokeInfo *strokeInfo,
                                               const KUndo2MagicString &name,
                                               Flags flags)
    : KisPainterBasedStrokeStrategy(QLatin1String("FREEHAND_STROKE"), name,
                                    resources, strokeInfo),
      m_d(new Private(resources))
{
    init(flags);
}

FreehandStrokeStrategy::FreehandStrokeStrategy(KisResourcesSnapshotSP resources,
                                               KisFreehandStrokeInfo *strokeInfo,
                                               const KUndo2MagicString &name,
                                               Flags flags,
                                               int dabRandomSeed)
    : KisPainterBasedStrokeStrategy(QLatin1String("FREEHAND_STROKE"), name, resources, strokeInfo)
    , m_d(new Private(resources, dabRandomSeed))
{
    init(flags);
}

FreehandStrokeStrategy::FreehandStrokeStrategy(KisResourcesSnapshotSP resources,
                                               QVector<KisFreehandStrokeInfo*> strokeInfos,
                                               const KUndo2MagicString &name,
                                               Flags flags)
    : KisPainterBasedStrokeStrategy(QLatin1String("FREEHAND_STROKE"), name,
                                    resources, strokeInfos),
      m_d(new Private(resources))
{
    init(flags);
}

FreehandStrokeStrategy::FreehandStrokeStrategy(const FreehandStrokeStrategy &rhs, int levelOfDetail)
    : KisPainterBasedStrokeStrategy(rhs, levelOfDetail),
      m_d(new Private(*rhs.m_d))
{
    m_d->randomSource.setLevelOfDetail(levelOfDetail);
}

FreehandStrokeStrategy::~FreehandStrokeStrategy()
{
    KisStrokeSpeedMonitor::instance()->notifyStrokeFinished(m_d->efficiencyMeasurer.averageCursorSpeed(),
                                                            m_d->efficiencyMeasurer.averageRenderingSpeed(),
                                                            m_d->efficiencyMeasurer.averageFps(),
                                                            m_d->resources->currentPaintOpPreset());

    KisUpdateTimeMonitor::instance()->endStrokeMeasure();
}

void FreehandStrokeStrategy::init(Flags flags)
{
    setSupportsWrapAroundMode(true);
    setSupportsMaskingBrush(true);
    setSupportsIndirectPainting(true);
    setSupportsContinuedInterstrokeData(flags & SupportsContinuedInterstrokeData);
    setSupportsTimedMergeId(flags & SupportsTimedMergeId);

    m_d->supportsPrediction = flags & SupportsPrediction;

    enableJob(KisSimpleStrokeStrategy::JOB_DOSTROKE);

    if (m_d->needsAsynchronousUpdates) {
        /**
         * In case the paintop uses asynchronous updates, we should set priority to it,
         * because FPS is controlled separately, not by the queue's merging algorithm.
         */
        setBalancingRatioOverride(0.01); // set priority to updates
    }

    KisUpdateTimeMonitor::instance()->startStrokeMeasure();
    m_d->efficiencyMeasurer.setEnabled(KisStrokeSpeedMonitor::instance()->haveStrokeSpeedMeasurement());
}

void FreehandStrokeStrategy::initStrokeCallback()
{
    KisPainterBasedStrokeStrategy::initStrokeCallback();

    m_d->efficiencyMeasurer.notifyRenderingStarted();
}

void FreehandStrokeStrategy::paintersInitialized()
{
    // Reasons for the "prediction_render_setup" marks. The setup job
    // always writes exactly one mark, so that the cause for a missing
    // prediction renderer can be read from the measurement data:
    //   0: the renderer was created
    //   1: the stroke does not support prediction
    //   2: the target node is not a KisPaintLayer
    //   3: the stroke does not use exactly one masked painter
    //   4: the preset needs masking brush rendering
    //   5: the painter is missing or mirrors the stroke
    //   6: the target device has no bounds or wraps around the image
    //   7: a level-of-detail clone dropped the prediction
    static const qint64 ReasonCreated = 0;
    static const qint64 ReasonNotSupported = 1;
    static const qint64 ReasonNotPaintLayer = 2;
    static const qint64 ReasonNotSinglePainter = 3;
    static const qint64 ReasonMaskingBrush = 4;
    static const qint64 ReasonMirroring = 5;
    static const qint64 ReasonWrapAround = 6;

    if (!m_d->supportsPrediction) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonNotSupported);
        return;
    }

    KisPaintLayer *layer = dynamic_cast<KisPaintLayer*>(targetNode().data());
    if (!layer) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonNotPaintLayer);
        return;
    }

    if (numMaskedPainters() != 1) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonNotSinglePainter);
        return;
    }

    if (m_d->resources->needsMaskingBrushRendering()) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonMaskingBrush);
        return;
    }

    KisPainter *painter = mainPainter(0);
    if (!painter || painter->hasMirroring()) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonMirroring);
        return;
    }

    KisPaintDeviceSP device = targetDevice();
    if (!device || !device->defaultBounds() || device->defaultBounds()->wrapAroundMode()) {
        KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonWrapAround);
        return;
    }

    m_d->indirectPrediction = device != layer->paintDevice();
    m_d->predictionRenderer.reset(
        new KisStrokePredictionRenderer(m_d->resources,
                                        device,
                                        activeSelection(),
                                        m_d->indirectPrediction,
                                        m_d->resources->indirectPaintingCompositeOp()));

    KisLatencyProbe::mark("prediction_render_setup", 0, 0, ReasonCreated);
}

bool FreehandStrokeStrategy::hasPredictionRenderer() const
{
    return bool(m_d->predictionRenderer);
}

void FreehandStrokeStrategy::setPredictionProjectionReaderForTest(
    std::function<void(KisPaintDeviceSP, const QPointF &, const QPointF &)> reader)
{
    m_d->predictionProjectionReaderForTest = std::move(reader);
}

void FreehandStrokeStrategy::finishStrokeCallback()
{
    clearLayerPrediction();

    m_d->efficiencyMeasurer.notifyRenderingFinished();
    KisPainterBasedStrokeStrategy::finishStrokeCallback();
}

void FreehandStrokeStrategy::cancelStrokeCallback()
{
    clearLayerPrediction();

    KisPainterBasedStrokeStrategy::cancelStrokeCallback();
}


FreehandStrokeStrategy::Data::~Data() = default;

void FreehandStrokeStrategy::doStrokeCallback(KisStrokeJobData *data)
{
    if (KisAsynchronousStrokeUpdateHelper::UpdateData *d =
            dynamic_cast<KisAsynchronousStrokeUpdateHelper::UpdateData*>(data)) {

        // this job is lod-clonable in contrast to FreehandStrokeRunnableJobDataWithUpdate!
        tryDoUpdate(d->forceUpdate);

    } else if (Data *d = dynamic_cast<Data*>(data)) {
        KisMaskedFreehandStrokePainter *maskedPainter = this->maskedPainter(d->strokeInfoId);

        KisUpdateTimeMonitor::instance()->reportPaintOpPreset(maskedPainter->preset());
        KisRandomSourceSP rnd = m_d->randomSource.source();
        KisPerStrokeRandomSourceSP strokeRnd = m_d->randomSource.perStrokeSource();

        switch(d->type) {
        case Data::POINT:
            d->pi1.setRandomSource(rnd);
            d->pi1.setPerStrokeRandomSource(strokeRnd);
            maskedPainter->paintAt(d->pi1);
            m_d->lastPaintedInfo = d->pi1;
            m_d->hasLastPaintedInfo = true;
            KisLatencyProbe::mark("stroke_dab", d->pi1.pos().x(), d->pi1.pos().y());
            m_d->efficiencyMeasurer.addSample(d->pi1.pos());
            break;
        case Data::LINE:
            d->pi1.setRandomSource(rnd);
            d->pi2.setRandomSource(rnd);
            d->pi1.setPerStrokeRandomSource(strokeRnd);
            d->pi2.setPerStrokeRandomSource(strokeRnd);
            maskedPainter->paintLine(d->pi1, d->pi2);
            m_d->lastPaintedInfo = d->pi2;
            m_d->hasLastPaintedInfo = true;
            KisLatencyProbe::mark("stroke_dab", d->pi2.pos().x(), d->pi2.pos().y());
            m_d->efficiencyMeasurer.addSample(d->pi2.pos());
            break;
        case Data::CURVE:
            d->pi1.setRandomSource(rnd);
            d->pi2.setRandomSource(rnd);
            d->pi1.setPerStrokeRandomSource(strokeRnd);
            d->pi2.setPerStrokeRandomSource(strokeRnd);
            maskedPainter->paintBezierCurve(d->pi1,
                                         d->control1,
                                         d->control2,
                                         d->pi2);
            m_d->lastPaintedInfo = d->pi2;
            m_d->hasLastPaintedInfo = true;
            KisLatencyProbe::mark("stroke_dab", d->pi2.pos().x(), d->pi2.pos().y());
            m_d->efficiencyMeasurer.addSample(d->pi2.pos());
            break;
        case Data::POLYLINE:
            maskedPainter->paintPolyline(d->points, 0, d->points.size());
            if (!d->points.isEmpty()) {
                KisLatencyProbe::mark("stroke_dab", d->points.last().x(), d->points.last().y());
            }
            m_d->efficiencyMeasurer.addSamples(d->points);
            break;
        case Data::POLYGON:
            maskedPainter->paintPolygon(d->points);
            if (!d->points.isEmpty()) {
                KisLatencyProbe::mark("stroke_dab", d->points.last().x(), d->points.last().y());
            }
            m_d->efficiencyMeasurer.addSamples(d->points);
            break;
        case Data::RECT:
            maskedPainter->paintRect(d->rect);
            KisLatencyProbe::mark("stroke_dab", d->rect.center().x(), d->rect.center().y());
            m_d->efficiencyMeasurer.addSample(d->rect.topLeft());
            m_d->efficiencyMeasurer.addSample(d->rect.topRight());
            m_d->efficiencyMeasurer.addSample(d->rect.bottomRight());
            m_d->efficiencyMeasurer.addSample(d->rect.bottomLeft());
            break;
        case Data::ELLIPSE:
            maskedPainter->paintEllipse(d->rect);
            KisLatencyProbe::mark("stroke_dab", d->rect.center().x(), d->rect.center().y());
            // TODO: add speed measures
            break;
        case Data::PAINTER_PATH:
            maskedPainter->paintPainterPath(d->path);
            KisLatencyProbe::mark("stroke_dab", d->path.boundingRect().center().x(), d->path.boundingRect().center().y());
            // TODO: add speed measures
            break;
        case Data::QPAINTER_PATH:
            maskedPainter->drawPainterPath(d->path, d->pen);
            KisLatencyProbe::mark("stroke_dab", d->path.boundingRect().center().x(), d->path.boundingRect().center().y());
            break;
        case Data::QPAINTER_PATH_FILL:
            maskedPainter->drawAndFillPainterPath(d->path, d->pen, d->customColor);
            KisLatencyProbe::mark("stroke_dab", d->path.boundingRect().center().x(), d->path.boundingRect().center().y());
            break;
        case Data::PREDICTION:
            m_d->predictionPoints = d->points;
            m_d->predictionDtMs = d->dtMs;
            m_d->predictionChanged = true;
            KisLatencyProbe::mark("prediction_received", 0, 0, d->points.size());
            break;
        };

        tryDoUpdate();
    } else {
        KisPainterBasedStrokeStrategy::doStrokeCallback(data);

        FreehandStrokeRunnableJobDataWithUpdate *dataWithUpdate =
            dynamic_cast<FreehandStrokeRunnableJobDataWithUpdate*>(data);

        if (dataWithUpdate) {
            tryDoUpdate();
        }
    }
}

void FreehandStrokeStrategy::tryDoUpdate(bool forceEnd)
{
    // we should enter this function only once!
    std::unique_lock<std::mutex> entryLock(m_d->updateEntryMutex, std::try_to_lock);
    if (!entryLock.owns_lock()) return;

    if (m_d->needsAsynchronousUpdates) {
        if (forceEnd || m_d->timeSinceLastUpdate.elapsed() > m_d->currentUpdatePeriod) {
            m_d->timeSinceLastUpdate.restart();

            for (int i = 0; i < numMaskedPainters(); i++) {
                KisMaskedFreehandStrokePainter *maskedPainter = this->maskedPainter(i);

                // TODO: well, we should count all N simultaneous painters for FPS rate!
                QVector<KisRunnableStrokeJobData*> jobs;

                bool needsMoreUpdates = false;

                std::tie(m_d->currentUpdatePeriod, needsMoreUpdates) =
                    maskedPainter->doAsynchronousUpdate(jobs);

                if (!jobs.isEmpty() ||
                    maskedPainter->hasDirtyRegion() ||
                    (forceEnd && needsMoreUpdates)) {

                    KritaUtils::addJobSequential(jobs,
                        [this] () {
                            this->issueSetDirtySignals();
                        }
                    );

                    if (forceEnd && needsMoreUpdates) {
                        KritaUtils::addJobSequential(jobs,
                            [this] () {
                                this->tryDoUpdate(true);
                            }
                        );
                    }


                    runnableJobsInterface()->addRunnableJobs(jobs);
                    m_d->efficiencyMeasurer.notifyFrameRenderingStarted();
                }

            }
        }
    } else {
        issueSetDirtySignals();
    }


}

void FreehandStrokeStrategy::issueSetDirtySignals()
{
    // The prediction starts from the last point whose dabs are reserved, so
    // dabs that paintLine reserved and the brush has not written yet leave a
    // gap at the seam between the confirmed line and the prediction. Write
    // them into the layer before the prediction is rendered.
    if (m_d->predictionRenderer &&
        m_d->predictionChanged &&
        !m_d->predictionPoints.isEmpty()) {

        KisLatencyProbe::mark("prediction_real_flush_begin");

        int flushedJobs = 0;
        int flushCount = 0;

        while (true) {
            QVector<KisRunnableStrokeJobData*> jobs;
            const bool needsMoreUpdates =
                maskedPainter(0)->doAsynchronousUpdate(jobs).second;

            for (KisRunnableStrokeJobData *job : jobs) {
                job->run();
                delete job;
                flushedJobs++;
            }

            if (jobs.isEmpty() && !needsMoreUpdates) {
                break;
            }

            if (++flushCount >= 64) {
                KisLatencyProbe::mark("prediction_real_flush_limit", 0, 0, 64);
                break;
            }
        }

        KisLatencyProbe::mark("prediction_real_flush_end", 0, 0, flushedJobs);
    }

    QVector<QRect> dirtyRects;

    for (int i = 0; i < numMaskedPainters(); i++) {
        KisMaskedFreehandStrokePainter *maskedPainter = this->maskedPainter(i);
        dirtyRects.append(maskedPainter->takeDirtyRegion());
    }

    auto queuePredictionRenderJob =
        [this] (QVector<KisRunnableStrokeJobData*> &jobs) {

        if (!m_d->predictionRenderer || !m_d->predictionChanged) return;

        const QVector<QPointF> points = m_d->predictionPoints;
        const QVector<qreal> dtMs = m_d->predictionDtMs;
        const KisPaintInformation lastPaintedInfo = m_d->lastPaintedInfo;
        const bool hasLastPaintedInfo = m_d->hasLastPaintedInfo;

        KisDistanceInformation distance;
        if (!points.isEmpty() && hasLastPaintedInfo && mainStrokeInfo(0)) {
            distance = *mainStrokeInfo(0)->dragDistance;
        }

        const bool indirectPrediction = m_d->indirectPrediction;
        const KisStrokeRandomSource randomSource = m_d->randomSource;

        KisPainter *const confirmedPainter = mainPainter(0);
        KisPaintOp *const realPaintOp = confirmedPainter ? confirmedPainter->paintOp() : nullptr;

        m_d->predictionChanged = false;

        KritaUtils::addJobConcurrent(jobs,
            [this, points, dtMs, lastPaintedInfo, hasLastPaintedInfo,
             distance, indirectPrediction, randomSource, realPaintOp] () {

                std::unique_lock<std::mutex> renderLock(m_d->predictionRenderMutex,
                                                        std::try_to_lock);
                if (!renderLock.owns_lock()) {
                    m_d->predictionChanged = true;
                    KisLatencyProbe::mark("prediction_render_skip", 0, 0, 1);
                    return;
                }

                KisPaintLayer *layer = dynamic_cast<KisPaintLayer*>(targetNode().data());
                if (!layer || !m_d->predictionRenderer) return;

                QRect currentRect;

                if (!points.isEmpty() && hasLastPaintedInfo) {
                    KisStrokePredictionRenderer::Result result =
                        m_d->predictionRenderer->render(lastPaintedInfo,
                                                        distance,
                                                        randomSource,
                                                        points,
                                                        dtMs,
                                                        realPaintOp);
                    if (!result.rect.isEmpty()) {
                        layer->setPrediction(result.device,
                                             result.rect,
                                             indirectPrediction ?
                                                 KisIndirectPaintingSupport::PredictionMode::ReplaceTemporaryTarget :
                                                 KisIndirectPaintingSupport::PredictionMode::ReplaceOriginal);
                        currentRect = result.rect;
                    } else {
                        layer->clearPrediction();
                    }
                } else {
                    layer->clearPrediction();
                }

                QVector<QRect> predictionRects;
                if (!m_d->lastPredictionRect.isEmpty()) {
                    predictionRects.append(m_d->lastPredictionRect);
                }
                if (!currentRect.isEmpty()) {
                    predictionRects.append(currentRect);
                }
                m_d->lastPredictionRect = currentRect;

                if (!predictionRects.isEmpty()) {
                    targetNode()->setDirty(predictionRects);
                }
            });

        if (m_d->predictionProjectionReaderForTest &&
            hasLastPaintedInfo &&
            !points.isEmpty()) {

            auto reader = m_d->predictionProjectionReaderForTest;
            const QPointF lastRealPosition = lastPaintedInfo.pos();
            const QPointF firstPredictedPosition = points.first();

            KritaUtils::addJobBarrier(jobs,
                [this, reader, lastRealPosition, firstPredictedPosition] () {
                    reader(targetNode()->projection(),
                           lastRealPosition,
                           firstPredictedPosition);
                });
        }
    };

    if (needsMaskingUpdates()) {

        // optimize the rects so that they would never intersect with each other!
        // that is a mandatory step for the multithreaded execution of merging jobs

        // sanity check: updates from the brush should have already been normalized
        //               to the wrapping rect
        const KisDefaultBoundsBaseSP defaultBounds = targetNode()->projection()->defaultBounds();
        if (defaultBounds->wrapAroundMode()) {
            const QRect wrapRect = defaultBounds->imageBorderRect();
            for (auto it = dirtyRects.begin(); it != dirtyRects.end(); ++it) {
                KIS_SAFE_ASSERT_RECOVER(wrapRect.contains(*it)) {
                    ENTER_FUNCTION() << ppVar(*it) << ppVar(wrapRect);
                    *it = *it & wrapRect;
                }
            }
        }

        const int maxPatchSizeForMaskingUpdates = 64;
        const QRect totalRect =
            std::accumulate(dirtyRects.constBegin(), dirtyRects.constEnd(), QRect(), std::bit_or<QRect>());

        dirtyRects = KisPaintOpUtils::splitAndFilterDabRect(totalRect, dirtyRects, maxPatchSizeForMaskingUpdates);

        QVector<KisRunnableStrokeJobData*> jobs = doMaskingBrushUpdates(dirtyRects);

        KritaUtils::addJobSequential(jobs,
            [this, dirtyRects] () {
                this->targetNode()->setDirty(dirtyRects);
            }
        );

        queuePredictionRenderJob(jobs);

        runnableJobsInterface()->addRunnableJobs(jobs);

    } else {
        targetNode()->setDirty(dirtyRects);

        QVector<KisRunnableStrokeJobData*> jobs;
        queuePredictionRenderJob(jobs);

        if (!jobs.isEmpty()) {
            runnableJobsInterface()->addRunnableJobs(jobs);
        }
    }

    //KisUpdateTimeMonitor::instance()->reportJobFinished(data, dirtyRects);
}

KisStrokeStrategy* FreehandStrokeStrategy::createLodClone(int levelOfDetail)
{
    if (!m_d->resources->presetAllowsLod()) return 0;
    if (!m_d->resources->currentNode()->supportsLodPainting()) return 0;

    KisLatencyProbe::mark("prediction_render_setup", 0, 0, 7);
    m_d->supportsPrediction = false;

    FreehandStrokeStrategy *clone = new FreehandStrokeStrategy(*this, levelOfDetail);
    clone->m_d->supportsPrediction = false;
    return clone;
}

void FreehandStrokeStrategy::clearLayerPrediction()
{
    std::unique_lock<std::mutex> renderLock(m_d->predictionRenderMutex);

    KisPaintLayer *layer = dynamic_cast<KisPaintLayer*>(targetNode().data());
    if (!layer) return;

    layer->clearPrediction();

    if (!m_d->lastPredictionRect.isEmpty()) {
        targetNode()->setDirty(m_d->lastPredictionRect);
        m_d->lastPredictionRect = QRect();
    }

    m_d->predictionChanged = false;
}

void FreehandStrokeStrategy::notifyUserStartedStroke()
{
    m_d->efficiencyMeasurer.notifyCursorMoveStarted();
}

void FreehandStrokeStrategy::notifyUserEndedStroke()
{
    m_d->efficiencyMeasurer.notifyCursorMoveFinished();
}
