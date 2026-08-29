/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

bool KisNodeModel::dropMimeData(const QMimeData *data,
                                Qt::DropAction action,
                                int row,
                                int column,
                                const QModelIndex &parent)
{
    Q_UNUSED(column);

    const bool copyNode = action == Qt::CopyAction;
    KisNodeDummy *const parentDummy = DropMimeDataAccess::parentDummy(this, parent);
    KisNodeDummy *aboveThisDummy = nullptr;

    if (row == -1) {
        aboveThisDummy = parent.isValid() ? DropMimeDataAccess::lastChild(parentDummy) : nullptr;
    } else if (row < DropMimeDataAccess::rowCount(this, parent)) {
        aboveThisDummy = DropMimeDataAccess::dummyFromRow(this, row, parent);
    }

    return DropMimeDataAccess::insertMimeLayers(data,
                                                DropMimeDataAccess::context(this),
                                                parentDummy,
                                                aboveThisDummy,
                                                copyNode);
}
