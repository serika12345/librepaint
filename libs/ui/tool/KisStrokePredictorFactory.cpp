/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictorFactory.h"

#include <KisLinearStrokePredictor.h>

std::unique_ptr<KisStrokePredictor> KisStrokePredictorFactory::create(
    const KoPointerEvent &press,
    const KisStrokePredictionSettings &settings)
{
    Q_UNUSED(press);
    return std::make_unique<KisLinearStrokePredictor>(settings);
}
