/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_WINTAB_TRUST_H
#define KIS_WINTAB_TRUST_H

#include "kritainputui_export.h"

#include <QPointF>
#include <QtGlobal>

/**
 * Decides whether the Wintab packet position may replace the position of the
 * QTabletEvent during one proximity.
 *
 * Q4 stage 4: the driver's tablet range (lcSysOrg/lcSysExt) is not always
 * correct. Huion, for example, reports a wrong system rectangle when the
 * screens have different scale factors. Blender checks the first press of a
 * proximity against the cursor and keeps the Qt position when the two
 * disagree. This class holds that decision.
 *
 * The state is Undecided before the first press, so the hover that precedes a
 * press is corrected. The first press compares the mapped position with the
 * cursor; a difference of 2.0 px or less on both axes trusts the driver's
 * range for the rest of the proximity, a larger difference distrusts it and
 * leaves the Qt position in place until the next proximity.
 *
 * This class is pure computation and builds on every platform. The caller
 * passes the cursor position, so the contract test needs no real cursor.
 */
class KRITAINPUTUI_EXPORT KisWintabTrust
{
public:
    enum class State {
        Undecided,
        Trusted,
        Distrusted
    };

    /**
     * The largest difference on one axis that still trusts the driver's
     * range. Blender uses 1 px; Krita keeps the driver's range without a
     * check. The user chose 2 px.
     */
    static constexpr qreal MaxDifferencePixels = 2.0;

    /**
     * Starts a proximity from QEvent::TabletEnterProximity and returns to
     * Undecided.
     */
    void beginProximity();

    /**
     * Ends a proximity from QEvent::TabletLeaveProximity.
     */
    void endProximity();

    /**
     * Starts a proximity when Qt did not send the enter event for the first
     * tablet event of a proximity.
     */
    void noteTabletEvent();

    State state() const;

    /**
     * True while the position may be replaced: Undecided and Trusted.
     */
    bool correctionAllowed() const;

    /**
     * Compares the mapped position with the cursor on the first press of a
     * proximity and returns the state. Later calls return the decision of the
     * proximity without changing it.
     */
    State decide(const QPointF &mappedPosition, const QPointF &cursorPosition);

    /**
     * The largest axis difference of the last decision, in logical pixels.
     */
    qreal differencePixels() const;

private:
    State m_state = State::Undecided;
    qreal m_differencePixels = 0.0;
    bool m_inProximity = false;
};

#endif // KIS_WINTAB_TRUST_H
