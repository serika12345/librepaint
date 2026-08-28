/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::nodesUpdated()
{
    const KisNodeSP node = NodeUpdateAccess::activeNode(this);
    if (!node) {
        return;
    }

    NodeUpdateAccess::updateLayers(this);
    NodeUpdateAccess::updateMasks(this);
    NodeUpdateAccess::updateView(this);
    NodeUpdateAccess::notifySelectionChanged(this);
    NodeUpdateAccess::setTimelinePinned(this, NodeUpdateAccess::isPinnedToTimeline(node));
}
