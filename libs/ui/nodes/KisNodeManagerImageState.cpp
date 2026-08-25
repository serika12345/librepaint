/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

#include "nodes/KisNodeActivationActionCreatorVisitor.h"

#include <kis_image.h>
#include <kis_layer.h>
#include <kis_mask.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <krita_utils.h>

KisNodeSP KisNodeManager::nearestNodeAfterRemoval(KisNodeSP node) const
{
    return KritaUtils::nearestNodeAfterRemoval(node);
}

bool KisNodeManager::activeNodeIsAnimated()
{
    KisNodeSP node = activeNode();
    return node && node->isAnimated();
}

bool KisNodeManager::activeSelectionIsEditable()
{
    KisLayerSP layer = activeLayer();
    if (layer) {
        KisSelectionMaskSP mask = layer->selectionMask();
        if (mask) {
            return mask->isEditable();
        }
    }

    return true;
}

void KisNodeManager::updateImageNodeSettings(KisImageWSP image)
{
    if (image) {
        image->rootLayer()->updateSettings();
    }
}

void KisNodeManager::createNodeActivationActions(KisImageWSP image, KisKActionCollection *collection)
{
    if (image) {
        KisNodeActivationActionCreatorVisitor visitor(collection, this);
        image->rootLayer()->accept(visitor);
    }
}

KisLayerSP KisNodeManager::layerForNode(KisNodeSP node) const
{
    KisMaskSP mask = maskForNode(node);
    if (mask) {
        node = mask->parent();
    }
    return qobject_cast<KisLayer *>(node.data());
}

KisMaskSP KisNodeManager::maskForNode(KisNodeSP node) const
{
    return dynamic_cast<KisMask *>(node.data());
}

KisSelectionSP KisNodeManager::selectionForNode(KisNodeSP node, KisImageWSP image) const
{
    KisLayerSP layer = layerForNode(node);
    if (layer) {
        return layer->selection();
    }
    return image ? image->globalSelection() : KisSelectionSP();
}
