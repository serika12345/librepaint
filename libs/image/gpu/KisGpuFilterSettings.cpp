/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu {
namespace {
std::optional<FilterKind> nativeKind(const QString &id)
{
    if (id == QLatin1String("invert")) return FilterKind::InvertColors;
    if (id == QLatin1String("hsvadjustment")) return FilterKind::HsvAdjustment;
    if (id == QLatin1String("maximize")) return FilterKind::MaximizeChannel;
    if (id == QLatin1String("minimize")) return FilterKind::MinimizeChannel;
    if (id == QLatin1String("fastcoloroverlay")) return FilterKind::ColorOverlay;
    return std::nullopt;
}
}

bool FilterSettings::isValid(PixelFormat format) const
{
    if (!pixelByteCount(format) || kind > FilterKind::ColorOverlay) return false;
    if (format == PixelFormat::Rgba16 && kind < FilterKind::MaximizeChannel) return false;
    if (kind == FilterKind::ColorOverlay) return overlay.isValid(format);
    if (kind != FilterKind::HsvAdjustment) return true;
    HsvAdjustment hsv{hue, saturation, value, hsvType, compatibilityMode};
    hsv.colorize = colorize;
    hsv.luma = luma;
    hsv.lumaFrozen = true;
    return hsv.isValid();
}

bool FilterSettings::operator==(const FilterSettings &other) const
{
    return kind == other.kind && hue == other.hue && saturation == other.saturation &&
        value == other.value && hsvType == other.hsvType && compatibilityMode == other.compatibilityMode &&
        colorize == other.colorize && luma == other.luma && overlay.color == other.overlay.color &&
        overlay.opacity == other.overlay.opacity && overlay.blendMode == other.overlay.blendMode;
}

bool FilterDefinition::isValid(PixelFormat format) const
{
    const auto kind = nativeKind(id);
    return resolved && kind && *kind == settings.kind && version >= 0 &&
        !xml.isEmpty() && xml.size() <= MaximumXmlBytes && parameters.size() <= 256 && settings.isValid(format);
}

qsizetype FilterDefinition::byteCount() const
{
    qsizetype bytes = sizeof(FilterDefinition) + xml.size() + id.size() * qsizetype(sizeof(QChar));
    for (auto i = parameters.cbegin(); i != parameters.cend(); ++i) {
        bytes += sizeof(QVariant) + (i.key().size() + i->toString().size()) * qsizetype(sizeof(QChar));
        if (i->metaType().id() == QMetaType::QByteArray) bytes += i->toByteArray().size();
    }
    return bytes;
}

bool FilterDefinition::operator==(const FilterDefinition &other) const
{
    return id == other.id && version == other.version && xml == other.xml &&
        parameters == other.parameters && settings == other.settings && resolved == other.resolved;
}

bool freezeFilterSettings(const QString &id, const QMap<QString, QVariant> &parameters,
    const std::array<double, 3> &luma, const ColorOverlay &overlay, PixelFormat format, FilterSettings *settings)
{
    const auto kind = nativeKind(id);
    if (!settings || !kind) return false;
    FilterSettings result;
    result.kind = *kind;
    if (*kind == FilterKind::HsvAdjustment) {
        HsvAdjustment hsv{parameters.value(QStringLiteral("h"), 0).toInt(),
                          parameters.value(QStringLiteral("s"), 0).toInt(),
                          parameters.value(QStringLiteral("v"), 0).toInt(),
                          HsvAdjustmentType(parameters.value(QStringLiteral("type"), 1).toInt()),
                          parameters.value(QStringLiteral("compatibilityMode"), true).toBool()};
        const int type = parameters.value(QStringLiteral("type"), 1).toInt();
        if (type < 0 || type > 4) return false;
        hsv.colorize = parameters.value(QStringLiteral("colorize"), false).toBool();
        if (hsv.needsLuma()) {
            if (!freezeLumaCoefficients(luma, &hsv.luma)) return false;
            hsv.lumaFrozen = true;
        }
        if (!hsv.isValid()) return false;
        result.hue = hsv.hue;
        result.saturation = hsv.saturation;
        result.value = hsv.value;
        result.hsvType = hsv.type;
        result.compatibilityMode = hsv.compatibilityMode;
        result.colorize = hsv.colorize;
        result.luma = hsv.luma;
    } else if (*kind == FilterKind::ColorOverlay) {
        result.overlay = overlay;
    }
    if (!result.isValid(format)) return false;
    *settings = result;
    return true;
}
}
