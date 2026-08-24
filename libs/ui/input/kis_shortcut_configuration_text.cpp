/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_shortcut_configuration_text.h"

#include <QKeySequence>
#include <KLocalizedString>

#include <boost/preprocessor/repeat_from_to.hpp>

QString KisShortcutConfigurationText::inputText(const KisShortcutConfiguration &shortcut)
{
    switch (shortcut.type()) {
    case KisShortcutConfiguration::KeyCombinationType:
        return keysToText(shortcut.keys());
    case KisShortcutConfiguration::MouseButtonType:
        return buttonsInputToText(shortcut.keys(), shortcut.buttons());
    case KisShortcutConfiguration::MouseWheelType:
        return wheelInputToText(shortcut.keys(), shortcut.wheel());
    case KisShortcutConfiguration::GestureType:
    case KisShortcutConfiguration::MacOSGestureType:
        return gestureToText(shortcut.gesture());
    default:
        return QString();
    }
}

QString KisShortcutConfigurationText::buttonsToText(Qt::MouseButtons buttons)
{
    QString text;
    const QString separator = i18nc("Separator in the list of mouse buttons for shortcut", " + ");
    int buttonCount = 0;

    if (buttons & Qt::LeftButton) {
        text.append(i18nc("Left Mouse Button", "Left"));
        buttonCount++;
    }
    if (buttons & Qt::RightButton) {
        if (buttonCount++ > 0) {
            text.append(separator);
        }
        text.append(i18nc("Right Mouse Button", "Right"));
    }
    if (buttons & Qt::MiddleButton) {
        if (buttonCount++ > 0) {
            text.append(separator);
        }
        text.append(i18nc("Middle Mouse Button", "Middle"));
    }
    if (buttons & Qt::BackButton) {
        if (buttonCount++ > 0) {
            text.append(separator);
        }
        text.append(i18nc("Mouse Back Button", "Back"));
    }
    if (buttons & Qt::ForwardButton) {
        if (buttonCount++ > 0) {
            text.append(separator);
        }
        text.append(i18nc("Mouse Forward Button", "Forward"));
    }
    if (buttons & Qt::TaskButton) {
        if (buttonCount++ > 0) {
            text.append(separator);
        }
        text.append(i18nc("Mouse Task Button", "Task"));
    }

#define EXTRA_BUTTON(z, n, _) \
    if (buttons & Qt::ExtraButton##n) { \
        if (buttonCount++ > 0) { \
            text.append(separator); \
        } \
        text.append(i18nc("Mouse Button", "Mouse %1", n + 3)); \
    }
    BOOST_PP_REPEAT_FROM_TO(4, 25, EXTRA_BUTTON, _)
#undef EXTRA_BUTTON

    if (buttonCount == 0) {
        text.append(i18nc("No mouse buttons for shortcut", "None"));
    } else {
        text = i18ncp(
            "%1 = List of mouse buttons for shortcut. "
            "Plural form is chosen upon the number of buttons in that list.",
            "%1 Button", "%1 Buttons", text, buttonCount);
    }

    return text;
}

QString KisShortcutConfigurationText::keysToText(const QList<Qt::Key> &keys)
{
    QString output;

    for (Qt::Key key : keys) {
#if defined(Q_OS_MAC)
        output.append(QKeySequence(key).toString(QKeySequence::NativeText));
#else
        if (!output.isEmpty()) {
            output.append(i18nc("Separator in the list of keys for shortcut", " + "));
        }

        switch (key) {
        case Qt::Key_Control:
            output.append(i18nc("Ctrl key", "Ctrl"));
            break;
        case Qt::Key_Meta:
            output.append(i18nc("Meta key", "Meta"));
            break;
        case Qt::Key_Alt:
            output.append(i18nc("Alt key", "Alt"));
            break;
        case Qt::Key_Shift:
            output.append(i18nc("Shift key", "Shift"));
            break;
        default:
            output.append(QKeySequence(key).toString(QKeySequence::NativeText));
            break;
        }
#endif
    }

    if (output.isEmpty()) {
        output = i18nc("No keys for shortcut", "None");
    }
    return output;
}

QString KisShortcutConfigurationText::wheelToText(KisShortcutConfiguration::MouseWheelMovement wheel)
{
    switch (wheel) {
    case KisShortcutConfiguration::WheelUp:
        return i18n("Mouse Wheel Up");
    case KisShortcutConfiguration::WheelDown:
        return i18n("Mouse Wheel Down");
    case KisShortcutConfiguration::WheelLeft:
        return i18n("Mouse Wheel Left");
    case KisShortcutConfiguration::WheelRight:
        return i18n("Mouse Wheel Right");
    case KisShortcutConfiguration::WheelTrackpad:
        return i18n("Trackpad Pan");
    default:
        return i18nc("No mouse wheel buttons for shortcut", "None");
    }
}

QString KisShortcutConfigurationText::buttonsInputToText(const QList<Qt::Key> &keys,
                                                         Qt::MouseButtons buttons)
{
    const QString buttonsText = buttonsToText(buttons);
    return keys.isEmpty()
        ? buttonsText
        : i18nc("%1 = modifier keys in shortcut; %2 = mouse buttons in shortcut",
                "%1 + %2", keysToText(keys), buttonsText);
}

QString KisShortcutConfigurationText::wheelInputToText(
    const QList<Qt::Key> &keys,
    KisShortcutConfiguration::MouseWheelMovement wheel)
{
    const QString wheelText = wheelToText(wheel);
    return keys.isEmpty()
        ? wheelText
        : i18nc("%1 = modifier keys in shortcut; %2 = mouse wheel buttons in shortcut",
                "%1 + %2", keysToText(keys), wheelText);
}

QString KisShortcutConfigurationText::gestureToText(KisShortcutConfiguration::GestureAction action)
{
    switch (action) {
#ifdef Q_OS_MACOS
    case KisShortcutConfiguration::PinchGesture:
        return i18n("Pinch Gesture");
    case KisShortcutConfiguration::PanGesture:
        return i18n("Pan Gesture");
    case KisShortcutConfiguration::RotateGesture:
        return i18n("Rotate Gesture");
    case KisShortcutConfiguration::SmartZoomGesture:
        return i18n("Smart Zoom Gesture");
#else
    case KisShortcutConfiguration::OneFingerTap:
        return i18n("One Finger Tap");
    case KisShortcutConfiguration::TwoFingerTap:
        return i18n("Two Finger Tap");
    case KisShortcutConfiguration::ThreeFingerTap:
        return i18n("Three Finger Tap");
    case KisShortcutConfiguration::FourFingerTap:
        return i18n("Four Finger Tap");
    case KisShortcutConfiguration::FiveFingerTap:
        return i18n("Five Finger Tap");
    case KisShortcutConfiguration::OneFingerDrag:
        return i18n("One Finger Drag");
    case KisShortcutConfiguration::TwoFingerDrag:
        return i18n("Two Finger Drag");
    case KisShortcutConfiguration::ThreeFingerDrag:
        return i18n("Three Finger Drag");
    case KisShortcutConfiguration::FourFingerDrag:
        return i18n("Four Finger Drag");
    case KisShortcutConfiguration::FiveFingerDrag:
        return i18n("Five Finger Drag");
    case KisShortcutConfiguration::OneFingerHold:
        return i18n("One Finger Hold");
#endif
    default:
        return i18n("No Gesture");
    }
}
