/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <kundo2magicstring.h>

KisNodeSP KisNodeManager::createNode(const QString &nodeType, bool quiet, KisPaintDeviceSP copyFrom)
{
    if (!NodeTypeAccess::finishPendingOperations(this)) {
        return KisNodeSP();
    }

    KisNodeSP active = NodeTypeAccess::activeNode(this);
    if (!active) {
        active = NodeTypeAccess::rootNode(this);
    }
    KIS_ASSERT_RECOVER_RETURN_VALUE(active, KisNodeSP());

    NodeCreationKind kind;
    if (nodeType == QLatin1String("KisPaintLayer")) {
        kind = NodeCreationKind::PaintLayer;
    } else if (nodeType == QLatin1String("KisGroupLayer")) {
        kind = NodeCreationKind::GroupLayer;
    } else if (nodeType == QLatin1String("KisAdjustmentLayer")) {
        kind = NodeCreationKind::AdjustmentLayer;
    } else if (nodeType == QLatin1String("KisGeneratorLayer")) {
        kind = NodeCreationKind::GeneratorLayer;
    } else if (nodeType == QLatin1String("KisShapeLayer")) {
        kind = NodeCreationKind::ShapeLayer;
    } else if (nodeType == QLatin1String("KisCloneLayer")) {
        kind = NodeCreationKind::CloneLayer;
    } else if (nodeType == QLatin1String("KisTransparencyMask")) {
        kind = NodeCreationKind::TransparencyMask;
    } else if (nodeType == QLatin1String("KisFilterMask")) {
        kind = NodeCreationKind::FilterMask;
    } else if (nodeType == QLatin1String("FastColorOverlayFilterMask")) {
        kind = NodeCreationKind::FastColorOverlayMask;
    } else if (nodeType == QLatin1String("KisColorizeMask")) {
        kind = NodeCreationKind::ColorizeMask;
    } else if (nodeType == QLatin1String("KisTransformMask")) {
        kind = NodeCreationKind::TransformMask;
    } else if (nodeType == QLatin1String("KisSelectionMask")) {
        kind = NodeCreationKind::SelectionMask;
    } else if (nodeType == QLatin1String("KisFileLayer")) {
        kind = NodeCreationKind::FileLayer;
    } else {
        return KisNodeSP();
    }

    KisNodeList selected;
    if (kind == NodeCreationKind::CloneLayer) {
        selected = NodeTypeAccess::selectedNodes(this);
        if (selected.isEmpty()) {
            selected.append(active);
        }
    }

    return NodeTypeAccess::createNode(this, kind, active, selected, copyFrom.data(), quiet);
}

void KisNodeManager::convertNode(const QString &nodeType)
{
    if (!NodeTypeAccess::finishPendingOperations(this)) {
        return;
    }

    const KisNodeSP active = NodeTypeAccess::activeNode(this);
    if (!active || !NodeTypeAccess::canModifyLayer(this, active)) {
        return;
    }

    if (nodeType == QLatin1String("KisPaintLayer")) {
        NodeTypeAccess::convertNode(this, NodeConversionKind::PaintLayer, active);
    } else if (nodeType == QLatin1String("KisFileLayer")) {
        NodeTypeAccess::convertNode(this, NodeConversionKind::FileLayer, active);
    } else if (nodeType == QLatin1String("KisSelectionMask") || nodeType == QLatin1String("KisFilterMask")
               || nodeType == QLatin1String("KisTransparencyMask")) {
        const NodeConversionKind kind = nodeType == QLatin1String("KisSelectionMask")
            ? NodeConversionKind::SelectionMask
            : nodeType == QLatin1String("KisFilterMask") ? NodeConversionKind::FilterMask
                                                         : NodeConversionKind::TransparencyMask;
        KisPaintDevice *copyFrom = NodeTypeAccess::paintDevice(active);
        if (!copyFrom) {
            copyFrom = NodeTypeAccess::projection(active);
        }

        NodeTypeAccess::beginConversion(this, kundo2_i18n("Convert to a Selection Mask"));
        const bool converted = NodeTypeAccess::convertToMask(this, kind, active, copyFrom);
        NodeTypeAccess::endConversion(this);
        if (!converted) {
            NodeTypeAccess::finishPendingOperationsForced(this);
            NodeTypeAccess::undoLastConversion(this);
        }
    } else {
        NodeTypeAccess::reportUnsupportedNodeType(nodeType);
    }
}
