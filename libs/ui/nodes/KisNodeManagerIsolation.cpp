/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::toggleIsolateActiveNode()
{
    IsolationAccess::toggleLayerAction(this);
}

void KisNodeManager::setIsolateActiveLayerMode(bool checked)
{
    if (!IsolationAccess::imageAvailable(this)) {
        return;
    }

    changeIsolationMode(checked, IsolationAccess::isIsolatingGroup(this));
}

void KisNodeManager::setIsolateActiveGroupMode(bool checked)
{
    if (!IsolationAccess::imageAvailable(this)) {
        return;
    }

    changeIsolationMode(IsolationAccess::isIsolatingLayer(this), checked);
}

void KisNodeManager::changeIsolationMode(bool isolateActiveLayer, bool isolateActiveGroup)
{
    const bool imageAvailable = IsolationAccess::imageAvailable(this);
    const KisNodeSP active = IsolationAccess::activeNode(this);
    if (!imageAvailable || !active) {
        return;
    }

    if (isolateActiveLayer || isolateActiveGroup) {
        if (!IsolationAccess::startIsolatedMode(this, active, isolateActiveLayer, isolateActiveGroup)) {
            reinitializeIsolationActionGroup();
        }
    } else {
        IsolationAccess::stopIsolatedMode(this);
    }
}

void KisNodeManager::changeIsolationRoot(KisNodeSP isolationRoot)
{
    if (!IsolationAccess::imageAvailable(this) || !isolationRoot) {
        return;
    }

    const bool isolateActiveLayer = IsolationAccess::isIsolatingLayer(this);
    const bool isolateActiveGroup = IsolationAccess::isIsolatingGroup(this);
    if (!IsolationAccess::startIsolatedMode(this, isolationRoot, isolateActiveLayer, isolateActiveGroup)) {
        reinitializeIsolationActionGroup();
    }
}

void KisNodeManager::handleExternalIsolationChange()
{
    if (!IsolationAccess::isActiveWindow(this)) {
        return;
    }

    IsolationAccess::setLayerActionChecked(this, IsolationAccess::isIsolatingLayer(this));
    IsolationAccess::setGroupActionChecked(this, IsolationAccess::isIsolatingGroup(this));
}

void KisNodeManager::reinitializeIsolationActionGroup()
{
    IsolationAccess::setLayerActionChecked(this, false);
    IsolationAccess::setGroupActionChecked(this, false);
}
