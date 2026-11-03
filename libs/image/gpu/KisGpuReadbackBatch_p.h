/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_READBACK_BATCH_P_H
#define KIS_GPU_READBACK_BATCH_P_H

#include "KisGpuDevice.h"
#include "KisGpuNativeResources_p.h"
#include <QByteArray>
#include <atomic>
#include <functional>
#include <vector>

namespace LibrePaint::Image::Gpu::Detail {
/** Native callbacks retain borrowed result addresses even after a submission error. */
struct ReadbackBatch
{
    struct Result {
        NativeBuffer buffer;
        QByteArray data;
        QString diagnostic;
        ReadbackBatch *owner = nullptr;
        quint64 size = 0;
        std::function<void()> completed;
    };
    static std::shared_ptr<ReadbackBatch> create(Device &device, qsizetype count);
    bool recordBuffer(Device &device, WGPUCommandEncoder commands, qsizetype index, WGPUBuffer source, quint64 size);
    void start(Device &device);
    std::vector<std::unique_ptr<Result>> results;
    std::atomic<qsizetype> remaining{0};
};
}
#endif
