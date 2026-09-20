/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "svg/SvgMeshGradient.h"

#include <QTest>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

class SvgMeshGradientContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsExposeEmptyMeshAndStableEnumValues();
    void typeAndCoordinateUnitsRoundTripIndependently();
    void validMeshForwardsBoundsAndTransform();
    void copyOwnsIndependentMeshState();
};

void SvgMeshGradientContractTest::defaultsExposeEmptyMeshAndStableEnumValues()
{
    QCOMPARE(int(SvgMeshGradient::BILINEAR), 0);
    QCOMPARE(int(SvgMeshGradient::BICUBIC), 1);

    const SvgMeshGradient gradient;
    QCOMPARE(gradient.type(), SvgMeshGradient::BILINEAR);
    QCOMPARE(gradient.gradientUnits(), KoFlake::UserSpaceOnUse);
    QVERIFY(gradient.getMeshArray());
    QCOMPARE(gradient.getMeshArray()->numRows(), 0);
    QCOMPARE(gradient.getMeshArray()->numColumns(), 0);
    QVERIFY(!gradient.isValid());
}

void SvgMeshGradientContractTest::typeAndCoordinateUnitsRoundTripIndependently()
{
    SvgMeshGradient gradient;

    gradient.setType(SvgMeshGradient::BICUBIC);
    QCOMPARE(gradient.type(), SvgMeshGradient::BICUBIC);
    QCOMPARE(gradient.gradientUnits(), KoFlake::UserSpaceOnUse);

    gradient.setGradientUnits(KoFlake::ObjectBoundingBox);
    QCOMPARE(gradient.gradientUnits(), KoFlake::ObjectBoundingBox);
    QCOMPARE(gradient.type(), SvgMeshGradient::BICUBIC);

    gradient.setType(SvgMeshGradient::BILINEAR);
    QCOMPARE(gradient.gradientUnits(), KoFlake::ObjectBoundingBox);

    gradient.setGradientUnits();
    QCOMPARE(gradient.gradientUnits(), KoFlake::UserSpaceOnUse);
    QCOMPARE(gradient.type(), SvgMeshGradient::BILINEAR);
}

void SvgMeshGradientContractTest::validMeshForwardsBoundsAndTransform()
{
    SvgMeshGradient gradient;
    gradient.getMeshArray()->createDefaultMesh(1, 1, QColor(Qt::cyan), QSizeF(4.0, 3.0));

    QVERIFY(gradient.isValid());
    QCOMPARE(gradient.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));

    gradient.setTransform(QTransform::fromTranslate(2.0, -3.0));
    QCOMPARE(gradient.boundingRect(), QRectF(2.0, -3.0, 1.0, 1.0));
}

void SvgMeshGradientContractTest::copyOwnsIndependentMeshState()
{
    SvgMeshGradient original;
    original.setType(SvgMeshGradient::BICUBIC);
    original.setGradientUnits(KoFlake::ObjectBoundingBox);
    original.getMeshArray()->createDefaultMesh(1, 1, QColor(Qt::cyan), QSizeF(4.0, 3.0));

    const SvgMeshGradient copy(original);
    QCOMPARE(copy.type(), SvgMeshGradient::BICUBIC);
    QCOMPARE(copy.gradientUnits(), KoFlake::ObjectBoundingBox);
    QVERIFY(copy.getMeshArray().data() != original.getMeshArray().data());
    QCOMPARE(copy.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));

    original.setType(SvgMeshGradient::BILINEAR);
    original.setGradientUnits();
    original.setTransform(QTransform::fromTranslate(5.0, 6.0));

    QCOMPARE(copy.type(), SvgMeshGradient::BICUBIC);
    QCOMPARE(copy.gradientUnits(), KoFlake::ObjectBoundingBox);
    QCOMPARE(copy.boundingRect(), QRectF(0.0, 0.0, 1.0, 1.0));
    QCOMPARE(original.boundingRect(), QRectF(5.0, 6.0, 1.0, 1.0));
}

QTEST_GUILESS_MAIN(SvgMeshGradientContractTest)

#include "SvgMeshGradientContractTest.moc"
