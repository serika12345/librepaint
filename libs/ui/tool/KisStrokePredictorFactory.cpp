/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictorFactory.h"

#include <KisKalmanStrokePredictor.h>
#include <KisLinearStrokePredictor.h>

std::unique_ptr<KisStrokePredictor> KisStrokePredictorFactory::create(
    const KoPointerEvent &press,
    const KisStrokePredictionSettings &settings,
    const QString &predictorName)
{
    Q_UNUSED(press);

    if (predictorName == QStringLiteral("linear")) {
        return std::make_unique<KisLinearStrokePredictor>(settings);
    }
    if (predictorName == QStringLiteral("kalman-jerk")) {
        return std::make_unique<KisKalmanStrokePredictor>(settings, true);
    }
    return std::make_unique<KisKalmanStrokePredictor>(settings);
}
