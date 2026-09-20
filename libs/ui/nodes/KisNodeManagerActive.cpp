/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

KisNodeSP KisNodeManager::activeNode()
{
    return ActiveAccess::activeNode(this);
}

KisLayerSP KisNodeManager::activeLayer()
{
    return ActiveAccess::activeLayer(this);
}

KisPaintDeviceSP KisNodeManager::activePaintDevice()
{
    if (ActiveAccess::hasActiveMask(this)) {
        return ActiveAccess::activeMaskDevice(this);
    }
    return ActiveAccess::activeLayerDevice(this);
}

const KoColorSpace *KisNodeManager::activeColorSpace()
{
    if (ActiveAccess::hasActiveMaskDevice(this)) {
        return ActiveAccess::activeMaskColorSpace(this);
    }

    Q_ASSERT(ActiveAccess::hasActiveLayer(this));
    if (ActiveAccess::activeLayerHasParent(this)) {
        return ActiveAccess::activeLayerParentColorSpace(this);
    }
    return ActiveAccess::imageColorSpace(this);
}
