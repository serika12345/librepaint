/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionReplay.h"

#include "KisKalmanStrokePredictor.h"
#include "KisLinearStrokePredictor.h"
#include "KisStrokePrediction.h"
#include "KisStrokePredictionCap.h"

#include <QFile>
#include <QJsonObject>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <memory>

namespace
{
constexpr qreal MinimumGapDeadlineMs = 8.0;
constexpr int GapIntervalCount = 8;
constexpr qreal GapDeadlineFactor = 2.5;
constexpr qreal StopDirectionWindowMs = 12.0;
constexpr qreal StopWindowMs = 200.0;

qreal medianOf(QVector<qreal> values)
{
    if (values.isEmpty()) return 0.0;
    std::sort(values.begin(), values.end());
    const int size = values.size();
    if (size % 2 == 1) return values.at(size / 2);
    return (values.at(size / 2 - 1) + values.at(size / 2)) / 2.0;
}

/// Deadline of the display timer after a sample: the fixed gap reset or the
/// median of the recent arrival intervals. The timer bounds the wait for a
/// pointer event, so it has to stay above the floor.
qreal expiryDeadlineMs(const QVector<qreal> &intervals,
                       const KisStrokePredictionReplaySettings &settings)
{
    if (!settings.gapExpiry) return settings.gapResetMs;

    QVector<qreal> recent;
    const int first = std::max(0, int(intervals.size()) - GapIntervalCount);
    for (int i = first; i < intervals.size(); ++i) {
        recent.append(intervals.at(i));
    }
    return std::max(GapDeadlineFactor * medianOf(recent), MinimumGapDeadlineMs);
}

/// One change of the displayed state. The line end is a prediction tip while
/// the predictor is valid and the last reflected real point otherwise.
struct DisplayStep {
    qreal timeMs = 0.0;
    QPointF lastReal;
    QPointF end;
};

/// Frames collected after the last sample arrived, in frame order.
struct StrokeTimeline {
    QVector<DisplayStep> steps;
    QVector<QPointF> predictionAtSample;
    QVector<bool> predictionValid;
    int midStrokeExpiry = 0;
};

StrokeTimeline buildTimeline(const QVector<KisStrokePredictionReplaySample> &ordered,
                             const KisStrokePredictionReplaySettings &settings)
{
    StrokeTimeline timeline;
    timeline.predictionAtSample.resize(ordered.size());
    timeline.predictionValid.fill(false, ordered.size());

    KisStrokePredictionSettings predictorSettings;
    predictorSettings.horizonMs = settings.horizonMs;
    predictorSettings.anchorMs = settings.anchorMs;
    if (!settings.decel) {
        predictorSettings.decelRatioZero = 0.0;
        predictorSettings.decelRatioFull = 0.01;
    }
    std::unique_ptr<KisStrokePredictor> predictor;
    if (settings.predictor == QStringLiteral("kalman")) {
        predictor = std::make_unique<KisKalmanStrokePredictor>(predictorSettings);
    } else {
        predictor = std::make_unique<KisLinearStrokePredictor>(predictorSettings);
    }
    const qreal maxMovePx =
        settings.capDiameters > 0.0 ? settings.capDiameters * settings.diameter : 0.0;

    const qreal lastArrivalMs = ordered.constLast().arrivalMs;

    QVector<qreal> intervals;
    qreal pendingExpiryMs = -1.0;
    QPointF lastReal;

    for (int i = 0; i < ordered.size(); ++i) {
        const KisStrokePredictionReplaySample &sample = ordered.at(i);

        if (pendingExpiryMs >= 0.0 && pendingExpiryMs < sample.arrivalMs) {
            DisplayStep step;
            step.timeMs = pendingExpiryMs;
            step.lastReal = lastReal;
            step.end = lastReal;
            timeline.steps.append(step);
            if (pendingExpiryMs < lastArrivalMs) {
                ++timeline.midStrokeExpiry;
            }
            pendingExpiryMs = -1.0;
        }

        KisStrokePredictionSample predictionSample;
        predictionSample.documentPos = sample.position;
        predictionSample.timeMs = sample.eventMs;
        predictionSample.pressure = sample.pressure;
        predictor->addSample(predictionSample);

        KisStrokePredictionResult result = predictor->predict(settings.horizonMs);
        KisStrokePredictionCap::apply(result, maxMovePx);
        const bool valid = result.valid && !result.polyline.isEmpty();

        timeline.predictionValid[i] = valid;
        if (valid) {
            timeline.predictionAtSample[i] = result.polyline.constLast();
        }

        DisplayStep step;
        step.timeMs = sample.arrivalMs;
        step.lastReal = sample.position;
        step.end = valid ? result.polyline.constLast() : sample.position;
        timeline.steps.append(step);

        lastReal = sample.position;

        if (i > 0) {
            intervals.append(sample.arrivalMs - ordered.at(i - 1).arrivalMs);
        }
        pendingExpiryMs = sample.arrivalMs + expiryDeadlineMs(intervals, settings);
    }

    if (pendingExpiryMs >= 0.0) {
        DisplayStep step;
        step.timeMs = pendingExpiryMs;
        step.lastReal = lastReal;
        step.end = lastReal;
        timeline.steps.append(step);
    }

    return timeline;
}

/// Direction of travel at the end of the stroke: from the real point 12 ms
/// before the last one to the last one.
QPointF stopDirection(const QVector<KisStrokePredictionReplaySample> &ordered)
{
    const KisStrokePredictionReplaySample &last = ordered.constLast();
    int start = 0;
    for (int i = 0; i < ordered.size(); ++i) {
        if (ordered.at(i).eventMs <= last.eventMs - StopDirectionWindowMs) {
            start = i;
        }
    }

    QPointF direction = last.position - ordered.at(start).position;
    const qreal length = std::hypot(direction.x(), direction.y());
    return length > 0.0 ? direction / length : QPointF(0.0, 0.0);
}

qreal projectOn(const QPointF &value, const QPointF &direction)
{
    return value.x() * direction.x() + value.y() * direction.y();
}

KisStrokePredictionReplayStat summarize(QVector<qreal> values)
{
    KisStrokePredictionReplayStat stat;

    if (values.isEmpty()) return stat;

    std::sort(values.begin(), values.end());
    const int size = values.size();

    stat.n = size;
    stat.p50 = size % 2 == 1
        ? values.at(size / 2)
        : (values.at(size / 2 - 1) + values.at(size / 2)) / 2.0;
    stat.p95 = values.at(std::min(size - 1, int(0.95 * size)));
    stat.max = values.constLast();

    return stat;
}

QJsonObject statJson(const KisStrokePredictionReplayStat &stat)
{
    QJsonObject object;
    object.insert(QStringLiteral("n"), stat.n);
    if (stat.n > 0) {
        object.insert(QStringLiteral("p50"), stat.p50);
        object.insert(QStringLiteral("p95"), stat.p95);
        object.insert(QStringLiteral("max"), stat.max);
    }
    return object;
}

/// Value of the line end after the last sample arrived, per frame. The frame
/// shows the state that the samples and the expiries reached pipelineMs
/// earlier.
QVector<qreal> stopProjections(const QVector<KisStrokePredictionReplaySample> &ordered,
                               const StrokeTimeline &timeline,
                               const QPointF &direction,
                               const KisStrokePredictionReplaySettings &settings)
{
    QVector<qreal> projections;

    const qreal firstArrivalMs = ordered.constFirst().arrivalMs;
    const qreal lastArrivalMs = ordered.constLast().arrivalMs;
    const QPointF lastPosition = ordered.constLast().position;

    int stepIndex = 0;
    const DisplayStep *current = nullptr;

    for (int frame = 0;; ++frame) {
        const qreal frameMs = firstArrivalMs + frame * settings.frameMs;
        if (frameMs > lastArrivalMs + StopWindowMs) break;

        const qreal shownMs = frameMs - settings.pipelineMs;
        while (stepIndex < timeline.steps.size()
               && timeline.steps.at(stepIndex).timeMs <= shownMs) {
            current = &timeline.steps.at(stepIndex);
            ++stepIndex;
        }

        if (shownMs < lastArrivalMs) continue;

        const QPointF end = current ? current->end : lastPosition;
        projections.append(projectOn(end - lastPosition, direction));
    }

    return projections;
}

/// Projection and distance of the predicted point against the real position
/// one horizon later, for every sample whose prediction is valid and whose
/// horizon stays inside the sampled times.
void sampleDistances(const QVector<KisStrokePredictionReplaySample> &ordered,
                     const StrokeTimeline &timeline,
                     const QPointF &direction,
                     qreal horizonMs,
                     QVector<qreal> *ahead,
                     QVector<qreal> *error)
{
    for (int i = 0; i < ordered.size(); ++i) {
        if (!timeline.predictionValid.at(i)) continue;

        const qreal targetMs = ordered.at(i).eventMs + horizonMs;

        int bracket = -1;
        for (int j = 0; j + 1 < ordered.size(); ++j) {
            const qreal fromMs = ordered.at(j).eventMs;
            const qreal toMs = ordered.at(j + 1).eventMs;
            if (toMs > fromMs && fromMs <= targetMs && targetMs <= toMs) {
                bracket = j;
                break;
            }
        }
        if (bracket < 0) continue;

        const qreal fromMs = ordered.at(bracket).eventMs;
        const qreal toMs = ordered.at(bracket + 1).eventMs;
        const qreal fraction = (targetMs - fromMs) / (toMs - fromMs);

        const QPointF actual = ordered.at(bracket).position
            + fraction * (ordered.at(bracket + 1).position - ordered.at(bracket).position);
        const QPointF difference = timeline.predictionAtSample.at(i) - actual;

        ahead->append(projectOn(difference, direction));
        error->append(std::hypot(difference.x(), difference.y()));
    }
}
} // namespace

bool loadKisStrokePredictionReplayInput(const QString &path,
                                        QVector<KisStrokePredictionReplayStroke> *strokes,
                                        QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *error = QStringLiteral("cannot open %1").arg(path);
        return false;
    }

    QTextStream stream(&file);
    int lineNumber = 0;
    QString currentStroke;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        ++lineNumber;
        if (line.isEmpty()) continue;
        if (lineNumber == 1 && line.startsWith(QStringLiteral("stroke"))) continue;

        const QStringList fields = line.split(QLatin1Char(','));
        if (fields.size() < 6) {
            *error = QStringLiteral("%1:%2: expected 6 columns").arg(path).arg(lineNumber);
            return false;
        }

        bool arrivalOk = false;
        bool eventOk = false;
        bool xOk = false;
        bool yOk = false;
        bool pressureOk = false;

        const double arrivalNs = fields.at(1).toDouble(&arrivalOk);
        const double eventMs = fields.at(2).toDouble(&eventOk);
        const double x = fields.at(3).toDouble(&xOk);
        const double y = fields.at(4).toDouble(&yOk);
        const double pressure = fields.at(5).toDouble(&pressureOk);

        if (!arrivalOk || !eventOk || !xOk || !yOk || !pressureOk) {
            *error = QStringLiteral("%1:%2: a field is not a number").arg(path).arg(lineNumber);
            return false;
        }

        if (fields.at(0) != currentStroke) {
            currentStroke = fields.at(0);
            strokes->append(KisStrokePredictionReplayStroke());
        }
        if (strokes->isEmpty()) {
            *error = QStringLiteral("%1:%2: a row came before its stroke").arg(path).arg(lineNumber);
            return false;
        }

        KisStrokePredictionReplaySample sample;
        sample.arrivalMs = arrivalNs / 1e6;
        sample.eventMs = eventMs;
        sample.position = QPointF(x, y);
        sample.pressure = pressure;

        strokes->last().samples.append(sample);
    }

    return true;
}

KisStrokePredictionReplayReport runKisStrokePredictionReplay(
    const QVector<KisStrokePredictionReplayStroke> &strokes,
    const KisStrokePredictionReplaySettings &settings)
{
    QVector<qreal> stopOvershoot;
    QVector<qreal> stopRetract;
    QVector<qreal> ahead;
    QVector<qreal> error;
    QVector<qreal> midStrokeExpiry;

    for (const KisStrokePredictionReplayStroke &stroke : strokes) {
        if (stroke.samples.isEmpty()) continue;

        QVector<KisStrokePredictionReplaySample> ordered = stroke.samples;
        std::sort(ordered.begin(), ordered.end(),
                  [](const KisStrokePredictionReplaySample &a,
                     const KisStrokePredictionReplaySample &b) {
                      return a.arrivalMs < b.arrivalMs;
                  });

        const StrokeTimeline timeline = buildTimeline(ordered, settings);
        const QPointF direction = stopDirection(ordered);
        const QVector<qreal> projections =
            stopProjections(ordered, timeline, direction, settings);

        qreal overshoot = 0.0;
        qreal retract = 0.0;
        if (!projections.isEmpty()) {
            int peak = 0;
            for (int i = 1; i < projections.size(); ++i) {
                if (projections.at(i) > projections.at(peak)) {
                    peak = i;
                }
            }
            overshoot = std::max(0.0, projections.at(peak));
            for (int i = peak; i + 1 < projections.size(); ++i) {
                retract = std::max(retract, projections.at(i) - projections.at(i + 1));
            }
        }

        stopOvershoot.append(overshoot);
        stopRetract.append(retract);
        midStrokeExpiry.append(timeline.midStrokeExpiry);

        sampleDistances(ordered, timeline, direction, settings.horizonMs, &ahead, &error);
    }

    KisStrokePredictionReplayReport report;
    report.stopOvershootPx = summarize(stopOvershoot);
    report.stopRetractPx = summarize(stopRetract);
    report.aheadPx = summarize(ahead);
    report.predictionErrorPx = summarize(error);
    report.midStrokeExpiry = summarize(midStrokeExpiry);

    return report;
}

QJsonObject kisStrokePredictionReplayReportJson(
    const KisStrokePredictionReplayReport &report,
    const KisStrokePredictionReplaySettings &settings)
{
    QJsonObject arguments;
    arguments.insert(QStringLiteral("predictor"), settings.predictor);
    arguments.insert(QStringLiteral("horizon_ms"), settings.horizonMs);
    arguments.insert(QStringLiteral("anchor_ms"), settings.anchorMs);
    arguments.insert(QStringLiteral("diameter"), settings.diameter);
    arguments.insert(QStringLiteral("cap_diameters"), settings.capDiameters);
    arguments.insert(QStringLiteral("decel"), settings.decel ? QStringLiteral("on")
                                                            : QStringLiteral("off"));
    arguments.insert(QStringLiteral("expiry"), settings.gapExpiry ? QStringLiteral("gap")
                                                                 : QStringLiteral("fixed"));
    arguments.insert(QStringLiteral("gap_reset_ms"), settings.gapResetMs);
    arguments.insert(QStringLiteral("frame_ms"), settings.frameMs);
    arguments.insert(QStringLiteral("pipeline_ms"), settings.pipelineMs);

    QJsonObject root;
    root.insert(QStringLiteral("arguments"), arguments);
    root.insert(QStringLiteral("replay.stop_overshoot_px"), statJson(report.stopOvershootPx));
    root.insert(QStringLiteral("replay.stop_retract_px"), statJson(report.stopRetractPx));
    root.insert(QStringLiteral("replay.ahead_px"), statJson(report.aheadPx));
    root.insert(QStringLiteral("replay.pred_error_px"), statJson(report.predictionErrorPx));
    root.insert(QStringLiteral("replay.mid_stroke_expiry"), statJson(report.midStrokeExpiry));

    return root;
}
