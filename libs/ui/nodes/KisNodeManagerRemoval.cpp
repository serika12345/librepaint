/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::removeSingleNode(KisNodeSP node)
{
    if (!node || !RemovalAccess::parentNode(node)) {
        return;
    }

    removeSelectedNodes(KisNodeList{node});
}

void KisNodeManager::removeSelectedNodes(KisNodeList nodes)
{
    if (!RemovalAccess::canModifyLayers(this, nodes)) {
        return;
    }

    RemovalAccess::removeNodes(this, nodes, RemovalAccess::activeNode(this));
}

void KisNodeManager::removeNode()
{
    removeSelectedNodes(RemovalAccess::selectedNodes(this));
}
