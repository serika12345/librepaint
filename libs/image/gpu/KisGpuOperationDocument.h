/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_DOCUMENT_H
#define KIS_GPU_OPERATION_DOCUMENT_H

#include <array>
#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QSharedPointer>
#include <QSize>
#include <QString>
#include <QUuid>
#include <kritaimage_export.h>
#include "KisGpuRasterPixels.h"
#include <variant>

namespace Krita::Image::Gpu {

using LayerId = quint64;

struct OperationId
{
    QUuid document;
    quint64 sequence = 0;

    bool isNull() const { return document.isNull() || sequence == 0; }
    bool operator==(const OperationId &other) const
    { return document == other.document && sequence == other.sequence; }
    bool operator!=(const OperationId &other) const { return !(*this == other); }
};

struct InputSample
{
    QPointF position;
    qreal pressure = 1;
    qreal xTilt = 0;
    qreal yTilt = 0;
    qreal rotation = 0;
    quint64 timestamp = 0;

    bool operator==(const InputSample &other) const
    {
        return position == other.position && pressure == other.pressure &&
            xTilt == other.xTilt && yTilt == other.yTilt &&
            rotation == other.rotation && timestamp == other.timestamp;
    }
};

/** Frozen settings for the resource-free circular brush, algorithm version 1. */
struct Brush
{
    std::array<quint8, 4> rgba{0, 0, 0, 255};
    qreal diameter = 10;
    qreal spacing = 0.1;
    quint8 opacity = 255;
    bool erase = false;

    bool operator==(const Brush &other) const
    {
        return rgba == other.rgba && diameter == other.diameter &&
            spacing == other.spacing && opacity == other.opacity && erase == other.erase;
    }
};

enum class ShapeKind { Freehand, Line, Ellipse };

struct Shape
{
    ShapeKind kind = ShapeKind::Freehand;
    QPointF first;
    QPointF second;
    qreal rotation = 0;
};

/**
 * The input and intent of one stroke. GPU dabs and pixels are derived values.
 * Immutable blocks preserve accepted input when previews and history share it.
 * Block boundaries carry no drawing semantics.
 */
struct KRITAIMAGE_EXPORT PaintOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    Brush brush;
    quint64 randomSeed = 0;
    Shape shape;
    QList<QSharedPointer<const QList<InputSample>>> sampleBlocks;
    qsizetype sampleCount = 0;

    QList<InputSample> inputSamples() const;
};

/** Whole-tile replacement; retained compressed pixels preserve external results. */
struct TileWriteOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    QList<QSharedPointer<const RasterTile>> tiles;
};
using PaintSnapshot = QSharedPointer<const PaintOperation>;
using TileWriteSnapshot = QSharedPointer<const TileWriteOperation>;
using LayerOperation = std::variant<PaintSnapshot, TileWriteSnapshot>;

struct RasterLayer
{
    QString name;
    bool visible = true;
    QUuid uuid;
    QList<RgbaTile> tiles;
    std::array<quint8, 4> defaultPixel{};
};
struct RasterImage
{
    QSize size;
    QList<RasterLayer> layers;
    qsizetype activeLayer = 0;
};

struct Layer
{
    LayerId id = 0;
    QString name;
    bool visible = true;
    QList<LayerOperation> operations;
    QUuid uuid;
    std::array<quint8, 4> defaultPixel{};
};

/** Immutable logical document version; all layers are flat RGBA8/sRGB paint layers. */
class KRITAIMAGE_EXPORT DocumentVersion
{
public:
    QUuid documentId() const { return m_document; }
    quint64 revision() const { return m_revision; }
    /** Admission state is queried on the OperationDocument owner's thread. */
    bool isCommitted() const { return m_committed; }
    QSize size;
    QList<Layer> layers;
    LayerId activeLayer = 0;
    qsizetype sampleCount = 0;
    qsizetype rasterBytes = 0;

private:
    friend class OperationDocument;
    DocumentVersion() = default;
    DocumentVersion(const DocumentVersion &) = default;
    QUuid m_document;
    quint64 m_revision = 0;
    // Admission is owned by OperationDocument's thread; snapshot content is immutable.
    mutable bool m_committed = false;
};

using Snapshot = QSharedPointer<const DocumentVersion>;

enum class Result {
    Success,
    InvalidInput,
    CapacityExceeded,
    StaleOperation,
    StaleVersion,
    LayerUnavailable,
    EditInProgress,
    EmptyOperation,
    NothingToUndo,
    NothingToRedo,
    Closed,
    InvalidRecovery
};

struct Limits
{
    qsizetype samplesPerStroke = 1024 * 1024;
    qsizetype samplesPerDocument = 4 * 1024 * 1024;
    qsizetype layerCount = 4096;
    qsizetype recoveryBytes = 512 * 1024 * 1024;
    qsizetype rasterBytes = 256 * 1024 * 1024;
};

/**
 * Owns retained painting operations, flat layer state and the document's history.
 *
 * Call on one owning thread. Snapshots are immutable and can outlive the owner;
 * consumers retain a snapshot for background work. Each accepted append or shape
 * update produces a new preview. Commit adopts that exact preview as one history
 * entry; cancel preserves the committed version and redo branch. Undo/redo only
 * select existing versions and execute no paint operation or external effect.
 *
 * Construction requires positive dimensions and limits; invalid values throw
 * std::invalid_argument. Requests validate the complete input before adoption.
 * Rejected requests preserve accepted input and both history branches.
 */
class KRITAIMAGE_EXPORT OperationDocument
{
public:
    static constexpr int HistoryLimit = 250;
    static constexpr qreal CoordinateLimit = RgbaTile::CoordinateLimit;

    explicit OperationDocument(QSize size, Limits limits = {});
    /** Validated flat raster import starts saved, with preserved layer UUIDs and empty history. */
    explicit OperationDocument(const RasterImage &image, Limits limits = {});
    OperationDocument(const OperationDocument &) = delete;
    OperationDocument &operator=(const OperationDocument &) = delete;

    Snapshot snapshot() const;
    Snapshot preview() const;
    LayerId activeLayer() const;
    int undoCount() const;
    int redoCount() const;
    bool isModified() const;
    bool hasActiveStroke() const;
    OperationId activeOperation() const;

    /** Returns an empty identifier for a closed/busy document or invalid brush/layer. */
    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 randomSeed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    Result updateShape(OperationId id, const Shape &shape);
    Result commitStroke(OperationId id);
    Result cancelStroke(OperationId id);
    /** Replaces full tiles, including hidden layers, in one history entry. */
    Result replaceTiles(LayerId layer, const QList<RgbaTile> &tiles);

    /** Adds and selects a layer in one history entry; returns zero on rejection. */
    LayerId addLayer(const QString &name);
    /** Selection is screen state; selecting an existing layer adds no history entry. */
    Result selectLayer(LayerId layer);
    Result setLayerVisible(LayerId layer, bool visible);
    Result undo();
    Result redo();

    /** Call after successful atomic saving of this snapshot, including an older pinned version. */
    Result markSaved(const Snapshot &saved);
    /** Bounded recovery of logical versions, both history branches and pending input. */
    QByteArray recoveryData() const;
    /** Validates the whole record before replacing state; starts a new operation identity. */
    Result restoreRecovery(const QByteArray &data);
    /** Releases uncommitted input and history; the last committed snapshot remains available. */
    void close();

private:
    Result checkOperation(OperationId id) const;
    Result checkMutation() const;
    qsizetype layerIndex(LayerId layer) const;
    void replacePreview(const QSharedPointer<const PaintOperation> &operation);
    void commitVersion(const Snapshot &version);

    Limits m_limits;
    QUuid m_identity;
    QList<Snapshot> m_history;
    int m_index = 0;
    quint64 m_nextRevision = 0;
    quint64 m_nextOperation = 0;
    LayerId m_nextLayer = 1;
    quint64 m_savedRevision = 0;
    QSharedPointer<const PaintOperation> m_edit;
    Snapshot m_preview;
    bool m_closed = false;
};

}
#endif
