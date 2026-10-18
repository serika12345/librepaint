/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_IMAGE_H
#define KIS_GPU_PAINT_IMAGE_H

#include "KisGpuOperationDocument.h"
#include <kritagpuimage_export.h>
#include <QObject>
#include <QRect>
#include <memory>
#include <utility>

namespace LibrePaint::Image::Gpu {
class Execution;
struct ExecutionOutcome;
struct PixelBytesOutcome;
class PaintImage;

/** GUI-thread pixel identity shared by views of one layer. Immutable versions
 * stay readable through removal and discarded history on the owning image.
 */
class KRITAGPUIMAGE_EXPORT LayerPixelReference
{
public:
    LayerId layer() const { return m_layer; }
    Snapshot version() const { return m_version; }
private:
    friend class PaintImage;
    LayerPixelReference(LayerId layer, QUuid uuid, Snapshot version)
        : m_layer(layer), m_uuid(uuid), m_version(std::move(version)) {}
    LayerId m_layer;
    QUuid m_uuid;
    Snapshot m_version;
};
using LayerPixelReferenceSP = QSharedPointer<LayerPixelReference>;

/** Shared editable image, history, notifications and GPU attachment.
 * Calls and destruction use the GUI/device thread. Shared execution construction
 * keeps the execution alive; borrowed execution outlives every image reference.
 * Accepted edits update the GPU
 * request before changed() is emitted; admission failures retain logical edits.
 * Invalid construction throws std::invalid_argument, and exhausted attachment
 * capacity throws std::length_error. The last image reference closes its handle.
 */
class KRITAGPUIMAGE_EXPORT PaintImage : public QObject
{
    Q_OBJECT
public:
    explicit PaintImage(Execution &execution, QSize size, Limits limits = {});
    explicit PaintImage(Execution &execution, const RasterImage &image, Limits limits = {});
    explicit PaintImage(std::shared_ptr<Execution> execution, QSize size, Limits limits = {});
    explicit PaintImage(std::shared_ptr<Execution> execution, const RasterImage &image, Limits limits = {});
    ~PaintImage() override;
    /** Borrowed logical owner; successful recovery replaces this reference. */
    const OperationDocument &operations() const;
    Limits limits() const;
    quint64 handle() const;
    /** Borrowed GUI-thread execution owner; shared construction pins its lifetime. */
    Execution &execution() const;
    /** Independent committed image with empty, saved history on the same execution.
     * Immutable input and raster blocks may be shared. Operation identities are
     * fresh; preserveLayerUuids retains public layer identities for snapshots.
     * Returns null off-thread; attachment capacity throws std::length_error.
     */
    QSharedPointer<PaintImage> clone(bool preserveLayerUuids = false) const;
    /** Clones logical values into the destination execution, with the same guarantees. */
    QSharedPointer<PaintImage> cloneTo(Execution &destination, bool preserveLayerUuids = false) const;
    /** Uses the destination image's execution lifetime, including shared ownership. */
    QSharedPointer<PaintImage> cloneTo(const PaintImage &destination, bool preserveLayerUuids = false) const;
    /** Independent editable copy of retained layer pixels on this execution.
     * Shares immutable input and compressed blocks, with fresh operation identity
     * and empty saved history. Removed layers use their last retained version.
     * Returns null for foreign, unavailable or off-thread references; attachment
     * capacity throws std::length_error.
     */
    QSharedPointer<PaintImage> cloneLayerPixels(const LayerPixelReferenceSP &pixels) const;
    ExecutionOutcome lastRequestOutcome() const;
    /** Synchronous canonical pixels from the current committed image version. */
    PixelBytesOutcome readRgbaPixels(LayerId layer, QRect region, qsizetype maximumBytes) const;
    /** Retains a live layer's pixel identity; null for unavailable/off-thread requests.
     * The image weakly tracks references and keeps their last committed pixels.
     */
    LayerPixelReferenceSP retainLayerPixels(LayerId layer);

    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 seed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    Result flushSmoothing(OperationId id);
    Result updateShape(OperationId id, const Shape &shape);
    Result updateFigure(OperationId id, const Shape &shape, const FigureStyle &style);
    /** Complete figure adoption publishes one committed version and one notification. */
    Result paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                       const Shape &shape, const FigureStyle &style, quint64 randomSeed);
    Result commitStroke(OperationId id);
    Result cancelStroke(OperationId id);
    Result replaceTiles(LayerId layer, const QList<RgbaTile> &tiles);
    /** Replaces nonempty packed pixel rectangles in one history entry. */
    Result replaceRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes);
    /** Direct packed RGBA8 writes preserve history, notifications and fixed versions.
     * Ignores surplus input bytes and accepts empty rectangles. Transfers and
     * compression finish before logical adoption; rejected writes are atomic.
     */
    Result writeRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes);
    /** Direct writes also reach retained removed layers and their surviving history.
     * The current scene and its history position stay unchanged for detached layers.
     */
    Result writeRgbaPixels(const LayerPixelReferenceSP &pixels, QRect region, const QByteArray &bytes);
    /** Direct background writes request pixels without adding history or edit notifications. */
    Result writeProjectionBackground(const std::array<quint8, 4> &rgba);
    Result writeLayerName(LayerId layer, const QString &name);
    Result writeLayerVisible(LayerId layer, bool visible);
    Result writeLayerOpacity(LayerId layer, quint8 opacity);
    Result writeLayerLocked(LayerId layer, bool locked);
    Result writeLayerPinnedToTimeline(LayerId layer, bool pinned);
    LayerId addLayer(const QString &name);
    LayerId addLayer(const QString &name, qsizetype index, bool pinnedToTimeline);
    /** Inserts validated committed inputs as one history entry, preserving the
     * supplied fresh public identity. Rejections leave history unchanged.
     */
    LayerId insertLayer(const OperationDocument &source, LayerId layer,
                        const RasterLayer &properties, qsizetype index);
    /** Uses retained pixels, including removed layers, without reading back GPU pixels. */
    LayerId insertLayer(const PaintImage &source, const LayerPixelReferenceSP &pixels,
                        const RasterLayer &properties, qsizetype index);
    Result selectLayer(LayerId layer);
    Result selectLayers(const QList<LayerId> &layers, LayerId active);
    Result duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource);
    Result setLayerVisible(LayerId layer, bool visible);
    Result setLayerVisible(LayerId layer, bool visible, PropertyHistory history);
    Result setLayerOpacity(LayerId layer, quint8 opacity);
    Result setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history);
    Result setLayerLocked(LayerId layer, bool locked);
    Result setLayerLocked(LayerId layer, bool locked, PropertyHistory history);
    Result setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerChannelMask(LayerId layer, quint8 mask);
    Result renameLayer(LayerId layer, const QString &name);
    Result renameLayer(LayerId layer, const QString &name, PropertyHistory history);
    Result moveLayer(LayerId layer, qsizetype index);
    Result reorderLayers(const QList<LayerId> &order);
    Result removeLayer(LayerId layer);
    Result removeLayers(const QList<LayerId> &layers, const QString &replacementName = {});
    /** Clears whole layers atomically while retained versions remain readable. */
    Result clearLayers(const QList<LayerId> &layers);
    /** Publishes one canvas-bounded color fill before notifying attached consumers. */
    Result fillLayer(LayerId layer, const std::array<quint8, 3> &rgb, quint8 opacity = 255,
                     bool erase = false, bool alphaLocked = false);
    Result undo();
    Result redo();
    Result setHistoryIndex(int index);
    /** Records completed file saving; file owners publish their metadata changes. */
    Result markSaved(const Snapshot &version);
    /** Adopts validated logical recovery on the same attachment and notifies readers. */
    Result restoreRecovery(const QByteArray &data);
    void close();

Q_SIGNALS:
    void changed();
    /** Saving changes history cleanliness without changing pixels or GPU requests. */
    void savedStateChanged();

private:
    PaintImage(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits,
               std::shared_ptr<Execution> retainedExecution = {});
    bool onThread() const;
    Result publish(Result result);
    Result requestAfterWrite(Result result);
    void requestCurrentVersion();
    void refreshLayerPixelReferences();
    Result applyRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, bool recordHistory,
                           Snapshot *retainedVersion = nullptr);
    struct Private;
    std::unique_ptr<Private> m_d;
};
using PaintImageSP = QSharedPointer<PaintImage>;
}
#endif
