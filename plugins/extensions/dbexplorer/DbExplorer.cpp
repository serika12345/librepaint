/*
 *  SPDX-FileCopyrightText: 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DbExplorer.h"


#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/ui/orchestration/kis_action.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include "DlgDbExplorer.h"
#include "ui/orchestration/KisActionPlugin.h"

K_PLUGIN_FACTORY_WITH_JSON(DbExplorerFactory, "kritadbexplorer.json", registerPlugin<DbExplorer>();)

DbExplorer::DbExplorer(QObject *parent, const QVariantList &)
    : KisActionPlugin(parent)
{
    KisAction *action  = createAction("dbexplorer");
    connect(action, SIGNAL(triggered()), this, SLOT(slotDbExplorer()));
}


DbExplorer::~DbExplorer()
{
}

void DbExplorer::slotDbExplorer()
{
    DlgDbExplorer dlgDbExplorer(viewManager()->mainWindowAsQWidget());
    dlgDbExplorer.exec();
}

#include "DbExplorer.moc"
