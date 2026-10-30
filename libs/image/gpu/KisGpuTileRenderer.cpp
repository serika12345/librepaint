/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRenderer.h"
#include "KisGpuPaintJob_p.h"
#include "KisGpuNonlinearRgba8_p.h"
#include "KisGpuRgba8AlphaBlend_p.h"
#include "KisGpuRgba16Compositing_p.h"

#include <QFile>
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

TileRenderer::Private::Private(DevicePtr device, qsizetype byteBudget)
    : device(std::move(device)), budget(std::make_shared<Budget>(Budget{this->device, byteBudget})),
      unusedCoverage(createBuffer(this->device->nativeDevice(), sizeof(quint32), WGPUBufferUsage_Storage)),
      unusedSelection(createBuffer(this->device->nativeDevice(), SelectionMask::AlphaTileBytes,
                                   WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)),
      unusedPattern(createBuffer(this->device->nativeDevice(), sizeof(quint32),
                                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)),
      opaqueMask(createBuffer(this->device->nativeDevice(), SelectionMask::AlphaTileBytes,
                              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst))
{
    if (!prepareNonlinearRgba8(false) || !prepareGreaterRgba8(false)) return;
    if (opaqueMask) {
        const QByteArray opaque(SelectionMask::AlphaTileBytes, char(255));
        wgpuQueueWriteBuffer(this->device->queue(), opaqueMask.get(), 0,
                             opaque.constData(), opaque.size());
    }
    pipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/paint.wgsl"));
    if (!unusedCoverage || !unusedSelection || !unusedPattern || !opaqueMask || !pipeline) return;
    layout.reset(wgpuComputePipelineGetBindGroupLayout(pipeline.get(), 0));
    if (!layout) error = QStringLiteral("GPU paint bindings could not be created");
    if (error.isEmpty()) initializeProjection();
}

TileRenderer::Private::~Private()
{ budget->used -= nonlinearBytes + greaterBytes + cosineBytes + powerBytes; }

bool TileRenderer::Private::prepareRgba16Cosine()
{
    if (cosineView) return true;
    if (!ensureTileCapacity(Rgba16CosineBytes / RgbaTile::byteCount(PixelFormat::Rgba16), PixelFormat::Rgba16)) return false;
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size = {256, 256, 2};
    descriptor.format = WGPUTextureFormat_RGBA8Uint;
    descriptor.mipLevelCount = descriptor.sampleCount = 1;
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    NativeTexture texture(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
    WGPUTextureViewDescriptor viewDescriptor{};
    viewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    viewDescriptor.format = WGPUTextureFormat_RGBA8Uint;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.arrayLayerCount = 2;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    NativeTextureView view(texture ? wgpuTextureCreateView(texture.get(), &viewDescriptor) : nullptr);
    if (!texture || !view || !device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU RGBA16 cosine texture allocation failed");
        return false;
    }
    const auto &bytes = rgba16CosineValues();
    WGPUTexelCopyTextureInfo destination{};
    destination.texture = texture.get();
    destination.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferLayout layout{};
    layout.bytesPerRow = 256 * 4;
    layout.rowsPerImage = 256;
    wgpuQueueWriteTexture(device->queue(), &destination, bytes.constData(), size_t(bytes.size()), &layout, &descriptor.size);
    if (!device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU RGBA16 cosine resource upload failed");
        return false;
    }
    cosineBytes = Rgba16CosineBytes;
    budget->used += cosineBytes;
    cosineTexture = std::move(texture);
    cosineView = std::move(view);
    return true;
}

bool TileRenderer::Private::prepareRgba16Power()
{
    if (powerView) return true;
    if (!ensureTileCapacity(Rgba16PowerBytes / RgbaTile::byteCount(PixelFormat::Rgba16), PixelFormat::Rgba16)) return false;
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size = {256, 256, 8};
    descriptor.format = WGPUTextureFormat_RGBA8Uint;
    descriptor.mipLevelCount = descriptor.sampleCount = 1;
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    NativeTexture texture(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
    WGPUTextureViewDescriptor viewDescriptor{};
    viewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    viewDescriptor.format = WGPUTextureFormat_RGBA8Uint;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.arrayLayerCount = 8;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    NativeTextureView view(texture ? wgpuTextureCreateView(texture.get(), &viewDescriptor) : nullptr);
    if (!texture || !view || !device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU RGBA16 power texture allocation failed");
        return false;
    }
    const auto &bytes = rgba16PowerValues();
    WGPUTexelCopyTextureInfo destination{};
    destination.texture = texture.get();
    destination.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferLayout layout{};
    layout.bytesPerRow = 256 * 4;
    layout.rowsPerImage = 256;
    wgpuQueueWriteTexture(device->queue(), &destination, bytes.constData(), size_t(bytes.size()), &layout, &descriptor.size);
    if (!device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU RGBA16 power resource upload failed");
        return false;
    }
    powerBytes = Rgba16PowerBytes;
    budget->used += powerBytes;
    powerTexture = std::move(texture);
    powerView = std::move(view);
    return true;
}

bool TileRenderer::Private::prepareNonlinearRgba8(bool full)
{
    if ((full && nonlinearBytes) || (!full && nonlinearView)) return true;
    if (full && !ensureTileCapacity(NonlinearRgba8Bytes / TileRenderer::TileBytes, PixelFormat::Rgba8)) return false;
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size = full ? WGPUExtent3D{256, 256, NonlinearRgba8Layers} : WGPUExtent3D{1, 1, 1};
    descriptor.format = WGPUTextureFormat_RGBA8Uint;
    descriptor.mipLevelCount = descriptor.sampleCount = 1;
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    NativeTexture texture(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
    WGPUTextureViewDescriptor viewDescriptor{};
    viewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    viewDescriptor.format = WGPUTextureFormat_RGBA8Uint;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.arrayLayerCount = descriptor.size.depthOrArrayLayers;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    NativeTextureView view(texture ? wgpuTextureCreateView(texture.get(), &viewDescriptor) : nullptr);
    if (!texture || !view || !device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU nonlinear RGBA8 texture allocation failed");
        return false;
    }
    if (full) {
        const auto &bytes = nonlinearRgba8Values();
        WGPUTexelCopyTextureInfo destination{};
        destination.texture = texture.get();
        destination.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferLayout layout{};
        layout.bytesPerRow = 256 * 4;
        layout.rowsPerImage = 256;
        wgpuQueueWriteTexture(device->queue(), &destination, bytes.constData(), size_t(bytes.size()), &layout, &descriptor.size);
        if (!device->diagnostic().isEmpty()) {
            error = QStringLiteral("GPU nonlinear RGBA8 resource upload failed");
            return false;
        }
        nonlinearBytes = NonlinearRgba8Bytes;
        budget->used += nonlinearBytes;
    }
    nonlinearTexture = std::move(texture);
    nonlinearView = std::move(view);
    return true;
}

bool TileRenderer::Private::prepareGreaterRgba8(bool full)
{
    if ((full && greaterBytes) || (!full && greaterView)) return true;
    if (full && !ensureTileCapacity(GreaterRgba8Bytes / TileRenderer::TileBytes, PixelFormat::Rgba8)) return false;
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size = full ? WGPUExtent3D{256, 256, 1} : WGPUExtent3D{1, 1, 1};
    descriptor.format = WGPUTextureFormat_RGBA8Uint;
    descriptor.mipLevelCount = descriptor.sampleCount = 1;
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    NativeTexture texture(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
    WGPUTextureViewDescriptor viewDescriptor{};
    viewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    viewDescriptor.format = WGPUTextureFormat_RGBA8Uint;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.arrayLayerCount = descriptor.size.depthOrArrayLayers;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    NativeTextureView view(texture ? wgpuTextureCreateView(texture.get(), &viewDescriptor) : nullptr);
    if (!texture || !view || !device->diagnostic().isEmpty()) {
        error = QStringLiteral("GPU Greater RGBA8 texture allocation failed");
        return false;
    }
    if (full) {
        const auto &bytes = greaterRgba8Values();
        WGPUTexelCopyTextureInfo destination{};
        destination.texture = texture.get();
        destination.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferLayout layout{};
        layout.bytesPerRow = 256 * 4;
        layout.rowsPerImage = 256;
        wgpuQueueWriteTexture(device->queue(), &destination, bytes.constData(), size_t(bytes.size()), &layout, &descriptor.size);
        if (!device->diagnostic().isEmpty()) {
            error = QStringLiteral("GPU Greater RGBA8 resource upload failed");
            return false;
        }
        greaterBytes = GreaterRgba8Bytes;
        budget->used += greaterBytes;
    }
    greaterTexture = std::move(texture);
    greaterView = std::move(view);
    return true;
}

NativeComputePipeline TileRenderer::Private::createPipeline(const QString &path, const char *entryPoint,
    quint32 firstBlend, quint32 secondBlend, WGPUBindGroupLayout sharedLayout, bool separableBlendEnabled, quint32 coupledPixelFormat, quint32 nativeLabKind)
{
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl")), shader(path);
    if (!shared.open(QIODevice::ReadOnly) || !shader.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("GPU compute resources could not be loaded");
        return {};
    }
    QByteArray source = shared.readAll() + '\n';
    const auto coupled = [](quint32 mode) { return mode >= 88 && mode <= 126 ? mode : 0u; };
    source += "const COLOR_BLEND_MODE_A: u32 = " + QByteArray::number(coupled(firstBlend)) + "u;\n";
    source += "const COLOR_BLEND_MODE_B: u32 = " + QByteArray::number(coupled(secondBlend)) + "u;\n";
    source += "const WORD_BLEND_MODE_A: u32 = " + QByteArray::number(coupledPixelFormat == 2u ? firstBlend : 0u) + "u;\n";
    source += "const WORD_BLEND_MODE_B: u32 = " + QByteArray::number(coupledPixelFormat == 2u ? secondBlend : 0u) + "u;\n";
    source += QByteArray("const SEPARABLE_BLEND_ENABLED: bool = ") + (separableBlendEnabled ? "true;\n" : "false;\n");
    source += "const COUPLED_PIXEL_FORMAT: u32 = " + QByteArray::number(coupledPixelFormat) + "u;\n";
    source += "const NATIVE_LAB_KIND: u32 = " + QByteArray::number(nativeLabKind) + "u;\n";
    const bool painting = path.endsWith(QLatin1String("/paint.wgsl"));
    if (painting || path.endsWith(QLatin1String("/over.wgsl")) || path.endsWith(QLatin1String("/selection-move.wgsl")) || path.endsWith(QLatin1String("/affine.wgsl")) || path.endsWith(QLatin1String("/region.wgsl"))) {
        QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
        QFile separable(QStringLiteral(":/librepaint/gpu/shaders/separable.wgsl"));
        QFile doubles(QStringLiteral(":/librepaint/gpu/shaders/binary64.wgsl"));
        QFile color(QStringLiteral(":/librepaint/gpu/shaders/color-blend.wgsl"));
        QFile normal(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba8.wgsl"));
        QFile native(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba16.wgsl"));
        QFile copy(QStringLiteral(":/librepaint/gpu/shaders/copy.wgsl"));
        QFile separable16(QStringLiteral(":/librepaint/gpu/shaders/separable-rgba16.wgsl"));
        if (!arithmetic.open(QIODevice::ReadOnly) || !separable.open(QIODevice::ReadOnly) ||
            !normal.open(QIODevice::ReadOnly) || !native.open(QIODevice::ReadOnly) || !separable16.open(QIODevice::ReadOnly) ||
            !copy.open(QIODevice::ReadOnly) || !doubles.open(QIODevice::ReadOnly) || !color.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU normal-composition arithmetic could not be loaded");
            return {};
        }
        source += arithmetic.readAll() + '\n';
        source += doubles.readAll() + '\n';
        source += color.readAll() + '\n';
        source += separable.readAll() + '\n';
        source += normal.readAll() + '\n';
        source += native.readAll() + '\n';
        source += separable16.readAll() + '\n';
        source += copy.readAll() + '\n';
    }
    if (painting) {
        QFile gaussian(QStringLiteral(":/librepaint/gpu/shaders/gaussian.wgsl"));
        if (!gaussian.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU native brush arithmetic could not be loaded");
            return {};
        }
        source += gaussian.readAll() + '\n';
    }
    if (path.endsWith(QLatin1String("/region.wgsl"))) {
        QFile lab(QStringLiteral(":/librepaint/gpu/shaders/native-lab.wgsl"));
        if (!lab.open(QIODevice::ReadOnly)) { error = QStringLiteral("Native Lab arithmetic could not be loaded"); return {}; }
        source += lab.readAll() + '\n';
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
    descriptor.compute.module = module.get();
    descriptor.compute.entryPoint = {entryPoint, WGPU_STRLEN};
    NativeComputePipeline result(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &descriptor));
    if (!result || !device->diagnostic().isEmpty()) {
        error = device->diagnostic().isEmpty() ? QStringLiteral("GPU compute pipeline creation failed") : device->diagnostic();
        return {};
    }
    return result;
}

WGPUComputePipeline TileRenderer::Private::blendPipeline(bool paint, bool paired, PixelFormat format, quint32 firstBlend, quint32 secondBlend)
{
    const auto coupled = [](quint32 mode) { return mode >= 88 && mode <= 126 ? mode : 0u; };
    const bool separableBlendEnabled = format == PixelFormat::Rgba16 &&
        ((firstBlend && !coupled(firstBlend)) || (secondBlend && !coupled(secondBlend)));
    if (format == PixelFormat::Rgba8) {
        firstBlend = coupled(firstBlend);
        secondBlend = coupled(secondBlend);
    }
    if (firstBlend == secondBlend) secondBlend = 0;
    if (!firstBlend && !secondBlend)
        return paint ? pipeline.get() : paired ? pairedProjectionPipeline.get() : projectionPipeline.get();
    if (secondBlend < firstBlend) std::swap(firstBlend, secondBlend);
    const auto key = firstBlend | (secondBlend << 8) | (quint32(paint) << 16) | (quint32(paired) << 17) | (quint32(separableBlendEnabled) << 18) | (quint32(format == PixelFormat::Rgba16) << 19);
    const auto found = blendPipelines.find(key);
    if (found != blendPipelines.end()) return found->second.get();
    auto compiled = createPipeline(paint ? QStringLiteral(":/librepaint/gpu/shaders/paint.wgsl") :
        QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"), paired ? "mainPair" : "main", firstBlend, secondBlend,
        paint ? layout.get() : paired ? pairedProjectionLayout.get() : projectionLayout.get(), separableBlendEnabled, format == PixelFormat::Rgba16 ? 2u : 1u);
    if (!compiled) return nullptr;
    if (blendPipelines.size() >= 8) blendPipelines.erase(blendPipelines.begin());
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
    m_d = std::make_unique<Private>(std::move(device), byteBudget);
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

RenderOutcome TileRenderer::recordImpl(WGPUCommandEncoder commands, const Snapshot &document,
                                      const RenderedSnapshot &base, std::optional<QRect> region,
                                      const QList<LayerId> &layers)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || !d.device->ownsCommands(commands) || !document ||
        (base && (base->m_data->owner != d.budget || base->document()->documentId() != document->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU versions require their owning device and document")};
    }
    if (d.regionDepth++ == 0) d.regionCache.clear();
    const auto regionScope = qScopeGuard([&] { if (--d.regionDepth == 0) d.regionCache.clear(); });
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
    if (base && base->document() == document && base->m_data->region == region && base->m_data->selected == selected &&
        ((region && region->isEmpty()) || std::all_of(requestedGroups.cbegin(), requestedGroups.cend(),
            [&](LayerId group) { return base->m_data->groupInputs.contains(group); })))
        return {RenderOutcome::Recorded, base, {}};
    const bool cosine = document->format == PixelFormat::Rgba16 && std::any_of(document->layers.cbegin(), document->layers.cend(), [&](const auto &layer) {
        return selected.contains(layer.id) && std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &operation) {
            const auto paint = std::get_if<PaintSnapshot>(&operation);
            return paint && !(*paint)->brush.erase && qRound(float((*paint)->brush.opacity) * 65535) > 0 &&
                (((*paint)->figureStyle.outline && usesRgba16Cosine((*paint)->brush.blendMode)) ||
                 (hasFigureFill((*paint)->figureStyle) && usesRgba16Cosine((*paint)->figureStyle.fillBlendMode)));
        });
    });
    if (cosine && !d.prepareRgba16Cosine())
        return {d.error.isEmpty() ? RenderOutcome::CapacityExceeded : RenderOutcome::DeviceFailure, {},
                d.error.isEmpty() ? QStringLiteral("GPU RGBA16 cosine resource budget exhausted") : d.error};
    const bool powerValues = document->format == PixelFormat::Rgba16 && std::any_of(document->layers.cbegin(), document->layers.cend(), [&](const auto &layer) {
        return selected.contains(layer.id) && std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &operation) {
            const auto paint = std::get_if<PaintSnapshot>(&operation);
            return paint && !(*paint)->brush.erase && qRound(float((*paint)->brush.opacity) * 65535) > 0 &&
                (((*paint)->figureStyle.outline && usesRgba16PowerValues((*paint)->brush.blendMode)) ||
                 (hasFigureFill((*paint)->figureStyle) && usesRgba16PowerValues((*paint)->figureStyle.fillBlendMode)));
        });
    });
    if (powerValues && !d.prepareRgba16Power())
        return {d.error.isEmpty() ? RenderOutcome::CapacityExceeded : RenderOutcome::DeviceFailure, {},
                d.error.isEmpty() ? QStringLiteral("GPU RGBA16 power resource budget exhausted") : d.error};
    const bool nonlinear = document->format == PixelFormat::Rgba8 && std::any_of(document->layers.cbegin(), document->layers.cend(), [&](const auto &layer) {
        if (!selected.contains(layer.id)) return false;
        return std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &operation) {
            const auto paint = std::get_if<PaintSnapshot>(&operation);
            return paint && !(*paint)->brush.erase && qRound(float((*paint)->brush.opacity) * 255) > 0 &&
                (((*paint)->figureStyle.outline && usesNonlinearRgba8((*paint)->brush.blendMode)) ||
                 (hasFigureFill((*paint)->figureStyle) && usesNonlinearRgba8((*paint)->figureStyle.fillBlendMode)));
        });
    });
    if (nonlinear && !d.prepareNonlinearRgba8(true))
        return {d.error.isEmpty() ? RenderOutcome::CapacityExceeded : RenderOutcome::DeviceFailure, {},
                d.error.isEmpty() ? QStringLiteral("GPU nonlinear RGBA8 resource budget exhausted") : d.error};
    const bool greater = document->format == PixelFormat::Rgba8 && std::any_of(document->layers.cbegin(), document->layers.cend(), [&](const auto &layer) {
        return selected.contains(layer.id) && std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &operation) {
            const auto paint = std::get_if<PaintSnapshot>(&operation);
            return paint && !(*paint)->brush.erase && ((*paint)->brush.blendMode == LayerBlendMode::Greater ||
                (hasFigureFill((*paint)->figureStyle) && (*paint)->figureStyle.fillBlendMode == LayerBlendMode::Greater));
        });
    });
    if (greater && !d.prepareGreaterRgba8(true))
        return {d.error.isEmpty() ? RenderOutcome::CapacityExceeded : RenderOutcome::DeviceFailure, {},
                d.error.isEmpty() ? QStringLiteral("GPU Greater alpha resource budget exhausted") : d.error};
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
        if (!preparePaintJobs(*d.device, d.layout.get(), jobs, recordedJobs, empty,
                              d.unusedCoverage.get(), d.unusedSelection.get(), d.unusedPattern.get(),
                              document->format == PixelFormat::Rgba16 && d.cosineView ? d.cosineView.get() : d.nonlinearView.get(),
                              document->format == PixelFormat::Rgba16 && d.powerView ? d.powerView.get() : d.greaterView.get()))
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
                const auto program = d.blendPipeline(true, false, document->format, jobs[i].values.strokeBlendMode, jobs[i].values.fillBlendMode);
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
        if (layer.passThrough && !requestedGroups.contains(layer.id)) continue;
        const bool clipsPainting = std::any_of(layer.operations.cbegin(), layer.operations.cend(), [](const auto &value) {
            const auto paint = std::get_if<PaintSnapshot>(&value);
            return paint && ((*paint)->brush.alphaLocked || !((*paint)->brush.channelWriteMask & 8u));
        });
        std::shared_ptr<const LayerAllocation> allocation;
        if ((needsSourceExtents || clipsPainting) && layer.kind == LayerKind::Paint) {
            if (base && base->document()->size == document->size && base->m_data->allocations.contains(layer.id)) {
                const auto &previous = base->document()->layers;
                const auto old = std::find_if(previous.cbegin(), previous.cend(), [&](const auto &value) { return value.id == layer.id; });
                if (old != previous.cend() && old->position == layer.position && old->defaultPixel == layer.defaultPixel &&
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
        qsizetype prefix = 0;
        qsizetype reusedPrefix = 0;
        QSet<quint64> reusedCoordinates;
        PaintSnapshot appendedOperation;
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
                for (qsizetype i = 0; i < shared; ++i)
                    sharedSpatialOperation = sharedSpatialOperation || std::holds_alternative<TranslationSnapshot>(layer.operations[i]) ||
                        std::holds_alternative<AffineSnapshot>(layer.operations[i]) ||
                        std::holds_alternative<CropSnapshot>(layer.operations[i]);
                if (matches && (!sharedSpatialOperation || !base->m_data->region || base->m_data->region->contains(sourceRegion))) {
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
                    if (!base->m_data->region || base->m_data->region->contains(sourceRegion)) prefix = reusedPrefix;
                }
                break;
            }
        }
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
                    (!base->m_data->region || base->m_data->region->contains(paintRegion));
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
                            old->operations == other.operations;
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
                    auto cached = d.regionCache.constFind(operation.region.data());
                    if (cached == d.regionCache.cend() || cached->source.toStrongRef() != operation.region) {
                        const auto prepared = prepareRegion(commands, operation.region, base);
                        if (prepared.code != RenderOutcome::Recorded) return prepared;
                        cached = d.regionCache.constFind(operation.region.data());
                    }
                    if (cached == d.regionCache.cend())
                        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU contiguous coverage was not generated")};
                    regionCoverage = cached->coverage.pixels;
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
                                      (operation.figureStyle.pattern ? 8u : 0u)};
                    job.values.colorHigh = color[1];
                    job.values.fillColorHigh = fill[1];
                    job.values.pixelFormat = quint32(document->format);
                    if (operation.clearsSelectedPixels) job.values.paintFlags |= ClearSelectedPixels;
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
                        std::memcpy(job.values.patternInverse.data(), pattern.inverse.data(), sizeof(pattern.inverse));
                        job.values.patternFormat = quint32(pattern.format);
                        job.values.patternSampled = pattern.sampled;
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
                        job.values.patternReserved0 = operation.region->source->size.width();
                        job.values.patternReserved1 = operation.region->source->size.height();
                        job.selectionCoverage = std::shared_ptr<NativeBuffer>(regionCoverage, &regionCoverage->pixels);
                        d.device->retainForFrame(regionCoverage);
                    } else if (operation.selectionKind == SelectionKind::Freehand) {
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
                    std::memcpy(&job.values.canvasOpacityBits, &opacity, sizeof(opacity));
                    if (operation.brush.autoBrush) job.values.paintFlags |= 512u;
                    job.output->mayContainHiddenRgb = operation.brush.blendMode != LayerBlendMode::Normal ||
                        operation.figureStyle.fillBlendMode != LayerBlendMode::Normal ||
                        operation.brush.alphaLocked || operation.brush.erase ||
                        !(operation.brush.channelWriteMask & 8) || (opacity > 0 && opacity < 1) ||
                        (operation.selectionMask && operation.selectionMask->isAlpha()) ||
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
                auto cropped = d.cropTiles(commands, tiles, (*crop)->rect.normalized(), layer.defaultPixel, document->format,
                                           operationRegions[end + 1], spatialPins);
                if (cropped.code != RenderOutcome::Recorded) return {cropped.code, {}, cropped.diagnostic};
                tiles = std::move(cropped.tiles);
            } else if (const auto filter = std::get_if<FilterSnapshot>(&layer.operations[end])) {
                auto filtered = d.filterTiles(commands, tiles, **filter, data->defaults.value(layer.id),
                                              document->size, operationRegions[end + 1], spatialPins);
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
}
