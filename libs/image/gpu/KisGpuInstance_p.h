/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_INSTANCE_P_H
#define KIS_GPU_INSTANCE_P_H

#include "kritaimagegpuinstance_export.h"
#include <webgpu/wgpu.h>

namespace KisGpuTileStorage {
/** Borrowed process-wide API instance, shared across executables and plugins. */
KRITAIMAGEGPUINSTANCE_EXPORT WGPUInstance processInstance();
}

#endif
