/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TILE_STORE_H
#define KIS_GPU_TILE_STORE_H

#include <QPoint>
#include <QRect>
#include <memory>
#include <webgpu/wgpu.h>

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
public:
    static constexpr quint64 TileBytes = 64 * 64 * 4;
    enum class Status { Pending, Succeeded, Failed };
    enum class Error { None, InvalidVersion, BudgetExceeded };
    enum class CompositeOp { Over, Erase };

    class Version {
    public:
        Version() = default;
        qsizetype tileCount() const;
        /** Borrowed for read-only GPU use; retain this version through completion. */
        WGPUBuffer tile(QPoint coordinate) const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<const VersionData> d;
    };

    class Completion {
    public:
        Completion() = default;
        Status status() const;
        quint64 sequence() const;
    private:
        friend class KisGpuTileStore;
        std::shared_ptr<CompletionData> d;
    };

    struct Edit {
        Error error = Error::None;
        Version version;
        Completion completion;
    };
    struct Statistics {
        quint64 residentBytes = 0;
        quint64 commandUploadBytes = 0;
        quint64 tileCopyBytes = 0;
        quint64 submissions = 0;
    };

    /** Null device/resources or API/limits mismatch throw std::runtime_error; requires wgpu-native 27.0.4.0. */
    KisGpuTileStore(WGPUDevice device, quint64 budgetBytes);
    ~KisGpuTileStore();
    KisGpuTileStore(const KisGpuTileStore &) = delete;
    KisGpuTileStore &operator=(const KisGpuTileStore &) = delete;

    Version emptyVersion() const;
    /**
     * Replace a rectangle with straight RGBA8 (R in the low byte) on the GPU.
     * The returned version shares untouched tiles with base. Empty rectangles
     * retain base's completion without submission. Rejection leaves base and the queue unchanged.
     * A version is accepted only after its completion succeeds.
     */
    Edit fill(const Version &base, QRect rectangle, quint32 rgba);
    /** Composite a constant source with uniform opacity and selection coverage. */
    Edit paint(const Version &base, QRect rectangle, quint32 rgba,
               CompositeOp operation = CompositeOp::Over, quint8 opacity = 255, quint8 coverage = 255);
    /** Dispatch completion callbacks and release finished submissions; never waits. */
    void poll();
    Statistics statistics() const;

private:
    enum class UpdateKind { Fill, Over, Erase };
    Edit update(const Version &base, QRect rectangle, quint32 rgba,
                UpdateKind kind, quint8 opacity, quint8 coverage);
    struct Private;
    std::unique_ptr<Private> d;
};

#endif
