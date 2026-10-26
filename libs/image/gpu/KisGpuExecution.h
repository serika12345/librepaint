/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_EXECUTION_H
#define KIS_GPU_EXECUTION_H

#include "KisGpuTileRenderer.h"
#include <QImage>
#include <array>
#include <future>
#include <optional>


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
    std::optional<bool> containsTransparency{};
    std::optional<QRect> bounds{};
    /** Explicitly written regions retained by complete-layer reads for lossless saving. */
    QList<QRect> writtenAreas{};
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
struct ColorSampleOutcome
{
    ExecutionCode code = ExecutionCode::InvalidRequest;
    Snapshot document;
    quint64 generation = 0;
    std::array<quint8, 4> rgba{};
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
     * document. Completes earlier admitted native reads before starting this read;
     * an unsubmitted frame remains Busy. Preserves newer requests and publication.
     * Reads selected layers,
     * including hidden ones, and optionally the visible projection. Returns full
     * sparse tiles intersecting the document-clipped region by default.
     * LayerCoordinates preserves off-canvas layer tiles; the projection remains
     * clipped to the canvas. Absent tiles use the pinned document's projectionDefaultPixels().
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
    /** Canonical alpha-weighted color of a fixed layer or the canvas projection.
     * Radius 1 selects the center; radii 2..900 select integer points strictly
     * inside a disk of radius (radius - 1). Alpha averages all sample positions;
     * RGB is weighted by alpha. Integer ties round upward. Zero aggregate alpha
     * returns zero RGBA. Layer zero samples transparent pixels beyond the canvas;
     * other layers retain off-canvas and implicit pixels. Transfers use 32 bytes,
     * independent of the sampled area. Thread, asynchronous completion, lifetime
     * and rejection follow readPixelsAsync(); requests/publication stay current.
     */
    std::future<ColorSampleOutcome> readColorSampleAsync(DocumentHandle handle, const Snapshot &document,
                                                        LayerId layer, QPoint point, int radius);
    /** Fixed-version projection alpha classification without transferring tile pixels.
     * A successful result contains the boolean and no tiles. Canvas clipping,
     * asynchronous completion, lifetime, and rejection follow readPixelsAsync().
     */
    std::future<PixelReadbackOutcome> readTransparencyAsync(DocumentHandle handle, const Snapshot &document, QRect region);
    /** Exact nontransparent layer bounds, including off-canvas pixels. An opaque
     * implicit pixel selects nondefault pixels instead; the caller supplies its
     * default bounds. Layer zero measures the canvas-clipped projection.
     * Streams one tile at a time, transfers sixteen bytes per summary, retains
     * the version, and preserves requests and publication.
     * Rejection and thread ownership follow readPixels().
     * includeTransparentColors measures every nondefault RGBA value for content-preserving moves.
     */
    PixelReadbackOutcome readBounds(DocumentHandle handle, const Snapshot &document, LayerId layer,
                                   bool includeTransparentColors = false);
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
     * Implicit pixels use the returned document's projectionDefaultPixels().
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

    /** Nearest-sample thumbnail of a fixed layer or the canvas-clipped projection.
     * Source pixels stay on the GPU. Only bounded chunks of the requested image
     * are transferred. Upscaling retains the native thumbnail's transparent padding.
     * Output and native working storage are bounded; rejection returns no image
     * and preserves requests, publication and editing state.
     * includeLayerTileExtent unions the source with retained tile bounds for
     * coarse history previews, including implicit projection padding.
     */
    ImageReadbackOutcome readThumbnail(DocumentHandle handle, const Snapshot &document,
                                      LayerId layer, QRect source, QSize size, qsizetype maximumBytes,
                                      bool includeLayerTileExtent = false);

    /** Resizes a fixed projection with the native document-preview rules.
     * Bicubic passes retain alpha-weighted integer rounding and content cropping.
     * Reductions below 1/256 use Qt area/bilinear rules. When the cropped source
     * fits inside the requested result, Qt enlarges the bounded returned pixels.
     * GPU working storage and transfers are bounded; failed reads return no image
     * and preserve requests, publication and document state.
     */
    ImageReadbackOutcome readResizedProjectionImage(DocumentHandle handle, const Snapshot &document,
                                                    QSize scaledCanvas, qsizetype maximumBytes);

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
    PixelReadbackOutcome readLayerTileExtent(DocumentHandle handle, const Snapshot &document, LayerId layer);
    PixelReadbackOutcome readBoundsImpl(DocumentHandle handle, const Snapshot &document, LayerId layer,
                                        bool includeTransparentColors, bool sparseProjection);
    std::future<PixelReadbackOutcome> readPixelsAsyncImpl(DocumentHandle handle, const Snapshot &document,
                                   QRect region, const QList<LayerId> &layers, bool includeProjection,
                                   PixelReadArea area, bool transparencyOnly, bool boundsOnly = false,
                                   bool includeTransparentColors = false, bool tileExtentOnly = false);
};
}
#endif
