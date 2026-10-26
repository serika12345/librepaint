/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRenderer.h"
#include "KisGpuPaintJob_p.h"

#include <QFile>
#include <QMap>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
bool containsHiddenRgb(const QByteArray &pixels)
{
    for (qsizetype i = 0; i < pixels.size(); i += 4)
        if (pixels[i + 3] == 0 && (pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0)) return true;
    return false;
}
}

TileRenderer::Private::Private(DevicePtr device, qsizetype byteBudget)
    : device(std::move(device)), budget(std::make_shared<Budget>(Budget{this->device, byteBudget})),
      zero(createBuffer(this->device->nativeDevice(), TileBytes,
                        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst)),
      unusedCoverage(createBuffer(this->device->nativeDevice(), sizeof(quint32), WGPUBufferUsage_Storage)),
      unusedSelection(createBuffer(this->device->nativeDevice(), SelectionMask::AlphaTileBytes,
                                   WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)),
      unusedPattern(createBuffer(this->device->nativeDevice(), sizeof(quint32),
                                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)),
      opaqueMask(createBuffer(this->device->nativeDevice(), SelectionMask::AlphaTileBytes,
                              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst))
{
    if (opaqueMask) {
        const QByteArray opaque(SelectionMask::AlphaTileBytes, char(255));
        wgpuQueueWriteBuffer(this->device->queue(), opaqueMask.get(), 0,
                             opaque.constData(), opaque.size());
    }
    pipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/paint.wgsl"));
    if (!zero || !unusedCoverage || !unusedSelection || !unusedPattern || !opaqueMask || !pipeline) return;
    layout.reset(wgpuComputePipelineGetBindGroupLayout(pipeline.get(), 0));
    if (!layout) error = QStringLiteral("GPU paint bindings could not be created");
    if (error.isEmpty()) initializeProjection();
}

NativeComputePipeline TileRenderer::Private::createPipeline(const QString &path, const char *entryPoint)
{
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl")), shader(path);
    if (!shared.open(QIODevice::ReadOnly) || !shader.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("GPU compute resources could not be loaded");
        return {};
    }
    QByteArray source = shared.readAll() + '\n';
    const bool painting = path.endsWith(QLatin1String("/paint.wgsl"));
    if (painting || path.endsWith(QLatin1String("/over.wgsl"))) {
        QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
        QFile separable(QStringLiteral(":/librepaint/gpu/shaders/separable.wgsl"));
        QFile normal(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba8.wgsl"));
        if (!arithmetic.open(QIODevice::ReadOnly) || !separable.open(QIODevice::ReadOnly) ||
            !normal.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU RGBA8 arithmetic could not be loaded");
            return {};
        }
        source += arithmetic.readAll() + '\n';
        source += separable.readAll() + '\n';
        source += normal.readAll() + '\n';
    }
    if (painting) {
        QFile gaussian(QStringLiteral(":/librepaint/gpu/shaders/gaussian.wgsl"));
        if (!gaussian.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU Gaussian brush arithmetic could not be loaded");
            return {};
        }
        source += gaussian.readAll() + '\n';
    }
    source += shader.readAll();
    WGPUShaderSourceWGSL code{};
    code.chain.sType = WGPUSType_ShaderSourceWGSL;
    code.code = {source.constData(), size_t(source.size())};
    WGPUShaderModuleDescriptor shaderDescriptor{};
    shaderDescriptor.nextInChain = &code.chain;
    NativeShader module(wgpuDeviceCreateShaderModule(device->nativeDevice(), &shaderDescriptor));
    if (!module || !device->diagnostic().isEmpty()) {
        error = device->diagnostic();
        return {};
    }
    WGPUComputePipelineDescriptor descriptor{};
    descriptor.compute.module = module.get();
    descriptor.compute.entryPoint = {entryPoint, WGPU_STRLEN};
    NativeComputePipeline result(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &descriptor));
    if (!result || !device->diagnostic().isEmpty()) {
        error = device->diagnostic().isEmpty() ? QStringLiteral("GPU compute pipeline creation failed") : device->diagnostic();
        return {};
    }
    return result;
}

std::shared_ptr<Tile> TileRenderer::Private::allocateTile()
{
    if (!ensureTileCapacity(1)) return {};
    auto pixels = createBuffer(device->nativeDevice(), TileBytes,
                               WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    if (!pixels || !device->diagnostic().isEmpty()) return {};
    return std::make_shared<Tile>(std::move(pixels), budget);
}

bool TileRenderer::Private::ensureTileCapacity(qsizetype count)
{
    if (coverageVersion.isNull() || count > (budget->limit - budget->used) / TileBytes) {
        coverage.clear();
        coverageVersion.clear();
        coverageLayer = 0;
    }
    if (backdropVersion.isNull() || count > (budget->limit - budget->used) / TileBytes) {
        projectionBackdrops.clear();
        backdropVersion.clear();
    }
    return count <= (budget->limit - budget->used) / TileBytes;
}

RenderedVersion::RenderedVersion(std::shared_ptr<Data> data) : m_data(std::move(data)) {}
DevicePtr RenderedVersion::device() const { return m_data->owner->device; }
Snapshot RenderedVersion::document() const { return m_data->document; }
QList<TileView> RenderedVersion::tiles(LayerId layer) const
{
    QList<TileView> result;
    const auto found = m_data->layers.constFind(layer);
    if (found == m_data->layers.cend()) return result;
    result.reserve(found->size());
    for (auto i = found->cbegin(); i != found->cend(); ++i) result.append({coordinate(i.key()), i.value()->pixels.get()});
    return result;
}

TileRenderer::TileRenderer(DevicePtr device, qsizetype byteBudget)
{
    if (!device || device->thread() != QThread::currentThread() || byteBudget < TileBytes) {
        throw std::invalid_argument("invalid GPU owner thread or tile budget");
    }
    m_d = std::make_unique<Private>(std::move(device), byteBudget);
}
TileRenderer::~TileRenderer() = default;
qsizetype TileRenderer::residentBytes() const
{
    m_d->ensureTileCapacity(0);
    return m_d->budget->used;
}

RenderOutcome TileRenderer::record(WGPUCommandEncoder commands, const Snapshot &document, const RenderedSnapshot &base)
{
    QList<LayerId> layers;
    if (document) for (const auto &layer : document->layers) layers.append(layer.id);
    return recordImpl(commands, document, base, std::nullopt, layers);
}

RenderOutcome TileRenderer::recordRegion(WGPUCommandEncoder commands, const Snapshot &document,
                                        QRect region, const QList<LayerId> &layers, const RenderedSnapshot &base)
{
    constexpr qint64 limit = qint64(OperationDocument::CoordinateLimit);
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU tile region is outside the supported range")};
    }
    if (region.isEmpty()) return recordImpl(commands, document, base, QRect{}, layers);
    const int left = int(std::floor(qreal(region.left()) / TileSize));
    const int top = int(std::floor(qreal(region.top()) / TileSize));
    const int right = int(std::floor(qreal(region.right()) / TileSize));
    const int bottom = int(std::floor(qreal(region.bottom()) / TileSize));
    return recordImpl(commands, document, base,
                      QRect(left * TileSize, top * TileSize, (right - left + 1) * TileSize, (bottom - top + 1) * TileSize), layers);
}

RenderOutcome TileRenderer::recordImpl(WGPUCommandEncoder commands, const Snapshot &document,
                                      const RenderedSnapshot &base, std::optional<QRect> region,
                                      const QList<LayerId> &layers)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || !d.device->ownsCommands(commands) || !document ||
        (base && (base->m_data->owner != d.budget || base->document()->documentId() != document->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU versions require their owning device and document")};
    }
    if (!d.error.isEmpty() || d.device->isLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    if (!d.selectionZeroUploaded) {
        wgpuCommandEncoderClearBuffer(commands, d.unusedSelection.get(), 0, SelectionMask::AlphaTileBytes);
        d.selectionZeroUploaded = true;
    }
    QSet<LayerId> available, selected, requestedGroups;
    QHash<LayerId, const Layer *> nodes;
    QHash<QUuid, QList<const Layer *>> children;
    QList<const Layer *> selectedGroups;
    for (const auto &layer : document->layers) {
        available.insert(layer.id);
        nodes.insert(layer.id, &layer);
        children[layer.parentUuid].append(&layer);
    }
    for (const auto layer : layers) {
        if (!available.contains(layer) || selected.contains(layer)) {
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU tile layer is unavailable or repeated")};
        }
        selected.insert(layer);
        if (nodes.value(layer)->kind == LayerKind::Group) {
            selectedGroups.append(nodes.value(layer));
            requestedGroups.insert(layer);
        }
    }
    for (qsizetype index = 0; index < selectedGroups.size(); ++index) {
        for (const auto child : children.value(selectedGroups[index]->uuid)) {
            if (!child->visible || !child->opacity || selected.contains(child->id)) continue;
            selected.insert(child->id);
            if (child->kind == LayerKind::Group) selectedGroups.append(child);
        }
    }
    if (base && base->document() == document && base->m_data->region == region && base->m_data->selected == selected &&
        ((region && region->isEmpty()) || std::all_of(requestedGroups.cbegin(), requestedGroups.cend(),
            [&](LayerId group) { return base->m_data->groupInputs.contains(group); })))
        return {RenderOutcome::Recorded, base, {}};
    auto data = std::make_shared<RenderedVersion::Data>();
    data->document = document;
    data->owner = d.budget;
    data->region = region;
    data->selected = selected;
    if (!selectedGroups.isEmpty()) data->defaultPixels = document->projectionDefaultPixels();
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect layerDomain(-limit, -limit, 2 * limit, 2 * limit);
    const QRect paintRegion = region ? region->intersected(layerDomain) : layerDomain;
    std::vector<PaintJob> jobs;
    struct FillUpload { std::shared_ptr<NativeBuffer> buffer; QList<FigureFillEdge> edges; };
    std::vector<FillUpload> fillUploads;
    struct Checkpoint { qsizetype operation = 0; QSharedPointer<const RasterTile> pixels; };
    struct Upload { std::shared_ptr<Tile> tile; QByteArray rgba; };
    std::vector<Upload> uploads;
    struct MaskUpload { std::shared_ptr<NativeBuffer> buffer; QByteArray bytes; };
    std::vector<MaskUpload> maskUploads;
    // Binary and alpha coverage tiles share a coordinate but not a tile size.
    QHash<QPair<QPoint, bool>, std::shared_ptr<NativeBuffer>> selectionMaskBuffers;
    LayerId pendingCoverageLayer = 0;
    bool preserveCoverage = false;
    Tiles nextCoverage;
    bool coverageInitialized = false;
    size_t recordedJobs = 0, recordedFills = 0, recordedUploads = 0, recordedMaskUploads = 0;
    QList<std::shared_ptr<Tile>> spatialPins;
    const auto flushPaint = [&]() -> std::optional<RenderOutcome> {
        const bool reuseCoverage = preserveCoverage && d.coverageVersion.toStrongRef() == base &&
            d.coverageLayer == pendingCoverageLayer;
        if (reuseCoverage && !coverageInitialized) {
            const auto &inputs = data->strokeInputs[pendingCoverageLayer];
            for (auto tile = d.coverage.cbegin(); tile != d.coverage.cend(); ++tile)
                if (inputs.contains(tile.key())) nextCoverage[tile.key()] = tile.value();
        }
        if (pendingCoverageLayer) coverageInitialized = true;
        for (size_t i = recordedJobs; i < jobs.size(); ++i) {
            auto &job = jobs[i];
            if (job.retainCoverage) {
                nextCoverage.remove(job.coverageCoordinate);
                if (d.budget->limit - d.budget->used >= TileBytes) {
                    job.coverageOutput = d.allocateTile();
                    if (!job.coverageOutput)
                        return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU coverage allocation failed")};
                    if (reuseCoverage && job.extendsCoverage)
                        job.coverageInput = d.coverage.value(job.coverageCoordinate);
                    if (job.coverageInput) {
                        job.geometry = std::move(job.appendedGeometry);
                        job.values.count = quint32(job.geometry.size());
                        job.values.paintFlags |= 128u;
                    }
                    job.values.paintFlags |= 256u;
                    nextCoverage[job.coverageCoordinate] = job.coverageOutput;
                }
            }
        }
        if (!preparePaintJobs(*d.device, d.layout.get(), jobs, recordedJobs, d.zero.get(),
                              d.unusedCoverage.get(), d.unusedSelection.get(), d.unusedPattern.get()))
            return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
        if (!d.zeroUploaded) {
            wgpuCommandEncoderClearBuffer(commands, d.zero.get(), 0, TileBytes);
            d.zeroUploaded = true;
        }
        for (size_t i = recordedFills; i < fillUploads.size(); ++i) {
            const auto &upload = fillUploads[i];
            wgpuQueueWriteBuffer(d.device->queue(), upload.buffer->get(), 0, upload.edges.constData(),
                                 quint64(upload.edges.size()) * sizeof(FigureFillEdge));
        }
        for (size_t i = recordedUploads; i < uploads.size(); ++i) {
            const auto &upload = uploads[i];
            wgpuQueueWriteBuffer(d.device->queue(), upload.tile->pixels.get(), 0, upload.rgba.constData(), size_t(upload.rgba.size()));
        }
        for (size_t i = recordedMaskUploads; i < maskUploads.size(); ++i) {
            const auto &upload = maskUploads[i];
            wgpuQueueWriteBuffer(d.device->queue(), upload.buffer->get(), 0, upload.bytes.constData(), size_t(upload.bytes.size()));
        }
        if (recordedJobs < jobs.size()) {
            const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
            if (!pass) return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint commands failed")};
            wgpuComputePassEncoderSetPipeline(pass, d.pipeline.get());
            for (size_t i = recordedJobs; i < jobs.size(); ++i) {
                wgpuComputePassEncoderSetBindGroup(pass, 0, jobs[i].bindings.get(), 0, nullptr);
                wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
            }
            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);
        }
        recordedJobs = jobs.size();
        recordedFills = fillUploads.size();
        recordedUploads = uploads.size();
        recordedMaskUploads = maskUploads.size();
        return std::nullopt;
    };
    if (base && base->document()->projectionBackground == document->projectionBackground)
        data->background = base->m_data->background;
    if (!document->directProjectionLayer() && packedPixel(document->projectionBackground) && !data->background) {
        if (!d.ensureTileCapacity(1))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU canvas background tile budget exhausted")};
        PaintJob fill;
        fill.output = d.allocateTile();
        fill.values.color = packedPixel(document->projectionBackground);
        fill.values.uniformFill = 1;
        if (!fill.output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU canvas background allocation failed")};
        fill.output->mayContainHiddenRgb = (fill.values.color >> 24) == 0;
        data->background = fill.output;
        jobs.push_back(std::move(fill));
    }
    for (const auto &layer : document->layers) {
        if (!selected.contains(layer.id) || (region && region->isEmpty())) continue;
        if (layer.passThrough && !requestedGroups.contains(layer.id)) continue;
        const auto uniform = data->defaultPixels.value(layer.id, layer.defaultPixel);
        QList<QRect> operationRegions(layer.operations.size() + 1, paintRegion);
        for (qsizetype i = layer.operations.size(); i-- > 0;) {
            auto needed = operationRegions[i + 1];
            if (const auto crop = std::get_if<CropSnapshot>(&layer.operations[i]); crop && !needed.isEmpty()) {
                const int left = pixelTileIndex(needed.left(), TileSize);
                const int top = pixelTileIndex(needed.top(), TileSize);
                const int right = pixelTileIndex(needed.right(), TileSize);
                const int bottom = pixelTileIndex(needed.bottom(), TileSize);
                needed = QRect(left * TileSize, top * TileSize, (right - left + 1) * TileSize,
                               (bottom - top + 1) * TileSize).intersected((*crop)->rect.normalized());
            }
            if (const auto translation = std::get_if<TranslationSnapshot>(&layer.operations[i]); translation && !needed.isEmpty()) {
                needed = needed.translated(-(*translation)->offset).intersected(layerDomain);
                if (!needed.isEmpty()) {
                    const int left = pixelTileIndex(needed.left(), TileSize);
                    const int top = pixelTileIndex(needed.top(), TileSize);
                    const int right = pixelTileIndex(needed.right(), TileSize);
                    const int bottom = pixelTileIndex(needed.bottom(), TileSize);
                    needed = QRect(left * TileSize, top * TileSize,
                                   (right - left + 1) * TileSize, (bottom - top + 1) * TileSize).intersected(layerDomain);
                }
            }
            operationRegions[i] = needed;
        }
        qsizetype prefix = 0;
        qsizetype reusedPrefix = 0;
        QSet<quint64> reusedCoordinates;
        std::optional<qsizetype> appendedFrom;
        Tiles strokeInputs;
        if (base) {
            for (const auto &old : base->document()->layers) {
                if (old.id != layer.id) continue;
                if (base->m_data->defaultPixels.value(layer.id, old.defaultPixel) != uniform) break;
                data->defaults[layer.id] = base->m_data->defaults.value(layer.id);
                const bool droppingStroke = old.operations.size() == layer.operations.size() + 1 &&
                    base->m_data->strokeInputs.contains(layer.id);
                if ((old.operations.size() > layer.operations.size() && !droppingStroke) || !base->m_data->selected.contains(layer.id)) break;
                qsizetype shared = droppingStroke ? layer.operations.size() : old.operations.size();
                bool changingStroke = false;
                qsizetype previousSamples = -1;
                if (shared > 0 && shared == layer.operations.size() && base->m_data->strokeInputs.contains(layer.id)) {
                    const auto previous = std::get_if<PaintSnapshot>(&old.operations.back());
                    const auto current = std::get_if<PaintSnapshot>(&layer.operations.back());
                    changingStroke = previous && current && *previous != *current && (*previous)->id == (*current)->id &&
                        (*previous)->brush == (*current)->brush;
                    if (changingStroke && !(*current)->brush.gaussianWash && !(*current)->brush.pixelPerfect && !(*current)->brush.basicSmoothing && (*previous)->shape.kind == ShapeKind::Freehand &&
                        (*current)->shape.kind == ShapeKind::Freehand && (*current)->sampleCount >= (*previous)->sampleCount)
                        previousSamples = (*previous)->sampleCount;
                    if (changingStroke) --shared;
                }
                bool matches = true;
                for (qsizetype i = 0; i < shared; ++i) {
                    if (old.operations[i] != layer.operations[i]) { matches = false; break; }
                }
                const auto sourceRegion = operationRegions[shared];
                bool sharedSpatialOperation = false;
                for (qsizetype i = 0; i < shared; ++i)
                    sharedSpatialOperation = sharedSpatialOperation || std::holds_alternative<TranslationSnapshot>(layer.operations[i]) ||
                        std::holds_alternative<CropSnapshot>(layer.operations[i]);
                if (matches && (!sharedSpatialOperation || !base->m_data->region || base->m_data->region->contains(sourceRegion))) {
                    const auto &oldTiles = base->m_data->layers[layer.id];
                    for (auto tile = oldTiles.cbegin(); tile != oldTiles.cend(); ++tile) {
                        if (!sourceRegion.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) continue;
                        data->layers[layer.id][tile.key()] = tile.value();
                        reusedCoordinates.insert(tile.key());
                    }
                    if (changingStroke || droppingStroke) {
                        if (previousSamples >= 0) appendedFrom = std::max<qsizetype>(0, previousSamples - 1);
                        if (appendedFrom && document->pendingOperation() == std::get<PaintSnapshot>(layer.operations.back())->id) {
                            pendingCoverageLayer = layer.id;
                            preserveCoverage = true;
                        }
                        const auto &inputs = base->m_data->strokeInputs[layer.id];
                        for (auto input = inputs.cbegin(); input != inputs.cend(); ++input) {
                            if (!reusedCoordinates.contains(input.key())) continue;
                            if (appendedFrom) strokeInputs[input.key()] = input.value();
                            else if (input.value() && input.value() != data->defaults.value(layer.id))
                                data->layers[layer.id][input.key()] = input.value();
                            else data->layers[layer.id].remove(input.key());
                        }
                    } else if (base->m_data->strokeInputs.contains(layer.id) && !old.operations.isEmpty()) {
                        const auto paint = std::get_if<PaintSnapshot>(&old.operations.back());
                        if (paint && document->pendingOperation() == (*paint)->id) {
                            pendingCoverageLayer = layer.id;
                            preserveCoverage = true;
                            auto &inputs = data->strokeInputs[layer.id];
                            const auto &oldInputs = base->m_data->strokeInputs[layer.id];
                            for (auto input = oldInputs.cbegin(); input != oldInputs.cend(); ++input)
                                if (reusedCoordinates.contains(input.key())) inputs[input.key()] = input.value();
                        }
                    }
                    reusedPrefix = shared;
                    if (!base->m_data->region || base->m_data->region->contains(sourceRegion)) prefix = reusedPrefix;
                }
                break;
            }
        }
        if (packedPixel(uniform) && !data->defaults.value(layer.id)) {
            if (!d.ensureTileCapacity(1)) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU default pixel tile budget exhausted")};
            }
            PaintJob fill;
            fill.output = d.allocateTile();
            fill.values.color = packedPixel(uniform);
            fill.values.uniformFill = 1;
            if (!fill.output) {
                return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU default pixel allocation failed")};
            }
            fill.output->mayContainHiddenRgb = (fill.values.color >> 24) == 0;
            data->defaults[layer.id] = fill.output;
            jobs.push_back(std::move(fill));
        }
        if (layer.kind == LayerKind::Group) continue;
        auto &tiles = data->layers[layer.id];
        for (qsizetype epoch = prefix; epoch < layer.operations.size();) {
            qsizetype end = epoch;
            while (end < layer.operations.size() && !std::holds_alternative<TranslationSnapshot>(layer.operations[end]) &&
                   !std::holds_alternative<CropSnapshot>(layer.operations[end]) &&
                   !std::holds_alternative<FilterSnapshot>(layer.operations[end])) ++end;
            const auto paintRegion = operationRegions[epoch];
            QMap<quint64, std::array<std::shared_ptr<Tile>, 2>> writable;
            QMap<quint64, Checkpoint> checkpoints;
            for (qsizetype op = epoch; op < end; ++op) {
                const auto write = std::get_if<TileWriteSnapshot>(&layer.operations[op]);
                if (!write) continue;
                for (const auto &tile : (*write)->tiles) {
                    const auto point = tile->coordinate();
                    if (!paintRegion.intersects(QRect(point * TileSize, QSize(TileSize, TileSize)))) continue;
                    const auto coordinate = key(point.x(), point.y());
                    if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                    checkpoints[coordinate] = {op, tile};
                    if (checkpoints.size() > d.budget->limit / TileBytes) {
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
                    }
                }
            }
            if (!d.ensureTileCapacity(checkpoints.size())) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
            }
            // Each coordinate starts at its last whole-tile write. Later pixel-local
            // paint operations alternate the two writable buffers in operation order.
            for (auto checkpoint = checkpoints.cbegin(); checkpoint != checkpoints.cend(); ++checkpoint) {
                const auto pixels = checkpoint->pixels->rgba();
                if (pixels.size() != TileBytes) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("CPU raster checkpoint is incomplete")};
                const auto output = d.allocateTile();
                if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU checkpoint allocation failed")};
                output->mayContainHiddenRgb = containsHiddenRgb(pixels);
                tiles[checkpoint.key()] = output;
                writable[checkpoint.key()][0] = output;
                uploads.push_back({output, pixels});
            }
            for (qsizetype op = epoch; op < end; ++op) {
                const auto paint = std::get_if<PaintSnapshot>(&layer.operations[op]);
                if (!paint || paintRegion.isEmpty()) continue;
                const auto &operation = **paint;
                const bool fillEnabled = hasFigureFill(operation.figureStyle);
                if (!operation.figureStyle.outline && !fillEnabled) continue;
                const bool pending = document->pendingOperation() == operation.id;
                const bool extending = appendedFrom && op == layer.operations.size() - 1;
                const bool retainCoverage = pending && !operation.brush.gaussianWash && operation.shape.kind == ShapeKind::Freehand &&
                    !operation.brush.pixelPerfect && !operation.brush.basicSmoothing && !fillEnabled;
                if (retainCoverage) pendingCoverageLayer = layer.id;
                if (pending) data->strokeInputs[layer.id];
                if (pending && extending) data->strokeInputs[layer.id] = strokeInputs;
                const auto diameter = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * 16)));
                const auto step = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * operation.brush.spacing * 16)));
                const auto gaussian = operation.brush.gaussianWash ? gaussianDabs(operation, pending) : std::optional<QList<GaussianDab>>(QList<GaussianDab>{});
                if (!gaussian) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("This Gaussian WASH geometry or brush setting is not yet supported")};
                const auto geometry = operation.brush.gaussianWash ? std::optional<PaintGeometry>(PaintGeometry{}) : paintGeometry(operation, step);
                if (!geometry) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU derived geometry limit exceeded")};
                const auto &segments = geometry->outline;
                bool appendedOnly = extending && retainCoverage && preserveCoverage &&
                    d.coverageVersion.toStrongRef() == base && d.coverageLayer == layer.id &&
                    base->m_data->selected == selected && base->m_data->defaultPixels == data->defaultPixels &&
                    (!base->m_data->region || base->m_data->region->contains(paintRegion));
                if (appendedOnly) {
                    const qint64 regionTiles =
                        (qint64(pixelTileIndex(paintRegion.right(), TileSize)) - pixelTileIndex(paintRegion.left(), TileSize) + 1) *
                        (qint64(pixelTileIndex(paintRegion.bottom(), TileSize)) - pixelTileIndex(paintRegion.top(), TileSize) + 1);
                    appendedOnly = regionTiles <= (d.budget->limit - d.budget->used) / TileBytes / 2;
                    for (auto input = strokeInputs.cbegin(); appendedOnly && input != strokeInputs.cend(); ++input)
                        appendedOnly = bool(d.coverage.value(input.key()));
                    for (const auto &other : document->layers) {
                        if (!appendedOnly) break;
                        if (other.id == layer.id || !selected.contains(other.id)) continue;
                        const auto &oldLayers = base->document()->layers;
                        const auto old = std::find_if(oldLayers.cbegin(), oldLayers.cend(),
                                                      [&](const auto &candidate) { return candidate.id == other.id; });
                        appendedOnly = old != oldLayers.cend() && old->defaultPixel == other.defaultPixel &&
                            old->operations == other.operations;
                    }
                }
                QMap<quint64, QList<PaintSegment>> affected;
                QMap<quint64, QList<GaussianDab>> gaussianAffected;
                qsizetype gaussianReferences = 0;
                QSet<quint64> changedCoordinates;
                QMap<quint64, QList<PaintSegment>> appendedGeometry;
                for (const auto &dab : *gaussian) {
                    if (!operation.figureStyle.outline) break;
                    const auto bounds = dab.bounds().intersected(paintRegion);
                    if (bounds.isEmpty()) continue;
                    const int left = pixelTileIndex(bounds.left(), TileSize), right = pixelTileIndex(bounds.right(), TileSize);
                    const int top = pixelTileIndex(bounds.top(), TileSize), bottom = pixelTileIndex(bounds.bottom(), TileSize);
                    if (qint64(right - left + 1) * (bottom - top + 1) > d.budget->limit / TileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian tile budget exhausted")};
                    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                        const auto coordinate = key(x, y);
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        affected[coordinate];
                        if (gaussianReferences == PathGeometrySegmentLimit)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian geometry reference limit exceeded")};
                        gaussianAffected[coordinate].append(dab);
                        ++gaussianReferences;
                        if (affected.size() > d.budget->limit / TileBytes)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian tile budget exhausted")};
                    }
                }
                for (qsizetype index = appendedOnly ? *appendedFrom : 0;
                     operation.figureStyle.outline && index < segments.size(); ++index) {
                    const auto &segment = segments[index];
                    const auto coordinates = paintSegmentTiles(segment, diameter, operation.brush, paintRegion,
                                                                 d.budget->limit / TileBytes);
                    if (!coordinates) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                    for (const auto coordinate : *coordinates) {
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        affected[coordinate].append(segment);
                        if (extending && index >= *appendedFrom) {
                            changedCoordinates.insert(coordinate);
                            if (retainCoverage) appendedGeometry[coordinate].append(segment);
                        }
                        if (affected.size() > d.budget->limit / TileBytes)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                    }
                }
                if (fillEnabled) {
                    const auto filled = figureFillTiles(geometry->fill, operation.brush, paintRegion,
                                                         d.budget->limit / TileBytes);
                    if (!filled) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill tile budget exhausted")};
                    for (const auto coordinate : *filled) {
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        if (!affected.contains(coordinate)) affected.insert(coordinate, {});
                    }
                    if (affected.size() > d.budget->limit / TileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill tile budget exhausted")};
                }
                if (extending) {
                    for (auto tile = affected.begin(); tile != affected.end();) {
                        if (strokeInputs.contains(tile.key()) && !changedCoordinates.contains(tile.key())) tile = affected.erase(tile);
                        else ++tile;
                    }
                }
                qsizetype required = 0;
                for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                    const auto pool = writable.value(tile.key());
                    const int next = pool[0] && tiles.value(tile.key()) == pool[0] ? 1 : 0;
                    if (!pool[next]) ++required;
                }
                if (!d.ensureTileCapacity(required)) {
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                }
                std::shared_ptr<NativeBuffer> fillBuffer;
                if (fillEnabled && !affected.isEmpty()) {
                    auto edges = geometry->fill;
                    // A point outline can survive an empty fill. Storage bindings
                    // still require one edge; a zero edge contributes no area.
                    if (edges.isEmpty()) edges.append(FigureFillEdge{});
                    const quint64 bytes = quint64(edges.size()) * sizeof(FigureFillEdge);
                    if (bytes > std::numeric_limits<quint32>::max())
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill geometry limit exceeded")};
                    fillBuffer = std::make_shared<NativeBuffer>(createBuffer(d.device->nativeDevice(), bytes,
                                                                             WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst));
                    if (!*fillBuffer) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU fill geometry allocation failed")};
                    fillUploads.push_back({fillBuffer, edges});
                }
                std::shared_ptr<NativeBuffer> patternBuffer;
                if (operation.figureStyle.pattern && !affected.isEmpty()) {
                    const auto &pattern = *operation.figureStyle.pattern;
                    if (!pattern.isValid())
                        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU pattern fill has invalid pixels")};
                    WGPULimits limits{};
                    if (wgpuDeviceGetLimits(d.device->nativeDevice(), &limits) != WGPUStatus_Success ||
                        quint64(pattern.byteCount()) > limits.maxStorageBufferBindingSize ||
                        quint64(pattern.byteCount()) > limits.maxBufferSize)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU pattern fill exceeds the storage binding limit")};
                    patternBuffer = std::make_shared<NativeBuffer>(createBuffer(d.device->nativeDevice(),
                        quint64(pattern.byteCount()), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst));
                    if (!*patternBuffer)
                        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU pattern fill allocation failed")};
                    wgpuQueueWriteBuffer(d.device->queue(), patternBuffer->get(), 0,
                                         pattern.rgba.constData(), size_t(pattern.rgba.size()));
                }
                for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                    const auto bytes = quint64(tile.value().size()) * sizeof(PaintSegment);
                    if (bytes > std::numeric_limits<quint32>::max()) {
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU operation buffer limit exceeded")};
                    }
                    PaintJob job;
                    job.input = extending && strokeInputs.contains(tile.key()) ? strokeInputs.value(tile.key()) :
                        tiles.value(tile.key(), data->defaults.value(layer.id));
                    if (pending) data->strokeInputs[layer.id][tile.key()] = job.input;
                    auto &pool = writable[tile.key()];
                    auto &output = pool[pool[0] && job.input == pool[0] ? 1 : 0];
                    if (!output) output = d.allocateTile();
                    job.output = output;
                    if (!job.output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
                    job.geometry = tile.value();
                    job.gaussianGeometry = gaussianAffected.value(tile.key());
                    job.retainCoverage = retainCoverage;
                    job.coverageCoordinate = tile.key();
                    job.extendsCoverage = extending;
                    job.appendedGeometry = appendedGeometry.value(tile.key());
                    job.fillEdges = fillBuffer;
                    job.pattern = patternBuffer;
                    const auto point = coordinate(tile.key()) * TileSize;
                    job.values = {point.x(), point.y(), 0, 0,
                                  packedPixel(operation.brush.rgba),
                                  operation.brush.opacity, quint32(operation.brush.erase), step, quint32(tile.value().size()), diameter, 0, quint32(operation.brush.alphaLocked) | (quint32(operation.brush.channelWriteMask) << 1) |
                                      (quint32(operation.brush.mirrorHorizontal) << 5) | (quint32(operation.brush.mirrorVertical) << 6),
                                  operation.brush.mirrorCenter.x() * 16, operation.brush.mirrorCenter.y() * 16,
                                  operation.figureStyle.fill ? packedPixel(*operation.figureStyle.fill) : 0,
                                  quint32(fillEnabled) | (operation.figureStyle.outline ? 0u : 2u) |
                                      (operation.shape.windingFill ? 4u : 0u) |
                                      (operation.figureStyle.pattern ? 8u : 0u) |
                                      (operation.figureStyle.canvasOpacity ? 16u : 0u)};
                    if (operation.figureStyle.pattern) {
                        job.values.patternWidth = quint32(operation.figureStyle.pattern->size.width());
                        job.values.patternHeight = quint32(operation.figureStyle.pattern->size.height());
                    }
                    job.values.fillBlendMode = quint32(operation.figureStyle.fillBlendMode);
                    job.values.strokeBlendMode = quint32(operation.brush.blendMode);
                    job.values.selectionX = operation.selectionRect.x();
                    job.values.selectionY = operation.selectionRect.y();
                    job.values.selectionWidth = operation.selectionRect.width();
                    job.values.selectionHeight = operation.selectionRect.height();
                    if (operation.selectionInverted) job.values.paintFlags |= 1024u;
                    if (operation.selectionKind == SelectionKind::Ellipse) job.values.paintFlags |= 2048u;
                    if (operation.selectionKind == SelectionKind::Freehand) {
                        if (!operation.selectionMask || operation.selectionMask->isEmpty())
                            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU freehand selection has no coverage")};
                        job.values.paintFlags |= 4096u;
                        const bool alpha = operation.selectionMask->isAlpha();
                        if (alpha) job.values.paintFlags |= 8192u;
                        const QPoint maskTile = coordinate(tile.key());
                        const auto cacheKey = qMakePair(maskTile, alpha);
                        auto cached = selectionMaskBuffers.constFind(cacheKey);
                        if (cached != selectionMaskBuffers.cend()) {
                            job.selectionCoverage = *cached;
                        } else {
                            const auto coverage = operation.selectionMask->tiles().constFind(
                                SelectionTileKey(maskTile.x(), maskTile.y()));
                            if (coverage != operation.selectionMask->tiles().cend()) {
                                auto buffer = std::make_shared<NativeBuffer>(createBuffer(d.device->nativeDevice(),
                                    operation.selectionMask->tileBytes(), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst));
                                if (!*buffer)
                                    return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selection coverage allocation failed")};
                                maskUploads.push_back({buffer, coverage.value()});
                                selectionMaskBuffers.insert(cacheKey, buffer);
                                job.selectionCoverage = buffer;
                            }
                        }
                    }
                    if (operation.figureStyle.canvasOpacity) {
                        const float opacity = float(*operation.figureStyle.canvasOpacity);
                        std::memcpy(&job.values.paddingX, &opacity, sizeof(opacity));
                    }
                    if (operation.brush.gaussianWash) {
                        const float opacity = float(operation.brush.gaussianWash->canvasOpacity);
                        std::memcpy(&job.values.paddingX, &opacity, sizeof(opacity));
                        job.values.count = quint32(job.gaussianGeometry.size());
                        job.values.paintFlags |= 512u;
                    }
                    job.output->mayContainHiddenRgb = operation.brush.alphaLocked || operation.brush.erase ||
                        operation.figureStyle.canvasOpacity ||
                        operation.figureStyle.pattern || (job.input && job.input->mayContainHiddenRgb);
                    tiles[tile.key()] = job.output;
                    jobs.push_back(std::move(job));
                }
                if (appendedOnly) {
                    if (const auto failure = flushPaint()) return *failure;
                }
            }
            if (end == layer.operations.size()) break;
            if (const auto failure = flushPaint()) return *failure;
            if (const auto crop = std::get_if<CropSnapshot>(&layer.operations[end])) {
                auto cropped = d.cropTiles(commands, tiles, (*crop)->rect.normalized(), packedPixel(layer.defaultPixel),
                                           operationRegions[end + 1], spatialPins);
                if (cropped.code != RenderOutcome::Recorded) return {cropped.code, {}, cropped.diagnostic};
                tiles = std::move(cropped.tiles);
            } else if (const auto filter = std::get_if<FilterSnapshot>(&layer.operations[end])) {
                auto filtered = d.filterTiles(commands, tiles, **filter, data->defaults.value(layer.id),
                                              document->size, operationRegions[end + 1], spatialPins);
                if (filtered.code != RenderOutcome::Recorded) return {filtered.code, {}, filtered.diagnostic};
                tiles = std::move(filtered.tiles);
            } else {
                const auto &translation = std::get<TranslationSnapshot>(layer.operations[end]);
                auto moved = d.translateTiles(commands, tiles, data->defaults.value(layer.id), translation->offset,
                                               operationRegions[end + 1], spatialPins);
                if (moved.code != RenderOutcome::Recorded) return {moved.code, {}, moved.diagnostic};
                tiles = std::move(moved.tiles);
            }
            epoch = end + 1;
        }
    }
    if (const auto failure = flushPaint()) return *failure;
    const RenderedSnapshot version(new RenderedVersion(data));
    if (!region || !region->isEmpty()) {
        QHash<QUuid, LayerId> groups;
        QHash<LayerId, int> remaining;
        QList<LayerId> ready;
        for (const auto group : selectedGroups) { groups.insert(group->uuid, group->id); remaining.insert(group->id, 0); }
        for (const auto group : selectedGroups)
            if (groups.contains(group->parentUuid)) ++remaining[groups.value(group->parentUuid)];
        for (const auto group : selectedGroups) if (remaining.value(group->id) == 0) ready.append(group->id);
        for (qsizetype index = 0; index < ready.size(); ++index) {
            const auto group = ready[index];
            if (!nodes.value(group)->passThrough || requestedGroups.contains(group)) {
                ProjectedSnapshot previous;
                if (base && base->m_data->groupInputs.contains(group)) {
                    auto cached = std::make_shared<ProjectedVersion::Data>();
                    cached->source = base;
                    cached->region = paintRegion;
                    cached->tiles = base->m_data->layers.value(group);
                    cached->inputs = base->m_data->groupInputs.value(group);
                    cached->directLayerProjection = base->m_data->directGroups.contains(group);
                    previous = ProjectedSnapshot(new ProjectedVersion(cached));
                }
                const auto projected = composeImpl(commands, version, paintRegion, previous, group);
                if (projected.code != RenderOutcome::Recorded) return {projected.code, {}, projected.diagnostic};
                data->layers[group] = projected.version->m_data->tiles;
                data->groupInputs[group] = projected.version->m_data->inputs;
                if (projected.version->m_data->directLayerProjection) data->directGroups.insert(group);
            }
            const auto parent = groups.value(nodes.value(group)->parentUuid);
            if (parent && --remaining[parent] == 0) ready.append(parent);
        }
    }
    d.coverage = std::move(nextCoverage);
    d.coverageVersion = version;
    d.coverageLayer = pendingCoverageLayer;
    return {RenderOutcome::Recorded, version, {}};
}
}
