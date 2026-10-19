/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 * SPDX-FileCopyrightText: 2005 C. Boemann <cbo@boemann.dk>
 * SPDX-FileCopyrightText: 2013 Juan Palacios <jpalaciosdev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintJob_p.h"
#include <QPainter>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
namespace Detail {
struct ResizeReadbackResources
{
    std::shared_ptr<Budget> budget;
    qsizetype bytes;
    NativeBuffer accumulated, spans, weights, output, parameters;
    ResizeReadbackResources(std::shared_ptr<Budget> owner, qsizetype size)
        : budget(std::move(owner)), bytes(size) { budget->used += bytes; }
    ~ResizeReadbackResources() { budget->used -= bytes; }
};
}
namespace {
struct AxisSpan { int first; QList<qint32> weights; };

int cubicWeight(int distance)
{
    const int t = std::abs(distance);
    int value = 0;
    if (t < 256) value = (3 * t - 1280) * t * t / 2 + (256 << 16);
    else if (t < 512) value = ((1280 - t) * t / 2 - 4 * 256 * 256) * t + (2 * 256 << 16);
    value = (value + 0x8000) >> 16;
    return value >= 128 ? value - 1 : value;
}

AxisSpan cubicSpan(int destination, qreal scale)
{
    // Preserve the 8-bit position and 255-sum coefficient rules used by
    // kis_filter_weights_buffer.h and kis_filter_weights_applicator.h.
    const int positionScale = scale < 1. && scale > 1. / 256 ? int(scale * 256) : 256;
    const int support = scale < 1. && scale > 1. / 256 ? int(512 / scale) : 512;
    const int sourceCenter = int((destination + .5) / scale * 256);
    const int nextCenter = (sourceCenter - 128 + 255) / 256 * 256 + 128;
    const int phase = ((nextCenter - sourceCenter) * positionScale) >> 8;
    const int begin = -support - phase * 256 / positionScale;
    const int end = support - phase * 256 / positionScale;
    const int count = 2 * support / 256 + (begin % 256 == 0 && end % 256 == 0);
    AxisSpan result{nextCenter / 256 + begin / 256, {}};
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        const int weight = cubicWeight(phase + (begin / 256 + i) * positionScale);
        result.weights.append(weight);
        sum += weight;
    }
    if (sum > 0 && sum != 255) {
        const qreal correction = 255. / sum;
        sum = 0;
        for (auto &weight : result.weights) { weight = qRound(weight * correction); sum += weight; }
    }
    while (sum != 255) {
        const int correction = sum < 255 ? 1 : -1;
        *std::max_element(result.weights.begin(), result.weights.end()) += correction;
        sum += correction;
    }
    return result;
}

AxisSpan areaSpan(int destination, int sourceSize, int outputSize, int origin)
{
    // Qt 6.11.1's document-preview fallback uses 16-bit positions and
    // 14-bit area weights (8-bit bilinear weights when enlarging).
    const bool up = outputSize >= sourceSize;
    const qint64 increment = (qint64(sourceSize) << 16) / outputSize;
    const qint64 position = (up ? qint64(32768) * sourceSize / outputSize - 32768 : 0) + destination * increment;
    const int first = int(position >> 16);
    AxisSpan result{origin + std::max(0, first), {}};
    if (up) {
        const int fraction = first < 0 || first >= sourceSize - 1 ? 0 : int((position >> 8) & 255);
        result.weights = {256 - fraction, fraction};
    } else {
        const int coverage = int((qint64(outputSize) * 16384 + sourceSize - 1) / sourceSize);
        const int firstWeight = int((65536 - (position & 65535)) * coverage >> 16);
        result.weights.append(firstWeight);
        int remaining = 16384 - firstWeight;
        while (remaining > coverage) { result.weights.append(coverage); remaining -= coverage; }
        result.weights.append(remaining);
    }
    return result;
}

QImage cropTransparentEdges(const QImage &image)
{
    int left = image.width(), top = image.height(), right = -1, bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        const auto *line = image.constScanLine(y);
        for (int x = 0; x < image.width(); ++x) if (line[x * 4 + 3]) {
            left = std::min(left, x); right = std::max(right, x);
            top = std::min(top, y); bottom = std::max(bottom, y);
        }
    }
    if (right < left) return {};
    const QRect bounds(QPoint(left, top), QPoint(right, bottom));
    return bounds == image.rect() ? image : image.copy(bounds);
}
}

bool TileRenderer::Private::recordResizePass(WGPUCommandEncoder commands, WGPUBuffer source,
    WGPUBuffer accumulated, WGPUBuffer spans, WGPUBuffer weights, WGPUBuffer output,
    WGPUBuffer parameters, quint32 count)
{
    if (!resizePipeline) {
        resizePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/resize.wgsl"));
        if (resizePipeline) resizeLayout.reset(wgpuComputePipelineGetBindGroupLayout(resizePipeline.get(), 0));
    }
    if (!resizePipeline || !resizeLayout) return false;
    const WGPUBuffer buffers[] = {source ? source : zero.get(), accumulated, spans, weights, output, parameters};
    WGPUBindGroupEntry entries[6]{};
    for (quint32 i = 0; i < 6; ++i) {
        entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = resizeLayout.get(); descriptor.entryCount = 6; descriptor.entries = entries;
    Detail::NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings || !device->diagnostic().isEmpty()) return false;
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, resizePipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return true;
}

ImageReadbackOutcome Execution::readResizedProjectionImage(DocumentHandle handle, const Snapshot &pinned,
                                                         QSize scaledCanvas, qsizetype maximumBytes)
{
    using namespace Detail;
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        return ImageReadbackOutcome{code, {}, 0, {}, diagnostic};
    };
    const auto checked = readPixels(handle, pinned, {}, {}, true);
    if (checked.code != ExecutionCode::Success) return rejected(checked.code, checked.diagnostic);
    if (scaledCanvas.isEmpty() || maximumBytes < 0)
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Projection resize dimensions or capacity are invalid"));
    const qint64 pixels = qint64(scaledCanvas.width()) * scaledCanvas.height();
    if (pixels > maximumBytes / 4 || pixels > std::numeric_limits<int>::max() / 4)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection resize output capacity exceeded"));
    const qreal scaleX = qreal(scaledCanvas.width()) / pinned->size.width();
    const qreal scaleY = qreal(scaledCanvas.height()) / pinned->size.height();
    const bool area = scaleX < 1. / 256 || scaleY < 1. / 256;
    ImageReadbackOutcome result{ExecutionCode::Success, pinned, checked.generation, {}, {}};
    const bool constant = std::all_of(pinned->layers.cbegin(), pinned->layers.cend(),
        [](const auto &layer) { return !layer.visible || layer.operations.isEmpty(); });
    if (constant) {
        const auto pixel = readThumbnail(handle, pinned, 0, {0, 0, 1, 1}, {1, 1}, 4);
        if (pixel.code != ExecutionCode::Success) return pixel;
        const auto color = pixel.image.pixelColor(0, 0);
        if (!color.alpha()) return result;
        const auto size = area ? pinned->size.scaled(scaledCanvas, Qt::KeepAspectRatio).expandedTo({1, 1}) : scaledCanvas;
        result.image = QImage(size, area ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBA8888);
        if (result.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Constant preview allocation failed"));
        result.image.fill(color);
        return result;
    }
    QRect source(QPoint(), pinned->size);
    if (!pinned->projectionBackground[3]) {
        const auto bounds = readBoundsImpl(handle, pinned, 0, false, true);
        if (bounds.code != ExecutionCode::Success) return rejected(bounds.code, bounds.diagnostic);
        source = *bounds.bounds;
    }
    if (source.isEmpty()) return result;
    if (scaledCanvas == pinned->size) return readThumbnail(handle, pinned, 0, source, source.size(), maximumBytes);
    QRect destination(QPoint(qRound(source.x() * scaleX), qRound(source.y() * scaleY)),
                      QPoint(qRound((source.x() + source.width()) * scaleX) - 1,
                             qRound((source.y() + source.height()) * scaleY) - 1));
    if (area) destination = QRect(QPoint(), source.size().scaled(scaledCanvas, Qt::KeepAspectRatio).expandedTo({1, 1}));
    else {
        if (!destination.width()) destination.setWidth(1);
        if (!destination.height()) destination.setHeight(1);
    }
    if (destination.isEmpty()) return result;
    const bool upX = destination.width() >= source.width(), upY = destination.height() >= source.height();
    if (area && upX && upY) {
        auto read = readThumbnail(handle, pinned, 0, source, source.size(), maximumBytes);
        if (read.code != ExecutionCode::Success) return read;
        try {
            const auto pixels = read.image.convertToFormat(QImage::Format_ARGB32);
            read.image = QImage(destination.size(), QImage::Format_ARGB32_Premultiplied);
            if (read.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Preview enlargement allocation failed"));
            read.image.fill(Qt::transparent);
            QPainter painter(&read.image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.scale(qreal(destination.width()) / pixels.width(), qreal(destination.height()) / pixels.height());
            painter.drawImage(QPoint(), pixels);
            painter.end();
            return read;
        } catch (const std::bad_alloc &) {
            return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Preview enlargement allocation failed"));
        }
    }
    const quint32 mode = !area ? 0 : !upX && !upY ? 2 : 1;
    result.image = QImage(destination.size(), area ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGBA8888);
    if (result.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection resize image allocation failed"));
    auto &d = *m_d;
    if (d.maximumReadbackBytes < 4)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection resize transfer capacity exceeded"));
    QList<LayerId> generated;
    for (const auto &layer : pinned->layers) if (layer.visible) generated.append(layer.id);
    const auto budget = d.renderer->m_d->budget;
    bool begun = false;
    try {
        for (int y = 0; y < destination.height(); ++y) {
            const auto vertical = area ? areaSpan(y, source.height(), destination.height(), source.y()) : cubicSpan(y + destination.y(), scaleY);
            const int rowStart = std::clamp(vertical.first, source.top(), source.bottom());
            const int rowEnd = std::clamp(vertical.first + int(vertical.weights.size()) - 1, source.top(), source.bottom());
            const qsizetype rows = rowEnd - rowStart + 1;
            for (int x = 0; x < destination.width();) {
                int columns = std::min({64, destination.width() - x, int(d.maximumReadbackBytes / 4)});
                QList<std::array<qint32, 4>> spans;
                QList<qint32> weights;
                QSet<int> tileColumns;
                qsizetype workingBytes = 0;
                quint32 yOffset = 0;
                for (;;) {
                    spans.clear(); weights.clear(); tileColumns.clear();
                    for (int column = 0; column < columns; ++column) {
                        const auto horizontal = area ? areaSpan(x + column, source.width(), destination.width(), source.x()) : cubicSpan(x + column + destination.x(), scaleX);
                        spans.append({horizontal.first, int(weights.size()), int(horizontal.weights.size()), 0});
                        weights.append(horizontal.weights);
                        const int first = std::clamp(horizontal.first, source.left(), source.right()) / TileRenderer::TileSize;
                        const int last = std::clamp(horizontal.first + int(horizontal.weights.size()) - 1, source.left(), source.right()) / TileRenderer::TileSize;
                        for (int tile = first; tile <= last; ++tile) tileColumns.insert(tile);
                    }
                    yOffset = quint32(weights.size());
                    weights.append(vertical.weights);
                    workingBytes = rows * columns * 16 + columns * 20 + weights.size() * 4 + 80;
                    if (columns == 1 || workingBytes <= std::min(qsizetype(TileRenderer::TileBytes), d.byteBudget / 2)) break;
                    columns = std::max(1, columns / 2);
                }
                if (workingBytes > budget->limit - budget->used) {
                    const auto retired = retireDisplayCaches();
                    if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
                }
                if (workingBytes > budget->limit - budget->used)
                    return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection resize working storage is pinned"));
                auto work = std::make_shared<ResizeReadbackResources>(budget, workingBytes);
                const auto storageUsage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
                work->accumulated = createBuffer(d.device->nativeDevice(), rows * columns * 16, storageUsage);
                work->spans = createBuffer(d.device->nativeDevice(), spans.size() * 16, storageUsage);
                work->weights = createBuffer(d.device->nativeDevice(), weights.size() * 4, storageUsage);
                work->output = createBuffer(d.device->nativeDevice(), columns * 4, storageUsage | WGPUBufferUsage_CopySrc);
                work->parameters = createBuffer(d.device->nativeDevice(), 80, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
                if (!work->accumulated || !work->spans || !work->weights || !work->output || !work->parameters)
                    return rejected(ExecutionCode::DeviceFailure, QStringLiteral("Projection resize GPU allocation failed"));
                wgpuQueueWriteBuffer(d.device->queue(), work->spans.get(), 0, spans.constData(), spans.size() * 16);
                wgpuQueueWriteBuffer(d.device->queue(), work->weights.get(), 0, weights.constData(), weights.size() * 4);
                std::array<quint32, 20> parameters{0, mode, quint32(columns), quint32(rows), quint32(rowStart), 0, 0, 0,
                    0, 0, packedPixel(pinned->projectionBackground), quint32(source.left()), quint32(source.right()),
                    quint32(source.top()), quint32(source.bottom()), quint32(vertical.first), yOffset, quint32(vertical.weights.size()), 0, 0};
                bool first = true;
                auto ordered = tileColumns.values();
                std::sort(ordered.begin(), ordered.end());
                for (int tileY = rowStart / TileRenderer::TileSize; tileY <= rowEnd / TileRenderer::TileSize; ++tileY) {
                    for (int tileX : ordered) {
                        const QPoint coordinate(tileX, tileY);
                        const QRect region(coordinate * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                        RenderedSnapshot rendered;
                        ProjectedSnapshot projected;
                        for (int attempt = 0; attempt < 2; ++attempt) {
                            const auto frame = beginReadbackFrame();
                            if (frame.code != ExecutionCode::Success) return rejected(frame.code, frame.diagnostic);
                            begun = true;
                            const auto document = d.documents.constFind(handle);
                            const auto paint = d.renderer->recordRegion(frame.commands, pinned, region, generated, document->rendered);
                            ProjectionOutcome compose;
                            if (paint.code == RenderOutcome::Recorded)
                                compose = d.renderer->compose(frame.commands, paint.version, region.intersected(QRect(QPoint(), pinned->size)), document->projected);
                            const auto code = paint.code != RenderOutcome::Recorded ? paint.code : compose.code;
                            if (code == RenderOutcome::Recorded) { rendered = paint.version; projected = compose.version; break; }
                            cancelFrame(); begun = false;
                            if (code != RenderOutcome::CapacityExceeded || attempt)
                                return rejected(code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded : ExecutionCode::DeviceFailure,
                                    paint.code != RenderOutcome::Recorded ? paint.diagnostic : compose.diagnostic);
                            const auto retired = retireDisplayCaches();
                            if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
                        }
                        WGPUBuffer sourceBuffer = nullptr;
                        for (const auto &tile : projected->tiles()) if (tile.coordinate == coordinate) sourceBuffer = tile.pixels;
                        const int firstRow = std::max(rowStart, tileY * TileRenderer::TileSize);
                        parameters[5] = quint32(tileX * TileRenderer::TileSize); parameters[6] = quint32(tileY * TileRenderer::TileSize);
                        parameters[7] = quint32(firstRow);
                        parameters[8] = quint32(std::min(rowEnd + 1, (tileY + 1) * TileRenderer::TileSize) - firstRow);
                        parameters[9] = sourceBuffer ? 1 : 0;
                        wgpuQueueWriteBuffer(d.device->queue(), work->parameters.get(), 0, parameters.data(), 80);
                        if (first) wgpuCommandEncoderClearBuffer(d.commands, work->accumulated.get(), 0, rows * columns * 16);
                        first = false;
                        if (!d.renderer->m_d->recordResizePass(d.commands, sourceBuffer, work->accumulated.get(), work->spans.get(),
                            work->weights.get(), work->output.get(), work->parameters.get(), parameters[8] * columns))
                            throw std::runtime_error(QStringLiteral("Projection resize sampling could not be recorded: %1 %2")
                                .arg(d.renderer->m_d->error, d.device->diagnostic()).toStdString());
                        const auto submitted = endFrame(true); begun = false;
                        if (submitted.code != ExecutionCode::Success) return rejected(submitted.code, submitted.diagnostic);
                    }
                }
                struct Transfer { QByteArray bytes; QString diagnostic; };
                std::future<Transfer> future;
                for (quint32 pass = 1; pass <= 2; ++pass) {
                    const auto frame = beginReadbackFrame();
                    if (frame.code != ExecutionCode::Success) return rejected(frame.code, frame.diagnostic);
                    begun = true;
                    parameters[0] = pass;
                    wgpuQueueWriteBuffer(d.device->queue(), work->parameters.get(), 0, parameters.data(), 80);
                    if (!d.renderer->m_d->recordResizePass(d.commands, nullptr, work->accumulated.get(), work->spans.get(),
                        work->weights.get(), work->output.get(), work->parameters.get(), pass == 1 ? rows * columns : columns))
                        throw std::runtime_error(QStringLiteral("Projection resize finalization could not be recorded: %1 %2")
                            .arg(d.renderer->m_d->error, d.device->diagnostic()).toStdString());
                    if (pass == 2) {
                        auto reads = ReadbackBatch::create(*d.device, 1);
                        if (!reads) throw std::runtime_error("Projection resize readback is occupied");
                        auto promise = std::make_shared<std::promise<Transfer>>();
                        future = promise->get_future();
                        auto *read = reads->results.front().get();
                        read->completed = [read, promise] { promise->set_value({read->data, read->diagnostic}); };
                        d.readbackResize = work;
                        if (!reads->recordBuffer(*d.device, d.commands, 0, work->output.get(), columns * 4))
                            throw std::runtime_error("Projection resize readback could not be recorded");
                    }
                    const auto submitted = endFrame(true); begun = false;
                    if (submitted.code != ExecutionCode::Success) return rejected(submitted.code, submitted.diagnostic);
                }
                const auto transfer = future.get();
                if (transfer.bytes.size() != columns * 4) return rejected(ExecutionCode::DeviceFailure, transfer.diagnostic);
                std::memcpy(result.image.scanLine(y) + x * 4, transfer.bytes.constData(), size_t(columns * 4));
                x += columns;
            }
        }
        if (!area) result.image = cropTransparentEdges(result.image);
        return result;
    } catch (const std::exception &error) {
        if (begun) cancelFrame();
        retireCompletedReadback();
        return rejected(ExecutionCode::DeviceFailure, QString::fromUtf8(error.what()));
    }
}
}
