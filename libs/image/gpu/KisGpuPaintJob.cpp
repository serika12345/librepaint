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

bool preparePaintJob(QRhi &rhi, PaintJob &job, QRhiBuffer *zero)
{
    if (!job.output) return false;
    job.output->mayContainHiddenRgb = job.values.uniformFill
        ? (job.values.color >> 24) == 0 && (job.values.color & 0x00ffffffu) != 0
        : job.input && job.input->mayContainHiddenRgb;
    if (!job.geometry.isEmpty()) {
        job.samples = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer,
                                            quint32(job.geometry.size() * sizeof(PaintSegment))));
        if (!job.samples->create()) return false;
    }
    job.parameters = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(PaintParameters)));
    job.bindings = own(rhi, rhi.newShaderResourceBindings());
    if (!job.parameters->create()) return false;
    job.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, job.parameters.get()),
        QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, job.samples ? job.samples.get() : zero),
        QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::ComputeStage, job.input ? job.input->pixels.get() : zero),
        QRhiShaderResourceBinding::bufferStore(3, QRhiShaderResourceBinding::ComputeStage, job.output->pixels.get())
    });
    return job.bindings->create();
}
}
