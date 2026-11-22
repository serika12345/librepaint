/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_CAP_H
#define KIS_STROKE_PREDICTION_CAP_H

#include <KisStrokePrediction.h>

#include <QTransform>

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

    /**
     * Map every point of @p result with @p documentToPixel, and update
     * moveLength, velocity and speed to the mapped coordinates. The
     * predictors work in document coordinates; the stroke paints in image
     * pixels, so the prediction is mapped before the cap and the stroke use it.
     * A result that is not valid is left unchanged.
     */
    static void mapToPixels(KisStrokePredictionResult &result, const QTransform &documentToPixel);
};

#endif // KIS_STROKE_PREDICTION_CAP_H
