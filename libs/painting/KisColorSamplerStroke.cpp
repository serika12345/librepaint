/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSamplerStroke.h"

#include <KoColor.h>

#include <kis_assert.h>
#include <kis_image_interfaces.h>
#include <kis_paint_device.h>
#include <strokes/kis_color_sampler_stroke_strategy.h>

struct KisColorSamplerStroke::Private
{
    KisStrokesFacade *strokesFacade {nullptr};
    KisStrokeId strokeId;
};

KisColorSamplerStroke::KisColorSamplerStroke(QObject *parent)
    : QObject(parent)
    , m_d(new Private)
{
}

KisColorSamplerStroke::~KisColorSamplerStroke()
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(!m_d->strokeId);
}

bool KisColorSamplerStroke::isActive() const
{
    return bool(m_d->strokeId);
}

void KisColorSamplerStroke::start(KisStrokesFacade *strokesFacade, int radius, int blend)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!m_d->strokeId);
    KIS_SAFE_ASSERT_RECOVER_RETURN(strokesFacade);

    KisColorSamplerStrokeStrategy *strategy = new KisColorSamplerStrokeStrategy(radius, blend);
    connect(strategy, &KisColorSamplerStrokeStrategy::sigColorUpdated,
            this, &KisColorSamplerStroke::sigColorUpdated);
    connect(strategy, &KisColorSamplerStrokeStrategy::sigFinalColorSelected,
            this, &KisColorSamplerStroke::sigFinalColorSelected);

    m_d->strokesFacade = strokesFacade;
    m_d->strokeId = strokesFacade->startStroke(strategy);
}

void KisColorSamplerStroke::addSample(const KisPaintDeviceSP &device,
                                      const QPoint &imagePoint,
                                      const KoColor &currentColor)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->strokeId);
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->strokesFacade);
    KIS_SAFE_ASSERT_RECOVER_RETURN(device);

    m_d->strokesFacade->addJob(
        m_d->strokeId,
        new KisColorSamplerStrokeStrategy::Data(device, imagePoint, currentColor));
}

void KisColorSamplerStroke::finish()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->strokeId);
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->strokesFacade);

    m_d->strokesFacade->addJob(
        m_d->strokeId,
        new KisColorSamplerStrokeStrategy::FinalizeData());
    m_d->strokesFacade->endStroke(m_d->strokeId);
    m_d->strokeId.clear();
    m_d->strokesFacade = nullptr;
}
