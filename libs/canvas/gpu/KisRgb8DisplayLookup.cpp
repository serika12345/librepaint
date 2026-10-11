/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisRgb8DisplayLookup.h"
#include <KoColorModelStandardIds.h>
#include <KoColorSpace.h>
#include <algorithm>

namespace KisRgb8DisplayLookup {
Result build(const KoColorSpace *source, const KoColorSpace *destination,
             KoColorConversionTransformation::Intent intent,
             KoColorConversionTransformation::ConversionFlags flags,
             const std::atomic_bool &cancelled)
{
    if (cancelled.load()) return {Status::Cancelled, {}};
    const auto supported = [](const KoColorSpace *space) {
        return space && space->colorModelId() == RGBAColorModelID
            && space->colorDepthId() == Integer8BitsColorDepthID && space->pixelSize() == 4;
    };
    if (!supported(source) || !supported(destination)) return {Status::InvalidColorSpace, {}};
    if (*source == *destination) return {Status::Ready, {}};
    std::unique_ptr<KoColorConversionTransformation> converter(
        source->createColorConverter(destination, intent, flags));
    if (!converter || !converter->isValid()) return {Status::ConversionFailed, {}};
    constexpr qsizetype PlaneBytes = 256 * 256 * 4;
    QByteArray values(256 * PlaneBytes, 0), input(PlaneBytes, 0);
    auto *pixels = reinterpret_cast<quint8 *>(input.data());
    bool identity = true;
    for (int blue = 0; blue < 256; ++blue) {
        if (cancelled.load()) return {Status::Cancelled, {}};
        for (int green = 0; green < 256; ++green) {
            for (int red = 0; red < 256; ++red) {
                const int offset = (red + 256 * green) * 4;
                pixels[offset] = quint8(blue);
                pixels[offset + 1] = quint8(green);
                pixels[offset + 2] = quint8(red);
                pixels[offset + 3] = 255;
            }
        }
        auto *output = reinterpret_cast<quint8 *>(values.data() + blue * PlaneBytes);
        converter->transform(pixels, output, 256 * 256);
        for (qsizetype i = 0; i < PlaneBytes; i += 4) {
            identity = identity && output[i] == pixels[i]
                && output[i + 1] == pixels[i + 1] && output[i + 2] == pixels[i + 2];
            std::swap(output[i], output[i + 2]);
        }
    }
    if (cancelled.load()) return {Status::Cancelled, {}};
    return {Status::Ready, identity ? nullptr : std::make_shared<const QByteArray>(std::move(values))};
}
}
