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

namespace LibrePaint::Image::Gpu {
class Execution;
struct ExecutionOutcome;
struct PixelBytesOutcome;

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
    ExecutionOutcome lastRequestOutcome() const;
    /** Synchronous canonical pixels from the current committed image version. */
    PixelBytesOutcome readRgbaPixels(LayerId layer, QRect region, qsizetype maximumBytes) const;

    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 seed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    Result updateShape(OperationId id, const Shape &shape);
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
    /** Direct background writes request pixels without adding history or edit notifications. */
    Result writeProjectionBackground(const std::array<quint8, 4> &rgba);
    Result writeLayerName(LayerId layer, const QString &name);
    Result writeLayerVisible(LayerId layer, bool visible);
    Result writeLayerOpacity(LayerId layer, quint8 opacity);
    Result writeLayerLocked(LayerId layer, bool locked);
    Result writeLayerPinnedToTimeline(LayerId layer, bool pinned);
    LayerId addLayer(const QString &name);
    Result selectLayer(LayerId layer);
    Result setLayerVisible(LayerId layer, bool visible);
    Result setLayerOpacity(LayerId layer, quint8 opacity);
    Result setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history);
    Result setLayerLocked(LayerId layer, bool locked);
    Result renameLayer(LayerId layer, const QString &name);
    Result renameLayer(LayerId layer, const QString &name, PropertyHistory history);
    Result moveLayer(LayerId layer, qsizetype index);
    Result removeLayer(LayerId layer);
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
    Result applyRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, bool recordHistory);
    struct Private;
    std::unique_ptr<Private> m_d;
};
using PaintImageSP = QSharedPointer<PaintImage>;
}
#endif
