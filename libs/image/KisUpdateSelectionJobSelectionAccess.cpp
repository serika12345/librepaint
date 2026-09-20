/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisUpdateSelectionJobSelectionAccess_p.h"

#include "kis_node.h"
#include "kis_projection_leaf.h"
#include "kis_selection.h"

KisNodeSP KisUpdateSelectionJobSelectionAccess::parentNode(KisSelection *selection)
{
    return selection->parentNode();
}

QRect KisUpdateSelectionJobSelectionAccess::extent(KisNode *node)
{
    return node->extent();
}

void KisUpdateSelectionJobSelectionAccess::updateProjection(KisSelection *selection)
{
    selection->updateProjection();
}

void KisUpdateSelectionJobSelectionAccess::updateProjection(KisSelection *selection, const QRect &rect)
{
    selection->updateProjection(rect);
}

void KisUpdateSelectionJobSelectionAccess::notifySelectionChanged(KisSelection *selection)
{
    selection->notifySelectionChanged();
}

bool KisUpdateSelectionJobSelectionAccess::isOverlayProjectionLeaf(KisNode *node)
{
    return node->projectionLeaf()->isOverlayProjectionLeaf();
}

void KisUpdateSelectionJobSelectionAccess::setDirty(KisNode *node, const QRect &rect)
{
    node->setDirty(rect);
}
