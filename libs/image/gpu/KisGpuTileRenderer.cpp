/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRenderer.h"
#include "KisGpuPaintJob_p.h"
#include "KisGpuPipelineCache_p.h"
#include <QCryptographicHash>
#include "KisGpuRgba8AlphaBlend_p.h"
#include "KisGpuRgba16Compositing_p.h"

#include <QFile>
#include <QCryptographicHash>
#include <QMap>
#include <QThread>
#include <QScopeGuard>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
QList<QRect> layerOperationRegions(const Layer &layer, const QRect &paintRegion)
{
    constexpr int TileSize = RgbaTile::Size;
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect layerDomain(-limit, -limit, 2 * limit, 2 * limit);
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
            const auto sourceRegion = needed.translated(-(*translation)->offset).intersected(layerDomain);
            needed = (*translation)->selectedPixels ?
                needed.united(sourceRegion.intersected((*translation)->copyRect)) : sourceRegion;
            if (!needed.isEmpty()) {
                const int left = pixelTileIndex(needed.left(), TileSize);
                const int top = pixelTileIndex(needed.top(), TileSize);
                const int right = pixelTileIndex(needed.right(), TileSize);
                const int bottom = pixelTileIndex(needed.bottom(), TileSize);
                needed = QRect(left * TileSize, top * TileSize,
                               (right - left + 1) * TileSize, (bottom - top + 1) * TileSize).intersected(layerDomain);
            }
        }
        if (const auto filter = std::get_if<FilterSnapshot>(&layer.operations[i]);
            filter && isConvolutionFilter((*filter)->kind) && !needed.isEmpty())
            needed = needed.adjusted(-1, -1, 1, 1).intersected(layerDomain);
        if (const auto affine = std::get_if<AffineSnapshot>(&layer.operations[i]); affine && !needed.isEmpty()) {
            auto transformed = needed;
            for (qsizetype stage = (*affine)->plan->passes.size(); stage-- > 0;)
                transformed = affineNeededRegion((*affine)->plan->passes[stage], transformed);
            if ((*affine)->selection) needed |= transformed.intersected((*affine)->selection->copyRect);
            else needed = transformed;
            needed &= layerDomain;
        }
        operationRegions[i] = needed;
    }
    return operationRegions;
}

bool containsHiddenRgb(const QByteArray &pixels, PixelFormat format)
{
    const int bytes = pixelByteCount(format);
    for (qsizetype i = 0; i < pixels.size(); i += bytes) {
        const auto pixel = rgbaPixel(QByteArrayView(pixels).sliced(i, bytes), format);
        if (pixel && (*pixel)[3] == 0 && ((*pixel)[0] || (*pixel)[1] || (*pixel)[2])) return true;
    }
    return false;
}
}

TileRenderer::Private::Private(TileRenderer &renderer, DevicePtr device, qsizetype byteBudget)
    : renderer(renderer), device(std::move(device)), budget(std::make_shared<Budget>(Budget{this->device, byteBudget})),
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
    if (!unusedCoverage || !unusedSelection || !unusedPattern || !opaqueMask) {
        error = QStringLiteral("GPU paint resources could not be created");
        return;
    }
    initializeProjection();
}

TileRenderer::Private::~Private() = default;

NativeComputePipeline TileRenderer::Private::createPipeline(const QString &path, const char *entryPoint,
    WGPUBindGroupLayout sharedLayout, quint32 firstBlend, quint32 secondBlend,
    quint32 coupledPixelFormat, bool generatedBrush)
{
    if (device->isLost()) { error = QStringLiteral("GPU device lost"); return {}; }
    QByteArray source;
    const auto append = [&](const QString &resource) {
        QFile file(resource);
        if (!file.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU shader resource could not be loaded: %1").arg(resource);
            return false;
        }
        source += file.readAll() + '\n';
        return true;
    };
    const auto library = [&](const char *name) {
        return append(QStringLiteral(":/librepaint/gpu/shaders/") + QLatin1String(name));
    };
    if (!library("rgba.wgsl")) return {};
    const bool painting = path.endsWith(QLatin1String("/paint.wgsl"));
    const bool region = path.endsWith(QLatin1String("/region.wgsl"));
    const bool composite = painting || path.endsWith(QLatin1String("/over.wgsl")) ||
        path.endsWith(QLatin1String("/selection-move.wgsl")) || path.endsWith(QLatin1String("/filter.wgsl"));
    if ((composite || region) && !library("binary32.wgsl")) return {};
    const auto coupled = [](quint32 mode) { return mode >= 88 && mode <= 126 ? mode : 0u; };
    const std::array<double, 7> values{double(coupled(firstBlend)), double(coupled(secondBlend)),
        double(coupledPixelFormat == 2u ? firstBlend : 0u), double(coupledPixelFormat == 2u ? secondBlend : 0u),
        double(coupledPixelFormat == 2u && ((firstBlend && !coupled(firstBlend)) || (secondBlend && !coupled(secondBlend)))),
        double(coupledPixelFormat), double(generatedBrush)};
    const std::array<const char *, 7> keys{"COLOR_BLEND_MODE_A", "COLOR_BLEND_MODE_B", "WORD_BLEND_MODE_A",
        "WORD_BLEND_MODE_B", "SEPARABLE_BLEND_ENABLED", "COUPLED_PIXEL_FORMAT", "GENERATED_BRUSH_ENABLED"};
    if (composite && !library("composite-settings.wgsl")) return {};
    if (composite) {
        for (const auto name : {"color-blend.wgsl", "separable.wgsl", "normal-rgba8.wgsl",
                               "normal-rgba16.wgsl", "separable-rgba16.wgsl", "copy.wgsl"})
            if (!library(name)) return {};
    }
    if (painting) {
        for (const auto name : {"gaussian.wgsl", "image-brush.wgsl", "gradient.wgsl", "gradient-angular.wgsl"})
            if (!library(name)) return {};
    }
    if (region && !library("native-lab.wgsl")) return {};
    if (!append(path)) return {};
    if (!device->m_programs) device->m_programs = std::make_shared<PipelineCache>();
    auto &cache = *device->m_programs;
    const auto moduleKey = QCryptographicHash::hash(source, QCryptographicHash::Sha256);
    QByteArray identity = moduleKey + QByteArray(entryPoint) + '\0';
    const auto layoutIdentity = reinterpret_cast<quintptr>(sharedLayout);
    identity.append(reinterpret_cast<const char *>(&layoutIdentity), sizeof(layoutIdentity));
    if (composite) identity.append(reinterpret_cast<const char *>(values.data()), sizeof(values));
    const auto pipelineKey = QCryptographicHash::hash(identity, QCryptographicHash::Sha256);
    if (const auto cached = cache.pipelines.value(pipelineKey)) {
        wgpuComputePipelineAddRef(cached->get());
        return NativeComputePipeline(cached->get());
    }
    auto module = cache.modules.value(moduleKey);
    if (!module) {
        WGPUShaderSourceWGSL code{};
        code.chain.sType = WGPUSType_ShaderSourceWGSL;
        code.code = {source.constData(), size_t(source.size())};
        WGPUShaderModuleDescriptor descriptor{};
        descriptor.nextInChain = &code.chain;
        NativeShader compiled(wgpuDeviceCreateShaderModule(device->nativeDevice(), &descriptor));
        if (!compiled || !device->diagnostic().isEmpty()) { error = device->diagnostic(); return {}; }
        module = std::make_shared<NativeShader>(std::move(compiled));
        cache.modules.insert(moduleKey, module);
    }
    NativeResource<WGPUPipelineLayout, wgpuPipelineLayoutRelease> explicitLayout;
    if (sharedLayout) {
        WGPUPipelineLayoutDescriptor descriptor{};
        descriptor.bindGroupLayoutCount = 1;
        descriptor.bindGroupLayouts = &sharedLayout;
        explicitLayout.reset(wgpuDeviceCreatePipelineLayout(device->nativeDevice(), &descriptor));
        if (!explicitLayout) return {};
    }
    WGPUComputePipelineDescriptor descriptor{};
    descriptor.layout = explicitLayout.get();
    descriptor.compute.module = module->get();
    descriptor.compute.entryPoint = {entryPoint, WGPU_STRLEN};
    std::array<WGPUConstantEntry, 7> constants{};
    if (composite) {
        for (size_t i = 0; i < constants.size(); ++i) {
            constants[i].key = {keys[i], WGPU_STRLEN};
            constants[i].value = values[i];
        }
        descriptor.compute.constantCount = constants.size();
        descriptor.compute.constants = constants.data();
    }
    NativeComputePipeline compiled(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &descriptor));
    if (!compiled || !device->diagnostic().isEmpty()) {
        error = device->diagnostic().isEmpty() ? QStringLiteral("GPU compute pipeline creation failed") : device->diagnostic();
        return {};
    }
    const auto retained = std::make_shared<NativeComputePipeline>(std::move(compiled));
    cache.pipelines.insert(pipelineKey, retained);
    wgpuComputePipelineAddRef(retained->get());
    return NativeComputePipeline(retained->get());
}

WGPUComputePipeline TileRenderer::Private::blendPipeline(bool paint, bool paired, PixelFormat format,
    quint32 firstBlend, quint32 secondBlend, bool generatedBrush)
{
    if (!firstBlend && !secondBlend && !generatedBrush)
        return paint ? pipeline.get() : paired ? pairedProjectionPipeline.get() : projectionPipeline.get();
    if (secondBlend < firstBlend) std::swap(firstBlend, secondBlend);
    const auto key = firstBlend | (secondBlend << 8u) | (quint32(paint) << 16u) | (quint32(paired) << 17u) |
        (quint32(format == PixelFormat::Rgba16) << 18u) | (quint32(generatedBrush) << 19u);
    const auto found = blendPipelines.find(key);
    if (found != blendPipelines.end()) return found->second.get();
    auto compiled = createPipeline(paint ? QStringLiteral(":/librepaint/gpu/shaders/paint.wgsl") :
        QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"), paired ? "mainPair" : "main",
        paint ? layout.get() : paired ? pairedProjectionLayout.get() : projectionLayout.get(),
        firstBlend, secondBlend, format == PixelFormat::Rgba16 ? 2u : 1u, generatedBrush);
    if (!compiled) return nullptr;
    const auto result = compiled.get();
    blendPipelines.emplace(key, std::move(compiled));
    return result;
}

WGPUBuffer TileRenderer::Private::zeroBuffer(WGPUCommandEncoder commands, PixelFormat format)
{
    auto &buffer = zero[format == PixelFormat::Rgba16 ? 1 : 0];
    if (!buffer) {
        const auto bytes = RgbaTile::byteCount(format);
        buffer = createBuffer(device->nativeDevice(), bytes,
                              WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        if (!buffer) return nullptr;
        wgpuCommandEncoderClearBuffer(commands, buffer.get(), 0, bytes);
    }
    return buffer.get();
}

std::shared_ptr<Tile> TileRenderer::Private::allocateTile(PixelFormat format)
{
    if (!ensureTileCapacity(1, format)) return {};
    auto pixels = createBuffer(device->nativeDevice(), RgbaTile::byteCount(format),
                               WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    if (!pixels || !device->diagnostic().isEmpty()) return {};
    return std::make_shared<Tile>(std::move(pixels), budget);
}

bool TileRenderer::Private::ensureTileCapacity(qsizetype count, PixelFormat format)
{
    if (count > (budget->limit - budget->used) / RgbaTile::byteCount(format)) patternCache.clear();
    if (count > (budget->limit - budget->used) / RgbaTile::byteCount(format)) {
        nativeLabValues.reset();
        nativeLabDefinition.clear();
    }
    if (coverageVersion.isNull() || count > (budget->limit - budget->used) / RgbaTile::byteCount(format)) {
        coverage.clear();
        coverageVersion.clear();
        coverageLayer = 0;
    }
    if (backdropVersion.isNull() || count > (budget->limit - budget->used) / RgbaTile::byteCount(format)) {
        projectionBackdrops.clear();
        backdropVersion.clear();
    }
    return count <= (budget->limit - budget->used) / RgbaTile::byteCount(format);
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
    m_d = std::make_unique<Private>(*this, std::move(device), byteBudget);
}
TileRenderer::~TileRenderer() = default;
qsizetype TileRenderer::residentBytes() const
{
    m_d->ensureTileCapacity(0, PixelFormat::Rgba8);
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

ProjectionOutcome TileRenderer::recordInitialProjection(WGPUCommandEncoder commands,
    const FrozenLayerProjectionSP &initial, QRect region, const RenderedSnapshot &base)
{
    if (!initial || !initial->source)
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Missing frozen layer projection")};
    const auto aligned = [](QRect region, const FrozenLayerProjectionSP &value) {
        region &= value->extent;
        if (region.isEmpty()) return QRect{};
        const auto first = QPoint(pixelTileIndex(region.left(), TileSize), pixelTileIndex(region.top(), TileSize));
        const auto last = QPoint(pixelTileIndex(region.right(), TileSize), pixelTileIndex(region.bottom(), TileSize));
        return QRect(first * TileSize, (last + QPoint(1, 1)) * TileSize - QPoint(1, 1));
    };
    auto &cache = m_d->initialProjections;
    const auto cached = [&](const FrozenLayerProjectionSP &value, QRect area) {
        const auto found = cache.constFind(value.data());
        if (found != cache.cend()) for (const auto &entry : *found)
            if (entry.region == area) return entry.pixels;
        return ProjectedSnapshot{};
    };
    struct Pending { FrozenLayerProjectionSP value; QRect region; bool expanded = false; };
    region = aligned(region, initial);
    std::vector<Pending> pending{{initial, region}};
    QSet<const FrozenLayerProjection *> visiting;
    while (!pending.empty()) {
        auto &task = pending.back();
        if (cached(task.value, task.region)) { pending.pop_back(); continue; }
        if (!task.expanded) {
            if (visiting.contains(task.value.data()))
                return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Cyclic frozen layer projection")};
            visiting.insert(task.value.data()); task.expanded = true;
            QHash<LayerId, const Layer *> nodes;
            QHash<QUuid, QList<const Layer *>> children;
            for (const auto &layer : task.value->source->layers) {
                nodes.insert(layer.id, &layer); children[layer.parentUuid].append(&layer);
            }
            QList<const Layer *> selected;
            QSet<LayerId> included;
            for (const auto &input : task.value->source->projectionInputs()) {
                selected.append(nodes.value(input.layer)); included.insert(input.layer);
            }
            std::vector<Pending> dependencies;
            for (qsizetype i = 0; i < selected.size(); ++i) {
                const auto layer = selected[i];
                if (layer->kind == LayerKind::Group) {
                    for (const auto child : children.value(layer->uuid)) {
                        if (!child->visible || !child->opacity || included.contains(child->id)) continue;
                        included.insert(child->id); selected.append(child);
                    }
                } else if (layer->initialProjection) {
                    const auto area = aligned(layerOperationRegions(*layer, task.region).first(), layer->initialProjection);
                    if (!cached(layer->initialProjection, area)) dependencies.push_back({layer->initialProjection, area});
                }
            }
            for (auto dependency = dependencies.rbegin(); dependency != dependencies.rend(); ++dependency)
                pending.push_back(std::move(*dependency));
            continue;
        }
        QList<LayerId> layers;
        for (const auto &input : task.value->source->projectionInputs()) layers.append(input.layer);
        const auto compatible = base && base->document()->documentId() == task.value->source->documentId() ? base : RenderedSnapshot{};
        const auto recorded = recordRegion(commands, task.value->source, task.region, layers, compatible);
        if (recorded.code != RenderOutcome::Recorded) return {recorded.code, {}, recorded.diagnostic};
        auto projected = composeImpl(commands, recorded.version, task.region, {}, 0, task.value.data(),
            false, task.value->nativeSources);
        if (projected.code != RenderOutcome::Recorded) return projected;
        if (task.value->normalizeRootProjection) {
            // The root projection is applied normally onto a new empty paint device.
            auto pixels = std::make_shared<RenderedVersion::Data>();
            pixels->document = task.value->source; pixels->owner = m_d->budget;
            pixels->region = task.region; pixels->selected = {0};
            pixels->layers[0] = projected.version->m_data->tiles;
            pixels->sourceExtents[0] = task.value->extent;
            projected = composeImpl(commands, RenderedSnapshot(new RenderedVersion(pixels)), task.region,
                {}, 0, task.value.data(), true);
            if (projected.code != RenderOutcome::Recorded) return projected;
        }
        cache[task.value.data()].append({task.region, projected.version});
        visiting.remove(task.value.data()); pending.pop_back();
    }
    return {RenderOutcome::Recorded, cached(initial, region), {}};
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
    if (d.regionDepth++ == 0) { d.regionCache.clear(); d.initialProjections.clear(); }
    const auto regionScope = qScopeGuard([&] {
        if (--d.regionDepth == 0) { d.regionCache.clear(); d.initialProjections.clear(); }
    });
    if (!d.error.isEmpty() || d.device->isLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    if (!d.selectionZeroUploaded) {
        wgpuCommandEncoderClearBuffer(commands, d.unusedSelection.get(), 0, SelectionMask::AlphaTileBytes);
        d.selectionZeroUploaded = true;
    }
    const auto tileBytes = RgbaTile::byteCount(document->format);
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
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect layerDomain(-limit, -limit, 2 * limit, 2 * limit);
    const QRect paintRegion = region ? region->intersected(layerDomain) : layerDomain;
    auto recordedRegion = region;
    QHash<LayerId, QRect> layerRegions;
    const bool spatialEffects = std::any_of(document->layers.cbegin(), document->layers.cend(), [](const auto &node) {
        return node.visible && isFilterNode(node.kind) && node.filter && isConvolutionFilter(node.filter->settings.kind);
    });
    if (spatialEffects && !paintRegion.isEmpty()) {
        struct GroupInput { LayerId id; QRect region; };
        QList<GroupInput> pendingGroups;
        const auto addNode = [&](const Layer &node, QRect output) {
            const auto needed = filterMaskRegions(document, node, output).first();
            if (layerRegions.contains(node.id) && layerRegions.value(node.id).contains(needed)) return;
            layerRegions[node.id] |= needed;
            if (node.kind == LayerKind::Group) pendingGroups.append({node.id, layerRegions.value(node.id)});
        };
        const auto addScope = [&](const QList<LayerProjectionInput> &inputs, QRect output) {
            const auto regions = filterProjectionRegions(document, inputs, output);
            for (const auto &input : inputs) {
                const auto node = nodes.value(input.layer);
                if (node && selected.contains(node->id)) addNode(*node, regions.output.value(node->id));
            }
        };
        QSet<LayerId> roots;
        for (const auto id : layers) {
            roots.insert(id);
            addNode(*nodes.value(id), paintRegion);
        }
        QList<LayerProjectionInput> rootInputs;
        for (const auto &input : document->projectionInputs())
            if (roots.contains(input.layer)) rootInputs.append(input);
        addScope(rootInputs, paintRegion);
        for (const auto id : layers)
            if (nodes.value(id)->kind == LayerKind::Adjustment)
                addScope(document->projectionInputsThrough(id), paintRegion);
        for (qsizetype i = 0; i < pendingGroups.size(); ++i) {
            const auto task = pendingGroups[i];
            addScope(document->projectionInputs(task.id), task.region);
        }
        for (const auto id : selected) {
            if (!layerRegions.contains(id)) layerRegions.insert(id, paintRegion);
            if (recordedRegion) *recordedRegion |= layerRegions.value(id);
        }
    }
    if (base && base->document() == document && base->m_data->region == recordedRegion &&
        base->m_data->layerRegions == layerRegions && base->m_data->selected == selected &&
        ((region && region->isEmpty()) || std::all_of(requestedGroups.cbegin(), requestedGroups.cend(),
            [&](LayerId group) { return base->m_data->groupInputs.contains(group); })))
        return {RenderOutcome::Recorded, base, {}};
    auto data = std::make_shared<RenderedVersion::Data>();
    data->document = document;
    data->owner = d.budget;
    data->region = recordedRegion;
    data->layerRegions = layerRegions;
    if (spatialEffects) data->nativeFilterRegions = nativeFilterOutputRegions(document);
    data->selected = selected;
    if (!selectedGroups.isEmpty()) data->defaultPixels = document->projectionDefaultPixels();
    std::vector<PaintJob> jobs;
    struct FillUpload { std::shared_ptr<NativeBuffer> buffer; QList<FigureFillEdge> edges; };
    std::vector<FillUpload> fillUploads;
    struct Checkpoint { qsizetype operation = 0; QSharedPointer<const RasterTile> pixels; };
    struct Upload { std::shared_ptr<Tile> tile; QByteArray rgba; };
    std::vector<Upload> uploads;
    struct MaskUpload { std::shared_ptr<NativeBuffer> buffer; QByteArray bytes; };
    std::vector<MaskUpload> maskUploads;
    // A cold replay can contain different frozen masks at one coordinate.
    QHash<QPair<const SelectionMask *, QPoint>, std::shared_ptr<NativeBuffer>> selectionMaskBuffers;
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
            d.retainTile(job.input); d.retainTile(job.coverageInput);
            if (job.retainCoverage) {
                nextCoverage.remove(job.coverageCoordinate);
                if (d.budget->limit - d.budget->used >= TileBytes) {
                    job.coverageOutput = d.allocateTile(PixelFormat::Rgba8);
                    if (!job.coverageOutput)
                        return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU coverage allocation failed")};
                    if (reuseCoverage && job.extendsCoverage)
                        job.coverageInput = d.coverage.value(job.coverageCoordinate);
                    if (job.coverageInput) {
                        job.geometry = std::move(job.appendedGeometry);
                        job.values.count = quint32(std::get<QList<MarkerDab>>(job.geometry).size());
                        job.values.paintFlags |= 128u;
                    }
                    job.values.paintFlags |= 256u;
                    nextCoverage[job.coverageCoordinate] = job.coverageOutput;
                }
            }
        }
        for (size_t i = recordedJobs; i < jobs.size(); ++i) d.retainTile(jobs[i].coverageInput);
        const auto empty = d.zeroBuffer(commands, document->format);
        if (!empty) return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU empty tile allocation failed")};
        if (recordedJobs < jobs.size() && !d.pipeline) {
            d.pipeline = d.createPipeline(QStringLiteral(":/librepaint/gpu/shaders/paint.wgsl"));
            if (!d.pipeline) return RenderOutcome{RenderOutcome::DeviceFailure, {}, d.error};
            d.layout.reset(wgpuComputePipelineGetBindGroupLayout(d.pipeline.get(), 0));
            if (!d.layout) {
                d.error = QStringLiteral("GPU paint bindings could not be created");
                return RenderOutcome{RenderOutcome::DeviceFailure, {}, d.error};
            }
        }
        if (!preparePaintJobs(*d.device, d.layout.get(), jobs, recordedJobs, empty,
                              d.unusedCoverage.get(), d.unusedSelection.get(), d.unusedPattern.get()))
            return RenderOutcome{RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
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
            for (size_t i = recordedJobs; i < jobs.size(); ++i) {
                const auto program = d.blendPipeline(true, false, document->format, jobs[i].values.strokeBlendMode,
                    jobs[i].values.fillBlendMode, (jobs[i].values.paintFlags & 512u) != 0u);
                if (!program) {
                    wgpuComputePassEncoderEnd(pass);
                    wgpuComputePassEncoderRelease(pass);
                    return RenderOutcome{RenderOutcome::DeviceFailure, {}, d.error};
                }
                wgpuComputePassEncoderSetPipeline(pass, program);
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
    if (!document->directProjectionLayer() && document->projectionBackground != RgbaPixel{} && !data->background) {
        if (!d.ensureTileCapacity(1, document->format))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU canvas background tile budget exhausted")};
        PaintJob fill;
        fill.output = d.allocateTile(document->format);
        const auto words = packedPixelWords(document->projectionBackground, document->format);
        fill.values.color = words[0];
        fill.values.colorHigh = words[1];
        fill.values.pixelFormat = quint32(document->format);
        fill.values.paintFlags |= UniformPixelFill;
        if (!fill.output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU canvas background allocation failed")};
        fill.output->mayContainHiddenRgb = document->projectionBackground[3] == 0;
        data->background = fill.output;
        jobs.push_back(std::move(fill));
    }
    const bool needsSourceExtents = std::any_of(document->layers.cbegin(), document->layers.cend(), [](const auto &layer) {
        return transparentSourceAffectsBlend(layer.blendMode) || !(layer.projectionChannelMask & 8u);
    });
    for (const auto &layer : document->layers) {
        if (!selected.contains(layer.id) || (region && region->isEmpty())) continue;
        if (isFilterNode(layer.kind)) continue;
        if (layer.passThrough && !requestedGroups.contains(layer.id)) continue;
        const auto ownPixels = layer.kind == LayerKind::Paint ? document->renderedPixels(layer.id) : LayerPixelsSP{};
        const auto startPixels = ownPixels ? ownPixels : layer.pixels;
        const auto *start = startPixels ? dynamic_cast<const RenderedLayerPixels *>(startPixels.get()) : nullptr;
        if (startPixels && (!start || start->owner != d.budget))
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU layer pixels belong to another renderer")};
        if (ownPixels) {
            // An earlier frame rendered exactly this version; no operation is evaluated again.
            data->layers[layer.id] = start->tiles;
            data->defaultPixels[layer.id] = start->uniform;
            if (start->defaultTile) data->defaults[layer.id] = start->defaultTile;
            if (needsSourceExtents) {
                auto allocation = std::make_shared<LayerAllocation>();
                for (const auto &block : start->allocatedBlocks) allocation->blocks += block;
                allocation->extent = allocation->blocks.boundingRect();
                if (layer.defaultPixel[3]) allocation->extent |= QRect(QPoint(), document->size);
                data->sourceExtents[layer.id] = allocation->extent;
                data->allocations[layer.id] = std::move(allocation);
            }
            continue;
        }
        const bool clipsPainting =std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &value) {
            const auto paint = std::get_if<PaintSnapshot>(&value);
            return paint && ((*paint)->brush.alphaLocked || !((*paint)->brush.channelWriteMask & 8u));
        });
        std::shared_ptr<const LayerAllocation> allocation;
        if ((needsSourceExtents || clipsPainting) && layer.kind == LayerKind::Paint) {
            if (base && base->document()->size == document->size && base->m_data->allocations.contains(layer.id)) {
                const auto &previous = base->document()->layers;
                const auto old = std::find_if(previous.cbegin(), previous.cend(), [&](const auto &value) { return value.id == layer.id; });
                if (old != previous.cend() && old->position == layer.position && old->defaultPixel == layer.defaultPixel &&
                    old->pixels == layer.pixels && old->initialProjection == layer.initialProjection &&
                    old->operations == layer.operations && base->document()->pendingOperation() == document->pendingOperation())
                    allocation = base->m_data->allocations.value(layer.id);
            }
            if (!allocation) {
                auto computed = paintLayerAllocation(document, layer, PaintGeometryElementLimit);
                if (!computed) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU layer allocation geometry capacity exceeded")};
                allocation = std::make_shared<LayerAllocation>(std::move(*computed));
            }
            data->allocations[layer.id] = allocation;
            data->sourceExtents[layer.id] = allocation->extent;
        }
        const auto uniform = data->defaultPixels.value(layer.id, layer.defaultPixel);
        const auto operationRegions = layerOperationRegions(layer, layerRegions.value(layer.id, paintRegion));
        const std::optional<QRect> previousRegion = base && base->m_data->layerRegions.contains(layer.id)
            ? std::optional<QRect>(base->m_data->layerRegions.value(layer.id)) : base ? base->m_data->region : std::nullopt;
        qsizetype prefix = 0;
        qsizetype reusedPrefix = 0;
        QSet<quint64> reusedCoordinates;
        PaintSnapshot appendedOperation;
        Tiles strokeInputs;
        bool reusedInitial = false;
        if (base) {
            for (const auto &old : base->document()->layers) {
                if (old.id != layer.id) continue;
                if (old.pixels != layer.pixels || old.initialProjection != layer.initialProjection) break;
                if (base->m_data->defaultPixels.value(layer.id, old.defaultPixel) != uniform) break;
                data->defaults[layer.id] = base->m_data->defaults.value(layer.id);
                const bool droppingStroke = old.operations.size() == layer.operations.size() + 1 &&
                    base->m_data->strokeInputs.contains(layer.id);
                if ((old.operations.size() > layer.operations.size() && !droppingStroke) || !base->m_data->selected.contains(layer.id)) break;
                qsizetype shared = droppingStroke ? layer.operations.size() : old.operations.size();
                bool changingStroke = false;
                PaintSnapshot growingStroke;
                if (shared > 0 && shared == layer.operations.size() && base->m_data->strokeInputs.contains(layer.id)) {
                    const auto previous = std::get_if<PaintSnapshot>(&old.operations.back());
                    const auto current = std::get_if<PaintSnapshot>(&layer.operations.back());
                    changingStroke = previous && current && *previous != *current && (*previous)->id == (*current)->id &&
                        (*previous)->brush == (*current)->brush;
                    if (changingStroke && !(*current)->brush.autoBrush && !(*current)->brush.pixelPerfect && !(*current)->brush.basicSmoothing && (*previous)->shape.kind == ShapeKind::Freehand &&
                        (*current)->shape.kind == ShapeKind::Freehand && (*current)->sampleCount >= (*previous)->sampleCount &&
                        (*previous)->figureStyle.outline && (*current)->figureStyle.outline &&
                        !hasFigureFill((*previous)->figureStyle) && !hasFigureFill((*current)->figureStyle))
                        growingStroke = *previous;
                    if (changingStroke) --shared;
                }
                bool matches = true;
                for (qsizetype i = 0; i < shared; ++i) {
                    if (old.operations[i] != layer.operations[i]) { matches = false; break; }
                }
                const auto sourceRegion = operationRegions[shared];
                bool sharedSpatialOperation = false;
                for (qsizetype i = 0; i < shared; ++i) {
                    sharedSpatialOperation = sharedSpatialOperation || std::holds_alternative<TranslationSnapshot>(layer.operations[i]) ||
                        std::holds_alternative<AffineSnapshot>(layer.operations[i]) ||
                        std::holds_alternative<CropSnapshot>(layer.operations[i]);
                    if (const auto filter = std::get_if<FilterSnapshot>(&layer.operations[i]); filter && isConvolutionFilter((*filter)->kind))
                        sharedSpatialOperation = true;
                }
                if (matches && (!sharedSpatialOperation || !previousRegion || previousRegion->contains(sourceRegion))) {
                    const auto &oldTiles = base->m_data->layers[layer.id];
                    for (auto tile = oldTiles.cbegin(); tile != oldTiles.cend(); ++tile) {
                        if (!sourceRegion.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) continue;
                        data->layers[layer.id][tile.key()] = tile.value();
                        reusedCoordinates.insert(tile.key());
                    }
                    if (changingStroke || droppingStroke) {
                        appendedOperation = growingStroke;
                        if (appendedOperation && document->pendingOperation() == std::get<PaintSnapshot>(layer.operations.back())->id) {
                            pendingCoverageLayer = layer.id;
                            preserveCoverage = true;
                        }
                        const auto &inputs = base->m_data->strokeInputs[layer.id];
                        for (auto input = inputs.cbegin(); input != inputs.cend(); ++input) {
                            if (!reusedCoordinates.contains(input.key())) continue;
                            if (appendedOperation) strokeInputs[input.key()] = input.value();
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
                    if (!previousRegion || previousRegion->contains(sourceRegion)) {
                        prefix = reusedPrefix;
                        reusedInitial = true;
                    }
                }
                break;
            }
        }
        if (start && start->defaultTile && start->uniform == uniform && !data->defaults.value(layer.id))
            data->defaults[layer.id] = start->defaultTile;
        if (uniform != RgbaPixel{} && !data->defaults.value(layer.id)) {
            if (!d.ensureTileCapacity(1, document->format)) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU default pixel tile budget exhausted")};
            }
            PaintJob fill;
            fill.output = d.allocateTile(document->format);
            const auto words = packedPixelWords(uniform, document->format);
            fill.values.color = words[0];
            fill.values.colorHigh = words[1];
            fill.values.pixelFormat = quint32(document->format);
            fill.values.paintFlags |= UniformPixelFill;
            if (!fill.output) {
                return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU default pixel allocation failed")};
            }
            fill.output->mayContainHiddenRgb = uniform[3] == 0;
            data->defaults[layer.id] = fill.output;
            jobs.push_back(std::move(fill));
        }
        if (layer.kind == LayerKind::Group) continue;
        auto &tiles = data->layers[layer.id];
        if (start) {
            const auto startRegion = operationRegions.first();
            for (auto tile = start->tiles.cbegin(); tile != start->tiles.cend(); ++tile) {
                if (reusedCoordinates.contains(tile.key()) ||
                    !startRegion.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) continue;
                tiles[tile.key()] = tile.value();
            }
        }
        if (layer.initialProjection && !reusedInitial) {
            if (const auto failure = flushPaint()) return *failure;
            const auto initial = recordInitialProjection(commands, layer.initialProjection, operationRegions.first(), base);
            if (initial.code != RenderOutcome::Recorded) return {initial.code, {}, initial.diagnostic};
            const auto restored = d.restoreImplicitPixels(commands, initial.version->m_data->tiles,
                layer.initialProjection->allocatedBlocks, layer.defaultPixel, document->format);
            if (restored.code != RenderOutcome::Recorded) return {restored.code, {}, restored.diagnostic};
            for (auto tile = restored.tiles.cbegin(); tile != restored.tiles.cend(); ++tile)
                if (!reusedCoordinates.contains(tile.key())) tiles[tile.key()] = tile.value();
        }
        for (qsizetype epoch = prefix; epoch < layer.operations.size();) {
            qsizetype end = epoch;
            while (end < layer.operations.size() && !std::holds_alternative<TranslationSnapshot>(layer.operations[end]) &&
                   !std::holds_alternative<CropSnapshot>(layer.operations[end]) &&
                   !std::holds_alternative<FilterSnapshot>(layer.operations[end]) &&
                   !std::holds_alternative<AffineSnapshot>(layer.operations[end])) ++end;
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
                    if (checkpoints.size() > d.budget->limit / tileBytes) {
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
                    }
                }
            }
            if (!d.ensureTileCapacity(checkpoints.size(), document->format)) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
            }
            // Each coordinate starts at its last whole-tile write. Later pixel-local
            // paint operations alternate the two writable buffers in operation order.
            for (auto checkpoint = checkpoints.cbegin(); checkpoint != checkpoints.cend(); ++checkpoint) {
                const auto pixels = checkpoint->pixels->rgba();
                if (pixels.size() != tileBytes) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("CPU raster checkpoint is incomplete")};
                const auto output = d.allocateTile(document->format);
                if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU checkpoint allocation failed")};
                output->mayContainHiddenRgb = containsHiddenRgb(pixels, document->format);
                tiles[checkpoint.key()] = output;
                writable[checkpoint.key()][0] = output;
                uploads.push_back({output, pixels});
            }
            for (qsizetype op = epoch; op < end; ++op) {
                const auto paint = std::get_if<PaintSnapshot>(&layer.operations[op]);
                if (!paint || paintRegion.isEmpty()) continue;
                const auto &operation = **paint;
                auto compositionBounds = operation.compositionBounds;
                std::optional<QRect> nonCopyDestination;
                if (operation.brush.alphaLocked || !(operation.brush.channelWriteMask & 8u)) {
                    auto permitted = allocation->beforeOperations[op];
                    if (compositionBounds) permitted &= *compositionBounds;
                    if (hasCopyBlend(operation)) nonCopyDestination = permitted;
                    else {
                        if (permitted.isEmpty()) continue;
                        compositionBounds = permitted;
                    }
                }
                const bool fillEnabled = hasFigureFill(operation.figureStyle);
                const bool replacesAlpha = usesAllocatedSourceRegion(operation, document->format);
                if (!operation.figureStyle.outline && !fillEnabled) continue;
                QRect geometryRegion = paintRegion;
                if (replacesAlpha) {
                    const QPoint first(pixelTileIndex(paintRegion.left(), TileSize), pixelTileIndex(paintRegion.top(), TileSize));
                    const QPoint last(pixelTileIndex(paintRegion.right(), TileSize), pixelTileIndex(paintRegion.bottom(), TileSize));
                    geometryRegion = QRect(first * TileSize, (last + QPoint(1, 1)) * TileSize - QPoint(1, 1));
                }
                const bool pending = document->pendingOperation() == operation.id;
                const bool extending = appendedOperation && op == layer.operations.size() - 1;
                const bool retainCoverage = pending && !operation.brush.autoBrush && operation.shape.kind == ShapeKind::Freehand &&
                    !operation.brush.pixelPerfect && !operation.brush.basicSmoothing && !fillEnabled;
                if (retainCoverage) pendingCoverageLayer = layer.id;
                if (pending) data->strokeInputs[layer.id];
                if (pending && extending) data->strokeInputs[layer.id] = strokeInputs;
                const auto gaussian = operation.brush.autoBrush ? autoBrushDabs(operation, pending) :
                    std::optional<QList<AutoBrushDab>>(QList<AutoBrushDab>{});
                if (!gaussian) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("This generated brush geometry or setting is not yet supported")};
                const auto marker = operation.brush.autoBrush ? std::optional<QList<MarkerDab>>(QList<MarkerDab>{}) :
                    markerDabs(operation, pending, geometryRegion, replacesAlpha);
                if (!marker) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU native marker geometry limit exceeded")};
                const auto fillEdges = figureFillEdges(operation);
                if (!fillEdges) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill geometry limit exceeded")};
                QHash<quint64, quint32> sourceBlocks;
                if (replacesAlpha) {
                    qsizetype references = 0;
                    const auto markBlock = [&](int x, int y) {
                        const int tx = pixelTileIndex(x, 4), ty = pixelTileIndex(y, 4);
                        sourceBlocks[key(tx, ty)] |= 1u << ((y - ty * 4) * 4 + x - tx * 4);
                    };
                    const auto markBounds = [&](QRect bounds) {
                        bounds = bounds.intersected(geometryRegion);
                        if (bounds.isEmpty()) return true;
                        const int left = pixelTileIndex(bounds.left(), PixelHistoryBlockSize);
                        const int right = pixelTileIndex(bounds.right(), PixelHistoryBlockSize);
                        const int top = pixelTileIndex(bounds.top(), PixelHistoryBlockSize);
                        const int bottom = pixelTileIndex(bounds.bottom(), PixelHistoryBlockSize);
                        if (qint64(right - left + 1) * (bottom - top + 1) > d.budget->limit / tileBytes * 16) return false;
                        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                            if (++references > PaintGeometryElementLimit * 16) return false;
                            markBlock(x, y);
                        }
                        return true;
                    };
                    if (operation.figureStyle.outline) for (const auto &dab : *gaussian)
                        if (!markBounds(dab.bounds()))
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU paint source region capacity exceeded")};
                    for (const auto &dab : *marker)
                        if (!markBounds(dab.bounds()))
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU paint source region capacity exceeded")};
                    if (fillEnabled) {
                        const auto blocks = figureFillTiles(*fillEdges, operation.brush, geometryRegion,
                                                            d.budget->limit / tileBytes * 16, PixelHistoryBlockSize);
                        if (!blocks) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill source region capacity exceeded")};
                        for (const auto block : *blocks) {
                            const auto point = coordinate(block);
                            markBlock(point.x(), point.y());
                        }
                    }
                }
                qsizetype appendedIndex = 0;
                if (extending) {
                    const auto previous = markerDabs(*appendedOperation, true, geometryRegion, replacesAlpha);
                    if (!previous || previous->size() > marker->size())
                        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU appended marker geometry does not retain its prefix")};
                    appendedIndex = previous->size();
                }
                bool appendedOnly = !replacesAlpha && extending && retainCoverage && preserveCoverage &&
                    d.coverageVersion.toStrongRef() == base && d.coverageLayer == layer.id &&
                    base->m_data->selected == selected && base->m_data->defaultPixels == data->defaultPixels &&
                    (!previousRegion || previousRegion->contains(paintRegion));
                if (appendedOnly) {
                    const qint64 regionTiles =
                        (qint64(pixelTileIndex(paintRegion.right(), TileSize)) - pixelTileIndex(paintRegion.left(), TileSize) + 1) *
                        (qint64(pixelTileIndex(paintRegion.bottom(), TileSize)) - pixelTileIndex(paintRegion.top(), TileSize) + 1);
                    appendedOnly = regionTiles <= (d.budget->limit - d.budget->used) / tileBytes / 2;
                    for (auto input = strokeInputs.cbegin(); appendedOnly && input != strokeInputs.cend(); ++input)
                        appendedOnly = bool(d.coverage.value(input.key()));
                    for (const auto &other : document->layers) {
                        if (!appendedOnly) break;
                        if (other.id == layer.id || !selected.contains(other.id)) continue;
                        const auto &oldLayers = base->document()->layers;
                        const auto old = std::find_if(oldLayers.cbegin(), oldLayers.cend(),
                                                      [&](const auto &candidate) { return candidate.id == other.id; });
                        appendedOnly = old != oldLayers.cend() && old->defaultPixel == other.defaultPixel &&
                            old->pixels == other.pixels && old->operations == other.operations;
                    }
                }
                QMap<quint64, QList<MarkerDab>> affected;
                QMap<quint64, QList<AutoBrushDab>> gaussianAffected;
                qsizetype gaussianReferences = 0;
                QSet<quint64> changedCoordinates;
                QMap<quint64, QList<MarkerDab>> appendedGeometry;
                for (const auto &dab : *gaussian) {
                    if (!operation.figureStyle.outline) break;
                    const auto bounds = dab.bounds().intersected(geometryRegion);
                    if (bounds.isEmpty()) continue;
                    const int left = pixelTileIndex(bounds.left(), TileSize), right = pixelTileIndex(bounds.right(), TileSize);
                    const int top = pixelTileIndex(bounds.top(), TileSize), bottom = pixelTileIndex(bounds.bottom(), TileSize);
                    if (qint64(right - left + 1) * (bottom - top + 1) > d.budget->limit / tileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian tile budget exhausted")};
                    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                        const auto coordinate = key(x, y);
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        affected[coordinate];
                        if (gaussianReferences == PaintGeometryElementLimit)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian geometry reference limit exceeded")};
                        gaussianAffected[coordinate].append(dab);
                        ++gaussianReferences;
                        if (affected.size() > d.budget->limit / tileBytes)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU Gaussian tile budget exhausted")};
                    }
                }
                qsizetype markerReferences = 0;
                for (qsizetype index = appendedOnly ? appendedIndex : 0; index < marker->size(); ++index) {
                    const auto &dab = (*marker)[index];
                    const auto bounds = dab.bounds().intersected(geometryRegion);
                    if (bounds.isEmpty()) continue;
                    const int left = pixelTileIndex(bounds.left(), TileSize), right = pixelTileIndex(bounds.right(), TileSize);
                    const int top = pixelTileIndex(bounds.top(), TileSize), bottom = pixelTileIndex(bounds.bottom(), TileSize);
                    if (qint64(right - left + 1) * (bottom - top + 1) > d.budget->limit / tileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU marker tile budget exhausted")};
                    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                        const auto coordinate = key(x, y);
                        const QRect tileBounds(QPoint(x, y) * TileSize, QSize(TileSize, TileSize));
                        if (!replacesAlpha && !markerDabIntersects(dab, tileBounds.intersected(geometryRegion))) continue;
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        if (markerReferences == PaintGeometryElementLimit)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU marker geometry reference limit exceeded")};
                        affected[coordinate].append(dab);
                        ++markerReferences;
                        if (extending && index >= appendedIndex) {
                            changedCoordinates.insert(coordinate);
                            if (retainCoverage) appendedGeometry[coordinate].append(dab);
                        }
                        if (affected.size() > d.budget->limit / tileBytes)
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU marker tile budget exhausted")};
                    }
                }
                if (fillEnabled) {
                    const auto filled = figureFillTiles(*fillEdges, operation.brush, geometryRegion,
                                                         d.budget->limit / tileBytes);
                    if (!filled) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill tile budget exhausted")};
                    for (const auto coordinate : *filled) {
                        if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                        const auto checkpoint = checkpoints.constFind(coordinate);
                        if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                        if (!affected.contains(coordinate)) affected.insert(coordinate, {});
                    }
                    if (affected.size() > d.budget->limit / tileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU fill tile budget exhausted")};
                }
                if (compositionBounds) {
                    for (auto tile = affected.begin(); tile != affected.end();) {
                        if (!compositionBounds->intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) tile = affected.erase(tile);
                        else ++tile;
                    }
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
                std::shared_ptr<BufferStorage> regionCoverage;
                if (operation.region && !affected.isEmpty()) {
                    if (const auto failure = flushPaint()) return *failure;
                    const auto native = operation.region->pixels;
                    if (native) {
                        const std::lock_guard<std::mutex> lock(native->mutex);
                        if (native->device.lock() == d.device) regionCoverage = native->region;
                    }
                    if (!regionCoverage)
                        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Generated region coverage is unavailable")};
                }
                if (!d.ensureTileCapacity(required, document->format)) {
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                }
                std::shared_ptr<NativeBuffer> fillBuffer;
                if (fillEnabled && !affected.isEmpty()) {
                    auto edges = *fillEdges;
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
                quint32 textureWordOffset = 0, imageWordOffset = 0;
                const auto imageTips = operation.brush.autoBrush ? operation.brush.autoBrush->imageTips : QSharedPointer<const ImageBrushTips>();
                const auto maskTips = operation.brush.additiveMaskingBrush && operation.brush.additiveMaskingBrush->autoBrush ?
                    operation.brush.additiveMaskingBrush->autoBrush->imageTips : QSharedPointer<const ImageBrushTips>();
                if ((operation.figureStyle.pattern || operation.figureStyle.gradient || operation.brush.texture || imageTips || maskTips) && !affected.isEmpty()) {
                    QByteArray resource;
                    if (operation.figureStyle.pattern) {
                        const auto &pattern = *operation.figureStyle.pattern;
                        if (!pattern.isValid())
                            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU pattern fill has invalid pixels")};
                        resource = pattern.rgba;
                    }
                    if (operation.figureStyle.gradient) {
                        const auto &gradient = *operation.figureStyle.gradient;
                        if (!gradient.isValid())
                            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU gradient fill has invalid resources")};
                        resource = gradient.rgba16;
                        resource.append(gradient.noise);
                    }
                    textureWordOffset = quint32(resource.size() / 4);
                    if (operation.brush.texture) resource.append(operation.brush.texture->alpha);
                    imageWordOffset = quint32(resource.size() / 4);
                    if (imageTips) resource.append(imageTips->pixels);
                    if (maskTips && maskTips != imageTips) resource.append(maskTips->pixels);
                    WGPULimits limits{};
                    if (wgpuDeviceGetLimits(d.device->nativeDevice(), &limits) != WGPUStatus_Success ||
                        quint64(resource.size()) > limits.maxStorageBufferBindingSize ||
                        quint64(resource.size()) > limits.maxBufferSize)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU painting resource exceeds the storage binding limit")};
                    const auto key = QCryptographicHash::hash(resource, QCryptographicHash::Sha256);
                    auto cached = d.patternCache.constFind(key);
                    if (cached == d.patternCache.cend() || cached->bytes != resource) {
                        if (d.patternCache.size() >= 4) d.patternCache.erase(d.patternCache.begin());
                        const auto fits = [&] { return resource.size() <= d.budget->limit - d.budget->used; };
                        if (!fits()) {
                            d.ensureTileCapacity((resource.size() + TileBytes - 1) / TileBytes, PixelFormat::Rgba8);
                        }
                        if (!fits()) {
                            d.patternCache.clear();
                            if (!fits())
                                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU painting resource exceeds the resident budget")};
                        }
                        auto buffer = createBuffer(d.device->nativeDevice(), quint64(resource.size()),
                                                   WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
                        if (!buffer)
                            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU painting resource allocation failed")};
                        wgpuQueueWriteBuffer(d.device->queue(), buffer.get(), 0, resource.constData(), size_t(resource.size()));
                        d.patternCache.insert(key, {resource, std::make_shared<BufferStorage>(std::move(buffer), d.budget)});
                        cached = d.patternCache.constFind(key);
                    }
                    d.device->retainForFrame(cached->storage);
                    patternBuffer = std::shared_ptr<NativeBuffer>(cached->storage, &cached->storage->pixels);
                }
                for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                    const auto bytes = quint64(tile.value().size()) * sizeof(MarkerDab);
                    if (bytes > std::numeric_limits<quint32>::max()) {
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU operation buffer limit exceeded")};
                    }
                    PaintJob job;
                    job.input = extending && strokeInputs.contains(tile.key()) ? strokeInputs.value(tile.key()) :
                        tiles.value(tile.key(), data->defaults.value(layer.id));
                    if (pending) data->strokeInputs[layer.id][tile.key()] = job.input;
                    auto &pool = writable[tile.key()];
                    auto &output = pool[pool[0] && job.input == pool[0] ? 1 : 0];
                    if (!output) output = d.allocateTile(document->format);
                    job.output = output;
                    if (!job.output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
                    if (operation.brush.autoBrush) job.geometry = gaussianAffected.value(tile.key());
                    else job.geometry = tile.value();
                    job.retainCoverage = retainCoverage;
                    job.coverageCoordinate = tile.key();
                    job.extendsCoverage = extending;
                    job.appendedGeometry = appendedGeometry.value(tile.key());
                    job.fillEdges = fillBuffer;
                    job.pattern = patternBuffer;
                    const auto point = coordinate(tile.key()) * TileSize;
                    const float opacity = std::max(0.f, float(operation.brush.opacity));
                    const quint32 count = quint32(std::visit([](const auto &dabs) { return dabs.size(); }, job.geometry));
                    const auto color = packedPixelWords(operation.brush.rgba, document->format);
                    const auto fill = operation.figureStyle.fill ? packedPixelWords(*operation.figureStyle.fill, document->format) :
                        std::array<quint32, 2>{};
                    job.values = {point.x(), point.y(), color[0], 0,
                                  quint32(qRound(opacity * maximumComponent(document->format))), quint32(operation.brush.erase), count,
                                  quint32(operation.brush.alphaLocked) | (quint32(operation.brush.channelWriteMask) << 1) |
                                      (quint32(operation.brush.mirrorHorizontal) << 5) | (quint32(operation.brush.mirrorVertical) << 6),
                                  operation.brush.mirrorCenter.x() * 16, operation.brush.mirrorCenter.y() * 16,
                                  fill[0],
                                  quint32(fillEnabled) | (operation.figureStyle.outline ? 0u : 2u) |
                                      (operation.shape.windingFill ? 4u : 0u) |
                                      (operation.figureStyle.pattern ? 8u : 0u) |
                                      (operation.figureStyle.gradient ? 16u : 0u)};
                    job.values.colorHigh = color[1];
                    job.values.textureReserved[0] = imageWordOffset;
                    job.values.fillColorHigh = fill[1];
                    job.values.pixelFormat = quint32(document->format);
                    if (operation.clearsSelectedPixels) job.values.paintFlags |= ClearSelectedPixels;
                    if (operation.brush.maskPainting) job.values.paintFlags |= MaskPainting;
                    job.values.sourceBlocks = sourceBlocks.value(tile.key());
                    if (document->format == PixelFormat::Rgba16 && replacesAlpha) job.values.paintFlags |= PartialRgba16Source;
                    if (compositionBounds) {
                        const auto &bounds = *compositionBounds;
                        job.values.compositionX = bounds.x();
                        job.values.compositionY = bounds.y();
                        job.values.compositionWidth = bounds.width();
                        job.values.compositionHeight = bounds.height();
                    }
                    if (nonCopyDestination) {
                        job.values.paintFlags |= RestrictNonCopyDestination;
                        const auto &bounds = *nonCopyDestination;
                        job.values.destinationX = bounds.x();
                        job.values.destinationY = bounds.y();
                        job.values.destinationWidth = bounds.width();
                        job.values.destinationHeight = bounds.height();
                    }
                    if (operation.figureStyle.pattern) {
                        const auto &pattern = *operation.figureStyle.pattern;
                        job.values.patternWidth = quint32(pattern.size.width());
                        job.values.patternHeight = quint32(pattern.size.height());
                        std::transform(pattern.inverse.cbegin(), pattern.inverse.cend(), job.values.patternInverse.begin(),
                                       [](double value) { return float(value); });
                        job.values.patternFormat = quint32(pattern.format);
                        job.values.patternSampled = pattern.sampled;
                    }
                    if (operation.figureStyle.gradient) {
                        const auto &gradient = *operation.figureStyle.gradient;
                        job.values.patternWidth = quint32(gradient.rgba16.size() / 8);
                        job.values.patternHeight = quint32(gradient.shape);
                        const std::array<double, 6> geometry{gradient.start.x(), gradient.start.y(),
                            gradient.end.x(), gradient.end.y(), gradient.antiAliasThreshold, 0};
                        std::transform(geometry.cbegin(), geometry.cend(), job.values.patternInverse.begin(),
                                       [](double value) { return float(value); });
                        job.values.patternFormat = quint32(gradient.reverse) | (quint32(gradient.dither) << 1);
                        job.values.patternSampled = quint32(gradient.repeat);
                    }
                    if (operation.brush.texture) {
                        const auto &texture = *operation.brush.texture;
                        job.values.textureWidth = texture.size.width();
                        job.values.textureHeight = texture.size.height();
                        job.values.textureX = texture.offset.x();
                        job.values.textureY = texture.offset.y();
                        job.values.textureWordOffset = textureWordOffset;
                    }
                    job.values.fillBlendMode = quint32(operation.figureStyle.fillBlendMode);
                    job.values.strokeBlendMode = quint32(operation.brush.blendMode);
                    job.values.selectionX = operation.selectionRect.x();
                    job.values.selectionY = operation.selectionRect.y();
                    job.values.selectionWidth = operation.selectionRect.width();
                    job.values.selectionHeight = operation.selectionRect.height();
                    if (operation.selectionInverted) job.values.paintFlags |= 1024u;
                    if (operation.selectionKind == SelectionKind::Ellipse) job.values.paintFlags |= 2048u;
                    if (regionCoverage) {
                        job.values.paintFlags |= ContiguousRegion;
                        job.values.patternReserved0 = operation.region->coverageSize.width();
                        job.values.patternReserved1 = operation.region->coverageSize.height();
                        job.values.selectionX = operation.region->coverageOrigin.x();
                        job.values.selectionY = operation.region->coverageOrigin.y();
                        job.values.selectionWidth = operation.region->documentSize.width();
                        job.values.selectionHeight = operation.region->documentSize.height();
                        job.selectionCoverage = std::shared_ptr<NativeBuffer>(regionCoverage, &regionCoverage->pixels);
                        d.device->retainForFrame(regionCoverage);
                    } else if (operation.selectionKind == SelectionKind::Freehand) {
                        if (!operation.selectionMask || operation.selectionMask->isEmpty())
                            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU freehand selection has no coverage")};
                        job.values.paintFlags |= 4096u;
                        const bool alpha = operation.selectionMask->isAlpha();
                        if (alpha) job.values.paintFlags |= 8192u;
                        const QPoint maskTile = coordinate(tile.key());
                        const auto cacheKey = qMakePair(operation.selectionMask.data(), maskTile);
                        auto cached = selectionMaskBuffers.constFind(cacheKey);
                        if (operation.selectionMask->isGpuGenerated()) {
                            const auto mask = d.coverageTile(commands, operation.selectionMask, tile.key());
                            if (!mask) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU-generated selection coverage could not be retained")};
                            job.selectionCoverage = std::shared_ptr<NativeBuffer>(mask->storage, &mask->storage->pixels);
                            d.device->retainForFrame(mask->storage);
                        } else if (cached != selectionMaskBuffers.cend()) {
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
                    std::memcpy(&job.values.canvasOpacityBits, &opacity, sizeof(opacity));
                    if (operation.brush.autoBrush) job.values.paintFlags |= 512u;
                    if (operation.brush.autoBrush && operation.brush.autoBrush->incremental) job.values.paintFlags |= 4194304u;
                    if (operation.brush.additiveMaskingBrush) job.values.paintFlags |= 2097152u;
                    job.output->mayContainHiddenRgb = operation.brush.blendMode != LayerBlendMode::Normal ||
                        operation.figureStyle.fillBlendMode != LayerBlendMode::Normal ||
                        operation.brush.alphaLocked || operation.brush.erase ||
                        !(operation.brush.channelWriteMask & 8) || (opacity > 0 && opacity < 1) ||
                        (operation.selectionMask && operation.selectionMask->isAlpha()) ||
                        operation.figureStyle.pattern || operation.figureStyle.gradient || (job.input && job.input->mayContainHiddenRgb);
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
                auto cropped = d.cropTiles(commands, tiles, (*crop)->rect.normalized(), layer.defaultPixel, document->format,
                                           operationRegions[end + 1], spatialPins);
                if (cropped.code != RenderOutcome::Recorded) return {cropped.code, {}, cropped.diagnostic};
                tiles = std::move(cropped.tiles);
            } else if (const auto filter = std::get_if<FilterSnapshot>(&layer.operations[end])) {
                auto filtered = d.filterTiles(commands, tiles, **filter, data->defaults.value(layer.id),
                                              QRect(QPoint(), document->size), document->format, operationRegions[end + 1], spatialPins);
                if (filtered.code != RenderOutcome::Recorded) return {filtered.code, {}, filtered.diagnostic};
                tiles = std::move(filtered.tiles);
            } else if (const auto affine = std::get_if<AffineSnapshot>(&layer.operations[end])) {
                const auto result = (*affine)->selection ?
                    d.affineSelection(commands, tiles, uniform, layer.defaultPixel, document->format, **affine, operationRegions[end + 1]) :
                    d.affineTiles(commands, tiles, uniform, document->format, *(*affine)->plan, operationRegions[end + 1]);
                if (result.code != RenderOutcome::Recorded) return {result.code, {}, result.diagnostic};
                tiles = result.tiles;
            } else {
                const auto &translation = std::get<TranslationSnapshot>(layer.operations[end]);
                const auto firstPin = spatialPins.size();
                auto moved = translation->selectedPixels ?
                    d.translateSelection(commands, tiles, uniform, layer.defaultPixel, document->format, *translation,
                                         operationRegions[end + 1], spatialPins, data->selectionCoverage) :
                    d.translateTiles(commands, tiles, data->defaults.value(layer.id), translation->offset, document->format,
                                     operationRegions[end + 1], spatialPins);
                if (translation->selectedPixels)
                    for (qsizetype pin = firstPin; pin < spatialPins.size(); ++pin) data->selectionWork.append(spatialPins[pin]);
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
            const auto groupRegion = layerRegions.value(group, paintRegion);
            if (!nodes.value(group)->passThrough || requestedGroups.contains(group)) {
                ProjectedSnapshot previous;
                if (base && base->m_data->groupInputs.contains(group)) {
                    auto cached = std::make_shared<ProjectedVersion::Data>();
                    cached->source = base;
                    cached->region = base->m_data->layerRegions.value(group, base->m_data->region.value_or(groupRegion));
                    cached->tiles = base->m_data->layers.value(group);
                    cached->inputs = base->m_data->groupInputs.value(group);
                    cached->directLayerProjection = base->m_data->directGroups.contains(group);
                    previous = ProjectedSnapshot(new ProjectedVersion(cached));
                }
                const auto projected = composeImpl(commands, version, groupRegion, previous, group);
                if (projected.code != RenderOutcome::Recorded) return {projected.code, {}, projected.diagnostic};
                data->layers[group] = projected.version->m_data->tiles;
                data->sourceExtents[group] = projected.version->m_data->sourceExtent;
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

void TileRenderer::Private::publishLayerPixels(const RenderedSnapshot &version)
{
    if (!version || version->m_data->owner != budget || version->m_data->region) return;
    const auto &data = *version->m_data;
    const auto &document = data.document;
    const auto pending = document->pendingOperation();
    for (const auto &layer : document->layers) {
        if (layer.kind != LayerKind::Paint || !data.selected.contains(layer.id) || data.strokeInputs.contains(layer.id) ||
            document->renderedPixels(layer.id))
            continue;
        if (!pending.isNull() && std::any_of(layer.operations.cbegin(), layer.operations.cend(), [&](const auto &operation) {
                return std::visit([&](const auto &value) { return value->id == pending; }, operation);
            }))
            continue;
        if (layer.operations.isEmpty() && !layer.initialProjection) {
            if (layer.pixels) document->setRenderedPixels(layer.id, layer.pixels);
            if (layer.pixels || data.layers.value(layer.id).isEmpty()) continue;
        }
        auto allocation = data.allocations.value(layer.id);
        if (!allocation) {
            auto computed = paintLayerAllocation(document, layer, PaintGeometryElementLimit);
            if (!computed) continue;
            allocation = std::make_shared<LayerAllocation>(std::move(*computed));
        }
        auto pixels = std::make_shared<RenderedLayerPixels>();
        pixels->owner = budget;
        pixels->tiles = data.layers.value(layer.id);
        pixels->defaultTile = data.defaults.value(layer.id);
        pixels->uniform = data.defaultPixels.value(layer.id, layer.defaultPixel);
        for (const auto &block : allocation->blocks) pixels->allocatedBlocks.append(block);
        pixels->defaultPixel = pixels->uniform;
        for (const auto &tile : std::as_const(pixels->tiles))
            if (tile) pixels->storageBytes.insert(tile->storage.get(), qsizetype(wgpuBufferGetSize(tile->pixels.get())));
        if (pixels->defaultTile) pixels->storageBytes.insert(pixels->defaultTile->storage.get(),
            qsizetype(wgpuBufferGetSize(pixels->defaultTile->pixels.get())));
        document->setRenderedPixels(layer.id, pixels);
    }
}
}
