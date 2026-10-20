/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_DOCUMENT_H
#define KIS_GPU_OPERATION_DOCUMENT_H

#include <array>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QPointF>
#include <QRect>
#include <QSharedPointer>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QUuid>
#include <kritaimagegpuoperations_export.h>
#include "KisGpuRasterPixels.h"
#include <variant>
#include <memory>
#include <optional>

namespace LibrePaint::Image::Gpu {

using LayerId = quint64;

/** Attributes enabled for one layer by the properties dialog. Component masks
 * select the RGBA bits whose values participate in Undo and Redo.
 */
struct LayerPropertiesSelection
{
    LayerId layer = 0;
    bool name = false;
    bool opacity = false;
    bool visible = false;
    bool locked = false;
    bool colorLabel = false;
    quint8 protectedChannels = 0;
    quint8 projectionChannels = 0;
    bool passThrough = false;
    bool blendMode = false;
};

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
    /** Normalized drawing speed supplied by the input interpreter. */
    qreal speed = 0;

    bool operator==(const InputSample &other) const
    {
        return position == other.position && pressure == other.pressure &&
            xTilt == other.xTilt && yTilt == other.yTilt &&
            rotation == other.rotation && timestamp == other.timestamp && speed == other.speed;
    }
};

struct WeightedSmoothing
{
    qreal distanceMin = 0;
    qreal distanceMax = 0;
    qreal tailAggressiveness = 0;
    bool smoothPressure = false;

    bool operator==(const WeightedSmoothing &other) const
    {
        return distanceMin == other.distanceMin && distanceMax == other.distanceMax &&
            tailAggressiveness == other.tailAggressiveness && smoothPressure == other.smoothPressure;
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
    bool alphaLocked = false;
    /** Writable R, G, B, A channels in bits 0 through 3, frozen at stroke start. */
    quint8 channelWriteMask = 15;
    bool mirrorHorizontal = false;
    bool mirrorVertical = false;
    /** Absolute document-pixel axes, rounded when input begins. */
    QPoint mirrorCenter;
    /** Pixel-centered freehand input with adjacent corner samples held back. */
    bool pixelPerfect = false;
    /** Basic freehand interpolation retains raw sensors and explicit idle boundaries. */
    bool basicSmoothing = false;
    /** Weighted positions feed basic interpolation; distances are frozen in document pixels. */
    std::optional<WeightedSmoothing> weightedSmoothing{};

    bool operator==(const Brush &other) const
    {
        return rgba == other.rgba && diameter == other.diameter &&
            spacing == other.spacing && opacity == other.opacity && erase == other.erase &&
            alphaLocked == other.alphaLocked && channelWriteMask == other.channelWriteMask &&
            mirrorHorizontal == other.mirrorHorizontal && mirrorVertical == other.mirrorVertical &&
            mirrorCenter == other.mirrorCenter && pixelPerfect == other.pixelPerfect &&
            basicSmoothing == other.basicSmoothing && weightedSmoothing == other.weightedSmoothing;
    }
};

enum class ShapeKind { Freehand, Line, Ellipse, Rectangle, RoundedRectangle, Polyline, Polygon, Path };
enum class PathCommand : quint8 { MoveTo, LineTo, CubicTo };

struct Shape
{
    /** Lines retain intermediate sensor values at their projected input positions. */
    ShapeKind kind = ShapeKind::Freehand;
    QPointF first;
    QPointF second;
    qreal rotation = 0;
    QSizeF cornerRadii{0, 0};
    /** Commands consume one input point, or three for the cubic controls and endpoint. */
    QList<PathCommand> pathCommands{};
    bool windingFill = false;
};

/** Solid figure coverage, separate from the outline brush and its color. */
struct FigureStyle
{
    bool outline = true;
    std::optional<std::array<quint8, 4>> fill;
};

/**
 * The input and intent of one stroke. GPU dabs and pixels are derived values.
 * Immutable blocks preserve accepted input when previews and history share it.
 * Block boundaries carry no drawing semantics.
 */
struct KRITAIMAGEGPUOPERATIONS_EXPORT PaintOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    Brush brush;
    quint64 randomSeed = 0;
    Shape shape;
    QList<QSharedPointer<const QList<InputSample>>> sampleBlocks;
    qsizetype sampleCount = 0;
    FigureStyle figureStyle;
    /** Accepted input counts at which an idle timeout completed the current curve. */
    QList<qsizetype> smoothingFlushes{};
    bool smoothingFinished = false;

    QList<InputSample> inputSamples() const;
};

/** Whole-tile replacement; retained compressed pixels preserve external results. */
struct TileWriteOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    QList<QSharedPointer<const RasterTile>> tiles;
    /** One external write can preserve different neighbouring history pixels. */
    quint64 writeGroup = 0;
    /** Empty denotes complete tiles; otherwise these areas identify changed pixel blocks. */
    QList<QRect> writtenAreas;
};
using PaintSnapshot = QSharedPointer<const PaintOperation>;
using TileWriteSnapshot = QSharedPointer<const TileWriteOperation>;
/** Integer relocation of all preceding layer pixels, preserving their RGBA values. */
struct TranslationOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    QPoint offset;
};
using TranslationSnapshot = QSharedPointer<const TranslationOperation>;
/** Resets preceding pixels outside the requested rectangle to the layer default. */
struct CropOperation
{
    OperationId id;
    quint64 version = 1;
    LayerId layer = 0;
    QRect rect;
};
using CropSnapshot = QSharedPointer<const CropOperation>;
using LayerOperation = std::variant<PaintSnapshot, TileWriteSnapshot, TranslationSnapshot, CropSnapshot>;

enum class LayerKind : quint8 { Paint, Group };
enum class LayerBlendMode : quint8 { Normal, Multiply, Screen, Darken, Lighten };

struct RasterLayer
{
    QString name;
    bool visible = true;
    QUuid uuid;
    QList<RgbaTile> tiles;
    std::array<quint8, 4> defaultPixel{};
    quint8 opacity = 255;
    bool userLocked = false;
    bool pinnedToTimeline = false;
    quint8 channelWriteMask = 15;
    /** Public device position; tile coordinates remain in image space. */
    QPoint position{};
    LayerKind kind = LayerKind::Paint;
    QUuid parentUuid{};
    /** Label identity; its displayed color is owned by the UI configuration. */
    int colorLabelIndex = 0;
    /** RGBA components contributed to the parent projection. */
    quint8 projectionChannelMask = 15;
    /** Groups contribute their descendants directly to the enclosing composition. */
    bool passThrough = false;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
};
/** Validates parent identities, group parents and cycles. Null identities are
 * assigned during raster import and can only be referenced after assignment.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT bool validRasterLayerHierarchy(const QList<RasterLayer> &layers);
struct RasterImage
{
    QSize size;
    QList<RasterLayer> layers;
    /** Bottom-to-top list index; zero for an empty document. */
    qsizetype activeLayer = 0;
    std::array<quint8, 4> projectionBackground{};
};

struct Layer
{
    LayerId id = 0;
    QString name;
    bool visible = true;
    QList<LayerOperation> operations;
    QUuid uuid;
    std::array<quint8, 4> defaultPixel{};
    quint8 opacity = 255;
    bool userLocked = false;
    bool pinnedToTimeline = false;
    quint8 channelWriteMask = 15;
    QPoint position{};
    LayerKind kind = LayerKind::Paint;
    QUuid parentUuid{};
    int colorLabelIndex = 0;
    quint8 projectionChannelMask = 15;
    /** Groups contribute their descendants directly to the enclosing composition. */
    bool passThrough = false;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
};

/** One visible layer contributed to a composition boundary, after passing
 * ancestor opacity and immediate-parent channel selection through groups.
 */
struct LayerProjectionInput
{
    LayerId layer = 0;
    quint8 opacity = 255;
    quint8 channelMask = 15;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
};

/** Immutable RGBA8/sRGB document version with paint layers and group ownership. */
class KRITAIMAGEGPUOPERATIONS_EXPORT DocumentVersion
{
public:
    QUuid documentId() const { return m_document; }
    quint64 revision() const { return m_revision; }
    /** Identity of this history state, preserved by direct writes outside undo. */
    quint64 historyRevision() const { return m_modifiedRevision; }
    /** Admission state is queried on the OperationDocument owner's thread. */
    bool isCommitted() const { return m_committed; }
    OperationId pendingOperation() const { return m_committed ? OperationId{} : m_pendingOperation; }
    /** Uniform projection values before each node's own opacity. Group values
     * derive from child defaults, order, visibility, opacity and projection components.
     */
    QHash<LayerId, std::array<quint8, 4>> projectionDefaultPixels() const;
    /** Bottom-to-top inputs for the root, or one group's isolated subtree.
     * A group boundary excludes its own attributes and all outside state.
     * Unavailable group identities throw std::invalid_argument.
     */
    QList<LayerProjectionInput> projectionInputs(LayerId group = 0) const;
    QSize size;
    std::array<quint8, 4> projectionBackground{};
    QList<Layer> layers;
    LayerId activeLayer = 0;
    QList<LayerId> selectedLayers;
    qsizetype sampleCount = 0;
    qsizetype rasterBytes = 0;

    /** Transparent root projections reuse one full-opacity child, preserving hidden RGB. */
    LayerId directProjectionLayer() const
    {
        if (projectionBackground[3] != 0) return 0;
        const Layer *only = nullptr;
        for (const auto &layer : layers) {
            if (!layer.parentUuid.isNull()) continue;
            if (only) return 0;
            only = &layer;
        }
        return only && only->blendMode == LayerBlendMode::Normal && !only->passThrough && only->visible && only->opacity == 255 && only->projectionChannelMask == 15 ? only->id : 0;
    }

private:
    friend class OperationDocument;
    DocumentVersion() = default;
    DocumentVersion(const DocumentVersion &) = default;
    QUuid m_document;
    quint64 m_revision = 0;
    quint64 m_modifiedRevision = 0;
    // Admission is owned by OperationDocument's thread; snapshot content is immutable.
    mutable bool m_committed = false;
    OperationId m_pendingOperation;
};

using Snapshot = QSharedPointer<const DocumentVersion>;

/** Frozen layer pixels and fresh destination properties for one prepared node. */
struct LayerInsertion
{
    Snapshot source;
    LayerId sourceLayer = 0;
    RasterLayer properties;
};

struct CanvasResize
{
    QRect rect;
    bool cropLayers = false;
};

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
    InvalidRecovery,
    Busy,
    DeviceFailure
};

/** Consecutive panel adjustments form one undo step; independent edits retain separate steps. */
enum class PropertyHistory { Separate, MergeConsecutive };

/** Domain action for one retained undo step, independent of its current property values. */
enum class HistoryAction {
    None, AddLayer, RemoveLayer, MoveLayer, RenameLayer, ChangeOpacity,
    ChangeVisibility, ChangeLock, ChangeTimelinePin, PaintStroke, DrawLine,
    DrawEllipse, ReplacePixels, DrawRectangle, DrawPolyline, DrawPolygon, ChangeChannelLock, DuplicateLayer, DrawPath,
    ClearPixels, FillLayer, TranslateLayer, CropLayer, ResizeCanvas, CropImage, ChangeProjectionChannels, ChangeLayerProperties, ChangePassThrough, ChangeBlendMode
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
 * Owns retained painting operations, layer hierarchy and the document's history.
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
class KRITAIMAGEGPUOPERATIONS_EXPORT OperationDocument
{
public:
    static constexpr int HistoryLimit = 250;
    static constexpr qreal CoordinateLimit = RgbaTile::CoordinateLimit;

    explicit OperationDocument(QSize size, Limits limits = {});
    /** Validated raster import starts saved, with preserved layer UUIDs and empty history. */
    explicit OperationDocument(const RasterImage &image, Limits limits = {});
    /** Captured input is bounded before a producer allocates its retained copy. */
    Limits limits() const { return m_limits; }
    OperationDocument(const OperationDocument &) = delete;
    OperationDocument &operator=(const OperationDocument &) = delete;

    Snapshot snapshot() const;
    Snapshot preview() const;
    LayerId activeLayer() const;
    int undoCount() const;
    int redoCount() const;
    /** Shared immutable versions for the public history display, oldest first. */
    QList<Snapshot> historyVersions() const;
    /** Action reaching the given history position; None for the initial or invalid position. */
    HistoryAction historyAction(int index) const;
    /** Original rectangle and layer identity for resuming a retained crop action. */
    CropSnapshot historyCrop(int index) const;
    std::optional<CanvasResize> historyCanvasResize(int index) const;
    /** -1 when the saved version has left the retained history. */
    int savedHistoryIndex() const;
    bool isModified() const;
    bool hasActiveStroke() const;
    OperationId activeOperation() const;

    /** Returns an empty identifier for a closed/busy document or invalid brush/layer. */
    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 randomSeed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    /** Completes basic interpolation at the current input count. Repeated calls
     * preserve the version; other modes return InvalidInput and fewer than two
     * samples return EmptyOperation. Original sensor input remains unchanged.
     */
    Result flushSmoothing(OperationId id);
    Result updateShape(OperationId id, const Shape &shape);
    /** Freezes solid fill and outline intent together with the figure geometry. */
    Result updateFigure(OperationId id, const Shape &shape, const FigureStyle &style);
    /** Validates and commits one complete figure without exposing an intermediate preview. */
    Result paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                       const Shape &shape, const FigureStyle &style, quint64 randomSeed);
    Result commitStroke(OperationId id);
    Result cancelStroke(OperationId id);
    /** Replaces full tiles, including hidden layers, in one history entry. */
    Result replaceTiles(LayerId layer, const QList<RgbaTile> &tiles);
    /** A placement preview freezes its layer and starting version. Updates use
     * an integer offset from the start; commit records one history entry and
     * cancellation retains the committed version and redo branch.
     */
    OperationId beginLayerTranslation(LayerId layer);
    Result updateLayerTranslation(OperationId id, QPoint offset);
    Result commitLayerTranslation(OperationId id);
    Result cancelLayerTranslation(OperationId id);
    /** Absolute device position outside undo. Updates the layer's shared
     * history range, retaining pinned versions, recorded placements and saved
     * state. Active input rejects the write without changing either version.
     */
    Result writeLayerPosition(LayerId layer, QPoint position);

    /** Adds and selects a layer in one history entry; returns zero on rejection. */
    LayerId addLayer(const QString &name);
    /** Inserts and selects a paint layer at a document-list index. A nonnull
     * parent UUID identifies an existing group; null places it at the root.
     */
    LayerId addLayer(const QString &name, qsizetype index, bool pinnedToTimeline, const QUuid &parentUuid = {});
    /** Copies one committed paint layer or empty group into a new identity and one history entry.
     * Properties supply a fresh UUID and attributes; pixels retain their source
     * default and immutable blocks. Zero reports rejected input or capacity.
     */
    LayerId insertLayer(const OperationDocument &source, LayerId sourceLayer,
                        const RasterLayer &properties, qsizetype index);
    /** Adopts a prepared forest in list order as one history entry. Parent UUIDs
     * refer to existing groups or other prepared nodes. Selects its roots and
     * activates the first root; Undo restores the original selection and active
     * layer. Rejected requests retain the complete document and input state.
     */
    Result insertLayers(const QList<LayerInsertion> &layers, qsizetype index);
    /** Selection is screen state; selecting an existing layer adds no history entry. */
    Result selectLayer(LayerId layer);
    /** A nonempty screen selection and an independent existing active layer.
     * Preserves history, pending input and fixed snapshots.
     */
    Result selectLayers(const QList<LayerId> &layers, LayerId active);
    /** Duplicates selected subtrees above their highest root, climbing locked parents.
     * Input identities are in bottom-to-top list order. Selected descendants of another selected root are included once.
     * Names correspond to the selected identities. Copies have independent layer
     * and operation identities while retaining immutable input and pixel blocks.
     * A source outside the filtered selection selects the first copied root.
     */
    Result duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource);
    Result setLayerVisible(LayerId layer, bool visible);
    Result setLayerVisible(LayerId layer, bool visible, PropertyHistory history);
    Result setLayerOpacity(LayerId layer, quint8 opacity);
    Result setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history);
    Result setLayerLocked(LayerId layer, bool locked);
    Result setLayerLocked(LayerId layer, bool locked, PropertyHistory history);
    Result setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerChannelMask(LayerId layer, quint8 mask);
    Result setLayerProjectionMask(LayerId layer, quint8 mask, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerProjectionMask(LayerId layer, quint8 mask);
    Result setLayerPassThrough(LayerId layer, bool enabled, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerPassThrough(LayerId layer, bool enabled);
    Result setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerBlendMode(LayerId layer, LayerBlendMode mode);
    /** Direct background writes preserve history, pending input and fixed snapshots. */
    Result writeProjectionBackground(const std::array<quint8, 4> &rgba);
    /** Direct property writes preserve history transitions, pending input and fixed snapshots. */
    Result writeLayerName(LayerId layer, const QString &name);
    Result writeLayerVisible(LayerId layer, bool visible);
    Result writeLayerOpacity(LayerId layer, quint8 opacity);
    Result writeLayerLocked(LayerId layer, bool locked);
    Result writeLayerPinnedToTimeline(LayerId layer, bool pinned);
    Result writeLayerColorLabel(LayerId layer, int index);
    /** Records selected attributes from the fixed before version and current
     * after version as one edit. Both belong to this document and retain the
     * selected layer identities. Pixels, hierarchy, placement and other edits
     * completed during preview remain current. Busy, stale or rejected requests
     * preserve history, pending input and fixed versions.
     * Enabled attributes participate even when unchanged; unselected attributes
     * and components retain subsequent direct writes across this history step.
     */
    Result recordLayerProperties(const Snapshot &before, const Snapshot &after,
                                 const QList<LayerPropertiesSelection> &selection);
    /** Restores selected attributes from a fixed version outside undo, including
     * their exact component values. Preserves other edits, Redo, pending input
     * and fixed versions; rejects the complete request before adoption.
     */
    Result restoreLayerProperties(const Snapshot &before, const QList<LayerPropertiesSelection> &selection);
    Result renameLayer(LayerId layer, const QString &name);
    Result renameLayer(LayerId layer, const QString &name, PropertyHistory history);
    /** Moves within the bottom-to-top layer list, preserving the selected identity. */
    Result moveLayer(LayerId layer, qsizetype index);
    /** Adopts a complete bottom-to-top permutation in one history entry.
     * Rejects missing, repeated or foreign identities before changing any state.
     */
    Result reorderLayers(const QList<LayerId> &order);
    /** Moves selected roots into an existing group, or the root for a null parent.
     * A null sibling places them first; otherwise it must be a remaining child
     * of that parent. Roots follow native tree order and include their descendants.
     * Both sides of the history step select the filtered roots. A zero activeRoot
     * retains the independent active layer; a specified moving root becomes
     * active after the move. Undo retains the original active layer.
     * Fixed versions and pixel inputs remain unchanged.
     * Locked parents, cycles and invalid identities reject the complete request;
     * an unchanged hierarchy preserves the current version and history.
     */
    Result moveLayers(const QList<LayerId> &layers, const QUuid &parentUuid, const QUuid &aboveUuid,
                      LayerId activeRoot = 0);
    /** Removes a layer and its descendants; selection follows the nearest remaining list position. */
    Result removeLayer(LayerId layer);
    /** Removes a set atomically in one history entry. Selection follows the
     * next remaining layer, or the preceding layer when no next layer remains.
     * A nonempty replacement name creates an empty layer when all are removed.
     */
    Result removeLayers(const QList<LayerId> &layers, const QString &replacementName = {});
    /** Clears retained paint and raster inputs in one undo step, preserving each
     * layer's default pixel, identity, properties and selection. The complete
     * request must name distinct, available, unlocked layers.
     */
    Result clearLayers(const QList<LayerId> &layers);
    /** One undo step; normalizes the pixel rectangle while retaining its original
     * UI values. Defaults, position and properties stay unchanged. Programmatic
     * cropping accepts locked layers; interactive callers enforce editability.
     */
    Result cropLayer(LayerId layer, const QRect &rect);
    /** Moves the requested canvas origin to zero and optionally crops all layers.
     * Preserves layer properties and selection in one history step. An unchanged
     * canvas without layer cropping preserves the current version and history.
     */
    Result resizeCanvas(const QRect &rect, bool cropLayers);
    /** Fills canvas bounds on one layer as one history step, preserving pixels outside them. */
    Result fillLayer(LayerId layer, const std::array<quint8, 3> &rgb, quint8 opacity = 255,
                     bool erase = false, bool alphaLocked = false);
    Result undo();
    Result redo();
    /** Selects one retained history state without executing intermediate edits. */
    Result setHistoryIndex(int index);

    /** Call after successful atomic saving of this snapshot, including an older pinned version. */
    Result markSaved(const Snapshot &saved);
    /** Bounded recovery of logical versions, both history branches and pending input. */
    QByteArray recoveryData() const;
    /** Validates the whole record before replacing state; starts a new operation identity. */
    /** Records predating color labels adopt retained format values by UUID.
     * Labels present in the record take precedence over these values.
     */
    Result restoreRecovery(const QByteArray &data, const QHash<QUuid, int> &legacyColorLabels = {});
    /** Releases uncommitted input and history; the last committed snapshot remains available. */
    void close();

private:
    friend class PaintImage;
    enum class LayerProperty : quint8 { None, Name, Opacity, Visible, Locked, PinnedToTimeline, ChannelMask, Position, ProjectionMask, ColorLabel, PassThrough, BlendMode };
    struct LayerPropertyEdit {
        LayerProperty property;
        LayerId layer;
        QVariant before, after;
        quint8 componentMask = 15;
    };
    struct LayerHistoryChange {
        LayerProperty property = LayerProperty::None;
        LayerId layer = 0;
        QVariant before, after;
        bool mergeable = false;
        bool duplicatedLayers = false;
        bool clearedLayers = false;
        bool filledLayer = false;
        CropSnapshot crop;
        std::optional<CanvasResize> canvasResize;
        QList<LayerPropertyEdit> propertyEdits;
        bool changedLayerProperties = false;
    };
    std::unique_ptr<OperationDocument> cloneCurrent(bool preserveLayerUuids) const;
    LayerId insertVersion(const Snapshot &source, LayerId sourceLayer,
                          const RasterLayer &properties, qsizetype index);
    std::unique_ptr<OperationDocument> cloneVersion(const Snapshot &source, bool preserveLayerUuids,
                                                  LayerId onlyLayer = 0) const;
    struct VersionPixelWrite
    {
        Snapshot before;
        LayerId layer = 0;
        QList<QSharedPointer<const RasterTile>> tiles;
        QList<QRect> areas;
    };
    Result applyPixelVersions(const QList<VersionPixelWrite> &writes, bool recordHistory,
                              Snapshot *retainedVersion = nullptr);
    QList<Snapshot> layerPositionVersions(const Snapshot &source, LayerId layer) const;
    Result applyLayerPosition(LayerId layer, QPoint position, Snapshot *retainedVersion);
    template<class T> Result writeLayerProperty(LayerId layer, LayerProperty property, T Layer::*member,
                                              const T &value, quint8 componentMask = 15);
    Result writeLayerMask(LayerId layer, LayerProperty property, quint8 Layer::*member, quint8 value);
    Result restoreLayerHistoryChange(const LayerHistoryChange &change, bool forward);
    Result restoreLayerPropertyEdit(const LayerPropertyEdit &change, bool forward);
    Result collectLayerPropertyEdits(const Snapshot &before, const Snapshot &after,
                                    const QList<LayerPropertiesSelection> &selection,
                                    QList<LayerPropertyEdit> &edits) const;
    Result restoreLayerPropertyEdits(const QList<LayerPropertyEdit> &edits);
    Result checkOperation(OperationId id) const;
    Result checkMutation() const;
    qsizetype layerIndex(LayerId layer) const;
    void replacePreview(const QSharedPointer<const PaintOperation> &operation);
    void commitVersion(const Snapshot &version);
    void commitVersion(const Snapshot &version, const LayerHistoryChange &change);
    void commitPropertyVersion(const Snapshot &version, LayerHistoryChange change, PropertyHistory history);

    Limits m_limits;
    QUuid m_identity;
    QList<Snapshot> m_history;
    QList<LayerHistoryChange> m_layerHistoryChanges;
    int m_index = 0;
    quint64 m_nextRevision = 0;
    quint64 m_nextOperation = 0;
    LayerId m_nextLayer = 1;
    quint64 m_savedRevision = 0;
    QSharedPointer<const PaintOperation> m_edit;
    TranslationSnapshot m_translation;
    Snapshot m_preview;
    bool m_closed = false;
};

}
#endif
