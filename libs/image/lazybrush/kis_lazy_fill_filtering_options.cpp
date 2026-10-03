/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_lazy_fill_tools.h"
#include <qnumeric.h>
#include <QtGlobal>

namespace KisLazyFillTools
{

FilteringOptions::FilteringOptions(bool _useEdgeDetection,
                                   qreal _edgeDetectionSize,
                                   qreal _fuzzyRadius,
                                   qreal _cleanUpAmount)
    : useEdgeDetection(_useEdgeDetection)
    , edgeDetectionSize(_edgeDetectionSize)
    , fuzzyRadius(_fuzzyRadius)
    , cleanUpAmount(_cleanUpAmount)
{
}

bool operator==(const FilteringOptions &t1, const FilteringOptions &t2)
{
    return t1.useEdgeDetection == t2.useEdgeDetection && qFuzzyCompare(t1.edgeDetectionSize, t2.edgeDetectionSize)
        && qFuzzyCompare(t1.fuzzyRadius, t2.fuzzyRadius) && qFuzzyCompare(t1.cleanUpAmount, t2.cleanUpAmount);
}

} // namespace KisLazyFillTools
