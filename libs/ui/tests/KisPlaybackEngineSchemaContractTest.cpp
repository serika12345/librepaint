/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/KisPlaybackEngine.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PLAYBACK_ENGINE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPlaybackEngine::method)), signature>)

class PlaybackEngineConstructorProbe final : public KisPlaybackEngine
{
public:
    using KisPlaybackEngine::KisPlaybackEngine;

    void seek(int frameIndex, SeekOptionFlags options) override;
    void setMute(bool value) override;
    bool isMute() override;
    bool supportsAudio() override;
    bool supportsVariablePlaybackSpeed() override;
    PlaybackStats playbackStatistics() const override;

protected:
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;
};
} // namespace

class KisPlaybackEngineSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void playbackEngineTypeLifetimeAndSeekPolicySchemaRemainsStable();
    void playbackStatisticsValueSchemaRemainsStable();
    void playbackTransportAndSeekSignaturesRemainStable();
    void playbackKeyframeNavigationSignaturesRemainStable();
    void playbackAudioDropPolicyAndNotificationSignaturesRemainStable();
};

void KisPlaybackEngineSchemaContractTest::playbackEngineTypeLifetimeAndSeekPolicySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisPlaybackEngine>);
    static_assert(std::is_base_of_v<QObject, KisPlaybackEngine>);
    static_assert(std::is_base_of_v<KoCanvasObserverBase, KisPlaybackEngine>);
    static_assert(std::is_abstract_v<KisPlaybackEngine>);
    static_assert(std::has_virtual_destructor_v<KisPlaybackEngine>);
    static_assert(std::is_constructible_v<PlaybackEngineConstructorProbe, QObject *>);
    static_assert(std::is_default_constructible_v<PlaybackEngineConstructorProbe>);

    static_assert(std::is_enum_v<SeekOption>);
    static_assert(std::is_same_v<SeekOptionFlags, QFlags<SeekOption>>);
    static_assert(SEEK_NONE == 0);
    static_assert(SEEK_PUSH_AUDIO == 1);
    static_assert(SEEK_FINALIZE == 2);
}

void KisPlaybackEngineSchemaContractTest::playbackStatisticsValueSchemaRemainsStable()
{
    using PlaybackStats = KisPlaybackEngine::PlaybackStats;

    static_assert(std::is_aggregate_v<PlaybackStats>);
    static_assert(std::is_same_v<decltype(PlaybackStats::expectedFps), qreal>);
    static_assert(std::is_same_v<decltype(PlaybackStats::realFps), qreal>);
    static_assert(std::is_same_v<decltype(PlaybackStats::droppedFramesPortion), qreal>);

    PlaybackStats original;
    QCOMPARE(original.expectedFps, 0.0);
    QCOMPARE(original.realFps, 0.0);
    QCOMPARE(original.droppedFramesPortion, 0.0);

    PlaybackStats copy = original;
    copy.realFps = 24.0;
    QCOMPARE(original.realFps, 0.0);
    QCOMPARE(copy.realFps, 24.0);
}

void KisPlaybackEngineSchemaContractTest::playbackTransportAndSeekSignaturesRemainStable()
{
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(play, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(pause, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(playPause, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(stop, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(seek, void (KisPlaybackEngine::*)(int, SeekOptionFlags));
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(previousFrame, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(nextFrame, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(firstFrame, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(lastFrame, void (KisPlaybackEngine::*)());

    static_assert(std::is_same_v<decltype(std::declval<KisPlaybackEngine &>().seek(0)), void>);
}

void KisPlaybackEngineSchemaContractTest::playbackKeyframeNavigationSignaturesRemainStable()
{
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(previousKeyframe, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(nextKeyframe, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(previousMatchingKeyframe, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(nextMatchingKeyframe, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(previousUnfilteredKeyframe, void (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(nextUnfilteredKeyframe, void (KisPlaybackEngine::*)());
}

void KisPlaybackEngineSchemaContractTest::playbackAudioDropPolicyAndNotificationSignaturesRemainStable()
{
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(setMute, void (KisPlaybackEngine::*)(bool));
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(isMute, bool (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(setDropFramesMode, void (KisPlaybackEngine::*)(bool));
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(dropFrames, bool (KisPlaybackEngine::*)() const);
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(supportsAudio, bool (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(supportsVariablePlaybackSpeed, bool (KisPlaybackEngine::*)());
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(playbackStatistics,
                                     KisPlaybackEngine::PlaybackStats (KisPlaybackEngine::*)() const);
    ASSERT_PLAYBACK_ENGINE_SIGNATURE(sigDropFramesModeChanged, void (KisPlaybackEngine::*)(bool));
}

QTEST_GUILESS_MAIN(KisPlaybackEngineSchemaContractTest)

#include "KisPlaybackEngineSchemaContractTest.moc"
