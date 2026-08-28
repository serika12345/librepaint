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

KisNodeSP KisNodeManager::ImageStateAccess::nearestNodeAfterRemoval(KisNodeSP node)
{
    return KritaUtils::nearestNodeAfterRemoval(node);
}

bool KisNodeManager::ImageStateAccess::isAnimated(KisNodeSP node)
{
    return node->isAnimated();
}

KisNodeSP KisNodeManager::ImageStateAccess::activeLayerNode(KisNodeManager *manager)
{
    return manager->activeLayer();
}

KisNodeSP KisNodeManager::ImageStateAccess::selectionMaskNode(KisNodeSP layer)
{
    return qobject_cast<KisLayer *>(layer.data())->selectionMask();
}

bool KisNodeManager::ImageStateAccess::isEditable(KisNodeSP mask)
{
    return qobject_cast<KisSelectionMask *>(mask.data())->isEditable();
}

void KisNodeManager::ImageStateAccess::updateImageNodeSettings(KisImageWSP image)
{
    if (image) {
        image->rootLayer()->updateSettings();
    }
}

void KisNodeManager::ImageStateAccess::createNodeActivationActions(KisImageWSP image,
                                                                   KisKActionCollection *collection,
                                                                   KisNodeManager *manager)
{
    if (image) {
        KisNodeActivationActionCreatorVisitor visitor(collection, manager);
        image->rootLayer()->accept(visitor);
    }
}

bool KisNodeManager::ImageStateAccess::isLayer(KisNodeSP node)
{
    return qobject_cast<KisLayer *>(node.data());
}

bool KisNodeManager::ImageStateAccess::isMask(KisNodeSP node)
{
    return dynamic_cast<KisMask *>(node.data());
}

KisNodeSP KisNodeManager::ImageStateAccess::parentNode(KisNodeSP mask)
{
    return dynamic_cast<KisMask *>(mask.data())->parent();
}

KisLayerSP KisNodeManager::ImageStateAccess::toLayer(KisNodeSP node)
{
    return qobject_cast<KisLayer *>(node.data());
}

KisMaskSP KisNodeManager::ImageStateAccess::toMask(KisNodeSP node)
{
    return dynamic_cast<KisMask *>(node.data());
}

KisSelectionSP KisNodeManager::ImageStateAccess::selection(KisNodeSP layer)
{
    return qobject_cast<KisLayer *>(layer.data())->selection();
}

KisSelectionSP KisNodeManager::ImageStateAccess::globalSelection(KisImageWSP image)
{
    return image ? image->globalSelection() : KisSelectionSP();
}
