/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::activateNextSiblingNode()
{
    NavigationAccess::activateNextNode(this, true);
}

void KisNodeManager::activatePreviousSiblingNode()
{
    NavigationAccess::activatePreviousNode(this, true);
}
