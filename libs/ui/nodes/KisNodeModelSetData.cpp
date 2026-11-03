/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"
#include "kis_types.h"
#include <QPersistentModelIndex>
#include <QPointer>
#include <qabstractitemmodel.h>
#include <QtGlobal>
#include <qvariant.h>

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

        // Activating a GPU node can synchronously rebuild the facade and reset
        // the model. The reset already notifies views about the replaced rows.
        const QPersistentModelIndex changedIndex(index);
        const QPointer<KisNodeModel> model(this);
        SetDataAccess::setActiveNodeIndex(this, newActiveNode);
        SetDataAccess::setSelectionAdapterActiveNode(this, activatedNode);
        if (!model) {
            return true;
        }

        if (role == KisNodeModel::AlternateActiveRole) {
            Q_EMIT toggleIsolateActiveNode();
        }

        if (model && changedIndex.isValid()) {
            const QModelIndex currentIndex(changedIndex);
            Q_EMIT dataChanged(currentIndex.siblingAtColumn(0),
                               currentIndex.siblingAtColumn(SetDataAccess::dummyColumns(this)));
        }
        return true;
    }

    return SetDataAccess::setRemainingData(this, index, value, role);
}
