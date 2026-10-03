/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceStorage.h"
#include "KisStorageFilterProxyModelSource_p.h"

#include "KisStorageModel.h"
#include <qabstractitemmodel.h>

namespace KisStorageFilterProxyModelSource
{
KisResourceStorageSP storageForIndex(QAbstractItemModel *model,
                                     const QModelIndex &index)
{
    auto *storageModel = dynamic_cast<KisStorageModel *>(model);
    return storageModel ? storageModel->storageForIndex(index) : KisResourceStorageSP();
}
}
