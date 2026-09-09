/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/KisPlaybackEngineQT.h"
#include "animation/cache/KisFrameCacheSwapper.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(method, ...)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPlaybackEngineQT::method)), __VA_ARGS__>)
#define ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(method, ...)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisFrameCacheSwapper::method)), __VA_ARGS__>)

} // namespace

class KisPlaybackEngineQtSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void seekMuteAndDropPolicySignaturesRemainStable();
    void capabilityRateAndStatisticsSignaturesRemainStable();
    void frameCacheSwapperTypeConstructionAndLifetimeSchemaRemainStable();
    void frameCacheSwapperStorageSignaturesRemainStable();
    void frameCacheSwapperMetadataSignaturesRemainStable();
};

void KisPlaybackEngineQtSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPlaybackEngineQT>);
    static_assert(std::is_base_of_v<KisPlaybackEngine, KisPlaybackEngineQT>);
    static_assert(std::is_constructible_v<KisPlaybackEngineQT, QObject *>);
    static_assert(std::is_default_constructible_v<KisPlaybackEngineQT>);
    static_assert(std::has_virtual_destructor_v<KisPlaybackEngineQT>);

    QVERIFY(true);
}

void KisPlaybackEngineQtSchemaContractTest::seekMuteAndDropPolicySignaturesRemainStable()
{
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(seek, void (KisPlaybackEngineQT::*)(int, SeekOptionFlags));
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(setMute, void (KisPlaybackEngineQT::*)(bool));
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(isMute, bool (KisPlaybackEngineQT::*)());
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(setDropFramesMode, void (KisPlaybackEngineQT::*)(bool));
    static_assert(std::is_same_v<decltype(std::declval<KisPlaybackEngineQT &>().seek(0)), void>);
}

void KisPlaybackEngineQtSchemaContractTest::capabilityRateAndStatisticsSignaturesRemainStable()
{
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(supportsAudio, bool (KisPlaybackEngineQT::*)());
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(supportsVariablePlaybackSpeed, bool (KisPlaybackEngineQT::*)());
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(activeFramesPerSecond,
                                        boost::optional<int64_t> (KisPlaybackEngineQT::*)() const);
    ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE(playbackStatistics,
                                        KisPlaybackEngine::PlaybackStats (KisPlaybackEngineQT::*)() const);
}

void KisPlaybackEngineQtSchemaContractTest::frameCacheSwapperTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisFrameCacheSwapper>);
    static_assert(std::is_base_of_v<KisAbstractFrameCacheSwapper, KisFrameCacheSwapper>);
    static_assert(std::is_constructible_v<KisFrameCacheSwapper, const KisOpenGLUpdateInfoBuilder &>);
    static_assert(std::is_constructible_v<KisFrameCacheSwapper, const KisOpenGLUpdateInfoBuilder &, const QString &>);
    static_assert(std::has_virtual_destructor_v<KisFrameCacheSwapper>);

    QVERIFY(true);
}

void KisPlaybackEngineQtSchemaContractTest::frameCacheSwapperStorageSignaturesRemainStable()
{
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(saveFrame,
                                         void (KisFrameCacheSwapper::*)(int, KisOpenGLUpdateInfoSP, const QRect &));
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(loadFrame, KisOpenGLUpdateInfoSP (KisFrameCacheSwapper::*)(int));
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(moveFrame, void (KisFrameCacheSwapper::*)(int, int));
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(forgetFrame, void (KisFrameCacheSwapper::*)(int));
}

void KisPlaybackEngineQtSchemaContractTest::frameCacheSwapperMetadataSignaturesRemainStable()
{
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(hasFrame, bool (KisFrameCacheSwapper::*)(int) const);
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(frameLevelOfDetail, int (KisFrameCacheSwapper::*)(int) const);
    ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE(frameDirtyRect, QRect (KisFrameCacheSwapper::*)(int) const);
}

#undef ASSERT_FRAME_CACHE_SWAPPER_SIGNATURE
#undef ASSERT_QT_PLAYBACK_ENGINE_SIGNATURE

QTEST_APPLESS_MAIN(KisPlaybackEngineQtSchemaContractTest)

#include "KisPlaybackEngineQtSchemaContractTest.moc"
