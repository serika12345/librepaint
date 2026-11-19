/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_KALMAN_STROKE_PREDICTOR_H
#define KIS_KALMAN_STROKE_PREDICTOR_H

#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>

#include <QScopedPointer>

#include <kritatools_export.h>

/**
 * A Kalman predictor of the stroke end with four states per axis: position,
 * velocity, acceleration and the change of the acceleration, following the
 * AndroidX stroke predictor. The confidence falls when the change of the
 * acceleration is large, so a stroke that starts or stops is predicted less
 * far. The maximum move is left to KisStrokePredictionCap, so the settings
 * lose their maxMovePx, and the deceleration settings are not used.
 */
class KRITATOOLS_EXPORT KisKalmanStrokePredictor : public KisStrokePredictor
{
public:
    explicit KisKalmanStrokePredictor(const KisStrokePredictionSettings &settings);
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
