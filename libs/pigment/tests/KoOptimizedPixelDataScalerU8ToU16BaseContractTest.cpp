/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoOptimizedPixelDataScalerU8ToU16Base.h>

#include <QTest>

#include <array>
#include <memory>

class RecordingPixelDataScaler : public KoOptimizedPixelDataScalerU8ToU16Base
{
public:
    explicit RecordingPixelDataScaler(int channelsPerPixel, int *destructionCount = nullptr)
        : KoOptimizedPixelDataScalerU8ToU16Base(channelsPerPixel)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingPixelDataScaler() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void convertU8ToU16(const quint8 *src, int srcRowStride, quint8 *dst, int dstRowStride, int numRows, int numColumns)
        const override
    {
        ++u8ToU16CallCount;
        u8ToU16Source = src;
        u8ToU16SourceStride = srcRowStride;
        u8ToU16Destination = dst;
        u8ToU16DestinationStride = dstRowStride;
        u8ToU16Rows = numRows;
        u8ToU16Columns = numColumns;

        dst[0] = static_cast<quint8>(src[0] + 1);
        dst[dstRowStride] = static_cast<quint8>(src[srcRowStride] + 2);
    }

    void convertU16ToU8(const quint8 *src, int srcRowStride, quint8 *dst, int dstRowStride, int numRows, int numColumns)
        const override
    {
        ++u16ToU8CallCount;
        u16ToU8Source = src;
        u16ToU8SourceStride = srcRowStride;
        u16ToU8Destination = dst;
        u16ToU8DestinationStride = dstRowStride;
        u16ToU8Rows = numRows;
        u16ToU8Columns = numColumns;

        dst[0] = static_cast<quint8>(src[0] - 1);
        dst[dstRowStride] = static_cast<quint8>(src[srcRowStride] - 2);
    }

    mutable int u8ToU16CallCount = 0;
    mutable const quint8 *u8ToU16Source = nullptr;
    mutable int u8ToU16SourceStride = -1;
    mutable quint8 *u8ToU16Destination = nullptr;
    mutable int u8ToU16DestinationStride = -1;
    mutable int u8ToU16Rows = -1;
    mutable int u8ToU16Columns = -1;

    mutable int u16ToU8CallCount = 0;
    mutable const quint8 *u16ToU8Source = nullptr;
    mutable int u16ToU8SourceStride = -1;
    mutable quint8 *u16ToU8Destination = nullptr;
    mutable int u16ToU8DestinationStride = -1;
    mutable int u16ToU8Rows = -1;
    mutable int u16ToU8Columns = -1;

private:
    int *m_destructionCount;
};

class KoOptimizedPixelDataScalerU8ToU16BaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void protocolPreservesChannelCountAndConversionArguments();
    void baseOwnershipDestroysDerivedExactlyOnce();
};

void KoOptimizedPixelDataScalerU8ToU16BaseContractTest::protocolPreservesChannelCountAndConversionArguments()
{
    RecordingPixelDataScaler scaler(4);
    const KoOptimizedPixelDataScalerU8ToU16Base &interface = scaler;

    QCOMPARE(interface.channelsPerPixel(), 4);

    std::array<quint8, 32> u8Source{};
    std::array<quint8, 32> u16Destination{};
    u8Source[0] = 17;
    u8Source[7] = 29;

    interface.convertU8ToU16(u8Source.data(), 7, u16Destination.data(), 13, 2, 3);

    QCOMPARE(scaler.u8ToU16CallCount, 1);
    QCOMPARE(scaler.u8ToU16Source, u8Source.data());
    QCOMPARE(scaler.u8ToU16SourceStride, 7);
    QCOMPARE(scaler.u8ToU16Destination, u16Destination.data());
    QCOMPARE(scaler.u8ToU16DestinationStride, 13);
    QCOMPARE(scaler.u8ToU16Rows, 2);
    QCOMPARE(scaler.u8ToU16Columns, 3);
    QCOMPARE(u16Destination[0], quint8(18));
    QCOMPARE(u16Destination[13], quint8(31));

    std::array<quint8, 32> u16Source{};
    std::array<quint8, 32> u8Destination{};
    u16Source[0] = 73;
    u16Source[12] = 91;

    interface.convertU16ToU8(u16Source.data(), 12, u8Destination.data(), 5, 2, 4);

    QCOMPARE(scaler.u16ToU8CallCount, 1);
    QCOMPARE(scaler.u16ToU8Source, u16Source.data());
    QCOMPARE(scaler.u16ToU8SourceStride, 12);
    QCOMPARE(scaler.u16ToU8Destination, u8Destination.data());
    QCOMPARE(scaler.u16ToU8DestinationStride, 5);
    QCOMPARE(scaler.u16ToU8Rows, 2);
    QCOMPARE(scaler.u16ToU8Columns, 4);
    QCOMPARE(u8Destination[0], quint8(72));
    QCOMPARE(u8Destination[5], quint8(89));
}

void KoOptimizedPixelDataScalerU8ToU16BaseContractTest::baseOwnershipDestroysDerivedExactlyOnce()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KoOptimizedPixelDataScalerU8ToU16Base> scaler =
            std::make_unique<RecordingPixelDataScaler>(3, &destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoOptimizedPixelDataScalerU8ToU16BaseContractTest)

#include "KoOptimizedPixelDataScalerU8ToU16BaseContractTest.moc"
