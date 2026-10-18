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

bool preparePaintJob(Device &device, WGPUBindGroupLayout layout, PaintJob &job,
                     WGPUBuffer zero, WGPUBuffer unusedCoverage)
{
    if (!job.output) return false;
    if (!job.geometry.isEmpty()) {
        job.samples = createBuffer(device.nativeDevice(), quint64(job.geometry.size()) * sizeof(PaintSegment),
                                    WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        if (!job.samples) return false;
    }
    job.parameters = createBuffer(device.nativeDevice(), sizeof(PaintParameters), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!job.parameters) return false;
    WGPUBindGroupEntry entries[7]{};
    WGPUBuffer buffers[] = {job.parameters.get(), job.samples ? job.samples.get() : zero,
                            job.input ? job.input->pixels.get() : zero, job.output->pixels.get(),
                            job.fillEdges ? job.fillEdges->get() : zero,
                            job.coverageInput ? job.coverageInput->pixels.get() : zero,
                            job.coverageOutput ? job.coverageOutput->pixels.get() : unusedCoverage};
    for (quint32 i = 0; i < 7; ++i) {
        entries[i].binding = i;
        entries[i].buffer = buffers[i];
        entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = layout;
    descriptor.entryCount = 7;
    descriptor.entries = entries;
    job.bindings.reset(wgpuDeviceCreateBindGroup(device.nativeDevice(), &descriptor));
    return bool(job.bindings) && device.diagnostic().isEmpty();
}
}
