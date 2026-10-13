/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_IMAGE_H
#define KIS_GPU_PAINT_IMAGE_H

#include "KisGpuOperationDocument.h"
#include <QObject>
#include <memory>

namespace Krita::Image::Gpu {
class Execution;
struct ExecutionOutcome;

/** Shared editable image, history, notifications and GPU attachment.
 * Calls and destruction use the GUI/device thread. The borrowed execution
 * owner outlives every shared image reference. Accepted edits update the GPU
 * request before changed() is emitted; admission failures retain logical edits.
 * Invalid construction throws std::invalid_argument, and exhausted attachment
 * capacity throws std::length_error. The last image reference closes its handle.
 */
class KRITAIMAGE_EXPORT PaintImage : public QObject
{
    Q_OBJECT
public:
    explicit PaintImage(Execution &execution, QSize size, Limits limits = {});
    explicit PaintImage(Execution &execution, const RasterImage &image, Limits limits = {});
    ~PaintImage() override;
    /** Borrowed logical owner; successful recovery replaces this reference. */
    const OperationDocument &operations() const;
    quint64 handle() const;
    ExecutionOutcome lastRequestOutcome() const;

    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 seed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    Result updateShape(OperationId id, const Shape &shape);
    Result commitStroke(OperationId id);
    Result cancelStroke(OperationId id);
    Result replaceTiles(LayerId layer, const QList<RgbaTile> &tiles);
    /** Direct external writes publish pixels without editing notifications or history. */
    Result writeTiles(LayerId layer, const QList<RgbaTile> &tiles);
    LayerId addLayer(const QString &name);
    Result selectLayer(LayerId layer);
    Result setLayerVisible(LayerId layer, bool visible);
    Result renameLayer(LayerId layer, const QString &name);
    Result moveLayer(LayerId layer, qsizetype index);
    Result removeLayer(LayerId layer);
    Result undo();
    Result redo();
    /** Records completed file saving; file owners publish their metadata changes. */
    Result markSaved(const Snapshot &version);
    /** Adopts validated logical recovery on the same attachment and notifies readers. */
    Result restoreRecovery(const QByteArray &data);
    void close();

Q_SIGNALS:
    void changed();

private:
    PaintImage(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits);
    bool onThread() const;
    Result publish(Result result);
    struct Private;
    std::unique_ptr<Private> m_d;
};
using PaintImageSP = QSharedPointer<PaintImage>;
}
#endif
