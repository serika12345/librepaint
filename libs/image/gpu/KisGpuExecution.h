/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_EXECUTION_H
#define KIS_GPU_EXECUTION_H

#include "KisGpuTileRenderer.h"

class QRhiRenderTarget;
class QRhiSwapChain;

namespace Krita::Image::Gpu {

using DocumentHandle = quint64;
enum class ExecutionCode { Success, InvalidRequest, CapacityExceeded, Busy, DeviceFailure, SurfaceOutOfDate };

struct ExecutionOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    QString diagnostic;
};
struct DocumentAdmission
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    DocumentHandle handle = 0;
    QString diagnostic;
};
struct FrameOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    QRhiCommandBuffer *commands = nullptr;
    QRhiRenderTarget *target = nullptr;
    QString diagnostic;
};

/** Full tile bytes in straight, byte-ordered RGBA8; layer zero denotes the projection. */
struct PixelTile
{
    LayerId layer = 0;
    QPoint coordinate;
    QByteArray rgba;
};
struct PixelReadbackOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    Snapshot document;
    quint64 generation = 0;
    QRect region;
    QList<PixelTile> tiles;
    QString diagnostic;
};

/**
 * Owns the bounded document requests and frame submission on one GPU queue.
 * All calls and destruction use the device thread. Each attached document has
 * one latest immutable request; the logical owner retains accepted input and
 * history. At most one frame and one projection attempt are active at a time.
 *
 * Presentation frames join drawing, composition and display without CPU pixel
 * transfers or completion waits. Readback frames serve synchronous external
 * reads and finish at submission. Command buffers and presentation targets are
 * borrowed until submitFrame()/cancelFrame(); keep the swap chain valid then.
 * A frame publishes its GPU version only after successful native submission
 * while its document handle and request remain current. Superseded and closed
 * documents suppress presentation. Cancellation preserves the published state.
 */
class KRITAIMAGE_EXPORT Execution
{
public:
    explicit Execution(DevicePtr device, qsizetype byteBudget = 128 * 1024 * 1024,
                       qsizetype maximumDocuments = 16,
                       qsizetype maximumReadbackBytes = 64 * 1024 * 1024);
    ~Execution();
    Execution(const Execution &) = delete;
    Execution &operator=(const Execution &) = delete;

    /** One attachment per document; handle zero denotes rejection. */
    DocumentAdmission open(const Snapshot &document);
    ExecutionOutcome request(DocumentHandle handle, const Snapshot &document);
    ExecutionOutcome close(DocumentHandle handle);
    Snapshot requested(DocumentHandle handle) const;
    Snapshot published(DocumentHandle handle) const;
    qsizetype pendingDocuments() const;

    FrameOutcome beginPresentationFrame(QRhiSwapChain &surface);
    FrameOutcome beginReadbackFrame();
    ProjectionOutcome project(DocumentHandle handle, QRect region);
    ExecutionOutcome submitFrame();
    ExecutionOutcome cancelFrame();

    /**
     * Synchronous external read of a retained version belonging to an attached
     * document. Preserves newer requests and publication. Reads selected layers,
     * including hidden ones, and optionally the visible projection. Returns full
     * sparse tiles intersecting the document-clipped region; absent tiles are
     * transparent. The byte limit counts distinct transferred tile buffers.
     * Rejection returns no partial pixels. CPU values outlive the device.
     */
    PixelReadbackOutcome readPixels(DocumentHandle handle, const Snapshot &document,
                                   QRect region, const QList<LayerId> &layers = {},
                                   bool includeProjection = true);

    /** Rebuilds accepted documents lazily on a new device; active frames reject replacement. */
    ExecutionOutcome replaceDevice(DevicePtr device);
    DevicePtr device() const;
    quint64 generation() const;
    qsizetype residentBytes() const;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
    FrameOutcome beginFrame(QRhiSwapChain *surface);
    ExecutionOutcome endFrame(bool publish);
    void retireCompletedReadback();
};
}
#endif
