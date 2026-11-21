/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictorFactory.h"

#include <KisKalmanStrokePredictor.h>
#include <KisLinearStrokePredictor.h>
#include <KisPlatformPredictionBuffer.h>
#include <KisPlatformStrokePredictor.h>

#include <KoPointerEvent.h>

#include <QtGlobal>

std::unique_ptr<KisStrokePredictor> KisStrokePredictorFactory::create(
    const KoPointerEvent &press,
    const KisStrokePredictionSettings &settings,
    const QString &predictorName,
    const QTransform &widgetToDocument)
{
    Q_UNUSED(press);
    Q_UNUSED(widgetToDocument);

#ifdef Q_OS_IOS
    if (KisPlatformPredictionBuffer::instance().supplierInstalled() && press.isTabletEvent()) {
        return std::make_unique<KisPlatformStrokePredictor>(
            settings,
            KisStrokePredictionSource::IosSystem,
            KisPlatformPredictionBuffer::instance(),
            widgetToDocument);
    }
#endif

    if (predictorName == QStringLiteral("linear")) {
        return std::make_unique<KisLinearStrokePredictor>(settings);
    }
    if (predictorName == QStringLiteral("kalman-jerk")) {
        return std::make_unique<KisKalmanStrokePredictor>(settings, true);
    }
    return std::make_unique<KisKalmanStrokePredictor>(settings);
}
