/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <kundo2magicstring.h>

void KisNodeManager::mirrorNodeX()
{
    const KisNodeList nodes = MirrorAccess::selectedNodes(this);

    KUndo2MagicString commandName;
    if (nodes.size() == 1 && MirrorAccess::isMask(nodes[0])) {
        commandName = kundo2_i18n("Mirror Mask Horizontally");
    } else {
        commandName = kundo2_i18np("Mirror Layer Horizontally", "Mirror %1 Layers Horizontally", nodes.size());
    }
    mirrorNodes(nodes, commandName, Qt::Horizontal, MirrorAccess::selection(this));
}

void KisNodeManager::mirrorNodeY()
{
    const KisNodeList nodes = MirrorAccess::selectedNodes(this);

    KUndo2MagicString commandName;
    if (nodes.size() == 1 && MirrorAccess::isMask(nodes[0])) {
        commandName = kundo2_i18n("Mirror Mask Vertically");
    } else {
        commandName = kundo2_i18np("Mirror Layer Vertically", "Mirror %1 Layers Vertically", nodes.size());
    }
    mirrorNodes(nodes, commandName, Qt::Vertical, MirrorAccess::selection(this));
}

void KisNodeManager::mirrorAllNodesX()
{
    mirrorNode(MirrorAccess::rootNode(this),
               kundo2_i18n("Mirror All Layers Horizontally"),
               Qt::Horizontal,
               MirrorAccess::selection(this));
}

void KisNodeManager::mirrorAllNodesY()
{
    mirrorNode(MirrorAccess::rootNode(this),
               kundo2_i18n("Mirror All Layers Vertically"),
               Qt::Vertical,
               MirrorAccess::selection(this));
}

void KisNodeManager::mirrorNode(KisNodeSP node,
                                const KUndo2MagicString &actionName,
                                Qt::Orientation orientation,
                                KisSelectionSP selection)
{
    mirrorNodes({node}, actionName, orientation, selection);
}

void KisNodeManager::mirrorNodes(KisNodeList nodes,
                                 const KUndo2MagicString &actionName,
                                 Qt::Orientation orientation,
                                 KisSelectionSP selection)
{
    for (const KisNodeSP &node : nodes) {
        if (!MirrorAccess::canModifyLayer(this, node)) {
            return;
        }
    }

    MirrorAccess::applyToNodes(this, nodes, orientation, selection, actionName);
    MirrorAccess::nodesUpdated(this);
}
