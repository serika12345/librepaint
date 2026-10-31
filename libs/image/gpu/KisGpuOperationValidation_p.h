/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_VALIDATION_P_H
#define KIS_GPU_OPERATION_VALIDATION_P_H

#include "KisGpuOperationDocument.h"
#include "KisGpuRgba16Compositing_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QSet>
#include <QHash>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
constexpr qsizetype LayerNameByteLimit = 65536;

inline bool validBlendMode(LayerBlendMode mode, PixelFormat format)
{
    if (format == PixelFormat::Rgba16) return mode == LayerBlendMode::Normal || mode == LayerBlendMode::Copy || usesSeparableRgba16Blend(mode) || isCoupledBlend(mode) || usesSpecialAlphaRgba16Blend(mode);
    switch (mode) {
    case LayerBlendMode::Normal:
    case LayerBlendMode::Copy:
    case LayerBlendMode::Multiply:
    case LayerBlendMode::Screen:
    case LayerBlendMode::Darken:
    case LayerBlendMode::Lighten:
    case LayerBlendMode::Addition:
    case LayerBlendMode::LinearDodge:
    case LayerBlendMode::Difference:
    case LayerBlendMode::Exclusion:
    case LayerBlendMode::Subtract:
    case LayerBlendMode::LinearBurn:
    case LayerBlendMode::InverseSubtract:
    case LayerBlendMode::GrainMerge:
    case LayerBlendMode::GrainExtract:
    case LayerBlendMode::Overlay:
    case LayerBlendMode::HardLight:
    case LayerBlendMode::PinLight:
    case LayerBlendMode::Divide:
    case LayerBlendMode::ColorDodge:
    case LayerBlendMode::ColorBurn:
    case LayerBlendMode::HardMix:
    case LayerBlendMode::VividLight:
    case LayerBlendMode::LinearLight:
    case LayerBlendMode::Equivalence:
    case LayerBlendMode::Allanon:
    case LayerBlendMode::HardMixPhotoshop:
    case LayerBlendMode::Erase:
    case LayerBlendMode::Behind:
    case LayerBlendMode::CopyRed:
    case LayerBlendMode::CopyGreen:
    case LayerBlendMode::CopyBlue:
    case LayerBlendMode::BitwiseXor:
    case LayerBlendMode::BitwiseOr:
    case LayerBlendMode::BitwiseAnd:
    case LayerBlendMode::BitwiseNand:
    case LayerBlendMode::BitwiseNor:
    case LayerBlendMode::BitwiseXnor:
    case LayerBlendMode::Implication:
    case LayerBlendMode::NotImplication:
    case LayerBlendMode::Converse:
    case LayerBlendMode::NotConverse:
    case LayerBlendMode::DestinationIn:
    case LayerBlendMode::DestinationAtop:
    case LayerBlendMode::Reflect:
    case LayerBlendMode::Glow:
    case LayerBlendMode::Freeze:
    case LayerBlendMode::Heat:
    case LayerBlendMode::GlowHeat:
    case LayerBlendMode::HeatGlow:
    case LayerBlendMode::ReflectFreeze:
    case LayerBlendMode::FreezeReflect:
    case LayerBlendMode::QuadraticHybrid:
    case LayerBlendMode::Parallel:
    case LayerBlendMode::HardOverlay:
    case LayerBlendMode::SoftLightPegtopDelphi:
    case LayerBlendMode::Negation:
    case LayerBlendMode::PenumbraA:
    case LayerBlendMode::PenumbraB:
    case LayerBlendMode::FlatLight:
    case LayerBlendMode::HardMixSofterPhotoshop:
    case LayerBlendMode::GeometricMean:
    case LayerBlendMode::Interpolation:
    case LayerBlendMode::InterpolationB:
    case LayerBlendMode::PenumbraC:
    case LayerBlendMode::PenumbraD:
    case LayerBlendMode::SoftLightIFSIllusions:
    case LayerBlendMode::SoftLightSvg:
    case LayerBlendMode::SoftLightPhotoshop:
    case LayerBlendMode::GammaLight:
    case LayerBlendMode::GammaIllumination:
    case LayerBlendMode::PNormA:
    case LayerBlendMode::PNormB:
    case LayerBlendMode::SuperLight:
    case LayerBlendMode::TintIFSIllusions:
    case LayerBlendMode::FogLightenIFSIllusions:
    case LayerBlendMode::EasyDodge:
    case LayerBlendMode::GammaDark:
    case LayerBlendMode::ShadeIFSIllusions:
    case LayerBlendMode::FogDarkenIFSIllusions:
    case LayerBlendMode::EasyBurn:
    case LayerBlendMode::Modulo:
    case LayerBlendMode::ModuloContinuous:
    case LayerBlendMode::DivisiveModulo:
    case LayerBlendMode::DivisiveModuloContinuous:
    case LayerBlendMode::ModuloShift:
    case LayerBlendMode::ModuloShiftContinuous:
    case LayerBlendMode::ArcTangent:
    case LayerBlendMode::AdditiveSubtractive:
    case LayerBlendMode::ColorHsy:
    case LayerBlendMode::HueHsy:
    case LayerBlendMode::TintHsy:
    case LayerBlendMode::SaturationHsy:
    case LayerBlendMode::IncreaseSaturationHsy:
    case LayerBlendMode::DecreaseSaturationHsy:
    case LayerBlendMode::LightnessHsy:
    case LayerBlendMode::IncreaseLightnessHsy:
    case LayerBlendMode::DecreaseLightnessHsy:
    case LayerBlendMode::ColorHsi:
    case LayerBlendMode::HueHsi:
    case LayerBlendMode::SaturationHsi:
    case LayerBlendMode::IncreaseSaturationHsi:
    case LayerBlendMode::DecreaseSaturationHsi:
    case LayerBlendMode::LightnessHsi:
    case LayerBlendMode::IncreaseLightnessHsi:
    case LayerBlendMode::DecreaseLightnessHsi:
    case LayerBlendMode::ColorHsl:
    case LayerBlendMode::HueHsl:
    case LayerBlendMode::SaturationHsl:
    case LayerBlendMode::IncreaseSaturationHsl:
    case LayerBlendMode::DecreaseSaturationHsl:
    case LayerBlendMode::LightnessHsl:
    case LayerBlendMode::IncreaseLightnessHsl:
    case LayerBlendMode::DecreaseLightnessHsl:
    case LayerBlendMode::ColorHsv:
    case LayerBlendMode::HueHsv:
    case LayerBlendMode::SaturationHsv:
    case LayerBlendMode::IncreaseSaturationHsv:
    case LayerBlendMode::DecreaseSaturationHsv:
    case LayerBlendMode::LightnessHsv:
    case LayerBlendMode::IncreaseLightnessHsv:
    case LayerBlendMode::DecreaseLightnessHsv:
    case LayerBlendMode::DarkerColor:
    case LayerBlendMode::LighterColor:
    case LayerBlendMode::TangentNormalMap:
    case LayerBlendMode::ReorientedNormalMap:
    case LayerBlendMode::LambertLighting:
    case LayerBlendMode::LambertLightingGamma22:
    case LayerBlendMode::Marker:
    case LayerBlendMode::Greater:
    case LayerBlendMode::LuminositySai:
        return true;
    }
    return false;
}

template<class LayerValue>
bool validLayerHierarchy(const QList<LayerValue> &layers, PixelFormat format)
{
    QHash<QUuid, const LayerValue *> identities;
    for (const auto &layer : layers) {
        if (layer.kind != LayerKind::Paint && layer.kind != LayerKind::Group) return false;
        if (layer.kind != LayerKind::Group && layer.passThrough) return false;
        if (!validBlendMode(layer.blendMode, format)) return false;
        if (layer.kind == LayerKind::Group && layer.transparencyMask) return false;
        if (layer.transparencyMask && layer.transparencyMask->byteCount() > SelectionMask::MaximumBytes) return false;
        if (!layer.uuid.isNull()) {
            if (identities.contains(layer.uuid)) return false;
            identities.insert(layer.uuid, &layer);
        }
    }
    for (const auto &layer : layers) {
        if (layer.parentUuid.isNull()) continue;
        const auto parent = identities.constFind(layer.parentUuid);
        if (parent == identities.cend() || (*parent)->kind != LayerKind::Group) return false;
    }
    QHash<QUuid, quint8> colours;
    for (auto entry = identities.cbegin(); entry != identities.cend(); ++entry) {
        auto current = entry.key();
        while (!current.isNull() && colours.value(current) == 0) {
            colours[current] = 1;
            current = identities.value(current)->parentUuid;
        }
        if (!current.isNull() && colours.value(current) == 1) return false;
        current = entry.key();
        while (!current.isNull() && colours.value(current) == 1) {
            colours[current] = 2;
            current = identities.value(current)->parentUuid;
        }
    }
    return true;
}

inline bool validCropRect(qint32 x, qint32 y, qint32 width, qint32 height)
{
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    return x >= -limit && x <= limit && y >= -limit && y <= limit &&
        qint64(x) + width >= -limit && qint64(x) + width <= limit &&
        qint64(y) + height >= -limit && qint64(y) + height <= limit;
}
inline bool validCropRect(const QRect &rect)
{ return validCropRect(rect.x(), rect.y(), rect.width(), rect.height()); }
/** Null/empty rectangles disable selection; nonempty rectangles use positive
 * extents inside the absolute document coordinate limit.
 */
inline bool validSelectionRect(const QRect &rect)
{
    if (rect.isEmpty()) return rect.x() == 0 && rect.y() == 0;
    return validCropRect(rect) && rect.width() > 0 && rect.height() > 0;
}
/** Rectangular and elliptical kinds carry no coverage; Freehand requires a
 * nonempty mask whose tight bounds equal the recorded selection rectangle.
 */
inline bool validSelectionState(const QRect &rect, SelectionKind kind, const SelectionMaskSP &mask)
{
    if (kind != SelectionKind::Rectangle && kind != SelectionKind::Ellipse && kind != SelectionKind::Freehand)
        return false;
    if (kind == SelectionKind::Freehand)
        return mask && !mask->defaultCoverage() && !mask->isEmpty() && !rect.isEmpty() && rect == mask->bounds() &&
            mask->byteCount() <= SelectionMask::MaximumBytes;
    return !mask;
}
inline bool validCanvasRect(const QRect &rect)
{
    return validCropRect(rect) && rect.width() > 0 && rect.height() > 0 &&
        rect.width() <= OperationDocument::CoordinateLimit && rect.height() <= OperationDocument::CoordinateLimit;
}

// A shifted 64-pixel history grid cuts a 256-pixel GPU tile into at most 5 by 5 pieces.
constexpr qsizetype PixelWriteAreasPerTile = 25;

inline bool validPixelWriteArea(qint32 x, qint32 y, qint32 width, qint32 height)
{
    if (width <= 0 || height <= 0 || width > PixelHistoryBlockSize || height > PixelHistoryBlockSize ||
        x < -RgbaTile::CoordinateLimit || y < -RgbaTile::CoordinateLimit ||
        qint64(x) + width > RgbaTile::CoordinateLimit || qint64(y) + height > RgbaTile::CoordinateLimit) return false;
    if (pixelTileIndex(x, RgbaTile::Size) != pixelTileIndex(x + width - 1, RgbaTile::Size) ||
        pixelTileIndex(y, RgbaTile::Size) != pixelTileIndex(y + height - 1, RgbaTile::Size)) return false;
    return true;
}

inline bool validWrittenAreas(const TileWriteOperation &write)
{
    if (write.writtenAreas.isEmpty()) return write.writeGroup == 0;
    if (!write.writeGroup || write.writeGroup > write.id.sequence ||
        write.writtenAreas.size() > write.tiles.size() * PixelWriteAreasPerTile) return false;
    QSet<QPoint> tiles, referencedTiles;
    QHash<QPoint, QList<QRect>> occupied;
    for (const auto &tile : write.tiles) tiles.insert(tile->coordinate());
    for (const auto &area : write.writtenAreas) {
        if (!validPixelWriteArea(area.x(), area.y(), area.width(), area.height())) return false;
        const QPoint tile(pixelTileIndex(area.left(), RgbaTile::Size), pixelTileIndex(area.top(), RgbaTile::Size));
        if (!tiles.contains(tile)) return false;
        auto &areas = occupied[tile];
        if (areas.size() >= PixelWriteAreasPerTile) return false;
        for (const auto &previous : areas) if (previous.intersects(area)) return false;
        areas.append(area);
        referencedTiles.insert(tile);
    }
    return referencedTiles == tiles;
}

inline bool validLayerName(const QString &name)
{
    if (name.size() > LayerNameByteLimit) return false;
    const auto bytes = name.toUtf8();
    return bytes.size() <= LayerNameByteLimit && QString::fromUtf8(bytes) == name;
}

inline bool validPoint(const QPointF &point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y()) &&
        std::abs(point.x()) <= OperationDocument::CoordinateLimit &&
        std::abs(point.y()) <= OperationDocument::CoordinateLimit;
}

inline bool validSample(const InputSample &sample)
{
    return validPoint(sample.position) && std::isfinite(sample.pressure) &&
        sample.pressure >= 0 && sample.pressure <= 1 &&
        std::isfinite(sample.xTilt) && std::abs(sample.xTilt) <= 90 &&
        std::isfinite(sample.yTilt) && std::abs(sample.yTilt) <= 90 &&
        std::isfinite(sample.rotation) && std::isfinite(sample.speed) && sample.speed >= 0 && sample.speed <= 1 &&
        std::isfinite(sample.canvasRotation) && std::abs(sample.canvasRotation) <= 360;
}

inline bool validPressureResponse(const PressureResponse &value)
{
    if (!std::isfinite(value.strength) || value.strength < 0 || value.strength > 1 ||
        value.kind < PressureResponse::Kind::Constant || value.kind > PressureResponse::Kind::Curve) return false;
    if (value.kind != PressureResponse::Kind::Curve) return value.curve.isEmpty();
    if (value.curve.size() != 256) return false;
    for (const auto number : value.curve)
        if (!std::isfinite(number) || number < 0 || number > 1) return false;
    return true;
}

inline bool validBrush(const Brush &brush, PixelFormat format)
{
    if (brush.additiveMaskingBrush) {
        const auto &mask = *brush.additiveMaskingBrush;
        if (!brush.autoBrush || !mask.autoBrush || mask.additiveMaskingBrush || mask.texture ||
            !mask.maskPainting || mask.rgba != RgbaPixel{255, 255, 255, 255} ||
            mask.opacity != 1 || mask.erase || mask.alphaLocked ||
            mask.blendMode != LayerBlendMode::Normal || mask.channelWriteMask != 15 ||
            !validBrush(mask, PixelFormat::Rgba8)) return false;
    }
    if (brush.texture) {
        const auto &t = *brush.texture;
        const qint64 pixels = qint64(t.size.width()) * t.size.height();
        if (!brush.autoBrush || t.size.width() <= 0 || t.size.height() <= 0 || pixels > 16 * 1024 * 1024 ||
            t.alpha.size() != ((pixels + 3) & ~qint64(3)) ||
            t.mode < BrushTexture::Mode::Multiply || t.mode > BrushTexture::Mode::Darken ||
            !std::isfinite(t.strength) || t.strength < 0 || t.strength > 1 ||
            (t.pressure && !validPressureResponse(*t.pressure)) ||
            (t.fade && !validPressureResponse(*t.fade)) || t.fadeLength <= 0 || t.curveMode < 0 || t.curveMode > 4 ||
            std::abs(qint64(t.offset.x())) > OperationDocument::CoordinateLimit ||
            std::abs(qint64(t.offset.y())) > OperationDocument::CoordinateLimit) return false;
    }
    if (brush.maskPainting && format != PixelFormat::Rgba8) return false;
    if (!validPressureResponse(brush.sizeResponse) || !std::isfinite(brush.autoSpacingCoefficient) ||
        brush.autoSpacingCoefficient < 0) return false;
    if (!std::isfinite(brush.opacity) || brush.opacity < 0 || brush.opacity > 1) return false;
    if (!validPixel(brush.rgba, format)) return false;
    if (brush.autoBrush) {
        const auto &value = *brush.autoBrush;
        if (value.precisionLevel < 1 || value.precisionLevel > 5 ||
            !std::isfinite(value.angle) || !std::isfinite(value.flow) || value.flow < 0 || value.flow > 1 ||
            !validPressureResponse(value.opacityResponse) || !validPressureResponse(value.flowResponse) ||
            (value.sharpnessResponse && !validPressureResponse(*value.sharpnessResponse)) ||
            value.sharpnessSoftness < 0 || value.sharpnessSoftness > 100 ||
            (value.mask != AutoBrush::Mask::GaussianCircle && value.mask != AutoBrush::Mask::Circle &&
             value.mask != AutoBrush::Mask::Rectangle) ||
            !std::isfinite(value.fade) || value.fade < 0 || value.fade > 1 ||
            (value.verticalFade && (!std::isfinite(*value.verticalFade) || *value.verticalFade < 0 || *value.verticalFade > 1)) ||
            !std::isfinite(value.ratio) || value.ratio <= 0 || value.ratio > 1 ||
            (value.mask == AutoBrush::Mask::GaussianCircle && value.antialiasEdges && value.ratio != 1)) return false;
    }
    if (brush.weightedSmoothing) {
        const auto &value = *brush.weightedSmoothing;
        if (!brush.basicSmoothing || !std::isfinite(value.distanceMin) || !std::isfinite(value.distanceMax) ||
            value.distanceMin < 0 || value.distanceMax < 0 ||
            value.distanceMin > OperationDocument::CoordinateLimit || value.distanceMax > OperationDocument::CoordinateLimit ||
            !std::isfinite(value.tailAggressiveness) || value.tailAggressiveness < 0 || value.tailAggressiveness > 1) return false;
    }
    return !(brush.basicSmoothing && brush.pixelPerfect) &&
        std::abs(qint64(brush.mirrorCenter.x())) <= OperationDocument::CoordinateLimit &&
        std::abs(qint64(brush.mirrorCenter.y())) <= OperationDocument::CoordinateLimit &&
        validBlendMode(brush.blendMode, format) &&
        (!brush.erase || brush.blendMode == LayerBlendMode::Normal) &&
        brush.channelWriteMask <= 15 && std::isfinite(brush.diameter) && brush.diameter > 0 &&
        brush.diameter <= OperationDocument::CoordinateLimit &&
        std::isfinite(brush.spacing) && brush.spacing > 0 && brush.spacing <= 1;
}

inline bool validShape(const Shape &shape)
{
    if (!validPoint(shape.first) || !validPoint(shape.second) || !std::isfinite(shape.rotation) ||
        !std::isfinite(shape.cornerRadii.width()) || !std::isfinite(shape.cornerRadii.height()) ||
        shape.cornerRadii.width() < 0 || shape.cornerRadii.height() < 0 ||
        shape.cornerRadii.width() > OperationDocument::CoordinateLimit ||
        shape.cornerRadii.height() > OperationDocument::CoordinateLimit ||
        (shape.kind != ShapeKind::RoundedRectangle && shape.cornerRadii != QSizeF(0, 0)) ||
        (shape.kind != ShapeKind::Path &&
         (((shape.kind != ShapeKind::Ellipse && shape.kind != ShapeKind::Rectangle && shape.kind != ShapeKind::RoundedRectangle) &&
           !shape.pathCommands.isEmpty()) || shape.windingFill || !shape.pathCalls.isEmpty()))) {
        return false;
    }
    if (shape.kind == ShapeKind::Path || !shape.pathCommands.isEmpty()) {
        if (!shape.pathCommands.isEmpty() && shape.pathCommands.first() != PathCommand::MoveTo &&
            shape.pathCommands.first() != PathCommand::Point) return false;
        for (const auto command : shape.pathCommands) {
            if (command == PathCommand::Point) {
                if (shape.kind != ShapeKind::Path || !shape.pathCalls.isEmpty()) return false;
            } else if (command != PathCommand::MoveTo && command != PathCommand::LineTo && command != PathCommand::CubicTo) return false;
        }
        qsizetype previousEnd = 0;
        for (const auto &call : shape.pathCalls) {
            if (call.first < previousEnd || call.second <= call.first) return false;
            previousEnd = call.second;
        }
    }
    switch (shape.kind) {
    case ShapeKind::Freehand:
    case ShapeKind::Line:
    case ShapeKind::Polyline:
    case ShapeKind::Polygon:
    case ShapeKind::Path:
        return true;
    case ShapeKind::Ellipse:
    case ShapeKind::Rectangle:
    case ShapeKind::RoundedRectangle:
        return shape.first.x() != shape.second.x() && shape.first.y() != shape.second.y();
    }
    return false;
}

inline bool validShapeSampleCount(const Shape &shape, qsizetype count)
{
    if (shape.kind == ShapeKind::Path || !shape.pathCommands.isEmpty()) {
        qsizetype position = 0, call = 0;
        bool insideCall = false;
        for (const auto command : shape.pathCommands) {
            if (call < shape.pathCalls.size() && position == shape.pathCalls[call].first) {
                if (command != PathCommand::MoveTo) return false;
                insideCall = true;
            }
            const qsizetype points = command == PathCommand::CubicTo ? 3 : 1;
            if (count < points) return false;
            count -= points;
            position += points;
            if (call < shape.pathCalls.size()) {
                const auto &range = shape.pathCalls[call];
                if ((!insideCall && range.first < position) || (insideCall && range.second < position)) return false;
                if (insideCall && range.second == position) { ++call; insideCall = false; }
            }
        }
        return count == 0 && call == shape.pathCalls.size() && !insideCall;
    }
    return (shape.kind != ShapeKind::Polyline && shape.kind != ShapeKind::Polygon) || count >= 2;
}

inline bool validFigureStyle(const Shape &shape, const FigureStyle &style, PixelFormat format)
{
    if (hasFigureFill(style) && shape.pathCommands.contains(PathCommand::Point)) return false;
    if (style.pattern && !style.pattern->isValid()) return false;
    if (style.pattern && style.pattern->format == PixelFormat::Rgba16 && format != PixelFormat::Rgba16) return false;
    if (!validBlendMode(style.fillBlendMode, format)) return false;
    // A non-normal fill mode owns only the fill source. Outline brush
    // compositing stays source-over, so this combination is rejected rather
    // than silently blending the outline.
    if (style.fillBlendMode != LayerBlendMode::Normal && (!hasFigureFill(style) || style.outline)) return false;
    return !hasFigureFill(style) || (shape.kind != ShapeKind::Freehand && shape.kind != ShapeKind::Line);
}
}
#endif
