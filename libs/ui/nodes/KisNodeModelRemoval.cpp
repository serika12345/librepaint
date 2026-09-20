/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

void KisNodeModel::slotBeginRemoveDummy(KisNodeDummy *dummy)
{
    if (!dummy) {
        return;
    }

    const RemovalAccess::Plan plan = RemovalAccess::prepare(this, dummy);

    if (plan.itemIndex.isValid()) {
        RemovalAccess::disconnectDummy(this, dummy);
        Q_EMIT sigBeforeBeginRemoveRows(plan.parentIndex, plan.itemIndex.row(), plan.itemIndex.row());
        RemovalAccess::beginRemoval(this, plan);
    }

    RemovalAccess::notifyNodeRemoved(this, dummy);
}
