/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_types.h"
#include "nodes/kis_node_manager.h"
#include <QPointer>
#include <QtGlobal>

void KisNodeManager::slotImageRequestNodeReselection(KisNodeSP activeNode, const KisNodeList &selectedNodes)
{
    const QPointer<KisNodeManager> lifetime(this);
    if (activeNode) {
        slotNonUiActivatedNode(activeNode);
    }
    if (lifetime && !selectedNodes.isEmpty()) {
        slotSetSelectedNodes(selectedNodes);
    }
}

void KisNodeManager::slotSetSelectedNodes(const KisNodeList &nodes)
{
    const QPointer<KisNodeManager> lifetime(this);
    SelectionStateAccess::setSelectedNodes(this, nodes);
    if (lifetime) Q_EMIT sigUiNeedChangeSelectedNodes(nodes);
}
