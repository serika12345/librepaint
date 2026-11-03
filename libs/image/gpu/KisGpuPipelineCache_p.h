/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PIPELINE_CACHE_P_H
#define KIS_GPU_PIPELINE_CACHE_P_H
#include "KisGpuNativeResources_p.h"
#include <QByteArray>
#include <QHash>
#include <memory>
namespace LibrePaint::Image::Gpu::Detail {
/** Device-owned programs contain native handles and never retain a DevicePtr. */
struct PipelineCache {
    QHash<QByteArray, std::shared_ptr<NativeShader>> modules;
    QHash<QByteArray, std::shared_ptr<NativeComputePipeline>> pipelines;
};
}
#endif
