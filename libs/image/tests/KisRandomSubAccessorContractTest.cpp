/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_random_sub_accessor.h"

#include "KisRandomSubAccessorPaintDeviceAccess_p.h"

#include <QTest>

#include <array>
#include <vector>

namespace
{

struct MixCall {
    std::array<const quint8 *, 4> pixels{};
    std::array<qint16, 4> weights{};
    int numColors = 0;
    quint8 *destination = nullptr;
    int weightSum = 0;
};

struct PaintDeviceToken {
    int references = 0;
    int accessorCreations = 0;
    int accessorDestructions = 0;
    int referencesAtAccessorDestruction = -1;
    int finalReleases = 0;
    quint8 mixedValue = 0x7d;
    std::vector<QPoint> moves;
    std::vector<MixCall> mixes;
    std::array<std::array<quint8, 2>, 4> oldPixels{{
        {{0x10, 0x11}},
        {{0x20, 0x21}},
        {{0x30, 0x31}},
        {{0x40, 0x41}},
    }};
    std::array<std::array<quint8, 2>, 4> currentPixels{{
        {{0xa0, 0xa1}},
        {{0xb0, 0xb1}},
        {{0xc0, 0xc1}},
        {{0xd0, 0xd1}},
    }};
};

KisPaintDevice *paintDevicePointer(PaintDeviceToken *token)
{
    return reinterpret_cast<KisPaintDevice *>(token);
}

PaintDeviceToken *paintDeviceToken(KisPaintDevice *device)
{
    return reinterpret_cast<PaintDeviceToken *>(device);
}

const PaintDeviceToken *paintDeviceToken(const KisPaintDevice *device)
{
    return reinterpret_cast<const PaintDeviceToken *>(device);
}

KisPaintDeviceSP sharedPaintDevice(PaintDeviceToken *token)
{
    return KisPaintDeviceSP(paintDevicePointer(token));
}

class RandomConstAccessorSentinel final : public KisRandomConstAccessorNG
{
public:
    explicit RandomConstAccessorSentinel(PaintDeviceToken *device)
        : m_device(device)
    {
    }

    ~RandomConstAccessorSentinel() override
    {
        ++m_device->accessorDestructions;
        m_device->referencesAtAccessorDestruction = m_device->references;
    }

    void moveTo(qint32 x, qint32 y) override
    {
        m_x = x;
        m_y = y;
        m_device->moves.emplace_back(x, y);
    }

    qint32 numContiguousColumns(qint32) const override
    {
        return 1;
    }

    qint32 numContiguousRows(qint32) const override
    {
        return 1;
    }

    qint32 rowStride(qint32, qint32) const override
    {
        return 2;
    }

    const quint8 *oldRawData() const override
    {
        return m_device->oldPixels[currentPixelIndex()].data();
    }

    const quint8 *rawDataConst() const override
    {
        return m_device->currentPixels[currentPixelIndex()].data();
    }

    qint32 x() const override
    {
        return m_x;
    }

    qint32 y() const override
    {
        return m_y;
    }

private:
    std::size_t currentPixelIndex() const
    {
        return (m_device->moves.size() - 1) % m_device->oldPixels.size();
    }

private:
    PaintDeviceToken *m_device;
    qint32 m_x = 0;
    qint32 m_y = 0;
};

void compareMoves(const PaintDeviceToken &device, const std::array<QPoint, 4> &expected)
{
    QCOMPARE(device.moves.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QCOMPARE(device.moves[i], expected[i]);
    }
}

void compareWeights(const MixCall &call, const std::array<qint16, 4> &expected)
{
    QCOMPARE(call.numColors, 4);
    QCOMPARE(call.weightSum, 256);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        QCOMPARE(call.weights[i], expected[i]);
    }
}

} // namespace

void kisSharedPtrAddReference(KisPaintDevice *device)
{
    ++paintDeviceToken(device)->references;
}

bool kisSharedPtrRelease(KisPaintDevice *device)
{
    PaintDeviceToken *const token = paintDeviceToken(device);
    --token->references;
    if (token->references == 0) {
        ++token->finalReleases;
    }
    return true;
}

KisRandomConstAccessorSP kisRandomSubAccessorCreateRandomConstAccessor(const KisPaintDevice *device)
{
    PaintDeviceToken *const token = const_cast<PaintDeviceToken *>(paintDeviceToken(device));
    ++token->accessorCreations;
    return new RandomConstAccessorSentinel(token);
}

void kisRandomSubAccessorMixColors(const KisPaintDevice *device,
                                   const quint8 **pixels,
                                   const qint16 *weights,
                                   int numColors,
                                   quint8 *destination,
                                   int weightSum)
{
    PaintDeviceToken *const token = const_cast<PaintDeviceToken *>(paintDeviceToken(device));
    MixCall call;
    call.numColors = numColors;
    call.destination = destination;
    call.weightSum = weightSum;
    for (int i = 0; i < numColors; ++i) {
        call.pixels[std::size_t(i)] = pixels[i];
        call.weights[std::size_t(i)] = weights[i];
    }
    token->mixes.push_back(call);
    *destination = token->mixedValue;
}

class KisRandomSubAccessorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultPositionRetainsDeviceAndSamplesFourNeighbors();
    void moveOverloadsUseFloorCoordinatesAndBilinearWeights();
    void oldAndCurrentSamplingUseSeparatePixelViews();
    void finalReleaseDestroysAccessorAndReleasesDevice();
};

void KisRandomSubAccessorContractTest::defaultPositionRetainsDeviceAndSamplesFourNeighbors()
{
    PaintDeviceToken device;
    quint8 destination = 0;
    {
        KisPaintDeviceSP callerDevice = sharedPaintDevice(&device);
        KisRandomSubAccessor accessor(callerDevice);

        callerDevice.clear();
        QCOMPARE(device.references, 1);
        QCOMPARE(device.accessorCreations, 1);

        accessor.sampledRawData(&destination);

        compareMoves(device, {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}});
        QCOMPARE(device.mixes.size(), std::size_t(1));
        QCOMPARE(device.mixes.front().destination, &destination);
        QCOMPARE(destination, device.mixedValue);
    }

    QCOMPARE(device.accessorDestructions, 1);
    QCOMPARE(device.references, 0);
    QCOMPARE(device.finalReleases, 1);
}

void KisRandomSubAccessorContractTest::moveOverloadsUseFloorCoordinatesAndBilinearWeights()
{
    PaintDeviceToken device;
    KisRandomSubAccessor accessor(sharedPaintDevice(&device));
    quint8 destination = 0;

    accessor.moveTo(10.25, 20.5);
    accessor.sampledRawData(&destination);

    compareMoves(device, {{{10, 20}, {11, 20}, {10, 21}, {11, 21}}});
    QCOMPARE(device.mixes.size(), std::size_t(1));
    compareWeights(device.mixes.front(), {{96, 32, 96, 32}});

    device.moves.clear();
    accessor.moveTo(QPointF(-1.25, -2.5));
    accessor.sampledRawData(&destination);

    compareMoves(device, {{{-2, -3}, {-1, -3}, {-2, -2}, {-1, -2}}});
    QCOMPARE(device.mixes.size(), std::size_t(2));
    compareWeights(device.mixes.back(), {{32, 96, 32, 96}});
}

void KisRandomSubAccessorContractTest::oldAndCurrentSamplingUseSeparatePixelViews()
{
    PaintDeviceToken device;
    KisRandomSubAccessor accessor(sharedPaintDevice(&device));
    accessor.moveTo(3.5, 4.25);
    quint8 destination = 0;

    accessor.sampledOldRawData(&destination);
    accessor.sampledRawData(&destination);

    QCOMPARE(device.mixes.size(), std::size_t(2));
    for (std::size_t i = 0; i < device.oldPixels.size(); ++i) {
        QCOMPARE(device.mixes[0].pixels[i], device.oldPixels[i].data());
        QCOMPARE(device.mixes[1].pixels[i], device.currentPixels[i].data());
        QVERIFY(device.mixes[0].pixels[i] != device.mixes[1].pixels[i]);
    }
}

void KisRandomSubAccessorContractTest::finalReleaseDestroysAccessorAndReleasesDevice()
{
    PaintDeviceToken device;
    KisPaintDeviceSP callerDevice = sharedPaintDevice(&device);
    KisRandomSubAccessorSP firstOwner = new KisRandomSubAccessor(callerDevice);
    KisRandomSubAccessorSP finalOwner(firstOwner);

    callerDevice.clear();
    firstOwner.clear();
    QCOMPARE(device.accessorDestructions, 0);
    QVERIFY(device.references > 0);

    finalOwner.clear();
    QCOMPARE(device.accessorDestructions, 1);
    QCOMPARE(device.referencesAtAccessorDestruction, 1);
    QCOMPARE(device.references, 0);
    QCOMPARE(device.finalReleases, 1);
}

QTEST_GUILESS_MAIN(KisRandomSubAccessorContractTest)

#include "KisRandomSubAccessorContractTest.moc"
