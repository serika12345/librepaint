/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisStorageChooserWidgetModelSource_p.h"

#include "KisStorageModel.h"

namespace KisStorageChooserWidgetModelSource
{
QAbstractItemModel *model()
{
    return KisStorageModel::instance();
}
}
