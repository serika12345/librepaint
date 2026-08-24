/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISSHORTCUTCONFIGURATIONTEXT_H
#define KISSHORTCUTCONFIGURATIONTEXT_H

#include <kis_shortcut_configuration.h>

class KisShortcutConfigurationText
{
public:
    static QString inputText(const KisShortcutConfiguration &shortcut);
    static QString buttonsToText(Qt::MouseButtons buttons);
    static QString keysToText(const QList<Qt::Key> &keys);
    static QString wheelToText(KisShortcutConfiguration::MouseWheelMovement wheel);
    static QString buttonsInputToText(const QList<Qt::Key> &keys, Qt::MouseButtons buttons);
    static QString wheelInputToText(const QList<Qt::Key> &keys,
                                    KisShortcutConfiguration::MouseWheelMovement wheel);
    static QString gestureToText(KisShortcutConfiguration::GestureAction action);
};

#endif // KISSHORTCUTCONFIGURATIONTEXT_H
