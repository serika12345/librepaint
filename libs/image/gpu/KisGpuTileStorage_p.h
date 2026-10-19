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
    Detail::NativeBindingLayout boundsLayout;
    Detail::NativeComputePipeline boundsPipeline;
    Detail::NativeBindingLayout translationLayout;
    Detail::NativeComputePipeline translationPipeline;
    Detail::NativeBindingLayout thumbnailLayout;
    Detail::NativeComputePipeline thumbnailPipeline;
    Detail::NativeBindingLayout resizeLayout;
    Detail::NativeComputePipeline resizePipeline;
    bool zeroUploaded = false;
    QString error;
    QWeakPointer<const RenderedVersion> coverageVersion;
    LayerId coverageLayer = 0;
    Detail::Tiles coverage;

    Private(DevicePtr device, qsizetype byteBudget);
    Detail::NativeComputePipeline createPipeline(const QString &path);
    void initializeProjection();
    Detail::NativeBuffer recordTransparency(WGPUCommandEncoder commands, const ProjectedSnapshot &projection, QRect region);
    Detail::NativeBuffer recordBounds(WGPUCommandEncoder commands, const QList<TileView> &tiles,
                                     QRect region, const std::array<quint8, 4> &defaultPixel, bool includeTransparentColors);
    bool recordThumbnailSamples(WGPUCommandEncoder commands, WGPUBuffer source, WGPUBuffer output,
                                WGPUBuffer samples, WGPUBuffer parameters, quint32 count);
    bool recordResizePass(WGPUCommandEncoder commands, WGPUBuffer source, WGPUBuffer accumulated,
                          WGPUBuffer spans, WGPUBuffer weights, WGPUBuffer output,
                          WGPUBuffer parameters, quint32 count);
    std::shared_ptr<Detail::Tile> allocateTile();
    bool ensureTileCapacity(qsizetype count);
    struct TranslationResult {
        RenderOutcome::Code code;
        Detail::Tiles tiles;
        QString diagnostic;
    };
    TranslationResult translateTiles(WGPUCommandEncoder commands, const Detail::Tiles &source,
                                     const std::shared_ptr<Detail::Tile> &fallback, QPoint offset,
                                     QRect region, QList<std::shared_ptr<Detail::Tile>> &pins);
};
}
#endif
