/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include "KisGpuDevice.h"
#include <algorithm>

using namespace KisGpuTileStorage;

KisGpuTileStore::TextureData::TextureData(std::shared_ptr<DeviceState> state, QRect rectangle)
    : owner(std::move(state)), bounds(rectangle), bytes(quint64(bounds.width()) * bounds.height() * 4)
{
    memory = NativeDevice::reserveMemory(owner->nativeOwner, bytes);
    if (!memory) throw std::runtime_error("GPU texture budget was not reserved before allocation");
    WGPUTextureDescriptor descriptor{};
    descriptor.size = {quint32(bounds.width()), quint32(bounds.height()), 1};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.mipLevelCount = 1;
    descriptor.sampleCount = 1;
    descriptor.usage = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst
        | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
    texture = wgpuDeviceCreateTexture(owner->device, &descriptor);
    if (!texture) throw std::runtime_error("Cannot allocate GPU image texture");
    owner->residentBytes.fetch_add(bytes);
}

KisGpuTileStore::TextureData::~TextureData()
{
    wgpuTextureRelease(texture);
    owner->residentBytes.fetch_sub(bytes);
}

WGPUTexture KisGpuTileStore::TextureSnapshot::texture() const { return d ? d->texture : nullptr; }
QRect KisGpuTileStore::TextureSnapshot::bounds() const { return d ? d->bounds : QRect(); }
bool KisGpuTileStore::TextureSnapshot::usesDevice(const KisGpuDevice &device) const
{
    return d && d->owner->nativeOwner == device.d;
}

KisGpuTileStore::TextureSnapshot KisGpuTileStore::textureSnapshot(const Version &source, QRect bounds)
{
    TextureSnapshot result;
    if (!source.d || source.d->owner != d->state) {
        result.error = Error::InvalidVersion;
        return result;
    }
    if (!deviceAvailable()) {
        result.error = Error::DeviceLost;
        return result;
    }
    if (bounds.isEmpty()) {
        result.error = Error::InvalidCommand;
        return result;
    }
    if (d->pending.size() >= d->maximumPending) {
        result.error = Error::QueueFull;
        return result;
    }
    const quint64 bytes = quint64(bounds.width()) * bounds.height() * 4;
    if (quint32(bounds.width()) > d->limits.maxTextureDimension2D
        || quint32(bounds.height()) > d->limits.maxTextureDimension2D || bytes > d->availableForOperation(0)) {
        result.error = Error::BudgetExceeded;
        return result;
    }
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    result.d = std::make_shared<TextureData>(d->state, bounds);
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    Handle<WGPUTextureView, wgpuTextureViewRelease> view(wgpuTextureCreateView(result.d->texture, nullptr));
    WGPURenderPassColorAttachment attachment{};
    attachment.view = view.value;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor clear{};
    clear.colorAttachmentCount = 1;
    clear.colorAttachments = &attachment;
    {
        Handle<WGPURenderPassEncoder, wgpuRenderPassEncoderRelease> pass(
            wgpuCommandEncoderBeginRenderPass(encoder.value, &clear));
        wgpuRenderPassEncoderEnd(pass.value);
    }
    const qint64 left = bounds.x(), top = bounds.y();
    const qint64 right = left + bounds.width(), bottom = top + bounds.height();
    quint64 copiedBytes = 0;
    for (const auto &entry : source.d->tiles) {
        const qint64 tileLeft = qint64(entry.first.first) * 64, tileTop = qint64(entry.first.second) * 64;
        const qint64 x0 = std::max(left, tileLeft), y0 = std::max(top, tileTop);
        const qint64 x1 = std::min(right, tileLeft + 64), y1 = std::min(bottom, tileTop + 64);
        if (x0 >= x1 || y0 >= y1) continue;
        WGPUTexelCopyBufferInfo input{};
        input.buffer = entry.second.allocation->buffer;
        input.layout.offset = entry.second.offset + ((y0 - tileTop) * 64 + x0 - tileLeft) * 4;
        input.layout.bytesPerRow = 64 * 4;
        input.layout.rowsPerImage = 64;
        WGPUTexelCopyTextureInfo output{};
        output.texture = result.d->texture;
        output.aspect = WGPUTextureAspect_All;
        output.origin = {quint32(x0 - left), quint32(y0 - top), 0};
        const WGPUExtent3D extent{quint32(x1 - x0), quint32(y1 - y0), 1};
        wgpuCommandEncoderCopyBufferToTexture(encoder.value, &input, &output, &extent);
        copiedBytes += quint64(x1 - x0) * (y1 - y0) * 4;
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commands(
        wgpuCommandEncoderFinish(encoder.value, nullptr));
    Private::Pending pending{result.completion.d, source, {}, {}, {}, {}};
    pending.texture = result.d;
    d->submit(std::move(pending), commands.value, {}, {}, 0, 0);
    d->statistics.textureCopyBytes += copiedBytes;
    errors.submitted = true;
    return result;
}
