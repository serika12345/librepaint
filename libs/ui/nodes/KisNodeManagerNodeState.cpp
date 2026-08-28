/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

bool KisNodeManager::activeNodeIsLayer()
{
    const KisNodeSP node = activeNode();
    return node && NodeStateAccess::isLayer(node);
}

bool KisNodeManager::activeNodeInherits(const QString &type)
{
    const KisNodeSP node = activeNode();
    return node && NodeStateAccess::inheritsType(node, type);
}

bool KisNodeManager::activeNodeIsEditable()
{
    const KisNodeSP node = activeNode();
    return node && NodeStateAccess::isEditable(node);
}

bool KisNodeManager::activeNodeHasEditablePaintDevice()
{
    const KisNodeSP node = activeNode();
    return node && NodeStateAccess::hasEditablePaintDevice(node);
}

bool KisNodeManager::isNodeHidden(KisNodeSP node, bool isGlobalSelectionHidden)
{
    if (node && NodeStateAccess::isFakeNode(node)) {
        return true;
    }

    if (isGlobalSelectionHidden && NodeStateAccess::isSelectionMask(node)) {
        const KisNodeSP parent = NodeStateAccess::parentNode(node);
        return !parent || !NodeStateAccess::parentNode(parent);
    }

    return false;
}
