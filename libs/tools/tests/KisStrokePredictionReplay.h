/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_REPLAY_H
#define KIS_STROKE_PREDICTION_REPLAY_H

#include <QPointF>
#include <QString>
#include <QVector>

class QJsonObject;

/**
 * Input settings of the prediction replay. The names follow the command line
 * arguments of KisStrokePredictionReplay.
 */
struct KisStrokePredictionReplaySettings {
    QString predictor = QStringLiteral("linear"); // the predictor under test
    qreal horizonMs = 16.7;
    qreal anchorMs = 24.0;
    qreal diameter = 10.0;
    qreal capDiameters = 3.0;        // 0 = no cap
    bool decel = true;               // false = deceleration factor off
    bool gapExpiry = false;          // false = fixed expiry, true = gap expiry
    qreal gapResetMs = 20.0;         // deadline of the fixed expiry
    qreal frameMs = 16.667;
    qreal pipelineMs = 0.0;
};

/**
 * One pointer event of a stroke. arrivalMs is when the tool saw the event,
 * eventMs is the event time the tool recorded for it.
 */
struct KisStrokePredictionReplaySample {
    qreal arrivalMs = 0.0;
    qreal eventMs = 0.0;
    QPointF position;
    qreal pressure = 1.0;
};

struct KisStrokePredictionReplayStroke {
    QVector<KisStrokePredictionReplaySample> samples;
};

/// Distribution of one metric over the strokes or samples of a replay.
struct KisStrokePredictionReplayStat {
    int n = 0;
    qreal p50 = 0.0;
    qreal p95 = 0.0;
    qreal max = 0.0;
};

struct KisStrokePredictionReplayReport {
    KisStrokePredictionReplayStat stopOvershootPx;
    KisStrokePredictionReplayStat stopRetractPx;
    KisStrokePredictionReplayStat aheadPx;
    KisStrokePredictionReplayStat predictionErrorPx;
    KisStrokePredictionReplayStat midStrokeExpiry;
    /// Valid predictions and samples of every stroke, summed over the replay.
    int validPredictions = 0;
    int predictionSamples = 0;
};

/**
 * Read the rows "stroke,arrival_ns,event_ms,x,y,pressure" written by
 * scripts/perf/pen-latency/prediction_replay_input.py. The header row is
 * skipped. Returns false and fills @p error when a row is malformed.
 */
bool loadKisStrokePredictionReplayInput(const QString &path,
                                        QVector<KisStrokePredictionReplayStroke> *strokes,
                                        QString *error);

/**
 * Run the predictor on every stroke with the expiry timer and the frame clock
 * of the tool and measure the line end after the stroke stops. The report
 * also counts the samples that produced a valid prediction.
 */
KisStrokePredictionReplayReport runKisStrokePredictionReplay(
    const QVector<KisStrokePredictionReplayStroke> &strokes,
    const KisStrokePredictionReplaySettings &settings);

/// JSON object of the report: one member per metric name and the arguments.
QJsonObject kisStrokePredictionReplayReportJson(
    const KisStrokePredictionReplayReport &report,
    const KisStrokePredictionReplaySettings &settings);

#endif // KIS_STROKE_PREDICTION_REPLAY_H
