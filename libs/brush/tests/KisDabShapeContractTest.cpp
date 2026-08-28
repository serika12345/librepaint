/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dab_shape.h"

#include <QTest>

#include <cmath>
#include <limits>

class KisDabShapeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultShapeIsUnscaledAndUnrotated();
    void shapePreservesScaleRatioAndRotation();
    void equalityUsesFuzzyComponentComparison();
};

void KisDabShapeContractTest::defaultShapeIsUnscaledAndUnrotated()
{
    const KisDabShape shape;

    QCOMPARE(shape.scale(), 1.0);
    QCOMPARE(shape.scaleX(), 1.0);
    QCOMPARE(shape.scaleY(), 1.0);
    QCOMPARE(shape.ratio(), 1.0);
    QCOMPARE(shape.rotation(), 0.0);
}

void KisDabShapeContractTest::shapePreservesScaleRatioAndRotation()
{
    const KisDabShape shape(2.5, 0.4, -0.75);

    QCOMPARE(shape.scale(), 2.5);
    QCOMPARE(shape.scaleX(), 2.5);
    QCOMPARE(shape.scaleY(), 1.0);
    QCOMPARE(shape.ratio(), 0.4);
    QCOMPARE(shape.rotation(), -0.75);
}

void KisDabShapeContractTest::equalityUsesFuzzyComponentComparison()
{
    const KisDabShape shape(2.5, 0.4, -0.75);
    const KisDabShape nextScale(std::nextafter(2.5, std::numeric_limits<qreal>::infinity()), 0.4, -0.75);

    QVERIFY(shape == KisDabShape(2.5, 0.4, -0.75));
    QVERIFY(shape == nextScale);
    QVERIFY(!(shape == KisDabShape(2.6, 0.4, -0.75)));
    QVERIFY(!(shape == KisDabShape(2.5, 0.5, -0.75)));
    QVERIFY(!(shape == KisDabShape(2.5, 0.4, -0.5)));
}

QTEST_GUILESS_MAIN(KisDabShapeContractTest)

#include "KisDabShapeContractTest.moc"
