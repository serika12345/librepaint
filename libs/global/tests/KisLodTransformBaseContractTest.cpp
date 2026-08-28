/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_lod_transform_base.h"

#include <QPointF>
#include <QTest>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

namespace
{

class MockBounds
{
public:
    int currentLevelOfDetail() const
    {
        return 3;
    }
};

class MockPaintDevice
{
public:
    MockBounds *defaultBounds()
    {
        return &bounds;
    }

private:
    MockBounds bounds;
};

} // namespace

class KisLodTransformBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lodScaleConversionsClampAndRoundDown();
    void deviceLevelOfDetailSelectsScale();
    void transformMapsAndInvertsRectanglesAndPoints();
    void alignedRectExpandsToLodBoundaries();
    void integerRectScalingRoundTripsAlignedCoordinates();
    void scalarTransformScalesValues();
};

void KisLodTransformBaseContractTest::lodScaleConversionsClampAndRoundDown()
{
    QCOMPARE(KisLodTransformBase::lodToScale(-1), 1.0);
    QCOMPARE(KisLodTransformBase::lodToScale(0), 1.0);
    QCOMPARE(KisLodTransformBase::lodToScale(1), 0.5);
    QCOMPARE(KisLodTransformBase::lodToScale(3), 0.125);

    QCOMPARE(KisLodTransformBase::lodToInvScale(-1), 1.0);
    QCOMPARE(KisLodTransformBase::lodToInvScale(3), 8.0);

    QCOMPARE(KisLodTransformBase::scaleToLod(2.0, 6), 0);
    QCOMPARE(KisLodTransformBase::scaleToLod(0.5, 6), 1);
    QCOMPARE(KisLodTransformBase::scaleToLod(0.24, 6), 2);
    QCOMPARE(KisLodTransformBase::scaleToLod(0.01, 4), 4);
}

void KisLodTransformBaseContractTest::deviceLevelOfDetailSelectsScale()
{
    MockPaintDevice device;

    QCOMPARE(KisLodTransformBase::lodToScale(&device), 0.125);
    const KisLodTransformScalar scalar(&device);
    QCOMPARE(scalar.scale(16.0), 2.0);
}

void KisLodTransformBaseContractTest::transformMapsAndInvertsRectanglesAndPoints()
{
    const KisLodTransformBase transform(2);

    QCOMPARE(transform.transform(), QTransform::fromScale(0.25, 0.25));
    QCOMPARE(transform.map(QRectF(4.0, 8.0, 8.0, 12.0)), QRectF(1.0, 2.0, 2.0, 3.0));
    QCOMPARE(transform.map(QRect(4, 8, 8, 12)), QRect(1, 2, 2, 3));
    QCOMPARE(transform.map(QPointF(8.0, 12.0)), QPointF(2.0, 3.0));
    QCOMPARE(transform.mapInverted(QRectF(1.0, 2.0, 2.0, 3.0)), QRectF(4.0, 8.0, 8.0, 12.0));
    QCOMPARE(transform.mapInverted(QRect(1, 2, 2, 3)), QRect(4, 8, 8, 12));
}

void KisLodTransformBaseContractTest::alignedRectExpandsToLodBoundaries()
{
    QCOMPARE(KisLodTransformBase::alignedRect(QRect(3, 5, 4, 5), 2), QRect(0, 4, 8, 8));
}

void KisLodTransformBaseContractTest::integerRectScalingRoundTripsAlignedCoordinates()
{
    const QRect aligned(4, 8, 8, 8);
    const QRect scaled = KisLodTransformBase::scaledRect(aligned, 2);

    QCOMPARE(scaled, QRect(1, 2, 2, 2));
    QCOMPARE(KisLodTransformBase::upscaledRect(scaled, 2), aligned);
    QCOMPARE(KisLodTransformBase::coordToLodCoord(9, 2), 2);
    QCOMPARE(KisLodTransformBase::coordToLodCoord(-9, 2), -2);
}

void KisLodTransformBaseContractTest::scalarTransformScalesValues()
{
    const KisLodTransformScalar scalar(3);

    QCOMPARE(scalar.scale(16.0), 2.0);
}

QTEST_GUILESS_MAIN(KisLodTransformBaseContractTest)

#include "KisLodTransformBaseContractTest.moc"
