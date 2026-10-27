/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintJob_p.h"
#include <algorithm>
#include <cstring>

namespace LibrePaint::Image::Gpu::Detail {
quint32 packedPixel(const RgbaPixel &rgba)
{
    return quint32(rgba[0]) | quint32(rgba[1]) << 8 | quint32(rgba[2]) << 16 | quint32(rgba[3]) << 24;
}

std::array<quint32, 2> packedPixelWords(const RgbaPixel &rgba, PixelFormat format)
{
    if (format == PixelFormat::Rgba8) return {packedPixel(rgba), 0};
    return {quint32(rgba[0]) | (quint32(rgba[1]) << 16),
            quint32(rgba[2]) | (quint32(rgba[3]) << 16)};
}

bool preparePaintJobs(Device &device, WGPUBindGroupLayout layout, std::vector<PaintJob> &jobs,
                      std::size_t first, WGPUBuffer zero, WGPUBuffer unusedCoverage,
                      WGPUBuffer unusedSelection, WGPUBuffer unusedPattern)
{
    if (first == jobs.size()) return true;
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(device.nativeDevice(), &limits) != WGPUStatus_Success) return false;
    const quint64 alignment = std::max(limits.minUniformBufferOffsetAlignment, limits.minStorageBufferOffsetAlignment);
    if (!alignment || (alignment & (alignment - 1))) return false;
    const auto aligned = [alignment](quint64 bytes) { return (bytes + alignment - 1) / alignment * alignment; };
    const quint64 parameterStride = aligned(sizeof(PaintParameters));
    const auto geometryBytes = [](const PaintJob &job) {
        return std::visit([](const auto &dabs) {
            return quint64(dabs.size()) * sizeof(dabs[0]);
        }, job.geometry);
    };
    const auto stride = [&](const PaintJob &job) {
        return parameterStride + aligned(geometryBytes(job));
    };
    constexpr quint64 batchLimit = 64 * 1024;
    while (first < jobs.size()) {
        std::size_t end = first;
        quint64 bytes = 0;
        do {
            const auto &job = jobs[end];
            if (!job.output || geometryBytes(job) > limits.maxStorageBufferBindingSize) return false;
            const quint64 jobBytes = stride(job);
            if (jobBytes > limits.maxBufferSize) return false;
            if (end != first && (bytes + jobBytes > batchLimit || bytes + jobBytes > limits.maxBufferSize)) break;
            bytes += jobBytes;
            ++end;
        } while (end < jobs.size() && bytes < batchLimit);
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = bytes;
        bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_Storage;
        bufferDescriptor.mappedAtCreation = true;
        NativeBuffer buffer(wgpuDeviceCreateBuffer(device.nativeDevice(), &bufferDescriptor));
        if (!buffer) return false;
        auto mapped = static_cast<char *>(wgpuBufferGetMappedRange(buffer.get(), 0, std::size_t(bytes)));
        if (!mapped) return false;
        quint64 offset = 0;
        for (std::size_t index = first; index < end; ++index) {
            const auto &job = jobs[index];
            std::memcpy(mapped + offset, &job.values, sizeof(PaintParameters));
            std::visit([&](const auto &dabs) {
                if (!dabs.isEmpty())
                    std::memcpy(mapped + offset + parameterStride, dabs.constData(), std::size_t(geometryBytes(job)));
            }, job.geometry);
            offset += stride(job);
        }
        wgpuBufferUnmap(buffer.get());
        offset = 0;
        for (std::size_t index = first; index < end; ++index) {
            auto &job = jobs[index];
            wgpuBufferAddRef(buffer.get());
            job.inputs.reset(buffer.get());
            WGPUBindGroupEntry entries[9]{};
            WGPUBuffer buffers[] = {job.inputs.get(), geometryBytes(job) == 0 ? zero : job.inputs.get(),
                                    job.input ? job.input->pixels.get() : zero, job.output->pixels.get(),
                                    job.fillEdges ? job.fillEdges->get() : zero,
                                    job.coverageInput ? job.coverageInput->pixels.get() : zero,
                                    job.coverageOutput ? job.coverageOutput->pixels.get() : unusedCoverage,
                                    job.selectionCoverage ? job.selectionCoverage->get() : unusedSelection,
                                    job.pattern ? job.pattern->get() : unusedPattern};
            for (quint32 i = 0; i < 9; ++i) {
                entries[i].binding = i;
                entries[i].buffer = buffers[i];
                entries[i].size = wgpuBufferGetSize(buffers[i]);
            }
            entries[0].offset = offset;
            entries[0].size = sizeof(PaintParameters);
            if (geometryBytes(job) != 0) {
                entries[1].offset = offset + parameterStride;
                entries[1].size = geometryBytes(job);
            }
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = layout;
            descriptor.entryCount = 9;
            descriptor.entries = entries;
            job.bindings.reset(wgpuDeviceCreateBindGroup(device.nativeDevice(), &descriptor));
            if (!job.bindings || !device.diagnostic().isEmpty()) return false;
            offset += stride(job);
        }
        first = end;
    }
    return true;
}
}
