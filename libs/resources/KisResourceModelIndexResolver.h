/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISRESOURCEMODELINDEXRESOLVER_H
#define KISRESOURCEMODELINDEXRESOLVER_H

#include "kritaresources_export.h"

#include <QModelIndex>
#include <QString>

class KRITARESOURCES_EXPORT KisResourceModelIndexResolver
{
public:
    static QModelIndex resourceIndex(const QString &resourceType, int resourceId);
};

#endif // KISRESOURCEMODELINDEXRESOLVER_H
