/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_quick_shape.h"

#include <QTest>

#include <cmath>

namespace {

QVector<QPointF> straightLine(const QPointF &start, const QPointF &end, int samples)
{
    QVector<QPointF> points;
    for (int i = 0; i < samples; ++i) {
        const qreal t = qreal(i) / (samples - 1);
        points.append(start + (end - start) * t);
    }
    return points;
}

QVector<QPointF> ellipseSamples(const QPointF &center,
                                qreal radiusX,
                                qreal radiusY,
                                qreal rotation,
                                int samples)
{
    const QPointF axis(std::cos(rotation), std::sin(rotation));
    const QPointF perpendicular(-axis.y(), axis.x());

    QVector<QPointF> points;
    for (int i = 0; i < samples; ++i) {
        const qreal t = 2.0 * M_PI * qreal(i) / samples;
        points.append(center
                      + axis * (radiusX * std::cos(t))
                      + perpendicular * (radiusY * std::sin(t)));
    }
    return points;
}

} // namespace

class KisQuickShapeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void recognizesStraightLines();
    void recognizesTwoSampleLine();
    void recognizesWobblyLine();
    void recognizesEllipse();
    void circleSnapKeepsLargestRadius();
    void rejectsOpenArc();
    void rejectsShortGesture();
    void trackerRestartsHoldAfterMovement();
    void trackerRecognizesHeldGesture();
    void trackerMovesLineEndpointWithThePointer();
    void trackerScalesEllipseAboutItsCenter();
    void trackerRotatesAndScalesEllipse_data();
    void trackerRotatesAndScalesEllipse();
};

void KisQuickShapeContractTest::recognizesStraightLines()
{
    const KisQuickShape horizontal =
        KisQuickShape::recognize(straightLine(QPointF(10.0, 20.0), QPointF(210.0, 20.0), 16));
    QCOMPARE(horizontal.type(), KisQuickShape::Line);
    QCOMPARE(horizontal.lineStart(), QPointF(10.0, 20.0));
    QCOMPARE(horizontal.lineEnd(), QPointF(210.0, 20.0));

    const KisQuickShape diagonal =
        KisQuickShape::recognize(straightLine(QPointF(0.0, 0.0), QPointF(120.0, 90.0), 24));
    QCOMPARE(diagonal.type(), KisQuickShape::Line);
}

void KisQuickShapeContractTest::recognizesTwoSampleLine()
{
    // A short drag with few move events still describes a straight gesture.
    const KisQuickShape shape =
        KisQuickShape::recognize(QVector<QPointF>{QPointF(20.0, 20.0), QPointF(120.0, 60.0)});
    QCOMPARE(shape.type(), KisQuickShape::Line);
    QCOMPARE(shape.lineEnd(), QPointF(120.0, 60.0));
}

void KisQuickShapeContractTest::recognizesWobblyLine()
{
    QVector<QPointF> points;
    for (int i = 0; i < 40; ++i) {
        const qreal x = 20.0 + 8.0 * i;
        const qreal wobble = (i % 2 == 0) ? 1.5 : -1.5;
        points.append(QPointF(x, 100.0 + wobble));
    }

    const KisQuickShape shape = KisQuickShape::recognize(points);
    QCOMPARE(shape.type(), KisQuickShape::Line);
}

void KisQuickShapeContractTest::recognizesEllipse()
{
    const QVector<QPointF> points =
        ellipseSamples(QPointF(150.0, 90.0), 80.0, 40.0, 0.0, 64);
    const KisQuickShape shape = KisQuickShape::recognize(points);

    QCOMPARE(shape.type(), KisQuickShape::Ellipse);
    QVERIFY(qAbs(shape.center().x() - 150.0) < 1.0);
    QVERIFY(qAbs(shape.center().y() - 90.0) < 1.0);
    QVERIFY(qAbs(shape.radiusX() - 80.0) < 1.0);
    QVERIFY(qAbs(shape.radiusY() - 40.0) < 1.0);

    const QVector<QPointF> rotated =
        ellipseSamples(QPointF(0.0, 0.0), 70.0, 25.0, M_PI / 6.0, 64);
    const KisQuickShape rotatedShape = KisQuickShape::recognize(rotated);
    QCOMPARE(rotatedShape.type(), KisQuickShape::Ellipse);
    QVERIFY(qAbs(rotatedShape.rotation() - M_PI / 6.0) < 0.05);
}

void KisQuickShapeContractTest::circleSnapKeepsLargestRadius()
{
    const QVector<QPointF> points =
        ellipseSamples(QPointF(0.0, 0.0), 100.0, 30.0, 0.0, 64);
    const KisQuickShape shape = KisQuickShape::recognize(points);
    QCOMPARE(shape.type(), KisQuickShape::Ellipse);
    QVERIFY(!shape.isSnappedToCircle());
    QVERIFY(qAbs(shape.radiusX() - 100.0) < 1.5);
    QVERIFY(qAbs(shape.radiusY() - 30.0) < 1.5);

    KisQuickShape circle = shape;
    circle.setSnappedToCircle(true);
    QVERIFY(qAbs(circle.radiusX() - 100.0) < 1.5);
    QVERIFY(qAbs(circle.radiusY() - 100.0) < 1.5);

    circle.setSnappedToCircle(false);
    QVERIFY(qAbs(circle.radiusY() - 30.0) < 1.5);
}

void KisQuickShapeContractTest::rejectsOpenArc()
{
    // Three quarters of a circle: the closing gap is far too wide for an
    // ellipse and the samples do not follow the chord.
    QVector<QPointF> points;
    const int samples = 48;
    for (int i = 0; i < samples; ++i) {
        const qreal t = 1.5 * M_PI * qreal(i) / samples;
        points.append(QPointF(120.0 * std::cos(t), 120.0 * std::sin(t)));
    }

    QCOMPARE(KisQuickShape::recognize(points).type(), KisQuickShape::NoShape);
}

void KisQuickShapeContractTest::rejectsShortGesture()
{
    const QVector<QPointF> points = straightLine(QPointF(4.0, 4.0), QPointF(7.0, 5.0), 6);
    QCOMPARE(KisQuickShape::recognize(points).type(), KisQuickShape::NoShape);
}

void KisQuickShapeContractTest::trackerRestartsHoldAfterMovement()
{
    KisQuickShapeTracker tracker;
    tracker.begin(QPointF(10.0, 10.0));
    QVERIFY(tracker.isTracking());

    QVERIFY(!tracker.extend(QPointF(11.0, 10.0)));
    QVERIFY(tracker.extend(QPointF(20.0, 10.0)));
    QVERIFY(tracker.isTracking());
    QCOMPARE(tracker.pointCount(), 3);
}

void KisQuickShapeContractTest::trackerRecognizesHeldGesture()
{
    KisQuickShapeTracker tracker;
    const QVector<QPointF> points =
        straightLine(QPointF(0.0, 0.0), QPointF(160.0, 0.0), 12);

    tracker.begin(points.first());
    for (int i = 1; i < points.size(); ++i) {
        tracker.extend(points.at(i));
    }

    QVERIFY(tracker.recognize());
    QVERIFY(!tracker.isTracking());
    QCOMPARE(tracker.shape().type(), KisQuickShape::Line);
    QVERIFY(!tracker.recognize());

    QVERIFY(!tracker.toggleCircle());
}

void KisQuickShapeContractTest::trackerMovesLineEndpointWithThePointer()
{
    const QVector<QPointF> points =
        straightLine(QPointF(20.0, 30.0), QPointF(140.0, 30.0), 12);

    KisQuickShapeTracker tracker;
    tracker.begin(points.first());
    for (int i = 1; i < points.size(); ++i) {
        tracker.extend(points.at(i));
    }
    QVERIFY(tracker.recognize());

    const QPointF start = tracker.shape().lineStart();
    QCOMPARE(start, QPointF(20.0, 30.0));
    QCOMPARE(tracker.shape().lineEnd(), points.last());

    // The endpoint follows fine movement, shortening, extension and crossing
    // the start. Returning to the start can then be followed by another drag.
    const QVector<QPointF> positions {
        QPointF(139.0, 31.0),
        QPointF(20.0, 80.0),
        QPointF(400.0, 400.0),
        QPointF(-40.0, 10.0),
        start,
        QPointF(140.0, 30.0)
    };
    for (const QPointF &position : positions) {
        QVERIFY(tracker.adjustTo(position));
        QCOMPARE(tracker.shape().lineStart(), start);
        QCOMPARE(tracker.shape().lineEnd(), position);
        QVERIFY(!tracker.adjustTo(position));
    }
}

void KisQuickShapeContractTest::trackerScalesEllipseAboutItsCenter()
{
    const QVector<QPointF> points =
        ellipseSamples(QPointF(100.0, 100.0), 60.0, 30.0, 0.0, 64);

    KisQuickShapeTracker tracker;
    tracker.begin(points.first());
    for (int i = 1; i < points.size(); ++i) {
        tracker.extend(points.at(i));
    }
    QVERIFY(tracker.recognize());

    const QPointF center = tracker.shape().center();
    const qreal radiusX = tracker.shape().radiusX();
    const qreal radiusY = tracker.shape().radiusY();
    const QPointF holdPosition = points.last();
    const QPointF fromCenter = holdPosition - center;

    // Ignore the hand tremor that stays inside the hold slop.
    QVERIFY(!tracker.adjustTo(holdPosition + QPointF(1.0, 1.0)));
    QVERIFY(qAbs(tracker.shape().radiusX() - radiusX) < 0.001);

    // Moving twice as far from the center as the hold position doubles the size.
    QVERIFY(tracker.adjustTo(center + fromCenter * 2.0));
    QCOMPARE(tracker.shape().center(), center);
    QVERIFY(qAbs(tracker.shape().radiusX() - 2.0 * radiusX) < 0.5);
    QVERIFY(qAbs(tracker.shape().radiusY() - 2.0 * radiusY) < 0.5);

    // The shape never collapses into nothing.
    QVERIFY(tracker.adjustTo(center + fromCenter * 0.001));
    QCOMPARE(tracker.shape().center(), center);
    QVERIFY(tracker.shape().radiusY() > 1.9);
}

void KisQuickShapeContractTest::trackerRotatesAndScalesEllipse_data()
{
    QTest::addColumn<qreal>("angle");
    QTest::addColumn<qreal>("scale");
    QTest::newRow("counterclockwise") << qreal(M_PI / 4) << qreal(1.0);
    QTest::newRow("clockwise") << qreal(-M_PI / 2) << qreal(1.0);
    QTest::newRow("rotate-and-enlarge") << qreal(2 * M_PI / 3) << qreal(2.0);
    QTest::newRow("angle-wrap-and-shrink") << qreal(-10 * M_PI / 9) << qreal(0.5);
}

void KisQuickShapeContractTest::trackerRotatesAndScalesEllipse()
{
    QFETCH(qreal, angle);
    QFETCH(qreal, scale);
    const QVector<QPointF> points =
        ellipseSamples(QPointF(100, 100), 60, 30, M_PI / 6, 64);
    KisQuickShapeTracker tracker;
    tracker.begin(points.first());
    for (int i = 1; i < points.size(); ++i) {
        tracker.extend(points.at(i));
    }
    QVERIFY(tracker.recognize());
    const KisQuickShape original = tracker.shape();
    const QPointF reference = points.last() - original.center();
    const QPointF rotated(reference.x() * std::cos(angle) - reference.y() * std::sin(angle),
                          reference.x() * std::sin(angle) + reference.y() * std::cos(angle));
    const QPointF position = original.center() + rotated * scale;

    // Returning to the held point restores the fitted shape without drift.
    for (int i = 0; i < 3; ++i) {
        QVERIFY(tracker.adjustTo(position));
        const KisQuickShape &shape = tracker.shape();
        QCOMPARE(shape.center(), original.center());
        QVERIFY(qAbs(shape.radiusX() - original.radiusX() * scale) < 0.001);
        QVERIFY(qAbs(shape.radiusY() - original.radiusY() * scale) < 0.001);
        QVERIFY(qAbs(std::cos(shape.rotation()) - std::cos(original.rotation() + angle)) < 0.001);
        QVERIFY(qAbs(std::sin(shape.rotation()) - std::sin(original.rotation() + angle)) < 0.001);
        QVERIFY(!tracker.adjustTo(position));

        // A circle round trip retains the edited ellipse orientation and size.
        const qreal rotation = shape.rotation();
        QVERIFY(tracker.toggleCircle());
        QCOMPARE(shape.radiusX(), shape.radiusY());
        QVERIFY(tracker.toggleCircle());
        QCOMPARE(shape.rotation(), rotation);
        QVERIFY(qAbs(shape.radiusY() - original.radiusY() * scale) < 0.001);

        QVERIFY(tracker.adjustTo(points.last()));
        QVERIFY(qAbs(shape.rotation() - original.rotation()) < 0.001);
        QVERIFY(qAbs(shape.radiusX() - original.radiusX()) < 0.001);
    }
}

QTEST_MAIN(KisQuickShapeContractTest)

#include "KisQuickShapeContractTest.moc"
