/*
 * SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 * SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisDabSpacing.h"
#include <QVector2D>

qreal KisDabSpacing::nextIsotropic(const QPointF &start, const QPointF &end, qreal spacing, qreal &accumulated)
{
    if (start == end) return -1;
    spacing = qMax(qreal(.5), spacing);
    const qreal length = QVector2D(end - start).length();
    const qreal remaining = spacing - accumulated;
    if (remaining <= 0) {
        accumulated = 0;
        return 0;
    }
    if (remaining <= length) {
        accumulated = 0;
        return remaining / length;
    }
    accumulated += length;
    return -1;
}
