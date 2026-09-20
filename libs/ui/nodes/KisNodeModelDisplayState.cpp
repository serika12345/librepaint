/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

bool KisNodeModel::showGlobalSelection() const
{
    return DisplayStateAccess::hasDisplayModeAdapter(this) ? DisplayStateAccess::showGlobalSelectionMask(this) : false;
}

void KisNodeModel::setPreferredThumnalSize(int preferredSize) const
{
    DisplayStateAccess::setPreferredThumbnailSize(this, preferredSize);
}

void KisNodeModel::setShowGlobalSelection(bool value)
{
    if (DisplayStateAccess::hasDisplayModeAdapter(this)) {
        DisplayStateAccess::setShowGlobalSelectionMask(this, value);
    }
}

void KisNodeModel::setIdleTaskManager(KisIdleTasksManager *idleTasksManager)
{
    DisplayStateAccess::setIdleTaskManager(this, idleTasksManager);
}

bool KisNodeModel::hasDummiesFacade()
{
    return DisplayStateAccess::hasDummiesFacade(this);
}
