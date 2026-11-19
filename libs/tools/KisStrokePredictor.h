/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTOR_H
#define KIS_STROKE_PREDICTOR_H

#include <QPointF>

#include <kritatools_export.h>

struct KisStrokePredictionResult;

/**
 * One real point of a stroke, in document coordinates.
 */
struct KisStrokePredictionSample {
    QPointF documentPos; // real point in document coordinates
    qreal timeMs = 0.0;  // Qt event time of the point, KoPointerEvent::time()
    qreal pressure = 0.0;
    bool tablet = false; // KoPointerEvent::isTabletEvent()
};

/// Which implementation produced a prediction.
enum class KisStrokePredictionSource { None = 0, Linear = 1, Kalman = 2, IosSystem = 3, AndroidSystem = 4 };

/**
 * Common interface of the stroke predictors. The tool owns one predictor for
 * a stroke and asks it for the prediction at a length it chooses.
 */
class KRITATOOLS_EXPORT KisStrokePredictor
{
public:
    virtual ~KisStrokePredictor() = default;
    virtual void reset() = 0;
    virtual void addSample(const KisStrokePredictionSample &sample) = 0;
    /// horizonMs is how far past the latest real point the caller wants the
    /// prediction to reach, measured from that point's event time.
    virtual KisStrokePredictionResult predict(qreal horizonMs) const = 0;
    virtual KisStrokePredictionSource source() const = 0;
};

#endif // KIS_STROKE_PREDICTOR_H
