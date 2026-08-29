/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

#include "kis_shared_ptr.h"

KisNodeSP KisNodeModel::nodeFromIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    KisNodeDummy *dummy = IndexMappingAccess::dummyFromIndex(this, index);
    return dummy ? IndexMappingAccess::nodeForDummy(dummy) : KisNodeSP();
}

QModelIndex KisNodeModel::indexFromNode(KisNodeSP node) const
{
    KisNodeDummy *dummy = IndexMappingAccess::dummyForNode(this, node);
    return dummy ? IndexMappingAccess::indexFromDummy(this, dummy) : QModelIndex();
}
