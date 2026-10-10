/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TILE_STORE_H
#define KIS_GPU_TILE_STORE_H

#include <QPoint>
#include <QByteArray>
#include <QRect>
#include <QVector>
#include <QSizeF>
#include <memory>
#include <optional>
#include <webgpu/wgpu.h>

class KisGpuDevice;

/**
 * Immutable sparse RGBA8 tiles on one externally created GPU device. Calls,
 * including poll(), belong to the device's submitting thread. The store keeps
 * its own device reference; versions may outlive the store. Destruction waits
 * for this store's final submission. Tile coordinates are signed, in 64px units.
 */
class KisGpuTileStore
{
    struct VersionData;
    struct CompletionData;
    struct ReadbackData;
    struct TextureData;
    struct BrushTextureData;
public:
    static constexpr quint64 TileBytes = 64 * 64 * 4;
    enum class Status { Pending, Succeeded, Failed };
    enum class Error { None, InvalidVersion, BudgetExceeded, InvalidCommand, QueueFull, DeviceLost };
    enum class CompositeOp { Over, Erase };

    struct PaintCommand {
        QRect rectangle;
        quint32 rgba = 0;
        CompositeOp operation = CompositeOp::Over;
        quint8 opacity = 255;
        quint8 coverage = 255;
    };

    /** Default two-spike circle mask, unrotated, without edge supersampling. */
    struct DabCommand {
        QPointF center;
        QSizeF diameter;
        QSizeF fade = QSizeF(1, 1);
        quint32 rgba = 0;
        CompositeOp operation = CompositeOp::Over;
        quint8 opacity = 255;
        quint8 coverage = 255;
    };

    /** Borrowed read-only TileBytes range; a null buffer denotes an absent tile. */
    struct TileView {
        WGPUBuffer buffer = nullptr;
        quint64 offset = 0;
    };

    class Version {
    public:
        Version() = default;
        /** Identity of the immutable version, independent of pixel equality. */
        bool operator==(const Version &other) const { return d == other.d; }
        qsizetype tileCount() const;
        /** Coordinates of allocated tiles; metadata only, with no pixel transfer. */
        QVector<QPoint> tileCoordinates() const;
        /** Retain this version through completion of any GPU read of the returned range. */
        TileView tile(QPoint coordinate) const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<const VersionData> d;
    };

    /** One input in bottom-to-top projection order; an uninitialized mask means unmasked. */
    struct Layer {
        Version pixels;
        Version mask{};
        quint8 opacity = 255;
        CompositeOp operation = CompositeOp::Over;
    };

    class Completion {
    public:
        Completion() = default;
        Status status() const;
        quint64 sequence() const;
        /** GPU compute-pass duration after success, when the device was created with timestamps enabled. */
        std::optional<quint64> gpuComputeNanoseconds() const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<CompletionData> d;
    };

    struct Edit {
        Error error = Error::None;
        Version version;
        Completion completion;
    };
    struct Readback {
        Error error = Error::None;
        Completion completion;
        /** Tightly packed straight RGBA8; available only after successful completion. */
        QByteArray bytes() const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<ReadbackData> d;
    };
    struct TextureSnapshot {
        Error error = Error::None;
        Completion completion;
        /** Borrowed read-only straight RGBA8 texture. Retain this snapshot through every GPU consumer. */
        WGPUTexture texture() const;
        QRect bounds() const;
        /** True when this image can be consumed on the given device. */
        bool usesDevice(const KisGpuDevice &device) const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<TextureData> d;
    };
    struct Statistics {
        quint64 residentBytes = 0;
        quint64 commandUploadBytes = 0;
        quint64 tileCopyBytes = 0;
        quint64 submissions = 0;
        quint64 computeDispatches = 0;
        quint64 pixelReadbackBytes = 0;
        quint64 pixelUploadBytes = 0;
        quint64 timingReadbackBytes = 0;
        quint64 textureCopyBytes = 0;
    };

    /** Immutable prepared alpha8 pattern on this store; may outlive its owner. */
    class BrushTexture {
    public:
        BrushTexture() = default;
        explicit operator bool() const { return bool(d); }
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<BrushTextureData> d;
    };
    struct BrushTextureResult {
        Error error = Error::None;
        Completion completion;
        BrushTexture texture;
    };

    /**
     * Shares the device owner's loss state; stores may outlive that owner but become unavailable.
     * Unavailable device, unsupported limits, or zero pending limit throw std::runtime_error.
     */
    KisGpuTileStore(KisGpuDevice &device, quint64 budgetBytes, quint32 maximumPending = 256);
    ~KisGpuTileStore();
    KisGpuTileStore(const KisGpuTileStore &) = delete;
    KisGpuTileStore &operator=(const KisGpuTileStore &) = delete;

    Version emptyVersion() const;
    /**
     * Replace a rectangle with straight RGBA8 (R in the low byte) on the GPU.
     * The returned version shares untouched tiles with base. Empty rectangles
     * retain base's completion without submission. Rejection leaves base and the queue unchanged.
     * Callers adopt the returned version only after its completion succeeds.
     */
    Edit fill(const Version &base, QRect rectangle, quint32 rgba);
    /** Composite a constant source with uniform opacity and selection coverage. */
    Edit paint(const Version &base, QRect rectangle, quint32 rgba,
               CompositeOp operation = CompositeOp::Over, quint8 opacity = 255, quint8 coverage = 255);
    /** Apply commands in order in one version/submission, copying each changed tile once. */
    Edit paint(const Version &base, const QVector<PaintCommand> &commands);
    /** Generate ordered ellipse dabs on the GPU. Diameter > 0, fade in (0,1], finite values. */
    Edit paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip);
    /** Apply the alpha of an immutable GPU selection to each dab; absent selection tiles have zero coverage. */
    Edit paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip, const Version &selection);
    /** Repeat the prepared pattern at integer canvas coordinates before opacity/selection blending. */
    Edit paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip,
                   const BrushTexture &texture, QPoint origin, const Version *selection = nullptr);
    /** Explicit one-time pattern upload, padded to four bytes; counts against pixel transfer and resident budgets. */
    BrushTextureResult uploadBrushTexture(QSize size, const QByteArray &alpha);
    /** Composite source pixels at matching canvas coordinates; missing source tiles are transparent. */
    Edit composite(const Version &base, const Version &source, QRect rectangle,
                   CompositeOp operation = CompositeOp::Over, quint8 opacity = 255, quint8 coverage = 255);
    /** Recompose damaged canvas pixels from layers, sharing the rest of previous, in one submission. */
    Edit project(const Version &previous, const QVector<Layer> &layers, QRect damage);
    /** Composite using the alpha channel of a GPU mask; absent mask tiles have zero coverage. */
    Edit compositeMasked(const Version &base, const Version &source, const Version &mask, QRect rectangle,
                         CompositeOp operation = CompositeOp::Over, quint8 opacity = 255);
    /**
     * Explicit asynchronous CPU read. Pins source through completion, including pending edits.
     * Staging counts against the store budget and is released by poll(). Missing pixels are zero.
     * Call on the submitting thread; after success the CPU result may outlive the store.
     */
    Readback readback(const Version &source, QRect bounds);
    /**
     * Copy a nonempty canvas rectangle to an immutable GPU texture, clearing absent pixels.
     * Pins pending source pixels through completion. The texture counts against this store's
     * budget, may outlive the store, and supports sampling and explicit copying as an input.
     * Adopt only after completion succeeds; submission and collection use the owning thread.
     */
    TextureSnapshot textureSnapshot(const Version &source, QRect bounds);
    /** Import tightly packed RGBA8 pixels into a new version; CPU input may be released on return. */
    Edit upload(const Version &base, QRect bounds, const QByteArray &pixels);
    /**
     * Device owner reports loss from any thread while this store lives, before explicit destruction.
     * Synchronizes with mapped CPU reads; adoption and resource collection remain in poll().
     */
    void invalidateDevice();
    bool deviceAvailable() const;
    /** Dispatch completion callbacks and release finished submissions; never waits. */
    void poll();
    Statistics statistics() const;

private:
    enum class UpdateKind { Fill, Over, Erase, DabOver, DabErase };
    struct UpdateCommand {
        QRect rectangle;
        quint32 rgba;
        UpdateKind kind;
        quint8 opacity, coverage;
        QPointF center;
        QSizeF diameter, fade;
    };
    Edit compositePixels(const Version &base, const Version &source, const Version *mask, QRect rectangle,
                         CompositeOp operation, quint8 opacity, quint8 coverage);
    Edit paintDabCommands(const Version &base, const QVector<DabCommand> &commands, QRect clip, const Version *selection,
                          const BrushTexture *texture = nullptr, QPoint origin = {});
    Edit update(const Version &base, const QVector<UpdateCommand> &commands, const Version *selection = nullptr,
                const BrushTexture *texture = nullptr, QPoint origin = {});
    struct Private;
    std::unique_ptr<Private> d;
};

#endif
