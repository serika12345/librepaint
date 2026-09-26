/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisQuickPinchRecognizer.h>

#include <QTest>

class KisQuickPinchRecognizerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fitAnimationUsesTheSelectedOrientation();
    void fitAnimationUsesTheVisibleViewportBelowAnOverlay();
    void fitAnimationInterpolatesOneContinuousTransform();
    void fitOrientationFollowsNearestQuarterTurn_data();
    void fitOrientationFollowsNearestQuarterTurn();
    void fastInwardPinchFitsWhenTheFingersLift();
    void slowPinchRemainsAtTheChosenZoom();
    void smallPinchRemainsAtTheChosenZoom();
    void outwardPinchRemainsAtTheChosenZoom();
    void rotationGestureDoesNotFitTheCanvas();
    void reversedPinchRemainsAtTheChosenZoom();
    void canceledGestureCannotFitTheCanvas();
};

void KisQuickPinchRecognizerContractTest::fitAnimationUsesTheSelectedOrientation()
{
    const qreal portraitZoom = KisQuickPinchTransform::fittedZoom(
        2.0, QSizeF(600.0, 400.0), QSizeF(300.0, 500.0), 10.0, 90.0);
    const qreal landscapeZoom = KisQuickPinchTransform::fittedZoom(
        2.0, QSizeF(600.0, 400.0), QSizeF(300.0, 500.0), 10.0, 0.0);

    QVERIFY(qAbs(portraitZoom - 1.4) < 0.000001);
    QVERIFY(qAbs(landscapeZoom - (2.0 * 280.0 / 600.0)) < 0.000001);
}

void KisQuickPinchRecognizerContractTest::fitAnimationUsesTheVisibleViewportBelowAnOverlay()
{
    const KisQuickPinchFitTarget target = KisQuickPinchTransform::fittedTarget(
        1.0,
        QSizeF(1000.0, 1000.0),
        QRectF(0.0, 60.0, 1000.0, 740.0),
        20.0,
        0.0);

    QVERIFY(qAbs(target.zoom - 0.7) < 0.000001);
    QCOMPARE(target.viewCenter, QPointF(500.0, 430.0));
}

void KisQuickPinchRecognizerContractTest::fitAnimationInterpolatesOneContinuousTransform()
{
    const KisQuickPinchTransform transform(0.0,
                                           90.0,
                                           1.0,
                                           4.0,
                                           QPointF(0.0, 0.0),
                                           QPointF(100.0, 200.0));
    const KisQuickPinchTransformFrame frame = transform.frameAt(0.5);

    QCOMPARE(frame.rotation, 45.0);
    QCOMPARE(frame.zoom, 2.0);
    QCOMPARE(frame.viewCenter, QPointF(50.0, 100.0));
}

void KisQuickPinchRecognizerContractTest::fitOrientationFollowsNearestQuarterTurn_data()
{
    QTest::addColumn<qreal>("currentRotation");
    QTest::addColumn<qreal>("expectedRotation");

    QTest::addRow("landscape-before-clockwise-threshold") << 44.0 << 0.0;
    QTest::addRow("portrait-at-clockwise-threshold") << 45.0 << 90.0;
    QTest::addRow("portrait-before-inverted-threshold") << 134.0 << 90.0;
    QTest::addRow("inverted-at-clockwise-threshold") << 135.0 << 180.0;
    QTest::addRow("portrait-at-counterclockwise-threshold") << -45.0 << -90.0;
    QTest::addRow("wrapped-landscape") << 359.0 << 360.0;
}

void KisQuickPinchRecognizerContractTest::fitOrientationFollowsNearestQuarterTurn()
{
    QFETCH(qreal, currentRotation);
    QFETCH(qreal, expectedRotation);

    QCOMPARE(KisQuickPinchRecognizer::snappedCanvasRotation(currentRotation),
             expectedRotation);
}

void KisQuickPinchRecognizerContractTest::fastInwardPinchFitsWhenTheFingersLift()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(120.0, 0.0), 180);

    QVERIFY(recognizer.shouldFitOnRelease(200));
}

void KisQuickPinchRecognizerContractTest::slowPinchRemainsAtTheChosenZoom()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(120.0, 0.0), 351);

    QVERIFY(!recognizer.shouldFitOnRelease(351));
}

void KisQuickPinchRecognizerContractTest::smallPinchRemainsAtTheChosenZoom()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(150.0, 0.0), 150);

    QVERIFY(!recognizer.shouldFitOnRelease(170));
}

void KisQuickPinchRecognizerContractTest::outwardPinchRemainsAtTheChosenZoom()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(260.0, 0.0), 120);

    QVERIFY(!recognizer.shouldFitOnRelease(140));
}

void KisQuickPinchRecognizerContractTest::rotationGestureDoesNotFitTheCanvas()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(115.91, 31.06), 180);

    QVERIFY(!recognizer.shouldFitOnRelease(200));
}

void KisQuickPinchRecognizerContractTest::reversedPinchRemainsAtTheChosenZoom()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(100.0, 0.0), 120);
    recognizer.update(QPointF(0.0, 0.0), QPointF(120.0, 0.0), 180);

    QVERIFY(!recognizer.shouldFitOnRelease(200));
}

void KisQuickPinchRecognizerContractTest::canceledGestureCannotFitTheCanvas()
{
    KisQuickPinchRecognizer recognizer;
    recognizer.begin(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    recognizer.update(QPointF(0.0, 0.0), QPointF(120.0, 0.0), 180);
    recognizer.cancel();

    QVERIFY(!recognizer.shouldFitOnRelease(200));
}

QTEST_GUILESS_MAIN(KisQuickPinchRecognizerContractTest)

#include "KisQuickPinchRecognizerContractTest.moc"
