/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_grid_config.h>

#include <QTest>

#include <utility>

namespace
{
} // namespace

class KisGridConfigValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typesEnumsAndTrigoCacheValuesRemainStable();
};

void KisGridConfigValueContractTest::typesEnumsAndTrigoCacheValuesRemainStable()
{

    QCOMPARE(int(KisGridConfig::GRID_RECTANGULAR), 0);
    QCOMPARE(int(KisGridConfig::GRID_ISOMETRIC_LEGACY), 1);
    QCOMPARE(int(KisGridConfig::GRID_ISOMETRIC), 2);

    QCOMPARE(int(KisGridConfig::LINE_SOLID), 0);
    QCOMPARE(int(KisGridConfig::LINE_DASHED), 1);
    QCOMPARE(int(KisGridConfig::LINE_DOTTED), 2);
    QCOMPARE(int(KisGridConfig::LINE_NONE), 3);

    KisGridConfig::TrigoCache cache{
        1.25,
        2.5,
        3.75,
        4.25,
        5.5,
        6.75,
        8.0,
    };


    QCOMPARE(cache.correctedAngleRightCellSize, 1.25);
    QCOMPARE(cache.correctedAngleRightOffsetX, 2.5);
    QCOMPARE(cache.tanAngleRight, 3.75);
    QCOMPARE(cache.correctedAngleLeftCellSize, 4.25);
    QCOMPARE(cache.correctedAngleLeftOffsetX, 5.5);
    QCOMPARE(cache.tanAngleLeft, 6.75);
    QCOMPARE(cache.verticalSpace, 8.0);

    KisGridConfig::TrigoCache copy = cache;
    copy.correctedAngleRightCellSize = 9.0;
    copy.correctedAngleLeftOffsetX = 10.0;
    copy.verticalSpace = 11.0;

    QCOMPARE(cache.correctedAngleRightCellSize, 1.25);
    QCOMPARE(cache.correctedAngleLeftOffsetX, 5.5);
    QCOMPARE(cache.verticalSpace, 8.0);
    QCOMPARE(copy.correctedAngleRightCellSize, 9.0);
    QCOMPARE(copy.correctedAngleLeftOffsetX, 10.0);
    QCOMPARE(copy.verticalSpace, 11.0);
}

QTEST_GUILESS_MAIN(KisGridConfigValueContractTest)

#include "KisGridConfigValueContractTest.moc"
