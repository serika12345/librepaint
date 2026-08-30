/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRenderedDab.h"

#include <QTest>

namespace
{

int fixedDeviceDestructions = 0;
int fixedDeviceBoundsReads = 0;

void resetFixedDeviceObservations()
{
    fixedDeviceDestructions = 0;
    fixedDeviceBoundsReads = 0;
}

} // namespace

KisFixedPaintDevice::KisFixedPaintDevice(const KoColorSpace *colorSpace,
                                         KisOptimizedByteArray::MemoryAllocatorSP allocator)
    : m_colorSpace(colorSpace)
    , m_data(allocator)
{
}

KisFixedPaintDevice::~KisFixedPaintDevice()
{
    ++fixedDeviceDestructions;
}

void KisFixedPaintDevice::setRect(const QRect &rc)
{
    m_bounds = rc;
}

QRect KisFixedPaintDevice::bounds() const
{
    ++fixedDeviceBoundsReads;
    return m_bounds;
}

void KisFixedPaintDevice::convertFromQImage(const QImage &, const QString &)
{
    qFatal("KisRenderedDab must not request image conversion");
}

QImage KisFixedPaintDevice::convertToQImage(const KoColorProfile *,
                                            KoColorConversionTransformation::Intent,
                                            KoColorConversionTransformation::ConversionFlags) const
{
    qFatal("KisRenderedDab must not request image conversion");
}

QImage KisFixedPaintDevice::convertToQImage(const KoColorProfile *,
                                            qint32,
                                            qint32,
                                            qint32,
                                            qint32,
                                            KoColorConversionTransformation::Intent,
                                            KoColorConversionTransformation::ConversionFlags) const
{
    qFatal("KisRenderedDab must not request image conversion");
}

class KisRenderedDabContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultConstructionUsesStableMemberValues();
    void deviceConstructionCapturesOffsetAndRetainsOwnership();
    void copyPreservesMembersAndRealBoundsUsesCurrentDeviceSize();
};

void KisRenderedDabContractTest::defaultConstructionUsesStableMemberValues()
{
    const KisRenderedDab dab;

    QVERIFY(dab.device.isNull());
    QCOMPARE(dab.offset, QPoint());
    QCOMPARE(dab.opacity, OPACITY_OPAQUE_F);
    QCOMPARE(dab.flow, OPACITY_OPAQUE_F);
    QCOMPARE(dab.averageOpacity, OPACITY_TRANSPARENT_F);
}

void KisRenderedDabContractTest::deviceConstructionCapturesOffsetAndRetainsOwnership()
{
    resetFixedDeviceObservations();
    const QRect initialBounds(-9, 11, 23, 29);
    KisFixedPaintDevice *const deviceAddress = new KisFixedPaintDevice(nullptr);
    deviceAddress->setRect(initialBounds);

    {
        KisFixedPaintDeviceSP device(deviceAddress);
        QCOMPARE(deviceAddress->refCount(), 1);

        KisRenderedDab dab(device);
        QCOMPARE(deviceAddress->refCount(), 2);
        QCOMPARE(dab.device.data(), deviceAddress);
        QCOMPARE(dab.offset, initialBounds.topLeft());
        QCOMPARE(fixedDeviceBoundsReads, 1);

        device.clear();
        QCOMPARE(deviceAddress->refCount(), 1);
        QCOMPARE(dab.realBounds(), initialBounds);
        QCOMPARE(fixedDeviceBoundsReads, 2);
        QCOMPARE(fixedDeviceDestructions, 0);
    }

    QCOMPARE(fixedDeviceDestructions, 1);
}

void KisRenderedDabContractTest::copyPreservesMembersAndRealBoundsUsesCurrentDeviceSize()
{
    resetFixedDeviceObservations();
    KisFixedPaintDevice *const deviceAddress = new KisFixedPaintDevice(nullptr);
    deviceAddress->setRect(QRect(3, 5, 7, 11));

    {
        KisRenderedDab original{KisFixedPaintDeviceSP(deviceAddress)};
        original.offset = QPoint(-13, 17);
        original.opacity = 0.25;
        original.flow = 0.5;
        original.averageOpacity = 0.75;

        {
            KisRenderedDab copy(original);
            QCOMPARE(deviceAddress->refCount(), 2);
            QCOMPARE(copy.device.data(), deviceAddress);
            QCOMPARE(copy.offset, QPoint(-13, 17));
            QCOMPARE(copy.opacity, 0.25);
            QCOMPARE(copy.flow, 0.5);
            QCOMPARE(copy.averageOpacity, 0.75);

            original.offset = QPoint(100, 200);
            original.opacity = 0.9;
            deviceAddress->setRect(QRect(101, 103, 19, 23));

            QCOMPARE(copy.offset, QPoint(-13, 17));
            QCOMPARE(copy.opacity, 0.25);
            QCOMPARE(copy.realBounds(), QRect(QPoint(-13, 17), QSize(19, 23)));
            QCOMPARE(fixedDeviceBoundsReads, 2);
        }

        QCOMPARE(deviceAddress->refCount(), 1);
        QCOMPARE(fixedDeviceDestructions, 0);
    }

    QCOMPARE(fixedDeviceDestructions, 1);
}

QTEST_GUILESS_MAIN(KisRenderedDabContractTest)

#include "KisRenderedDabContractTest.moc"
