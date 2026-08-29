/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

QModelIndex KisNodeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!StructureAccess::hasDummiesFacade(this) || !hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    QModelIndex itemIndex;

    KisNodeDummy *dummy = StructureAccess::dummyFromRow(this, row, parent);
    if (dummy) {
        itemIndex = StructureAccess::indexFromDummy(this, dummy);
    }

    if (itemIndex.isValid() && itemIndex.column() != column) {
        itemIndex = createIndex(itemIndex.row(), column, itemIndex.internalPointer());
    }

    return itemIndex;
}

int KisNodeModel::rowCount(const QModelIndex &parent) const
{
    if (!StructureAccess::hasDummiesFacade(this)) {
        return 0;
    }
    if (parent.column() > 0) {
        return 0;
    }
    return StructureAccess::rowCount(this, parent);
}

int KisNodeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }
    return 1 + StructureAccess::dummyColumns(this);
}

QModelIndex KisNodeModel::parent(const QModelIndex &index) const
{
    if (!StructureAccess::hasDummiesFacade(this) || !index.isValid()) {
        return QModelIndex();
    }

    KisNodeDummy *dummy = StructureAccess::dummyFromIndex(this, index);
    KisNodeDummy *parentDummy = StructureAccess::parentDummy(dummy);

    QModelIndex parentIndex;

    if (parentDummy) {
        parentIndex = StructureAccess::indexFromDummy(this, parentDummy);
    }

    return parentIndex;
}

QModelIndex KisNodeModel::sibling(int row, int column, const QModelIndex &index) const
{
    if (row == index.row()) {
        if (column == index.column()) {
            return index;
        }
        if (!StructureAccess::hasMatchingModel(this, index)) {
            return QModelIndex();
        }
        return createIndex(row, column, index.internalPointer());
    }
    return this->index(row, column, parent(index));
}
