/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

bool KisNodeModel::canDropMimeData(const QMimeData *data,
                                   Qt::DropAction action,
                                   int row,
                                   int column,
                                   const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Dropping onto an item must remain allowed because returning false
        // conflicts with the item flags maintained by setDropEnabled().
        return true;
    }

    return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
}
