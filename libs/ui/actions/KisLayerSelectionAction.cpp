/*
 * SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLayerSelectionAction.h"

#include <QAction>
#include <QMenu>

#include <klocalizedstring.h>

#include <kis_canvas2.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <nodes/kis_node_manager.h>
#include <kis_painting_utils.h>
#include <workspace/KisViewManager.h>

namespace
{
enum LayerSelectionMode {
    TopLayer,
    AllLayers,
    Ask
};

enum SelectionOverrideMode {
    Replace,
    Add
};

void selectNodes(KisCanvas2 *canvas,
                 const KisNodeList &nodesToSelect,
                 int selectionOverrideMode,
                 bool includeGroups)
{
    KisNodeManager *nodeManager = canvas->viewManager()->nodeManager();
    KisNodeSP activeNode = nodeManager->activeNode();
    KisNodeList selectedNodes;

    if (includeGroups) {
        selectedNodes = nodesToSelect;
    } else {
        for (const KisNodeSP &node : nodesToSelect) {
            if (!dynamic_cast<const KisGroupLayer *>(node.data())) {
                selectedNodes.append(node);
            }
        }
    }

    for (const KisNodeSP &node : selectedNodes) {
        KisNodeSP parent = node->parent();
        while (parent) {
            if (dynamic_cast<KisGroupLayer *>(parent.data())) {
                parent->setCollapsed(false);
            }
            parent = parent->parent();
        }
    }

    if (selectionOverrideMode == Add) {
        const KisNodeList currentSelection = nodeManager->selectedNodes();
        for (const KisNodeSP &node : currentSelection) {
            if (!selectedNodes.contains(node)) {
                selectedNodes.append(node);
            }
        }
    }

    if (!selectedNodes.contains(activeNode)) {
        activeNode = selectedNodes.last();
    }
    nodeManager->slotImageRequestNodeReselection(activeNode, selectedNodes);
}
}

void KisLayerSelectionAction::select(KisCanvas2 *canvas,
                                     const QPoint &imagePosition,
                                     const QPoint &widgetPosition,
                                     int layerSelectionMode,
                                     int selectionOverrideMode)
{
    KisNodeList nodesToSelect;
    if (layerSelectionMode == TopLayer) {
        KisNodeSP foundNode = KisPaintingUtils::findNode(canvas->image()->root(), imagePosition, false);
        if (!foundNode) {
            return;
        }
        nodesToSelect.append(foundNode);
    } else {
        const KisNodeList foundNodes = KisPaintingUtils::findNodes(
            canvas->image()->root()->firstChild(), imagePosition, false, layerSelectionMode == Ask);
        if (foundNodes.isEmpty()) {
            return;
        }

        if (layerSelectionMode == AllLayers) {
            nodesToSelect = foundNodes;
        } else {
            QWidget *canvasWidget = canvas->canvasWidget();
            QMenu *menu = new QMenu(canvasWidget);
            menu->setAttribute(Qt::WA_DeleteOnClose);
            int numberOfLayers = 0;

            for (int i = foundNodes.size() - 1; i >= 0; --i) {
                const KisNodeSP node = foundNodes[i];
                int indentation = -1;
                for (KisNodeSP parent = node; parent->parent(); parent = parent->parent()) {
                    ++indentation;
                }
                QAction *action = menu->addAction(QString(4 * indentation, ' ') + node->name());
                QObject::connect(action, &QAction::triggered, [canvas, node, selectionOverrideMode]() {
                    selectNodes(canvas, {node}, selectionOverrideMode, true);
                });
                if (!dynamic_cast<const KisGroupLayer *>(node.data())) {
                    ++numberOfLayers;
                }
            }

            menu->addSeparator();
            QAction *selectAllAction = menu->addAction(
                i18nc("Menu entry for the select layer under cursor canvas input action", "Select all layers"));
            selectAllAction->setVisible(numberOfLayers > 1);
            QObject::connect(selectAllAction,
                             &QAction::triggered,
                             [canvas, foundNodes, selectionOverrideMode]() {
                                 selectNodes(canvas, foundNodes, selectionOverrideMode, false);
                             });
            menu->popup(canvasWidget->mapToGlobal(widgetPosition));
            return;
        }
    }

    selectNodes(canvas, nodesToSelect, selectionOverrideMode, true);
}
