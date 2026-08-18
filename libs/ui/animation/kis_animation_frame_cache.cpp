/*
 *  SPDX-FileCopyrightText: 2015 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/kis_animation_frame_cache.h"

#include "kis_debug.h"

#include "kis_image.h"
#include "kis_image_animation_interface.h"
#include "kis_time_span.h"
#include "KisPart.h"
#include "animation/kis_animation_cache_populator.h"

#include "animation/cache/KisAbstractFrameCacheSwapper.h"
#include "animation/cache/KisFrameCacheSwapper.h"
#include "animation/cache/KisInMemoryFrameCacheSwapper.h"

#include "kis_image_config.h"
#include "kis_config_notifier.h"

#include "opengl/kis_opengl_image_textures.h"

#include <animation/kis_animation_frame_cache_index.h>
#include <kis_algebra_2d.h>
#include <cmath>


struct KisAnimationFrameCache::Private
{
    Private(KisOpenGLImageTexturesSP _textures)
        : textures(_textures)
    {
        image = textures->image();
    }

    ~Private()
    {
    }

    KisOpenGLImageTexturesSP textures;
    KisImageWSP image;

    QScopedPointer<KisAbstractFrameCacheSwapper> swapper;
    int frameSizeLimit = 777;

    KisOpenGLUpdateInfoSP fetchFrameDataImpl(KisImageSP image, const QRect &requestedRect, int lod);

    struct Frame
    {
        KisOpenGLUpdateInfoSP openGlFrame;
        int length;

        Frame(KisOpenGLUpdateInfoSP info, int length)
            : openGlFrame(info), length(length)
        {}
    };

    KisAnimationFrameCacheIndex frameIndex;

    int getFrameIdAtTime(int time) const
    {
        return frameIndex.frameIdAtTime(time);
    }

    bool hasFrame(int time) const {
        return frameIndex.contains(time);
    }

    KisOpenGLUpdateInfoSP getFrame(int time)
    {
        const int frameId = getFrameIdAtTime(time);
        return frameId >= 0 ? swapper->loadFrame(frameId) : 0;
    }

    void addFrame(KisOpenGLUpdateInfoSP info, const KisTimeSpan& range)
    {
        applyStorageOperations(frameIndex.insert(range));
        swapper->saveFrame(range.start(), info, image->bounds());
    }

    void applyStorageOperations(const KisAnimationFrameCacheIndex::ChangeSet &changes)
    {
        for (const KisAnimationFrameCacheIndex::StorageOperation &operation : changes.storageOperations) {
            if (operation.type == KisAnimationFrameCacheIndex::StorageOperation::Move) {
                swapper->moveFrame(operation.sourceFrameId, operation.destinationFrameId);
            } else {
                swapper->forgetFrame(operation.sourceFrameId);
            }
        }
    }

    /**
     * Invalidate any cached frames within the given time range.
     * @param range
     * @return true if frames were invalidated, false if nothing was changed
     */
    bool invalidate(const KisTimeSpan& range)
    {
        const KisAnimationFrameCacheIndex::ChangeSet changes = frameIndex.invalidate(range);
        applyStorageOperations(changes);
        return changes.changed;
    }

    int effectiveLevelOfDetail(const QRect &rc) const {
        if (!frameSizeLimit) return 0;

        const int maxDimension = KisAlgebra2D::maxDimension(rc);

        const qreal minLod = -std::log2(qreal(frameSizeLimit) / maxDimension);
        const int lodLimit = qMax(0, qCeil(minLod));
        return lodLimit;
    }


    // TODO: verify that we don't have any leak here!
    typedef QMap<KisOpenGLImageTexturesSP, KisAnimationFrameCache*> CachesMap;
    static CachesMap caches;
};

KisAnimationFrameCache::Private::CachesMap KisAnimationFrameCache::Private::caches;

KisAnimationFrameCacheSP KisAnimationFrameCache::getFrameCache(KisOpenGLImageTexturesSP textures)
{
    KisAnimationFrameCache *cache;

    Private::CachesMap::iterator it = Private::caches.find(textures);
    if (it == Private::caches.end()) {
        cache = new KisAnimationFrameCache(textures);
        Private::caches.insert(textures, cache);
    } else {
        cache = it.value();
    }

    return cache;
}

const QList<KisAnimationFrameCache *> KisAnimationFrameCache::caches()
{
    return Private::caches.values();
}

const KisAnimationFrameCacheSP KisAnimationFrameCache::cacheForImage(KisImageWSP image)
{
    auto it = std::find_if(Private::caches.begin(), Private::caches.end(),
                           [image] (KisAnimationFrameCache *cache) { return cache->image() == image; });

    return it != Private::caches.end() ? *it : nullptr;
}

KisAnimationFrameCache::KisAnimationFrameCache(KisOpenGLImageTexturesSP textures)
    : m_d(new Private(textures))
{
    // create swapping backend
    slotConfigChanged();

    connect(m_d->image->animationInterface(), SIGNAL(sigFramesChanged(KisTimeSpan,QRect)), this, SLOT(framesChanged(KisTimeSpan,QRect)));
    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
}

KisAnimationFrameCache::~KisAnimationFrameCache()
{
    Private::caches.remove(m_d->textures);
}

bool KisAnimationFrameCache::uploadFrame(int time)
{
    KisOpenGLUpdateInfoSP info = m_d->getFrame(time);

    if (!info) {
        // Do nothing!
        //
        // Previously we were trying to start cache regeneration in this point,
        // but it caused even bigger slowdowns when scrubbing
    } else {
        m_d->textures->recalculateCache(info, false);
    }

    return bool(info);
}

bool KisAnimationFrameCache::shouldUploadNewFrame(int newTime, int oldTime) const
{
    return m_d->frameIndex.shouldUploadNewFrame(newTime, oldTime);
}

KisAnimationFrameCache::CacheStatus KisAnimationFrameCache::frameStatus(int time) const
{
    return m_d->hasFrame(time) ? Cached : Uncached;
}

bool KisAnimationFrameCache::tryGlueSameFrames(const KisTimeSpan &range)
{
    const KisAnimationFrameCacheIndex::ChangeSet changes = m_d->frameIndex.glue(range);
    m_d->applyStorageOperations(changes);

    if (changes.changed) {
        Q_EMIT changed();
    }

    return changes.changed;
}

KisImageWSP KisAnimationFrameCache::image()
{
    return m_d->image;
}

void KisAnimationFrameCache::framesChanged(const KisTimeSpan &range, const QRect &rect)
{
    Q_UNUSED(rect);

    if (!range.isValid()) return;

    bool cacheChanged = m_d->invalidate(range);

    if (cacheChanged) {
        Q_EMIT changed();
    }
}

void KisAnimationFrameCache::slotConfigChanged()
{
    m_d->frameIndex.clear();

    KisImageConfig cfg(true);

    if (cfg.useOnDiskAnimationCacheSwapping()) {
        m_d->swapper.reset(new KisFrameCacheSwapper(m_d->textures->updateInfoBuilder(), cfg.swapDir()));
    } else {
        m_d->swapper.reset(new KisInMemoryFrameCacheSwapper());
    }

    m_d->frameSizeLimit = cfg.useAnimationCacheFrameSizeLimit() ? cfg.animationCacheFrameSizeLimit() : 0;
    Q_EMIT changed();
}

KisOpenGLUpdateInfoSP KisAnimationFrameCache::Private::fetchFrameDataImpl(KisImageSP image, const QRect &requestedRect, int lod)
{
    if (lod > 0) {
        KisPaintDeviceSP tempDevice = new KisPaintDevice(image->projection()->colorSpace());
        tempDevice->prepareClone(image->projection());
        image->projection()->generateLodCloneDevice(tempDevice, image->projection()->extent(), lod);

        const QRect fetchRect = KisLodTransform::alignedRect(requestedRect, lod);
        return textures->updateInfoBuilder().buildUpdateInfo(fetchRect, tempDevice, image->bounds(), lod, true);
    } else {
        return textures->updateCache(requestedRect, image);
    }
}

KisOpenGLUpdateInfoSP KisAnimationFrameCache::fetchFrameData(int time, KisImageSP image, const KisRegion &requestedRegion) const
{
    if (time != image->animationInterface()->currentTime()) {
        qWarning() << "WARNING: KisAnimationFrameCache::frameReady image's time doesn't coincide with the requested time!";
        qWarning() << "    "  << ppVar(image->animationInterface()->currentTime()) << ppVar(time);
    }

    // the frames are always generated at full scale
    KIS_SAFE_ASSERT_RECOVER_NOOP(image->currentLevelOfDetail() == 0);

    const int lod = m_d->effectiveLevelOfDetail(requestedRegion.boundingRect());

    KisOpenGLUpdateInfoSP totalInfo;

    Q_FOREACH (const QRect &rc, requestedRegion.rects()) {
        KisOpenGLUpdateInfoSP info = m_d->fetchFrameDataImpl(image, rc, lod);
        if (!totalInfo) {
            totalInfo = info;
        } else {
            const bool result = totalInfo->tryMergeWith(*info);
            KIS_SAFE_ASSERT_RECOVER_NOOP(result);
        }
    }

    return totalInfo;
}

void KisAnimationFrameCache::addConvertedFrameData(KisOpenGLUpdateInfoSP info, int time)
{
    const KisTimeSpan identicalRange =
        KisTimeSpan::calculateIdenticalFramesRecursive(m_d->image->root(), time);

    m_d->addFrame(info, identicalRange);

    Q_EMIT changed();
}

void KisAnimationFrameCache::dropLowQualityFrames(const KisTimeSpan &range, const QRect &regionOfInterest, const QRect &minimalRect)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!range.isInfinite());
    if (m_d->frameIndex.isEmpty()) return;

    const QList<QPair<int, int>> frames = m_d->frameIndex.rangesIntersecting(range);
    for (const QPair<int, int> &frame : frames) {
        const int frameId = frame.first;
        const QRect frameRect = m_d->swapper->frameDirtyRect(frameId);
        const int frameLod = m_d->swapper->frameLevelOfDetail(frameId);

        if (frameLod > m_d->effectiveLevelOfDetail(regionOfInterest) || !frameRect.contains(minimalRect)) {
            m_d->swapper->forgetFrame(frameId);
            m_d->frameIndex.removeFrame(frameId);
        }
    }
}

bool KisAnimationFrameCache::framesHaveValidRoi(const KisTimeSpan &range, const QRect &regionOfInterest)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(!range.isInfinite(), false);
    const QList<QPair<int, int>> frames = m_d->frameIndex.rangesIntersecting(range);
    if (frames.isEmpty()) return false;

    int expectedNextTime = range.start();
    for (const QPair<int, int> &frame : frames) {
        const int frameId = frame.first;
        const int frameLength = frame.second;
        const int coveredStart = qMax(frameId, range.start());

        if (coveredStart != expectedNextTime) {
            return false;
        }

        if (!m_d->swapper->frameDirtyRect(frameId).contains(regionOfInterest)) {
            return false;
        }

        if (frameLength == -1 || frameId + frameLength - 1 >= range.end()) {
            return true;
        }

        expectedNextTime = frameId + frameLength;
    }

    return false;
}
