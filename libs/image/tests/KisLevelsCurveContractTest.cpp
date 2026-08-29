/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisLevelsCurve.h>

#include <QTest>

#include <cmath>

namespace
{
int safeAssertCount = 0;
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KisLevelsCurveContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsAndParameterizedCurveEvaluateLevels();
    void settersAndResetsUpdateOnlyTheirLevelGroups();
    void copyAssignmentAndEqualityPreserveValueSemantics();
    void transferTablesSampleEndpointsAndInvalidateAfterMutation();
    void serializationRoundTripsAndRejectsMalformedText();
};

void KisLevelsCurveContractTest::defaultsAndParameterizedCurveEvaluateLevels()
{
    static_assert(KisLevelsCurve::defaultInputBlackPoint() == 0.0);
    static_assert(KisLevelsCurve::defaultInputWhitePoint() == 1.0);
    static_assert(KisLevelsCurve::defaultInputGamma() == 1.0);
    static_assert(KisLevelsCurve::defaultOutputBlackPoint() == 0.0);
    static_assert(KisLevelsCurve::defaultOutputWhitePoint() == 1.0);

    KisLevelsCurve identity;
    QCOMPARE(identity.inputBlackPoint(), 0.0);
    QCOMPARE(identity.inputWhitePoint(), 1.0);
    QCOMPARE(identity.inputGamma(), 1.0);
    QCOMPARE(identity.outputBlackPoint(), 0.0);
    QCOMPARE(identity.outputWhitePoint(), 1.0);
    QVERIFY(identity.isIdentity());
    QCOMPARE(identity.value(-0.25), 0.0);
    QCOMPARE(identity.value(0.25), 0.25);
    QCOMPARE(identity.value(1.25), 1.0);

    const KisLevelsCurve adjusted(0.2, 0.8, 2.0, 0.1, 0.9);
    QCOMPARE(adjusted.inputBlackPoint(), 0.2);
    QCOMPARE(adjusted.inputWhitePoint(), 0.8);
    QCOMPARE(adjusted.inputGamma(), 2.0);
    QCOMPARE(adjusted.outputBlackPoint(), 0.1);
    QCOMPARE(adjusted.outputWhitePoint(), 0.9);
    QVERIFY(!adjusted.isIdentity());
    QCOMPARE(adjusted.value(0.1), 0.1);
    QCOMPARE(adjusted.value(0.2), 0.1);
    QCOMPARE(adjusted.value(0.8), 0.9);
    QCOMPARE(adjusted.value(0.9), 0.9);

    const qreal expectedMiddle = 0.1 + 0.8 * std::sqrt(0.5);
    QVERIFY(qAbs(adjusted.value(0.5) - expectedMiddle) < 1e-12);
}

void KisLevelsCurveContractTest::settersAndResetsUpdateOnlyTheirLevelGroups()
{
    KisLevelsCurve curve;
    const QVector<qreal> identityTransfer = curve.floatTransfer(5);

    curve.setInputBlackPoint(0.15);
    curve.setInputWhitePoint(0.85);
    curve.setInputGamma(1.5);
    curve.setOutputBlackPoint(0.25);
    curve.setOutputWhitePoint(0.75);

    QCOMPARE(curve.inputBlackPoint(), 0.15);
    QCOMPARE(curve.inputWhitePoint(), 0.85);
    QCOMPARE(curve.inputGamma(), 1.5);
    QCOMPARE(curve.outputBlackPoint(), 0.25);
    QCOMPARE(curve.outputWhitePoint(), 0.75);
    QVERIFY(curve.floatTransfer(5) != identityTransfer);

    curve.resetInputLevels();
    QCOMPARE(curve.inputBlackPoint(), KisLevelsCurve::defaultInputBlackPoint());
    QCOMPARE(curve.inputWhitePoint(), KisLevelsCurve::defaultInputWhitePoint());
    QCOMPARE(curve.inputGamma(), KisLevelsCurve::defaultInputGamma());
    QCOMPARE(curve.outputBlackPoint(), 0.25);
    QCOMPARE(curve.outputWhitePoint(), 0.75);

    curve.setInputBlackPoint(0.1);
    curve.setInputWhitePoint(0.9);
    curve.setInputGamma(2.0);
    curve.resetOutputLevels();
    QCOMPARE(curve.inputBlackPoint(), 0.1);
    QCOMPARE(curve.inputWhitePoint(), 0.9);
    QCOMPARE(curve.inputGamma(), 2.0);
    QCOMPARE(curve.outputBlackPoint(), KisLevelsCurve::defaultOutputBlackPoint());
    QCOMPARE(curve.outputWhitePoint(), KisLevelsCurve::defaultOutputWhitePoint());

    curve.resetAll();
    QVERIFY(curve.isIdentity());
    QCOMPARE(curve.floatTransfer(5), identityTransfer);
}

void KisLevelsCurveContractTest::copyAssignmentAndEqualityPreserveValueSemantics()
{
    KisLevelsCurve original(0.2, 0.8, 1.5, 0.1, 0.9);
    original.setName(QStringLiteral("Original"));
    const QVector<qreal> originalFloatTransfer = original.floatTransfer(5);
    const QVector<quint16> originalUint16Transfer = original.uint16Transfer(5);

    KisLevelsCurve copied(original);
    QCOMPARE(copied.name(), QStringLiteral("Original"));
    QCOMPARE(copied.floatTransfer(5), originalFloatTransfer);
    QCOMPARE(copied.uint16Transfer(5), originalUint16Transfer);
    QVERIFY(copied == original);

    copied.setName(QStringLiteral("Renamed"));
    QCOMPARE(copied.name(), QStringLiteral("Renamed"));
    QVERIFY(copied == original);

    KisLevelsCurve assigned;
    KisLevelsCurve &assignmentResult = (assigned = copied);
    QCOMPARE(&assignmentResult, &assigned);
    QCOMPARE(assigned.name(), QStringLiteral("Renamed"));
    QCOMPARE(assigned.floatTransfer(5), originalFloatTransfer);
    QCOMPARE(assigned.uint16Transfer(5), originalUint16Transfer);
    QVERIFY(assigned == copied);

    assigned.setInputGamma(2.0);
    QVERIFY(!(assigned == copied));
    QCOMPARE(original.floatTransfer(5), originalFloatTransfer);
    QCOMPARE(original.uint16Transfer(5), originalUint16Transfer);
}

void KisLevelsCurveContractTest::transferTablesSampleEndpointsAndInvalidateAfterMutation()
{
    KisLevelsCurve identity;
    const QVector<qreal> identityFloat = identity.floatTransfer();
    const QVector<quint16> identityUint16 = identity.uint16Transfer();

    QCOMPARE(identityFloat.size(), 256);
    QCOMPARE(identityFloat.first(), 0.0);
    QCOMPARE(identityFloat.last(), 1.0);
    QCOMPARE(identityFloat.at(128), qreal(128.0 / 255.0));
    QCOMPARE(identityUint16.size(), 256);
    QCOMPARE(identityUint16.first(), quint16(0));
    QCOMPARE(identityUint16.last(), quint16(0xffff));
    QCOMPARE(identityUint16.at(128), quint16(32896));

    KisLevelsCurve adjusted(0.2, 0.8, 2.0, 0.1, 0.9);
    const QVector<qreal> adjustedFloat = adjusted.floatTransfer(5);
    const QVector<quint16> adjustedUint16 = adjusted.uint16Transfer(5);
    for (int i = 0; i < 5; ++i) {
        const qreal x = qreal(i) / 4.0;
        const qreal expected = adjusted.value(x);
        QVERIFY(qAbs(adjustedFloat.at(i) - expected) < 1e-12);
        QCOMPARE(adjustedUint16.at(i), quint16(qRound(expected * 0xffff)));
    }

    adjusted.setInputGamma(1.0);
    QVERIFY(adjusted.floatTransfer(5) != adjustedFloat);
    QVERIFY(adjusted.uint16Transfer(5) != adjustedUint16);
}

void KisLevelsCurveContractTest::serializationRoundTripsAndRejectsMalformedText()
{
    safeAssertCount = 0;

    KisLevelsCurve source(0.2, 0.8, 1.5, 0.1, 0.9);
    source.setName(QStringLiteral("Display name"));
    QCOMPARE(source.toString(), QStringLiteral("0.2;0.8;1.5;0.1;0.9"));

    const KisLevelsCurve constructed(source.toString());
    QVERIFY(constructed == source);
    QVERIFY(constructed.name().isEmpty());

    KisLevelsCurve parsed;
    bool ok = false;
    parsed.fromString(QStringLiteral("0.25;0.75;2;0.2;0.8"), &ok);
    QVERIFY(ok);
    QVERIFY(parsed == KisLevelsCurve(0.25, 0.75, 2.0, 0.2, 0.8));

    parsed.fromString(QStringLiteral("0,2;0,8;1,5;0,1;0,9"), &ok);
    QVERIFY(ok);
    QVERIFY(parsed == source);

    parsed.fromString(QStringLiteral("0;1;1;0;1"));
    QVERIFY(parsed.isIdentity());

    parsed.setName(QStringLiteral("Preserved name"));
    parsed.setInputGamma(2.0);
    parsed.fromString(QString(), &ok);
    QVERIFY(!ok);
    QVERIFY(parsed.isIdentity());
    QCOMPARE(parsed.name(), QStringLiteral("Preserved name"));

    parsed.setInputGamma(2.0);
    parsed.fromString(QStringLiteral("0;1;1"), &ok);
    QVERIFY(!ok);
    QVERIFY(parsed.isIdentity());

    parsed.setInputGamma(2.0);
    parsed.fromString(QStringLiteral("0;1;invalid;0;1"), &ok);
    QVERIFY(!ok);
    QVERIFY(parsed.isIdentity());
    QCOMPARE(safeAssertCount, 3);
}

QTEST_GUILESS_MAIN(KisLevelsCurveContractTest)

#include "KisLevelsCurveContractTest.moc"
