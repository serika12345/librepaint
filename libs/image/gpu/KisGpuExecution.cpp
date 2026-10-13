/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuExecution_p.h"
#include <limits>
#include <stdexcept>

namespace Krita::Image::Gpu {
namespace {
ExecutionCode code(QRhi::FrameOpResult result)
{
    switch (result) {
    case QRhi::FrameOpSuccess: return ExecutionCode::Success;
    case QRhi::FrameOpSwapChainOutOfDate: return ExecutionCode::SurfaceOutOfDate;
    default: return ExecutionCode::DeviceFailure;
    }
}
}

Execution::Execution(DevicePtr device, qsizetype bytes, qsizetype documents, qsizetype readbackBytes)
{
    if (!device || device->rhi().thread() != QThread::currentThread() ||
        bytes < TileRenderer::TileBytes || documents <= 0 || readbackBytes <= 0 ||
        readbackBytes > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("invalid GPU execution owner or capacity");
    }
    m_d = std::make_unique<Private>(std::move(device), bytes, documents, readbackBytes);
}
Execution::~Execution()
{
    if (m_d->active) cancelFrame();
    if (m_d->device->readbackPending()) m_d->device->rhi().finish();
    retireCompletedReadback();
}

void Execution::retireCompletedReadback()
{
    auto &d = *m_d;
    if (!d.device->readbackPending()) {
        d.readbackProjected.clear();
        d.readbackRendered.clear();
        d.device->releaseCompletedReadback();
    }
}

DocumentAdmission Execution::open(const Snapshot &snapshot)
{
    auto &d = *m_d;
    if (!d.onThread() || !snapshot) return {ExecutionCode::InvalidRequest, 0, QStringLiteral("A document snapshot on the GPU thread is required")};
    for (const auto &document : d.documents) {
        if (document.requested->documentId() == snapshot->documentId()) {
            return {ExecutionCode::InvalidRequest, 0, QStringLiteral("The document is already attached")};
        }
    }
    if (d.documents.size() >= d.maximumDocuments || d.nextHandle == std::numeric_limits<quint64>::max()) {
        return {ExecutionCode::CapacityExceeded, 0, QStringLiteral("GPU document capacity exceeded")};
    }
    const auto handle = ++d.nextHandle;
    Private::Document document;
    document.requested = snapshot;
    d.documents.insert(handle, std::move(document));
    return {ExecutionCode::Success, handle, {}};
}

ExecutionOutcome Execution::request(DocumentHandle handle, const Snapshot &snapshot)
{
    auto &d = *m_d;
    if (!d.onThread() || !snapshot) return {ExecutionCode::InvalidRequest, QStringLiteral("A document snapshot on the GPU thread is required")};
    auto document = d.documents.find(handle);
    if (document == d.documents.end() || snapshot->documentId() != document->requested->documentId()) {
        return {ExecutionCode::InvalidRequest, QStringLiteral("Document handle is no longer active or belongs to another document")};
    }
    if (document->requested == snapshot) return {ExecutionCode::Success, {}};
    if (document->serial == std::numeric_limits<quint64>::max()) return {ExecutionCode::CapacityExceeded, QStringLiteral("GPU document request sequence exhausted")};
    document->requested = snapshot;
    ++document->serial;
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::replaceDocument(DocumentHandle handle, const Snapshot &snapshot)
{
    auto &d = *m_d;
    if (!d.onThread() || !snapshot) return {ExecutionCode::InvalidRequest, QStringLiteral("Restoring requires a document on the GPU thread")};
    auto document = d.documents.find(handle);
    if (document == d.documents.end()) return {ExecutionCode::InvalidRequest, QStringLiteral("Document handle is no longer active")};
    if (document->requested->documentId() == snapshot->documentId())
        return {ExecutionCode::InvalidRequest, QStringLiteral("Restoring requires a new document identity")};
    for (auto other = d.documents.cbegin(); other != d.documents.cend(); ++other) {
        if (other.key() != handle && other->requested->documentId() == snapshot->documentId())
            return {ExecutionCode::InvalidRequest, QStringLiteral("The restored document is already attached")};
    }
    if (document->serial == std::numeric_limits<quint64>::max())
        return {ExecutionCode::CapacityExceeded, QStringLiteral("GPU document request sequence exhausted")};
    document->requested = snapshot;
    ++document->serial;
    document->published.clear();
    document->projected.clear();
    document->rendered.clear();
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::close(DocumentHandle handle)
{
    auto &d = *m_d;
    if (!d.onThread() || !d.documents.remove(handle)) return {ExecutionCode::InvalidRequest, QStringLiteral("Document handle is no longer active")};
    return {ExecutionCode::Success, {}};
}
Snapshot Execution::requested(DocumentHandle handle) const
{
    if (!m_d->onThread()) return {};
    return m_d->documents.value(handle).requested;
}
Snapshot Execution::published(DocumentHandle handle) const
{
    if (!m_d->onThread()) return {};
    return m_d->documents.value(handle).published;
}
qsizetype Execution::pendingDocuments() const
{
    if (!m_d->onThread()) return 0;
    qsizetype result = 0;
    for (const auto &document : m_d->documents) {
        if (!document.rendered || document.published != document.requested) ++result;
    }
    return result;
}

FrameOutcome Execution::beginPresentationFrame(QRhiSwapChain &surface) { return beginFrame(&surface); }
FrameOutcome Execution::beginReadbackFrame() { return beginFrame(nullptr); }
FrameOutcome Execution::beginFrame(QRhiSwapChain *surface)
{
    auto &d = *m_d;
    auto &rhi = d.device->rhi();
    if (!d.onThread() || (surface && surface->rhi() != &rhi)) {
        return {ExecutionCode::InvalidRequest, nullptr, nullptr, QStringLiteral("Frame requires the owning GPU thread and surface")};
    }
    if (d.active || rhi.isRecordingFrame()) return {ExecutionCode::Busy, nullptr, nullptr, QStringLiteral("A GPU frame is already active")};
    if (rhi.isDeviceLost()) return {ExecutionCode::DeviceFailure, nullptr, nullptr, QStringLiteral("GPU device lost")};
    if (surface && (surface->currentPixelSize().isEmpty() || !surface->renderPassDescriptor())) {
        return {ExecutionCode::SurfaceOutOfDate, nullptr, nullptr, QStringLiteral("The native presentation surface requires creation or resizing")};
    }
    QRhiCommandBuffer *commands = nullptr;
    const auto result = surface ? rhi.beginFrame(surface) : rhi.beginOffscreenFrame(&commands);
    if (result != QRhi::FrameOpSuccess) return {code(result), nullptr, nullptr, QStringLiteral("GPU frame could not begin")};
    d.active = true;
    d.attempted = false;
    d.surface = surface;
    d.commands = surface ? surface->currentFrameCommandBuffer() : commands;
    d.stage = {};
    return {ExecutionCode::Success, d.commands, surface ? surface->currentFrameRenderTarget() : nullptr, {}};
}

ProjectionOutcome Execution::project(DocumentHandle handle, QRect region)
{
    auto &d = *m_d;
    if (!d.onThread() || !d.active || d.attempted) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("One projection attempt is permitted in an owning GPU frame")};
    }
    const auto document = d.documents.constFind(handle);
    if (document == d.documents.cend()) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Document handle is no longer active")};
    d.attempted = true;
    const auto rendered = d.renderer->record(*d.commands, document->requested, document->rendered);
    if (rendered.code != RenderOutcome::Recorded) return {rendered.code, {}, rendered.diagnostic};
    const auto projected = d.renderer->compose(*d.commands, rendered.version, region, document->projected);
    if (projected.code != RenderOutcome::Recorded) return projected;
    d.stage = {handle, document->serial, d.generation, rendered.version, projected.version};
    return projected;
}

ExecutionOutcome Execution::submitFrame() { return endFrame(true); }
ExecutionOutcome Execution::cancelFrame() { return endFrame(false); }
ExecutionOutcome Execution::endFrame(bool publish)
{
    auto &d = *m_d;
    if (!d.onThread() || !d.active) return {ExecutionCode::InvalidRequest, QStringLiteral("No owning GPU frame is active")};
    const auto stage = std::move(d.stage);
    const bool superseded = stage.handle && !d.isCurrent(stage);
    const auto flags = !publish || superseded ? QRhi::EndFrameFlags(QRhi::SkipPresent) : QRhi::EndFrameFlags{};
    auto &rhi = d.device->rhi();
    const auto result = d.surface ? rhi.endFrame(d.surface, flags) : rhi.endOffscreenFrame(flags);
    d.active = false;
    d.attempted = false;
    d.surface = nullptr;
    d.commands = nullptr;
    retireCompletedReadback();
    if (result != QRhi::FrameOpSuccess) return {code(result), QStringLiteral("GPU frame submission failed")};
    if (publish && stage.handle) {
        if (!d.isCurrent(stage)) return {ExecutionCode::InvalidRequest, QStringLiteral("GPU frame was superseded or its document closed")};
        auto &document = d.documents[stage.handle];
        document.rendered = stage.rendered;
        document.projected = stage.projected;
        document.published = stage.rendered->document();
    }
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::replaceDevice(DevicePtr device)
{
    auto &d = *m_d;
    if (!d.onThread() || !device || device->rhi().thread() != QThread::currentThread()) {
        return {ExecutionCode::InvalidRequest, QStringLiteral("Replacement requires a device on the owning thread")};
    }
    if (d.active || d.device->rhi().isRecordingFrame() || device->rhi().isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("GPU replacement requires submitted frames")};
    if (d.generation == std::numeric_limits<quint64>::max()) return {ExecutionCode::CapacityExceeded, QStringLiteral("GPU generation exhausted")};
    auto renderer = std::make_unique<TileRenderer>(device, d.byteBudget);
    if (d.device->readbackPending()) d.device->rhi().finish();
    d.device->releaseCompletedReadback();
    d.readbackProjected.clear();
    d.readbackRendered.clear();
    for (auto &document : d.documents) {
        document.projected.clear();
        document.rendered.clear();
    }
    d.renderer = std::move(renderer);
    d.device = std::move(device);
    ++d.generation;
    return {ExecutionCode::Success, {}};
}
DevicePtr Execution::device() const { return m_d->onThread() ? m_d->device : DevicePtr{}; }
quint64 Execution::generation() const { return m_d->onThread() ? m_d->generation : 0; }
qsizetype Execution::residentBytes() const { return m_d->onThread() ? m_d->renderer->residentBytes() : 0; }
}
