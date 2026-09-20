/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

KisNodeList KisNodeManager::selectedNodes()
{
    return AccessorAccess::selectedNodes(this);
}

KisNodeSelectionAdapter *KisNodeManager::nodeSelectionAdapter() const
{
    return AccessorAccess::nodeSelectionAdapter(this);
}

KisNodeInsertionAdapter *KisNodeManager::nodeInsertionAdapter() const
{
    return AccessorAccess::nodeInsertionAdapter(this);
}

KisNodeDisplayModeAdapter *KisNodeManager::nodeDisplayModeAdapter() const
{
    return AccessorAccess::nodeDisplayModeAdapter(this);
}
