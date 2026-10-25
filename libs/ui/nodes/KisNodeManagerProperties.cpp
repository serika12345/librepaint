/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_base_node.h"
#include "kis_image.h"
#include "kis_layer_properties_icons.h"
#include "kis_node.h"
#include "kis_types.h"
#include "nodes/kis_node_manager.h"

bool KisNodeManager::trySetNodeProperties(KisNodeSP node, KisImageSP image, KisBaseNode::PropertyList properties) const
{
    if (PropertyAccess::isPaintLayer(node) && PropertyAccess::containsOnionSkin(properties)
        && PropertyAccess::hasOpaqueBackground(node)) {
        PropertyAccess::showOnionSkinTransparencyWarning(this);
        return false;
    }

    if (image && image->isGpuTransparencyMaskView(node)) {
        const QString visibleId = KisLayerPropertiesIcons::visible.id();
        bool hasVisible = false;
        bool visible = false;
        for (const auto &property : properties) {
            if (property.id == visibleId) {
                hasVisible = true;
                visible = property.state.toBool();
            }
        }
        if (hasVisible &&
            !image->setGpuTransparencyMaskVisible(node->parent(), visible)) return false;
        // The mirror node owns no document properties beyond mask visibility.
        return true;
    }

    PropertyAccess::applyProperties(node, image, properties);
    return true;
}
