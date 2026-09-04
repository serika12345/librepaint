/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_update_scheduler.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_UPDATE_SCHEDULER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisUpdateScheduler::method)), signature>)
#define ASSERT_TESTABLE_UPDATE_SCHEDULER_SIGNATURE(method, signature)                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTestableUpdateScheduler::method)), signature>)
} // namespace

class KisUpdateSchedulerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateSchedulerTypeLifetimeAndThreadSchemaRemainStable();
    void updateSchedulerQueueControlSignaturesRemainStable();
    void updateSchedulerProjectionAndStrokeSignaturesRemainStable();
    void updateSchedulerLodConfigurationSignaturesRemainStable();
    void updateSchedulerCallbackCancellationAndTestableSchemaRemainStable();
};

void KisUpdateSchedulerSchemaContractTest::updateSchedulerTypeLifetimeAndThreadSchemaRemainStable()
{
    static_assert(std::is_class_v<KisUpdateScheduler>);
    static_assert(std::is_base_of_v<QObject, KisUpdateScheduler>);
    static_assert(std::is_base_of_v<KisStrokesFacade, KisUpdateScheduler>);
    static_assert(std::is_constructible_v<KisUpdateScheduler, KisProjectionUpdateListener *, QObject *>);
    static_assert(std::is_same_v<decltype(KisUpdateScheduler(std::declval<KisProjectionUpdateListener *>())),
                                 KisUpdateScheduler>);
    static_assert(std::has_virtual_destructor_v<KisUpdateScheduler>);

    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setThreadsLimit, void (KisUpdateScheduler::*)(int));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(threadsLimit, int (KisUpdateScheduler::*)() const);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setProgressProxy, void (KisUpdateScheduler::*)(KoProgressProxy *));
}

void KisUpdateSchedulerSchemaContractTest::updateSchedulerQueueControlSignaturesRemainStable()
{
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(immediateLockForReadOnly, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(unlock, void (KisUpdateScheduler::*)(bool));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(waitForDone, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(barrierLock, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(tryBarrierLock, bool (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(isIdle, bool (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(blockUpdates, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(unblockUpdates, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(addSpontaneousJob, void (KisUpdateScheduler::*)(KisSpontaneousJob *));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(hasUpdatesRunning, bool (KisUpdateScheduler::*)() const);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(doSomeUsefulWork, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(spareThreadAppeared, void (KisUpdateScheduler::*)());

    static_assert(std::is_same_v<decltype(std::declval<KisUpdateScheduler &>().unlock()), void>);
}

void KisUpdateSchedulerSchemaContractTest::updateSchedulerProjectionAndStrokeSignaturesRemainStable()
{
    using RectListSignature =
        void (KisUpdateScheduler::*)(KisNodeSP, const QVector<QRect> &, const QRect &, KisProjectionUpdateFlags);
    using RectSignature = void (KisUpdateScheduler::*)(KisNodeSP, const QRect &, const QRect &);

    ASSERT_UPDATE_SCHEDULER_SIGNATURE(updateProjection, RectListSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(fullRefreshAsync, RectListSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(updateProjection, RectSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(fullRefreshAsync, RectSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(continueUpdate, void (KisUpdateScheduler::*)(const QRect &));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(startStroke, KisStrokeId (KisUpdateScheduler::*)(KisStrokeStrategy *));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(addJob, void (KisUpdateScheduler::*)(KisStrokeId, KisStrokeJobData *));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(endStroke, void (KisUpdateScheduler::*)(KisStrokeId));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(cancelStroke, bool (KisUpdateScheduler::*)(KisStrokeId));
}

void KisUpdateSchedulerSchemaContractTest::updateSchedulerLodConfigurationSignaturesRemainStable()
{
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setLodPreferences, void (KisUpdateScheduler::*)(const KisLodPreferences &));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(lodPreferences, KisLodPreferences (KisUpdateScheduler::*)() const);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(explicitRegenerateLevelOfDetail, void (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setLod0ToNStrokeStrategyFactory,
                                      void (KisUpdateScheduler::*)(const KisLodSyncStrokeStrategyFactory &));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setSuspendResumeUpdatesStrokeStrategyFactory,
                                      void (KisUpdateScheduler::*)(const KisSuspendResumeStrategyPairFactory &));
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(currentLevelOfDetail, int (KisUpdateScheduler::*)() const);
}

void KisUpdateSchedulerSchemaContractTest::updateSchedulerCallbackCancellationAndTestableSchemaRemainStable()
{
    using CallbackSignature = void (KisUpdateScheduler::*)(const std::function<void()> &);

    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setPurgeRedoStateCallback, CallbackSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(setPostSyncLod0GUIPlaneRequestForResumeCallback, CallbackSignature);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(lodNPostExecutionUndoAdapter,
                                      KisPostExecutionUndoAdapter * (KisUpdateScheduler::*)() const);
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(tryCancelCurrentStrokeAsync, bool (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(tryUndoLastStrokeAsync, UndoResult (KisUpdateScheduler::*)());
    ASSERT_UPDATE_SCHEDULER_SIGNATURE(wrapAroundModeSupported, bool (KisUpdateScheduler::*)() const);

    static_assert(std::is_class_v<KisTestableUpdateScheduler>);
    static_assert(std::is_base_of_v<KisUpdateScheduler, KisTestableUpdateScheduler>);
    static_assert(std::is_constructible_v<KisTestableUpdateScheduler, KisProjectionUpdateListener *, qint32>);
    ASSERT_TESTABLE_UPDATE_SCHEDULER_SIGNATURE(updaterContext, KisUpdaterContext * (KisTestableUpdateScheduler::*)());
}

QTEST_APPLESS_MAIN(KisUpdateSchedulerSchemaContractTest)

#include "KisUpdateSchedulerSchemaContractTest.moc"
