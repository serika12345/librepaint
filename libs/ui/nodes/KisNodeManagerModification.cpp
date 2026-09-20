/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

#include <KLocalizedString>

bool KisNodeManager::canModifyLayers(KisNodeList nodes, bool showWarning)
{
    KisNodeSP lockedNode;
    for (KisNodeSP node : nodes) {
        if (!ModificationAccess::isEditable(node)) {
            lockedNode = node;
            break;
        }
    }

    if (lockedNode && showWarning) {
        const QString message = nodes.size() <= 1
            ? i18n("Layer is locked")
            : i18n("Layer \"%1\" is locked", ModificationAccess::name(lockedNode));
        ModificationAccess::showWarning(this, message);
    }

    return !lockedNode;
}

bool KisNodeManager::canModifyLayer(KisNodeSP node, bool showWarning)
{
    return canModifyLayers({node}, showWarning);
}

bool KisNodeManager::canMoveLayers(KisNodeList nodes, bool showWarning)
{
    KisNodeSP lockedParent;
    for (KisNodeSP node : nodes) {
        const KisNodeSP parent = ModificationAccess::parentNode(node);
        if (parent && !ModificationAccess::isEditable(parent)) {
            lockedParent = parent;
            break;
        }
    }

    if (lockedParent && showWarning) {
        ModificationAccess::showWarning(this, i18n("Layer \"%1\" is locked", ModificationAccess::name(lockedParent)));
    }

    return !lockedParent;
}

bool KisNodeManager::canMoveLayer(KisNodeSP node, bool showWarning)
{
    return canMoveLayers({node}, showWarning);
}
