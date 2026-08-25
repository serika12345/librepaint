/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_group_operations.h"

#include <algorithm>

#include <KoProperties.h>

#include "kis_group_layer.h"
#include "kis_image.h"
#include "kis_layer_utils.h"
#include "kis_node_operation_batch.h"

namespace KisNodeGroupOperations
{

bool createGroup(KisNodeOperationBatch *batch,
                 KisImageSP image,
                 const KisNodeList &nodes,
                 KisNodeSP activeNode,
                 const QString &groupName,
                 KisNodeSP *newGroup,
                 KisNodeSP *newLastChild)
{
    if (!activeNode) {
        return false;
    }

    KisNodeList groupedNodes = KisLayerUtils::sortMergeableNodes(image->root(), nodes);
    KisLayerUtils::filterMergeableNodes(groupedNodes);

    if (groupedNodes.isEmpty()) {
        return false;
    }

    if (KisLayerUtils::checkIsChildOf(activeNode, groupedNodes)) {
        activeNode = groupedNodes.first();
    }

    KisGroupLayerSP group = new KisGroupLayer(image.data(), groupName, OPACITY_OPAQUE_U8);
    batch->addNode(KisNodeList() << group, activeNode->parent(), activeNode, activeNode);
    batch->moveNode(groupedNodes, group, KisNodeSP(), activeNode);

    if (newGroup) {
        *newGroup = group;
    }
    if (newLastChild) {
        *newLastChild = groupedNodes.last();
    }

    return true;
}

bool ungroupNodes(KisNodeOperationBatch *batch,
                  const KisNodeList &selectedNodes,
                  KisNodeSP activeNode,
                  KisNodeSP *incompatibleNode,
                  KisNodeSP *destinationParent)
{
    if (incompatibleNode) {
        incompatibleNode->clear();
    }
    if (destinationParent) {
        destinationParent->clear();
    }
    if (!activeNode) {
        return false;
    }

    const auto checkCanMoveLayers = [incompatibleNode, destinationParent]
        (const KisNodeList &nodes, KisNodeSP newParent) {
        const auto it = std::find_if(nodes.cbegin(), nodes.cend(),
                                     [newParent](KisNodeSP node) {
            return !newParent->allowAsChild(node);
        });

        if (it == nodes.cend()) {
            return true;
        }

        if (incompatibleNode) {
            *incompatibleNode = *it;
        }
        if (destinationParent) {
            *destinationParent = newParent;
        }
        return false;
    };

    KisNodeSP parent = activeNode->parent();
    if (parent && dynamic_cast<KisGroupLayer *>(activeNode.data())) {
        const KisNodeList nodes = activeNode->childNodes(QStringList(), KoProperties());
        if (!checkCanMoveLayers(nodes, parent)) {
            return false;
        }

        batch->moveNode(nodes, parent, activeNode, activeNode);
        batch->removeNode(KisNodeList() << activeNode, activeNode);
        if (destinationParent) {
            *destinationParent = parent;
        }
        return true;
    }

    if (!parent || !parent->parent()) {
        return false;
    }

    KisNodeSP grandParent = parent->parent();
    if (!checkCanMoveLayers(selectedNodes, grandParent)) {
        return false;
    }

    const KisNodeList allChildNodes = parent->childNodes(QStringList(), KoProperties());
    const bool removeParent = KritaUtils::compareListsUnordered(allChildNodes, selectedNodes);

    batch->moveNode(selectedNodes, grandParent, parent, activeNode);
    if (removeParent) {
        batch->removeNode(KisNodeList() << parent, activeNode);
    }
    if (destinationParent) {
        *destinationParent = grandParent;
    }
    return true;
}

}
