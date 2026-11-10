/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_CANVAS_FRAME_UPDATE_GATE_H
#define KIS_CANVAS_FRAME_UPDATE_GATE_H

/**
 * Keeps canvas updates to at most one paint in flight and at most one dispatch
 * per display period. It is used when Qt does not provide the frame-rate
 * compression patch (the official Qt builds).
 *
 * The caller dispatches immediately when requestUpdate() returns true. A
 * request that arrives while a paint is in flight is kept pending; the caller
 * wakes the pending update from QWindow::requestUpdate() and calls
 * takePendingUpdateForDispatch() when that event (or its fallback timer)
 * arrives. frameSwapped() records that the previous paint finished.
 */
class KisCanvasFrameUpdateGate
{
public:
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /**
     * Returns true when the caller must dispatch the accumulated region now.
     * Otherwise the request is pending.
     */
    bool requestUpdate();

    /**
     * Records that the previous paint finished composition. The pending
     * update is dispatched by the next requestUpdate event, not here, so the
     * display period stays the pacing source.
     */
    void frameSwapped();

    /**
     * Returns true when the pending region must be dispatched now. The caller
     * calls this after the display-synchronized update event or its fallback
     * timer. It refuses while the previous paint is still in flight.
     */
    bool takePendingUpdateForDispatch();

    bool isInFlight() const;
    bool hasPendingUpdate() const;

    void reset();

private:
    bool m_enabled = false;
    bool m_inFlight = false;
    bool m_pending = false;
};

#endif // KIS_CANVAS_FRAME_UPDATE_GATE_H
