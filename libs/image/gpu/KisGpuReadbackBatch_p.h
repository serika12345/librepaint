/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_READBACK_BATCH_P_H
#define KIS_GPU_READBACK_BATCH_P_H

#include "KisGpuDevice.h"
#include <rhi/qrhi.h>
#include <vector>

namespace Krita::Image::Gpu::Detail {
/** Native callbacks retain borrowed result addresses even after a submission error. */
struct ReadbackBatch
{
    static std::shared_ptr<ReadbackBatch> create(Device &device, qsizetype count);
    std::vector<std::unique_ptr<QRhiReadbackResult>> results;
    qsizetype remaining = 0;
};
}
#endif
