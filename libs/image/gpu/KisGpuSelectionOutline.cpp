/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuExecution_p.h"
#include "KisGpuTileStorage_p.h"
#include "KisGpuReadbackBatch_p.h"
#include <QScopeGuard>
#include <array>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

ExecutionOutcome Execution::readSelectionOutline(const std::shared_ptr<BufferStorage> &pixels,
    QRect extent, std::optional<QRect> expectedBounds, QRect *bounds, QList<QRect> *outputRuns, quint32 format)
{
    auto &d = *m_d;
    auto &r = *d.renderer->m_d;
    if (!bounds || !outputRuns || !pixels || d.active || extent.isEmpty() || format > 2) return {ExecutionCode::InvalidRequest, {}};
    const auto size = extent.size();
    const quint64 countBytes = quint64(size.height()) * sizeof(quint32);
    if (size.isEmpty() || countBytes + 20 > quint64(d.maximumReadbackBytes))
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline counts exceed the metadata capacity")};
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(d.device->nativeDevice(), &limits) != WGPUStatus_Success ||
        quint64((size.height() + 63) / 64) > limits.maxComputeWorkgroupsPerDimension ||
        countBytes > qMin(limits.maxStorageBufferBindingSize, limits.maxBufferSize))
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline exceeds the device capacity")};
    const auto allocate = [&](quint64 bytes, WGPUBufferUsage usage) -> std::shared_ptr<BufferStorage> {
        if (bytes > quint64(qMax<qsizetype>(0, r.budget->limit - r.budget->used))) return {};
        auto value = createBuffer(d.device->nativeDevice(), bytes, usage);
        return value ? std::make_shared<BufferStorage>(std::move(value), r.budget) : std::shared_ptr<BufferStorage>();
    };
    const auto rows = allocate(countBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    const auto status = allocate(20, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    const auto dummy = allocate(16, WGPUBufferUsage_Storage);
    const auto uniform = allocate(32, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!rows || !status || !dummy || !uniform)
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline working capacity is unavailable")};
    if (!r.coverageGeometryLayout) {
        std::array<WGPUBindGroupLayoutEntry, 5> entries{};
        for (quint32 index = 0; index < entries.size(); ++index) {
            entries[index].binding = index;
            entries[index].visibility = WGPUShaderStage_Compute;
            entries[index].buffer.type = index == 0 ? WGPUBufferBindingType_Uniform :
                index == 1 ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;
        }
        WGPUBindGroupLayoutDescriptor descriptor{};
        descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
        r.coverageGeometryLayout.reset(wgpuDeviceCreateBindGroupLayout(d.device->nativeDevice(), &descriptor));
    }
    if (r.coverageGeometryLayout) {
        if (!r.coverageCountPipeline) r.coverageCountPipeline = r.createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/coverage-outline.wgsl"), "countRows", 0, 0, r.coverageGeometryLayout.get());
        if (!r.coverageOutlinePipeline) r.coverageOutlinePipeline = r.createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/coverage-outline.wgsl"), "writeRows", 0, 0, r.coverageGeometryLayout.get());
    }
    if (!r.coverageCountPipeline || !r.coverageOutlinePipeline)
        return {ExecutionCode::DeviceFailure, r.error};
    const qint32 parameters[8]{size.width(), size.height(), extent.x(), extent.y(), qint32(format), 0, 0, 0};
    const quint32 initial[5]{0, quint32(size.width()), quint32(size.height()), 0, 0};
    wgpuQueueWriteBuffer(d.device->queue(), uniform->pixels.get(), 0, parameters, sizeof(parameters));
    wgpuQueueWriteBuffer(d.device->queue(), status->pixels.get(), 0, initial, sizeof(initial));
    const auto record = [&](WGPUCommandEncoder commands, WGPUComputePipeline pipeline, WGPUBuffer output) {
        const WGPUBuffer buffers[]{uniform->pixels.get(), pixels->pixels.get(), rows->pixels.get(), output, status->pixels.get()};
        std::array<WGPUBindGroupEntry, 5> entries{};
        for (quint32 index = 0; index < entries.size(); ++index) {
            entries[index].binding = index; entries[index].buffer = buffers[index];
            entries[index].size = wgpuBufferGetSize(buffers[index]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = r.coverageGeometryLayout.get();
        descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
        NativeBindings bindings(wgpuDeviceCreateBindGroup(d.device->nativeDevice(), &descriptor));
        if (!bindings) return false;
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return false;
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (size.height() + 63) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        return true;
    };
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    if (!record(frame.commands, r.coverageCountPipeline.get(), dummy->pixels.get()))
        return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline count recording failed")};
    auto reads = ReadbackBatch::create(*d.device, 2);
    if (!reads || !reads->recordBuffer(*d.device, frame.commands, 0, rows->pixels.get(), countBytes) ||
        !reads->recordBuffer(*d.device, frame.commands, 1, status->pixels.get(), 20))
        return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline count read failed")};
    auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    if (reads->results[0]->data.size() != qsizetype(countBytes) || reads->results[1]->data.size() != 20)
        return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline metadata is incomplete")};
    quint32 actual[5]; std::memcpy(actual, reads->results[1]->data.constData(), sizeof(actual));
    const QRect actualBounds = actual[1] <= actual[3] && actual[2] <= actual[4]
        ? QRect(QPoint(actual[1], actual[2]), QPoint(actual[3], actual[4])).translated(extent.topLeft()) : QRect();
    if (actual[0] || (expectedBounds && actualBounds != *expectedBounds))
        return {ExecutionCode::DeviceFailure, QStringLiteral("Selection geometry and coverage bounds disagree")};
    QList<quint32> offsets(size.height());
    quint64 total = 0;
    for (int row = 0; row < size.height(); ++row) {
        quint32 count;
        std::memcpy(&count, reads->results[0]->data.constData() + row * sizeof(count), sizeof(count));
        if (count > quint32((size.width() + 1) / 2))
            return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline row count is invalid")};
        offsets[row] = quint32(total);
        total += count;
    }
    const quint64 bytes = total * 16;
    if (bytes > quint64(SelectionMask::MaximumBytes) || bytes > quint64(d.maximumReadbackBytes) ||
        bytes > qMin(limits.maxStorageBufferBindingSize, limits.maxBufferSize))
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline geometry exceeds its capacity")};
    QList<QRect> runs;
    if (total) {
        const auto output = allocate(bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
        if (!output) return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline allocation failed")};
        wgpuQueueWriteBuffer(d.device->queue(), rows->pixels.get(), 0, offsets.constData(), countBytes);
        frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
        if (!record(frame.commands, r.coverageOutlinePipeline.get(), output->pixels.get()))
            return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline recording failed")};
        reads = ReadbackBatch::create(*d.device, 1);
        if (!reads || !reads->recordBuffer(*d.device, frame.commands, 0, output->pixels.get(), bytes))
            return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline read failed")};
        submitted = endFrame(false);
        if (submitted.code != ExecutionCode::Success) return submitted;
        if (reads->results[0]->data.size() != qsizetype(bytes))
            return {ExecutionCode::DeviceFailure, QStringLiteral("Selection outline geometry is incomplete")};
        runs.reserve(qsizetype(total));
        for (quint64 index = 0; index < total; ++index) {
            qint32 values[4];
            std::memcpy(values, reads->results[0]->data.constData() + index * 16, 16);
            runs.append(QRect(values[0], values[1], values[2], values[3]));
        }
    }
    *bounds = actualBounds;
    *outputRuns = std::move(runs);
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::regionSelectionOutline(const RegionFillSP &region,
    const std::shared_ptr<BufferStorage> &pixels, const QRect &bounds, SelectionMaskSP *selection)
{
    if (!selection || !region || !pixels) return {ExecutionCode::InvalidRequest, {}};
    if (bounds.isEmpty()) { *selection = SelectionMask::empty(); return {ExecutionCode::Success, {}}; }
    QRect actual;
    QList<QRect> runs;
    const auto read = readSelectionOutline(pixels, QRect(region->coverageOrigin, region->coverageSize), bounds, &actual, &runs);
    if (read.code != ExecutionCode::Success) return read;
    const auto native = std::make_shared<SelectionPixels>();
    native->device = m_d->device;
    native->region = pixels;
    const auto result = SelectionMask::fromRegion(region, bounds, runs, native);
    if (!result) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection outline geometry is invalid")};
    *selection = result;
    return {ExecutionCode::Success, {}};
}
}
