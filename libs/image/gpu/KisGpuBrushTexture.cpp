/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <limits>

using namespace KisGpuTileStorage;

KisGpuTileStore::BrushTextureResult KisGpuTileStore::uploadBrushTexture(QSize size, const QByteArray &alpha)
{
    if (!deviceAvailable()) return {Error::DeviceLost, {}, {}};
    const qint64 count = qint64(size.width()) * size.height();
    if (size.width() <= 0 || size.height() <= 0 || count != alpha.size()) return {Error::InvalidCommand, {}, {}};
    const quint64 bytes = (quint64(count) + 3) / 4 * 4;
    if (bytes > std::min<quint64>({d->availableForOperation(0), d->limits.maxBufferSize,
        d->limits.maxStorageBufferBindingSize, quint64(std::numeric_limits<quint32>::max())})) {
        return {Error::BudgetExceeded, {}, {}};
    }
    if (d->pending.size() >= d->maximumPending) return {Error::QueueFull, {}, {}};
    BrushTextureResult result;
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto allocation = std::make_shared<Allocation>(d->state, bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto data = std::make_shared<BrushTextureData>(BrushTextureData{allocation, result.completion.d, size});
    QByteArray packed = alpha;
    packed.resize(qsizetype(bytes), '\0');
    Private::Recording encoder(d->state, 0);
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> buffer(encoder.finish());
    Private::Pending pending{result.completion.d, {}, {}, allocation, {}, {}};
    pending.brushTexture = data;
    d->submit(std::move(pending), buffer.value, {}, {}, 0, 0, packed);
    result.texture.d = std::move(data);
    errors.submitted = true;
    return result;
}
