/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

bool KisNodeManager::trySetNodeProperties(KisNodeSP node, KisImageSP image, KisBaseNode::PropertyList properties) const
{
    if (PropertyAccess::isPaintLayer(node) && PropertyAccess::containsOnionSkin(properties)
        && PropertyAccess::hasOpaqueBackground(node)) {
        PropertyAccess::showOnionSkinTransparencyWarning(this);
        return false;
    }

    PropertyAccess::applyProperties(node, image, properties);
    return true;
}
