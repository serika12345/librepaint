/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTOR_FACTORY_H
#define KIS_STROKE_PREDICTOR_FACTORY_H

#include <KisStrokePrediction.h>
#include <KisStrokePredictor.h>

#include <QString>
#include <QTransform>
#include <memory>

#include "kritaui_export.h"

class KoPointerEvent;

/**
 * Chooses the predictor of a stroke when the press arrives. The choice is
 * fixed for the stroke.
 *
 * On iOS an installed prediction supplier and a press of the Apple Pencil,
 * that is a tablet event, select the predictor that follows the prediction
 * points of the system. Every other press, and every press on the other
 * platforms, keeps the predictor that predictorName names: "kalman" selects
 * the Kalman predictor, every other value the linear one.
 */
class KRITAUI_EXPORT KisStrokePredictorFactory
{
public:
    /// predictorName is the hidden setting: "kalman" selects the Kalman
    /// predictor, every other value the linear one. widgetToDocument is the
    /// linear part of the canvas widget to document transform.
    static std::unique_ptr<KisStrokePredictor> create(const KoPointerEvent &press,
                                                      const KisStrokePredictionSettings &settings,
                                                      const QString &predictorName,
                                                      const QTransform &widgetToDocument);
};

#endif // KIS_STROKE_PREDICTOR_FACTORY_H
