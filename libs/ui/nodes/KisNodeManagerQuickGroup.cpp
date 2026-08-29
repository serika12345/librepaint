/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <kundo2magicstring.h>

bool KisNodeManager::createQuickGroupImpl(KisNodeOperationBatch *batch,
                                          const QString &overrideGroupName,
                                          KisNodeSP *newGroup,
                                          KisNodeSP *newLastChild)
{
    const KisNodeSP active = QuickGroupAccess::activeNode(this);
    if (!active || !QuickGroupAccess::canMoveLayer(this, active)) {
        return false;
    }

    const QString groupName = !overrideGroupName.isEmpty()
        ? overrideGroupName
        : QuickGroupAccess::nextLayerName(this, i18nc("A group of layers", "Group"));

    return QuickGroupAccess::createGroup(batch,
                                         QuickGroupAccess::selectedNodes(this),
                                         active,
                                         groupName,
                                         newGroup,
                                         newLastChild);
}

void KisNodeManager::createQuickGroup()
{
    KisNodeOperationBatch *batch = QuickGroupAccess::operationBatch(this, kundo2_i18n("Quick Group"));

    KisNodeSP parent;
    KisNodeSP above;
    createQuickGroupImpl(batch, QString(), &parent, &above);
}

void KisNodeManager::createQuickClippingGroup()
{
    KisNodeOperationBatch *batch = QuickGroupAccess::operationBatch(this, kundo2_i18n("Quick Clipping Group"));

    KisNodeSP parent;
    KisNodeSP above;
    const QString groupName =
        QuickGroupAccess::nextLayerName(this, i18nc("default name for a clipping group layer", "Clipping Group"));
    if (createQuickGroupImpl(batch, groupName, &parent, &above)) {
        QuickGroupAccess::addClippingMask(this,
                                          batch,
                                          parent,
                                          above,
                                          i18nc("default name for quick clip group mask layer", "Mask Layer"));
    }
}

void KisNodeManager::quickUngroup()
{
    const KisNodeSP active = QuickGroupAccess::activeNode(this);
    if (!active || !QuickGroupAccess::canModifyLayer(this, active)) {
        return;
    }

    KisNodeSP incompatibleNode;
    KisNodeSP destinationParent;
    KisNodeOperationBatch *batch = QuickGroupAccess::operationBatch(this, kundo2_i18n("Quick Ungroup"));
    if (!QuickGroupAccess::ungroupNodes(batch,
                                        QuickGroupAccess::selectedNodes(this),
                                        active,
                                        &incompatibleNode,
                                        &destinationParent)
        && incompatibleNode && destinationParent) {
        const QString message = QuickGroupAccess::parentNode(destinationParent)
            ? i18n("Cannot move layer \"%1\" into new parent \"%2\"",
                   QuickGroupAccess::nodeName(incompatibleNode),
                   QuickGroupAccess::nodeName(destinationParent))
            : i18n("Cannot move layer \"%1\" into the root layer", QuickGroupAccess::nodeName(incompatibleNode));
        QuickGroupAccess::showFloatingMessage(this, message);
    }
}
