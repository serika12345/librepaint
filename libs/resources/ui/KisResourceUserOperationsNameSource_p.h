/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEUSEROPERATIONSNAMESOURCE_P_H
#define KISRESOURCEUSEROPERATIONSNAMESOURCE_P_H

#include <QString>
#include <QVector>

class KisResourceModel;

namespace KisResourceUserOperationsNameSource
{
QVector<int> resourceIdsForName(KisResourceModel *resourceModel,
                                const QString &resourceName);
}

#endif // KISRESOURCEUSEROPERATIONSNAMESOURCE_P_H
