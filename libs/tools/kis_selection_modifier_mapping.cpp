/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2016 Michael Abrahams <miabraha@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kis_selection_modifier_mapping.h>

#include <QKeyEvent>

SelectionAction mapSelectionToolModifiers(Qt::KeyboardModifiers modifiers,
                                          bool swapControlAndAlt)
{
    const Qt::KeyboardModifier replaceModifier =
        swapControlAndAlt ? Qt::AltModifier : Qt::ControlModifier;
    const Qt::KeyboardModifier subtractModifier =
        swapControlAndAlt ? Qt::ControlModifier : Qt::AltModifier;
    const Qt::KeyboardModifiers intersectModifiers =
        (swapControlAndAlt ? Qt::ControlModifier : Qt::AltModifier) |
        Qt::ShiftModifier;

    if (modifiers == replaceModifier) {
        return SELECTION_REPLACE;
    }
    if (modifiers == intersectModifiers) {
        return SELECTION_INTERSECT;
    }
    if (modifiers == Qt::ShiftModifier) {
        return SELECTION_ADD;
    }
    if (modifiers == subtractModifier) {
        return SELECTION_SUBTRACT;
    }
    if (modifiers == (Qt::ControlModifier | Qt::AltModifier)) {
        return SELECTION_SYMMETRICDIFFERENCE;
    }
    return SELECTION_DEFAULT;
}

Qt::Key normalizedSelectionModifierKey(const QKeyEvent *event)
{
    Qt::Key key = static_cast<Qt::Key>(event->key());
    if (key == Qt::Key_Meta && event->modifiers().testFlag(Qt::ShiftModifier)) {
        key = Qt::Key_Alt;
    }
    return key;
}
