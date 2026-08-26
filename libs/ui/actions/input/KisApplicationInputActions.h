/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_APPLICATION_INPUT_ACTIONS_H
#define KIS_APPLICATION_INPUT_ACTIONS_H

#include <QList>
#include <kritaui_export.h>

class KisAbstractInputAction;
class KisCanvas2;
class KisInputManager;

KRITAUI_EXPORT QList<KisAbstractInputAction *> createApplicationInputActions();
KisCanvas2 *applicationInputCanvas(const KisInputManager *inputManager);

#endif
