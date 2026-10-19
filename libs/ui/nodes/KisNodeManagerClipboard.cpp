/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"
#include "kis_types.h"

#include <kundo2magicstring.h>
#include <qpoint.h>
#include <QPointer>

void KisNodeManager::cutLayersToClipboard()
{
    KisNodeList nodes = ClipboardAccess::selectedNodes(this);
    if (nodes.isEmpty()) {
        return;
    }

    auto it = nodes.begin();
    while (it != nodes.end()) {
        if (!ClipboardAccess::parentNode(*it)) {
            it = nodes.erase(it);
        } else {
            ++it;
        }
    }

    const QPointer<KisNodeManager> guard(this);
    if (!ClipboardAccess::setLayers(this, nodes, false) || !guard) return;

    if (ClipboardAccess::canModifyLayers(this, nodes)) {
        ClipboardAccess::removeNodes(this, nodes, kundo2_i18n("Cut Nodes"));
    }
}

void KisNodeManager::copyLayersToClipboard()
{
    ClipboardAccess::setLayers(this, ClipboardAccess::selectedNodes(this), true);
}

void KisNodeManager::pasteLayersFromClipboard(bool changeOffset, QPointF offset, KisProcessingApplicator *applicator)
{
    const QMimeData *data = ClipboardAccess::layersMimeData();
    if (!data) {
        return;
    }

    KisNodeSP targetNode = ClipboardAccess::activeNode(this);
    if (!targetNode) {
        targetNode = ClipboardAccess::rootNode(this);
    }

    ClipboardAccess::insertMimeLayersAsLastChild(this, data, targetNode, false, changeOffset, offset, applicator);
}
