/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/kis_dlg_preferences.h"

#include <KisImageConfigNotifier.h>

void KisDlgPreferences::notifyImageSettingsChanged()
{
    KisImageConfigNotifier::instance()->notifyConfigChanged();
}
