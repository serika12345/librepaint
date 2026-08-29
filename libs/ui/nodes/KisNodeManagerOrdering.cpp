/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::raiseNode()
{
    if (!OrderingAccess::canMoveLayers(this, OrderingAccess::selectedNodes(this))) {
        return;
    }

    OrderingAccess::raiseNodes(this, OrderingAccess::selectedNodes(this), OrderingAccess::activeNode(this));
}

void KisNodeManager::lowerNode()
{
    if (!OrderingAccess::canMoveLayers(this, OrderingAccess::selectedNodes(this))) {
        return;
    }

    OrderingAccess::lowerNodes(this, OrderingAccess::selectedNodes(this), OrderingAccess::activeNode(this));
}
