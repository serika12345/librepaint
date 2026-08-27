/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoColorModelStandardIdsUtils.h"

#include <QTest>

#include <stdexcept>
#include <type_traits>

namespace
{

enum ChannelCode {
    ChannelU8 = 8,
    ChannelU16 = 16,
    ChannelF16 = 116,
    ChannelF32 = 132,
};

template<typename T>
struct ChannelCodeForType {
    int operator()(int offset) const
    {
        if constexpr (std::is_same_v<T, quint8>) {
            return ChannelU8 + offset;
        } else if constexpr (std::is_same_v<T, quint16>) {
            return ChannelU16 + offset;
#ifdef HAVE_OPENEXR
        } else if constexpr (std::is_same_v<T, half>) {
            return ChannelF16 + offset;
#endif
        } else if constexpr (std::is_same_v<T, float>) {
            return ChannelF32 + offset;
        }

        return -1;
    }
};

void compareId(const KoID &value, const char *expectedId)
{
    QCOMPARE(value.id(), QString::fromLatin1(expectedId));
    QVERIFY(!value.name().isEmpty());
}

} // namespace

class KoColorModelStandardIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorModelIdentifiersPreserveSerializedValues();
    void colorDepthIdentifiersPreserveSerializedValues();
    void channelTypesMapToAndFromSupportedDepths();
};

void KoColorModelStandardIdsContractTest::colorModelIdentifiersPreserveSerializedValues()
{
    compareId(AlphaColorModelID, "A");
    compareId(RGBAColorModelID, "RGBA");
    compareId(XYZAColorModelID, "XYZA");
    compareId(LABAColorModelID, "LABA");
    compareId(CMYKAColorModelID, "CMYKA");
    compareId(GrayAColorModelID, "GRAYA");
    compareId(GrayColorModelID, "GRAY");
    compareId(YCbCrAColorModelID, "YCbCrA");
}

void KoColorModelStandardIdsContractTest::colorDepthIdentifiersPreserveSerializedValues()
{
    compareId(Integer8BitsColorDepthID, "U8");
    compareId(Integer16BitsColorDepthID, "U16");
    compareId(Float16BitsColorDepthID, "F16");
    compareId(Float32BitsColorDepthID, "F32");
    compareId(Float64BitsColorDepthID, "F64");
}

void KoColorModelStandardIdsContractTest::channelTypesMapToAndFromSupportedDepths()
{
    QCOMPARE(colorDepthIdForChannelType<quint8>(), Integer8BitsColorDepthID);
    QCOMPARE(colorDepthIdForChannelType<quint16>(), Integer16BitsColorDepthID);
#ifdef HAVE_OPENEXR
    QCOMPARE(colorDepthIdForChannelType<half>(), Float16BitsColorDepthID);
#endif
    QCOMPARE(colorDepthIdForChannelType<float>(), Float32BitsColorDepthID);
    QCOMPARE(colorDepthIdForChannelType<double>(), Float64BitsColorDepthID);

    QCOMPARE(channelTypeForColorDepthId<ChannelCodeForType>(Integer8BitsColorDepthID, 1), ChannelU8 + 1);
    QCOMPARE(channelTypeForColorDepthId<ChannelCodeForType>(Integer16BitsColorDepthID, 1), ChannelU16 + 1);
#ifdef HAVE_OPENEXR
    QCOMPARE(channelTypeForColorDepthId<ChannelCodeForType>(Float16BitsColorDepthID, 1), ChannelF16 + 1);
#endif
    QCOMPARE(channelTypeForColorDepthId<ChannelCodeForType>(Float32BitsColorDepthID, 1), ChannelF32 + 1);

    const KoID invalidDepth(QStringLiteral("invalid"));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, channelTypeForColorDepthId<ChannelCodeForType>(invalidDepth, 0));
}

QTEST_GUILESS_MAIN(KoColorModelStandardIdsContractTest)

#include "KoColorModelStandardIdsContractTest.moc"
