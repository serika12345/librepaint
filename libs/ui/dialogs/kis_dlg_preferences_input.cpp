/*
 * SPDX-FileCopyrightText: 1999 Michael Koch <koch@kde.org>
 * SPDX-FileCopyrightText: 2003-2011 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dlg_preferences.h"

#include <klocalizedstring.h>

#include <input/ui/config/kis_input_configuration_page.h>
#include <input/ui/wintab/drawpile_tablettester/tablettester.h>

#include "kis_icon_utils.h"

void TabletSettingsTab::slotTabletTest()
{
    TabletTestDialog tabletTestDialog(this);
    tabletTestDialog.exec();
}

void KisDlgPreferences::addInputConfigurationPage()
{
    m_inputConfiguration = new KisInputConfigurationPage();
    KPageWidgetItem *page = addPage(m_inputConfiguration, i18n("Canvas Input Settings"));
    page->setHeader(i18n("Canvas Input"));
    page->setObjectName("canvasinput");
    page->setIcon(KisIconUtils::loadIcon("config-canvas-input"));
    m_pages << page;

    connect(this, SIGNAL(accepted()), m_inputConfiguration, SLOT(saveChanges()));
    connect(this, SIGNAL(rejected()), m_inputConfiguration, SLOT(revertChanges()));
}

void KisDlgPreferences::setInputConfigurationDefaults()
{
    m_inputConfiguration->setDefaults();
}
