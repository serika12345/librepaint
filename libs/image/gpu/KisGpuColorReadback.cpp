/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintJob_p.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace LibrePaint::Image::Gpu {
namespace Detail {
struct ColorReadbackResources
{
    std::shared_ptr<Budget> budget;
    qsizetype bytes;
    NativeBuffer output;
    std::vector<NativeBuffer> parameters;
    ColorReadbackResources(std::shared_ptr<Budget> owner, qsizetype count)
        : budget(std::move(owner)), bytes(count) { budget->used += bytes; }
    ~ColorReadbackResources() { budget->used -= bytes; }
};
}
using namespace Detail;

bool TileRenderer::Private::recordColorSample(WGPUCommandEncoder commands, WGPUBuffer source,
    WGPUBuffer output, WGPUBuffer parameters, QSize size)
{
    if (!colorSamplePipeline) {
        colorSamplePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/sample-color.wgsl"));
        if (colorSamplePipeline)
            colorSampleLayout.reset(wgpuComputePipelineGetBindGroupLayout(colorSamplePipeline.get(), 0));
    }
    if (!colorSamplePipeline || !colorSampleLayout) return false;
    const WGPUBuffer buffers[] = {source ? source : zero.get(), output, parameters};
    WGPUBindGroupEntry entries[3]{};
    for (quint32 i = 0; i < 3; ++i) {
        entries[i].binding = i;
        entries[i].buffer = buffers[i];
        entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = colorSampleLayout.get();
    descriptor.entryCount = 3;
    descriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings || !device->diagnostic().isEmpty()) return false;
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, colorSamplePipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (size.width() + 15) / 16, (size.height() + 15) / 16, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    return true;
}

std::future<ColorSampleOutcome> Execution::readColorSampleAsync(DocumentHandle handle,
    const Snapshot &pinned, LayerId layer, QPoint point, int radius)
{
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        std::promise<ColorSampleOutcome> result;
        result.set_value({code, {}, 0, {}, diagnostic});
        return result.get_future();
    };
    auto &d = *m_d;
    if (!d.onThread() || !pinned || radius < 1 || radius > 900)
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Color sample requires a document, radius 1..900 and the GPU thread"));
    if (d.active || d.device->isRecordingFrame() || d.device->readbackPending())
        return rejected(ExecutionCode::Busy, QStringLiteral("Color sample requires submitted frames and an available readback"));
    retireCompletedReadback();
    const auto document = d.documents.constFind(handle);
    if (document == d.documents.cend() || document->requested->documentId() != pinned->documentId())
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Color sample document is no longer attached or belongs to another handle"));
    bool available = !layer;
    for (const auto &value : pinned->layers) available |= value.id == layer;
    if (!available) return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Color sample layer is unavailable"));
    const qint64 extent = radius - 1;
    constexpr qint64 limit = OperationDocument::CoordinateLimit;
    if (qint64(point.x()) - extent < -limit || qint64(point.y()) - extent < -limit ||
        qint64(point.x()) + extent >= limit || qint64(point.y()) + extent >= limit)
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Color sample region is outside the supported range"));
    if (d.maximumReadbackBytes < 32)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Color sample requires 32 readback bytes"));
    const QRect region(point - QPoint(int(extent), int(extent)), QSize(int(extent * 2 + 1), int(extent * 2 + 1)));
    constexpr int side = TileRenderer::TileSize;
    const QPoint first(pixelTileIndex(region.left(), side), pixelTileIndex(region.top(), side));
    const QPoint last(pixelTileIndex(region.right(), side), pixelTileIndex(region.bottom(), side));
    const qsizetype count = (last.x() - first.x() + 1) * (last.y() - first.y() + 1);
    const qsizetype workingBytes = 32 + count * 64;
    const auto budget = d.renderer->m_d->budget;
    if (workingBytes > budget->limit - budget->used) {
        const auto retired = retireDisplayCaches();
        if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
    }
    if (workingBytes > budget->limit - budget->used)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Color sample working storage is pinned"));
    bool begun = false;
    try {
        auto work = std::make_shared<ColorReadbackResources>(budget, workingBytes);
        work->output = createBuffer(d.device->nativeDevice(), 32,
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        if (!work->output) throw std::runtime_error("Color sample summary allocation failed");
        const auto frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return rejected(frame.code, frame.diagnostic);
        begun = true;
        const QRect canvas(QPoint(), pinned->size);
        const QRect sourceRegion = layer ? region : region.intersected(canvas);
        QList<LayerId> generated;
        if (layer) generated.append(layer);
        else for (const auto &value : pinned->projectionInputs()) generated.append(value.layer);
        RenderedSnapshot rendered;
        ProjectedSnapshot projected;
        if (!sourceRegion.isEmpty()) {
            const auto paint = d.renderer->recordRegion(frame.commands, pinned, sourceRegion, generated, document->rendered);
            ProjectionOutcome compose;
            if (paint.code == RenderOutcome::Recorded && !layer)
                compose = d.renderer->compose(frame.commands, paint.version, sourceRegion, document->projected);
            const auto code = paint.code != RenderOutcome::Recorded ? paint.code : layer ? RenderOutcome::Recorded : compose.code;
            if (code != RenderOutcome::Recorded) {
                cancelFrame();
                begun = false;
                return rejected(code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded : ExecutionCode::DeviceFailure,
                    paint.code != RenderOutcome::Recorded ? paint.diagnostic : compose.diagnostic);
            }
            rendered = paint.version;
            projected = compose.version;
        }
        QMap<quint64, WGPUBuffer> tiles;
        for (const auto &tile : projected ? projected->tiles() : rendered ? rendered->tiles(layer) : QList<TileView>{})
            tiles.insert(key(tile.coordinate.x(), tile.coordinate.y()), tile.pixels);
        const auto implicit = layer ? pinned->projectionDefaultPixels().value(layer) : std::array<quint8, 4>{};
        wgpuCommandEncoderClearBuffer(frame.commands, work->output.get(), 0, 32);
        for (int y = first.y(); y <= last.y(); ++y) {
            for (int x = first.x(); x <= last.x(); ++x) {
                const QPoint origin(x * side, y * side);
                const QRect clip = region.intersected(QRect(origin, QSize(side, side))).translated(-origin);
                const auto source = tiles.value(key(x, y));
                const std::array<quint32, 16> values = {
                    quint32(origin.x()), quint32(origin.y()), quint32(point.x()), quint32(point.y()),
                    quint32(clip.left()), quint32(clip.top()), quint32(clip.right() + 1), quint32(clip.bottom() + 1),
                    quint32(canvas.width()), quint32(canvas.height()), quint32(extent * extent), layer ? 0u : 1u,
                    packedPixel(implicit), source ? 1u : 0u, radius == 1 ? 1u : 0u, 0u};
                auto parameters = createBuffer(d.device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
                if (!parameters) throw std::runtime_error("Color sample parameters allocation failed");
                wgpuQueueWriteBuffer(d.device->queue(), parameters.get(), 0, values.data(), sizeof(values));
                if (!d.renderer->m_d->recordColorSample(frame.commands, source, work->output.get(), parameters.get(), clip.size()))
                    throw std::runtime_error("Color sample could not be recorded");
                work->parameters.push_back(std::move(parameters));
            }
        }
        auto reads = ReadbackBatch::create(*d.device, 1);
        if (!reads) throw std::runtime_error("Color sample readback is occupied");
        auto completion = std::make_shared<std::promise<ColorSampleOutcome>>();
        auto future = completion->get_future();
        auto *read = reads->results.front().get();
        const auto generation = d.generation;
        const auto maximumCount = quint32(region.width() * region.height());
        read->completed = [read, completion, pinned, generation, maximumCount] {
            ColorSampleOutcome result{ExecutionCode::Success, pinned, generation, {}, {}};
            std::array<quint32, 8> values{};
            if (read->data.size() == 32) std::memcpy(values.data(), read->data.constData(), 32);
            const quint64 alpha = values[6], count = values[7];
            bool valid = read->data.size() == 32 && count && count <= maximumCount && alpha <= count * 255;
            for (int channel = 0; channel < 3; ++channel) {
                const quint64 weighted = values[channel * 2] | (quint64(values[channel * 2 + 1]) << 32);
                valid &= weighted <= alpha * 255;
                if (alpha) result.rgba[channel] = quint8((weighted + alpha / 2) / alpha);
            }
            if (count) result.rgba[3] = quint8((alpha + count / 2) / count);
            if (!valid) {
                result.code = ExecutionCode::DeviceFailure;
                result.rgba = {};
                result.diagnostic = read->diagnostic.isEmpty() ? QStringLiteral("Invalid GPU color sample summary") : read->diagnostic;
            }
            completion->set_value(std::move(result));
        };
        d.readbackRendered = rendered;
        d.readbackProjected = projected;
        d.readbackColor = work;
        if (!reads->recordBuffer(*d.device, frame.commands, 0, work->output.get(), 32))
            throw std::runtime_error("Color sample readback could not be recorded");
        const auto submitted = endFrame(true, false);
        begun = false;
        if (submitted.code != ExecutionCode::Success) return rejected(submitted.code, submitted.diagnostic);
        return future;
    } catch (const std::exception &error) {
        if (begun) cancelFrame();
        retireCompletedReadback();
        return rejected(ExecutionCode::DeviceFailure, QString::fromUtf8(error.what()));
    }
}
}
