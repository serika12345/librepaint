/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTilePresentation.h"
#include "KisGpuPresentationResources_p.h"
#include "KisGpuViewTransform_p.h"
#include <QFile>
#include <QMatrix4x4>
#include <QThread>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace LibrePaint::Canvas::Gpu {
namespace {
struct Uniforms { float matrix[16]; float clip[4]; quint32 channels[4]; };
struct Draw { Detail::Buffer uniform; Detail::Bindings bindings; Uniforms values; };
}
struct TilePresentation::Private
{
    Image::Gpu::DevicePtr device;
    qsizetype maximumTiles;
    Detail::Shader shader;
    Detail::Layout layout;
    Detail::Pipeline pipeline;
    quint32 format = 0;
    QString error;
    Private(Image::Gpu::DevicePtr device, qsizetype maximumTiles)
        : device(std::move(device)), maximumTiles(maximumTiles)
    {
        QFile file(QStringLiteral(":/librepaint/canvas/shaders/tile.wgsl"));
        if (!file.open(QIODevice::ReadOnly)) { error = QStringLiteral("The native presentation shader could not be loaded"); return; }
        const QByteArray source = file.readAll();
        WGPUShaderSourceWGSL wgsl{};
        wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgsl.code = {source.constData(), size_t(source.size())};
        WGPUShaderModuleDescriptor descriptor{};
        descriptor.nextInChain = &wgsl.chain;
        shader.reset(wgpuDeviceCreateShaderModule(this->device->nativeDevice(), &descriptor));
        if (!shader) error = this->device->diagnostic();
    }
    bool preparePipeline(quint32 targetFormat)
    {
        if (pipeline && format == targetFormat) return true;
        WGPUBlendState blend{};
        blend.color = {WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};
        blend.alpha = blend.color;
        WGPUColorTargetState color{};
        color.format = WGPUTextureFormat(targetFormat);
        color.blend = &blend;
        color.writeMask = WGPUColorWriteMask_All;
        WGPUFragmentState fragment{};
        fragment.module = shader.get();
        fragment.entryPoint = {"fragmentMain", WGPU_STRLEN};
        fragment.targetCount = 1;
        fragment.targets = &color;
        WGPURenderPipelineDescriptor descriptor{};
        descriptor.vertex.module = shader.get();
        descriptor.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
        descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        descriptor.primitive.frontFace = WGPUFrontFace_CCW;
        descriptor.primitive.cullMode = WGPUCullMode_None;
        descriptor.multisample.count = 1;
        descriptor.multisample.mask = 0xffffffff;
        descriptor.fragment = &fragment;
        Detail::Pipeline next(wgpuDeviceCreateRenderPipeline(device->nativeDevice(), &descriptor));
        if (!next || !device->diagnostic().isEmpty()) return false;
        Detail::Layout nextLayout(wgpuRenderPipelineGetBindGroupLayout(next.get(), 0));
        if (!nextLayout) return false;
        pipeline = std::move(next);
        layout = std::move(nextLayout);
        format = targetFormat;
        return true;
    }
};
TilePresentation::TilePresentation(Image::Gpu::DevicePtr device, qsizetype maximumTiles)
{
    if (!device || device->thread() != QThread::currentThread() || maximumTiles <= 0)
        throw std::invalid_argument("invalid GPU presentation owner or tile limit");
    m_d = std::make_unique<Private>(std::move(device), maximumTiles);
}
TilePresentation::~TilePresentation() = default;
PresentationOutcome TilePresentation::record(WGPUCommandEncoder commands,
    const Image::Gpu::ProjectedSnapshot &projection, const PresentationTarget &target,
    const QTransform &transform, QColor clearColor, DisplayChannels channels)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || !d.device->ownsCommands(commands) ||
        target.device != d.device || !target.view || target.size.isEmpty() || !projection ||
        projection->device() != d.device || !Detail::validDocumentTransform(transform) || !clearColor.isValid() || channels.rgbaMask > 15)
        return {false, QStringLiteral("Presentation requires an owning GPU frame, an invertible affine transform and valid RGBA channel bits"), 0};
    if (!d.error.isEmpty() || d.device->isLost()) return {false, d.error.isEmpty() ? QStringLiteral("GPU device lost") : d.error, 0};
    const auto tiles = projection->tiles();
    const QRectF viewport(QPointF(0, 0), target.size);
    const QRectF region(projection->region());
    QList<Image::Gpu::TileView> visible;
    constexpr int size = Image::Gpu::TileRenderer::TileSize;
    for (const auto &tile : tiles) {
        if (!tile.pixels) return {false, QStringLiteral("Projection has no native pixels"), 0};
        const QRectF tileRect(tile.coordinate * size, QSize(size, size));
        if (transform.mapRect(tileRect.intersected(region)).intersects(viewport)) visible.append(tile);
    }
    if (visible.size() > d.maximumTiles) return {false, QStringLiteral("Visible GPU tile limit exceeded"), 0};
    if (!visible.isEmpty() && !d.preparePipeline(target.format))
        return {false, QStringLiteral("GPU presentation pipeline could not be created: %1").arg(d.device->diagnostic()), 0};
    QMatrix4x4 view;
    view.ortho(0, target.size.width(), target.size.height(), 0, -1, 1);
    view = view * QMatrix4x4(transform);
    std::vector<Draw> draws;
    draws.reserve(visible.size());
    for (const auto &tile : visible) {
        Draw draw;
        WGPUBufferDescriptor buffer{};
        buffer.size = sizeof(Uniforms);
        buffer.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        draw.uniform.reset(wgpuDeviceCreateBuffer(d.device->nativeDevice(), &buffer));
        auto matrix = view;
        const QPoint origin = tile.coordinate * size;
        matrix.translate(origin.x(), origin.y());
        matrix.scale(size, size);
        std::copy(matrix.constData(), matrix.constData() + 16, draw.values.matrix);
        draw.values.clip[0] = float(region.left() - origin.x()) / size;
        draw.values.clip[1] = float(region.top() - origin.y()) / size;
        draw.values.clip[2] = float(region.left() + region.width() - origin.x()) / size;
        draw.values.clip[3] = float(region.top() + region.height() - origin.y()) / size;
        draw.values.channels[0] = channels.rgbaMask;
        draw.values.channels[1] = channels.showSingleChannelAsColor;
        draw.values.channels[2] = draw.values.channels[3] = 0;
        if (!draw.uniform) return {false, QStringLiteral("GPU presentation uniform allocation failed"), 0};
        WGPUBindGroupEntry entries[2]{};
        entries[0].binding = 0;
        entries[0].buffer = draw.uniform.get();
        entries[0].size = sizeof(Uniforms);
        entries[1].binding = 1;
        entries[1].buffer = tile.pixels;
        entries[1].size = wgpuBufferGetSize(tile.pixels);
        WGPUBindGroupDescriptor bindings{};
        bindings.layout = d.layout.get();
        bindings.entryCount = 2;
        bindings.entries = entries;
        draw.bindings.reset(wgpuDeviceCreateBindGroup(d.device->nativeDevice(), &bindings));
        if (!draw.bindings || !d.device->diagnostic().isEmpty()) return {false, QStringLiteral("GPU presentation tile binding failed"), 0};
        draws.push_back(std::move(draw));
    }
    for (const auto &draw : draws) wgpuQueueWriteBuffer(d.device->queue(), draw.uniform.get(), 0, &draw.values, sizeof(Uniforms));
    WGPURenderPassColorAttachment attachment{};
    attachment.view = target.view;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    attachment.clearValue = {clearColor.redF() * clearColor.alphaF(), clearColor.greenF() * clearColor.alphaF(),
        clearColor.blueF() * clearColor.alphaF(), clearColor.alphaF()};
    WGPURenderPassDescriptor passDescriptor{};
    passDescriptor.colorAttachmentCount = 1;
    passDescriptor.colorAttachments = &attachment;
    auto pass = wgpuCommandEncoderBeginRenderPass(commands, &passDescriptor);
    if (!pass) return {false, QStringLiteral("GPU presentation pass could not begin"), 0};
    if (!draws.empty()) {
        wgpuRenderPassEncoderSetPipeline(pass, d.pipeline.get());
        wgpuRenderPassEncoderSetViewport(pass, 0, 0, target.size.width(), target.size.height(), 0, 1);
        for (const auto &draw : draws) {
            wgpuRenderPassEncoderSetBindGroup(pass, 0, draw.bindings.get(), 0, nullptr);
            wgpuRenderPassEncoderDraw(pass, 6, 1, 0, 0);
        }
    }
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
    return {true, {}, qsizetype(draws.size())};
}
}
