/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cubic_curve.h"

#include <QTest>

#include <memory>

namespace
{

constexpr qreal tolerance = 1e-9;

void compareNear(qreal actual, qreal expected)
{
    QVERIFY2(qAbs(actual - expected) <= tolerance, "curve value differs");
}

void comparePoint(const KisCubicCurvePoint &point, const QPointF &position, bool isCorner)
{
    QCOMPARE(point.position(), position);
    QCOMPARE(point.x(), position.x());
    QCOMPARE(point.y(), position.y());
    QCOMPARE(point.isSetAsCorner(), isCorner);
}

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    qFatal("The cubic curve contract reached an unexpected safe assertion: %s", assertion);
}

class KisCubicCurveContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pointValuePreservesCoordinatesCornerAndCopies();
    void constructorsSortAndExposeCurvePoints();
    void editingOverloadsPreserveOrderingAndInvalidateValues();
    void copyAssignmentMetadataAndPredicatesPreserveValueSemantics();
    void evaluationAndTransfersClampAndSample();
    void serializationRoundTripsCornerAndCompatibilityRoutes();
};

void KisCubicCurveContractTest::pointValuePreservesCoordinatesCornerAndCopies()
{
    const KisCubicCurvePoint defaultPoint;
    comparePoint(defaultPoint, QPointF(0.0, 0.0), false);

    const KisCubicCurvePoint coordinatePoint(0.25, 0.75, true);
    comparePoint(coordinatePoint, QPointF(0.25, 0.75), true);

    const KisCubicCurvePoint positionPoint(QPointF(0.4, 0.6));
    comparePoint(positionPoint, QPointF(0.4, 0.6), false);

    const KisCubicCurvePoint copiedPoint(coordinatePoint);
    QVERIFY(copiedPoint == coordinatePoint);

    KisCubicCurvePoint assignedPoint;
    KisCubicCurvePoint &assignmentResult = (assignedPoint = copiedPoint);
    QCOMPARE(&assignmentResult, &assignedPoint);
    QVERIFY(assignedPoint == coordinatePoint);

    assignedPoint.setX(0.35);
    assignedPoint.setY(0.65);
    comparePoint(assignedPoint, QPointF(0.35, 0.65), true);

    assignedPoint.setPosition(QPointF(0.1, 0.9));
    assignedPoint.setAsCorner(false);
    comparePoint(assignedPoint, QPointF(0.1, 0.9), false);
    QVERIFY(!(assignedPoint == coordinatePoint));
}

void KisCubicCurveContractTest::constructorsSortAndExposeCurvePoints()
{
    const KisCubicCurve defaults;
    QCOMPARE(defaults.curvePoints().size(), 2);
    comparePoint(defaults.curvePoints().at(0), QPointF(0.0, 0.0), false);
    comparePoint(defaults.curvePoints().at(1), QPointF(1.0, 1.0), false);
    QCOMPARE(defaults.points(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(1.0, 1.0)}));

    const QList<QPointF> positions{
        QPointF(0.8, 0.6),
        QPointF(0.1, 0.0),
        QPointF(0.5, 0.7),
    };
    const KisCubicCurve positionCurve(positions);
    QCOMPARE(positionCurve.curvePoints().size(), 3);
    comparePoint(positionCurve.curvePoints().at(0), QPointF(0.1, 0.0), false);
    comparePoint(positionCurve.curvePoints().at(1), QPointF(0.5, 0.7), false);
    comparePoint(positionCurve.curvePoints().at(2), QPointF(0.8, 0.6), false);

    const QList<KisCubicCurvePoint> curvePoints{
        KisCubicCurvePoint(0.9, 1.0),
        KisCubicCurvePoint(0.2, 0.1),
        KisCubicCurvePoint(0.6, 0.4, true),
    };
    const KisCubicCurve pointCurve(curvePoints);
    QCOMPARE(pointCurve.curvePoints().size(), 3);
    comparePoint(pointCurve.curvePoints().at(0), QPointF(0.2, 0.1), false);
    comparePoint(pointCurve.curvePoints().at(1), QPointF(0.6, 0.4), true);
    comparePoint(pointCurve.curvePoints().at(2), QPointF(0.9, 1.0), false);
}

void KisCubicCurveContractTest::editingOverloadsPreserveOrderingAndInvalidateValues()
{
    KisCubicCurve curve;

    QCOMPARE(curve.addPoint(KisCubicCurvePoint(0.75, 0.6, true)), 1);
    QCOMPARE(curve.addPoint(QPointF(0.25, 0.2), false), 1);
    QCOMPARE(curve.addPoint(QPointF(0.5, 0.4)), 2);
    QCOMPARE(curve.curvePoints().size(), 5);
    comparePoint(curve.curvePoints().at(1), QPointF(0.25, 0.2), false);
    comparePoint(curve.curvePoints().at(2), QPointF(0.5, 0.4), false);
    comparePoint(curve.curvePoints().at(3), QPointF(0.75, 0.6), true);

    curve.setPoint(1, KisCubicCurvePoint(0.9, 0.8, true));
    comparePoint(curve.curvePoints().at(3), QPointF(0.9, 0.8), true);

    curve.setPoint(3, QPointF(0.2, 0.3), false);
    comparePoint(curve.curvePoints().at(1), QPointF(0.2, 0.3), false);

    curve.setPoint(1, QPointF(0.3, 0.35));
    comparePoint(curve.curvePoints().at(1), QPointF(0.3, 0.35), false);

    curve.setPointPosition(1, QPointF(0.4, 0.45));
    curve.setPointAsCorner(1, true);
    comparePoint(curve.curvePoints().at(1), QPointF(0.4, 0.45), true);

    curve.removePoint(2);
    QCOMPARE(curve.curvePoints().size(), 4);
    comparePoint(curve.curvePoints().at(2), QPointF(0.75, 0.6), true);

    const QVector<qreal> transferBeforeReplacement = curve.floatTransfer(5);
    const QList<KisCubicCurvePoint> replacementPoints{
        KisCubicCurvePoint(0.0, 0.2),
        KisCubicCurvePoint(0.5, 0.6, true),
        KisCubicCurvePoint(1.0, 0.9),
    };
    curve.setPoints(replacementPoints);
    QCOMPARE(curve.curvePoints().size(), 3);
    comparePoint(curve.curvePoints().at(1), QPointF(0.5, 0.6), true);
    compareNear(curve.value(0.5), 0.6);
    QVERIFY(curve.floatTransfer(5) != transferBeforeReplacement);

    const QList<QPointF> replacementPositions{
        QPointF(0.0, 0.0),
        QPointF(0.5, 0.25),
        QPointF(1.0, 1.0),
    };
    curve.setPoints(replacementPositions);
    QCOMPARE(curve.curvePoints().size(), 3);
    comparePoint(curve.curvePoints().at(1), QPointF(0.5, 0.25), false);
    compareNear(curve.value(0.5), 0.25);
}

void KisCubicCurveContractTest::copyAssignmentMetadataAndPredicatesPreserveValueSemantics()
{
    const QList<KisCubicCurvePoint> identityPoints{
        KisCubicCurvePoint(0.0, 0.0),
        KisCubicCurvePoint(0.5, 0.5),
        KisCubicCurvePoint(1.0, 1.0),
    };
    KisCubicCurve source(identityPoints);
    source.setName(QStringLiteral("Source curve"));
    QVERIFY(source.isIdentity());
    QVERIFY(!source.isConstant(0.5));

    KisCubicCurve copied(source);
    QVERIFY(copied == source);
    QCOMPARE(copied.name(), QStringLiteral("Source curve"));
    copied.setPointPosition(1, QPointF(0.5, 0.7));
    QVERIFY(!(copied == source));
    comparePoint(source.curvePoints().at(1), QPointF(0.5, 0.5), false);

    KisCubicCurve assigned;
    KisCubicCurve &assignmentResult = (assigned = copied);
    QCOMPARE(&assignmentResult, &assigned);
    QVERIFY(assigned == copied);
    assigned.setPointAsCorner(1, true);
    QVERIFY(!(assigned == copied));

    KisCubicCurve firstNamed;
    firstNamed.setName(QStringLiteral("First display name"));
    KisCubicCurve secondNamed;
    secondNamed.setName(QStringLiteral("Second display name"));
    QCOMPARE(firstNamed.name(), QStringLiteral("First display name"));
    QCOMPARE(secondNamed.name(), QStringLiteral("Second display name"));
    QVERIFY(firstNamed == secondNamed);

    const KisCubicCurve constant(QList<QPointF>({QPointF(0.0, 0.4), QPointF(1.0, 0.4)}));
    QVERIFY(constant.isConstant(0.4));
    QVERIFY(!constant.isConstant(0.5));
    QVERIFY(!constant.isIdentity());

    auto lifetime = std::make_unique<KisCubicCurve>(source);
    QCOMPARE(lifetime->name(), QStringLiteral("Source curve"));
    lifetime.reset();
    QVERIFY(!lifetime);
}

void KisCubicCurveContractTest::evaluationAndTransfersClampAndSample()
{
    KisCubicCurve identity;
    QCOMPARE(identity.value(-0.25), 0.0);
    QCOMPARE(identity.value(0.25), 0.25);
    QCOMPARE(identity.value(0.75), 0.75);
    QCOMPARE(identity.value(1.25), 1.0);

    const QVector<qreal> expectedFloat{0.0, 0.25, 0.5, 0.75, 1.0};
    const QVector<quint16> expectedUint16{0, 16383, 32767, 49151, 65535};
    QCOMPARE(identity.floatTransfer(5), expectedFloat);
    QCOMPARE(identity.uint16Transfer(5), expectedUint16);
    QCOMPARE(identity.floatTransfer(5), expectedFloat);
    QCOMPARE(identity.uint16Transfer(5), expectedUint16);

    const QVector<qreal> linearTransfer{0.0, 0.2, 0.8, 1.0};
    QCOMPARE(KisCubicCurve::interpolateLinear(0.0, linearTransfer), 0.0);
    compareNear(KisCubicCurve::interpolateLinear(0.375, linearTransfer), 0.275);
    QCOMPARE(KisCubicCurve::interpolateLinear(1.0, linearTransfer), 1.0);

    const QList<KisCubicCurvePoint> cornerPoints{
        KisCubicCurvePoint(0.0, 1.0),
        KisCubicCurvePoint(0.5, 0.0, true),
        KisCubicCurvePoint(1.0, 1.0),
    };
    const KisCubicCurve cornerCurve(cornerPoints);
    compareNear(cornerCurve.value(0.25), 0.5);
    compareNear(cornerCurve.value(0.5), 0.0);
    compareNear(cornerCurve.value(0.75), 0.5);
}

void KisCubicCurveContractTest::serializationRoundTripsCornerAndCompatibilityRoutes()
{
    QCOMPARE(DEFAULT_CURVE_STRING, QStringLiteral("0,0;1,1;"));

    const QString serialized = QStringLiteral("0,1;0.5,0,is_corner;1,1;");
    const KisCubicCurve constructed(serialized);
    QCOMPARE(constructed.curvePoints().size(), 3);
    comparePoint(constructed.curvePoints().at(0), QPointF(0.0, 1.0), false);
    comparePoint(constructed.curvePoints().at(1), QPointF(0.5, 0.0), true);
    comparePoint(constructed.curvePoints().at(2), QPointF(1.0, 1.0), false);
    QCOMPARE(constructed.toString(), serialized);

    KisCubicCurve parsed;
    parsed.fromString(serialized);
    QVERIFY(parsed == constructed);
    QCOMPARE(parsed.toString(), serialized);
}

QTEST_GUILESS_MAIN(KisCubicCurveContractTest)

#include "KisCubicCurveContractTest.moc"
