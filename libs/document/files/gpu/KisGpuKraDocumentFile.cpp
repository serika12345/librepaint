/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../../gpu/KisGpuPaintDocument_p.h"
#include <kra/KisKraRasterCodec.h>
#include <QFile>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <KisGpuBlendMode.h>
#include <KoColorModelStandardIds.h>
#include <QDomDocument>
#include <limits>
#include <algorithm>
#include <cstring>
#include <QSaveFile>
#include <QHash>
#include <stdexcept>
#include <new>

namespace LibrePaint::Document::Gpu {
using namespace Image::Gpu;
namespace Kra = ImportExport::Kra;
namespace {
FileCode fileCode(Kra::Status code)
{
    switch (code) {
    case Kra::Status::Success: return FileCode::Success;
    case Kra::Status::InvalidData: return FileCode::InvalidData;
    case Kra::Status::UnsupportedData: return FileCode::UnsupportedData;
    case Kra::Status::CapacityExceeded: return FileCode::CapacityExceeded;
    case Kra::Status::IoFailure: return FileCode::IoFailure;
    }
    return FileCode::InvalidData;
}
FileCode fileCode(ExecutionCode code)
{
    switch (code) {
    case ExecutionCode::Success: return FileCode::Success;
    case ExecutionCode::InvalidRequest: return FileCode::InvalidData;
    case ExecutionCode::CapacityExceeded: return FileCode::CapacityExceeded;
    case ExecutionCode::Busy: return FileCode::Busy;
    case ExecutionCode::DeviceFailure:
    case ExecutionCode::SurfaceOutOfDate: return FileCode::DeviceFailure;
    }
    return FileCode::DeviceFailure;
}

RasterLayer rasterLayerDescription(const Layer &layer)
{
    RasterLayer value{layer.name, layer.visible, layer.uuid, {}, layer.defaultPixel, layer.opacity,
                      layer.userLocked, layer.pinnedToTimeline, layer.channelWriteMask};
    value.position = layer.position;
    value.kind = layer.kind;
    value.parentUuid = layer.parentUuid;
    value.colorLabelIndex = layer.colorLabelIndex;
    value.projectionChannelMask = layer.projectionChannelMask;
    value.passThrough = layer.passThrough;
    value.blendMode = layer.blendMode;
    value.transparencyMask = layer.transparencyMask;
    value.transparencyMaskPosition = layer.transparencyMaskPosition;
    value.transparencyMaskVisible = layer.transparencyMaskVisible;
    value.transparencyMaskLocked = layer.transparencyMaskLocked;
    value.transparencyMaskUuid = layer.transparencyMaskUuid;
    value.transparencyMaskName = layer.transparencyMaskName;
    value.transparencyMaskColorLabelIndex = layer.transparencyMaskColorLabelIndex;
    value.transparencyMaskPinnedToTimeline = layer.transparencyMaskPinnedToTimeline;
    value.transparencyMaskIndex = layer.transparencyMaskIndex;
    value.filter = layer.filter;
    value.filterSelection = layer.filterSelection;
    value.generator = layer.generator;
    return value;
}
ExecutionOutcome readKraMaskPixels(Execution &execution, RasterImage &image, qsizetype maximumBytes, qsizetype *bytes)
{
    try {
        qsizetype rasterBytes = 0;
        QHash<const SelectionMask *, SelectionMaskSP> externalMasks;
        ExecutionOutcome maskRead{ExecutionCode::Success, {}};
        const auto externalMask = [&](const SelectionMaskSP &mask) -> SelectionMaskSP {
            if (!mask || !mask->isGpuGenerated()) return mask;
            const auto cached = externalMasks.constFind(mask.data());
            if (cached != externalMasks.cend()) return cached.value();
            SelectionTileMap tiles;
            const auto bounds = mask->bounds();
            const auto coordinates = mask->tileCoordinates();
            if (!bounds.isEmpty()) {
                const quint64 bytes = quint64(coordinates.size()) * SelectionMask::AlphaTileBytes;
                if (bytes > quint64(qMax<qsizetype>(0, maximumBytes - rasterBytes)) || bytes > quint64(SelectionMask::MaximumBytes)) {
                    maskRead = {ExecutionCode::CapacityExceeded, QStringLiteral("KRA selection pixels exceed the raster capacity")};
                    return {};
                }
                for (const auto point : coordinates) {
                    const auto read = execution.readSelectionPixels(mask,
                        QRect(point * SelectionMask::TileSize, QSize(SelectionMask::TileSize, SelectionMask::TileSize)),
                        SelectionMask::AlphaTileBytes);
                    if (read.code != ExecutionCode::Success) { maskRead = {read.code, read.diagnostic}; return {}; }
                    tiles.insert({point.x(), point.y()}, read.bytes);
                }
                rasterBytes += qsizetype(bytes);
            }
            const auto literal = SelectionMask::fromAlphaTiles(tiles, mask->defaultCoverage());
            const auto allocated = literal ? literal->withAllocatedBlocks(mask->allocatedBlocks()) : SelectionMaskSP{};
            const auto result = allocated ? allocated->withVectorSelection(mask->vectorSelection()) : SelectionMaskSP{};
            if (!result || result->bounds() != bounds) {
                maskRead = {ExecutionCode::DeviceFailure, QStringLiteral("KRA selection pixels disagree with the frozen bounds")};
                return {};
            }
            externalMasks.insert(mask.data(), result);
            return result;
        };
        image.selectionMask = externalMask(image.selectionMask);
        if (maskRead.code != ExecutionCode::Success) return maskRead;
        for (auto &layer : image.layers) {
            layer.transparencyMask = externalMask(layer.transparencyMask);
            if (maskRead.code != ExecutionCode::Success) return maskRead;
            layer.filterSelection = externalMask(layer.filterSelection);
            if (maskRead.code != ExecutionCode::Success) return maskRead;
        }
        if (bytes) *bytes = rasterBytes;
        return {ExecutionCode::Success, {}};
    } catch (const std::bad_alloc &) {
        return {ExecutionCode::CapacityExceeded, QStringLiteral("KRA selection read allocation failed")};
    }
}

bool resolveKraFilters(RasterImage &image)
{
    if (std::none_of(image.layers.cbegin(), image.layers.cend(), [](const auto &layer) { return bool(layer.filter); })) return true;
    const auto registry = KoColorSpaceRegistry::instance();
    const auto depth = image.format == PixelFormat::Rgba16 ? Integer16BitsColorDepthID.id() : Integer8BitsColorDepthID.id();
    const auto standard = image.format == PixelFormat::Rgba16 ? registry->rgb16() : registry->rgb8();
    if (!standard) return false;
    const auto profile = image.colorProfile.isEmpty() ? standard->profile() :
        registry->createColorProfileFromData(RGBAColorModelID.id(), depth, image.colorProfile);
    const auto space = profile ? registry->colorSpace(RGBAColorModelID.id(), depth, profile) : nullptr;
    if (!space) return false;
    const auto coefficients = space->lumaCoefficients();
    const auto missing = std::numeric_limits<double>::quiet_NaN();
    const std::array<double, 3> luma = coefficients.size() < 3 ? std::array<double, 3>{missing, missing, missing} :
        std::array<double, 3>{coefficients[0], coefficients[1], coefficients[2]};
    for (auto &layer : image.layers) {
        if (!layer.filter) continue;
        auto &filter = *layer.filter;
        ColorOverlay overlay;
        if (filter.id == QLatin1String("fastcoloroverlay")) {
            const auto mode = layerBlendModeFromCompositeOp(filter.parameters.value(QStringLiteral("compositeop"), COMPOSITE_OVER).toString());
            if (!mode) return false;
            overlay.blendMode = *mode;
            overlay.opacity = filter.parameters.value(QStringLiteral("opacity"), 100).toInt();
            KoColor color(QColor(62, 140, 236), registry->rgb8());
            if (filter.parameters.contains(QStringLiteral("color"))) {
                const auto text = filter.parameters.value(QStringLiteral("color")).toString();
                QDomDocument xml;
                if (xml.setContent(text)) {
                    bool ok = false;
                    color = KoColor::fromXML(xml.documentElement().firstChildElement(), Integer16BitsColorDepthID.id(), &ok);
                    if (!ok) return false;
                } else {
                    const QColor encoded(text);
                    if (!encoded.isValid()) return false;
                    color = KoColor(encoded, registry->rgb8());
                }
            }
            color.convertTo(space);
            if (image.format == PixelFormat::Rgba16) {
                quint16 bgra[4];
                std::memcpy(bgra, color.data(), sizeof(bgra));
                overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
            } else {
                const auto *bgra = color.data();
                overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
            }
        }
        if (!freezeFilterSettings(filter.id, filter.parameters, luma, overlay, image.format, &filter.settings)) return false;
        filter.resolved = true;
        if (!filter.isValid(image.format)) return false;
    }
    return true;
}

}

FileOpenOutcome PaintDocument::openKra(Execution &execution, const QString &path, Limits limits)
{ return openKraImpl(execution, {}, path, limits); }

FileOpenOutcome PaintDocument::openKra(std::shared_ptr<Execution> execution, const QString &path, Limits limits)
{
    if (!execution) return {FileCode::InvalidData, {}, QStringLiteral("Opening requires a GPU execution owner")};
    return openKraImpl(*execution, execution, path, limits);
}

FileOpenOutcome PaintDocument::openKra(std::shared_ptr<Execution> execution, const Kra::Document &document, Limits limits)
{
    if (!execution) return {FileCode::InvalidData, {}, QStringLiteral("Opening requires a GPU execution owner")};
    return adoptKraImpl(*execution, execution, document, {}, limits);
}

FileOpenOutcome PaintDocument::openKraImpl(Execution &execution, std::shared_ptr<Execution> retainedExecution,
                                          const QString &path, Limits limits)
{
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return {FileCode::IoFailure, {}, input.errorString()};
    const auto loaded = Kra::RasterCodec::read(input, Private::formatLimits(limits));
    if (loaded.code != Kra::Status::Success) return {fileCode(loaded.code), {}, loaded.diagnostic};
    return adoptKraImpl(execution, retainedExecution, loaded.document, path, limits);
}

FileOpenOutcome PaintDocument::adoptKraImpl(Execution &execution, std::shared_ptr<Execution> retainedExecution,
                                           const Kra::Document &loaded, const QString &path, Limits limits)
{
    const auto metadata = Kra::RasterCodec::validateMetadata(loaded.description, loaded.files, Private::formatLimits(limits));
    if (metadata.code != Kra::Status::Success) return {fileCode(metadata.code), {}, metadata.diagnostic};
    try {
        auto raster = loaded.image;
        if (!resolveKraFilters(raster))
            return {FileCode::UnsupportedData, {}, QStringLiteral("KRA filter color or numeric settings are unavailable")};
        const auto image = retainedExecution ? PaintImageSP::create(retainedExecution, raster, limits) :
                                              PaintImageSP::create(execution, raster, limits);
        auto document = std::make_unique<PaintDocument>(image, limits);
        document->m_d->path = path;
        document->m_d->description = loaded.description;
        document->m_d->files = loaded.files;
        document->m_d->compressRaster = loaded.compressRaster;
        return {FileCode::Success, std::move(document), {}};
    } catch (const std::length_error &error) {
        return {FileCode::CapacityExceeded, {}, QString::fromUtf8(error.what())};
    } catch (const std::invalid_argument &error) {
        return {FileCode::InvalidData, {}, QString::fromUtf8(error.what())};
    }
}

Kra::MetadataOutcome PaintDocument::kraImageMetadata() const
{
    if (!onThread()) return {Kra::Status::InvalidData, {}, QStringLiteral("Metadata reads require the document thread")};
    return Kra::RasterCodec::readImageMetadata(m_d->description, m_d->files, Private::formatLimits(m_d->limits));
}

FileOutcome PaintDocument::saveKra(const QString &path, const Snapshot &version, const QImage &preview)
{ return writeKra(path, version, true, preview); }

FileOutcome PaintDocument::setKraImageMetadata(const Kra::ImageMetadata &metadata)
{
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Metadata updates require the document thread")};
    if (!m_d->image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    Kra::Document value;
    value.description = m_d->description;
    value.files = m_d->files;
    value.image.format = m_d->image->operations().snapshot()->format;
    value.image.colorProfile = m_d->image->operations().snapshot()->colorProfile;
    value.image.projectionBackground = m_d->image->operations().snapshot()->projectionBackground;
    if (metadata.layerXmp || metadata.layerCollapsed) {
        const auto version = m_d->image->operations().snapshot();
        value.image.size = version->size;
        value.image.projectionBackground = version->projectionBackground;
        value.image.selection = version->selection;
        value.image.selectionInverted = version->selectionInverted;
        value.image.selectionKind = version->selectionKind;
        value.image.selectionMask = version->selectionMask;
        for (const auto &layer : version->layers) {
            if (layer.id == version->activeLayer) value.image.activeLayer = value.image.layers.size();
            value.image.layers.append(rasterLayerDescription(layer));
        }
        const auto masks = readKraMaskPixels(m_d->execution, value.image, Private::formatLimits(m_d->limits).rasterBytes, nullptr);
        if (masks.code != ExecutionCode::Success) return {fileCode(masks.code), {}, masks.diagnostic};
    }
    const auto outcome = Kra::RasterCodec::updateImageMetadata(value, metadata, Private::formatLimits(m_d->limits));
    if (outcome.code != Kra::Status::Success) return {fileCode(outcome.code), {}, outcome.diagnostic};
    m_d->description = std::move(value.description);
    m_d->files = std::move(value.files);
    m_d->compressRaster = value.compressRaster;
    return {FileCode::Success, m_d->image->operations().snapshot(), {}};
}

FileOutcome PaintDocument::exportKra(const QString &path, const Snapshot &version, const QImage &preview)
{ return writeKra(path, version, false, preview); }

FileOutcome PaintDocument::writeKra(const QString &path, const Snapshot &version, bool adoptFile, const QImage &preview)
{
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires the document thread")};
    if (!m_d->image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    if (path.isEmpty()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires a path")};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {FileCode::IoFailure, {}, file.errorString()};
    const auto written = exportKra(file, version, preview);
    if (written.code != FileCode::Success) { file.cancelWriting(); return written; }
    if (!file.commit()) return {FileCode::IoFailure, {}, file.errorString()};
    if (adoptFile) return adoptSavedFile(path, written.version);
    return written;
}

FileOutcome PaintDocument::exportKra(QIODevice &destination, const Snapshot &version, const QImage &preview)
{
    auto &d = *m_d;
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saving requires the document thread")};
    if (!d.image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    if (!preview.isNull() && !version)
        return {FileCode::InvalidData, {}, QStringLiteral("A native preview requires its explicit image version")};
    const auto pinned = version ? version : d.image->operations().snapshot();
    if (!pinned || pinned->documentId() != d.image->operations().snapshot()->documentId() || !pinned->isCommitted())
        return {FileCode::InvalidData, {}, QStringLiteral("Saving requires a committed version of this document")};
    const auto limits = Private::formatLimits(d.limits);
    if (qint64(pinned->size.width()) * pinned->size.height() * pixelByteCount(pinned->format) > limits.archiveBytes)
        return {FileCode::CapacityExceeded, {}, QStringLiteral("KRA projection pixel capacity exceeded")};
    Kra::Document output;
    output.description = d.description;
    output.files = d.files;
    output.compressRaster = d.compressRaster;
    output.image.size = pinned->size;
    output.image.format = pinned->format;
    output.image.colorProfile = pinned->colorProfile;
    output.image.projectionBackground = pinned->projectionBackground;
    output.image.selection = pinned->selection;
    output.image.selectionInverted = pinned->selectionInverted;
    output.image.selectionKind = pinned->selectionKind;
    output.image.selectionMask = pinned->selectionMask;
    for (const auto &layer : pinned->layers) {
        if (pinned->activeLayer == layer.id) output.image.activeLayer = output.image.layers.size();
        output.image.layers.append(rasterLayerDescription(layer));
    }
    qsizetype rasterBytes = 0;
    const auto masks = readKraMaskPixels(d.execution, output.image, limits.rasterBytes, &rasterBytes);
    if (masks.code != ExecutionCode::Success) return {fileCode(masks.code), {}, masks.diagnostic};
    for (qsizetype index = 0; index < pinned->layers.size(); ++index) {
        const auto &layer = pinned->layers[index];
        auto &saved = output.image.layers[index];
        if (layer.kind == LayerKind::Paint && !layer.generator) {
            const auto pixels = d.execution.readLayerPixels(d.image->handle(), pinned, layer.id, limits.rasterBytes - rasterBytes);
            if (pixels.code != ExecutionCode::Success) return {fileCode(pixels.code), {}, pixels.diagnostic};
            saved.writtenAreas = pixels.writtenAreas;
            for (const auto &tile : pixels.tiles) {
                saved.tiles.append({tile.coordinate, tile.rgba, pinned->format});
                rasterBytes += tile.rgba.size();
            }
        }
    }
    const auto projection = d.execution.readProjectionImage(d.image->handle(), pinned, limits.archiveBytes);
    if (projection.code != ExecutionCode::Success) return {fileCode(projection.code), {}, projection.diagnostic};
    auto merged = projection.image;
    const auto registry = KoColorSpaceRegistry::instance();
    if (!registry->rgb8())
        return {FileCode::UnsupportedData, {}, QStringLiteral("The native RGB color conversion is unavailable")};
    if (pinned->format == PixelFormat::Rgba16 ||
        (!pinned->colorProfile.isEmpty() && pinned->colorProfile != registry->rgb8()->profile()->rawData())) {
        // KRA retains the document encoding; its merged PNG uses the native RGB8 conversion.
        const bool words = pinned->format == PixelFormat::Rgba16;
        const int pixelSize = pixelByteCount(pinned->format);
        QByteArray bgra(qsizetype(merged.width()) * merged.height() * pixelSize, Qt::Uninitialized);
        for (int y = 0; y < merged.height(); ++y) {
            if (words) {
                const auto *source = reinterpret_cast<const QRgba64 *>(merged.constScanLine(y));
                for (int x = 0; x < merged.width(); ++x) {
                    const quint16 pixel[] = {source[x].blue(), source[x].green(), source[x].red(), source[x].alpha()};
                    std::memcpy(bgra.data() + (qsizetype(y) * merged.width() + x) * pixelSize, pixel, sizeof(pixel));
                }
            } else {
                const auto *source = merged.constScanLine(y);
                auto *destination = bgra.data() + qsizetype(y) * merged.width() * pixelSize;
                std::memcpy(destination, source, merged.width() * pixelSize);
                for (int x = 0; x < merged.width(); ++x) std::swap(destination[x * 4], destination[x * 4 + 2]);
            }
        }
        const auto standard = words ? registry->rgb16() : registry->rgb8();
        if (!standard)
            return {FileCode::UnsupportedData, {}, QStringLiteral("The native RGB color conversion is unavailable")};
        const auto depth = words ? QStringLiteral("U16") : QStringLiteral("U8");
        const auto profile = pinned->colorProfile.isEmpty() ? standard->profile() :
            registry->createColorProfileFromData(QStringLiteral("RGBA"), depth, pinned->colorProfile);
        const auto encoding = profile ? registry->colorSpace(QStringLiteral("RGBA"), depth, profile) : nullptr;
        if (!encoding) return {FileCode::UnsupportedData, {}, QStringLiteral("The fixed image profile is unavailable")};
        merged = encoding->convertToQImage(reinterpret_cast<const quint8 *>(bgra.constData()),
            merged.width(), merged.height(), registry->rgb8()->profile(),
            KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    }
    const auto written = Kra::RasterCodec::write(destination, output, merged, limits, preview);
    if (written.code != Kra::Status::Success) return {fileCode(written.code), {}, written.diagnostic};
    return {FileCode::Success, pinned, {}};
}
}
