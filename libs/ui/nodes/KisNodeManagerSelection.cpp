/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::selectLayersImpl(SelectionProperty property, bool value, bool invertedValue)
{
    KisNodeList nodes = SelectionAccess::findNodes(this, property, value);
    const KisNodeList currentSelection = SelectionAccess::selectedNodes(this);

    if (SelectionAccess::sameNodesUnordered(nodes, currentSelection)) {
        nodes = SelectionAccess::findNodes(this, property, invertedValue);
    }

    if (!nodes.isEmpty()) {
        SelectionAccess::reselectNodes(this, nodes.last(), nodes);
    }
}

void KisNodeManager::selectAllNodes()
{
    selectLayersImpl(SelectionProperty::Any, true, true);
}

void KisNodeManager::selectVisibleNodes()
{
    selectLayersImpl(SelectionProperty::Visible, true, false);
}

void KisNodeManager::selectLockedNodes()
{
    selectLayersImpl(SelectionProperty::Locked, true, false);
}

void KisNodeManager::selectInvisibleNodes()
{
    selectLayersImpl(SelectionProperty::Visible, false, true);
}

void KisNodeManager::selectUnlockedNodes()
{
    selectLayersImpl(SelectionProperty::Locked, false, true);
}
