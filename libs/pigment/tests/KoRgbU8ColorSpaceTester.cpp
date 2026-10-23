/*
 *  SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 *  SPDX-FileCopyrightText: 2008 Bart Coppens <kde@bartcoppens.be>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoRgbU8ColorSpaceTester.h"

#include "KoColorModelStandardIds.h"

#include <simpletest.h>
#include <DebugPigment.h>
#include <string.h>

#include "KoColor.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoCompositeOp.h"
#include "KoMixColorsOp.h"
#include <KoCompositeOpRegistry.h>
#include <kistest.h>

#define NUM_CHANNELS 4

#define RED_CHANNEL 0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL 2
#define ALPHA_CHANNEL 3
#include <KoColorProfile.h>
#include <KoAlphaMaskApplicatorBase.h>
#include <KoAlphaMaskApplicator.h>
#include <KoOptimizedCompositeOpFactory.h>
#include <KoCompositeOpAlphaDarken.h>
#include <KoAlphaDarkenParamsWrapper.h>
#include <KoBgrColorSpaceTraits.h>
#include <memory>

void KoRgbU8ColorSpaceTester::testSourceOverPreservesDivisionRounding_data()
{
    QTest::addColumn<int>("sourceColor");
    QTest::addColumn<int>("sourceAlpha");
    QTest::addColumn<int>("destinationColor");
    QTest::addColumn<int>("destinationAlpha");
    QTest::addColumn<int>("outputAlpha");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("offset");
    // These source-over inputs all have the exact color 5494/33, which rounds to 166.
    // Their alpha denominators exercise the reciprocal estimates identified in Issue #91.
    struct Case { int source, sourceAlpha, destination, destinationAlpha, outputAlpha; };
    const Case cases[] = {
        {0, 9, 201, 45, 52},
        {166, 15, 167, 15, 29},
        {166, 30, 167, 32, 58},
        {0, 30, 201, 164, 175},
        {255, 48, 128, 136, 158},
        {0, 50, 218, 201, 212},
        {166, 51, 167, 60, 99},
        {166, 55, 167, 66, 107},
        {211, 60, 98, 51, 99},
        {166, 75, 218, 1, 76},
        {166, 75, 168, 34, 99},
        {166, 75, 167, 100, 146},
        {200, 80, 150, 237, 243},
        {83, 84, 228, 170, 198},
        {83, 84, 218, 203, 220},
        {166, 85, 167, 120, 165},
        {255, 102, 48, 127, 178},
        {166, 102, 167, 160, 198},
        {211, 102, 133, 226, 238},
        {166, 105, 167, 168, 204},
        {166, 111, 167, 185, 215},
        {166, 119, 167, 210, 231},
        {255, 128, 48, 192, 224},
        {211, 138, 48, 113, 190},
        {153, 166, 252, 75, 192},
        {166, 215, 173, 102, 231},
        {166, 225, 218, 18, 227},
        {173, 230, 48, 129, 243},
        {166, 249, 252, 60, 250},
        {166, 250, 218, 120, 252},
    };
    for (const auto &c : cases) for (const int width : {1, 8, 17}) for (const int offset : {0, 4}) {
        const auto name = QStringLiteral("alpha-%1-%2-color-%3-%4-width-%5-offset-%6")
            .arg(c.sourceAlpha).arg(c.destinationAlpha).arg(c.source).arg(c.destination).arg(width).arg(offset).toLatin1();
        QTest::newRow(name.constData()) << c.source << c.sourceAlpha << c.destination << c.destinationAlpha << c.outputAlpha << width << offset;
    }
}

void KoRgbU8ColorSpaceTester::testSourceOverPreservesDivisionRounding()
{
    QFETCH(int, sourceColor);
    QFETCH(int, sourceAlpha);
    QFETCH(int, destinationColor);
    QFETCH(int, destinationAlpha);
    QFETCH(int, outputAlpha);
    QFETCH(int, width);
    QFETCH(int, offset);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(cs);
    const auto op = cs->compositeOp(COMPOSITE_OVER);
    QVERIFY(op);
    QByteArray source(width * 4 + offset + 16, char(0x5a));
    QByteArray destination = source;
    const QByteArray sourcePixel = QByteArray(3, char(sourceColor)) + char(sourceAlpha);
    const QByteArray destinationPixel = QByteArray(3, char(destinationColor)) + char(destinationAlpha);
    const QByteArray expectedPixel = QByteArray(3, char(166)) + char(outputAlpha);
    source.replace(offset, width * 4, sourcePixel.repeated(width));
    destination.replace(offset, width * 4, destinationPixel.repeated(width));
    const auto unchangedSource = source;
    auto expected = destination;
    expected.replace(offset, width * 4, expectedPixel.repeated(width));
    op->composite(reinterpret_cast<quint8 *>(destination.data() + offset), width * 4,
                  reinterpret_cast<const quint8 *>(source.constData() + offset), width * 4,
                  nullptr, 0, 1, width, 1.0f);
    QCOMPARE(destination, expected);
    QCOMPARE(source, unchangedSource);
}

void KoRgbU8ColorSpaceTester::testSourceOverPreservesPixelsAcrossBatchBoundaries_data()
{
    QTest::addColumn<int>("sourceAlpha");
    QTest::addColumn<int>("destinationAlpha");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<QByteArray>("expectedPixel");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("offset");
    // Fixed scalar results agree on ARM64 and x86_64. A tile's width and
    // alignment must preserve these same editable RGBA8 values.
    struct Case { int sourceAlpha, destinationAlpha, opacity; const char *expected; };
    const Case cases[] = {
        {102, 50, 255, "1a3f6584"},
        {168, 136, 255, "193f65d6"},
        {180, 180, 255, "1a3f65e9"},
        {170, 238, 255, "1c446bf9"},
        {134, 226, 128, "295886ea"},
    };
    for (const auto &c : cases) for (const int width : {1, 8, 17}) for (const int offset : {0, 4}) {
        const QByteArray name = QByteArray::number(c.sourceAlpha) + '-' + QByteArray::number(c.destinationAlpha) +
            '-' + QByteArray::number(c.opacity) + '-' + QByteArray::number(width) + '-' + QByteArray::number(offset);
        QTest::newRow(name.constData()) << c.sourceAlpha << c.destinationAlpha << c.opacity <<
            QByteArray::fromHex(c.expected) << width << offset;
    }
}

void KoRgbU8ColorSpaceTester::testSourceOverPreservesPixelsAcrossBatchBoundaries()
{
    QFETCH(int, sourceAlpha);
    QFETCH(int, destinationAlpha);
    QFETCH(int, opacity);
    QFETCH(QByteArray, expectedPixel);
    QFETCH(int, width);
    QFETCH(int, offset);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(cs);
    const auto op = cs->compositeOp(COMPOSITE_OVER);
    QVERIFY(op);
    QByteArray source(width * 4 + offset + 16, char(0x5a));
    QByteArray destination = source;
    const QByteArray sourcePixel = QByteArray::fromHex("123456") + char(sourceAlpha);
    const QByteArray destinationPixel = QByteArray::fromHex("336699") + char(destinationAlpha);
    source.replace(offset, width * 4, sourcePixel.repeated(width));
    destination.replace(offset, width * 4, destinationPixel.repeated(width));
    const auto unchangedSource = source;
    auto expected = destination;
    expected.replace(offset, width * 4, expectedPixel.repeated(width));
    op->composite(reinterpret_cast<quint8 *>(destination.data() + offset), width * 4,
                  reinterpret_cast<const quint8 *>(source.constData() + offset), width * 4,
                  nullptr, 0, 1, width, opacity / 255.0f);
    QCOMPARE(destination, expected);
    QCOMPARE(source, unchangedSource);
}

void KoRgbU8ColorSpaceTester::testInverseFloatMaskPreservesPixelsAcrossBatches_data()
{
    QTest::addColumn<bool>("fill");
    QTest::addColumn<bool>("scalar");
    QTest::newRow("existing-color-selected") << false << false;
    QTest::newRow("brush-color-selected") << true << false;
    QTest::newRow("existing-color-scalar") << false << true;
    QTest::newRow("brush-color-scalar") << true << true;
}

void KoRgbU8ColorSpaceTester::testInverseFloatMaskPreservesPixelsAcrossBatches()
{
    QFETCH(bool, fill);
    QFETCH(bool, scalar);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(cs);
    std::unique_ptr<KoAlphaMaskApplicatorBase> scalarApplicator;
    if (scalar) scalarApplicator = std::make_unique<KoAlphaMaskApplicator<quint8, 4, 3, xsimd::generic>>();
    const QByteArray brushColor = QByteArray::fromHex("112233c9");
    const auto apply = [&](quint8 *pixels, const float *mask, int count) {
        if (fill) {
            const auto color = reinterpret_cast<const quint8 *>(brushColor.constData());
            if (scalar) scalarApplicator->fillInverseAlphaNormedFloatMaskWithColor(pixels, mask, color, count);
            else cs->fillInverseAlphaNormedFloatMaskWithColor(pixels, mask, color, count);
        } else {
            if (scalar) scalarApplicator->applyInverseNormedFloatMask(pixels, mask, count);
            else cs->applyInverseNormedFloatMask(pixels, mask, count);
        }
    };

    // Brush masks update editable pixels. Preserve the SIMD body's nearest-even
    // alpha, including exact half values, when rows or jobs split the same mask.
    struct Case { float mask; int alpha, applied, filled; };
    const Case cases[] = {
        {0.0f, 17, 17, 255}, {1.0f, 255, 0, 0},
        {0.5f, 1, 0, 128}, {0.5f, 3, 2, 128},
        {0.5f, 127, 64, 128}, {0.5f, 129, 64, 128},
        {0.25f, 2, 2, 191}, {0.25f, 6, 4, 191},
        {0.75f, 2, 0, 64}, {0.75f, 6, 2, 64},
        {0.1875f, 255, 207, 207}, {0.6875f, 255, 80, 80},
    };
    for (const auto &c : cases) {
        for (const int count : {0, 1, 3, 4, 5, 7, 8, 9, 12, 15, 16, 17, 31}) {
            for (const int offset : {0, 4}) {
                QByteArray pixels(offset + count * 4 + 16, char(0x5a));
                const QByteArray original = QByteArray::fromHex("a1b2c3") + char(c.alpha);
                pixels.replace(offset, count * 4, original.repeated(count));
                auto expected = pixels;
                // Both implementations replace the brush color's alpha with mask coverage.
                const QByteArray output = (fill ? brushColor.first(3) : original.first(3)) + char(fill ? c.filled : c.applied);
                expected.replace(offset, count * 4, output.repeated(count));
                auto partitioned = pixels;
                const QList<float> mask(count + 2, c.mask);
                const auto unchangedMask = mask;
                apply(reinterpret_cast<quint8 *>(pixels.data() + offset), mask.constData() + 1, count);
                for (int i = 0; i < count; ++i)
                    apply(reinterpret_cast<quint8 *>(partitioned.data() + offset + i * 4), mask.constData() + 1 + i, 1);
                const auto context = QStringLiteral("mask=%1 input-alpha=%2 count=%3 offset=%4")
                    .arg(c.mask).arg(c.alpha).arg(count).arg(offset).toUtf8();
                QVERIFY2(pixels == expected, context.constData());
                QVERIFY2(partitioned == expected, context.constData());
                QCOMPARE(mask, unchangedMask);
                QCOMPARE(brushColor, QByteArray::fromHex("112233c9"));
            }
        }
    }
}

void KoRgbU8ColorSpaceTester::testSourceOverPreservesMixedDestinationPixels()
{
    const auto op = KoColorSpaceRegistry::instance()->rgb8()->compositeOp(COMPOSITE_OVER);
    QVERIFY(op);
    const auto start = [](QByteArray &bytes, int offset) {
        return 32 + int((32 - (reinterpret_cast<quintptr>(bytes.data()) & 31)) & 31) + offset;
    };
    for (bool masked : {false, true}) for (int count : {1, 4, 8, 17, 57, 64, 257}) {
        for (int offset : {0, 4, 8, 12, 16, 20, 24, 28}) {
            QByteArray source(count * 4, '\0'), mask(count, '\0');
            QByteArray whole(count * 4 + 128, char(0x5a)), split(count * 4 + 128, char(0x5a));
            const int wholeStart = start(whole, offset), splitStart = start(split, offset);
            for (int i = 0; i < count; ++i) {
                const quint8 src[] = {117, 29, 201, 250};
                const quint8 dst[] = {102, 85, 68, quint8(i % 3 == 0 ? 0 : i % 3 == 1 ? 128 : 255)};
                memcpy(source.data() + i * 4, src, 4);
                memcpy(whole.data() + wholeStart + i * 4, dst, 4);
                memcpy(split.data() + splitStart + i * 4, dst, 4);
                mask[i] = char((i * 73 + 128) % 256);
            }
            const auto compose = [&](QByteArray &dst, int begin, int first, int length) {
                op->composite(reinterpret_cast<quint8 *>(dst.data() + begin + first * 4), count * 4,
                              reinterpret_cast<const quint8 *>(source.constData() + first * 4), count * 4,
                              masked ? reinterpret_cast<const quint8 *>(mask.constData() + first) : nullptr,
                              masked ? count : 0, 1, length, .57f);
            };
            compose(whole, wholeStart, 0, count);
            for (int i = 0; i < count; ++i) compose(split, splitStart, i, 1);
            const auto context = QStringLiteral("mixed destination count=%1 offset=%2 masked=%3")
                .arg(count).arg(offset).arg(masked).toUtf8();
            const auto actual = whole.sliced(wholeStart, count * 4);
            QVERIFY2(actual == split.sliced(splitStart, count * 4), context.constData());
            if (!masked) {
                // The actual default brush supplies A=250 at opacity .57 to an
                // empty layer. Its binary32 alpha is 142.5, rounded to even 142.
                for (int i = 0; i < count; i += 3)
                    QCOMPARE(actual.sliced(i * 4, 4), QByteArray::fromHex("751dc98e"));
            }
            struct GuardedPixels { const QByteArray *bytes; int begin; };
            const GuardedPixels guarded[] = {{&whole, wholeStart}, {&split, splitStart}};
            for (const auto &item : guarded) {
                QCOMPARE(item.bytes->first(item.begin), QByteArray(item.begin, char(0x5a)));
                const int end = item.begin + count * 4;
                QCOMPARE(item.bytes->sliced(end), QByteArray(item.bytes->size() - end, char(0x5a)));
            }
        }
    }
}

void KoRgbU8ColorSpaceTester::testAlphaDarkenPreservesPixelsAcrossBatches_data()
{
    QTest::addColumn<bool>("hard");
    QTest::addColumn<bool>("generic");
    QTest::newRow("creamy-selected") << false << false;
    QTest::newRow("creamy-generic") << false << true;
    QTest::newRow("hard-selected") << true << false;
    QTest::newRow("hard-generic") << true << true;
}

void KoRgbU8ColorSpaceTester::testAlphaDarkenPreservesPixelsAcrossBatches()
{
    QFETCH(bool, hard);
    QFETCH(bool, generic);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    const auto create = [&](bool fallback) -> std::unique_ptr<KoCompositeOp> {
        if (fallback) {
            if (hard) return std::make_unique<KoCompositeOpAlphaDarken<KoBgrU8Traits, KoAlphaDarkenParamsWrapperHard>>(cs);
            return std::make_unique<KoCompositeOpAlphaDarken<KoBgrU8Traits, KoAlphaDarkenParamsWrapperCreamy>>(cs);
        }
        return std::unique_ptr<KoCompositeOp>(hard ? KoOptimizedCompositeOpFactory::createAlphaDarkenOpHard32(cs)
                                                  : KoOptimizedCompositeOpFactory::createAlphaDarkenOpCreamy32(cs));
    };
    const auto op = create(generic);
    const auto other = create(!generic);
    QVERIFY(op && other);
    const auto composite = [](const KoCompositeOp *active, quint8 *dst, const quint8 *src,
                              const quint8 *mask, int count, float opacity, float flow, float average) {
        KoCompositeOp::ParameterInfo p;
        p.dstRowStart = dst;
        p.srcRowStart = src;
        p.maskRowStart = mask;
        p.dstRowStride = p.srcRowStride = qMax(1, count) * 4;
        p.maskRowStride = mask ? qMax(1, count) : 0;
        p.rows = 1;
        p.cols = count;
        p.flow = flow;
        p.setOpacityAndAverage(opacity, average);
        active->composite(p);
    };
    // WASH callers split dabs at tile boundaries. Control absolute alignment as
    // well as the offset so allocation layout cannot hide a scalar prefix.
    struct Pixels {
        QByteArray bytes;
        int begin;
        Pixels(int count, int offset) : bytes(count * 4 + 128, char(0x5a))
        {
            const auto alignment = reinterpret_cast<quintptr>(bytes.data()) & 31;
            begin = 32 + int((32 - alignment) & 31) + offset;
        }
        quint8 *data() { return reinterpret_cast<quint8 *>(bytes.data() + begin); }
        const quint8 *constData() const { return reinterpret_cast<const quint8 *>(bytes.constData() + begin); }
    };
    struct Case { const char *src; const char *dst; const char *expected; float opacity; int mask; float average; };
    const Case cases[] = {
        // Byte-domain binary32 alpha and final nearest-even quantization.
        {"11223332", "66554400", "1122331c", .57f, -1, .57f},
        {"11223396", "66554400", "11223356", .57f, -1, .57f},
        {"112233fa", "66554400", "1122338e", .57f, -1, .57f},
        {"0103ffff", "00000080", "00028080", .5f, -1, .5f},
        {"112233ff", "66554400", "11223340", .5f, 128, .5f},
        // Round each binary32 product and sum before quantizing the byte.
        {"000000cd", "00000033", "00000070", .5f, -1, .5f},
        {"000000dd", "0000003c", "00000076", .5f, -1, .5f},
        {"00000096", "00000066", "00000080", .57f, -1, .57f},
        // Decreasing pressure keeps the same correctly rounded quotient.
        {"00000033", "00000008", "00000021", .5f, -1, .8f},
        {"00000011", "00000004", "0000000e", .57f, -1, .8f},
        // A zero contribution preserves even the destination's transparent RGB.
        {"11223300", "66554400", "66554400", .57f, -1, .57f},
        {"112233ff", "66554400", "66554400", .57f, 0, .57f},
        {"112233ff", "66554480", "66554480", .57f, 0, .57f},
        {"112233ff", "66554400", "66554400", 0.0f, -1, 0.0f},
    };
    for (const auto &c : cases) {
        for (int count : {0, 1, 3, 4, 7, 8, 9, 17, 31}) for (int offset : {0, 4, 8, 12, 16, 20, 24, 28}) {
            Pixels source(count, 4), whole(count, offset), split(count, offset);
            const auto src = QByteArray::fromHex(c.src).repeated(count);
            const auto dst = QByteArray::fromHex(c.dst).repeated(count);
            memcpy(source.data(), src.constData(), src.size());
            memcpy(whole.data(), dst.constData(), dst.size());
            memcpy(split.data(), dst.constData(), dst.size());
            QByteArray expectedWhole(whole.bytes.constData(), whole.bytes.size());
            QByteArray expectedSplit(split.bytes.constData(), split.bytes.size());
            const auto expected = QByteArray::fromHex(c.expected).repeated(count);
            memcpy(expectedWhole.data() + whole.begin, expected.constData(), expected.size());
            memcpy(expectedSplit.data() + split.begin, expected.constData(), expected.size());
            const QByteArray mask(qMax(1, count), char(c.mask));
            const auto maskData = c.mask < 0 ? nullptr : reinterpret_cast<const quint8 *>(mask.constData());
            composite(op.get(), whole.data(), source.constData(), maskData, count, c.opacity, 1, c.average);
            for (int i = 0; i < count; ++i)
                composite(op.get(), split.data() + 4 * i, source.constData() + 4 * i,
                          maskData ? maskData + i : nullptr, 1, c.opacity, 1, c.average);
            const auto context = QStringLiteral("source=%1 mask=%2 count=%3 offset=%4 actual=%5 expected=%6")
                .arg(QLatin1String(c.src)).arg(c.mask).arg(count).arg(offset)
                .arg(QString::fromLatin1(whole.bytes.toHex()), QString::fromLatin1(expectedWhole.toHex())).toUtf8();
            QVERIFY2(whole.bytes == expectedWhole, context.constData());
            QCOMPARE(split.bytes, expectedSplit);
        }
    }

    const int alphas[] = {0, 50, 150, 250, 255, 128, 17, 1};
    const int masks[] = {255, 0, 128, 64, 0, 1, 255, 128};
    for (bool masked : {false, true}) for (float flow : {1.0f, .5f}) for (float average : {.57f, .8f}) {
        for (int count : {1, 4, 8, 17, 31, 257}) for (int offset : {0, 4, 8, 12, 16, 20, 24, 28}) {
            Pixels source(count, 4), whole(count, offset), split(count, offset), alternate(count, offset);
            QByteArray mask(count, '\0');
            quint32 state = 0x719f2a63;
            const auto sample = [&]() -> quint8 {
                state = state * 1664525u + 1013904223u;
                return state >> 24;
            };
            for (int i = 0; i < count; ++i) {
                const quint8 src[] = {sample(), sample(), sample(), i < 32 ? quint8(alphas[i % 8]) : sample()};
                const quint8 dst[] = {sample(), sample(), sample(), i < 32 ? quint8(alphas[(i + 2) % 8]) : sample()};
                memcpy(source.data() + i * 4, src, 4);
                for (auto *pixels : {&whole, &split, &alternate}) memcpy(pixels->data() + i * 4, dst, 4);
                mask[i] = char(i < 32 ? quint8(masks[i % 8]) : sample());
            }
            const auto maskData = masked ? reinterpret_cast<const quint8 *>(mask.constData()) : nullptr;
            composite(op.get(), whole.data(), source.constData(), maskData, count, .57f, flow, average);
            composite(other.get(), alternate.data(), source.constData(), maskData, count, .57f, flow, average);
            for (int i = 0; i < count; ++i)
                composite(op.get(), split.data() + i * 4, source.constData() + i * 4,
                          maskData ? maskData + i : nullptr, 1, .57f, flow, average);
            const auto context = QStringLiteral("mixed count=%1 offset=%2 masked=%3 flow=%4 average=%5")
                .arg(count).arg(offset).arg(masked).arg(flow).arg(average).toUtf8();
            const QByteArray result(reinterpret_cast<const char *>(whole.constData()), count * 4);
            QVERIFY2(result == QByteArray(reinterpret_cast<const char *>(split.constData()), count * 4), context.constData());
            QVERIFY2(result == QByteArray(reinterpret_cast<const char *>(alternate.constData()), count * 4), context.constData());
            for (const auto *pixels : {&whole, &split, &alternate}) {
                QCOMPARE(pixels->bytes.first(pixels->begin), QByteArray(pixels->begin, char(0x5a)));
                const auto end = pixels->begin + count * 4;
                QCOMPARE(pixels->bytes.sliced(end), QByteArray(pixels->bytes.size() - end, char(0x5a)));
            }
        }
    }

    // A half-pressure stroke crosses these alpha pairs repeatedly. Every pair
    // must survive a tile split with the same staged binary32 result.
    constexpr int count = 256 * 256;
    for (float opacity : {.5f, .57f}) for (float flow : {1.0f, .5f})
        for (float average : {opacity, .8f}) for (bool sourceVariesFirst : {false, true}) {
        for (int offset : {0, 4}) {
            Pixels source(count, 4), whole(count, offset), split(count, offset), alternate(count, offset);
            for (int i = 0; i < count; ++i) {
                const quint8 src[] = {0, 0, 0, quint8(sourceVariesFirst ? i % 256 : i / 256)};
                const quint8 dst[] = {0, 0, 0, quint8(sourceVariesFirst ? i / 256 : i % 256)};
                memcpy(source.data() + i * 4, src, 4);
                for (auto *pixels : {&whole, &split, &alternate}) memcpy(pixels->data() + i * 4, dst, 4);
            }
            composite(op.get(), whole.data(), source.constData(), nullptr, count, opacity, flow, average);
            composite(other.get(), alternate.data(), source.constData(), nullptr, count, opacity, flow, average);
            for (int i = 0; i < count; ++i)
                composite(op.get(), split.data() + i * 4, source.constData() + i * 4, nullptr, 1, opacity, flow, average);
            for (int i = 0; i < count; ++i) {
                const auto *w = whole.constData() + i * 4;
                const auto *s = split.constData() + i * 4;
                const auto *g = alternate.constData() + i * 4;
                if (!memcmp(w, s, 4) && !memcmp(w, g, 4)) continue;
                const auto actual = QByteArray(reinterpret_cast<const char *>(w), 4);
                const auto partitioned = QByteArray(reinterpret_cast<const char *>(s), 4);
                const auto fallback = QByteArray(reinterpret_cast<const char *>(g), 4);
                const auto context = QStringLiteral("source=%1 destination=%2 opacity=%3 flow=%4 average=%5 offset=%6 whole=%7 split=%8 other=%9")
                    .arg(int(source.constData()[i * 4 + 3])).arg(sourceVariesFirst ? i / 256 : i % 256)
                    .arg(opacity).arg(flow).arg(average).arg(offset).arg(QString::fromLatin1(actual.toHex()), QString::fromLatin1(partitioned.toHex()),
                                                QString::fromLatin1(fallback.toHex())).toUtf8();
                QFAIL(context.constData());
            }
        }
    }
}

void KoRgbU8ColorSpaceTester::testBasics()
{
}

#define PIXEL_RED 0
#define PIXEL_GREEN 1
#define PIXEL_BLUE 2
#define PIXEL_ALPHA 3

void KoRgbU8ColorSpaceTester::testMixColors()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    KoMixColorsOp * mixOp = cs->mixColorsOp();

    // Test mixColors.
    quint8 pixel1[4];
    quint8 pixel2[4];
    quint8 outputPixel[4];

    pixel1[PIXEL_RED] = 255;
    pixel1[PIXEL_GREEN] = 255;
    pixel1[PIXEL_BLUE] = 255;
    pixel1[PIXEL_ALPHA] = 255;

    pixel2[PIXEL_RED] = 0;
    pixel2[PIXEL_GREEN] = 0;
    pixel2[PIXEL_BLUE] = 0;
    pixel2[PIXEL_ALPHA] = 0;

    const quint8 *pixelPtrs[2];
    qint16 weights[2];

    pixelPtrs[0] = pixel1;
    pixelPtrs[1] = pixel2;

    weights[0] = 255;
    weights[1] = 0;

    mixOp->mixColors(pixelPtrs, weights, 2, outputPixel);

    QVERIFY((int)outputPixel[PIXEL_RED] == 255);
    QVERIFY((int)outputPixel[PIXEL_GREEN] == 255);
    QVERIFY((int)outputPixel[PIXEL_BLUE] == 255);
    QVERIFY((int)outputPixel[PIXEL_ALPHA] == 255);

    weights[0] = 0;
    weights[1] = 255;

    mixOp->mixColors(pixelPtrs, weights, 2, outputPixel);

    QVERIFY((int)outputPixel[PIXEL_RED] == 0);
    QVERIFY((int)outputPixel[PIXEL_GREEN] == 0);
    QVERIFY((int)outputPixel[PIXEL_BLUE] == 0);
    QVERIFY((int)outputPixel[PIXEL_ALPHA] == 0);

    weights[0] = 128;
    weights[1] = 127;

    mixOp->mixColors(pixelPtrs, weights, 2, outputPixel);

    QVERIFY((int)outputPixel[PIXEL_RED] == 255);
    QVERIFY((int)outputPixel[PIXEL_GREEN] == 255);
    QVERIFY((int)outputPixel[PIXEL_BLUE] == 255);
    QVERIFY((int)outputPixel[PIXEL_ALPHA] == 128);

    pixel1[PIXEL_RED] = 200;
    pixel1[PIXEL_GREEN] = 100;
    pixel1[PIXEL_BLUE] = 50;
    pixel1[PIXEL_ALPHA] = 255;

    pixel2[PIXEL_RED] = 100;
    pixel2[PIXEL_GREEN] = 200;
    pixel2[PIXEL_BLUE] = 20;
    pixel2[PIXEL_ALPHA] = 255;

    mixOp->mixColors(pixelPtrs, weights, 2, outputPixel);

    QVERIFY((int)outputPixel[PIXEL_RED] == 150);
    QCOMPARE((int)outputPixel[PIXEL_GREEN], 150);
    QVERIFY((int)outputPixel[PIXEL_BLUE] == 35);
    QVERIFY((int)outputPixel[PIXEL_ALPHA] == 255);

    pixel1[PIXEL_RED] = 0;
    pixel1[PIXEL_GREEN] = 0;
    pixel1[PIXEL_BLUE] = 0;
    pixel1[PIXEL_ALPHA] = 0;

    pixel2[PIXEL_RED] = 255;
    pixel2[PIXEL_GREEN] = 255;
    pixel2[PIXEL_BLUE] = 255;
    pixel2[PIXEL_ALPHA] = 254;

    weights[0] = 89;
    weights[1] = 166;

    mixOp->mixColors(pixelPtrs, weights, 2, outputPixel);

    QVERIFY((int)outputPixel[PIXEL_RED] == 255);
    QVERIFY((int)outputPixel[PIXEL_GREEN] == 255);
    QVERIFY((int)outputPixel[PIXEL_BLUE] == 255);
    QVERIFY((int)outputPixel[PIXEL_ALPHA] == 165);
}
void KoRgbU8ColorSpaceTester::testMixColorsAverage()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    KoMixColorsOp * mixOp = cs->mixColorsOp();

    // Test mixColors.
    quint8 pixel1[4];
    quint8 pixel2[4];
    quint8 outputPixel[4];

    pixel1[PIXEL_RED] = 255;
    pixel1[PIXEL_GREEN] = 255;
    pixel1[PIXEL_BLUE] = 255;
    pixel1[PIXEL_ALPHA] = 255;

    pixel2[PIXEL_RED] = 0;
    pixel2[PIXEL_GREEN] = 0;
    pixel2[PIXEL_BLUE] = 0;
    pixel2[PIXEL_ALPHA] = 0;

    const quint8 *pixelPtrs[2];

    pixelPtrs[0] = pixel1;
    pixelPtrs[1] = pixel2;

    mixOp->mixColors(pixelPtrs, 2, outputPixel);

    QCOMPARE((int)outputPixel[PIXEL_RED], 255);
    QCOMPARE((int)outputPixel[PIXEL_GREEN], 255);
    QCOMPARE((int)outputPixel[PIXEL_BLUE], 255);
    QCOMPARE((int)outputPixel[PIXEL_ALPHA], 128);

    pixel2[PIXEL_ALPHA] = 255;
    mixOp->mixColors(pixelPtrs, 2, outputPixel);

    QCOMPARE((int)outputPixel[PIXEL_RED], 128);
    QCOMPARE((int)outputPixel[PIXEL_GREEN], 128);
    QCOMPARE((int)outputPixel[PIXEL_BLUE], 128);
    QCOMPARE((int)outputPixel[PIXEL_ALPHA], 255);
}

void KoRgbU8ColorSpaceTester::testCompositeOps()
{
    // Just COMPOSITE_COPY for now

    QList<KoID> depthIDs = KoColorSpaceRegistry::instance()->colorDepthList(RGBAColorModelID.id(),
                           KoColorSpaceRegistry::AllColorSpaces);

    Q_FOREACH (const KoID& depthId, depthIDs) {

        if (depthId.id().contains("Float")) continue;

        qDebug() << depthId.id();
        const KoColorSpace* cs = KoColorSpaceRegistry::instance()->colorSpace(
                                     RGBAColorModelID.id(), depthId.id(), "");
        
        const KoCompositeOp* copyOp = cs->compositeOp(COMPOSITE_COPY);
        KoColor src(cs), dst(cs);

        QColor red(255, 0, 0);
        QColor blue(0, 0, 255);
        QColor transparentRed(255, 0, 0, 0);

        // Copying a color over another color should replace the original color
        src.fromQColor(red);
        dst.fromQColor(blue);
        
        qDebug() << src.toQColor() << dst.toQColor();

        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) != 0);

        copyOp->composite(dst.data(), cs->pixelSize(), src.data(), cs->pixelSize(),
                          0, 0, 1, 1, OPACITY_OPAQUE_F);

        src.fromQColor(red);
        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) == 0);

        // Copying something transparent over something non-transparent should, of course, make the dst transparent
        src.fromQColor(transparentRed);
        dst.fromQColor(blue);

        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) != 0);

        copyOp->composite(dst.data(), cs->pixelSize(), src.data(), cs->pixelSize(),
                          0, 0, 1, 1, OPACITY_OPAQUE_F);

        src.fromQColor(transparentRed);
        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) == 0);

        // Copying something solid over something transparent
        src.fromQColor(blue);
        dst.fromQColor(transparentRed);

        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) != 0);

        copyOp->composite(dst.data(), cs->pixelSize(), src.data(), cs->pixelSize(),
                          0, 0, 1, 1, OPACITY_OPAQUE_F);

        src.fromQColor(blue);
        QVERIFY(memcmp(dst.data(), src.data(), cs->pixelSize()) == 0);

    }
}

void KoRgbU8ColorSpaceTester::testCompositeOpsWithChannelFlags()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    QList<KoCompositeOp*> ops = cs->compositeOps();

    Q_FOREACH (const KoCompositeOp *op, ops) {
        /**
         * ALPHA_DARKEN composite op doesn't take channel
         * flags into account, so just skip it
         */
        if (op->id() == COMPOSITE_ALPHA_DARKEN) continue;
        if (op->id() == COMPOSITE_DISSOLVE) continue;

        quint8 src[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badDst[] = {12,12,12,0};

        KoCompositeOp::ParameterInfo params;
        params.maskRowStart  = 0;
        params.dstRowStride  = 0;
        params.srcRowStride  = 0;
        params.maskRowStride = 0;
        params.rows          = 1;
        params.cols          = 1;
        params.opacity       = 1.0f;
        params.flow          = 1.0f;

        QBitArray channelFlags(4, true);
        channelFlags[2] = false;
        params.channelFlags  = channelFlags;

        params.srcRowStart   = src;

        params.dstRowStart   = goodDst;
        op->composite(params);

        params.dstRowStart   = badDst;
        op->composite(params);

        /**
         * The badDst has zero alpha, so the channels should be zeroed
         * before increasing alpha of the pixel
         */
        if (badDst[3] != 0 && badDst[2] != 0) {
            qDebug() << op->id()
                     << "easy case:" << goodDst[2]
                     << "difficult case:" << badDst[2];

            qDebug() << "The composite op has failed to erase the color "
                "channel which was hidden by zero alpha.";
            qDebug() << "Expected Blue channel:" << 0;
            qDebug() << "Actual Blue channel:  " << badDst[2];

            QFAIL("Failed to erase color channel");
        }
    }
}

// for posix_memalign()
#include <stdlib.h>

#if defined Q_OS_WIN
#define MEMALIGN_ALLOC(p, a, s) ((*(p)) = _aligned_malloc((s), (a)), *(p) ? 0 : errno)
#define MEMALIGN_FREE(p) _aligned_free((p))
#else
#define MEMALIGN_ALLOC(p, a, s) posix_memalign((p), (a), (s))
#define MEMALIGN_FREE(p) free((p))
#endif

void KoRgbU8ColorSpaceTester::testCompositeCopyDivisionByZero()
{
    const KoColorSpace* cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoCompositeOp *op = cs->compositeOp(COMPOSITE_COPY);

    const int pixelAlignment = sizeof(float) * 16; // 512-bit alignment should be enough for everyone! (c)
    const int numPixels = 256;
    void *src = 0;
    void *dst = 0;

    int result = 0;

    result = MEMALIGN_ALLOC(&src, pixelAlignment, cs->pixelSize() * numPixels);
    KIS_ASSERT(!result);

    result = MEMALIGN_ALLOC(&dst, pixelAlignment, cs->pixelSize() * numPixels);
    KIS_ASSERT(!result);

    // generate buffers in misaligned manner
    int numTestablePixels = numPixels - 1;
    quint8 * srcPtr = reinterpret_cast<quint8*>(src) + cs->pixelSize();
    quint8 * dstPtr = reinterpret_cast<quint8*>(dst) + cs->pixelSize();


    auto testBadPixel = [cs, op, srcPtr, dstPtr, numTestablePixels] (int badPixelPos,
            const quint8 *goodSrc,
            const quint8 *goodDst,
            const quint8 *badSrc,
            const quint8 *badDst,
            quint8 opacity,
            quint8 *expectedDst) {

        quint8 *badPixelDstPtr = dstPtr + badPixelPos * cs->pixelSize();

        for (int i = 0; i < numTestablePixels; i++) {
            if (i != badPixelPos) {
                memcpy(srcPtr + cs->pixelSize() * i, goodSrc, cs->pixelSize());
                memcpy(dstPtr + cs->pixelSize() * i, goodDst, cs->pixelSize());
            } else {
                memcpy(srcPtr + cs->pixelSize() * i, badSrc, cs->pixelSize());
                memcpy(dstPtr + cs->pixelSize() * i, badDst, cs->pixelSize());
            }
        }

        const qreal opacityF = opacity / 255.0;
        op->composite(dstPtr, numTestablePixels * cs->pixelSize(),
                      srcPtr, numTestablePixels * cs->pixelSize(),
                      0, 0,
                      1, numTestablePixels, opacityF);

        if (memcmp(badPixelDstPtr, expectedDst, cs->pixelSize()) != 0) {
            qDebug() << "badPixelPos" << badPixelPos;
            qDebug() << "opacity" << opacity;
            qDebug() << "oriS" << badSrc[0] << badSrc[1] << badSrc[2] << badSrc[3];
            qDebug() << "oriD" << badDst[0] << badDst[1] << badDst[2] << badDst[3];
            qDebug() << "expD" << expectedDst[0] << expectedDst[1] << expectedDst[2] << expectedDst[3];
            qDebug() << "dst1" << badPixelDstPtr[0] << badPixelDstPtr[1] << badPixelDstPtr[2] << badPixelDstPtr[3];
#if !defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
            QFAIL("Failed to compose pixels");
#else
            qWarning() << "Skipping failed test when xsimd library is not used";
#endif
        }
    };

    /**
     * This test is supposed to catch irregularities between vector and scalar versions
     * of the composite op. In vector version we handle division be zero in a relaxed
     * way, so it some cases the content of the color channels may be different from
     * the one of the scalar versions of the algorithm. It is only allowed when the
     * pixel's alpha is null. In such case Krita considers pixel state as "undefined",
     * so any value is considered okay.
     */

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,129};
        quint8 badDst[] = {12,12,12,0};
        quint8 *expectedDst = badSrc;
        testBadPixel(1, goodSrc, goodDst, badSrc, badDst, 255, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,129};
        quint8 badDst[] = {12,12,12,0};
        quint8 *expectedDst = badSrc;
        testBadPixel(10, goodSrc, goodDst, badSrc, badDst, 255, expectedDst);
    }


    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,129};
        quint8 badDst[] = {12,12,12,0};
        quint8 expectedDst[] = {128,128,128,65};
        testBadPixel(1, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,129};
        quint8 badDst[] = {12,12,12,0};
        quint8 expectedDst[] = {128,128,128,65};
        testBadPixel(10, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,0};
        quint8 badDst[] = {12,12,12,0};
        quint8 expectedDst[] = {0,0,0,0}; // NOTE: the pixel has been changed, even though visually it hasn't (due to zero alpha)
        testBadPixel(1, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,0};
        quint8 badDst[] = {12,12,12,0};
        quint8 expectedDst[] = {255,255,255,0}; // NOTE: the result is different from the scalar version, but we don't care, since alpha is still zero
        testBadPixel(10, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,0};
        quint8 badDst[] = {12,12,12,127};
        quint8 expectedDst[] = {12,12,12,63};
        testBadPixel(1, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }

    {
        quint8 goodSrc[] = {128,128,128,129};
        quint8 goodDst[] = {10,10,10,11};
        quint8 badSrc[] = {128,128,128,0};
        quint8 badDst[] = {12,12,12,127};
        quint8 expectedDst[] = {12,12,12,63};
        testBadPixel(10, goodSrc, goodDst, badSrc, badDst, 128, expectedDst);
    }
}

KISTEST_MAIN(KoRgbU8ColorSpaceTester)
