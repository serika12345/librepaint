/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PIXEL_TOLERANCE_H
#define KIS_GPU_PIXEL_TOLERANCE_H

#include <QByteArray>
#include <QString>
#include <cstring>

// Saved pixels must stay within one 8-bit step of the native CPU engine and of
// other hosts (#107). The arrays hold interleaved four-channel pixels with alpha
// last. All four channels are compared, including RGB under zero alpha.
// Alpha must agree on being zero (#107).
// Returns an empty string when every compared channel is within the step,
// otherwise a description of the first channel outside it.
// bytesPerChannel is 1 for U8 (step 1) or 2 for U16 (step 257).
// channelsPerPixel is 4 for RGBA/BGRA or 1 for Alpha.
inline QString pixelStepDifference(const QByteArray &actual, const QByteArray &expected, int bytesPerChannel = 1, int channelsPerPixel = 4)
{
    if (actual.size() != expected.size())
        return QStringLiteral("size %1 differs from expected %2").arg(actual.size()).arg(expected.size());
    if (bytesPerChannel != 1 && bytesPerChannel != 2)
        return QStringLiteral("unsupported channel size %1").arg(bytesPerChannel);
    if (channelsPerPixel != 1 && channelsPerPixel != 4)
        return QStringLiteral("unsupported pixel channel count %1").arg(channelsPerPixel);
    if (actual.size() % (channelsPerPixel * bytesPerChannel))
        return QStringLiteral("incomplete RGBA pixel: %1 bytes").arg(actual.size());
    const quint32 step = bytesPerChannel == 2 ? 257u : 1u;
    const qsizetype channels = actual.size() / bytesPerChannel;
    const auto value = [bytesPerChannel](const QByteArray &bytes, qsizetype index) -> quint32 {
        if (bytesPerChannel == 2) {
            quint16 word;
            std::memcpy(&word, bytes.constData() + index * 2, 2);
            return word;
        }
        return quint8(bytes[index]);
    };
    for (qsizetype index = 0; index < channels; ++index) {
        const qsizetype alphaIndex = index - index % channelsPerPixel + channelsPerPixel - 1;
        const quint32 a = value(actual, index), e = value(expected, index);
        const quint32 actualAlpha = value(actual, alphaIndex), expectedAlpha = value(expected, alphaIndex);
        if ((actualAlpha == 0) != (expectedAlpha == 0))
            return QStringLiteral("pixel %1: actual alpha %2 expected alpha %3")
                .arg(index / channelsPerPixel).arg(actualAlpha).arg(expectedAlpha);
        if ((a > e ? a - e : e - a) > step)
            return QStringLiteral("channel %1 (pixel %2, component %3): actual %4 expected %5")
                .arg(index).arg(index / channelsPerPixel).arg(index % channelsPerPixel).arg(a).arg(e);
    }
    return {};
}

#define QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(actual, expected, bytesPerChannel) \
    do { \
        const QString difference = pixelStepDifference((actual), (expected), (bytesPerChannel)); \
        QVERIFY2(difference.isEmpty(), qPrintable(difference)); \
    } while (false)

#define QCOMPARE_PIXELS_WITHIN_STEP(actual, expected) \
    do { \
        const QString difference = pixelStepDifference((actual), (expected)); \
        QVERIFY2(difference.isEmpty(), qPrintable(difference)); \
    } while (false)

#define QCOMPARE_PIXELS16_WITHIN_STEP(actual, expected) \
    do { \
        const QString difference = pixelStepDifference((actual), (expected), 2); \
        QVERIFY2(difference.isEmpty(), qPrintable(difference)); \
    } while (false)

#endif
