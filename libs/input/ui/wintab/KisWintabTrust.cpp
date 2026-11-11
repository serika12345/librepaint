/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWintabTrust.h"

#include <QtGlobal>

void KisWintabTrust::beginProximity()
{
    m_state = State::Undecided;
    m_differencePixels = 0.0;
    m_inProximity = true;
}

void KisWintabTrust::endProximity()
{
    m_state = State::Undecided;
    m_differencePixels = 0.0;
    m_inProximity = false;
}

void KisWintabTrust::noteTabletEvent()
{
    if (!m_inProximity) {
        beginProximity();
    }
}

KisWintabTrust::State KisWintabTrust::state() const
{
    return m_state;
}

bool KisWintabTrust::correctionAllowed() const
{
    return m_state != State::Distrusted;
}

KisWintabTrust::State KisWintabTrust::decide(const QPointF &mappedPosition,
                                             const QPointF &cursorPosition)
{
    if (m_state != State::Undecided) {
        return m_state;
    }

    const QPointF delta = mappedPosition - cursorPosition;
    m_differencePixels = qMax(qAbs(delta.x()), qAbs(delta.y()));

    m_state = m_differencePixels <= MaxDifferencePixels
        ? State::Trusted : State::Distrusted;

    return m_state;
}

qreal KisWintabTrust::differencePixels() const
{
    return m_differencePixels;
}
