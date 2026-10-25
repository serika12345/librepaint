/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_IMAGE_P_H
#define KIS_GPU_PAINT_IMAGE_P_H

#include "KisGpuPaintImage.h"
#include "KisGpuExecution.h"
#include <QHash>

namespace LibrePaint::Image::Gpu {
/** Temporary GPU input for one retained-mask edit. The image starts from the
 * frozen coverage and is discarded after its alpha becomes one mask version.
 */
struct MaskStroke
{
    PaintImageSP image;
    LayerId layer = 0;
    LayerId imageLayer = 0;
    OperationId id;
};

struct PaintImage::Private
{
    std::shared_ptr<Execution> retainedExecution;
    Execution &execution;
    std::unique_ptr<OperationDocument> document;
    Limits limits;
    quint64 handle = 0;
    ExecutionOutcome lastRequest{ExecutionCode::Success, {}};
    QHash<QUuid, QWeakPointer<LayerPixelReference>> layerPixels{};
    std::unique_ptr<MaskStroke> maskStroke;
};
}
#endif
