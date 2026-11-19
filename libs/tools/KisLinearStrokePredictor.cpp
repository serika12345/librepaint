/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLinearStrokePredictor.h"

KisLinearStrokePredictor::KisLinearStrokePredictor(const KisStrokePredictionSettings &settings)
{
    KisStrokePredictionSettings uncapped = settings;
    uncapped.maxMovePx = 0.0;
    m_prediction.setSettings(uncapped);
}

void KisLinearStrokePredictor::reset()
{
    m_prediction.reset();
}

void KisLinearStrokePredictor::addSample(const KisStrokePredictionSample &sample)
{
    m_prediction.addSample(sample.documentPos, sample.timeMs, sample.pressure);
}

KisStrokePredictionResult KisLinearStrokePredictor::predict(qreal horizonMs) const
{
    KisStrokePredictionResult result = m_prediction.predict(horizonMs);
    result.source = KisStrokePredictionSource::Linear;
    return result;
}

KisStrokePredictionSource KisLinearStrokePredictor::source() const
{
    return KisStrokePredictionSource::Linear;
}
