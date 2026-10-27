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
#include <array>
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
    { this->budget->used += wgpuBufferGetSize(this->pixels.get()); }
    ~Tile() { budget->used -= wgpuBufferGetSize(pixels.get()); }
};
using Tiles = QMap<quint64, std::shared_ptr<Tile>>;
/** Alpha coverage tile uploaded for layer composition; one byte per pixel. */
struct CoverageTile
{
    std::shared_ptr<Budget> budget;
    NativeBuffer pixels;
    CoverageTile(NativeBuffer pixels, std::shared_ptr<Budget> budget)
        : budget(std::move(budget)), pixels(std::move(pixels))
    { this->budget->used += SelectionMask::AlphaTileBytes; }
    ~CoverageTile() { budget->used -= SelectionMask::AlphaTileBytes; }
};
using CoverageTiles = QMap<quint64, std::shared_ptr<CoverageTile>>;
struct ProjectionInput {
    std::shared_ptr<Tile> pixels;
    quint8 opacity = 255;
    quint8 channelMask = 15;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
    /** Null means the layer contributes without alpha coverage. */
    SelectionMaskSP mask;
    bool operator==(const ProjectionInput &other) const {
        return pixels == other.pixels && opacity == other.opacity && channelMask == other.channelMask &&
            blendMode == other.blendMode && mask == other.mask;
    }
};
using ProjectionInputs = QMap<quint64, QList<ProjectionInput>>;
}

namespace LibrePaint::Image::Gpu {
struct RenderedVersion::Data
{
    Snapshot document;
    std::shared_ptr<Detail::Budget> owner;
    QMap<LayerId, Detail::Tiles> layers;
    QMap<LayerId, std::shared_ptr<Detail::Tile>> defaults;
    QHash<LayerId, RgbaPixel> defaultPixels;
    QMap<LayerId, Detail::ProjectionInputs> groupInputs;
    QSet<LayerId> directGroups;
    std::shared_ptr<Detail::Tile> background;
    QMap<LayerId, Detail::Tiles> strokeInputs;
    std::optional<QRect> region;
    QSet<LayerId> selected;
};

struct ProjectedVersion::Data
{
    RenderedSnapshot source;
    QRect region;
    bool directLayerProjection = false;
    Detail::Tiles tiles;
    Detail::ProjectionInputs inputs;
};

struct TileRenderer::Private
{
    DevicePtr device;
    std::shared_ptr<Detail::Budget> budget;
    std::array<Detail::NativeBuffer, 2> zero;
    Detail::NativeBuffer unusedCoverage;
    Detail::NativeBuffer unusedSelection;
    Detail::NativeBuffer unusedPattern;
    /** Opaque 256 x 256 coverage bound for layers without a transparency mask. */
    Detail::NativeBuffer opaqueMask;
    struct MaskCacheEntry {
        SelectionMaskSP source;
        Detail::CoverageTiles tiles;
    };
    QHash<const SelectionMask *, MaskCacheEntry> maskTiles;
    Detail::NativeBindingLayout layout;
    Detail::NativeComputePipeline pipeline;
    Detail::NativeBindingLayout projectionLayout;
    Detail::NativeComputePipeline projectionPipeline;
    Detail::NativeBindingLayout pairedProjectionLayout;
    Detail::NativeComputePipeline pairedProjectionPipeline;
    Detail::NativeBindingLayout transparencyLayout;
    Detail::NativeComputePipeline transparencyPipeline;
    Detail::NativeBindingLayout boundsLayout;
    Detail::NativeComputePipeline boundsPipeline;
    Detail::NativeBindingLayout translationLayout;
    Detail::NativeComputePipeline translationPipeline;
    Detail::NativeBindingLayout cropLayout;
    Detail::NativeComputePipeline cropPipeline;
    Detail::NativeBindingLayout filterLayout;
    Detail::NativeComputePipeline filterPipeline;
    Detail::NativeBindingLayout thumbnailLayout;
    Detail::NativeComputePipeline thumbnailPipeline;
    Detail::NativeBindingLayout resizeLayout;
    Detail::NativeComputePipeline resizePipeline;
    Detail::NativeBindingLayout colorSampleLayout;
    Detail::NativeComputePipeline colorSamplePipeline;
    bool selectionZeroUploaded = false;
    QString error;
    QWeakPointer<const RenderedVersion> coverageVersion;
    LayerId coverageLayer = 0;
    Detail::Tiles coverage;
    QWeakPointer<const ProjectedVersion> backdropVersion;
    Detail::Tiles projectionBackdrops;

    Private(DevicePtr device, qsizetype byteBudget);
    Detail::NativeComputePipeline createPipeline(const QString &path, const char *entryPoint = "main");
    void initializeProjection();
    Detail::NativeBuffer recordTransparency(WGPUCommandEncoder commands, const ProjectedSnapshot &projection, QRect region);
    Detail::NativeBuffer recordBounds(WGPUCommandEncoder commands, const QList<TileView> &tiles,
                                     QRect region, const RgbaPixel &defaultPixel, PixelFormat format, bool includeTransparentColors);
    bool recordThumbnailSamples(WGPUCommandEncoder commands, WGPUBuffer source, WGPUBuffer output,
                                WGPUBuffer samples, WGPUBuffer parameters, quint32 count, PixelFormat format);
    bool recordColorSample(WGPUCommandEncoder commands, WGPUBuffer source, WGPUBuffer output,
                           WGPUBuffer parameters, QSize size, PixelFormat format);
    bool recordResizePass(WGPUCommandEncoder commands, WGPUBuffer source, WGPUBuffer accumulated,
                          WGPUBuffer spans, WGPUBuffer weights, WGPUBuffer output,
                          WGPUBuffer parameters, quint32 count);
    WGPUBuffer zeroBuffer(WGPUCommandEncoder commands, PixelFormat format);
    std::shared_ptr<Detail::Tile> allocateTile(PixelFormat format);
    bool ensureTileCapacity(qsizetype count, PixelFormat format);
    /** Uploads the mask's alpha tile for one document tile coordinate. */
    std::shared_ptr<Detail::CoverageTile> coverageTile(const SelectionMaskSP &mask, quint64 coordinate);
    void clearMaskTiles();
    struct TranslationResult {
        RenderOutcome::Code code;
        Detail::Tiles tiles;
        QString diagnostic;
    };
    TranslationResult translateTiles(WGPUCommandEncoder commands, const Detail::Tiles &source,
                                     const std::shared_ptr<Detail::Tile> &fallback, QPoint offset, PixelFormat format,
                                     QRect region, QList<std::shared_ptr<Detail::Tile>> &pins);
    struct CropResult {
        RenderOutcome::Code code;
        Detail::Tiles tiles;
        QString diagnostic;
    };
    CropResult cropTiles(WGPUCommandEncoder commands, const Detail::Tiles &source, QRect keep,
                        const RgbaPixel &defaultPixel, PixelFormat format, QRect region, QList<std::shared_ptr<Detail::Tile>> &pins);
    struct FilterResult {
        RenderOutcome::Code code;
        Detail::Tiles tiles;
        QString diagnostic;
    };
    /** Applies the frozen filter kind (Invert Colors, Grayscale or HSV
     * Adjustment) to RGB inside the frozen coverage for every existing tile
     * that intersects it. Each tile is transformed by its own compute pass, so
     * the filter never reads the layer back through the CPU.
     */
    FilterResult filterTiles(WGPUCommandEncoder commands, const Detail::Tiles &source,
                             const FilterOperation &filter, const std::shared_ptr<Detail::Tile> &fallback,
                             QSize documentSize, QRect region,
                             QList<std::shared_ptr<Detail::Tile>> &pins);
};
}
#endif
