/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

#include <kis_layer.h>
#include <kis_node.h>
#include <kis_selection_mask.h>

bool KisNodeManager::NodeStateAccess::isLayer(KisNodeSP node)
{
    return qobject_cast<const KisLayer *>(node.data());
}

bool KisNodeManager::NodeStateAccess::inheritsType(KisNodeSP node, const QString &type)
{
    return node->inherits(type.toLatin1());
}

bool KisNodeManager::NodeStateAccess::isEditable(KisNodeSP node)
{
    return node->isEditable(false);
}

bool KisNodeManager::NodeStateAccess::hasEditablePaintDevice(KisNodeSP node)
{
    return node->hasEditablePaintDevice();
}

bool KisNodeManager::NodeStateAccess::isFakeNode(KisNodeSP node)
{
    return node->isFakeNode();
}

bool KisNodeManager::NodeStateAccess::isSelectionMask(KisNodeSP node)
{
    return dynamic_cast<KisSelectionMask *>(node.data());
}

KisNodeSP KisNodeManager::NodeStateAccess::parentNode(KisNodeSP node)
{
    return node->parent();
}
