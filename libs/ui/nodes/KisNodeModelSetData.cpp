/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

bool KisNodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == KisNodeModel::DropEnabled) {
        const QMimeData *mimeData = static_cast<const QMimeData *>(value.value<void *>());
        SetDataAccess::setDropEnabled(this, mimeData);
        return true;
    }

    if (role == KisNodeModel::ActiveRole || role == KisNodeModel::AlternateActiveRole) {
        QModelIndex parentIndex;
        if (!index.isValid()) {
            parentIndex = SetDataAccess::takeParentOfRemovedNode(this);
        }

        KisNodeSP activatedNode;
        if (index.isValid() && value.toBool()) {
            activatedNode = SetDataAccess::nodeFromIndex(this, index);
        } else if (parentIndex.isValid() && value.toBool()) {
            activatedNode = SetDataAccess::nodeFromIndex(this, parentIndex);
        }

        const QModelIndex newActiveNode =
            activatedNode ? SetDataAccess::indexFromNode(this, activatedNode) : QModelIndex();
        if (role == KisNodeModel::ActiveRole && value.toBool()
            && SetDataAccess::activeNodeIndex(this) == newActiveNode) {
            return true;
        }

        SetDataAccess::setActiveNodeIndex(this, newActiveNode);
        SetDataAccess::setSelectionAdapterActiveNode(this, activatedNode);

        if (role == KisNodeModel::AlternateActiveRole) {
            Q_EMIT toggleIsolateActiveNode();
        }

        Q_EMIT dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(SetDataAccess::dummyColumns(this)));
        return true;
    }

    return SetDataAccess::setRemainingData(this, index, value, role);
}
