/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_safe_transform.h"

#include <QPolygonF>
#include <QTest>
#include <QTransform>

namespace
{

constexpr qreal tolerance = 1e-6;

void comparePoint(const QPointF &actual, const QPointF &expected, qreal allowedError = tolerance)
{
    QVERIFY2(qAbs(actual.x() - expected.x()) <= allowedError, "x coordinate differs");
    QVERIFY2(qAbs(actual.y() - expected.y()) <= allowedError, "y coordinate differs");
}

void compareRect(const QRectF &actual, const QRectF &expected, qreal allowedError = tolerance)
{
    QVERIFY2(qAbs(actual.left() - expected.left()) <= allowedError, "left edge differs");
    QVERIFY2(qAbs(actual.top() - expected.top()) <= allowedError, "top edge differs");
    QVERIFY2(qAbs(actual.right() - expected.right()) <= allowedError, "right edge differs");
    QVERIFY2(qAbs(actual.bottom() - expected.bottom()) <= allowedError, "bottom edge differs");
}

void comparePolygon(const QPolygonF &actual, const QPolygonF &expected, qreal allowedError = tolerance)
{
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype i = 0; i < actual.size(); ++i) {
        comparePoint(actual.at(i), expected.at(i), allowedError);
    }
}

void verifyIntegerRectWithinOnePixel(const QRect &actual, const QRect &expected)
{
    QVERIFY(qAbs(actual.left() - expected.left()) <= 1);
    QVERIFY(qAbs(actual.top() - expected.top()) <= 1);
    QVERIFY(qAbs(actual.right() - expected.right()) <= 1);
    QVERIFY(qAbs(actual.bottom() - expected.bottom()) <= 1);
}

void verifyFiniteAndInside(const QPolygonF &polygon, const QRectF &bounds)
{
    QVERIFY(!polygon.isEmpty());
    QVERIFY(polygon.isClosed());

    const QRectF tolerantBounds = bounds.adjusted(-tolerance, -tolerance, tolerance, tolerance);
    for (const QPointF &point : polygon) {
        QVERIFY(qIsFinite(point.x()));
        QVERIFY(qIsFinite(point.y()));
        QVERIFY(tolerantBounds.contains(point));
    }
}

} // namespace

void kis_assert_recoverable(const char *, const char *, int)
{
}

class KisSafeTransformContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identityMappingReturnsValuesThatOutliveTransform();
    void affineMappingAppliesForwardAndInverseGeometry();
    void projectiveMappingClipsBothCoordinateSpaces();
    void singularAffineUsesIdentityFallbackForBackwardMapping();
};

void KisSafeTransformContractTest::identityMappingReturnsValuesThatOutliveTransform()
{
    const QRect bounds(-20, -10, 100, 80);
    const QRect interest(5, 6, 20, 30);
    const QPolygonF polygon{QPointF(1.5, 2.5), QPointF(7.0, -3.0), QPointF(9.0, 8.0)};
    const QRectF floatingRect(-2.5, 1.25, 4.0, 2.5);
    const QRect integerRect(-2, 1, 4, 3);

    QPolygonF forwardPolygon;
    QPolygonF backwardPolygon;
    QPolygonF sourceClip;
    QPolygonF destinationClip;
    QRectF forwardFloatingRect;
    QRectF backwardFloatingRect;
    QRect forwardIntegerRect;
    QRect backwardIntegerRect;

    {
        KisSafeTransform transform(QTransform(), bounds, interest);
        forwardPolygon = transform.mapForward(polygon);
        backwardPolygon = transform.mapBackward(polygon);
        sourceClip = transform.srcClipPolygon();
        destinationClip = transform.dstClipPolygon();
        forwardFloatingRect = transform.mapRectForward(floatingRect);
        backwardFloatingRect = transform.mapRectBackward(floatingRect);
        forwardIntegerRect = transform.mapRectForward(integerRect);
        backwardIntegerRect = transform.mapRectBackward(integerRect);
    }

    comparePolygon(forwardPolygon, polygon);
    comparePolygon(backwardPolygon, polygon);
    QVERIFY(sourceClip.isEmpty());
    QVERIFY(destinationClip.isEmpty());
    compareRect(forwardFloatingRect, floatingRect);
    compareRect(backwardFloatingRect, floatingRect);
    QCOMPARE(forwardIntegerRect, integerRect);
    QCOMPARE(backwardIntegerRect, integerRect);
}

void KisSafeTransformContractTest::affineMappingAppliesForwardAndInverseGeometry()
{
    const QTransform affine(2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 13.0, -7.0, 1.0);
    KisSafeTransform transform(affine, QRect(-100, -100, 400, 400), QRect(-10, -10, 20, 20));

    const QPolygonF source{QPointF(1.0, 2.0), QPointF(4.0, 2.0), QPointF(4.0, 5.0)};
    const QPolygonF expected{QPointF(15.0, -1.0), QPointF(21.0, -1.0), QPointF(21.0, 8.0)};
    const QPolygonF mapped = transform.mapForward(source);

    comparePolygon(mapped, expected);
    comparePolygon(transform.mapBackward(mapped), source);
    compareRect(transform.mapRectForward(QRectF(-2.5, 1.25, 4.0, 2.5)), QRectF(8.0, -3.25, 8.0, 7.5));
    compareRect(transform.mapRectBackward(QRectF(8.0, -3.25, 8.0, 7.5)), QRectF(-2.5, 1.25, 4.0, 2.5));
    verifyIntegerRectWithinOnePixel(transform.mapRectForward(QRect(-2, 1, 4, 3)), QRect(9, -4, 8, 9));
    verifyIntegerRectWithinOnePixel(transform.mapRectBackward(QRect(9, -4, 8, 9)), QRect(-2, 1, 4, 3));
    QVERIFY(transform.srcClipPolygon().isEmpty());
    QVERIFY(transform.dstClipPolygon().isEmpty());
}

void KisSafeTransformContractTest::projectiveMappingClipsBothCoordinateSpaces()
{
    const QTransform
        projective(-0.177454, -0.805953, -0.00213713, -1.9295, -0.371835, -0.00290463, 3075.05, 2252.32, 7.62371);
    const QRect bounds(0, 0, 2048, 2048);
    const QRect interest(0, 1024, 512, 512);
    KisSafeTransform transform(projective, bounds, interest);

    const QPolygonF sourcePolygon{QRectF(interest)};
    const QPolygonF forwardPolygon = transform.mapForward(sourcePolygon);
    const QRectF forwardBounds = transform.mapRectForward(QRectF(interest));
    const QPolygonF backwardPolygon = transform.mapBackward(forwardPolygon);
    const QRectF backwardBounds = backwardPolygon.boundingRect();
    const QPolygonF sourceClip = transform.srcClipPolygon();
    const QPolygonF destinationClip = transform.dstClipPolygon();

    verifyFiniteAndInside(sourceClip, QRectF(bounds));
    verifyFiniteAndInside(destinationClip, QRectF(bounds));
    QVERIFY(sourceClip.containsPoint(QRectF(interest).center(), Qt::OddEvenFill));
    QVERIFY(destinationClip.containsPoint(forwardBounds.center(), Qt::OddEvenFill));
    verifyFiniteAndInside(forwardPolygon, QRectF(bounds));
    compareRect(forwardBounds, QRectF(10.0, 403.0, 274.0, 210.0), 1.5);
    compareRect(backwardBounds, QRectF(interest), 1.5);

    const QRect integerForward = transform.mapRectForward(interest);
    QCOMPARE(integerForward, forwardBounds.toAlignedRect());
    QCOMPARE(transform.mapRectBackward(integerForward),
             transform.mapRectBackward(QRectF(integerForward)).toAlignedRect());
}

void KisSafeTransformContractTest::singularAffineUsesIdentityFallbackForBackwardMapping()
{
    const QTransform singular(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    KisSafeTransform transform(singular, QRect(-10, -10, 20, 20), QRect(-5, -5, 10, 10));
    const QPolygonF source{QPointF(2.0, 3.0), QPointF(4.0, 7.0)};

    const QPolygonF collapsed = transform.mapForward(source);
    const QPolygonF expectedCollapsed{QPointF(0.0, 3.0), QPointF(0.0, 7.0)};
    comparePolygon(collapsed, expectedCollapsed);
    comparePolygon(transform.mapBackward(collapsed), collapsed);
}

QTEST_GUILESS_MAIN(KisSafeTransformContractTest)

#include "KisSafeTransformContractTest.moc"
