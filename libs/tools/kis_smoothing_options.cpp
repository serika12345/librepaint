/*
 *  SPDX-FileCopyrightText: 2012 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_smoothing_options.h"

#include "kis_assert.h"
#include "kis_signal_compressor.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace {

KConfigGroup toolSettings()
{
    return KSharedConfig::openConfig()->group(QString());
}

}

struct KisSmoothingOptions::Private {
    Private(bool useSavedSmoothing)
        : writeCompressor(500, KisSignalCompressor::FIRST_ACTIVE)
    {
        const KConfigGroup cfg = toolSettings();
        smoothingType = static_cast<SmoothingType>(useSavedSmoothing ? cfg.readEntry("LineSmoothingType", 1) : 1);
        smoothnessDistanceMin = useSavedSmoothing ? cfg.readEntry("LineSmoothingDistanceMin", 50.0) : 50.0;
        smoothnessDistanceMax = useSavedSmoothing ? cfg.readEntry("LineSmoothingDistanceMax", 50.0) : 50.0;
        smoothnessDistanceKeepAspectRatio = useSavedSmoothing ? cfg.readEntry("LineSmoothingDistanceKeepAspectRatio", true) : true;
        tailAggressiveness = useSavedSmoothing ? cfg.readEntry("LineSmoothingTailAggressiveness", 0.15) : 0.15;
        smoothPressure = useSavedSmoothing ? cfg.readEntry("LineSmoothingSmoothPressure", false) : false;
        useScalableDistance = useSavedSmoothing ? cfg.readEntry("LineSmoothingScalableDistance", true) : true;
        delayDistance = useSavedSmoothing ? cfg.readEntry("LineSmoothingDelayDistance", 50.0) : 50.0;
        useDelayDistance = useSavedSmoothing ? cfg.readEntry("LineSmoothingUseDelayDistance", true) : true;
        finishStabilizedCurve = useSavedSmoothing ? cfg.readEntry("LineSmoothingFinishStabilizedCurve", true) : true;
        stabilizeSensors = useSavedSmoothing ? cfg.readEntry("LineSmoothingStabilizeSensors", true) : true;
    }

    KisSignalCompressor writeCompressor;

    SmoothingType smoothingType;
    qreal smoothnessDistanceMin;
    qreal smoothnessDistanceMax;
    bool smoothnessDistanceKeepAspectRatio;
    qreal tailAggressiveness;
    bool smoothPressure;
    bool useScalableDistance;
    qreal delayDistance;
    bool useDelayDistance;
    bool finishStabilizedCurve;
    bool stabilizeSensors;
};

KisSmoothingOptions::KisSmoothingOptions(bool useSavedSmoothing)
    : m_d(new Private(useSavedSmoothing))
{

    connect(&m_d->writeCompressor, SIGNAL(timeout()), this, SLOT(slotWriteConfig()));
}

KisSmoothingOptions::~KisSmoothingOptions()
{
}

KisSmoothingOptions::SmoothingType KisSmoothingOptions::smoothingType() const
{
    return m_d->smoothingType;
}

void KisSmoothingOptions::setSmoothingType(KisSmoothingOptions::SmoothingType value)
{
    m_d->smoothingType = value;
    Q_EMIT sigSmoothingTypeChanged();
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::smoothnessDistanceMin() const
{
    return m_d->smoothnessDistanceMin;
}


void KisSmoothingOptions::setSmoothnessDistanceMin(qreal value)
{
    m_d->smoothnessDistanceMin = value;
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::smoothnessDistanceMax() const
{
    return m_d->smoothnessDistanceMax;
}

void KisSmoothingOptions::setSmoothnessDistanceMax(qreal value)
{
    m_d->smoothnessDistanceMax = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::smoothnessDistanceKeepAspectRatio() const
{
    return m_d->smoothnessDistanceKeepAspectRatio;
}

void KisSmoothingOptions::setSmoothnessDistanceKeepAspectRatio(bool value)
{
    m_d->smoothnessDistanceKeepAspectRatio = value;
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::tailAggressiveness() const
{
    return m_d->tailAggressiveness;
}

void KisSmoothingOptions::setTailAggressiveness(qreal value)
{
    m_d->tailAggressiveness = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::smoothPressure() const
{
    return m_d->smoothPressure;
}

void KisSmoothingOptions::setSmoothPressure(bool value)
{
    m_d->smoothPressure = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::useScalableDistance() const
{
    return m_d->smoothingType != STABILIZER ? m_d->useScalableDistance : true;
}

void KisSmoothingOptions::setUseScalableDistance(bool value)
{
    // stabilizer mush always have scalable distance on
    // see bug 421314
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->smoothingType != STABILIZER);

    m_d->useScalableDistance = value;
    m_d->writeCompressor.start();
}

qreal KisSmoothingOptions::delayDistance() const
{
    return m_d->delayDistance;
}

void KisSmoothingOptions::setDelayDistance(qreal value)
{
    m_d->delayDistance = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::useDelayDistance() const
{
    return m_d->useDelayDistance;
}

void KisSmoothingOptions::setUseDelayDistance(bool value)
{
    m_d->useDelayDistance = value;
    m_d->writeCompressor.start();
}

void KisSmoothingOptions::setFinishStabilizedCurve(bool value)
{
    m_d->finishStabilizedCurve = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::finishStabilizedCurve() const
{
    return m_d->finishStabilizedCurve;
}

void KisSmoothingOptions::setStabilizeSensors(bool value)
{
    m_d->stabilizeSensors = value;
    m_d->writeCompressor.start();
}

bool KisSmoothingOptions::stabilizeSensors() const
{
    return m_d->stabilizeSensors;
}

void KisSmoothingOptions::slotWriteConfig()
{
    KConfigGroup cfg = toolSettings();
    cfg.writeEntry("LineSmoothingType", static_cast<int>(m_d->smoothingType));
    cfg.writeEntry("LineSmoothingDistanceMin", m_d->smoothnessDistanceMin);
    cfg.writeEntry("LineSmoothingDistanceMax", m_d->smoothnessDistanceMax);
    cfg.writeEntry("LineSmoothingDistanceKeepAspectRatio", m_d->smoothnessDistanceKeepAspectRatio);
    cfg.writeEntry("LineSmoothingTailAggressiveness", m_d->tailAggressiveness);
    cfg.writeEntry("LineSmoothingSmoothPressure", m_d->smoothPressure);
    cfg.writeEntry("LineSmoothingScalableDistance", m_d->useScalableDistance);
    cfg.writeEntry("LineSmoothingDelayDistance", m_d->delayDistance);
    cfg.writeEntry("LineSmoothingUseDelayDistance", m_d->useDelayDistance);
    cfg.writeEntry("LineSmoothingFinishStabilizedCurve", m_d->finishStabilizedCurve);
    cfg.writeEntry("LineSmoothingStabilizeSensors", m_d->stabilizeSensors);
    cfg.sync();
}
