/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisInputEventSuppressor.h"

namespace {

bool isMouseEvent(KisInputEventSuppressor::EventType type)
{
    return type == KisInputEventSuppressor::EventType::MouseMove ||
           type == KisInputEventSuppressor::EventType::MousePress ||
           type == KisInputEventSuppressor::EventType::MouseRelease ||
           type == KisInputEventSuppressor::EventType::MouseDoubleClick;
}

}

KisInputEventSuppressor::KisInputEventSuppressor(
    bool useSecondaryButtonsWorkaround,
    bool supportsSyntheticMouseSuppression)
    : m_useSecondaryButtonsWorkaround(useSecondaryButtonsWorkaround)
    , m_supportsSyntheticMouseSuppression(supportsSyntheticMouseSuppression)
{
}

KisInputEventSuppressor::SuppressionReason
KisInputEventSuppressor::filter(const Event &event)
{
    if (m_suppressNextLeftMousePress &&
        event.type == EventType::MousePress &&
        event.button == Button::Left) {
        m_suppressNextLeftMousePress = false;
        return SuppressionReason::DelayedMousePress;
    }

    if (m_useSecondaryButtonsWorkaround) {
        if ((event.type == EventType::TabletPress ||
             event.type == EventType::TabletRelease) &&
            event.button != Button::Left) {
            return SuppressionReason::SecondaryTabletButton;
        }

        if ((event.type == EventType::MousePress ||
             event.type == EventType::MouseRelease ||
             event.type == EventType::MouseDoubleClick) &&
            event.button != Button::Left) {
            return SuppressionReason::None;
        }
    }

    if (isMouseEvent(event.type) &&
        (m_blockMouseEvents ||
         (m_supportsSyntheticMouseSuppression &&
          m_suppressSyntheticMouseEvents &&
          event.synthesized))) {
        return SuppressionReason::MouseEvent;
    }

    if (m_blockTouchEvents && event.type == EventType::TouchBegin) {
        return SuppressionReason::TouchBegin;
    }

    return SuppressionReason::None;
}

void KisInputEventSuppressor::startBlockingMouseEvents()
{
    m_blockMouseEvents = true;
}

void KisInputEventSuppressor::stopBlockingMouseEvents()
{
    m_blockMouseEvents = false;
}

bool KisInputEventSuppressor::isBlockingMouseEvents() const
{
    return m_blockMouseEvents;
}

void KisInputEventSuppressor::suppressNextLeftMousePress()
{
    m_suppressNextLeftMousePress = true;
}

void KisInputEventSuppressor::setSuppressSyntheticMouseEvents(bool value)
{
    m_suppressSyntheticMouseEvents = value;
}

void KisInputEventSuppressor::startBlockingTouchEvents()
{
    m_blockTouchEvents = true;
}

void KisInputEventSuppressor::stopBlockingTouchEvents()
{
    m_blockTouchEvents = false;
}
