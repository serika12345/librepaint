/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_CANVAS_ENTER_LEAVE_GUARD_H
#define KIS_CANVAS_ENTER_LEAVE_GUARD_H

namespace KisInputManagerDetail
{

/**
 * State that decides whether a QEvent::Leave delivered to the canvas is real.
 *
 * The Qt fork patches df3a678309 (a spurious WM_MOUSELEAVE when the stylus
 * enters the window), c132390390 (an unbalanced Leave after the last finger
 * leaves the touch screen) and 41adadd8db (accepted tablet events do not
 * synthesize mouse events) are reimplemented here with public Qt events.
 */
struct CanvasPointerState {
    bool tabletInProximity {false};
    bool tabletPressed {false};
    bool touchActive {false};
};

enum class CanvasLeaveAction {
    Ignore,
    Confirm
};

/**
 * A pressed stylus or a finger on the screen owns the pointer, so a Leave in
 * the middle of the stroke or the touch sequence is ignored. Otherwise the
 * cursor position is the truth: a Leave that arrives while the cursor is still
 * over the canvas is the unbalanced event that Qt sends around tablet and
 * touch input. L1 experiment for Q4 (official Qt). L2 removes it if the
 * official Qt starts to send balanced Enter/Leave events.
 */
inline CanvasLeaveAction canvasLeaveAction(const CanvasPointerState &state,
                                           bool pointerInsideCanvas)
{
    if (state.tabletPressed || state.touchActive) {
        return CanvasLeaveAction::Ignore;
    }
    if (pointerInsideCanvas) {
        return CanvasLeaveAction::Ignore;
    }
    return CanvasLeaveAction::Confirm;
}

/**
 * After the stylus leaves the tablet's proximity, confirm the Leave only when
 * the cursor is outside the canvas. Otherwise the mouse events that follow
 * the proximity carry the real Enter/Leave pair.
 */
inline bool confirmLeaveAfterProximity(bool pointerInsideCanvas)
{
    return !pointerInsideCanvas;
}

} // namespace KisInputManagerDetail

#endif // KIS_CANVAS_ENTER_LEAVE_GUARD_H
