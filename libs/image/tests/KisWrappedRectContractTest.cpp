/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_wrapped_rect.h"

#include <QTest>

#include <type_traits>

class KisWrappedRectContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void coordinatesNormalizeByConfiguredAxes();
    void clippingAndContainmentRespectConfiguredAxes();
    void normalizationOriginsDescribeWrappedCopies();
    void constructorPreservesSplitQuadrantsAndSourceRects();
    void multiplicationTilesOnlyInsideLimit();
};

void KisWrappedRectContractTest::coordinatesNormalizeByConfiguredAxes()
{
    const QRect wrapRect(0, 0, 100, 80);

    QCOMPARE(KisWrappedRect::xToWrappedX(-10, wrapRect, WRAPAROUND_BOTH), 90);
    QCOMPARE(KisWrappedRect::xToWrappedX(110, wrapRect, WRAPAROUND_BOTH), 10);
    QCOMPARE(KisWrappedRect::xToWrappedX(-10, wrapRect, WRAPAROUND_VERTICAL), -10);

    QCOMPARE(KisWrappedRect::yToWrappedY(-5, wrapRect, WRAPAROUND_BOTH), 75);
    QCOMPARE(KisWrappedRect::yToWrappedY(85, wrapRect, WRAPAROUND_BOTH), 5);
    QCOMPARE(KisWrappedRect::yToWrappedY(85, wrapRect, WRAPAROUND_HORIZONTAL), 85);

    const QPoint point(-10, 85);
    QCOMPARE(KisWrappedRect::ptToWrappedPt(point, wrapRect, WRAPAROUND_BOTH), QPoint(90, 5));
    QCOMPARE(KisWrappedRect::ptToWrappedPt(point, wrapRect, WRAPAROUND_HORIZONTAL), QPoint(90, 85));
    QCOMPARE(KisWrappedRect::ptToWrappedPt(point, wrapRect, WRAPAROUND_VERTICAL), QPoint(-10, 5));
}

void KisWrappedRectContractTest::clippingAndContainmentRespectConfiguredAxes()
{
    const QRect wrapRect(0, 0, 100, 80);
    const QRect outside(-10, -20, 130, 120);

    QCOMPARE(KisWrappedRect::clipToWrapRect(outside, wrapRect, WRAPAROUND_BOTH), wrapRect);
    QCOMPARE(KisWrappedRect::clipToWrapRect(outside, wrapRect, WRAPAROUND_HORIZONTAL), QRect(0, -20, 100, 120));
    QCOMPARE(KisWrappedRect::clipToWrapRect(outside, wrapRect, WRAPAROUND_VERTICAL), QRect(-10, 0, 130, 80));

    const QRect inside(10, 10, 20, 20);
    const QRect outsideX(-5, 10, 20, 20);
    const QRect outsideY(10, -5, 20, 20);
    QVERIFY(KisWrappedRect::wrapRectContains(inside, wrapRect, WRAPAROUND_BOTH));
    QVERIFY(!KisWrappedRect::wrapRectContains(outsideX, wrapRect, WRAPAROUND_HORIZONTAL));
    QVERIFY(KisWrappedRect::wrapRectContains(outsideX, wrapRect, WRAPAROUND_VERTICAL));
    QVERIFY(KisWrappedRect::wrapRectContains(outsideY, wrapRect, WRAPAROUND_HORIZONTAL));
    QVERIFY(!KisWrappedRect::wrapRectContains(outsideY, wrapRect, WRAPAROUND_VERTICAL));
    QVERIFY(!KisWrappedRect::wrapRectContains(outsideY, wrapRect, WRAPAROUND_BOTH));

    QVERIFY(!KisWrappedRect::wrapRectContains(QPoint(-1, 10), wrapRect, WRAPAROUND_HORIZONTAL));
    QVERIFY(KisWrappedRect::wrapRectContains(QPoint(-1, 10), wrapRect, WRAPAROUND_VERTICAL));
    QVERIFY(KisWrappedRect::wrapRectContains(QPoint(10, -1), wrapRect, WRAPAROUND_HORIZONTAL));
    QVERIFY(!KisWrappedRect::wrapRectContains(QPoint(10, -1), wrapRect, WRAPAROUND_VERTICAL));
    QVERIFY(!KisWrappedRect::wrapRectContains(QPoint(10, -1), wrapRect, WRAPAROUND_BOTH));
}

void KisWrappedRectContractTest::normalizationOriginsDescribeWrappedCopies()
{
    const QRect wrapRect(0, 0, 100, 80);
    const QRect inside(10, 10, 20, 20);
    const QRect crossingBottomRight(90, 70, 20, 20);

    const QVector<QPoint> insideOrigins{QPoint(10, 10)};
    QCOMPARE(KisWrappedRect::normalizationOriginsForRect(inside, wrapRect, WRAPAROUND_BOTH), insideOrigins);

    const QVector<QPoint> horizontalOrigins{QPoint(90, 70), QPoint(-10, 70)};
    QCOMPARE(KisWrappedRect::normalizationOriginsForRect(crossingBottomRight, wrapRect, WRAPAROUND_HORIZONTAL),
             horizontalOrigins);

    const QVector<QPoint> verticalOrigins{QPoint(90, 70), QPoint(90, -10)};
    QCOMPARE(KisWrappedRect::normalizationOriginsForRect(crossingBottomRight, wrapRect, WRAPAROUND_VERTICAL),
             verticalOrigins);

    const QVector<QPoint> bothOrigins{QPoint(90, 70), QPoint(-10, 70), QPoint(90, -10), QPoint(-10, -10)};
    QCOMPARE(KisWrappedRect::normalizationOriginsForRect(crossingBottomRight, wrapRect, WRAPAROUND_BOTH), bothOrigins);
}

void KisWrappedRectContractTest::constructorPreservesSplitQuadrantsAndSourceRects()
{
    static_assert(std::is_base_of_v<QVector<QRect>, KisWrappedRect>);
    static_assert(KisWrappedRect::TOPLEFT == 0);
    static_assert(KisWrappedRect::TOPRIGHT == 1);
    static_assert(KisWrappedRect::BOTTOMLEFT == 2);
    static_assert(KisWrappedRect::BOTTOMRIGHT == 3);

    const QRect wrapRect(0, 0, 100, 80);
    const QRect inside(10, 15, 20, 25);
    const KisWrappedRect unchanged(inside, wrapRect, WRAPAROUND_BOTH);
    QCOMPARE(unchanged.size(), 1);
    QVERIFY(!unchanged.isSplit());
    QCOMPARE(unchanged.topLeft(), inside);
    QCOMPARE(unchanged.wrapRect(), wrapRect);
    QCOMPARE(unchanged.originalRect(), inside);

    const QRect crossingBottomRight(90, 70, 20, 20);
    const KisWrappedRect split(crossingBottomRight, wrapRect, WRAPAROUND_BOTH);
    QCOMPARE(split.size(), 4);
    QVERIFY(split.isSplit());
    QCOMPARE(split.topLeft(), QRect(90, 70, 10, 10));
    QCOMPARE(split.topRight(), QRect(0, 70, 10, 10));
    QCOMPARE(split.bottomLeft(), QRect(90, 0, 10, 10));
    QCOMPARE(split.bottomRight(), QRect(0, 0, 10, 10));
    QCOMPARE(split.wrapRect(), wrapRect);
    QCOMPARE(split.originalRect(), crossingBottomRight);

    const KisWrappedRect horizontalSplit(QRect(90, 10, 20, 20), wrapRect, WRAPAROUND_HORIZONTAL);
    QCOMPARE(horizontalSplit.size(), 4);
    QCOMPARE(horizontalSplit.topLeft(), QRect(90, 10, 10, 20));
    QCOMPARE(horizontalSplit.topRight(), QRect(0, 10, 10, 20));
    QVERIFY(horizontalSplit.bottomLeft().isEmpty());
    QVERIFY(horizontalSplit.bottomRight().isEmpty());
}

void KisWrappedRectContractTest::multiplicationTilesOnlyInsideLimit()
{
    const QRect wrapRect(0, 0, 100, 80);
    const QRect source(10, 10, 20, 15);
    const QRect limit(-85, -65, 270, 161);

    const QVector<QRect> bothExpected{QRect(-85, -65, 15, 10),
                                      QRect(10, -65, 20, 10),
                                      QRect(110, -65, 20, 10),
                                      QRect(-85, 10, 15, 15),
                                      QRect(10, 10, 20, 15),
                                      QRect(110, 10, 20, 15),
                                      QRect(-85, 90, 15, 6),
                                      QRect(10, 90, 20, 6),
                                      QRect(110, 90, 20, 6)};
    QCOMPARE(KisWrappedRect::multiplyWrappedRect(source, wrapRect, limit, WRAPAROUND_BOTH), bothExpected);

    const QVector<QRect> horizontalExpected{QRect(-85, 10, 15, 15), QRect(10, 10, 20, 15), QRect(110, 10, 20, 15)};
    QCOMPARE(KisWrappedRect::multiplyWrappedRect(source, wrapRect, limit, WRAPAROUND_HORIZONTAL), horizontalExpected);

    const QVector<QRect> verticalExpected{QRect(10, -65, 20, 10), QRect(10, 10, 20, 15), QRect(10, 90, 20, 6)};
    QCOMPARE(KisWrappedRect::multiplyWrappedRect(source, wrapRect, limit, WRAPAROUND_VERTICAL), verticalExpected);

    const QVector<QRect> results = KisWrappedRect::multiplyWrappedRect(source, wrapRect, limit, WRAPAROUND_BOTH);
    for (const QRect &rect : results) {
        QVERIFY(limit.contains(rect));
    }
}

QTEST_GUILESS_MAIN(KisWrappedRectContractTest)

#include "KisWrappedRectContractTest.moc"
