/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_CAP_H
#define KIS_STROKE_PREDICTION_CAP_H

#include <KisStrokePrediction.h>

#include <kritatools_export.h>

/**
 * Common post-processing of a prediction: the move from the latest real point
 * to the last predicted point is shortened to a maximum length. Every
 * predictor gets the same rule, so the OS-provided predictions obey the same
 * limit against the overshoot after a stop as the predictions of the app.
 */
class KRITATOOLS_EXPORT KisStrokePredictionCap
{
public:
    /**
     * Cut the predicted part of @p result where the polyline from its
     * latestIndex reaches @p maxMovePx, and drop the points after the cut.
     * A result that is not valid and a @p maxMovePx of 0 or below are left
     * unchanged.
     */
    static void apply(KisStrokePredictionResult &result, qreal maxMovePx);
};

#endif // KIS_STROKE_PREDICTION_CAP_H
