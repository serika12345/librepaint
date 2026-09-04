/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/kis_animation_frame_cache.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAnimationFrameCache::method)), signature>)
} // namespace

class KisAnimationFrameCacheSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void animationFrameCacheTypeLifetimeAndStatusSchemaRemainStable();
    void animationFrameCacheLookupSignaturesRemainStable();
    void animationFrameCacheUploadAndStatusSignaturesRemainStable();
    void animationFrameCacheFrameDataSignaturesRemainStable();
    void animationFrameCacheRoiAndNotificationSignaturesRemainStable();
};

void KisAnimationFrameCacheSchemaContractTest::animationFrameCacheTypeLifetimeAndStatusSchemaRemainStable()
{
    static_assert(std::is_same_v<KisOpenGLImageTexturesSP, KisSharedPtr<KisOpenGLImageTextures>>);
    static_assert(std::is_class_v<KisAnimationFrameCache>);
    static_assert(std::is_enum_v<KisAnimationFrameCache::CacheStatus>);
    static_assert(static_cast<int>(KisAnimationFrameCache::CacheStatus::Cached) == 0);
    static_assert(static_cast<int>(KisAnimationFrameCache::CacheStatus::Uncached) == 1);
    static_assert(std::is_constructible_v<KisAnimationFrameCache, KisOpenGLImageTexturesSP>);
    static_assert(std::has_virtual_destructor_v<KisAnimationFrameCache>);
}

void KisAnimationFrameCacheSchemaContractTest::animationFrameCacheLookupSignaturesRemainStable()
{
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(getFrameCache, KisAnimationFrameCacheSP (*)(KisOpenGLImageTexturesSP));
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(caches, const QList<KisAnimationFrameCache *> (*)());
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(cacheForImage, const KisAnimationFrameCacheSP (*)(KisImageWSP));
}

void KisAnimationFrameCacheSchemaContractTest::animationFrameCacheUploadAndStatusSignaturesRemainStable()
{
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(uploadFrame, bool (KisAnimationFrameCache::*)(int));
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(shouldUploadNewFrame, bool (KisAnimationFrameCache::*)(int, int) const);
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(frameStatus,
                                           KisAnimationFrameCache::CacheStatus (KisAnimationFrameCache::*)(int) const);
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(tryGlueSameFrames, bool (KisAnimationFrameCache::*)(const KisTimeSpan &));
}

void KisAnimationFrameCacheSchemaContractTest::animationFrameCacheFrameDataSignaturesRemainStable()
{
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(image, KisImageWSP (KisAnimationFrameCache::*)());
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(
        fetchFrameData,
        KisOpenGLUpdateInfoSP (KisAnimationFrameCache::*)(int, KisImageSP, const KisRegion &) const);
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(addConvertedFrameData,
                                           void (KisAnimationFrameCache::*)(KisOpenGLUpdateInfoSP, int));
}

void KisAnimationFrameCacheSchemaContractTest::animationFrameCacheRoiAndNotificationSignaturesRemainStable()
{
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(
        dropLowQualityFrames,
        void (KisAnimationFrameCache::*)(const KisTimeSpan &, const QRect &, const QRect &));
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(framesHaveValidRoi,
                                           bool (KisAnimationFrameCache::*)(const KisTimeSpan &, const QRect &));
    ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(changed, void (KisAnimationFrameCache::*)());
}

QTEST_APPLESS_MAIN(KisAnimationFrameCacheSchemaContractTest)

#include "KisAnimationFrameCacheSchemaContractTest.moc"
