/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::slotImageRequestNodeReselection(KisNodeSP activeNode, const KisNodeList &selectedNodes)
{
    if (activeNode) {
        slotNonUiActivatedNode(activeNode);
    }
    if (!selectedNodes.isEmpty()) {
        slotSetSelectedNodes(selectedNodes);
    }
}

void KisNodeManager::slotSetSelectedNodes(const KisNodeList &nodes)
{
    SelectionStateAccess::setSelectedNodes(this, nodes);
    Q_EMIT sigUiNeedChangeSelectedNodes(nodes);
}
