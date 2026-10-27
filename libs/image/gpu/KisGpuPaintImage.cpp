/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintImage_p.h"
#include "KisGpuOperationValidation_p.h"
#include <QThread>
#include <algorithm>
#include <new>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
namespace {
Execution &requireExecution(const std::shared_ptr<Execution> &execution)
{
    if (!execution) throw std::invalid_argument("Image construction requires an execution owner");
    return *execution;
}

Result resultFor(ExecutionCode code)
{
    switch (code) {
    case ExecutionCode::Success: return Result::Success;
    case ExecutionCode::CapacityExceeded: return Result::CapacityExceeded;
    case ExecutionCode::Busy: return Result::EditInProgress;
    case ExecutionCode::DeviceFailure: return Result::DeviceFailure;
    case ExecutionCode::InvalidRequest:
    case ExecutionCode::SurfaceOutOfDate: return Result::InvalidInput;
    }
    return Result::DeviceFailure;
}

/** Converts retained alpha coverage into the transparent RGBA layer that hosts
 * one temporary mask edit. Only covered tiles are allocated; the stored RGB
 * values are irrelevant because the commit reads the alpha channel back.
 */
bool maskInputTiles(const SelectionMaskSP &mask, const QRect &canvas, qsizetype maximumBytes,
                    QList<RgbaTile> &tiles)
{
    if (!mask || mask->isEmpty() || canvas.isEmpty()) return false;
    if (mask->tiles().size() > maximumBytes / RgbaTile::ByteCount) return false;
    for (auto tile = mask->tiles().cbegin(); tile != mask->tiles().cend(); ++tile) {
        const QPoint origin(tile.key().first * SelectionMask::TileSize,
                            tile.key().second * SelectionMask::TileSize);
        const QRect area = QRect(origin, QSize(SelectionMask::TileSize, SelectionMask::TileSize)).intersected(canvas);
        if (area.isEmpty() || tile.value().size() != mask->tileBytes()) continue;
        RgbaTile output;
        output.coordinate = QPoint(tile.key().first, tile.key().second);
        output.rgba = QByteArray(RgbaTile::ByteCount, '\0');
        bool covered = false;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            const int localY = y - origin.y();
            for (int x = area.left(); x <= area.right(); ++x) {
                const int localX = x - origin.x();
                const qsizetype index = qsizetype(localY) * SelectionMask::TileSize + localX;
                const quint8 value = mask->isAlpha()
                    ? quint8(tile.value().at(index))
                    : ((quint8(tile.value().at(index >> 3)) & (1u << (index & 7))) ? 255 : 0);
                if (!value) continue;
                covered = true;
                const qsizetype pixel = index * 4;
                output.rgba[pixel + 0] = char(255);
                output.rgba[pixel + 1] = char(255);
                output.rgba[pixel + 2] = char(255);
                output.rgba[pixel + 3] = char(value);
            }
        }
        if (covered) tiles.append(std::move(output));
    }
    return !tiles.isEmpty();
}

RasterImage maskInputImage(const DocumentVersion &version, const SelectionMaskSP &mask, qsizetype maximumBytes)
{
    QList<RgbaTile> tiles;
    const QRect canvas(QPoint(), version.size);
    if (!maskInputTiles(mask, canvas, maximumBytes, tiles)) return {};
    RasterImage image;
    image.size = version.size;
    RasterLayer host;
    host.name = QStringLiteral("Transparency Mask");
    host.uuid = QUuid::createUuid();
    host.tiles = std::move(tiles);
    host.defaultPixel = {0, 0, 0, 0};
    image.layers.append(std::move(host));
    image.activeLayer = 0;
    image.selection = version.selection;
    image.selectionInverted = version.selectionInverted;
    image.selectionKind = version.selectionKind;
    image.selectionMask = version.selectionMask;
    return image;
}

/** Reads the temporary layer's sparse RGBA and retains nonzero alpha as
 * document coverage. Tiles are transferred per tile row when one read would
 * exceed the execution's transfer budget, and unchanged alpha tiles retain the
 * previous mask's immutable block instead of allocating a history copy.
 */
SelectionMaskSP readMaskCoverage(const PaintImage &source, LayerId layer, QSize size,
                                 const SelectionMaskSP &previous, Result &status)
{
    status = Result::Success;
    const QRect canvas(QPoint(), size);
    if (canvas.isEmpty()) {
        status = Result::InvalidInput;
        return {};
    }
    const auto version = source.operations().snapshot();
    Execution &execution = source.execution();
    QList<PixelTile> pixels;
    auto read = execution.readPixels(source.handle(), version, canvas, {layer}, false, PixelReadArea::Canvas);
    if (read.code == ExecutionCode::Success) {
        pixels = std::move(read.tiles);
    } else if (read.code == ExecutionCode::CapacityExceeded) {
        for (int y = 0; y < size.height(); y += SelectionMask::TileSize) {
            const QRect band(0, y, size.width(), std::min(SelectionMask::TileSize, size.height() - y));
            auto part = execution.readPixels(source.handle(), version, band, {layer}, false, PixelReadArea::Canvas);
            if (part.code != ExecutionCode::Success) {
                status = resultFor(part.code);
                return {};
            }
            pixels.append(part.tiles);
        }
    } else {
        status = resultFor(read.code);
        return {};
    }
    SelectionTileMap alphaTiles;
    for (const auto &tile : pixels) {
        if (tile.layer != layer || tile.rgba.size() != RgbaTile::ByteCount) continue;
        const QPoint origin = tile.coordinate * SelectionMask::TileSize;
        const QRect area = QRect(origin, QSize(SelectionMask::TileSize, SelectionMask::TileSize)).intersected(canvas);
        if (area.isEmpty()) continue;
        QByteArray coverage;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            const int localY = y - origin.y();
            for (int x = area.left(); x <= area.right(); ++x) {
                const int localX = x - origin.x();
                const qsizetype index = qsizetype(localY) * SelectionMask::TileSize + localX;
                const quint8 alpha = quint8(tile.rgba.at(index * 4 + 3));
                if (!alpha) continue;
                if (coverage.isEmpty()) coverage = QByteArray(SelectionMask::AlphaTileBytes, '\0');
                coverage[index] = char(alpha);
            }
        }
        if (coverage.isEmpty()) continue;
        const SelectionTileKey key(tile.coordinate.x(), tile.coordinate.y());
        if (previous && previous->isAlpha()) {
            const auto old = previous->tiles().constFind(key);
            if (old != previous->tiles().cend() && old.value() == coverage) coverage = old.value();
        }
        alphaTiles.insert(key, coverage);
    }
    auto mask = SelectionMask::fromAlphaTiles(alphaTiles);
    if (!mask) {
        // Coverage must stay representable across history and KRA.
        // Fully erased input keeps the smallest visible sample so the retained
        // mask continues to hide the layer almost completely.
        const QByteArray visible(1, char(1));
        mask = SelectionMask::fromAlphaCoverage(QRect(0, 0, 1, 1), visible, canvas);
    }
    if (!mask) status = Result::CapacityExceeded;
    return mask;
}

const Layer *maskPaintLayer(const Snapshot &version, LayerId layer)
{
    if (!version) return nullptr;
    for (const auto &value : version->layers)
        if (value.id == layer && value.kind == LayerKind::Paint && value.transparencyMask &&
            !value.userLocked && value.visible) return &value;
    return nullptr;
}

RasterImage safeMaskInputImage(const DocumentVersion &version, const SelectionMaskSP &mask,
                               qsizetype maximumBytes) noexcept
{
    try {
        return maskInputImage(version, mask, maximumBytes);
    } catch (const std::bad_alloc &) {
        return {};
    }
}

SelectionMaskSP safeReadMaskCoverage(const PaintImage &source, LayerId layer, QSize size,
                                     const SelectionMaskSP &previous, Result &status) noexcept
{
    try {
        return readMaskCoverage(source, layer, size, previous, status);
    } catch (const std::bad_alloc &) {
        status = Result::CapacityExceeded;
        return {};
    }
}
}
PaintImage::PaintImage(Execution &execution, QSize size, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(size, limits), limits) {}
PaintImage::PaintImage(Execution &execution, const RasterImage &image, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(image, limits), limits) {}
PaintImage::PaintImage(std::shared_ptr<Execution> execution, QSize size, Limits limits)
    : PaintImage(requireExecution(execution), std::make_unique<OperationDocument>(size, limits), limits, execution) {}
PaintImage::PaintImage(std::shared_ptr<Execution> execution, const RasterImage &image, Limits limits)
    : PaintImage(requireExecution(execution), std::make_unique<OperationDocument>(image, limits), limits, execution) {}
PaintImage::PaintImage(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits,
                       std::shared_ptr<Execution> retainedExecution)
    : m_d(new Private{std::move(retainedExecution), execution, std::move(document), limits})
{
    const auto admitted = execution.open(m_d->document->snapshot());
    if (admitted.code == ExecutionCode::CapacityExceeded) throw std::length_error(admitted.diagnostic.toStdString());
    if (admitted.code != ExecutionCode::Success) throw std::invalid_argument(admitted.diagnostic.toStdString());
    m_d->handle = admitted.handle;
}
PaintImage::~PaintImage()
{
    if (m_d->handle) m_d->execution.close(m_d->handle);
}
const OperationDocument &PaintImage::operations() const { return *m_d->document; }
Limits PaintImage::limits() const { return m_d->limits; }
quint64 PaintImage::handle() const { return m_d->handle; }
Execution &PaintImage::execution() const { return m_d->execution; }
PaintImageSP PaintImage::clone(bool preserveLayerUuids) const
{ return cloneTo(*this, preserveLayerUuids); }

PaintImageSP PaintImage::cloneTo(const PaintImage &destination, bool preserveLayerUuids) const
{
    if (!onThread() || !destination.onThread()) return {};
    return PaintImageSP(new PaintImage(destination.m_d->execution, m_d->document->cloneCurrent(preserveLayerUuids),
                                       m_d->limits, destination.m_d->retainedExecution));
}

PaintImageSP PaintImage::cloneTo(Execution &destination, bool preserveLayerUuids) const
{
    if (!onThread()) return {};
    return PaintImageSP(new PaintImage(destination, m_d->document->cloneCurrent(preserveLayerUuids), m_d->limits,
        &destination == &m_d->execution ? m_d->retainedExecution : std::shared_ptr<Execution>{}));
}
PaintImageSP PaintImage::cloneLayerPixels(const LayerPixelReferenceSP &pixels) const
{
    if (!onThread() || !pixels) return {};
    auto document = m_d->document->cloneVersion(pixels->version(), false, pixels->layer());
    if (!document) return {};
    return PaintImageSP(new PaintImage(m_d->execution, std::move(document), m_d->limits, m_d->retainedExecution));
}
PaintImageSP PaintImage::cloneLayerForest(const QList<LayerInsertion> &layers) const
{
    if (!onThread()) return {};
    return cloneLayerForest(layers, m_d->document->snapshot()->size);
}
PaintImageSP PaintImage::cloneLayerForest(const QList<LayerInsertion> &layers, QSize size) const
{
    RasterImage input;
    input.size = size;
    input.format = m_d->document->snapshot()->format;
    input.colorProfile = m_d->document->snapshot()->colorProfile;
    OperationDocument prepared(input, m_d->limits);
    if (prepared.insertLayers(layers, 0) != Result::Success) return {};
    return PaintImageSP(new PaintImage(m_d->execution, prepared.cloneCurrent(true),
                                       m_d->limits, m_d->retainedExecution));
}
ExecutionOutcome PaintImage::lastRequestOutcome() const { return m_d->lastRequest; }
PixelBytesOutcome PaintImage::readRgbaPixels(LayerId layer, QRect region, qsizetype maximumBytes) const
{
    if (!onThread()) {
        PixelBytesOutcome outcome;
        outcome.diagnostic = QStringLiteral("Image pixel reads require the owning thread");
        return outcome;
    }
    return m_d->execution.readRgbaPixels(m_d->handle, m_d->document->snapshot(), region, layer, maximumBytes);
}
bool PaintImage::onThread() const { return QThread::currentThread() == thread(); }
Result PaintImage::writeProjectionBackground(const RgbaPixel &rgba)
{
    return onThread() ? requestAfterWrite(m_d->document->writeProjectionBackground(rgba)) : Result::InvalidInput;
}
Result PaintImage::requestAfterWrite(Result result)
{
    if (result == Result::Success) requestCurrentVersion();
    return result;
}
Result PaintImage::writeLayerName(LayerId layer, const QString &name)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerName(layer, name)) : Result::InvalidInput; }
Result PaintImage::writeLayerVisible(LayerId layer, bool visible)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::writeLayerOpacity(LayerId layer, quint8 opacity)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerOpacity(layer, opacity)) : Result::InvalidInput; }
Result PaintImage::writeLayerLocked(LayerId layer, bool locked)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerLocked(layer, locked)) : Result::InvalidInput; }
Result PaintImage::writeLayerPinnedToTimeline(LayerId layer, bool pinned)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerPinnedToTimeline(layer, pinned)) : Result::InvalidInput; }
Result PaintImage::writeLayerColorLabel(LayerId layer, int index)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerColorLabel(layer, index)) : Result::InvalidInput; }
Result PaintImage::recordLayerProperties(const Snapshot &before, const Snapshot &after,
                                        const QList<LayerPropertiesSelection> &selection)
{ return onThread() ? publish(m_d->document->recordLayerProperties(before, after, selection)) : Result::InvalidInput; }
Result PaintImage::restoreLayerProperties(const Snapshot &before, const QList<LayerPropertiesSelection> &selection)
{ return onThread() ? publish(m_d->document->restoreLayerProperties(before, selection)) : Result::InvalidInput; }
LayerPixelReferenceSP PaintImage::retainLayerPixels(LayerId layer)
{
    if (!onThread() || !m_d->handle) return {};
    const auto version = layerPixelVersion();
    const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    if (found == version->layers.cend()) return {};
    auto reference = m_d->layerPixels.value(found->uuid).toStrongRef();
    if (!reference) {
        reference.reset(new LayerPixelReference(layer, found->uuid, version));
        m_d->layerPixels.insert(found->uuid, reference);
    }
    return reference;
}
OperationId PaintImage::beginColorConversion()
{ return onThread() ? m_d->document->beginColorConversion() : OperationId{}; }
Result PaintImage::cancelColorConversion(OperationId id)
{ return onThread() ? m_d->document->cancelColorConversion(id) : Result::InvalidInput; }
Result PaintImage::commitColorConversion(OperationId id, const Snapshot &before, PixelFormat format,
    const QByteArray &profile, const RgbaPixel &background, const QList<ConvertedLayerPixels> &layers)
{
    if (!onThread()) return Result::InvalidInput;
    Result result;
    try { result = m_d->document->commitColorConversion(id, before, format, profile, background, layers); }
    catch (const std::bad_alloc &) { return Result::CapacityExceeded; }
    return publish(result);
}

Result PaintImage::setSelectionRect(const QRect &rect, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    const bool effectiveInverted = !rect.isEmpty() && inverted;
    if (rect.normalized() == m_d->document->selectionRect() &&
        effectiveInverted == m_d->document->selectionInverted() &&
        m_d->document->selectionKind() == SelectionKind::Rectangle) return Result::Success;
    return publish(m_d->document->setSelectionRect(rect, inverted));
}
Result PaintImage::setSelectionEllipse(const QRect &rect, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    const bool effectiveInverted = !rect.isEmpty() && inverted;
    if (rect.normalized() == m_d->document->selectionRect() &&
        m_d->document->selectionKind() == SelectionKind::Ellipse &&
        effectiveInverted == m_d->document->selectionInverted()) return Result::Success;
    return publish(m_d->document->setSelectionEllipse(rect, inverted));
}
Result PaintImage::setSelectionMask(const SelectionMaskSP &mask, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    if (mask && m_d->document->selectionKind() == SelectionKind::Freehand &&
        m_d->document->selectionMask() == mask && m_d->document->selectionInverted() == inverted)
        return Result::Success;
    return publish(m_d->document->setSelectionMask(mask, inverted));
}
Result PaintImage::clearSelection()
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->document->selectionRect().isEmpty() && !m_d->document->selectionInverted() &&
        m_d->document->selectionKind() == SelectionKind::Rectangle) return Result::Success;
    return publish(m_d->document->clearSelection());
}
QRect PaintImage::selectionRect() const
{ return onThread() ? m_d->document->selectionRect() : QRect{}; }
bool PaintImage::selectionInverted() const
{ return onThread() && m_d->document->selectionInverted(); }
SelectionKind PaintImage::selectionKind() const
{ return onThread() ? m_d->document->selectionKind() : SelectionKind::Rectangle; }
SelectionMaskSP PaintImage::selectionMask() const
{ return onThread() ? m_d->document->selectionMask() : SelectionMaskSP{}; }
Result PaintImage::addTransparencyMaskFromSelection(LayerId layer)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->addTransparencyMaskFromSelection(layer));
}
Result PaintImage::setTransparencyMaskVisible(LayerId layer, bool visible)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->setTransparencyMaskVisible(layer, visible));
}
Result PaintImage::removeTransparencyMask(LayerId layer)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->removeTransparencyMask(layer));
}
SelectionMaskSP PaintImage::layerTransparencyMask(LayerId layer) const
{
    return onThread() ? m_d->document->layerTransparencyMask(layer) : SelectionMaskSP{};
}
bool PaintImage::layerTransparencyMaskVisible(LayerId layer) const
{
    return onThread() && m_d->document->layerTransparencyMaskVisible(layer);
}
void PaintImage::refreshLayerPixelReferences()
{
    const auto version = layerPixelVersion();
    for (auto it = m_d->layerPixels.begin(); it != m_d->layerPixels.end();) {
        const auto reference = it.value().toStrongRef();
        if (!reference) {
            it = m_d->layerPixels.erase(it);
            continue;
        }
        const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
            [&](const auto &layer) { return layer.uuid == reference->m_uuid; });
        if (found != version->layers.cend()) {
            reference->m_layer = found->id;
            reference->m_version = version;
        }
        ++it;
    }
}
Snapshot PaintImage::layerPixelVersion() const
{
    const auto preview = m_d->document->preview();
    if (!preview->pendingOperation().isNull()) {
        for (const auto &layer : preview->layers) {
            if (layer.operations.isEmpty()) continue;
            const auto translation = std::get_if<TranslationSnapshot>(&layer.operations.last());
            if (translation && (*translation)->id == preview->pendingOperation()) return preview;
        }
    }
    return m_d->document->snapshot();
}
void PaintImage::requestCurrentVersion()
{
    try {
        m_d->lastRequest = m_d->execution.request(m_d->handle, m_d->document->preview());
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("GPU request allocation exceeded capacity")};
    }
    refreshLayerPixelReferences();
}
Result PaintImage::publish(Result result)
{
    if (result == Result::Success) {
        requestCurrentVersion();
        Q_EMIT changed();
    }
    return result;
}
OperationId PaintImage::beginStroke(LayerId layer, const Brush &brush, quint64 seed)
{ return onThread() ? m_d->document->beginStroke(layer, brush, seed) : OperationId{}; }
OperationId PaintImage::beginTransparencyMaskStroke(LayerId layer, const Brush &brush, quint64 seed)
{
    if (!onThread() || !m_d->handle || m_d->maskStroke || m_d->document->hasActiveStroke()) return {};
    const auto version = m_d->document->snapshot();
    const Layer *target = maskPaintLayer(version, layer);
    if (!target) return {};
    RasterImage input = safeMaskInputImage(*version, target->transparencyMask, m_d->limits.rasterBytes);
    if (input.layers.isEmpty()) return {};
    PaintImageSP image;
    try {
        image.reset(new PaintImage(m_d->execution, input, m_d->limits));
    } catch (const std::length_error &) {
        return {};
    } catch (const std::invalid_argument &) {
        return {};
    }
    Brush maskBrush = brush;
    // Mask coverage owns its own alpha; layer channel locks and alpha locks
    // must not prevent the retained mask from changing.
    maskBrush.alphaLocked = false;
    maskBrush.channelWriteMask = 15;
    const LayerId imageLayer = image->operations().activeLayer();
    const auto id = image->beginStroke(imageLayer, maskBrush, seed);
    if (id.isNull()) return {};
    if (m_d->document->beginExternalStroke(id) != Result::Success) {
        image->cancelStroke(id);
        return {};
    }
    m_d->maskStroke.reset(new MaskStroke{image, layer, imageLayer, id});
    return id;
}
Result PaintImage::appendSamples(OperationId id, const QList<InputSample> &samples)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return m_d->maskStroke->image->appendSamples(id, samples);
    }
    return publish(m_d->document->appendSamples(id, samples));
}
Result PaintImage::flushSmoothing(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return m_d->maskStroke->image->flushSmoothing(id);
    }
    const auto before = m_d->document->preview();
    const auto result = m_d->document->flushSmoothing(id);
    return m_d->document->preview() == before ? result : publish(result);
}
Result PaintImage::updateShape(OperationId id, const Shape &shape)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return m_d->maskStroke->image->updateShape(id, shape);
    }
    return publish(m_d->document->updateShape(id, shape));
}
Result PaintImage::updateFigure(OperationId id, const Shape &shape, const FigureStyle &style)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return m_d->maskStroke->image->updateFigure(id, shape, style);
    }
    return publish(m_d->document->updateFigure(id, shape, style));
}
Result PaintImage::paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                               const Shape &shape, const FigureStyle &style, quint64 randomSeed)
{ return onThread() ? publish(m_d->document->paintFigure(layer, brush, samples, shape, style, randomSeed)) : Result::InvalidInput; }

Result PaintImage::paintTransparencyMaskFigure(LayerId layer, const Brush &brush,
                                               const QList<InputSample> &samples, const Shape &shape,
                                               const FigureStyle &style, quint64 randomSeed)
{
    return paintTransparencyMaskFigure(layer, brush, samples, shape, style, randomSeed, false);
}

Result PaintImage::paintTransparencyMaskFigure(LayerId layer, const Brush &brush,
                                               const QList<InputSample> &samples, const Shape &shape,
                                               const FigureStyle &style, quint64 randomSeed, bool filled)
{
    if (!onThread()) return Result::InvalidInput;
    if (!m_d->handle) return Result::Closed;
    if (m_d->maskStroke || m_d->document->hasActiveStroke()) return Result::EditInProgress;
    const auto version = m_d->document->snapshot();
    const Layer *target = maskPaintLayer(version, layer);
    if (!target) return Result::LayerUnavailable;
    RasterImage input = safeMaskInputImage(*version, target->transparencyMask, m_d->limits.rasterBytes);
    if (input.layers.isEmpty()) return Result::CapacityExceeded;
    PaintImageSP image;
    try {
        image.reset(new PaintImage(m_d->execution, input, m_d->limits));
    } catch (const std::length_error &) {
        return Result::CapacityExceeded;
    } catch (const std::invalid_argument &) {
        return Result::InvalidInput;
    }
    Brush maskBrush = brush;
    maskBrush.alphaLocked = false;
    maskBrush.channelWriteMask = 15;
    const LayerId imageLayer = image->operations().activeLayer();
    const auto result = image->paintFigure(imageLayer, maskBrush, samples, shape, style, randomSeed);
    if (result != Result::Success) return result;
    return replaceMaskFromImage(layer, *image, imageLayer, filled);
}

Result PaintImage::fillTransparencyMask(LayerId layer, const std::array<quint8, 3> &rgb,
                                        qreal opacity, bool erase, bool alphaLocked)
{
    Q_UNUSED(alphaLocked);
    Brush brush;
    brush.rgba = {rgb[0], rgb[1], rgb[2], 255};
    brush.opacity = opacity;
    brush.erase = erase;
    brush.channelWriteMask = 15;
    FigureStyle style;
    style.outline = false;
    style.fill = brush.rgba;
    const QSize size = m_d->document->snapshot()->size;
    const QPointF end(size.width(), size.height());
    return paintTransparencyMaskFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                       {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0, true);
}

Result PaintImage::fillTransparencyMaskPattern(LayerId layer, const PatternFill &pattern,
                                               qreal opacity, bool erase, bool alphaLocked)
{
    Q_UNUSED(alphaLocked);
    if (!pattern.isValid()) return Result::InvalidInput;
    Brush brush;
    brush.rgba = {0, 0, 0, 255};
    brush.opacity = opacity;
    brush.erase = erase;
    brush.channelWriteMask = 15;
    FigureStyle style;
    style.outline = false;
    style.pattern = pattern;
    const QSize size = m_d->document->snapshot()->size;
    const QPointF end(size.width(), size.height());
    return paintTransparencyMaskFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                       {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0, true);
}

Result PaintImage::replaceTransparencyMask(LayerId layer, const SelectionMaskSP &mask)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->replaceTransparencyMask(layer, mask));
}

Result PaintImage::replaceMaskFromImage(LayerId layer, const PaintImage &source, LayerId sourceLayer, bool filled)
{
    if (!onThread()) return Result::InvalidInput;
    Result status = Result::Success;
    const auto mask = safeReadMaskCoverage(source, sourceLayer, m_d->document->snapshot()->size,
                                           m_d->document->layerTransparencyMask(layer), status);
    if (!mask) return status;
    return publish(m_d->document->replaceTransparencyMask(layer, mask, filled));
}

Result PaintImage::commitMaskStroke()
{
    std::unique_ptr<MaskStroke> session = std::move(m_d->maskStroke);
    const auto committed = session->image->commitStroke(session->id);
    if (committed != Result::Success) {
        m_d->document->endExternalStroke(session->id);
        return committed;
    }
    Result status = Result::Success;
    const auto mask = safeReadMaskCoverage(*session->image, session->imageLayer,
                                          m_d->document->snapshot()->size,
                                          m_d->document->layerTransparencyMask(session->layer), status);
    if (!mask) {
        m_d->document->endExternalStroke(session->id);
        return status;
    }
    const auto ended = m_d->document->endExternalStroke(session->id);
    if (ended != Result::Success) return ended;
    return publish(m_d->document->replaceTransparencyMask(session->layer, mask));
}

Result PaintImage::commitStroke(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return commitMaskStroke();
    }
    return publish(m_d->document->commitStroke(id));
}
Result PaintImage::cancelStroke(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        std::unique_ptr<MaskStroke> session = std::move(m_d->maskStroke);
        const auto cancelled = session->image->cancelStroke(id);
        m_d->document->endExternalStroke(id);
        return cancelled;
    }
    return publish(m_d->document->cancelStroke(id));
}
OperationId PaintImage::beginLayerTranslation(LayerId layer)
{ return onThread() ? m_d->document->beginLayerTranslation(layer) : OperationId{}; }
Result PaintImage::updateLayerTranslation(OperationId id, QPoint offset)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    if (document.m_closed) return Result::Closed;
    if (!document.m_translation || document.m_translation->id != id) return Result::StaleOperation;
    const auto admitted = checkLayerTranslation(document.snapshot(), document.m_translation->layer, offset);
    return admitted == Result::Success ? publish(m_d->document->updateLayerTranslation(id, offset)) : admitted;
}
Result PaintImage::commitLayerTranslation(OperationId id)
{ return onThread() ? publish(m_d->document->commitLayerTranslation(id)) : Result::InvalidInput; }
Result PaintImage::cancelLayerTranslation(OperationId id)
{ return onThread() ? publish(m_d->document->cancelLayerTranslation(id)) : Result::InvalidInput; }
Result PaintImage::writeLayerPosition(LayerId layer, QPoint position)
{ return applyLayerPosition(layer, position, nullptr); }

Result PaintImage::writeLayerPosition(const LayerPixelReferenceSP &pixels, QPoint position)
{
    if (!onThread() || !pixels || m_d->layerPixels.value(pixels->m_uuid).toStrongRef() != pixels)
        return Result::InvalidInput;
    return applyLayerPosition(pixels->m_layer, position, &pixels->m_version);
}

Result PaintImage::applyLayerPosition(LayerId layer, QPoint position, Snapshot *retainedVersion)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto source = retainedVersion ? *retainedVersion : document.snapshot();
    if (!source || source->documentId() != document.snapshot()->documentId()) return Result::StaleVersion;
    bool available = false;
    for (const auto &value : source->layers) if (value.id == layer) available = true;
    if (!available) return Result::LayerUnavailable;
    for (const auto &version : document.layerPositionVersions(source, layer)) {
        for (const auto &value : version->layers) {
            if (value.id != layer) continue;
            const qint64 x = qint64(position.x()) - value.position.x();
            const qint64 y = qint64(position.y()) - value.position.y();
            constexpr qint64 limit = 2 * RgbaTile::CoordinateLimit;
            if (x < -limit || x > limit || y < -limit || y > limit) return Result::InvalidInput;
            if (value.kind == LayerKind::Group) continue;
            const auto admitted = checkLayerTranslation(version, layer, QPoint(int(x), int(y)));
            if (admitted != Result::Success) return admitted;
        }
    }
    return requestAfterWrite(m_d->document->applyLayerPosition(layer, position, retainedVersion));
}

Result PaintImage::checkLayerTranslation(const Snapshot &version, LayerId layer, QPoint offset) const
{
    if (offset.isNull()) return Result::Success;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (qAbs(qint64(offset.x())) > 2 * limit || qAbs(qint64(offset.y())) > 2 * limit) return Result::InvalidInput;
    const auto content = m_d->execution.readBounds(m_d->handle, version, layer, true);
    if (content.code == ExecutionCode::CapacityExceeded) return Result::CapacityExceeded;
    if (content.code == ExecutionCode::Busy) return Result::Busy;
    if (content.code != ExecutionCode::Success) return Result::DeviceFailure;
    const auto bounds = *content.bounds;
    if (!bounds.isEmpty() && (qint64(bounds.left()) + offset.x() < -limit ||
        qint64(bounds.right()) + offset.x() >= limit || qint64(bounds.top()) + offset.y() < -limit ||
        qint64(bounds.bottom()) + offset.y() >= limit)) return Result::InvalidInput;
    return Result::Success;
}
Result PaintImage::replaceTiles(LayerId layer, const QList<RgbaTile> &tiles)
{ return onThread() ? publish(m_d->document->replaceTiles(layer, tiles)) : Result::InvalidInput; }
LayerId PaintImage::addLayer(const QString &name)
{
    if (!onThread()) return 0;
    return addLayer(name, m_d->document->snapshot()->layers.size(), false);
}

LayerId PaintImage::addLayer(const QString &name, qsizetype index, bool pinnedToTimeline, const QUuid &parentUuid)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->addLayer(name, index, pinnedToTimeline, parentUuid);
    if (id) publish(Result::Success);
    return id;
}
LayerId PaintImage::insertLayer(const OperationDocument &source, LayerId layer,
                               const RasterLayer &properties, qsizetype index, bool activate)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->insertLayer(source, layer, properties, index, activate);
    if (id) publish(Result::Success);
    return id;
}

LayerId PaintImage::insertLayer(const PaintImage &source, const LayerPixelReferenceSP &pixels,
                               const RasterLayer &properties, qsizetype index, bool activate)
{
    if (!onThread() || !source.onThread() || !pixels || !pixels->version() ||
        pixels->version()->documentId() != source.operations().snapshot()->documentId()) return 0;
    const auto id = m_d->document->insertVersion(pixels->version(), pixels->layer(), properties, index, activate);
    if (id) publish(Result::Success);
    return id;
}

Result PaintImage::insertLayers(const QList<LayerInsertion> &layers, qsizetype index, bool activate,
                                bool clearPixelSelection)
{ return onThread() ? publish(m_d->document->insertLayers(layers, index, activate, clearPixelSelection)) : Result::InvalidInput; }

Result PaintImage::duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource)
{ return onThread() ? publish(m_d->document->duplicateLayers(layers, names, activeSource)) : Result::InvalidInput; }
Result PaintImage::clearLayers(const QList<LayerId> &layers)
{ return onThread() ? publish(m_d->document->clearLayers(layers)) : Result::InvalidInput; }
Result PaintImage::cropLayer(LayerId layer, const QRect &rect)
{ return onThread() ? publish(m_d->document->cropLayer(layer, rect)) : Result::InvalidInput; }
Result PaintImage::resizeCanvas(const QRect &rect, bool cropLayers)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    if (!Detail::validCanvasRect(rect)) return Result::InvalidInput;
    const auto before = document.snapshot();
    if (!cropLayers) {
        for (const auto &layer : before->layers) {
            if (layer.kind == LayerKind::Group) continue;
            const auto admitted = checkLayerTranslation(before, layer.id, -rect.topLeft());
            if (admitted != Result::Success) return admitted;
        }
    }
    const auto result = m_d->document->resizeCanvas(rect, cropLayers);
    return document.snapshot() == before ? result : publish(result);
}
Result PaintImage::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                             qreal opacity, bool erase, bool alphaLocked,
                             LayerBlendMode blendMode)
{ return onThread() ? publish(m_d->document->fillLayer(layer, rgb, opacity, erase, alphaLocked, blendMode)) : Result::InvalidInput; }
Result PaintImage::fillLayerPattern(LayerId layer, const PatternFill &pattern,
                                    qreal opacity, bool erase, bool alphaLocked,
                                    LayerBlendMode blendMode)
{ return onThread() ? publish(m_d->document->fillLayerPattern(layer, pattern, opacity, erase, alphaLocked, blendMode)) : Result::InvalidInput; }
Result PaintImage::invertColors(LayerId layer)
{ return onThread() ? publish(m_d->document->invertColors(layer)) : Result::InvalidInput; }
Result PaintImage::grayscale(LayerId layer)
{ return onThread() ? publish(m_d->document->grayscale(layer)) : Result::InvalidInput; }
Result PaintImage::hsvAdjustment(LayerId layer, const HsvAdjustment &settings)
{ return onThread() ? publish(m_d->document->hsvAdjustment(layer, settings)) : Result::InvalidInput; }
Result PaintImage::selectLayers(const QList<LayerId> &layers, LayerId active)
{ return onThread() ? publish(m_d->document->selectLayers(layers, active)) : Result::InvalidInput; }
Result PaintImage::selectLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->selectLayer(layer)) : Result::InvalidInput; }
Result PaintImage::setLayerVisible(LayerId layer, bool visible)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::setLayerVisible(LayerId layer, bool visible, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible, history)) : Result::InvalidInput; }
Result PaintImage::setLayerOpacity(LayerId layer, quint8 opacity)
{ return onThread() ? publish(m_d->document->setLayerOpacity(layer, opacity)) : Result::InvalidInput; }
Result PaintImage::setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerOpacity(layer, opacity, history)) : Result::InvalidInput; }
Result PaintImage::setLayerLocked(LayerId layer, bool locked)
{ return onThread() ? publish(m_d->document->setLayerLocked(layer, locked)) : Result::InvalidInput; }
Result PaintImage::setLayerLocked(LayerId layer, bool locked, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerLocked(layer, locked, history)) : Result::InvalidInput; }
Result PaintImage::setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerChannelMask(layer, mask, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerChannelMask(LayerId layer, quint8 mask)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerChannelMask(layer, mask)) : Result::InvalidInput; }
Result PaintImage::setLayerProjectionMask(LayerId layer, quint8 mask, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerProjectionMask(layer, mask, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerProjectionMask(LayerId layer, quint8 mask)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerProjectionMask(layer, mask)) : Result::InvalidInput; }
Result PaintImage::setLayerPassThrough(LayerId layer, bool enabled, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerPassThrough(layer, enabled, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerPassThrough(LayerId layer, bool enabled)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerPassThrough(layer, enabled)) : Result::InvalidInput; }
Result PaintImage::setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerBlendMode(layer, mode, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerBlendMode(LayerId layer, LayerBlendMode mode)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerBlendMode(layer, mode)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name, PropertyHistory history)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name, history)) : Result::InvalidInput; }
Result PaintImage::moveLayer(LayerId layer, qsizetype index)
{ return onThread() ? publish(m_d->document->moveLayer(layer, index)) : Result::InvalidInput; }
Result PaintImage::reorderLayers(const QList<LayerId> &order)
{ return onThread() ? publish(m_d->document->reorderLayers(order)) : Result::InvalidInput; }
Result PaintImage::moveLayers(const QList<LayerId> &layers, const QUuid &parentUuid, const QUuid &aboveUuid,
                              LayerId activeRoot)
{
    if (!onThread()) return Result::InvalidInput;
    const auto before = m_d->document->snapshot();
    const auto result = m_d->document->moveLayers(layers, parentUuid, aboveUuid, activeRoot);
    return m_d->document->snapshot() == before ? result : publish(result);
}
Result PaintImage::removeLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->removeLayer(layer)) : Result::InvalidInput; }
Result PaintImage::removeLayers(const QList<LayerId> &layers, const QString &replacementName)
{ return onThread() ? publish(m_d->document->removeLayers(layers, replacementName)) : Result::InvalidInput; }
Result PaintImage::undo() { return onThread() ? publish(m_d->document->undo()) : Result::InvalidInput; }
Result PaintImage::redo() { return onThread() ? publish(m_d->document->redo()) : Result::InvalidInput; }
Result PaintImage::setHistoryIndex(int index)
{ return onThread() ? publish(m_d->document->setHistoryIndex(index)) : Result::InvalidInput; }
void PaintImage::close()
{
    if (!onThread() || !m_d->handle) return;
    if (m_d->maskStroke) {
        std::unique_ptr<MaskStroke> session = std::move(m_d->maskStroke);
        session->image->cancelStroke(session->id);
        m_d->document->endExternalStroke(session->id);
    }
    m_d->execution.close(m_d->handle);
    m_d->handle = 0;
    m_d->document->close();
    refreshLayerPixelReferences();
    Q_EMIT changed();
}
Result PaintImage::markSaved(const Snapshot &version)
{
    if (!onThread()) return Result::InvalidInput;
    const auto result = m_d->document->markSaved(version);
    if (result == Result::Success) Q_EMIT savedStateChanged();
    return result;
}

}
