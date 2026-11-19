/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_LINEAR_STROKE_PREDICTOR_H
#define KIS_LINEAR_STROKE_PREDICTOR_H

#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>

#include <kritatools_export.h>

/**
 * The first-order linear prediction of KisStrokePrediction behind the common
 * predictor interface. It stays until the comparison with the Kalman
 * predictor in the replay and the measurements is done.
 *
 * The maximum move is left to KisStrokePredictionCap, so the settings given
 * to the constructor lose their maxMovePx.
 */
class KRITATOOLS_EXPORT KisLinearStrokePredictor : public KisStrokePredictor
{
public:
    explicit KisLinearStrokePredictor(const KisStrokePredictionSettings &settings);

    void reset() override;
    void addSample(const KisStrokePredictionSample &sample) override;
    KisStrokePredictionResult predict(qreal horizonMs) const override;
    KisStrokePredictionSource source() const override;

private:
    KisStrokePrediction m_prediction;
};

#endif // KIS_LINEAR_STROKE_PREDICTOR_H
