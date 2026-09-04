/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_strokes_queue.h"

#include <QTest>

#include <functional>
#include <type_traits>

namespace
{
#define ASSERT_STROKES_QUEUE_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisStrokesQueue::method)), signature>)
} // namespace

class KisStrokesQueueSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokesQueueTypeAndLifetimeSchemaRemainStable();
    void strokesQueueAdmissionAndCompletionSignaturesRemainStable();
    void strokesQueueStateAndProcessingSignaturesRemainStable();
    void strokesQueueLodAndFactorySignaturesRemainStable();
    void strokesQueueCallbacksAndDiagnosticsSignaturesRemainStable();
};

void KisStrokesQueueSchemaContractTest::strokesQueueTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisStrokesQueue>);
    static_assert(std::is_base_of_v<KisStrokesQueueMutatedJobInterface, KisStrokesQueue>);
    static_assert(std::is_default_constructible_v<KisStrokesQueue>);
    static_assert(std::is_destructible_v<KisStrokesQueue>);
}

void KisStrokesQueueSchemaContractTest::strokesQueueAdmissionAndCompletionSignaturesRemainStable()
{
    ASSERT_STROKES_QUEUE_SIGNATURE(startStroke, KisStrokeId (KisStrokesQueue::*)(KisStrokeStrategy *));
    ASSERT_STROKES_QUEUE_SIGNATURE(addJob, void (KisStrokesQueue::*)(KisStrokeId, KisStrokeJobData *));
    ASSERT_STROKES_QUEUE_SIGNATURE(addMutatedJobs, void (KisStrokesQueue::*)(KisStrokeId, QVector<KisStrokeJobData *>));
    ASSERT_STROKES_QUEUE_SIGNATURE(endStroke, void (KisStrokesQueue::*)(KisStrokeId));
    ASSERT_STROKES_QUEUE_SIGNATURE(cancelStroke, bool (KisStrokesQueue::*)(KisStrokeId));
    ASSERT_STROKES_QUEUE_SIGNATURE(tryCancelCurrentStrokeAsync, bool (KisStrokesQueue::*)());
    ASSERT_STROKES_QUEUE_SIGNATURE(tryUndoLastStrokeAsync, UndoResult (KisStrokesQueue::*)());
}

void KisStrokesQueueSchemaContractTest::strokesQueueStateAndProcessingSignaturesRemainStable()
{
    ASSERT_STROKES_QUEUE_SIGNATURE(balancingRatioOverride, qreal (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(currentStrokeName, KUndo2MagicString (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(hasOpenedStrokes, bool (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(isEmpty, bool (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(isIdle, bool (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(needsExclusiveAccess, bool (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(processQueue, void (KisStrokesQueue::*)(KisUpdaterContext &, bool));
    ASSERT_STROKES_QUEUE_SIGNATURE(sizeMetric, qint32 (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(wrapAroundModeSupported, bool (KisStrokesQueue::*)() const);
}

void KisStrokesQueueSchemaContractTest::strokesQueueLodAndFactorySignaturesRemainStable()
{
    ASSERT_STROKES_QUEUE_SIGNATURE(explicitRegenerateLevelOfDetail, void (KisStrokesQueue::*)());
    ASSERT_STROKES_QUEUE_SIGNATURE(lodNPostExecutionUndoAdapter,
                                   KisPostExecutionUndoAdapter * (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(lodPreferences, KisLodPreferences (KisStrokesQueue::*)() const);
    ASSERT_STROKES_QUEUE_SIGNATURE(setLod0ToNStrokeStrategyFactory,
                                   void (KisStrokesQueue::*)(const KisLodSyncStrokeStrategyFactory &));
    ASSERT_STROKES_QUEUE_SIGNATURE(setLodPreferences, void (KisStrokesQueue::*)(const KisLodPreferences &));
    ASSERT_STROKES_QUEUE_SIGNATURE(setSuspendResumeUpdatesStrokeStrategyFactory,
                                   void (KisStrokesQueue::*)(const KisSuspendResumeStrategyPairFactory &));
}

void KisStrokesQueueSchemaContractTest::strokesQueueCallbacksAndDiagnosticsSignaturesRemainStable()
{
    using Callback = std::function<void()>;

    ASSERT_STROKES_QUEUE_SIGNATURE(setPostSyncLod0GUIPlaneRequestForResumeCallback,
                                   void (KisStrokesQueue::*)(const Callback &));
    ASSERT_STROKES_QUEUE_SIGNATURE(setPurgeRedoStateCallback, void (KisStrokesQueue::*)(const Callback &));
    ASSERT_STROKES_QUEUE_SIGNATURE(notifyUFOChangedImage, void (KisStrokesQueue::*)());
    ASSERT_STROKES_QUEUE_SIGNATURE(debugDumpAllStrokes, void (KisStrokesQueue::*)());
}

QTEST_APPLESS_MAIN(KisStrokesQueueSchemaContractTest)

#include "KisStrokesQueueSchemaContractTest.moc"
