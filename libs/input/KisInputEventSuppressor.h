/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_INPUT_EVENT_SUPPRESSOR_H
#define KIS_INPUT_EVENT_SUPPRESSOR_H

#include "kritainput_export.h"

/**
 * Owns the input-policy state used to suppress mouse events synthesized from
 * tablet or touch input. Callers provide normalized event values and retain
 * ownership of the platform events, diagnostics, and event-filter connection.
 *
 * The constructor fixes platform and secondary-button behavior for the
 * suppressor lifetime. filter() consumes the one-shot delayed-press state and
 * otherwise returns a deterministic reason without external side effects.
 */
class KRITAINPUT_EXPORT KisInputEventSuppressor
{
public:
    enum class EventType {
        Other,
        MouseMove,
        MousePress,
        MouseRelease,
        MouseDoubleClick,
        TabletPress,
        TabletRelease,
        TouchBegin
    };

    enum class Button {
        None,
        Left,
        Other
    };

    enum class SuppressionReason {
        None,
        DelayedMousePress,
        MouseEvent,
        SecondaryTabletButton,
        TouchBegin
    };

    struct Event {
        EventType type {EventType::Other};
        Button button {Button::None};
        bool synthesized {false};
    };

    KisInputEventSuppressor(bool useSecondaryButtonsWorkaround,
                            bool supportsSyntheticMouseSuppression);

    SuppressionReason filter(const Event &event);

    void startBlockingMouseEvents();
    void stopBlockingMouseEvents();
    bool isBlockingMouseEvents() const;

    void suppressNextLeftMousePress();
    void setSuppressSyntheticMouseEvents(bool value);

    void startBlockingTouchEvents();
    void stopBlockingTouchEvents();

private:
    bool m_blockMouseEvents {false};
    bool m_suppressNextLeftMousePress {false};
    bool m_suppressSyntheticMouseEvents {false};
    bool m_blockTouchEvents {false};
    const bool m_useSecondaryButtonsWorkaround;
    const bool m_supportsSyntheticMouseSuppression;
};

#endif
