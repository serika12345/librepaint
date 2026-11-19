/*
 *  SPDX-FileCopyrightText: 2018 The Chromium Authors
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KIS_KALMAN_STROKE_PREDICTOR_H
#define KIS_KALMAN_STROKE_PREDICTOR_H

#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>

#include <QScopedPointer>

#include <kritatools_export.h>

/**
 * The Kalman predictor of the stroke end, ported from Chromium's
 * ui/base/prediction/kalman_predictor. It holds a filter for each axis and a
 * third filter that smooths the interval between samples.
 *
 * The prediction is one point at the latest real point plus
 * velocity * pred_dt + 0.5 * acceleration * pred_dt^2, where pred_dt is the
 * horizon times the confidence. The confidence is the speed factor and the
 * pressure factor of KisStrokePrediction. The anchor polyline comes from
 * KisStrokeSampleHistory. The maximum move is left to KisStrokePredictionCap,
 * so the settings lose their maxMovePx, and the deceleration settings are not
 * used.
 *
 * With jerkEnabled the confidence is multiplied by the jerk factor of
 * AndroidX: a change of the estimated acceleration of 0.1 px per sample
 * leaves it alone and 0.2 px per sample removes it. The result keeps the
 * Kalman source, and source() reports KalmanJerk so that the latency probe
 * can tell the two settings apart.
 */
class KRITATOOLS_EXPORT KisKalmanStrokePredictor : public KisStrokePredictor
{
public:
    explicit KisKalmanStrokePredictor(const KisStrokePredictionSettings &settings,
                                      bool jerkEnabled = false);
    ~KisKalmanStrokePredictor() override;

    void reset() override;
    void addSample(const KisStrokePredictionSample &sample) override;
    KisStrokePredictionResult predict(qreal horizonMs) const override;
    KisStrokePredictionSource source() const override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_KALMAN_STROKE_PREDICTOR_H
