/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_H
#define KIS_STROKE_PREDICTION_H

#include <QPointF>
#include <QScopedPointer>
#include <QVector>

#include <kritatools_export.h>

/**
 * Settings of the stroke predictor. The ranges are applied by
 * KisStrokePrediction::setSettings().
 */
struct KisStrokePredictionSettings {
    qreal horizonMs = 16.7;          // 8 to 25
    qreal anchorMs = 24.0;           // 0 to 100
    qreal velocityWindowMs = 12.0;   // 4 to 50
    qreal gapResetMs = 20.0;         // 5 to 100
    qreal speedZeroPxPerMs = 0.2;    // confidence 0 at or below this speed
    qreal speedFullPxPerMs = 0.6;    // confidence 1 at or above this speed
    qreal pressureZero = 0.05;       // confidence 0 at or below this pressure
    qreal pressureFull = 0.15;       // confidence 1 at or above this pressure
    qreal minMovePx = 0.5;           // no polyline below this move length
};

/**
 * One prediction of the stroke predictor, in document coordinates.
 */
struct KisStrokePredictionResult {
    bool valid = false;               // use polyline only when true
    QVector<QPointF> polyline;        // anchor point ... latest point ... prediction
    int latestIndex = -1;             // position of the latest real point
    QPointF velocity;                 // px/ms
    qreal speed = 0.0;                // px/ms
    qreal confidence = 0.0;           // 0 to 1
    qreal moveLength = 0.0;           // predicted move length px
};

/**
 * Predicts the last point of a freehand stroke from time-stamped samples.
 *
 * The predictor owns no thread, reads no clock and uses no random numbers:
 * the caller supplies the time of every sample. The same sequence of samples
 * always produces the same result. This class is the pure part of the
 * display-only prediction line; it does not draw and does not touch the
 * paint device.
 */
class KRITATOOLS_EXPORT KisStrokePrediction
{
public:
    KisStrokePrediction();
    ~KisStrokePrediction();

    /// Clamp the values to their documented ranges and keep them. The sample
    /// history is kept.
    void setSettings(const KisStrokePredictionSettings &settings);
    KisStrokePredictionSettings settings() const;

    /// Drop the sample history.
    void reset();

    /// Add one sample. Returns true when a gap or a clock going backwards
    /// dropped the history and this sample became the first one.
    bool addSample(const QPointF &pos, qreal timeMs, qreal pressure);

    KisStrokePredictionResult predict() const;

    int sampleCount() const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_STROKE_PREDICTION_H
