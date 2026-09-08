/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/KisAsyncAnimationRendererBase.h"
#include "animation/kis_animation_frame_cache.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ANIMATION_FRAME_CACHE_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAnimationFrameCache::method)), signature>)
#define ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAsyncAnimationRendererBase::method)), signature>)

class AsyncRendererConstructorProbe final : public KisAsyncAnimationRendererBase
{
public:
    using KisAsyncAnimationRendererBase::KisAsyncAnimationRendererBase;

    void frameCompletedCallback(int frame, const KisRegion &requestedRegion) override;
    void frameCancelledCallback(int frame, CancelReason cancelReason) override;
};
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
    void asyncRendererTypeAndFlagSchemaRemainStable();
    void asyncRendererCancellationReasonSchemaRemainStable();
    void asyncRendererConstructionAndRegenerationSignaturesRemainStable();
    void asyncRendererStateControlAndNotificationSignaturesRemainStable();
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

void KisAnimationFrameCacheSchemaContractTest::asyncRendererTypeAndFlagSchemaRemainStable()
{
    using Renderer = KisAsyncAnimationRendererBase;

    static_assert(std::is_class_v<Renderer>);
    static_assert(std::is_enum_v<Renderer::Flag>);
    static_assert(std::is_same_v<Renderer::Flags, QFlags<Renderer::Flag>>);
    static_assert(Renderer::None == 0x0);
    static_assert(Renderer::Cancellable == 0x1);
}

void KisAnimationFrameCacheSchemaContractTest::asyncRendererCancellationReasonSchemaRemainStable()
{
    using Renderer = KisAsyncAnimationRendererBase;

    static_assert(std::is_enum_v<Renderer::CancelReason>);
    static_assert(Renderer::UserCancelled == 0);
    static_assert(Renderer::RenderingFailed == 1);
    static_assert(Renderer::RenderingTimedOut == 2);
}

void KisAnimationFrameCacheSchemaContractTest::asyncRendererConstructionAndRegenerationSignaturesRemainStable()
{
    using Renderer = KisAsyncAnimationRendererBase;

    static_assert(std::is_abstract_v<Renderer>);
    static_assert(std::is_constructible_v<AsyncRendererConstructorProbe, QObject *>);
    static_assert(std::has_virtual_destructor_v<Renderer>);
    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(
        startFrameRegeneration,
        void (Renderer::*)(KisImageSP, int, const KisRegion &, Renderer::Flags, KisLockFrameGenerationLock &&));
    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(
        startFrameRegeneration,
        void (Renderer::*)(KisImageSP, int, Renderer::Flags, KisLockFrameGenerationLock &&));
}

void KisAnimationFrameCacheSchemaContractTest::asyncRendererStateControlAndNotificationSignaturesRemainStable()
{
    using Renderer = KisAsyncAnimationRendererBase;

    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(isActive, bool (Renderer::*)() const);
    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(cancelCurrentFrameRendering, void (Renderer::*)(Renderer::CancelReason));
    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(sigFrameCompleted, void (Renderer::*)(int));
    ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE(sigFrameCancelled, void (Renderer::*)(int, Renderer::CancelReason));
}

#undef ASSERT_ASYNC_ANIMATION_RENDERER_SIGNATURE

QTEST_APPLESS_MAIN(KisAnimationFrameCacheSchemaContractTest)

#include "KisAnimationFrameCacheSchemaContractTest.moc"
