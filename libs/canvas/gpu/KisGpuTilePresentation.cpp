/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTilePresentation.h"
#include "KisGpuCropPreview_p.h"
#include "KisGpuPresentationResources_p.h"
#include "KisGpuViewTransform_p.h"
#include <QFile>
#include <QMatrix4x4>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace LibrePaint::Canvas::Gpu {
namespace {
struct Uniforms {
    float matrix[16]; float clip[4]; quint32 channels[4]; quint32 background[4];
    quint32 checkerFirst[4]; quint32 checkerSecond[4];
    float checkerX[4]; float checkerY[4]; quint32 checker[4];
};
static_assert(sizeof(Uniforms) == 192);
struct Draw { Detail::Buffer uniform; Detail::Bindings bindings; Uniforms values{}; };
struct OutlineVertex { float x, y; };
struct OutlineUniforms { float viewport[4]; float color[4]; };
constexpr qsizetype maximumOutlineVertices = 65536;

bool outlineGeometry(const ToolOutline &outline, std::vector<OutlineVertex> &vertices)
{
    if (outline.contours.isEmpty()) return true;
    constexpr qreal limit = std::numeric_limits<float>::max() / 16.0;
    if (!outline.color.isValid() || !std::isfinite(outline.thickness) ||
        outline.thickness <= 0 || outline.thickness > limit || outline.contours.size() > maximumOutlineVertices)
        return false;
    qsizetype points = 0;
    const qreal half = outline.thickness * 0.5;
    const auto normal = [](QPointF direction) {
        const auto length = std::hypot(direction.x(), direction.y());
        return length ? QPointF(-direction.y(), direction.x()) / length : QPointF();
    };
    const auto joinedOffset = [&](QPointF adjacent, QPointF current) {
        const QPointF sum = normal(adjacent) + current;
        const auto length = std::hypot(sum.x(), sum.y());
        if (!length) return current * half;
        const auto miter = sum / length;
        const auto dot = QPointF::dotProduct(miter, current);
        const auto distance = dot ? half / dot : 0;
        return dot && std::abs(distance) < 5 * outline.thickness ? miter * distance : current * half;
    };
    for (const auto &contour : outline.contours) {
        const auto pointCost = std::max<qsizetype>(1, contour.size());
        if (pointCost > maximumOutlineVertices - points) return false;
        points += pointCost;
        QPolygonF clean;
        clean.reserve(contour.size());
        for (const auto &point : contour) {
            if (!std::isfinite(point.x()) || !std::isfinite(point.y()) ||
                std::abs(point.x()) > limit || std::abs(point.y()) > limit) return false;
            if (clean.isEmpty() || point != clean.last()) clean.append(point);
        }
        if (clean.size() < 2) continue;
        if (clean.size() - 1 > (maximumOutlineVertices - qsizetype(vertices.size())) / 6) return false;
        const bool closed = clean.isClosed();
        for (qsizetype i = 1; i < clean.size(); ++i) {
            const auto first = clean[i - 1], second = clean[i];
            const auto perpendicular = normal(second - first);
            auto firstOffset = perpendicular * half, secondOffset = firstOffset;
            if (closed || i > 1) {
                const auto previous = i > 1 ? clean[i - 2] : clean[clean.size() - 2];
                firstOffset = joinedOffset(first - previous, perpendicular);
            }
            if (closed || i + 1 < clean.size()) {
                const auto next = i + 1 < clean.size() ? clean[i + 1] : clean[1];
                secondOffset = joinedOffset(next - second, perpendicular);
            }
            const QPointF a = first - firstOffset, b = first + firstOffset;
            const QPointF c = second - secondOffset, d = second + secondOffset;
            for (const auto &point : {a, c, b, d, b, c}) vertices.push_back({float(point.x()), float(point.y())});
        }
    }
    return true;
}
}
struct TilePresentation::Private
{
    Image::Gpu::DevicePtr device;
    qsizetype maximumTiles;
    Detail::Shader shader;
    Detail::Layout layout;
    Detail::Pipeline pipeline;
    quint32 format = 0;
    Detail::Layout backgroundLayout;
    Detail::Pipeline backgroundPipeline;
    quint32 backgroundFormat = 0;
    Detail::Shader outlineShader;
    Detail::Pipeline outlinePipeline;
    Detail::Layout outlineLayout;
    Detail::Buffer outlineVertices;
    Detail::Buffer outlineUniform;
    Detail::Bindings outlineBindings;
    qsizetype outlineCapacity = 0;
    quint32 outlineFormat = 0;
    QString error;
    Detail::CropPreviewRenderer cropRenderer;
    Private(Image::Gpu::DevicePtr device, qsizetype maximumTiles)
        : device(std::move(device)), maximumTiles(maximumTiles), cropRenderer(this->device)
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
    bool preparePipeline(quint32 targetFormat, bool forBackground = false)
    {
        auto &selectedPipeline = forBackground ? backgroundPipeline : pipeline;
        auto &selectedLayout = forBackground ? backgroundLayout : layout;
        auto &selectedFormat = forBackground ? backgroundFormat : format;
        if (selectedPipeline && selectedFormat == targetFormat) return true;
        WGPUColorTargetState color{};
        color.format = WGPUTextureFormat(targetFormat);
        color.writeMask = WGPUColorWriteMask_All;
        WGPUFragmentState fragment{};
        fragment.module = shader.get();
        fragment.entryPoint = {forBackground ? "backgroundMain" : "fragmentMain", WGPU_STRLEN};
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
        selectedPipeline = std::move(next);
        selectedLayout = std::move(nextLayout);
        selectedFormat = targetFormat;
        return true;
    }

    bool prepareOutline(quint32 targetFormat, qsizetype vertices)
    {
        if (!outlineShader) {
            QFile file(QStringLiteral(":/librepaint/canvas/shaders/tool_outline.wgsl"));
            if (!file.open(QIODevice::ReadOnly)) return false;
            const auto source = file.readAll();
            WGPUShaderSourceWGSL wgsl{};
            wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
            wgsl.code = {source.constData(), size_t(source.size())};
            WGPUShaderModuleDescriptor descriptor{};
            descriptor.nextInChain = &wgsl.chain;
            outlineShader.reset(wgpuDeviceCreateShaderModule(device->nativeDevice(), &descriptor));
            if (!outlineShader) return false;
        }
        if (!outlinePipeline || outlineFormat != targetFormat) {
            WGPUBlendState blend{};
            blend.color = {WGPUBlendOperation_Subtract, WGPUBlendFactor_DstAlpha, WGPUBlendFactor_Src};
            blend.alpha = {WGPUBlendOperation_Add, WGPUBlendFactor_Zero, WGPUBlendFactor_One};
            WGPUColorTargetState color{};
            color.format = WGPUTextureFormat(targetFormat);
            color.blend = &blend;
            color.writeMask = WGPUColorWriteMask_All;
            WGPUFragmentState fragment{};
            fragment.module = outlineShader.get();
            fragment.entryPoint = {"fragmentMain", WGPU_STRLEN};
            fragment.targetCount = 1;
            fragment.targets = &color;
            WGPUVertexAttribute attribute{};
            attribute.format = WGPUVertexFormat_Float32x2;
            WGPUVertexBufferLayout vertex{};
            vertex.arrayStride = sizeof(OutlineVertex);
            vertex.stepMode = WGPUVertexStepMode_Vertex;
            vertex.attributeCount = 1;
            vertex.attributes = &attribute;
            WGPURenderPipelineDescriptor descriptor{};
            descriptor.vertex.module = outlineShader.get();
            descriptor.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
            descriptor.vertex.bufferCount = 1;
            descriptor.vertex.buffers = &vertex;
            descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
            descriptor.primitive.frontFace = WGPUFrontFace_CCW;
            descriptor.primitive.cullMode = WGPUCullMode_None;
            descriptor.multisample.count = 1;
            descriptor.multisample.mask = 0xffffffff;
            descriptor.fragment = &fragment;
            Detail::Pipeline next(wgpuDeviceCreateRenderPipeline(device->nativeDevice(), &descriptor));
            if (!next || !device->diagnostic().isEmpty()) return false;
            Detail::Layout layout(wgpuRenderPipelineGetBindGroupLayout(next.get(), 0));
            if (!layout) return false;
            if (!outlineUniform) {
                WGPUBufferDescriptor buffer{};
                buffer.size = sizeof(OutlineUniforms);
                buffer.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
                outlineUniform.reset(wgpuDeviceCreateBuffer(device->nativeDevice(), &buffer));
                if (!outlineUniform) return false;
            }
            WGPUBindGroupEntry entry{};
            entry.buffer = outlineUniform.get();
            entry.size = sizeof(OutlineUniforms);
            WGPUBindGroupDescriptor bindings{};
            bindings.layout = layout.get();
            bindings.entryCount = 1;
            bindings.entries = &entry;
            Detail::Bindings nextBindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &bindings));
            if (!nextBindings) return false;
            outlinePipeline = std::move(next);
            outlineLayout = std::move(layout);
            outlineBindings = std::move(nextBindings);
            outlineFormat = targetFormat;
        }
        if (outlineCapacity < vertices) {
            qsizetype capacity = std::max<qsizetype>(64, outlineCapacity);
            while (capacity < vertices) capacity *= 2;
            WGPUBufferDescriptor buffer{};
            buffer.size = capacity * sizeof(OutlineVertex);
            buffer.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            Detail::Buffer next(wgpuDeviceCreateBuffer(device->nativeDevice(), &buffer));
            if (!next) return false;
            outlineVertices = std::move(next);
            outlineCapacity = capacity;
        }
        return device->diagnostic().isEmpty();
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
    const QTransform &transform, QColor clearColor, DisplayChannels channels,
    const ToolOutline &outline, const std::optional<CheckerPattern> &checkers,
    const std::optional<CropPreview> &crop)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || !d.device->ownsCommands(commands) ||
        target.device != d.device || !target.view || target.size.isEmpty() || !projection ||
        projection->device() != d.device || !Detail::validDocumentTransform(transform) || !clearColor.isValid() || channels.rgbaMask > 15)
        return {false, QStringLiteral("Presentation requires an owning GPU frame, an invertible affine transform and valid RGBA channel bits"), 0};
    if (!d.error.isEmpty() || d.device->isLost()) return {false, d.error.isEmpty() ? QStringLiteral("GPU device lost") : d.error, 0};
    std::vector<Detail::CropPreviewRenderer::Draw> cropDraws;
    QString cropDiagnostic;
    if (crop && !d.cropRenderer.prepare(*crop, target, cropDraws, cropDiagnostic))
        return {false, cropDiagnostic, 0};
    if (checkers) {
        if (!checkers->first.isValid() || !checkers->second.isValid() || checkers->cellSize <= 0 ||
            !Detail::validDocumentTransform(checkers->targetToPattern))
            return {false, QStringLiteral("GPU canvas checks require valid colors, a positive cell size and an invertible pattern transform"), 0};
        const auto cells = checkers->targetToPattern.mapRect(QRectF(QPointF(), target.size));
        for (const qreal value : {cells.left(), cells.top(), cells.right(), cells.bottom()})
            if (!std::isfinite(value) || std::abs(value / checkers->cellSize) > 8388608)
                return {false, QStringLiteral("GPU canvas check coordinates exceed the supported range"), 0};
    }
    std::vector<OutlineVertex> outlineVertices;
    if (!outlineGeometry(outline, outlineVertices))
        return {false, QStringLiteral("GPU tool outline requires finite geometry, a positive width and at most 65536 vertices"), 0};
    if (!outlineVertices.empty() && !d.prepareOutline(target.format, qsizetype(outlineVertices.size())))
        return {false, QStringLiteral("GPU tool outline resources could not be prepared: %1").arg(d.device->diagnostic()), 0};
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
    if (checkers && !d.preparePipeline(target.format, true))
        return {false, QStringLiteral("GPU canvas background pipeline could not be created: %1").arg(d.device->diagnostic()), 0};
    const QRgb premultipliedBackground = qPremultiply(clearColor.rgba());
    const quint32 background[] = {quint32(qRed(premultipliedBackground)), quint32(qGreen(premultipliedBackground)),
                                  quint32(qBlue(premultipliedBackground)), quint32(qAlpha(premultipliedBackground))};
    Uniforms frame{};
    std::copy_n(background, 4, frame.background);
    frame.channels[0] = channels.rgbaMask;
    frame.channels[1] = channels.showSingleChannelAsColor;
    // The image boundary admits RGBA16 only with the default linear-sRGB encoding.
    frame.channels[2] = projection->document()->format == Image::Gpu::PixelFormat::Rgba16;
    if (checkers) {
        const auto color = [](QColor input, quint32 *output) {
            const auto value = qPremultiply(input.rgba());
            const quint32 rgba[] = {quint32(qRed(value)), quint32(qGreen(value)),
                                    quint32(qBlue(value)), quint32(qAlpha(value))};
            std::copy_n(rgba, 4, output);
        };
        color(checkers->first, frame.checkerFirst);
        color(checkers->second, frame.checkerSecond);
        const auto &pattern = checkers->targetToPattern;
        const float x[] = {float(pattern.m11()), float(pattern.m21()), float(pattern.dx()), 0};
        const float y[] = {float(pattern.m12()), float(pattern.m22()), float(pattern.dy()), 0};
        std::copy_n(x, 4, frame.checkerX);
        std::copy_n(y, 4, frame.checkerY);
        frame.checker[0] = 1;
        frame.checker[1] = quint32(checkers->cellSize);
    }
    QMatrix4x4 view;
    view.ortho(0, target.size.width(), target.size.height(), 0, -1, 1);
    view = view * QMatrix4x4(transform);
    std::vector<Draw> draws;
    draws.reserve(visible.size());
    for (const auto &tile : visible) {
        Draw draw;
        draw.values = frame;
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
    std::optional<Draw> backgroundDraw;
    if (checkers) {
        backgroundDraw.emplace();
        auto &draw = *backgroundDraw;
        draw.values = frame;
        auto matrix = view;
        const auto imageSize = projection->document()->size;
        matrix.scale(imageSize.width(), imageSize.height());
        std::copy_n(matrix.constData(), 16, draw.values.matrix);
        WGPUBufferDescriptor buffer{};
        buffer.size = sizeof(Uniforms);
        buffer.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        draw.uniform.reset(wgpuDeviceCreateBuffer(d.device->nativeDevice(), &buffer));
        if (!draw.uniform) return {false, QStringLiteral("GPU canvas background uniform allocation failed"), 0};
        WGPUBindGroupEntry entry{};
        entry.buffer = draw.uniform.get();
        entry.size = sizeof(Uniforms);
        WGPUBindGroupDescriptor bindings{};
        bindings.layout = d.backgroundLayout.get();
        bindings.entryCount = 1;
        bindings.entries = &entry;
        draw.bindings.reset(wgpuDeviceCreateBindGroup(d.device->nativeDevice(), &bindings));
        if (!draw.bindings || !d.device->diagnostic().isEmpty())
            return {false, QStringLiteral("GPU canvas background binding failed"), 0};
        wgpuQueueWriteBuffer(d.device->queue(), draw.uniform.get(), 0, &draw.values, sizeof(Uniforms));
    }
    for (const auto &draw : draws) wgpuQueueWriteBuffer(d.device->queue(), draw.uniform.get(), 0, &draw.values, sizeof(Uniforms));
    if (!outlineVertices.empty()) {
        const OutlineUniforms values{{float(target.size.width()), float(target.size.height()), 0, 0},
                                    {float(outline.color.redF()), float(outline.color.greenF()), float(outline.color.blueF()), 1}};
        wgpuQueueWriteBuffer(d.device->queue(), d.outlineUniform.get(), 0, &values, sizeof(values));
        wgpuQueueWriteBuffer(d.device->queue(), d.outlineVertices.get(), 0, outlineVertices.data(), outlineVertices.size() * sizeof(OutlineVertex));
    }
    WGPURenderPassColorAttachment attachment{};
    attachment.view = target.view;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    attachment.clearValue = {background[0] / 255.0, background[1] / 255.0,
                             background[2] / 255.0, background[3] / 255.0};
    WGPURenderPassDescriptor passDescriptor{};
    passDescriptor.colorAttachmentCount = 1;
    passDescriptor.colorAttachments = &attachment;
    auto pass = wgpuCommandEncoderBeginRenderPass(commands, &passDescriptor);
    if (!pass) return {false, QStringLiteral("GPU presentation pass could not begin"), 0};
    if (backgroundDraw) {
        wgpuRenderPassEncoderSetPipeline(pass, d.backgroundPipeline.get());
        wgpuRenderPassEncoderSetViewport(pass, 0, 0, target.size.width(), target.size.height(), 0, 1);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, backgroundDraw->bindings.get(), 0, nullptr);
        wgpuRenderPassEncoderDraw(pass, 6, 1, 0, 0);
    }
    if (!draws.empty()) {
        wgpuRenderPassEncoderSetPipeline(pass, d.pipeline.get());
        wgpuRenderPassEncoderSetViewport(pass, 0, 0, target.size.width(), target.size.height(), 0, 1);
        for (const auto &draw : draws) {
            wgpuRenderPassEncoderSetBindGroup(pass, 0, draw.bindings.get(), 0, nullptr);
            wgpuRenderPassEncoderDraw(pass, 6, 1, 0, 0);
        }
    }
    if (!outlineVertices.empty()) {
        wgpuRenderPassEncoderSetPipeline(pass, d.outlinePipeline.get());
        wgpuRenderPassEncoderSetViewport(pass, 0, 0, target.size.width(), target.size.height(), 0, 1);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, d.outlineBindings.get(), 0, nullptr);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, d.outlineVertices.get(), 0, outlineVertices.size() * sizeof(OutlineVertex));
        wgpuRenderPassEncoderDraw(pass, quint32(outlineVertices.size()), 1, 0, 0);
    }
    d.cropRenderer.draw(pass, cropDraws);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
    return {true, {}, qsizetype(draws.size())};
}
}
