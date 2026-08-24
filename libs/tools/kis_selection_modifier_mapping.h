/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2016 Michael Abrahams <miabraha@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef KIS_SELECTION_MODIFIER_MAPPING_H
#define KIS_SELECTION_MODIFIER_MAPPING_H

#include <Qt>

#include <kis_selection.h>
#include <kritatools_export.h>

class QKeyEvent;

KRITATOOLS_EXPORT SelectionAction mapSelectionToolModifiers(
    Qt::KeyboardModifiers modifiers,
    bool swapControlAndAlt);

KRITATOOLS_EXPORT Qt::Key normalizedSelectionModifierKey(const QKeyEvent *event);

#endif
