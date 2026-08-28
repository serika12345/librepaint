/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISSTORAGEFILTERPROXYMODELSOURCE_P_H
#define KISSTORAGEFILTERPROXYMODELSOURCE_P_H

#include <QModelIndex>

#include <KisResourceStorage.h>

class QAbstractItemModel;

namespace KisStorageFilterProxyModelSource
{
KisResourceStorageSP storageForIndex(QAbstractItemModel *model,
                                     const QModelIndex &index);
}

#endif // KISSTORAGEFILTERPROXYMODELSOURCE_P_H
