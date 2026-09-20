/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

KisNodeModel::KisNodeModel(QObject *parent, int clonedColumns)
    : QAbstractItemModel(parent)
    , m_d(static_cast<Private *>(LifecycleAccess::createPrivateState(qMax(0, clonedColumns))))
{
    LifecycleAccess::connectUpdateCompressor(this, m_d);
    LifecycleAccess::connectThumbnailCache(this, m_d);
}

KisNodeModel::~KisNodeModel()
{
    LifecycleAccess::destroyPrivateState(m_d);
}
