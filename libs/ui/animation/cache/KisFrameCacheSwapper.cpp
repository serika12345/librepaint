/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "animation/cache/KisFrameCacheSwapper.h"

#include <animation/kis_frame_cache_store.h>

#include "opengl/kis_opengl_update_info.h"
#include "opengl/KisOpenGLUpdateInfoBuilder.h"
#include "opengl/kis_texture_tile_update_info.h"

#include <kis_lod_transform.h>

#include <utility>

struct KisFrameCacheSwapper::Private
{
    Private(const KisOpenGLUpdateInfoBuilder &_builder, const QString &frameCachePath)
        : frameStore(frameCachePath),
          builder(_builder)
    {
    }

    KisFrameCacheStore frameStore;
    const KisOpenGLUpdateInfoBuilder &builder;
};

KisFrameCacheSwapper::KisFrameCacheSwapper(const KisOpenGLUpdateInfoBuilder &builder)
    : KisFrameCacheSwapper(builder, "")
{
}

KisFrameCacheSwapper::KisFrameCacheSwapper(const KisOpenGLUpdateInfoBuilder &builder, const QString &frameCachePath)
    : m_d(new Private(builder, frameCachePath))
{
}

KisFrameCacheSwapper::~KisFrameCacheSwapper()
{
}

void KisFrameCacheSwapper::saveFrame(int frameId, KisOpenGLUpdateInfoSP info, const QRect &imageBounds)
{
    KisFrameCacheStore::Frame frame;
    frame.dirtyImageRect = info->dirtyImageRect();
    frame.imageBounds = imageBounds;
    frame.levelOfDetail = info->levelOfDetail();

    for (const KisTextureTileUpdateInfoSP &tileInfo : std::as_const(info->tileList)) {
        if (!frame.data.pixelSize) {
            frame.data.pixelSize = tileInfo->pixelSize();
        } else {
            KIS_SAFE_ASSERT_RECOVER_RETURN(frame.data.pixelSize == tileInfo->pixelSize());
        }

        KisFrameDataSerializer::FrameTile tile {KisTileDataPoolSP()};
        tile.col = tileInfo->tileCol();
        tile.row = tileInfo->tileRow();
        tile.rect = tileInfo->realPatchRect();
        tile.data = std::move(tileInfo->takePixelData());
        KIS_SAFE_ASSERT_RECOVER(tile.data.data()) { continue; }
        frame.data.frameTiles.push_back(std::move(tile));
    }

    KIS_SAFE_ASSERT_RECOVER_RETURN(frame.isValid());
    m_d->frameStore.saveFrame(frameId, std::move(frame));
}

KisOpenGLUpdateInfoSP KisFrameCacheSwapper::loadFrame(int frameId)
{
    KisOpenGLUpdateInfoSP info = new KisOpenGLUpdateInfo();
    KisFrameCacheStore::Frame frame =
        m_d->frameStore.loadFrame(frameId, m_d->builder.tileDataPool());
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(frame.isValid(), info);

    info->assignDirtyImageRect(frame.dirtyImageRect);
    info->assignLevelOfDetail(frame.levelOfDetail);

    for (KisFrameDataSerializer::FrameTile &tile : frame.data.frameTiles) {
        const QRect patchRect = frame.levelOfDetail
            ? KisLodTransform::upscaledRect(tile.rect, frame.levelOfDetail)
            : tile.rect;
        const QRect fullSizeTileRect =
            m_d->builder.calculatePhysicalTileRect(tile.col, tile.row,
                                                   frame.imageBounds,
                                                   frame.levelOfDetail);

        KisTextureTileUpdateInfoSP tileInfo(
            new KisTextureTileUpdateInfo(tile.col, tile.row,
                                         fullSizeTileRect, patchRect,
                                         frame.imageBounds,
                                         frame.levelOfDetail,
                                         m_d->builder.tileDataPool()));
        tileInfo->putPixelData(std::move(tile.data), m_d->builder.destinationColorSpace());
        info->tileList << tileInfo;
    }

    return info;
}

void KisFrameCacheSwapper::moveFrame(int srcFrameId, int dstFrameId)
{
    m_d->frameStore.moveFrame(srcFrameId, dstFrameId);
}

void KisFrameCacheSwapper::forgetFrame(int frameId)
{
    m_d->frameStore.forgetFrame(frameId);
}

bool KisFrameCacheSwapper::hasFrame(int frameId) const
{
    return m_d->frameStore.hasFrame(frameId);
}

int KisFrameCacheSwapper::frameLevelOfDetail(int frameId) const
{
    return m_d->frameStore.frameLevelOfDetail(frameId);
}

QRect KisFrameCacheSwapper::frameDirtyRect(int frameId) const
{
    return m_d->frameStore.frameDirtyRect(frameId);
}
