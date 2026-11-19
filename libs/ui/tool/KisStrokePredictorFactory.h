/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTOR_FACTORY_H
#define KIS_STROKE_PREDICTOR_FACTORY_H

#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>

#include <QString>
#include <memory>

#include "kritaui_export.h"

class KoPointerEvent;

/**
 * Chooses the predictor of a stroke when the press arrives. The choice is
 * fixed for the stroke.
 */
class KRITAUI_EXPORT KisStrokePredictorFactory
{
public:
    /// The press decides the platform predictor in later work. predictorName
    /// is the hidden setting: "kalman" selects the Kalman predictor, every
    /// other value the linear one.
    static std::unique_ptr<KisStrokePredictor> create(const KoPointerEvent &press,
                                                      const KisStrokePredictionSettings &settings,
                                                      const QString &predictorName = QStringLiteral("linear"));
};

#endif // KIS_STROKE_PREDICTOR_FACTORY_H
