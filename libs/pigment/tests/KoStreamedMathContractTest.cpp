/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "KoStreamedMath.h"

#include <algorithm>
#include <array>
#include <type_traits>

namespace
{

using Architecture = xsimd::current_arch;
using Math = KoStreamedMath<Architecture>;
using FloatBatch = Math::float_v;
using IntBatch = Math::int_v;
using UintBatch = Math::uint_v;

template<typename Batch>
std::array<typename Batch::value_type, Batch::size> batchValues(const Batch &batch)
{
    std::array<typename Batch::value_type, Batch::size> values{};
    batch.store_unaligned(values.data());
    return values;
}

template<typename Batch>
Batch increasingBatch(typename Batch::value_type first, typename Batch::value_type increment = 1)
{
    std::array<typename Batch::value_type, Batch::size> values{};
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = first + typename Batch::value_type(i) * increment;
    }
    return Batch::load_unaligned(values.data());
}

template<typename Actual, typename Expected>
void compareBatch(const Actual &actual, const Expected &expected, qreal tolerance = 1e-5)
{
    const auto actualValues = batchValues(actual);
    const auto expectedValues = batchValues(expected);
    for (std::size_t i = 0; i < actualValues.size(); ++i) {
        QVERIFY(qAbs(qreal(actualValues[i]) - qreal(expectedValues[i])) <= tolerance);
    }
}

} // namespace

class KoStreamedMathContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scalarAndSimdRoundAndDivisionPreserveValues();
    void maskAlphaAndChannelsRoundTripExactBytes();
    void pixelWrappersPreserveChannelPolicies();
    void pixelRecoveryRestoresOnlySelectedFloatLanes();
};

void KoStreamedMathContractTest::scalarAndSimdRoundAndDivisionPreserveValues()
{
    static_assert(std::is_same_v<Math::float_v, FloatBatch>);
    static_assert(std::is_same_v<Math::int_v, IntBatch>);
    static_assert(std::is_same_v<Math::uint_v, UintBatch>);
    static_assert(std::is_same_v<OptiDiv<Architecture>::float_v, FloatBatch>);
    static_assert(std::is_same_v<PixelStateRecoverHelper<float, Architecture>::float_v, FloatBatch>);
    static_assert(
        std::is_same_v<PixelStateRecoverHelper<float, Architecture>::float_m, typename FloatBatch::batch_bool_type>);
    static_assert(std::is_same_v<PixelWrapper<float, Architecture>::float_v, FloatBatch>);
    static_assert(std::is_same_v<PixelWrapper<float, Architecture>::int_v, IntBatch>);
    static_assert(std::is_same_v<PixelWrapper<float, Architecture>::uint_v, UintBatch>);
    static_assert(std::is_class_v<Math>);
    static_assert(std::is_class_v<OptiDiv<Architecture>>);
    static_assert(std::is_class_v<OptiRound<Architecture, int>>);
    static_assert(std::is_class_v<PixelStateRecoverHelper<float, Architecture>>);
    static_assert(std::is_class_v<PixelWrapper<float, Architecture>>);
    static_assert(std::is_class_v<PixelWrapper<float, Architecture>::Pixel>);

    QCOMPARE((OptiRound<Architecture, int>::roundScalar(1.4f)), 1);
    QCOMPARE((OptiRound<Architecture, int>::roundScalar(1.6f)), 2);
    QCOMPARE((OptiRound<Architecture, int>::roundScalar(-1.6f)), -2);
    QVERIFY(qAbs(OptiDiv<Architecture>::divScalar(9.0f, 4.0f) - 2.25f) < 0.01f);

    const FloatBatch dividend = increasingBatch<FloatBatch>(8.0f, 4.0f);
    const FloatBatch divisor(4.0f);
    compareBatch(OptiDiv<Architecture>::divVector(dividend, divisor), dividend / divisor, 0.01);
}

void KoStreamedMathContractTest::maskAlphaAndChannelsRoundTripExactBytes()
{
    std::array<quint8, 8> source{1, 2, 3, 4, 5, 6, 7, 8};
    std::array<quint8, 8> copied{};
    KoStreamedMathFunctions::copyPixel<8>(source.data(), copied.data());
    QCOMPARE(copied, source);
    KoStreamedMathFunctions::clearPixel<8>(copied.data());
    QVERIFY(std::all_of(copied.cbegin(), copied.cend(), [](quint8 value) {
        return value == 0;
    }));

    alignas(64) std::array<quint32, FloatBatch::size> pixels{};
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = (quint32(40 + i) << 24) | (quint32(10 + i) << 16) | (quint32(20 + i) << 8) | quint32(30 + i);
    }

    const auto alpha = Math::template fetch_alpha_32<true>(pixels.data());
    FloatBatch red;
    FloatBatch green;
    FloatBatch blue;
    Math::template fetch_colors_32<true>(pixels.data(), red, green, blue);
    const auto alphaValues = batchValues(alpha);
    const auto redValues = batchValues(red);
    const auto greenValues = batchValues(green);
    const auto blueValues = batchValues(blue);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        QCOMPARE(alphaValues[i], float(40 + i));
        QCOMPARE(redValues[i], float(10 + i));
        QCOMPARE(greenValues[i], float(20 + i));
        QCOMPARE(blueValues[i], float(30 + i));
    }

    alignas(64) std::array<quint32, FloatBatch::size> alignedResult{};
    std::array<quint32, FloatBatch::size + 1> unalignedStorage{};
    Math::write_channels_32(alignedResult.data(), alpha, red, green, blue);
    Math::write_channels_32_unaligned(unalignedStorage.data() + 1, alpha, red, green, blue);
    QCOMPARE(alignedResult, pixels);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        QCOMPARE(unalignedStorage[i + 1], pixels[i]);
    }

    std::array<quint8, FloatBatch::size> masks{};
    for (std::size_t i = 0; i < masks.size(); ++i) {
        masks[i] = quint8(3 + i);
    }
    const auto fetchedMask = batchValues(Math::fetch_mask_8(masks.data()));
    for (std::size_t i = 0; i < masks.size(); ++i) {
        QCOMPARE(fetchedMask[i], float(masks[i]));
    }
    QCOMPARE(Math::round_float_to_u8(12.6f), quint8(13));
    QCOMPARE(Math::lerp_mixed_u8_float(10, 30, 0.25f), quint8(15));
}

void KoStreamedMathContractTest::pixelWrappersPreserveChannelPolicies()
{
    PixelWrapper<quint8, Architecture> bytes;
    QCOMPARE(batchValues(bytes.mask).front(), quint32(0xFF));
    QCOMPARE(batchValues(bytes.uint8Max).front(), 255.0f);
    QVERIFY(qAbs(batchValues(bytes.uint8Rec1).front() - 1.0f / 255.0f) < 1e-7f);
    QCOMPARE(bytes.lerpMixedUintFloat(10, 30, 0.25f), quint8(15));
    QCOMPARE(bytes.roundFloatToUint(12.6f), quint8(13));
    float byteAlpha = 127.5f;
    bytes.normalizeAlpha(byteAlpha);
    QVERIFY(qAbs(byteAlpha - 0.5f) < 1e-6f);
    bytes.denormalizeAlpha(byteAlpha);
    QVERIFY(qAbs(byteAlpha - 127.5f) < 1e-4f);

    std::array<quint8, FloatBatch::size * 4> bytePixels{};
    for (std::size_t i = 0; i < FloatBatch::size; ++i) {
        bytePixels[4 * i] = quint8(10 + i);
        bytePixels[4 * i + 1] = quint8(20 + i);
        bytePixels[4 * i + 2] = quint8(30 + i);
        bytePixels[4 * i + 3] = quint8(40 + i);
    }
    FloatBatch c1;
    FloatBatch c2;
    FloatBatch c3;
    FloatBatch a;
    bytes.read(bytePixels.data(), c1, c2, c3, a);
    std::array<quint8, FloatBatch::size * 4> byteRoundTrip{};
    bytes.write(byteRoundTrip.data(), c1, c2, c3, a);
    QCOMPARE(byteRoundTrip, bytePixels);
    std::array<quint8, FloatBatch::size * 4> byteCopy{};
    bytes.copyPixels(bytePixels.data(), byteCopy.data());
    QCOMPARE(byteCopy, bytePixels);
    bytes.clearPixels(byteCopy.data());
    QVERIFY(std::all_of(byteCopy.cbegin(), byteCopy.cend(), [](quint8 value) {
        return value == 0;
    }));

    PixelWrapper<quint16, Architecture> words;
    QCOMPARE(batchValues(words.mask).front(), quint32(0xFFFF));
    QCOMPARE(batchValues(words.uint16Max).front(), 65535.0f);
    QVERIFY(qAbs(batchValues(words.uint16Rec1).front() - 1.0f / 65535.0f) < 1e-9f);
    QCOMPARE(words.lerpMixedUintFloat(quint16(1000), quint16(3000), 0.25f), quint16(1500));
    QCOMPARE(words.roundFloatToUint(12.6f), quint16(13));
    float wordAlpha = 32767.5f;
    words.normalizeAlpha(wordAlpha);
    QVERIFY(qAbs(wordAlpha - 0.5f) < 1e-6f);
    words.denormalizeAlpha(wordAlpha);
    QVERIFY(qAbs(wordAlpha - 32767.5f) < 0.01f);

    std::array<quint16, FloatBatch::size * 4> wordPixels{};
    for (std::size_t i = 0; i < FloatBatch::size; ++i) {
        wordPixels[4 * i] = quint16(1000 + i);
        wordPixels[4 * i + 1] = quint16(2000 + i);
        wordPixels[4 * i + 2] = quint16(3000 + i);
        wordPixels[4 * i + 3] = quint16(4000 + i);
    }
    words.read(wordPixels.data(), c1, c2, c3, a);
    std::array<quint16, FloatBatch::size * 4> wordRoundTrip{};
    words.write(wordRoundTrip.data(), c1, c2, c3, a);
    QCOMPARE(wordRoundTrip, wordPixels);
    std::array<quint8, sizeof(wordPixels)> wordCopy{};
    words.copyPixels(reinterpret_cast<const quint8 *>(wordPixels.data()), wordCopy.data());
    QCOMPARE(memcmp(wordCopy.data(), wordPixels.data(), sizeof(wordPixels)), 0);
    words.clearPixels(wordCopy.data());
    QVERIFY(std::all_of(wordCopy.cbegin(), wordCopy.cend(), [](quint8 value) {
        return value == 0;
    }));

    PixelWrapper<float, Architecture> floats;
    PixelWrapper<float, Architecture>::Pixel pixel{1.0f, 2.0f, 3.0f, 0.5f};
    QCOMPARE(pixel.red, 1.0f);
    QCOMPARE(pixel.green, 2.0f);
    QCOMPARE(pixel.blue, 3.0f);
    QCOMPARE(pixel.alpha, 0.5f);
    QCOMPARE(floats.lerpMixedUintFloat(10.0f, 30.0f, 0.25f), 15.0f);
    QCOMPARE(floats.roundFloatToUint(12.6f), 12.6f);
    float floatAlpha = 0.5f;
    floats.normalizeAlpha(floatAlpha);
    floats.denormalizeAlpha(floatAlpha);
    QCOMPARE(floatAlpha, 0.5f);

    std::array<float, FloatBatch::size * 4> floatPixels{};
    for (std::size_t i = 0; i < FloatBatch::size; ++i) {
        floatPixels[4 * i] = float(1 + i);
        floatPixels[4 * i + 1] = float(11 + i);
        floatPixels[4 * i + 2] = float(21 + i);
        floatPixels[4 * i + 3] = float(31 + i);
    }
    floats.read(floatPixels.data(), c1, c2, c3, a);
    std::array<float, FloatBatch::size * 4> floatRoundTrip{};
    floats.write(floatRoundTrip.data(), c1, c2, c3, a);
    QCOMPARE(floatRoundTrip, floatPixels);
    std::array<quint8, sizeof(floatPixels)> floatCopy{};
    floats.copyPixels(reinterpret_cast<const quint8 *>(floatPixels.data()), floatCopy.data());
    QCOMPARE(memcmp(floatCopy.data(), floatPixels.data(), sizeof(floatPixels)), 0);
    floats.clearPixels(floatCopy.data());
    QVERIFY(std::all_of(floatCopy.cbegin(), floatCopy.cend(), [](quint8 value) {
        return value == 0;
    }));
}

void KoStreamedMathContractTest::pixelRecoveryRestoresOnlySelectedFloatLanes()
{
    const FloatBatch original1 = increasingBatch<FloatBatch>(1.0f);
    const FloatBatch original2 = increasingBatch<FloatBatch>(11.0f);
    const FloatBatch original3 = increasingBatch<FloatBatch>(21.0f);
    PixelStateRecoverHelper<float, Architecture> recover(original1, original2, original3);

    FloatBatch changed1(101.0f);
    FloatBatch changed2(102.0f);
    FloatBatch changed3(103.0f);
    const auto mask = original1 < FloatBatch(3.0f);
    recover.recoverPixels(mask, changed1, changed2, changed3);
    const auto values1 = batchValues(changed1);
    const auto values2 = batchValues(changed2);
    const auto values3 = batchValues(changed3);
    for (std::size_t i = 0; i < FloatBatch::size; ++i) {
        const bool selected = i < 2;
        QCOMPARE(values1[i], selected ? float(1 + i) : 101.0f);
        QCOMPARE(values2[i], selected ? float(11 + i) : 102.0f);
        QCOMPARE(values3[i], selected ? float(21 + i) : 103.0f);
    }

    PixelStateRecoverHelper<quint8, Architecture> noRecovery(original1, original2, original3);
    FloatBatch untouched1(7.0f);
    FloatBatch untouched2(8.0f);
    FloatBatch untouched3(9.0f);
    noRecovery.recoverPixels(mask, untouched1, untouched2, untouched3);
    compareBatch(untouched1, FloatBatch(7.0f));
    compareBatch(untouched2, FloatBatch(8.0f));
    compareBatch(untouched3, FloatBatch(9.0f));
}

QTEST_GUILESS_MAIN(KoStreamedMathContractTest)

#include "KoStreamedMathContractTest.moc"
