/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "floodfill/kis_fill_interval_map.h"

#include <QTest>

#include <algorithm>
#include <memory>

namespace
{

bool safeAssertTriggered = false;

QStack<KisFillInterval> sortedIntervals(QStack<KisFillInterval> intervals)
{
    std::sort(intervals.begin(), intervals.end(), [](const KisFillInterval &lhs, const KisFillInterval &rhs) {
        if (lhs.row != rhs.row) {
            return lhs.row < rhs.row;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.end < rhs.end;
    });
    return intervals;
}

void compareInterval(const KisFillInterval &actual, int start, int end, int row)
{
    QCOMPARE(actual.start, start);
    QCOMPARE(actual.end, end);
    QCOMPARE(actual.row, row);
}

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    safeAssertTriggered = true;
}

class KisFillIntervalMapContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionInsertionAndFetchPreserveIntervals();
    void rowCorrectionChangesFetchedCopiesOnly();
    void cropSplitsAnIntervalAroundTheIntersection();
    void cropTrimsStoredAndIncomingEdges();
    void clearRemovesEveryStoredInterval();
};

void KisFillIntervalMapContractTest::constructionInsertionAndFetchPreserveIntervals()
{
    safeAssertTriggered = false;

    auto map = std::make_unique<KisFillIntervalMap>();
    QVERIFY(map->fetchAllIntervals().isEmpty());

    map->insertInterval(KisFillInterval(5, 8, 2));
    map->insertInterval(KisFillInterval(-4, -1, -3));
    map->insertInterval(KisFillInterval(20, 22, 2));

    const auto intervals = sortedIntervals(map->fetchAllIntervals());
    QCOMPARE(intervals.size(), 3);
    compareInterval(intervals.at(0), -4, -1, -3);
    compareInterval(intervals.at(1), 5, 8, 2);
    compareInterval(intervals.at(2), 20, 22, 2);

    map.reset();
    QVERIFY(!map);
    QVERIFY(!safeAssertTriggered);
}

void KisFillIntervalMapContractTest::rowCorrectionChangesFetchedCopiesOnly()
{
    safeAssertTriggered = false;

    KisFillIntervalMap map;
    map.insertInterval(KisFillInterval(3, 7, -2));
    map.insertInterval(KisFillInterval(10, 12, 4));

    const auto corrected = sortedIntervals(map.fetchAllIntervals(5));
    QCOMPARE(corrected.size(), 2);
    compareInterval(corrected.at(0), 3, 7, 3);
    compareInterval(corrected.at(1), 10, 12, 9);

    const auto unchanged = sortedIntervals(map.fetchAllIntervals());
    QCOMPARE(unchanged.size(), 2);
    compareInterval(unchanged.at(0), 3, 7, -2);
    compareInterval(unchanged.at(1), 10, 12, 4);
    QVERIFY(!safeAssertTriggered);
}

void KisFillIntervalMapContractTest::cropSplitsAnIntervalAroundTheIntersection()
{
    safeAssertTriggered = false;

    KisFillIntervalMap map;
    map.insertInterval(KisFillInterval(0, 10, 6));

    KisFillInterval incoming(4, 6, 6);
    map.cropInterval(&incoming);

    QVERIFY(!incoming.isValid());
    QCOMPARE(incoming.start, 11);
    QCOMPARE(incoming.end, 6);

    const auto intervals = sortedIntervals(map.fetchAllIntervals());
    QCOMPARE(intervals.size(), 2);
    compareInterval(intervals.at(0), 0, 3, 6);
    compareInterval(intervals.at(1), 7, 10, 6);
    QVERIFY(!safeAssertTriggered);
}

void KisFillIntervalMapContractTest::cropTrimsStoredAndIncomingEdges()
{
    safeAssertTriggered = false;

    KisFillIntervalMap leftOverlap;
    leftOverlap.insertInterval(KisFillInterval(0, 10, 1));
    KisFillInterval incomingFromRight(8, 15, 1);
    leftOverlap.cropInterval(&incomingFromRight);

    compareInterval(incomingFromRight, 11, 15, 1);
    const auto leftIntervals = sortedIntervals(leftOverlap.fetchAllIntervals());
    QCOMPARE(leftIntervals.size(), 1);
    compareInterval(leftIntervals.at(0), 0, 7, 1);

    KisFillIntervalMap rightOverlap;
    rightOverlap.insertInterval(KisFillInterval(10, 20, 3));
    KisFillInterval incomingFromLeft(0, 12, 3);
    rightOverlap.cropInterval(&incomingFromLeft);

    compareInterval(incomingFromLeft, 0, 9, 3);
    const auto rightIntervals = sortedIntervals(rightOverlap.fetchAllIntervals());
    QCOMPARE(rightIntervals.size(), 1);
    compareInterval(rightIntervals.at(0), 13, 20, 3);
    QVERIFY(!safeAssertTriggered);
}

void KisFillIntervalMapContractTest::clearRemovesEveryStoredInterval()
{
    safeAssertTriggered = false;

    KisFillIntervalMap map;
    map.insertInterval(KisFillInterval(0, 2, 0));
    map.insertInterval(KisFillInterval(5, 9, 4));
    map.clear();

    QVERIFY(map.fetchAllIntervals().isEmpty());
    map.clear();
    QVERIFY(map.fetchAllIntervals().isEmpty());
    QVERIFY(!safeAssertTriggered);
}

QTEST_GUILESS_MAIN(KisFillIntervalMapContractTest)

#include "KisFillIntervalMapContractTest.moc"
