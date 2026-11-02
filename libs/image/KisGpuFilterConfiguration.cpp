/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuFilterConfiguration.h"
#include "KisGpuBlendMode.h"
#include <filter/kis_filter_configuration.h>
#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <cstring>
#include <limits>

std::optional<LibrePaint::Image::Gpu::FilterDefinition> freezeGpuFilterConfiguration(
    const KisFilterConfigurationSP &configuration, const KoColorSpace *colorSpace)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!configuration || !colorSpace || colorSpace->colorModelId() != RGBAColorModelID) return std::nullopt;
    const bool words = colorSpace->colorDepthId() == Integer16BitsColorDepthID;
    if (!words && colorSpace->colorDepthId() != Integer8BitsColorDepthID) return std::nullopt;
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    Gpu::FilterDefinition definition;
    definition.id = configuration->name();
    definition.version = configuration->version();
    definition.xml = configuration->toXML().toUtf8();
    if (definition.xml.isEmpty() || definition.xml.size() > Gpu::FilterDefinition::MaximumXmlBytes) return std::nullopt;
    // The native round trip gives Core-only strings/byte arrays. The retained
    // definition does not keep a color-space object or resource interface.
    auto canonical = configuration->clone();
    if (!canonical->fromXML(QString::fromUtf8(definition.xml), true)) return std::nullopt;
    definition.parameters = canonical->getProperties();
    Gpu::ColorOverlay overlay;
    if (definition.id == QLatin1String("fastcoloroverlay")) {
        const auto mode = Gpu::layerBlendModeFromCompositeOp(configuration->getString("compositeop", COMPOSITE_OVER));
        if (!mode) return std::nullopt;
        overlay.blendMode = *mode;
        overlay.opacity = configuration->getInt("opacity", 100);
        auto color = configuration->getColor("color", KoColor(QColor(62, 140, 236), KoColorSpaceRegistry::instance()->rgb8()));
        color.convertTo(colorSpace);
        if (words) {
            quint16 bgra[4];
            std::memcpy(bgra, color.data(), sizeof(bgra));
            overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
        } else {
            const auto *bgra = color.data();
            overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
        }
    }
    const auto source = colorSpace->lumaCoefficients();
    const auto missing = std::numeric_limits<double>::quiet_NaN();
    const std::array<double, 3> luma = source.size() < 3 ? std::array<double, 3>{missing, missing, missing} :
        std::array<double, 3>{source[0], source[1], source[2]};
    if (!Gpu::freezeFilterSettings(definition.id, definition.parameters, luma, overlay, format, &definition.settings) ||
        !definition.isValid(format)) return std::nullopt;
    return definition;
}
