/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <QStringList>

void KisNodeManager::slotSomethingActivatedNodeImpl(KisNodeSP node)
{
    KisDummiesFacadeBase *const facade = ActivationAccess::dummiesFacade(this);
    if (!facade || !ActivationAccess::isNodeVisible(this, node)) {
        return;
    }

    KIS_ASSERT_RECOVER_RETURN(node != ActivationAccess::activeNode(this));
    if (!ActivationAccess::activateNode(this, node)) {
        return;
    }

    if (node) {
        ActivationAccess::setLastActivatedNode(facade, node);
    }
    ActivationAccess::notifyUiNodeChange(this, node);
    ActivationAccess::notifyNodeActivated(this, node);
    ActivationAccess::nodesUpdated(this);

    if (node && ActivationAccess::canvasOnly(this)) {
        ActivationAccess::showNodeName(this, ActivationAccess::nodeName(node));
    }
}

void KisNodeManager::slotNonUiActivatedNode(KisNodeSP node)
{
    if (node && !ActivationAccess::hasGraphListener(node)) {
        node.clear();
    }

    if (node == ActivationAccess::activeNode(this)) {
        return;
    }

    slotSomethingActivatedNodeImpl(node);
}

void KisNodeManager::slotUiActivatedNode(KisNodeSP node)
{
    if (node && !ActivationAccess::hasGraphListener(node)) {
        node.clear();
    }

    if (node) {
        const QStringList vectorTools{QStringLiteral("InteractionTool"),
                                      QStringLiteral("KarbonGradientTool"),
                                      QStringLiteral("KarbonCalligraphyTool"),
                                      QStringLiteral("PathTool")};
        const QStringList pixelTools{QStringLiteral("KritaShape/KisToolBrush"),
                                     QStringLiteral("KritaShape/KisToolDyna"),
                                     QStringLiteral("KritaShape/KisToolMultiBrush"),
                                     QStringLiteral("KritaFill/KisToolFill"),
                                     QStringLiteral("KritaFill/KisToolGradient")};
        if (ActivationAccess::nodeHasVectorAbilities(node)) {
            if (pixelTools.contains(ActivationAccess::activeToolId())) {
                ActivationAccess::switchTool(QStringLiteral("InteractionTool"));
            }
        } else if (vectorTools.contains(ActivationAccess::activeToolId())) {
            ActivationAccess::switchTool(QStringLiteral("KritaShape/KisToolBrush"));
        }
    }

    if (node == ActivationAccess::activeNode(this)) {
        return;
    }

    slotSomethingActivatedNodeImpl(node);
}
