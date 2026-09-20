/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

KisNodeSP KisNodeManager::nearestNodeAfterRemoval(KisNodeSP node) const
{
    return ImageStateAccess::nearestNodeAfterRemoval(node);
}

bool KisNodeManager::activeNodeIsAnimated()
{
    KisNodeSP node = activeNode();
    return node && ImageStateAccess::isAnimated(node);
}

bool KisNodeManager::activeSelectionIsEditable()
{
    KisNodeSP layer = ImageStateAccess::activeLayerNode(this);
    if (layer) {
        KisNodeSP mask = ImageStateAccess::selectionMaskNode(layer);
        if (mask) {
            return ImageStateAccess::isEditable(mask);
        }
    }

    return true;
}

void KisNodeManager::updateImageNodeSettings(KisImageWSP image)
{
    ImageStateAccess::updateImageNodeSettings(image);
}

void KisNodeManager::createNodeActivationActions(KisImageWSP image, KisKActionCollection *collection)
{
    ImageStateAccess::createNodeActivationActions(image, collection, this);
}

KisLayerSP KisNodeManager::layerForNode(KisNodeSP node) const
{
    return ImageStateAccess::toLayer(owningLayerNode(node));
}

KisMaskSP KisNodeManager::maskForNode(KisNodeSP node) const
{
    return ImageStateAccess::toMask(node);
}

KisSelectionSP KisNodeManager::selectionForNode(KisNodeSP node, KisImageWSP image) const
{
    KisNodeSP layer = owningLayerNode(node);
    if (layer) {
        return ImageStateAccess::selection(layer);
    }
    return ImageStateAccess::globalSelection(image);
}

KisNodeSP KisNodeManager::owningLayerNode(KisNodeSP node) const
{
    if (ImageStateAccess::isMask(node)) {
        node = ImageStateAccess::parentNode(node);
    }
    return ImageStateAccess::isLayer(node) ? node : KisNodeSP();
}
