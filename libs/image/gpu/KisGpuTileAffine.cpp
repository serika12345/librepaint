/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
struct AffineWork {
    std::shared_ptr<BudgetUsage> usage;
    std::vector<NativeBuffer> buffers;
    QList<std::shared_ptr<BufferStorage>> inputs;
    qsizetype bytes = 0;
    explicit AffineWork(const std::shared_ptr<Budget> &owner) : usage(owner->usage) {}
    ~AffineWork() { usage->bytes -= bytes; }
    WGPUBuffer allocate(const std::shared_ptr<Budget> &owner, qsizetype size, WGPUBufferUsage flags) {
        size = std::max<qsizetype>(4, size);
        if (size > owner->limit - owner->used) return nullptr;
        auto buffer = createBuffer(owner->device->nativeDevice(), size, flags);
        if (!buffer) return nullptr;
        const auto actual = wgpuBufferGetSize(buffer.get());
        bytes += actual; usage->bytes += actual;
        const auto native = buffer.get(); buffers.push_back(std::move(buffer)); return native;
    }
};
QRect inverseRegion(const AffinePass &pass, QRect output)
{
    if (output.isEmpty()) return {};
    if (pass.kind == AffinePass::Remap) {
        if (!pass.mapWholeLayer) output &= pass.destination;
        if (output.isEmpty()) return {};
        const auto &m = pass.map;
        const auto point = [&](QPoint p) {
            p -= QPoint(m[4], m[5]); return QPoint(m[0] * p.x() + m[2] * p.y(), m[1] * p.x() + m[3] * p.y());
        };
        const auto a = point(output.topLeft()), b = point(output.bottomRight());
        auto result = QRect(QPoint(std::min(a.x(), b.x()), std::min(a.y(), b.y())),
                            QPoint(std::max(a.x(), b.x()), std::max(a.y(), b.y())));
        return pass.mapWholeLayer ? result : result.intersected(pass.source);
    }
    output &= pass.destination;
    if (output.isEmpty()) return {};
    const bool horizontal = pass.kind == AffinePass::Horizontal;
    const auto coordinate = [&](int pixel, int line) { return (pixel + .5 - pass.offset - line * pass.shear) / pass.scale; };
    const int first = horizontal ? output.left() : output.top(), last = horizontal ? output.right() : output.bottom();
    const int startLine = horizontal ? output.top() : output.left(), endLine = horizontal ? output.bottom() : output.right();
    const double values[]{coordinate(first, startLine), coordinate(first, endLine), coordinate(last, startLine), coordinate(last, endLine)};
    int span = 0; for (const auto &phase : pass.kernel->phases) span = std::max(span, phase.count);
    const auto limits = std::minmax_element(std::begin(values), std::end(values));
    if (!std::all_of(std::begin(values), std::end(values), [](double v) { return std::isfinite(v); })) return pass.source;
    const auto bounded = [](double v) { return int(std::clamp(v, -2. * RgbaTile::CoordinateLimit, 2. * RgbaTile::CoordinateLimit)); };
    const int minimum = bounded(std::floor(*limits.first) - span - 2);
    const int maximum = bounded(std::ceil(*limits.second) + span + 2);
    QRect result = horizontal ? QRect(minimum, startLine, maximum - minimum + 1, endLine - startLine + 1) :
                                QRect(startLine, minimum, endLine - startLine + 1, maximum - minimum + 1);
    if (pass.shear == 0) {
        if (horizontal) { result.setLeft(std::clamp(result.left(), pass.source.left(), pass.source.right())); result.setRight(std::clamp(result.right(), pass.source.left(), pass.source.right())); }
        else { result.setTop(std::clamp(result.top(), pass.source.top(), pass.source.bottom())); result.setBottom(std::clamp(result.bottom(), pass.source.top(), pass.source.bottom())); }
    }
    return result.intersected(pass.source);
}
std::array<quint32, 36> parameters(const AffinePass &pass, QPoint origin, const RgbaPixel &implicit,
                                  PixelFormat format, quint32 count)
{
    std::array<quint32, 36> value{quint32(origin.x()), quint32(origin.y()), quint32(format), quint32(pass.kind)};
    const auto rect = [&](int offset, QRect bounds) {
        value[offset] = bounds.x(); value[offset + 1] = bounds.y();
        value[offset + 2] = bounds.x() + bounds.width(); value[offset + 3] = bounds.y() + bounds.height();
    };
    rect(4, pass.source); rect(8, pass.destination);
    for (int i = 0; i < 4; ++i) value[12 + i] = pass.map[i];
    value[16] = pass.map[4]; value[17] = pass.map[5]; value[18] = pass.replaceLayer; value[19] = pass.mapWholeLayer;
    const double values[]{pass.scale, pass.shear, pass.offset, pass.kernel ? pass.kernel->support : 0};
    std::memcpy(value.data() + 20, values, sizeof(values));
    for (int i = 0; i < 4; ++i) value[28 + i] = implicit[i];
    value[32] = count; value[33] = pass.kernel ? pass.kernel->positionScale : 256;
    return value;
}
}

TileRenderer::Private::AffineResult TileRenderer::Private::affineTiles(WGPUCommandEncoder commands,
    const Tiles &source, const RgbaPixel &implicit, PixelFormat format, const AffinePlan &plan, QRect region)
{
    if (!validAffinePlan(plan)) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Invalid native affine plan")};
    const qsizetype tileBytes = RgbaTile::byteCount(format);
    const qsizetype maximum = budget->limit / tileBytes;
    QList<QRect> needed(plan.passes.size() + 1, region);
    for (qsizetype i = plan.passes.size(); i-- > 0;) needed[i] = affineNeededRegion(plan.passes[i], needed[i + 1]);
    Tiles current = source;
    for (qsizetype step = 0; step < plan.passes.size(); ++step) {
        const auto &pass = plan.passes[step]; const auto outputRegion = needed[step + 1];
        QSet<quint64> coordinates;
        const auto append = [&](QRect area) {
            area &= outputRegion;
            if (area.isEmpty()) return true;
            const int firstX = pixelTileIndex(area.left(), TileSize), lastX = pixelTileIndex(area.right(), TileSize);
            const int firstY = pixelTileIndex(area.top(), TileSize), lastY = pixelTileIndex(area.bottom(), TileSize);
            if (qint64(lastX - firstX + 1) * (lastY - firstY + 1) > maximum) return false;
            for (int y = firstY; y <= lastY; ++y) for (int x = firstX; x <= lastX; ++x) {
                coordinates.insert(key(x, y)); if (coordinates.size() > maximum) return false;
            }
            return true;
        };
        if (!pass.replaceLayer) {
            for (auto tile = current.cbegin(); tile != current.cend(); ++tile) {
                const QRect area(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize));
                const auto transformed = pass.mapWholeLayer ? affineOutputRegion(pass, area) : area;
                if (!append(transformed)) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine output coordinates exceed capacity")};
            }
        }
        if (!pass.mapWholeLayer && !append(pass.destination))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine destination coordinates exceed capacity")};
        struct Job { QPoint coordinate; QList<quint64> inputs; };
        QList<Job> jobs; qsizetype atlasBytes = 4;
        for (const auto point : coordinates) {
            const QPoint origin = coordinate(point); const QRect tileBounds(origin * TileSize, QSize(TileSize, TileSize));
            if (pass.kind != AffinePass::Remap && pass.source.contains(tileBounds) && !pass.destination.intersects(tileBounds)) continue;
            Job job{origin, {}};
            const QRect samples = inverseRegion(pass, tileBounds);
            for (auto input = current.cbegin(); input != current.cend(); ++input) {
                const QRect bounds(coordinate(input.key()) * TileSize, QSize(TileSize, TileSize));
                if (bounds.intersects(samples) || ((!pass.replaceLayer && !pass.mapWholeLayer) && bounds.intersects(tileBounds))) job.inputs.append(input.key());
            }
            std::sort(job.inputs.begin(), job.inputs.end(), [](quint64 a, quint64 b) {
                const auto aa = coordinate(a), bb = coordinate(b); return aa.y() == bb.y() ? aa.x() < bb.x() : aa.y() < bb.y();
            });
            atlasBytes = std::max(atlasBytes, job.inputs.size() * tileBytes); jobs.append(std::move(job));
        }
        if (!ensureTileCapacity(jobs.size(), format)) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine tile capacity exhausted")};
        auto work = std::make_shared<AffineWork>(budget);
        device->retainForFrame(work);
        const auto atlas = work->allocate(budget, atlasBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        const auto phases = work->allocate(budget, pass.kernel ? 256 * 16 : 16, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        const auto weights = work->allocate(budget, pass.kernel ? pass.kernel->weights.size() * 4 : 4, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        if (!atlas || !phases || !weights) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine working capacity exhausted")};
        if (pass.kernel) {
            std::array<qint32, 1024> values{};
            for (int i = 0; i < 256; ++i) { values[i * 4] = pass.kernel->phases[i].offset; values[i * 4 + 1] = pass.kernel->phases[i].count; values[i * 4 + 2] = pass.kernel->phases[i].center; }
            wgpuQueueWriteBuffer(device->queue(), phases, 0, values.data(), sizeof(values));
            wgpuQueueWriteBuffer(device->queue(), weights, 0, pass.kernel->weights.constData(), pass.kernel->weights.size() * 4);
        }
        if (!affinePipeline) {
            affinePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/affine.wgsl"));
            if (affinePipeline) affineLayout.reset(wgpuComputePipelineGetBindGroupLayout(affinePipeline.get(), 0));
        }
        if (!affinePipeline || !affineLayout) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("Affine pipeline creation failed")};
        Tiles result;
        for (const auto &job : jobs) {
            const auto output = allocateTile(format);
            const auto lookup = work->allocate(budget, std::max<qsizetype>(16, job.inputs.size() * 16), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
            const auto uniform = work->allocate(budget, 144, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
            if (!output || !lookup || !uniform) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine output working capacity exhausted")};
            QList<qint32> locations; locations.reserve(job.inputs.size() * 4);
            qsizetype offset = 0;
            for (const auto key : job.inputs) {
                const auto tile = current.value(key); const auto point = coordinate(key);
                locations.append(point.x()); locations.append(point.y()); locations.append(qint32(offset / 4)); locations.append(0);
                wgpuCommandEncoderCopyBufferToBuffer(commands, tile->pixels.get(), 0, atlas, offset, tileBytes);
                work->inputs.append(tile->storage); offset += tileBytes;
            }
            if (!locations.isEmpty()) wgpuQueueWriteBuffer(device->queue(), lookup, 0, locations.constData(), locations.size() * 4);
            const auto values = parameters(pass, job.coordinate * TileSize, implicit, format, job.inputs.size());
            wgpuQueueWriteBuffer(device->queue(), uniform, 0, values.data(), sizeof(values));
            const WGPUBuffer buffers[]{atlas, output->pixels.get(), lookup, phases, weights, uniform};
            WGPUBindGroupEntry entries[6]{};
            for (quint32 i = 0; i < 6; ++i) { entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]); }
            WGPUBindGroupDescriptor descriptor{}; descriptor.layout = affineLayout.get(); descriptor.entryCount = 6; descriptor.entries = entries;
            NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
            if (!bindings || !device->diagnostic().isEmpty()) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("Affine binding failed")};
            const auto compute = wgpuCommandEncoderBeginComputePass(commands, nullptr);
            wgpuComputePassEncoderSetPipeline(compute, affinePipeline.get());
            wgpuComputePassEncoderSetBindGroup(compute, 0, bindings.get(), 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(compute, TileSize / 8, TileSize / 8, 1);
            wgpuComputePassEncoderEnd(compute); wgpuComputePassEncoderRelease(compute);
            result.insert(key(job.coordinate.x(), job.coordinate.y()), output);
        }
        device->retainForFrame(work);
        current = std::move(result);
    }
    return {RenderOutcome::Recorded, std::move(current), {}};
}

TileRenderer::Private::AffineResult TileRenderer::Private::affineSelection(
    WGPUCommandEncoder commands, const Tiles &source, const RgbaPixel &implicit, const RgbaPixel &defaultPixel,
    PixelFormat format, const AffineTransformOperation &operation, QRect region, bool capturedOnly)
{
    if (!operation.selection || !operation.plan) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Missing affine selection")};
    retainTiles(source);
    auto inputRegion = region;
    for (qsizetype i = operation.plan->passes.size(); i-- > 0;)
        inputRegion = affineNeededRegion(operation.plan->passes[i], inputRegion);
    const auto needed = region.united(inputRegion.intersected(operation.selection->copyRect));
    QSet<quint64> coordinates;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile)
        if (needed.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) coordinates.insert(tile.key());
    const auto appendImplicit = [&](QRect area) {
        if (area.isEmpty() || !implicit[3]) return true;
        const int left = pixelTileIndex(area.left(), TileSize), right = pixelTileIndex(area.right(), TileSize);
        const int top = pixelTileIndex(area.top(), TileSize), bottom = pixelTileIndex(area.bottom(), TileSize);
        const auto maximum = budget->limit / RgbaTile::byteCount(format);
        if (qint64(right - left + 1) * (bottom - top + 1) > maximum) return false;
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
            coordinates.insert(key(x, y)); if (coordinates.size() > maximum) return false;
        }
        return true;
    };
    if (!appendImplicit(operation.selection->copyRect.intersected(inputRegion)) ||
        !appendImplicit(operation.selection->clearRect.intersected(region)))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Affine selection implicit coordinates exceed capacity")};
    Tiles captured, remainder;
    for (const auto point : coordinates) {
        const auto input = source.value(point);
        const auto split = splitSelectionTile(commands, {coordinate(point), input ? input->pixels.get() : nullptr},
                                              implicit, defaultPixel, format, *operation.selection);
        if (split.code != RenderOutcome::Recorded) return {split.code, {}, split.diagnostic};
        device->retainForFrame(split.coverage->storage);
        captured.insert(point, split.copied);
        if (region.intersects(QRect(coordinate(point) * TileSize, QSize(TileSize, TileSize)))) remainder.insert(point, split.cleared);
    }
    auto transformed = affineTiles(commands, captured, {}, format, *operation.plan, region);
    if (transformed.code != RenderOutcome::Recorded || capturedOnly) return transformed;
    for (auto tile = transformed.tiles.cbegin(); tile != transformed.tiles.cend(); ++tile) {
        const auto base = remainder.value(tile.key());
        const auto merged = mergeSelectionTile(commands, {coordinate(tile.key()), tile.value()->pixels.get()},
            {coordinate(tile.key()), base ? base->pixels.get() : nullptr}, implicit, format);
        if (merged.code != RenderOutcome::Recorded) return {merged.code, {}, merged.diagnostic};
        remainder.insert(tile.key(), merged.tile);
    }
    return {RenderOutcome::Recorded, std::move(remainder), {}};
}
}
