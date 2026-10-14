/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintJob_p.h"

namespace LibrePaint::Image::Gpu::Detail {
quint32 packedPixel(const std::array<quint8, 4> &rgba)
{
    return quint32(rgba[0]) | quint32(rgba[1]) << 8 | quint32(rgba[2]) << 16 | quint32(rgba[3]) << 24;
}

bool preparePaintJob(Device &device, WGPUBindGroupLayout layout, PaintJob &job, WGPUBuffer zero)
{
    if (!job.output) return false;
    job.output->mayContainHiddenRgb = job.values.uniformFill
        ? (job.values.color >> 24) == 0 && (job.values.color & 0x00ffffffu) != 0
        : job.input && job.input->mayContainHiddenRgb;
    if (!job.geometry.isEmpty()) {
        job.samples = createBuffer(device.nativeDevice(), quint64(job.geometry.size()) * sizeof(PaintSegment),
                                    WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        if (!job.samples) return false;
    }
    job.parameters = createBuffer(device.nativeDevice(), sizeof(PaintParameters), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!job.parameters) return false;
    WGPUBindGroupEntry entries[4]{};
    WGPUBuffer buffers[] = {job.parameters.get(), job.samples ? job.samples.get() : zero,
                            job.input ? job.input->pixels.get() : zero, job.output->pixels.get()};
    for (quint32 i = 0; i < 4; ++i) {
        entries[i].binding = i;
        entries[i].buffer = buffers[i];
        entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = layout;
    descriptor.entryCount = 4;
    descriptor.entries = entries;
    job.bindings.reset(wgpuDeviceCreateBindGroup(device.nativeDevice(), &descriptor));
    return bool(job.bindings) && device.diagnostic().isEmpty();
}
}
