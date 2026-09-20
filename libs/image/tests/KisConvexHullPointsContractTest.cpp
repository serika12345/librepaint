/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "kis_convex_hull.h"

namespace
{

void compareClosedHull(const QPolygon &actual, const QVector<QPoint> &expectedCorners)
{
    QCOMPARE(actual.size(), expectedCorners.size() + 1);
    QCOMPARE(actual.first(), actual.last());

    for (const QPoint &corner : expectedCorners) {
        QVERIFY2(actual.contains(corner),
                 qPrintable(QStringLiteral("missing corner (%1, %2)").arg(corner.x()).arg(corner.y())));
    }

    for (int index = 0; index < actual.size() - 1; ++index) {
        QVERIFY2(
            expectedCorners.contains(actual[index]),
            qPrintable(QStringLiteral("unexpected hull point (%1, %2)").arg(actual[index].x()).arg(actual[index].y())));
    }
}

} // namespace

class KisConvexHullPointsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unorderedPointsProduceClosedOuterBoundary();
};

void KisConvexHullPointsContractTest::unorderedPointsProduceClosedOuterBoundary()
{
    const QVector<QPoint> points{
        QPoint(4, 3),
        QPoint(0, 0),
        QPoint(2, 1),
        QPoint(4, 0),
        QPoint(1, 2),
        QPoint(0, 3),
        QPoint(4, 3),
        QPoint(2, 2),
    };

    const QPolygon hull = KisConvexHull::findConvexHull(points);

    compareClosedHull(hull, {QPoint(0, 0), QPoint(4, 0), QPoint(4, 3), QPoint(0, 3)});
    QVERIFY(!hull.contains(QPoint(2, 1)));
    QVERIFY(!hull.contains(QPoint(2, 2)));
}

QTEST_GUILESS_MAIN(KisConvexHullPointsContractTest)

#include "KisConvexHullPointsContractTest.moc"
