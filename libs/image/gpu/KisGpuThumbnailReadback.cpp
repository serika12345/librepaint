/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintJob_p.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <iterator>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
namespace Detail {
struct ThumbnailReadbackResources
{
    std::shared_ptr<Budget> budget;
    qsizetype bytes;
    NativeBuffer output, samples, parameters;
    ThumbnailReadbackResources(std::shared_ptr<Budget> owner, qsizetype count)
        : budget(std::move(owner)), bytes(count) { budget->used += bytes; }
    ~ThumbnailReadbackResources() { budget->used -= bytes; }
};
}
using namespace Detail;

bool TileRenderer::Private::recordThumbnailSamples(WGPUCommandEncoder commands, WGPUBuffer source,
    WGPUBuffer output, WGPUBuffer samples, WGPUBuffer parameters, quint32 count)
{
    if (!thumbnailPipeline) {
        thumbnailPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/thumbnail.wgsl"));
        if (thumbnailPipeline)
            thumbnailLayout.reset(wgpuComputePipelineGetBindGroupLayout(thumbnailPipeline.get(), 0));
    }
    if (!thumbnailPipeline || !thumbnailLayout) return false;
    const WGPUBuffer buffers[] = {source ? source : zero.get(), output, samples, parameters};
    WGPUBindGroupEntry entries[4]{};
    for (quint32 i = 0; i < 4; ++i) {
        entries[i].binding = i;
        entries[i].buffer = buffers[i];
        entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = thumbnailLayout.get();
    descriptor.entryCount = 4;
    descriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings || !device->diagnostic().isEmpty()) return false;
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, thumbnailPipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    return true;
}

ImageReadbackOutcome Execution::readThumbnail(DocumentHandle handle, const Snapshot &pinned,
    LayerId layer, QRect source, QSize size, qsizetype maximumBytes, bool includeLayerTileExtent)
{
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        return ImageReadbackOutcome{code, {}, 0, {}, diagnostic};
    };
    const auto checked = readPixels(handle, pinned, {}, layer ? QList<LayerId>{layer} : QList<LayerId>{},
                                    !layer, PixelReadArea::LayerCoordinates);
    if (checked.code != ExecutionCode::Success) return rejected(checked.code, checked.diagnostic);
    constexpr qint64 limit = OperationDocument::CoordinateLimit;
    if (size.width() < 0 || size.height() < 0 || source.width() < 0 || source.height() < 0 || maximumBytes < 0 ||
        source.x() < -limit || source.y() < -limit || qint64(source.x()) + source.width() > limit ||
        qint64(source.y()) + source.height() > limit)
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Thumbnail dimensions or source region are invalid"));
    const qint64 pixels = qint64(size.width()) * size.height();
    if (pixels > maximumBytes / 4 || pixels > std::numeric_limits<int>::max() / 4)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Thumbnail output pixel capacity exceeded"));
    ImageReadbackOutcome outcome{ExecutionCode::Success, pinned, checked.generation, {}, {}};
    if (size.isEmpty()) return outcome;
    outcome.image = QImage(size, QImage::Format_RGBA8888);
    if (outcome.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Thumbnail allocation failed"));
    outcome.image.fill(Qt::transparent);
    if (includeLayerTileExtent) {
        const auto extent = readLayerTileExtent(handle, pinned, layer);
        if (extent.code != ExecutionCode::Success) return rejected(extent.code, extent.diagnostic);
        source |= *extent.bounds;
    }
    if (source.isEmpty()) return outcome;
    QSize sampledSize = size;
    if (sampledSize.width() > source.width() || sampledSize.height() > source.height())
        sampledSize.scale(source.size(), Qt::KeepAspectRatio);
    if (!sampledSize.width()) sampledSize.setWidth(1);
    if (!sampledSize.height()) sampledSize.setHeight(1);
    auto &d = *m_d;
    const qsizetype chunkPixels = std::min({qsizetype(pixels), d.maximumReadbackBytes / 4,
        qsizetype(TileRenderer::TileBytes / 4), (d.byteBudget - qsizetype(16)) / 12});
    if (!chunkPixels) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Thumbnail transfer or working capacity exceeded"));
    const qsizetype workingBytes = chunkPixels * 12 + 16;
    const auto budget = d.renderer->m_d->budget;
    if (workingBytes > budget->limit - budget->used) {
        const auto retired = retireDisplayCaches();
        if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
    }
    if (workingBytes > budget->limit - budget->used)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Thumbnail working storage is pinned"));
    bool begun = false;
    try {
        auto work = std::make_shared<ThumbnailReadbackResources>(budget, workingBytes);
        work->output = createBuffer(d.device->nativeDevice(), chunkPixels * 4,
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        work->samples = createBuffer(d.device->nativeDevice(), chunkPixels * 8,
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        work->parameters = createBuffer(d.device->nativeDevice(), 16, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!work->output || !work->samples || !work->parameters)
            return rejected(ExecutionCode::DeviceFailure, QStringLiteral("Thumbnail GPU allocation failed"));
        const QRect canvas(QPoint(), pinned->size);
        QList<LayerId> generated;
        auto implicit = pinned->projectionBackground;
        if (layer) implicit = pinned->projectionDefaultPixels().value(layer);
        if (layer) generated.append(layer);
        else for (const auto &value : pinned->projectionInputs()) generated.append(value.layer);
        for (qsizetype start = 0; start < pixels; start += chunkPixels) {
            const qsizetype count = std::min(chunkPixels, qsizetype(pixels) - start);
            QMap<quint64, QList<std::array<quint32, 2>>> groups;
            for (qsizetype i = 0; i < count; ++i) {
                const qsizetype x = (start + i) % size.width(), y = (start + i) / size.width();
                if (x >= sampledSize.width() || y >= sampledSize.height()) continue;
                const QPoint point(source.x() + int(qint64(x) * source.width() / sampledSize.width()),
                                   source.y() + int(qint64(y) * source.height() / sampledSize.height()));
                const bool outsideProjection = !layer && !canvas.contains(point);
                if (outsideProjection && !includeLayerTileExtent) continue;
                const QPoint tile(pixelTileIndex(point.x(), TileRenderer::TileSize), pixelTileIndex(point.y(), TileRenderer::TileSize));
                const auto local = point - tile * TileRenderer::TileSize;
                const quint32 sample = quint32(local.y() * TileRenderer::TileSize + local.x()) |
                    (outsideProjection ? 65536u : 0u);
                groups[key(tile.x(), tile.y())].append({quint32(i), sample});
            }
            bool first = true;
            for (auto group = groups.cbegin(); group != groups.cend(); ++group) {
                const auto coordinate = Detail::coordinate(group.key());
                const QRect region(coordinate * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                RenderedSnapshot rendered;
                ProjectedSnapshot projected;
                for (int attempt = 0; attempt < 2; ++attempt) {
                    const auto frame = beginReadbackFrame();
                    if (frame.code != ExecutionCode::Success) return rejected(frame.code, frame.diagnostic);
                    begun = true;
                    if (!layer && !region.intersects(canvas)) break;
                    const auto document = d.documents.constFind(handle);
                    const auto paint = d.renderer->recordRegion(frame.commands, pinned, region, generated, document->rendered);
                    ProjectionOutcome compose;
                    if (paint.code == RenderOutcome::Recorded && !layer)
                        compose = d.renderer->compose(frame.commands, paint.version, region.intersected(canvas), document->projected);
                    const auto code = paint.code != RenderOutcome::Recorded ? paint.code : layer ? RenderOutcome::Recorded : compose.code;
                    if (code == RenderOutcome::Recorded) {
                        rendered = paint.version;
                        projected = compose.version;
                        break;
                    }
                    cancelFrame();
                    begun = false;
                    if (code != RenderOutcome::CapacityExceeded || attempt)
                        return rejected(code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded : ExecutionCode::DeviceFailure,
                            paint.code != RenderOutcome::Recorded ? paint.diagnostic : compose.diagnostic);
                    const auto retired = retireDisplayCaches();
                    if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
                }
                WGPUBuffer sourceBuffer = nullptr;
                for (const auto &tile : projected ? projected->tiles() : rendered ? rendered->tiles(layer) : QList<TileView>{})
                    if (tile.coordinate == coordinate) sourceBuffer = tile.pixels;
                const quint32 parameters[] = {sourceBuffer ? 1u : 0u, packedPixel(implicit), quint32(group->size()), 0};
                wgpuQueueWriteBuffer(d.device->queue(), work->samples.get(), 0, group->constData(), group->size() * 8);
                wgpuQueueWriteBuffer(d.device->queue(), work->parameters.get(), 0, parameters, sizeof(parameters));
                if (first) wgpuCommandEncoderClearBuffer(d.commands, work->output.get(), 0, chunkPixels * 4);
                first = false;
                if (!d.renderer->m_d->recordThumbnailSamples(d.commands, sourceBuffer, work->output.get(),
                    work->samples.get(), work->parameters.get(), quint32(group->size())))
                    throw std::runtime_error("Thumbnail sampling could not be recorded");
                const bool last = std::next(group) == groups.cend();
                struct Transfer { QByteArray bytes; QString diagnostic; };
                std::future<Transfer> future;
                if (last) {
                    auto reads = ReadbackBatch::create(*d.device, 1);
                    if (!reads) {
                        cancelFrame();
                        begun = false;
                        return rejected(ExecutionCode::Busy, QStringLiteral("Thumbnail readback is still occupied"));
                    }
                    auto result = std::make_shared<std::promise<Transfer>>();
                    future = result->get_future();
                    auto *read = reads->results.front().get();
                    read->completed = [read, result] { result->set_value({read->data, read->diagnostic}); };
                    d.readbackRendered = rendered;
                    d.readbackProjected = projected;
                    d.readbackThumbnail = work;
                    if (!reads->recordBuffer(*d.device, d.commands, 0, work->output.get(), count * 4))
                        throw std::runtime_error("Thumbnail readback could not be recorded");
                }
                const auto submitted = endFrame(true);
                begun = false;
                if (submitted.code != ExecutionCode::Success) return rejected(submitted.code, submitted.diagnostic);
                if (last) {
                    const auto transfer = future.get();
                    if (transfer.bytes.size() != count * 4)
                        return rejected(ExecutionCode::DeviceFailure, transfer.diagnostic);
                    std::memcpy(outcome.image.bits() + start * 4, transfer.bytes.constData(), size_t(count * 4));
                }
            }
        }
        return outcome;
    } catch (const std::exception &error) {
        if (begun) cancelFrame();
        retireCompletedReadback();
        return rejected(ExecutionCode::DeviceFailure, QString::fromUtf8(error.what()));
    }
}
}
