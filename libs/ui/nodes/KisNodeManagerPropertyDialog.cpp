/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::nodeProperties(KisNodeSP node)
{
    if (PropertyDialogAccess::isLayer(node)) {
        PropertyDialogAccess::showLayerProperties(this);
    } else if (PropertyDialogAccess::isMask(node)) {
        PropertyDialogAccess::showMaskProperties(this);
    }
}

void KisNodeManager::nodePropertiesIgnoreSelection(KisNodeSP node)
{
    Q_ASSERT(node);

    const KisNodeSP originalNode = PropertyDialogAccess::currentNode(this);
    PropertyDialogAccess::setCurrentNode(this, node);
    nodeProperties(node);
    PropertyDialogAccess::setCurrentNode(this, originalNode);
}
