/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_TILE_RENDERER_H
#define KIS_GPU_TILE_RENDERER_H

#include "KisGpuOperationDocument.h"
#include "KisGpuDevice.h"
#include <QRect>
#include <memory>

class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;

namespace Krita::Image::Gpu {

struct TileView
{
    QPoint coordinate;
    QRhiBuffer *pixels = nullptr;
};

/** Immutable sparse RGBA8/sRGB layer pixels, ordered on the owner's GPU queue. */
class KRITAIMAGE_EXPORT RenderedVersion
{
public:
    Snapshot document() const;
    /** Borrowed buffers remain valid while this version is retained. */
    QList<TileView> tiles(LayerId layer) const;

private:
    friend class TileRenderer;
    struct Data;
    explicit RenderedVersion(std::shared_ptr<Data> data);
    std::shared_ptr<const Data> m_data;
};

using RenderedSnapshot = QSharedPointer<const RenderedVersion>;

struct RenderOutcome
{
    enum Code { Recorded, InvalidVersion, CapacityExceeded, DeviceFailure } code = DeviceFailure;
    RenderedSnapshot version;
    QString diagnostic;
};

/** Immutable merged tiles for a pinned document version and requested region. */
class KRITAIMAGE_EXPORT ProjectedVersion
{
public:
    Snapshot document() const;
    QRect region() const;
    QList<TileView> tiles() const;

private:
    friend class TileRenderer;
    struct Data;
    explicit ProjectedVersion(std::shared_ptr<Data> data);
    std::shared_ptr<const Data> m_data;
};

using ProjectedSnapshot = QSharedPointer<const ProjectedVersion>;

struct ProjectionOutcome
{
    RenderOutcome::Code code = RenderOutcome::DeviceFailure;
    ProjectedSnapshot version;
    QString diagnostic;
};

/**
 * Generates sparse layer pixels from retained operations on a shared device.
 * The device, renderer and versions share one thread. A version can outlive
 * this renderer and keeps the device alive through its own last reader.
 *
 * record() joins the caller's active frame; it submits no commands, reads no
 * pixels and waits for no completion. Readers and presentation use that same
 * queue. The caller publishes a result after successful frame submission.
 * Fresh tiles preserve the input version, including on budget rejection.
 * Tile storage uses straight RGBA bytes, with 4x4 integer subpixel coverage.
 */
class KRITAIMAGE_EXPORT TileRenderer
{
public:
    static constexpr int TileSize = 256;
    static constexpr int TileBytes = TileSize * TileSize * 4;

    explicit TileRenderer(DevicePtr device, qsizetype byteBudget = 128 * 1024 * 1024);
    ~TileRenderer();
    TileRenderer(const TileRenderer &) = delete;
    TileRenderer &operator=(const TileRenderer &) = delete;

    RenderOutcome record(QRhiCommandBuffer &commands, const Snapshot &document,
                         const RenderedSnapshot &base = {});
    /** Merges visible layers in document order; shares unchanged projected tiles. */
    ProjectionOutcome compose(QRhiCommandBuffer &commands, const RenderedSnapshot &document,
                              QRect region, const ProjectedSnapshot &base = {});
    qsizetype residentBytes() const;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
