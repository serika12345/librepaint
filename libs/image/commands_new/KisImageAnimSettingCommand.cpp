/*
 *  SPDX-FileCopyrightText: 2025 Emmet O 'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageAnimSettingCommand.h"
#include "KisImageAnimSettingCommandAnimationAccess_p.h"

#include "kis_command_ids.h"
#include "kis_time_span.h"

KisImageAnimSettingCommand::KisImageAnimSettingCommand(KisImageAnimationInterface *const p_animInterface,
                                                       Settings p_after,
                                                       KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Update Animation Settings"), parent)
    , m_animInterface(p_animInterface)
    , m_after(p_after)
{
    m_before = {kisImageAnimSettingCommandFramerate(p_animInterface),
                kisImageAnimSettingCommandDocumentRange(p_animInterface).start(),
                kisImageAnimSettingCommandDocumentRange(p_animInterface).end()};
}

void KisImageAnimSettingCommand::redo()
{
    // SET image animation settings to after values..
    KIS_ASSERT(m_animInterface);

    KUndo2Command::redo();

    kisImageAnimSettingCommandSetFramerate(m_animInterface, m_after.FPS);
    kisImageAnimSettingCommandSetDocumentRange(m_animInterface,
                                               KisTimeSpan::fromTimeToTime(m_after.startFrame, m_after.endFrame));
}

void KisImageAnimSettingCommand::undo()
{
    // RESET image animation settings back to before values..
    KIS_ASSERT(m_animInterface);

    KUndo2Command::undo();

    kisImageAnimSettingCommandSetFramerate(m_animInterface, m_before.FPS);
    kisImageAnimSettingCommandSetDocumentRange(m_animInterface,
                                               KisTimeSpan::fromTimeToTime(m_before.startFrame, m_before.endFrame));
}

int KisImageAnimSettingCommand::id() const
{
    return KisCommandUtils::ImageAnimSettingCommandId;
}

bool KisImageAnimSettingCommand::canMergeWith(const KUndo2Command *p_other) const
{
    const KisImageAnimSettingCommand *other = dynamic_cast<const KisImageAnimSettingCommand *>(p_other);

    return other != nullptr;
}

bool KisImageAnimSettingCommand::mergeWith(const KUndo2Command *p_next)
{
    const KisImageAnimSettingCommand *next = dynamic_cast<const KisImageAnimSettingCommand *>(p_next);

    if (!next)
        return false;

    m_after = next->m_after;

    return true;
}
