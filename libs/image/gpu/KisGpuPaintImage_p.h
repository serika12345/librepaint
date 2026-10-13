/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_IMAGE_P_H
#define KIS_GPU_PAINT_IMAGE_P_H

#include "KisGpuPaintImage.h"
#include "KisGpuExecution.h"

namespace Krita::Image::Gpu {
struct PaintImage::Private
{
    Execution &execution;
    std::unique_ptr<OperationDocument> document;
    Limits limits;
    quint64 handle = 0;
    ExecutionOutcome lastRequest{ExecutionCode::Success, {}};
};
}
#endif
