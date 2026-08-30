/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoOptimizedPixelDataScalerU8ToU16Factory.h>

#include <QByteArray>
#include <QTest>

#include <array>
#include <cstring>
#include <memory>
#include <tuple>

std::tuple<bool, bool> vectorizationConfiguration()
{
    return {false, false};
}

class KoOptimizedPixelDataScalerU8ToU16FactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rgbaFactoryCreatesFourChannelScalerWithStridedConversion();
    void cmykaFactoryCreatesFiveChannelScalerWithStridedConversion();
};

namespace
{

void verifyScalerContract(std::unique_ptr<KoOptimizedPixelDataScalerU8ToU16Base> scaler, int channelsPerPixel)
{
    QVERIFY(scaler);
    QCOMPARE(scaler->channelsPerPixel(), channelsPerPixel);

    constexpr int rows = 2;
    constexpr int columns = 2;
    constexpr quint8 u16Padding = 0xa5;
    constexpr quint8 u8Padding = 0x5a;
    const int valuesPerRow = channelsPerPixel * columns;
    const int u8SourceStride = valuesPerRow + 3;
    const int u16DestinationStride = valuesPerRow * int(sizeof(quint16)) + 6;

    QByteArray u8Source(rows * u8SourceStride, char(0xcc));
    QByteArray u16Destination(rows * u16DestinationStride, char(u16Padding));
    const std::array<quint8, 20> u8Values{
        0, 1, 17, 63, 127, 128, 191, 200, 254, 255, 2, 18, 64, 126, 129, 190, 201, 253, 42, 213,
    };

    for (int row = 0; row < rows; ++row) {
        for (int index = 0; index < valuesPerRow; ++index) {
            u8Source[row * u8SourceStride + index] = char(u8Values[row * valuesPerRow + index]);
        }
    }
    const QByteArray originalU8Source = u8Source;

    scaler->convertU8ToU16(reinterpret_cast<const quint8 *>(u8Source.constData()),
                           u8SourceStride,
                           reinterpret_cast<quint8 *>(u16Destination.data()),
                           u16DestinationStride,
                           rows,
                           columns);

    QCOMPARE(u8Source, originalU8Source);
    for (int row = 0; row < rows; ++row) {
        for (int index = 0; index < valuesPerRow; ++index) {
            quint16 actual = 0;
            std::memcpy(&actual,
                        u16Destination.constData() + row * u16DestinationStride + index * int(sizeof(quint16)),
                        sizeof(actual));
            QCOMPARE(actual, quint16(u8Values[row * valuesPerRow + index]) * quint16(257));
        }
        for (int index = valuesPerRow * int(sizeof(quint16)); index < u16DestinationStride; ++index) {
            QCOMPARE(quint8(u16Destination[row * u16DestinationStride + index]), u16Padding);
        }
    }

    const int u16SourceStride = valuesPerRow * int(sizeof(quint16)) + 4;
    const int u8DestinationStride = valuesPerRow + 5;
    QByteArray u16Source(rows * u16SourceStride, char(0xcd));
    QByteArray u8Destination(rows * u8DestinationStride, char(u8Padding));
    const std::array<quint16, 20> u16Values{
        0,     1,     127,   128,   129,   255,   256,   32767, 32768, 32896,
        65000, 65279, 65280, 65407, 65408, 65534, 65535, 4242,  21930, 43860,
    };

    for (int row = 0; row < rows; ++row) {
        for (int index = 0; index < valuesPerRow; ++index) {
            const quint16 value = u16Values[row * valuesPerRow + index];
            std::memcpy(u16Source.data() + row * u16SourceStride + index * int(sizeof(quint16)), &value, sizeof(value));
        }
    }
    const QByteArray originalU16Source = u16Source;

    scaler->convertU16ToU8(reinterpret_cast<const quint8 *>(u16Source.constData()),
                           u16SourceStride,
                           reinterpret_cast<quint8 *>(u8Destination.data()),
                           u8DestinationStride,
                           rows,
                           columns);

    QCOMPARE(u16Source, originalU16Source);
    for (int row = 0; row < rows; ++row) {
        for (int index = 0; index < valuesPerRow; ++index) {
            const quint16 value = u16Values[row * valuesPerRow + index];
            const quint8 expected = quint8((value - (value >> 8) + 128) >> 8);
            QCOMPARE(quint8(u8Destination[row * u8DestinationStride + index]), expected);
        }
        for (int index = valuesPerRow; index < u8DestinationStride; ++index) {
            QCOMPARE(quint8(u8Destination[row * u8DestinationStride + index]), u8Padding);
        }
    }
}

} // namespace

void KoOptimizedPixelDataScalerU8ToU16FactoryContractTest::rgbaFactoryCreatesFourChannelScalerWithStridedConversion()
{
    verifyScalerContract(std::unique_ptr<KoOptimizedPixelDataScalerU8ToU16Base>(
                             KoOptimizedPixelDataScalerU8ToU16Factory::createRgbaScaler()),
                         4);
}

void KoOptimizedPixelDataScalerU8ToU16FactoryContractTest::cmykaFactoryCreatesFiveChannelScalerWithStridedConversion()
{
    verifyScalerContract(std::unique_ptr<KoOptimizedPixelDataScalerU8ToU16Base>(
                             KoOptimizedPixelDataScalerU8ToU16Factory::createCmykaScaler()),
                         5);
}

QTEST_GUILESS_MAIN(KoOptimizedPixelDataScalerU8ToU16FactoryContractTest)

#include "KoOptimizedPixelDataScalerU8ToU16FactoryContractTest.moc"
