/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "floodfill/kis_gap_map.h"

#include <QTest>

#include <type_traits>

class KisGapMapSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gapMapSchemaRemainsStable();
};

void KisGapMapSchemaContractTest::gapMapSchemaRemainsStable()
{
    using FillOpacityFunc = std::function<bool(KisPaintDevice *, const QRect &)>;
    using Distance = quint16 (KisGapMap::*)(int, int);
    using GapSize = int (KisGapMap::*)() const;
    using ElapsedMillis = quint64 (KisGapMap::*)() const;
    using TileRawData = quint8 *(KisTileOptimizedAccessor::*)(int, int);
    using RawData = quint8 *(KisTileOptimizedAccessor::*)(int, int);

    static_assert(std::is_base_of_v<KisShared, KisGapMap>);
    static_assert(std::is_same_v<KisGapMap::FillOpacityFunc, FillOpacityFunc>);
    static_assert(std::is_constructible_v<KisGapMap, int, const QRect &, const FillOpacityFunc &>);
    static_assert(KisGapMap::DISTANCE_INFINITE == UINT16_MAX);
    static_assert(std::is_same_v<decltype(&KisGapMap::distance), Distance>);
    static_assert(std::is_same_v<decltype(&KisGapMap::gapSize), GapSize>);
    static_assert(std::is_same_v<decltype(&KisGapMap::opacityElapsedMillis), ElapsedMillis>);
    static_assert(std::is_same_v<decltype(&KisGapMap::distanceElapsedMillis), ElapsedMillis>);
    static_assert(std::is_constructible_v<KisTileOptimizedAccessor, KisPaintDeviceSP &>);
    static_assert(std::is_same_v<decltype(&KisTileOptimizedAccessor::tileRawData), TileRawData>);
    static_assert(std::is_same_v<decltype(&KisTileOptimizedAccessor::rawData), RawData>);
}

QTEST_APPLESS_MAIN(KisGapMapSchemaContractTest)

#include "KisGapMapSchemaContractTest.moc"
