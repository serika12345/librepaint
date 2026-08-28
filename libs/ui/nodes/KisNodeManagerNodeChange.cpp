/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

qint32 KisNodeManager::convertOpacityToInt(qreal opacity)
{
    return qMin(255, int(opacity * 2.55 + 0.5));
}

void KisNodeManager::setNodeName(KisNodeSP node, const QString &name)
{
    if (!node || NodeChangeAccess::name(node) == name) {
        return;
    }
    NodeChangeAccess::setName(this, node, name);
}

void KisNodeManager::setNodeOpacity(KisNodeSP node, qint32 opacity)
{
    if (!node || NodeChangeAccess::opacity(node) == opacity) {
        return;
    }
    NodeChangeAccess::setOpacity(this, node, opacity);
}

void KisNodeManager::setNodeCompositeOp(KisNodeSP node, const KoCompositeOp *compositeOp)
{
    if (!node || NodeChangeAccess::compositeOp(node) == compositeOp) {
        return;
    }
    NodeChangeAccess::setCompositeOp(this, node, compositeOp);
}

void KisNodeManager::nodeOpacityChanged(qreal opacity)
{
    setNodeOpacity(activeNode(), convertOpacityToInt(opacity));
}

void KisNodeManager::nodeCompositeOpChanged(const KoCompositeOp *op)
{
    setNodeCompositeOp(activeNode(), op);
}
