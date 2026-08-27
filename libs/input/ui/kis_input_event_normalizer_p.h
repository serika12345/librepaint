/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_INPUT_EVENT_NORMALIZER_P_H
#define KIS_INPUT_EVENT_NORMALIZER_P_H

#include <QEvent>
#include <QMouseEvent>
#include <QTabletEvent>

#include <KisInputEventSuppressor.h>

namespace KisInputManagerDetail
{
inline KisInputEventSuppressor::Button normalizedButton(Qt::MouseButton button)
{
    if (button == Qt::LeftButton) {
        return KisInputEventSuppressor::Button::Left;
    }
    if (button == Qt::NoButton) {
        return KisInputEventSuppressor::Button::None;
    }
    return KisInputEventSuppressor::Button::Other;
}

inline KisInputEventSuppressor::Event normalizedSuppressionEvent(QEvent *event)
{
    using EventType = KisInputEventSuppressor::EventType;

    KisInputEventSuppressor::Event result;
    switch (event->type()) {
    case QEvent::MouseMove:
        result.type = EventType::MouseMove;
        break;
    case QEvent::MouseButtonPress:
        result.type = EventType::MousePress;
        break;
    case QEvent::MouseButtonRelease:
        result.type = EventType::MouseRelease;
        break;
    case QEvent::MouseButtonDblClick:
        result.type = EventType::MouseDoubleClick;
        break;
    case QEvent::TabletPress:
        result.type = EventType::TabletPress;
        break;
    case QEvent::TabletRelease:
        result.type = EventType::TabletRelease;
        break;
    case QEvent::TouchBegin:
        result.type = EventType::TouchBegin;
        break;
    default:
        return result;
    }

    if (result.type == EventType::MouseMove ||
        result.type == EventType::MousePress ||
        result.type == EventType::MouseRelease ||
        result.type == EventType::MouseDoubleClick) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        result.button = normalizedButton(mouseEvent->button());
        result.synthesized = mouseEvent->source() != Qt::MouseEventNotSynthesized;
    } else if (result.type == EventType::TabletPress ||
               result.type == EventType::TabletRelease) {
        result.button = normalizedButton(static_cast<QTabletEvent *>(event)->button());
    }

    return result;
}
} // namespace KisInputManagerDetail

#endif
