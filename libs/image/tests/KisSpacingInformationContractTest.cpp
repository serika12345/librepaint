/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kis_spacing_information.h"

#include <QTest>

class KisSpacingInformationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultSpacingIsEnabledAndZero();
    void isotropicConstructorsPreserveSpacingAndState();
    void anisotropicConstructorsPreserveSpacingAndState();
};

void KisSpacingInformationContractTest::defaultSpacingIsEnabledAndZero()
{
    const KisSpacingInformation spacing;

    QVERIFY(spacing.isDistanceSpacingEnabled());
    QCOMPARE(spacing.distanceSpacing(), QPointF());
    QVERIFY(spacing.isIsotropic());
    QCOMPARE(spacing.scalarApprox(), 0.0);
    QCOMPARE(spacing.rotation(), 0.0);
    QVERIFY(!spacing.coordinateSystemFlipped());
}

void KisSpacingInformationContractTest::isotropicConstructorsPreserveSpacingAndState()
{
    const KisSpacingInformation enabledSpacing(2.5);
    QVERIFY(enabledSpacing.isDistanceSpacingEnabled());
    QCOMPARE(enabledSpacing.distanceSpacing(), QPointF(2.5, 2.5));
    QVERIFY(enabledSpacing.isIsotropic());
    QCOMPARE(enabledSpacing.scalarApprox(), 2.5);
    QCOMPARE(enabledSpacing.rotation(), 0.0);
    QVERIFY(!enabledSpacing.coordinateSystemFlipped());

    const KisSpacingInformation disabledSpacing(false, 3.5);
    QVERIFY(!disabledSpacing.isDistanceSpacingEnabled());
    QCOMPARE(disabledSpacing.distanceSpacing(), QPointF(3.5, 3.5));
    QVERIFY(disabledSpacing.isIsotropic());
    QCOMPARE(disabledSpacing.scalarApprox(), 3.5);
}

void KisSpacingInformationContractTest::anisotropicConstructorsPreserveSpacingAndState()
{
    const KisSpacingInformation enabledSpacing(QPointF(3.0, 4.0), 0.75, true);
    QVERIFY(enabledSpacing.isDistanceSpacingEnabled());
    QCOMPARE(enabledSpacing.distanceSpacing(), QPointF(3.0, 4.0));
    QVERIFY(!enabledSpacing.isIsotropic());
    QCOMPARE(enabledSpacing.scalarApprox(), 5.0);
    QCOMPARE(enabledSpacing.rotation(), 0.75);
    QVERIFY(enabledSpacing.coordinateSystemFlipped());

    const KisSpacingInformation disabledSpacing(false, QPointF(5.0, 12.0), -0.5, false);
    QVERIFY(!disabledSpacing.isDistanceSpacingEnabled());
    QCOMPARE(disabledSpacing.distanceSpacing(), QPointF(5.0, 12.0));
    QVERIFY(!disabledSpacing.isIsotropic());
    QCOMPARE(disabledSpacing.scalarApprox(), 13.0);
    QCOMPARE(disabledSpacing.rotation(), -0.5);
    QVERIFY(!disabledSpacing.coordinateSystemFlipped());
}

QTEST_GUILESS_MAIN(KisSpacingInformationContractTest)

#include "KisSpacingInformationContractTest.moc"
