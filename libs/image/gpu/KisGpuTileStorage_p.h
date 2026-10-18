/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_TILE_STORAGE_P_H
#define KIS_GPU_TILE_STORAGE_P_H

#include "KisGpuTileRenderer.h"
#include "KisGpuPaintGeometry_p.h"
#include <QMap>
#include <QSet>
#include "KisGpuNativeResources_p.h"

namespace LibrePaint::Image::Gpu::Detail {
struct Budget { DevicePtr device; qsizetype limit = 0; qsizetype used = 0; };

struct Tile
{
    std::shared_ptr<Budget> budget;
    NativeBuffer pixels;
    bool mayContainHiddenRgb = true;
    Tile(NativeBuffer pixels, std::shared_ptr<Budget> budget)
        : budget(std::move(budget)), pixels(std::move(pixels))
    { this->budget->used += TileRenderer::TileBytes; }
    ~Tile() { budget->used -= TileRenderer::TileBytes; }
};
using Tiles = QMap<quint64, std::shared_ptr<Tile>>;
}

namespace LibrePaint::Image::Gpu {
struct RenderedVersion::Data
{
    Snapshot document;
    std::shared_ptr<Detail::Budget> owner;
    QMap<LayerId, Detail::Tiles> layers;
    QMap<LayerId, std::shared_ptr<Detail::Tile>> defaults;
    std::shared_ptr<Detail::Tile> background;
    QMap<LayerId, Detail::Tiles> strokeInputs;
    std::optional<QRect> region;
    QSet<LayerId> selected;
};

struct ProjectedVersion::Data
{
    struct Input {
        std::shared_ptr<Detail::Tile> pixels;
        quint8 opacity = 255;
        bool operator==(const Input &other) const { return pixels == other.pixels && opacity == other.opacity; }
    };
    RenderedSnapshot source;
    QRect region;
    bool directLayerProjection = false;
    Detail::Tiles tiles;
    QMap<quint64, QList<Input>> inputs;
};

struct TileRenderer::Private
{
    DevicePtr device;
    std::shared_ptr<Detail::Budget> budget;
    Detail::NativeBuffer zero;
    Detail::NativeBuffer unusedCoverage;
    Detail::NativeBindingLayout layout;
    Detail::NativeComputePipeline pipeline;
    Detail::NativeBindingLayout projectionLayout;
    Detail::NativeComputePipeline projectionPipeline;
    Detail::NativeBindingLayout transparencyLayout;
    Detail::NativeComputePipeline transparencyPipeline;
    bool zeroUploaded = false;
    QString error;
    QWeakPointer<const RenderedVersion> coverageVersion;
    LayerId coverageLayer = 0;
    Detail::Tiles coverage;

    Private(DevicePtr device, qsizetype byteBudget);
    Detail::NativeComputePipeline createPipeline(const QString &path);
    void initializeProjection();
    Detail::NativeBuffer recordTransparency(WGPUCommandEncoder commands, const ProjectedSnapshot &projection, QRect region);
    std::shared_ptr<Detail::Tile> allocateTile();
    bool ensureTileCapacity(qsizetype count);
};
}
#endif
