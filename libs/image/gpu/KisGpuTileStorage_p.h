/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_TILE_STORAGE_P_H
#define KIS_GPU_TILE_STORAGE_P_H

#include "KisGpuTileRenderer.h"
#include <QMap>
#include <QSet>
#include <rhi/qrhi.h>

namespace Krita::Image::Gpu::Detail {
struct Budget { DevicePtr device; qsizetype limit = 0; qsizetype used = 0; };

template<class T> struct ResourceDeleter
{
    QRhi *rhi = nullptr;
    void operator()(T *resource) const
    {
        if (!resource) return;
        if (rhi->isRecordingFrame()) resource->deleteLater();
        else delete resource;
    }
};
template<class T> using Resource = std::unique_ptr<T, ResourceDeleter<T>>;
template<class T> Resource<T> own(QRhi &rhi, T *resource) { return {resource, {&rhi}}; }

struct Tile
{
    std::shared_ptr<Budget> budget;
    Resource<QRhiBuffer> pixels;
    Tile(Resource<QRhiBuffer> pixels, std::shared_ptr<Budget> budget)
        : budget(std::move(budget)), pixels(std::move(pixels))
    { this->budget->used += TileRenderer::TileBytes; }
    ~Tile() { budget->used -= TileRenderer::TileBytes; }
};
using Tiles = QMap<quint64, std::shared_ptr<Tile>>;
inline quint64 key(int x, int y) { return (quint64(quint32(y)) << 32) | quint32(x); }
inline QPoint coordinate(quint64 key) { return {int(quint32(key)), int(quint32(key >> 32))}; }
}

namespace Krita::Image::Gpu {
struct RenderedVersion::Data
{
    Snapshot document;
    std::shared_ptr<Detail::Budget> owner;
    QMap<LayerId, Detail::Tiles> layers;
    QMap<LayerId, std::shared_ptr<Detail::Tile>> defaults;
    std::optional<QRect> region;
    QSet<LayerId> selected;
};

struct ProjectedVersion::Data
{
    RenderedSnapshot source;
    QRect region;
    Detail::Tiles tiles;
    QMap<quint64, QList<std::shared_ptr<Detail::Tile>>> inputs;
};

struct TileRenderer::Private
{
    DevicePtr device;
    QRhi &rhi;
    std::shared_ptr<Detail::Budget> budget;
    Detail::Resource<QRhiBuffer> zero;
    Detail::Resource<QRhiBuffer> layoutParameters;
    Detail::Resource<QRhiShaderResourceBindings> layout;
    Detail::Resource<QRhiComputePipeline> pipeline;
    Detail::Resource<QRhiShaderResourceBindings> projectionLayout;
    Detail::Resource<QRhiComputePipeline> projectionPipeline;
    bool zeroUploaded = false;
    QString error;

    Private(DevicePtr device, qsizetype byteBudget);
    void initializeProjection();
    std::shared_ptr<Detail::Tile> allocateTile();
};
}
#endif
