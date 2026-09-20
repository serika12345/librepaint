/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::moveNodeAt(KisNodeSP node, KisNodeSP parent, int index)
{
    TreeOperationAccess::moveNodeAt(this, node, parent, index);
}

void KisNodeManager::moveNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    TreeOperationAccess::moveNodes(this, nodes, parent, aboveThis, TreeOperationAccess::activeNode(this));
}

void KisNodeManager::copyNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    TreeOperationAccess::copyNodes(this, nodes, parent, aboveThis, TreeOperationAccess::activeNode(this));
}

void KisNodeManager::addNodesDirect(KisNodeList nodes, KisNodeSP parent, KisNodeSP aboveThis)
{
    TreeOperationAccess::addNodes(this, nodes, parent, aboveThis, TreeOperationAccess::activeNode(this));
}

void KisNodeManager::addNodeUndoable(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis)
{
    TreeOperationAccess::addNodeUndoable(this, node, parent, aboveThis);
}

void KisNodeManager::duplicateActiveNode()
{
    TreeOperationAccess::duplicateNodes(this,
                                        TreeOperationAccess::selectedNodes(this),
                                        TreeOperationAccess::activeNode(this));
}
