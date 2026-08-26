/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_change_frame_action.h"
#include "KisApplicationInputActions.h"

#include <klocalizedstring.h>
#include "application/ui/orchestration/kis_action.h"
#include "kis_input_manager.h"
#include "kis_canvas2.h"
#include "application/ui/workspace/KisViewManager.h"
#include "application/ui/orchestration/kis_action_manager.h"


struct KisChangeFrameAction::Private
{
};

KisChangeFrameAction::KisChangeFrameAction()
    : KisAbstractInputAction("Switch Time"),
      m_d(new Private)
{
    setName(i18n("Switch Time"));
    setDescription(i18n("The <i>Switch Time</i> action changes the current time of the animation."));

    QHash< QString, int > shortcuts;
    shortcuts.insert(i18n("Next Frame"), NextFrameShortcut);
    shortcuts.insert(i18n("Previous Frame"), PreviousFrameShortcut);
    setShortcutIndexes(shortcuts);
}

KisChangeFrameAction::~KisChangeFrameAction()
{
}

bool KisChangeFrameAction::isAvailable() const
{
    return applicationInputCanvas(inputManager())->viewManager()->activeNodeIsAnimated();
}

void KisChangeFrameAction::begin(int shortcut, QEvent *event)
{
    KisAbstractInputAction::begin(shortcut, event);

    switch(shortcut) {
    case NextFrameShortcut: {
        KisAction *action = applicationInputCanvas(inputManager())->viewManager()->actionManager()->actionByName("next_frame");
        if (action) {
            action->trigger();
        }
        break;
    }
    case PreviousFrameShortcut: {
        KisAction *action = applicationInputCanvas(inputManager())->viewManager()->actionManager()->actionByName("previous_frame");
        if (action) {
            action->trigger();
        }
        break;
    }
    }
}
