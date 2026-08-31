/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestKoColorSpaceMaths.h"

#include "KoColorSpaceMaths.h"

#include <QTest>

#include <cmath>
#include <limits>
#include <type_traits>

namespace
{

bool closeReal(qreal actual, qreal expected, qreal epsilon = 1e-5)
{
    return qAbs(actual - expected) <= epsilon;
}

template<typename T>
qreal asReal(T value)
{
    return static_cast<qreal>(value);
}

} // namespace

void TestKoColorSpaceMaths::numericTraitsAndLutsExposeStableDomains()
{
    using Uint8Traits = KoColorSpaceMathsTraits<quint8>;
    using Uint16Traits = KoColorSpaceMathsTraits<quint16>;
    using Int16Traits = KoColorSpaceMathsTraits<qint16>;
    using Uint32Traits = KoColorSpaceMathsTraits<quint32>;
    using FloatTraits = KoColorSpaceMathsTraits<float>;
    using DoubleTraits = KoColorSpaceMathsTraits<double>;

    static_assert(std::is_same_v<Uint8Traits::compositetype, qint32>);
    static_assert(std::is_same_v<Uint8Traits::mixtype, qint64>);
    static_assert(std::is_same_v<Uint16Traits::compositetype, qint64>);
    static_assert(std::is_same_v<Int16Traits::compositetype, qint64>);
    static_assert(std::is_same_v<Uint32Traits::compositetype, qint64>);
    static_assert(std::is_same_v<FloatTraits::compositetype, float>);
    static_assert(std::is_same_v<FloatTraits::mixtype, double>);
    static_assert(std::is_same_v<DoubleTraits::compositetype, double>);
    static_assert(std::is_same_v<DoubleTraits::mixtype, double>);

    QCOMPARE(Uint8Traits::zeroValue, quint8(0));
    QCOMPARE(Uint8Traits::halfValue, quint8(127));
    QCOMPARE(Uint8Traits::unitValue, quint8(255));
    QCOMPARE(Uint8Traits::min, quint8(0));
    QCOMPARE(Uint8Traits::max, quint8(255));
    QCOMPARE(Uint8Traits::epsilon, quint8(1));
    QCOMPARE(Uint8Traits::bits, qint8(8));
    QCOMPARE(Uint8Traits::channelValueType, KoChannelInfo::UINT8);

    QCOMPARE(Uint16Traits::zeroValue, quint16(0));
    QCOMPARE(Uint16Traits::halfValue, quint16(32767));
    QCOMPARE(Uint16Traits::unitValue, quint16(65535));
    QCOMPARE(Uint16Traits::max, quint16(65535));
    QCOMPARE(Uint16Traits::bits, qint8(16));
    QCOMPARE(Uint16Traits::channelValueType, KoChannelInfo::UINT16);

    QCOMPARE(Int16Traits::zeroValue, qint16(0));
    QCOMPARE(Int16Traits::unitValue, qint16(32767));
    QCOMPARE(Int16Traits::min, std::numeric_limits<qint16>::min());
    QCOMPARE(Int16Traits::max, std::numeric_limits<qint16>::max());
    QCOMPARE(Int16Traits::channelValueType, KoChannelInfo::INT16);

    QCOMPARE(Uint32Traits::zeroValue, quint32(0));
    QCOMPARE(Uint32Traits::unitValue, std::numeric_limits<quint32>::max());
    QCOMPARE(Uint32Traits::bits, qint8(32));
    QCOMPARE(Uint32Traits::channelValueType, KoChannelInfo::UINT32);

    QCOMPARE(FloatTraits::zeroValue, 0.0f);
    QCOMPARE(FloatTraits::halfValue, 0.5f);
    QCOMPARE(FloatTraits::unitValue, 1.0f);
    QCOMPARE(FloatTraits::min, -std::numeric_limits<float>::max());
    QCOMPARE(FloatTraits::max, std::numeric_limits<float>::max());
    QCOMPARE(FloatTraits::epsilon, std::numeric_limits<float>::epsilon());
    QCOMPARE(FloatTraits::bits, qint8(32));
    QCOMPARE(FloatTraits::channelValueType, KoChannelInfo::FLOAT32);

    QCOMPARE(DoubleTraits::zeroValue, 0.0);
    QCOMPARE(DoubleTraits::halfValue, 0.5);
    QCOMPARE(DoubleTraits::unitValue, 1.0);
    QCOMPARE(DoubleTraits::epsilon, std::numeric_limits<double>::epsilon());
    QCOMPARE(DoubleTraits::bits, qint8(64));
    QCOMPARE(DoubleTraits::channelValueType, KoChannelInfo::FLOAT64);

#ifdef HAVE_OPENEXR
    using HalfTraits = KoColorSpaceMathsTraits<half>;
    static_assert(std::is_same_v<HalfTraits::compositetype, double>);
    static_assert(std::is_same_v<HalfTraits::mixtype, double>);
    QVERIFY(closeReal(asReal(HalfTraits::zeroValue), 0.0));
    QVERIFY(closeReal(asReal(HalfTraits::halfValue), 0.5));
    QVERIFY(closeReal(asReal(HalfTraits::unitValue), 1.0));
    QVERIFY(HalfTraits::min < HalfTraits::zeroValue);
    QVERIFY(HalfTraits::max > HalfTraits::unitValue);
    QVERIFY(HalfTraits::epsilon > HalfTraits::zeroValue);
    QCOMPARE(HalfTraits::bits, qint8(16));
    QCOMPARE(HalfTraits::channelValueType, KoChannelInfo::FLOAT16);
#endif

    const KoIntegerToFloat<quint8> uint8Converter;
    QVERIFY(closeReal(uint8Converter(0), 0.0));
    QVERIFY(closeReal(uint8Converter(128), 0.5019608));
    QVERIFY(closeReal(uint8Converter(255), 1.0));

    QVERIFY(closeReal(KoLuts::Uint8ToFloat(0), 0.0));
    QVERIFY(closeReal(KoLuts::Uint8ToFloat(128), 0.5019608));
    QVERIFY(closeReal(KoLuts::Uint8ToFloat(255), 1.0));
    QVERIFY(closeReal(KoLuts::Uint16ToFloat(0), 0.0));
    QVERIFY(closeReal(KoLuts::Uint16ToFloat(32768), 0.5000076));
    QVERIFY(closeReal(KoLuts::Uint16ToFloat(65535), 1.0));
}

void TestKoColorSpaceMaths::coreMathPreservesConversionsAndSpecializedRounding()
{
    QCOMPARE(float2int(0.49f), 0);
    QCOMPARE(float2int(0.5f), 1);
    QCOMPARE(float2int(254.5), 255);

    QVERIFY(closeReal(KoColorSpaceMaths<float>::multiply(0.25f, 0.8f), 0.2));
    QVERIFY(closeReal(KoColorSpaceMaths<float>::multiply(0.25f, 0.8f, 0.5f), 0.1));
    QCOMPARE(KoColorSpaceMaths<quint8>::multiply(255, 128), quint8(128));
    QCOMPARE(KoColorSpaceMaths<quint8>::multiply(255, 128, 64), quint8(32));
    QCOMPARE(KoColorSpaceMaths<quint16>::multiply(65535, 32768), quint16(32768));

    QVERIFY(closeReal(KoColorSpaceMaths<float>::divide(0.25f, 0.5f), 0.5));
    QCOMPARE(KoColorSpaceMaths<quint8>::divide(64, 128), qint32(128));
    QCOMPARE(KoColorSpaceMaths<quint16>::divide(16384, 32768), qint64(32768));

    QVERIFY(closeReal(KoColorSpaceMaths<float>::invert(0.25f), 0.75));
    QCOMPARE(KoColorSpaceMaths<quint8>::invert(0x0f), quint8(0xf0));
    QCOMPARE(KoColorSpaceMaths<quint16>::invert(0x00ff), quint16(0xff00));

    QCOMPARE(KoColorSpaceMaths<quint16>::blend(65535, 0, 0), quint16(0));
    QCOMPARE(KoColorSpaceMaths<quint16>::blend(65535, 0, 65535), quint16(65535));
    QVERIFY(closeReal(KoColorSpaceMaths<float>::blend(0.8f, 0.2f, 0.25f), 0.35));
    QCOMPARE(KoColorSpaceMaths<quint8>::blend(200, 50, 0), quint8(50));
    QCOMPARE(KoColorSpaceMaths<quint8>::blend(200, 50, 255), quint8(200));
#ifdef HAVE_OPENEXR
    QVERIFY(closeReal(asReal(KoColorSpaceMaths<half>::blend(half(0.8f), half(0.2f), half(0.25f))), 0.35, 0.001));
#endif

    QCOMPARE((KoColorSpaceMaths<qint16, qint16>::scaleToA(1234)), qint16(1234));
    QCOMPARE((KoColorSpaceMaths<double, quint8>::scaleToA(0.5)), quint8(128));
    QVERIFY(closeReal(KoColorSpaceMaths<quint8, double>::scaleToA(128), 0.5019608));
    QCOMPARE((KoColorSpaceMaths<float, quint8>::scaleToA(0.5f)), quint8(128));
    QVERIFY(closeReal(KoColorSpaceMaths<quint8, float>::scaleToA(128), 0.5019608));
    QCOMPARE((KoColorSpaceMaths<quint8, quint16>::scaleToA(255)), quint16(65535));
    QCOMPARE((KoColorSpaceMaths<quint16, quint8>::scaleToA(65535)), quint8(255));
    QCOMPARE((KoColorSpaceMaths<quint16, quint16>::scaleToA(12345)), quint16(12345));
    QCOMPARE((KoColorSpaceMaths<float, float>::scaleToA(0.375f)), 0.375f);
#ifdef HAVE_OPENEXR
    QCOMPARE((KoColorSpaceMaths<half, quint8>::scaleToA(half(0.5f))), quint8(127));
    QVERIFY(closeReal(asReal(KoColorSpaceMaths<quint8, half>::scaleToA(128)), 0.5019608, 0.001));
#endif

    for (int value = 0; value < 256; ++value) {
        const quint16 expanded = KoColorSpaceMaths<quint8, quint16>::scaleToA(quint8(value));
        QCOMPARE((KoColorSpaceMaths<quint16, quint8>::scaleToA(expanded)), quint8(value));
    }
}

void TestKoColorSpaceMaths::boundsCompositeAndFuzzyPoliciesPreserveThresholds()
{
    QCOMPARE(KoColorSpaceMaths<float>:: xor (0.25f, 0.25f), 0.0f);
    QCOMPARE(KoColorSpaceMaths<float>::and(0.25f, 0.25f), 536870912.0f);
    QCOMPARE(KoColorSpaceMaths<float>:: or (0.25f, 0.25f), 536870912.0f);
    QVERIFY(closeReal(KoColorSpaceMaths<float>::modulus(0.75f, 0.5f), 0.25));

    QCOMPARE(KoColorSpaceMaths<qint16>::clamp(40000), qint64(32767));
    QCOMPARE(KoColorSpaceMaths<qint16>::clamp(-40000), qint64(-32768));
    QCOMPARE(KoColorSpaceMaths<float>::clamp(2.0f), 2.0f);
    QCOMPARE(KoColorSpaceMaths<double>::clamp(-2.0), -2.0);
    QCOMPARE(KoColorSpaceMaths<quint8>::clampAfterScale(300), quint8(255));

    QCOMPARE(KoColorSpaceMaths<float>::clampChannelToSDR(-0.25f), 0.0f);
    QCOMPARE(KoColorSpaceMaths<float>::clampChannelToSDR(1.25f), 1.0f);
    QCOMPARE(KoColorSpaceMaths<float>::clampChannelToSDRBottom(-0.25f), 0.0f);
    QCOMPARE(KoColorSpaceMaths<qint16>::clampChannelToSDR(-2), qint16(-2));
    QCOMPARE(KoColorSpaceMaths<qint16>::clampChannelToSDRBottom(-2), qint16(-2));

    QCOMPARE(KoColorSpaceMaths<qint16>::clampToSDR(-1), qint64(0));
    QCOMPARE(KoColorSpaceMaths<qint16>::clampToSDR(40000), qint64(32767));
    QCOMPARE(KoColorSpaceMaths<qint16>::clampToSDRTop(40000), qint64(32767));
    QCOMPARE(KoColorSpaceMaths<qint16>::clampToSDRBottom(-1), qint64(0));

    QCOMPARE(KoColorSpaceMaths<quint8>::divideInCompositeSpace(64, 128), qint32(127));
    QCOMPARE(KoColorSpaceMaths<quint8>::multiplyInCompositeSpace(128, 128), qint32(64));
    QVERIFY(closeReal(KoColorSpaceMaths<float>::divideInCompositeSpace(0.25f, 0.5f), 0.5));
    QVERIFY(closeReal(KoColorSpaceMaths<float>::multiplyInCompositeSpace(0.25f, 0.5f), 0.125));

    QVERIFY(KoColorSpaceMaths<quint8>::isUnsafeAsDivisor(0));
    QVERIFY(!KoColorSpaceMaths<quint8>::isUnsafeAsDivisor(1));
    QVERIFY(KoColorSpaceMaths<quint8>::isZeroValueFuzzy(0));
    QVERIFY(!KoColorSpaceMaths<quint8>::isZeroValueFuzzy(1));
    QVERIFY(KoColorSpaceMaths<quint8>::isUnitValueFuzzy(255));
    QVERIFY(!KoColorSpaceMaths<quint8>::isUnitValueFuzzy(254));
    QVERIFY(KoColorSpaceMaths<qint16>::isZeroValueClampedFuzzy(-1));
    QVERIFY(!KoColorSpaceMaths<qint16>::isZeroValueClampedFuzzy(1));
    QVERIFY(KoColorSpaceMaths<qint16>::isUnitValueClampedFuzzy(32767));
    QVERIFY(!KoColorSpaceMaths<qint16>::isUnitValueClampedFuzzy(32766));
    QVERIFY(KoColorSpaceMaths<quint8>::isHalfValueFuzzy(127));
    QVERIFY(!KoColorSpaceMaths<quint8>::isHalfValueFuzzy(128));

    QVERIFY(KoColorSpaceMaths<float>::isUnsafeAsDivisor(-1.0f));
    QVERIFY(KoColorSpaceMaths<float>::isUnsafeAsDivisor(0.0f));
    QVERIFY(!KoColorSpaceMaths<float>::isUnsafeAsDivisor(0.1f));
    QVERIFY(KoColorSpaceMaths<float>::isZeroValueFuzzy(0.0f));
    QVERIFY(!KoColorSpaceMaths<float>::isZeroValueFuzzy(0.001f));
    QVERIFY(KoColorSpaceMaths<float>::isUnitValueFuzzy(1.0f));
    QVERIFY(!KoColorSpaceMaths<float>::isUnitValueFuzzy(0.99f));
    QVERIFY(KoColorSpaceMaths<float>::isZeroValueClampedFuzzy(0.000001f));
    QVERIFY(!KoColorSpaceMaths<float>::isZeroValueClampedFuzzy(0.001f));
    QVERIFY(KoColorSpaceMaths<float>::isUnitValueClampedFuzzy(0.999999f));
    QVERIFY(!KoColorSpaceMaths<float>::isUnitValueClampedFuzzy(0.99f));
    QVERIFY(KoColorSpaceMaths<float>::isHalfValueFuzzy(0.5f));
    QVERIFY(!KoColorSpaceMaths<float>::isHalfValueFuzzy(0.49f));

    QVERIFY(KoColorSpaceMaths<double>::isUnsafeAsDivisor(-1.0));
    QVERIFY(!KoColorSpaceMaths<double>::isUnsafeAsDivisor(0.1));
    QVERIFY(KoColorSpaceMaths<double>::isZeroValueFuzzy(0.0));
    QVERIFY(!KoColorSpaceMaths<double>::isZeroValueFuzzy(0.001));
    QVERIFY(KoColorSpaceMaths<double>::isUnitValueFuzzy(1.0));
    QVERIFY(!KoColorSpaceMaths<double>::isUnitValueFuzzy(0.99));
    QVERIFY(KoColorSpaceMaths<double>::isZeroValueClampedFuzzy(1e-13));
    QVERIFY(!KoColorSpaceMaths<double>::isZeroValueClampedFuzzy(1e-6));
    QVERIFY(KoColorSpaceMaths<double>::isUnitValueClampedFuzzy(1.0 - 1e-13));
    QVERIFY(!KoColorSpaceMaths<double>::isUnitValueClampedFuzzy(0.99));
    QVERIFY(KoColorSpaceMaths<double>::isHalfValueFuzzy(0.5));
    QVERIFY(!KoColorSpaceMaths<double>::isHalfValueFuzzy(0.49));

#ifdef HAVE_OPENEXR
    QVERIFY(KoColorSpaceMaths<half>::isUnsafeAsDivisor(half(-1.0f)));
    QVERIFY(!KoColorSpaceMaths<half>::isUnsafeAsDivisor(half(0.1f)));
    QVERIFY(KoColorSpaceMaths<half>::isZeroValueFuzzy(half(0.001f)));
    QVERIFY(!KoColorSpaceMaths<half>::isZeroValueFuzzy(half(0.01f)));
    QVERIFY(KoColorSpaceMaths<half>::isUnitValueFuzzy(half(0.999f)));
    QVERIFY(!KoColorSpaceMaths<half>::isUnitValueFuzzy(half(0.99f)));
    QVERIFY(KoColorSpaceMaths<half>::isZeroValueClampedFuzzy(half(0.001f)));
    QVERIFY(!KoColorSpaceMaths<half>::isZeroValueClampedFuzzy(half(0.01f)));
    QVERIFY(KoColorSpaceMaths<half>::isUnitValueClampedFuzzy(half(0.999f)));
    QVERIFY(!KoColorSpaceMaths<half>::isUnitValueClampedFuzzy(half(0.99f)));
    QVERIFY(KoColorSpaceMaths<half>::isHalfValueFuzzy(half(0.5f)));
    QVERIFY(!KoColorSpaceMaths<half>::isHalfValueFuzzy(half(0.51f)));
#endif
}

void TestKoColorSpaceMaths::arithmeticFacadePreservesDelegationAndBounds()
{
    QVERIFY(closeReal(Arithmetic::pi, 3.141592653589793));
    QVERIFY(closeReal(Arithmetic::mul(0.25f, 0.8f), 0.2));
    QVERIFY(closeReal(Arithmetic::mul(0.25f, 0.8f, 0.5f), 0.1));
    QVERIFY(closeReal(Arithmetic::inv(0.25f), 0.75));
    QVERIFY(closeReal(Arithmetic::lerp(0.2f, 0.8f, 0.25f), 0.35));
    QCOMPARE(Arithmetic::scale<quint8>(0.5f), quint8(128));
    QVERIFY(closeReal(Arithmetic::div(0.25f, 0.5f), 0.5));
    QCOMPARE(Arithmetic:: xor (0.25f, 0.25f), 0.0f);
    QCOMPARE(Arithmetic::and(0.25f, 0.25f), 536870912.0f);
    QCOMPARE(Arithmetic:: or (0.25f, 0.25f), 536870912.0f);

    QCOMPARE(Arithmetic::clamp<qint16>(40000), qint16(32767));
    QCOMPARE(Arithmetic::clampChannelToSDR(-0.25f), 0.0f);
    QCOMPARE(Arithmetic::clampChannelToSDRBottom(-0.25f), 0.0f);
    QCOMPARE(Arithmetic::clampToSDR<qint16>(-1), qint16(0));
    QCOMPARE(Arithmetic::clampToSDRTop<qint16>(40000), qint16(32767));
    QCOMPARE(Arithmetic::clampToSDRBottom<qint16>(-1), qint16(0));
    QVERIFY(closeReal(Arithmetic::divideInCompositeSpace<float>(0.25f, 0.5f), 0.5));
    QVERIFY(closeReal(Arithmetic::multiplyInCompositeSpace<float>(0.25f, 0.5f), 0.125));

    QVERIFY(Arithmetic::isZeroValueFuzzy(0.0f));
    QVERIFY(Arithmetic::isUnitValueFuzzy(1.0f));
    QVERIFY(Arithmetic::isZeroValueClampedFuzzy(-0.1f));
    QVERIFY(Arithmetic::isUnitValueClampedFuzzy(1.1f));
    QVERIFY(Arithmetic::isHalfValueFuzzy(0.5f));
    QVERIFY(Arithmetic::isZeroValueStrict(0.0f));
    QVERIFY(!Arithmetic::isZeroValueStrict(0.000001f));
    QVERIFY(Arithmetic::isUnitValueStrict(1.0f));
    QVERIFY(!Arithmetic::isUnitValueStrict(0.999999f));
    QVERIFY(Arithmetic::isZeroValueClampedStrict(-0.1f));
    QVERIFY(Arithmetic::isUnitValueClampedStrict(1.1f));

    QCOMPARE(Arithmetic::min(0.8f, 0.2f, 0.5f), 0.2f);
    QCOMPARE(Arithmetic::max(0.8f, 0.2f, 0.5f), 0.8f);
    QCOMPARE(Arithmetic::zeroValue<float>(), 0.0f);
    QCOMPARE(Arithmetic::halfValue<float>(), 0.5f);
    QCOMPARE(Arithmetic::unitValue<float>(), 1.0f);
    QCOMPARE(Arithmetic::epsilon<float>(), std::numeric_limits<float>::epsilon());
    QVERIFY(closeReal(Arithmetic::unionShapeOpacity(0.25f, 0.5f), 0.625));
    QVERIFY(closeReal(Arithmetic::blend(0.8f, 0.5f, 0.2f, 0.25f, 0.6f), 0.4));
    QVERIFY(closeReal(Arithmetic::mod(0.75f, 0.5f), 0.25));
    QVERIFY(Arithmetic::isUnsafeAsDivisor(0.0f));
    QVERIFY(!Arithmetic::isUnsafeAsDivisor(0.1f));
}

void TestKoColorSpaceMaths::colorModelPoliciesPreserveLightnessAndSaturation()
{
    constexpr float red = 0.2f;
    constexpr float green = 0.4f;
    constexpr float blue = 0.8f;

    static_assert(HSYType::lightnessIsAverage);
    static_assert(HSIType::lightnessIsAverage);
    static_assert(HSLType::lightnessIsAverage);
    static_assert(!HSVType::lightnessIsAverage);

    QVERIFY(closeReal(HSYType::getLightness(red, green, blue), 0.3858));
    QVERIFY(closeReal(HSYType::getSaturation(red, green, blue), 0.6));
    QVERIFY(closeReal(HSIType::getLightness(red, green, blue), 0.4666667));
    QVERIFY(closeReal(HSIType::getSaturation(red, green, blue), 0.5714286));
    QVERIFY(closeReal(HSLType::getLightness(red, green, blue), 0.5));
    QVERIFY(closeReal(HSLType::getSaturation(red, green, blue), 0.6));
    QVERIFY(closeReal(HSVType::getLightness(red, green, blue), 0.8));
    QVERIFY(closeReal(HSVType::getSaturation(red, green, blue), 0.75));
}

void TestKoColorSpaceMaths::hueAndRgbPreservePrimarySectors()
{
    QVERIFY(closeReal(getHue(1.0f, 0.0f, 0.0f), 0.0));
    QVERIFY(closeReal(getHue(0.0f, 1.0f, 0.0f), 1.0 / 3.0));
    QVERIFY(closeReal(getHue(0.0f, 0.0f, 1.0f), 2.0 / 3.0));
    QCOMPARE(getHue(0.5f, 0.5f, 0.5f), -1.0f);

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    getRGB(red, green, blue, 0.0f);
    QCOMPARE(red, 1.0f);
    QCOMPARE(green, 0.0f);
    QCOMPARE(blue, 0.0f);

    getRGB(red, green, blue, 1.0f / 3.0f);
    QVERIFY(closeReal(red, 0.0));
    QVERIFY(closeReal(green, 1.0));
    QVERIFY(closeReal(blue, 0.0));

    getRGB(red, green, blue, 2.0f / 3.0f);
    QVERIFY(closeReal(red, 0.0));
    QVERIFY(closeReal(green, 0.0));
    QVERIFY(closeReal(blue, 1.0));

    getRGB(red, green, blue, -1.0f);
    QCOMPARE(red, 0.0f);
    QCOMPARE(green, 0.0f);
    QCOMPARE(blue, 0.0f);
}

void TestKoColorSpaceMaths::toneAndComponentAdjustmentsPreserveMappedValues()
{
    float red = -0.2f;
    float green = 0.4f;
    float blue = 0.8f;
    ToneMapping<HSLType>(red, green, blue);
    QVERIFY(closeReal(red, 0.0));
    QVERIFY(closeReal(green, 0.36));
    QVERIFY(closeReal(blue, 0.6));

    red = 0.2f;
    green = 0.8f;
    blue = 1.4f;
    ToneMapping<HSLType>(red, green, blue);
    QVERIFY(closeReal(red, 0.6));
    QVERIFY(closeReal(green, 0.8));
    QVERIFY(closeReal(blue, 1.0));

    red = 0.2f;
    green = 0.4f;
    blue = 0.8f;
    QVERIFY(closeReal(getLightness<HSLType>(red, green, blue), 0.5));
    QVERIFY(closeReal(getSaturation<HSYType>(red, green, blue), 0.6));

    addLightness<HSYType>(red, green, blue, 0.1f);
    QVERIFY(closeReal(red, 0.3));
    QVERIFY(closeReal(green, 0.5));
    QVERIFY(closeReal(blue, 0.9));

    red = 0.2f;
    green = 0.4f;
    blue = 0.8f;
    setLightness<HSLType>(red, green, blue, 0.25f);
    QVERIFY(closeReal(red, 0.0));
    QVERIFY(closeReal(green, 0.1666667));
    QVERIFY(closeReal(blue, 0.5));
    QVERIFY(closeReal(getLightness<HSLType>(red, green, blue), 0.25));

    red = 0.2f;
    green = 0.4f;
    blue = 0.8f;
    setSaturation<HSYType>(red, green, blue, 0.3f);
    QVERIFY(closeReal(red, 0.0));
    QVERIFY(closeReal(green, 0.1));
    QVERIFY(closeReal(blue, 0.3));
}

QTEST_GUILESS_MAIN(TestKoColorSpaceMaths)
