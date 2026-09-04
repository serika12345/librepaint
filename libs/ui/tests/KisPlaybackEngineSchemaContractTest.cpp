/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/KisPlaybackEngine.h>
#include <canvas/KisCanvasAnimationState.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PLAYBACK_ENGINE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPlaybackEngine::method)), signature>)

#define ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvasAnimationState::method)), signature>)

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
    void canvasAnimationStateTypeLifetimeAndPlaybackSchemaRemainStable();
    void canvasAnimationStatePlaybackAndMediaSignaturesRemainStable();
    void canvasAnimationStateSpeedFrameAndAudioSetupSignaturesRemainStable();
    void canvasAnimationStatePlaybackNotificationSignaturesRemainStable();
    void canvasAnimationStateAudioAndCancellationNotificationSignaturesRemainStable();
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

void KisPlaybackEngineSchemaContractTest::canvasAnimationStateTypeLifetimeAndPlaybackSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCanvasAnimationState>);
    static_assert(std::is_base_of_v<QObject, KisCanvasAnimationState>);
    static_assert(std::is_enum_v<PlaybackState>);
    static_assert(PAUSED == 1);
    static_assert(PLAYING == 2);
    static_assert(STOPPED == 0);
    static_assert(std::is_constructible_v<KisCanvasAnimationState, KisCanvas2 *>);
    static_assert(std::has_virtual_destructor_v<KisCanvasAnimationState>);
}

void KisPlaybackEngineSchemaContractTest::canvasAnimationStatePlaybackAndMediaSignaturesRemainStable()
{
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(currentVolume, qreal (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(displayProxy, KisFrameDisplayProxy * (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(mediaInfo, boost::optional<QFileInfo> (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(playbackOrigin, boost::optional<int> (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(playbackState, PlaybackState (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(setPlaybackState, void (KisCanvasAnimationState::*)(PlaybackState));
}

void KisPlaybackEngineSchemaContractTest::canvasAnimationStateSpeedFrameAndAudioSetupSignaturesRemainStable()
{
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(playbackSpeed, qreal (KisCanvasAnimationState::*)() const);
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(setPlaybackSpeed, void (KisCanvasAnimationState::*)(qreal));
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(setupAudioTracks, void (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(showFrame, void (KisCanvasAnimationState::*)(int, bool));

    using ShowFrameWithDefaultFinalize = decltype(std::declval<KisCanvasAnimationState &>().showFrame(0));
    static_assert(std::is_same_v<ShowFrameWithDefaultFinalize, void>);
}

void KisPlaybackEngineSchemaContractTest::canvasAnimationStatePlaybackNotificationSignaturesRemainStable()
{
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigFrameChanged, void (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigPlaybackMediaChanged, void (KisCanvasAnimationState::*)());
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigPlaybackSpeedChanged, void (KisCanvasAnimationState::*)(qreal));
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigPlaybackStateChanged, void (KisCanvasAnimationState::*)(PlaybackState));
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigPlaybackStatisticsUpdated, void (KisCanvasAnimationState::*)());
}

void KisPlaybackEngineSchemaContractTest::canvasAnimationStateAudioAndCancellationNotificationSignaturesRemainStable()
{
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigAudioLevelChanged, void (KisCanvasAnimationState::*)(qreal));
    ASSERT_CANVAS_ANIMATION_STATE_SIGNATURE(sigCancelPlayback, void (KisCanvasAnimationState::*)());
}

QTEST_GUILESS_MAIN(KisPlaybackEngineSchemaContractTest)

#include "KisPlaybackEngineSchemaContractTest.moc"
