/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_canvas_frame_update_gate.h"

void KisCanvasFrameUpdateGate::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_enabled) {
        reset();
    }
}

bool KisCanvasFrameUpdateGate::isEnabled() const
{
    return m_enabled;
}

bool KisCanvasFrameUpdateGate::requestUpdate()
{
    if (!m_enabled) {
        return true;
    }

    if (m_inFlight) {
        m_pending = true;
        return false;
    }

    m_pending = false;
    m_inFlight = true;
    return true;
}

void KisCanvasFrameUpdateGate::frameSwapped()
{
    m_inFlight = false;
}

bool KisCanvasFrameUpdateGate::takePendingUpdateForDispatch()
{
    if (!m_enabled || m_inFlight || !m_pending) {
        return false;
    }

    m_pending = false;
    m_inFlight = true;
    return true;
}

bool KisCanvasFrameUpdateGate::isInFlight() const
{
    return m_inFlight;
}

bool KisCanvasFrameUpdateGate::hasPendingUpdate() const
{
    return m_pending;
}

void KisCanvasFrameUpdateGate::reset()
{
    m_inFlight = false;
    m_pending = false;
}
