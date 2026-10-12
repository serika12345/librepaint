/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_VALUES_P_H
#define KIS_GPU_OPERATION_VALUES_P_H

#include "KisGpuOperationDocument.h"

namespace Krita::Image::Gpu::Detail {
inline OperationId operationId(const LayerOperation &value)
{ return std::visit([](const auto &operation) { return operation->id; }, value); }
inline LayerId operationLayer(const LayerOperation &value)
{ return std::visit([](const auto &operation) { return operation->layer; }, value); }
inline quint64 operationVersion(const LayerOperation &value)
{ return std::visit([](const auto &operation) { return operation->version; }, value); }
inline const void *operationPointer(const LayerOperation &value)
{ return std::visit([](const auto &operation) -> const void * { return operation.data(); }, value); }
inline qsizetype operationSamples(const LayerOperation &value)
{
    const auto paint = std::get_if<PaintSnapshot>(&value);
    return paint ? (*paint)->sampleCount : 0;
}
inline qsizetype operationRasterBytes(const LayerOperation &value)
{
    const auto write = std::get_if<TileWriteSnapshot>(&value);
    return write ? (*write)->tiles.size() * RgbaTile::ByteCount : 0;
}
}
#endif
