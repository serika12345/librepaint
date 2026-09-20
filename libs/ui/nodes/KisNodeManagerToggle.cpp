/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

void KisNodeManager::toggleNodeProperty(ToggleProperty property)
{
    const KisNodeList nodes = ToggleAccess::selectedNodes(this);
    const KisNodeSP active = ToggleAccess::activeNode(this);
    if (nodes.isEmpty() || !active || !ToggleAccess::supportsProperty(active, property)) {
        return;
    }

    const bool value = !ToggleAccess::propertyState(active, property);
    for (const KisNodeSP &node : nodes) {
        if (ToggleAccess::supportsProperty(node, property)) {
            ToggleAccess::setProperty(this, node, property, value);
        }
    }
}

void KisNodeManager::toggleLock()
{
    toggleNodeProperty(ToggleProperty::Locked);
}

void KisNodeManager::toggleVisibility()
{
    toggleNodeProperty(ToggleProperty::Visible);
}

void KisNodeManager::toggleAlphaLock()
{
    toggleNodeProperty(ToggleProperty::AlphaLocked);
}

void KisNodeManager::toggleInheritAlpha()
{
    toggleNodeProperty(ToggleProperty::InheritAlpha);
}
