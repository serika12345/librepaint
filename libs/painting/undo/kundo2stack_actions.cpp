/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2014 Mohit Goyal <mohit.bits2011@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "kundo2stack.h"

#include <KoIcon.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kstandardaction.h>

QAction *KUndo2Stack::createRedoAction(KisKActionCollection *actionCollection, const QString &actionName)
{
    QAction *action = KUndo2QStack::createRedoAction(actionCollection);

    if (actionName.isEmpty()) {
        action->setObjectName(KStandardAction::name(KStandardAction::Redo));
    } else {
        action->setObjectName(actionName);
    }

    action->setIcon(koIcon("edit-redo"));
    action->setIconText(i18n("Redo"));
    action->setShortcuts(KStandardShortcut::redo());

    actionCollection->addAction(action->objectName(), action);

    return action;
}

QAction *KUndo2Stack::createUndoAction(KisKActionCollection *actionCollection, const QString &actionName)
{
    QAction *action = KUndo2QStack::createUndoAction(actionCollection);

    if (actionName.isEmpty()) {
        action->setObjectName(KStandardAction::name(KStandardAction::Undo));
    } else {
        action->setObjectName(actionName);
    }

    action->setIcon(koIcon("edit-undo"));
    action->setIconText(i18n("Undo"));
    action->setShortcuts(KStandardShortcut::undo());

    actionCollection->addAction(action->objectName(), action);

    return action;
}
