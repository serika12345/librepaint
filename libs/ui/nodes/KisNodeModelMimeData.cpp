/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

QMimeData *KisNodeModel::mimeData(const QModelIndexList &indexes) const
{
    bool forceCopy = false;
    KisNodeList nodes;

    for (const QModelIndex &index : indexes) {
        if (index.column() != 0) {
            continue;
        }

        const KisNodeSP node = MimeDataAccess::nodeFromIndex(this, index);
        nodes.append(node);
        forceCopy |= !MimeDataAccess::isEditable(node, false);
    }

    return MimeDataAccess::createMimeData(nodes, MimeDataAccess::image(this), forceCopy);
}
