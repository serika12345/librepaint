/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_EXECUTION_H
#define KIS_GPU_EXECUTION_H

#include "KisGpuTileRenderer.h"
#include <QImage>
#include <future>


namespace LibrePaint::Image::Gpu {

class PresentationSurface;
using DocumentHandle = quint64;
enum class ExecutionCode { Success, InvalidRequest, CapacityExceeded, Busy, DeviceFailure, SurfaceOutOfDate };
enum class PixelReadArea { Canvas, LayerCoordinates };

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
    WGPUCommandEncoder commands = nullptr;
    WGPUTextureView target = nullptr;
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
struct ImageReadbackOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    Snapshot document;
    quint64 generation = 0;
    QImage image;
    QString diagnostic;
};
struct PixelBytesOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    Snapshot document;
    quint64 generation = 0;
    QRect region;
    QByteArray bytes;
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
 * reads; submission completes their requested CPU transfers. Command buffers and presentation targets are
 * borrowed until submitFrame()/cancelFrame(); keep the presentation surface valid then.
 * A frame publishes its GPU version only after successful native submission
 * while its document handle and request remain current. Superseded and closed
 * documents suppress presentation. Cancellation preserves the published state.
 */
class KRITAGPUIMAGE_EXPORT Execution
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
    /** Atomically restores a fresh logical identity without consuming another slot.
     * Keeps the handle, clears its derived versions and invalidates recorded
     * frames and reads from the previous document identity.
     */
    ExecutionOutcome replaceDocument(DocumentHandle handle, const Snapshot &document);
    ExecutionOutcome close(DocumentHandle handle);
    Snapshot requested(DocumentHandle handle) const;
    Snapshot published(DocumentHandle handle) const;
    qsizetype pendingDocuments() const;

    FrameOutcome beginPresentationFrame(PresentationSurface &surface);
    FrameOutcome beginReadbackFrame();
    ProjectionOutcome project(DocumentHandle handle, QRect region);
    ExecutionOutcome submitFrame();
    ExecutionOutcome cancelFrame();

    /**
     * Synchronous external read of a retained version belonging to an attached
     * document. Preserves newer requests and publication. Reads selected layers,
     * including hidden ones, and optionally the visible projection. Returns full
     * sparse tiles intersecting the document-clipped region by default.
     * LayerCoordinates preserves off-canvas layer tiles; the projection remains
     * clipped to the canvas. Absent layer tiles use the pinned layer's defaultPixel.
     * The byte limit counts distinct transferred tile buffers.
     * Rejection returns no partial pixels. CPU values outlive the device.
     */
    PixelReadbackOutcome readPixels(DocumentHandle handle, const Snapshot &document,
                                   QRect region, const QList<LayerId> &layers = {},
                                   bool includeProjection = true, PixelReadArea area = PixelReadArea::Canvas);
    /** Submits a fixed-version read without waiting for GPU completion.
     * pollReadbacks() drives native callbacks on the GPU thread. An admitted
     * read completes independently of later edits or document closure; its CPU
     * result outlives execution and the device. Rejections are ready futures
     * with no partial pixels. Discarding a future retains native callback storage
     * until completion. The same transfer budget as readPixels() applies.
     */
    std::future<PixelReadbackOutcome> readPixelsAsync(DocumentHandle handle, const Snapshot &document,
                                   QRect region, const QList<LayerId> &layers = {},
                                   bool includeProjection = true, PixelReadArea area = PixelReadArea::Canvas);
    /** Nonblocking callback processing; Busy denotes unfinished native reads. */
    ExecutionOutcome pollReadbacks();

    /** Packed straight RGBA8 rows for the exact requested rectangle.
     * Layer zero reads the visible projection, with transparent pixels outside
     * the canvas. Other layers preserve hidden, off-canvas and implicit pixels.
     * The byte limit counts output pixels; transfers use one tile at a time.
     * Rejection returns no partial bytes. Requests and publication stay current.
     */
    PixelBytesOutcome readRgbaPixels(DocumentHandle handle, const Snapshot &document,
                                     QRect region, LayerId layer, qsizetype maximumBytes);

    /** Complete sparse layer pixels, including hidden and off-canvas tiles.
     * Implicit pixels stay in the returned document's layer defaultPixel.
     * The output byte limit bounds CPU tiles; each transfer uses one tile.
     * Complete reads retire derived display caches after GPU completion while
     * preserving requests and publication. External GPU pins count against the
     * shared capacity. Rejection returns no partial result.
     */
    PixelReadbackOutcome readLayerPixels(DocumentHandle handle, const Snapshot &document,
                                        LayerId layer, qsizetype maximumBytes);
    /** Complete straight RGBA8/sRGB projection, streamed into a bounded CPU image.
     * Uses the same cache retirement and publication rules as readLayerPixels().
     */
    ImageReadbackOutcome readProjectionImage(DocumentHandle handle, const Snapshot &document,
                                            qsizetype maximumBytes);

    /** Rebuilds accepted documents lazily on a new device; active frames reject replacement. */
    ExecutionOutcome replaceDevice(DevicePtr device);
    DevicePtr device() const;
    quint64 generation() const;
    qsizetype residentBytes() const;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
    FrameOutcome beginFrame(PresentationSurface *surface);
    ExecutionOutcome endFrame(bool publish, bool waitForReadback = true);
    void retireCompletedReadback();
    ExecutionOutcome retireDisplayCaches();
};
}
#endif
