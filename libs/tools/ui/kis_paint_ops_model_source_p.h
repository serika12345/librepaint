/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINT_OPS_MODEL_SOURCE_P_H
#define KIS_PAINT_OPS_MODEL_SOURCE_P_H

#include <QIcon>
#include <QString>

class KisPaintOpFactory;

namespace KisPaintOpsModelSource
{

struct PaintOpState
{
    QString id;
    QString name;
    QString category;
    QIcon icon;
    qint32 priority {0};
};

PaintOpState paintOpState(KisPaintOpFactory *factory);
QString stableCategory();

}

#endif
