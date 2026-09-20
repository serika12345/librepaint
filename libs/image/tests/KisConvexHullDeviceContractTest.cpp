/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "kis_convex_hull.h"
#include "kis_convex_hull_p.h"

void kisSharedPtrAddReference(KisPaintDevice *)
{
}

bool kisSharedPtrRelease(KisPaintDevice *)
{
    return false;
}

namespace
{

KisConvexHullPrivate::BoundaryMode requestedMode;
bool receivedNullDevice = false;

} // namespace

namespace KisConvexHullPrivate
{

QVector<QPoint> retrieveBoundaryPoints(const KisPaintDeviceSP &device, BoundaryMode mode)
{
    receivedNullDevice = !device;
    requestedMode = mode;

    if (mode == BoundaryMode::SelectionLike) {
        return {
            QPoint(6, 7),
            QPoint(6, 8),
            QPoint(8, 7),
            QPoint(8, 8),
            QPoint(6, 9),
            QPoint(6, 10),
            QPoint(8, 9),
            QPoint(8, 10),
        };
    }

    return {
        QPoint(2, 3),
        QPoint(2, 4),
        QPoint(5, 3),
        QPoint(5, 4),
        QPoint(2, 4),
        QPoint(2, 5),
        QPoint(5, 4),
        QPoint(5, 5),
    };
}

} // namespace KisConvexHullPrivate

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

class KisConvexHullDeviceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void opaquePixelsProducePixelEdgeBoundary();
    void selectionLikeHullUsesSelectionBoundaryMode();
};

void KisConvexHullDeviceContractTest::opaquePixelsProducePixelEdgeBoundary()
{
    const QPolygon hull = KisConvexHull::findConvexHull(KisPaintDeviceSP());

    QVERIFY(receivedNullDevice);
    QCOMPARE(requestedMode, KisConvexHullPrivate::BoundaryMode::OpaquePixels);
    compareClosedHull(hull, {QPoint(2, 3), QPoint(5, 3), QPoint(5, 5), QPoint(2, 5)});
}

void KisConvexHullDeviceContractTest::selectionLikeHullUsesSelectionBoundaryMode()
{
    const QPolygon hull = KisConvexHull::findConvexHullSelectionLike(KisPaintDeviceSP());

    QVERIFY(receivedNullDevice);
    QCOMPARE(requestedMode, KisConvexHullPrivate::BoundaryMode::SelectionLike);
    compareClosedHull(hull, {QPoint(6, 7), QPoint(8, 7), QPoint(8, 10), QPoint(6, 10)});
}

QTEST_GUILESS_MAIN(KisConvexHullDeviceContractTest)

#include "KisConvexHullDeviceContractTest.moc"
