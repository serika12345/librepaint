/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include "KisGpuDevice.h"
#include <algorithm>

using namespace KisGpuTileStorage;

namespace {
struct TextureCopy {
    WGPUTexelCopyBufferInfo input{};
    WGPUOrigin3D origin{};
    WGPUExtent3D extent{};
    quint64 bytes() const { return quint64(extent.width) * extent.height * 4; }
    void encode(WGPUCommandEncoder encoder, WGPUTexture texture) const {
        WGPUTexelCopyTextureInfo output{};
        output.texture = texture;
        output.aspect = WGPUTextureAspect_All;
        output.origin = origin;
        wgpuCommandEncoderCopyBufferToTexture(encoder, &input, &output, &extent);
    }
};

std::optional<TextureCopy> textureCopy(QRect bounds, Coordinate coordinate, KisGpuTileStore::TileView source)
{
    const qint64 tileLeft = qint64(coordinate.first) * 64, tileTop = qint64(coordinate.second) * 64;
    const qint64 x0 = std::max(qint64(bounds.x()), tileLeft), y0 = std::max(qint64(bounds.y()), tileTop);
    const qint64 x1 = std::min(qint64(bounds.x()) + bounds.width(), tileLeft + 64);
    const qint64 y1 = std::min(qint64(bounds.y()) + bounds.height(), tileTop + 64);
    if (x0 >= x1 || y0 >= y1) return {};
    TextureCopy result;
    result.input.buffer = source.buffer;
    result.input.layout.offset = source.offset + ((y0 - tileTop) * 64 + x0 - tileLeft) * 4;
    result.input.layout.bytesPerRow = 64 * 4;
    result.input.layout.rowsPerImage = 64;
    result.origin = {quint32(x0 - bounds.x()), quint32(y0 - bounds.y()), 0};
    result.extent = {quint32(x1 - x0), quint32(y1 - y0), 1};
    return result;
}
}

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

KisGpuTileStore::TextureSnapshot KisGpuTileStore::textureSnapshot(const Version &source, QRect bounds, TextureSnapshot &&previous)
{
    const auto fresh = [&] {
        auto result = textureSnapshot(source, bounds);
        if (result.error == Error::None) previous = {};
        return result;
    };
    if (!previous.d || previous.d->owner != d->state || previous.bounds() != bounds
        || previous.completion.status() != Status::Succeeded) return fresh();
    TextureSnapshot result;
    if (!source.d || source.d->owner != d->state) { result.error = Error::InvalidVersion; return result; }
    if (!deviceAvailable()) { result.error = Error::DeviceLost; return result; }
    decltype(previous.d->sources) sources;
    std::vector<TextureCopy> copies;
    bool clearing = false;
    for (const auto &entry : source.d->tiles) {
        const auto copy = textureCopy(bounds, entry.first, {entry.second.allocation->buffer, entry.second.offset});
        if (!copy) continue;
        sources.emplace(entry.first, TextureData::SourceTile{entry.second.allocation, entry.second.offset});
        const auto old = previous.d->sources.find(entry.first);
        if (old == previous.d->sources.end() || old->second.allocation.lock() != entry.second.allocation
            || old->second.offset != entry.second.offset) copies.push_back(*copy);
    }
    for (const auto &entry : previous.d->sources) if (!sources.count(entry.first)) {
        copies.push_back(*textureCopy(bounds, entry.first, {}));
        clearing = true;
    }
    if (copies.empty()) return std::move(previous);
    if (previous.d.use_count() != 1) return fresh();
    if (d->pending.size() >= d->maximumPending) { result.error = Error::QueueFull; return result; }
    if ((clearing ? TileBytes : 0) > d->availableForOperation(0)) { result.error = Error::BudgetExceeded; return result; }
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    std::shared_ptr<Allocation> zero;
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    if (clearing) {
        zero = std::make_shared<Allocation>(d->state, TileBytes, WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        wgpuCommandEncoderClearBuffer(encoder.value, zero->buffer, 0, TileBytes);
    }
    quint64 copiedBytes = 0;
    for (auto &copy : copies) {
        if (!copy.input.buffer) copy.input.buffer = zero->buffer;
        copy.encode(encoder.value, previous.d->texture);
        copiedBytes += copy.bytes();
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commands(wgpuCommandEncoderFinish(encoder.value, nullptr));
    result.d = std::move(previous.d);
    previous = {};
    result.d->sources = std::move(sources);
    Private::Pending pending{result.completion.d, source, {}, zero, {}, {}};
    pending.texture = result.d;
    d->submit(std::move(pending), commands.value, {}, {}, 0, 0);
    d->statistics.textureCopyBytes += copiedBytes;
    errors.submitted = true;
    return result;
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
    d->statistics.imageTextureAllocationBytes += bytes;
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
    quint64 copiedBytes = 0;
    for (const auto &entry : source.d->tiles) {
        const auto copy = textureCopy(bounds, entry.first, {entry.second.allocation->buffer, entry.second.offset});
        if (!copy) continue;
        copy->encode(encoder.value, result.d->texture);
        result.d->sources.emplace(entry.first, TextureData::SourceTile{entry.second.allocation, entry.second.offset});
        copiedBytes += copy->bytes();
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
