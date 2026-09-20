/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

Qt::ItemFlags KisNodeModel::flags(const QModelIndex &index) const
{
    if (!ItemFlagsAccess::hasDummiesFacade(this) || !index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if (index.column() == 0) {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsSelectable;
        if (ItemFlagsAccess::isDropEnabled(this, index.internalId())) {
            flags |= Qt::ItemIsDropEnabled;
        }
    }

    return flags;
}
