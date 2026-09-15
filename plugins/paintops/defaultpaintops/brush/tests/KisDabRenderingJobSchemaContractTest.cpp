/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDabRenderingExecutor.h"
#include "KisDabRenderingJob.h"
#include "KisDabRenderingQueue.h"
#include "KisDabRenderingQueueCache.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_DAB_JOB_SIGNATURE(type, method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class KisDabRenderingJobSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dabRenderingJobIdentityAndCopySchemaRemainsStable();
    void dabRenderingJobTypeSchemaRemainsStable();
    void dabRenderingJobStatusSchemaRemainsStable();
    void dabRenderingJobPayloadSchemaRemainsStable();
    void dabRenderingJobRunnerSchemaRemainsStable();
    void dabRenderingQueueTypeConstructionAndLifetimeSchemaRemainStable();
    void dabRenderingQueueCacheInterfaceSchemaRemainStable();
    void dabRenderingQueueSchedulingSignaturesRemainStable();
    void dabRenderingQueueCacheResourceSignaturesRemainStable();
    void dabRenderingQueueMetricsSignaturesRemainStable();
    void dabRenderingQueueCacheSchemaRemainStable();
    void dabRenderingExecutorSchemaRemainStable();
};

void KisDabRenderingJobSchemaContractTest::dabRenderingJobIdentityAndCopySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisDabRenderingJob>);
    static_assert(std::is_constructible_v<KisDabRenderingJob, int, KisDabRenderingJob::JobType>);
    static_assert(std::is_constructible_v<KisDabRenderingJob, int, KisDabRenderingJob::JobType, qreal, qreal>);
    static_assert(std::is_copy_constructible_v<KisDabRenderingJob>);
    ASSERT_DAB_JOB_SIGNATURE(KisDabRenderingJob,
                             operator=,
                             KisDabRenderingJob & (KisDabRenderingJob::*)(const KisDabRenderingJob &));
    ASSERT_DAB_JOB_SIGNATURE(KisDabRenderingJob, dstDabOffset, QPoint (KisDabRenderingJob::*)() const);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingJobTypeSchemaRemainsStable()
{
    static_assert(std::is_enum_v<KisDabRenderingJob::JobType>);
    static_assert(KisDabRenderingJob::Dab == 0);
    static_assert(KisDabRenderingJob::Postprocess == 1);
    static_assert(KisDabRenderingJob::Copy == 2);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingJobStatusSchemaRemainsStable()
{
    static_assert(std::is_enum_v<KisDabRenderingJob::Status>);
    static_assert(KisDabRenderingJob::New == 0);
    static_assert(KisDabRenderingJob::Running == 1);
    static_assert(KisDabRenderingJob::Completed == 2);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingJobPayloadSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisDabRenderingJobSP, QSharedPointer<KisDabRenderingJob>>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::seqNo), int>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::generationInfo), KisDabCacheUtils::DabGenerationInfo>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::type), KisDabRenderingJob::JobType>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::originalDevice), KisFixedPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::postprocessedDevice), KisFixedPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::status), KisDabRenderingJob::Status>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::opacity), qreal>);
    static_assert(std::is_same_v<decltype(KisDabRenderingJob::flow), qreal>);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingJobRunnerSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisDabRenderingJobRunner>);
    static_assert(std::is_base_of_v<QRunnable, KisDabRenderingJobRunner>);
    static_assert(std::is_constructible_v<KisDabRenderingJobRunner,
                                          KisDabRenderingJobSP,
                                          KisDabRenderingQueue *,
                                          KisRunnableStrokeJobsInterface *>);
    static_assert(std::is_destructible_v<KisDabRenderingJobRunner>);
    ASSERT_DAB_JOB_SIGNATURE(KisDabRenderingJobRunner, run, void (KisDabRenderingJobRunner::*)());
    ASSERT_DAB_JOB_SIGNATURE(
        KisDabRenderingJobRunner,
        executeOneJob,
        int (*)(KisDabRenderingJob *, KisDabCacheUtils::DabRenderingResources *, KisDabRenderingQueue *));

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Queue = KisDabRenderingQueue;

    static_assert(std::is_class_v<Queue>);
    static_assert(std::is_constructible_v<Queue, const KoColorSpace *, KisDabCacheUtils::ResourcesFactory>);
    static_assert(std::is_destructible_v<Queue>);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueCacheInterfaceSchemaRemainStable()
{
    using Cache = KisDabRenderingQueue::CacheInterface;
    using Resources = KisDabCacheUtils::DabRenderingResources;

    static_assert(std::is_class_v<Cache>);
    static_assert(std::is_abstract_v<Cache>);
    static_assert(std::has_virtual_destructor_v<Cache>);
    ASSERT_DAB_JOB_SIGNATURE(Cache,
                             getDabType,
                             void (Cache::*)(bool,
                                             Resources *,
                                             const KisDabCacheUtils::DabRequestInfo &,
                                             KisDabCacheUtils::DabGenerationInfo *,
                                             bool *));
    ASSERT_DAB_JOB_SIGNATURE(Cache, hasSeparateOriginal, bool (Cache::*)(Resources *) const);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueSchedulingSignaturesRemainStable()
{
    using Queue = KisDabRenderingQueue;

    ASSERT_DAB_JOB_SIGNATURE(Queue,
                             addDab,
                             KisDabRenderingJobSP (Queue::*)(const KisDabCacheUtils::DabRequestInfo &, qreal, qreal));
    ASSERT_DAB_JOB_SIGNATURE(Queue, notifyJobFinished, QList<KisDabRenderingJobSP> (Queue::*)(int, int));
    ASSERT_DAB_JOB_SIGNATURE(Queue, takeReadyDabs, QList<KisRenderedDab> (Queue::*)(bool, int, bool *));

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueCacheResourceSignaturesRemainStable()
{
    using Queue = KisDabRenderingQueue;
    using Cache = Queue::CacheInterface;
    using Resources = KisDabCacheUtils::DabRenderingResources;

    ASSERT_DAB_JOB_SIGNATURE(Queue, hasPreparedDabs, bool (Queue::*)() const);
    ASSERT_DAB_JOB_SIGNATURE(Queue, setCacheInterface, void (Queue::*)(Cache *));
    ASSERT_DAB_JOB_SIGNATURE(Queue, fetchCachedPaintDevice, KisFixedPaintDeviceSP (Queue::*)());
    ASSERT_DAB_JOB_SIGNATURE(Queue, putResourcesToCache, void (Queue::*)(Resources *));
    ASSERT_DAB_JOB_SIGNATURE(Queue, fetchResourcesFromCache, Resources * (Queue::*)());

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueMetricsSignaturesRemainStable()
{
    using Queue = KisDabRenderingQueue;

    ASSERT_DAB_JOB_SIGNATURE(Queue, averageExecutionTime, qreal (Queue::*)() const);
    ASSERT_DAB_JOB_SIGNATURE(Queue, averageDabSize, int (Queue::*)() const);
    ASSERT_DAB_JOB_SIGNATURE(Queue, testingGetQueueSize, int (Queue::*)() const);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingQueueCacheSchemaRemainStable()
{
    using Cache = KisDabRenderingQueueCache;
    using Resources = KisDabCacheUtils::DabRenderingResources;

    static_assert(std::is_class_v<Cache>);
    static_assert(std::is_base_of_v<KisDabRenderingQueue::CacheInterface, Cache>);
    static_assert(std::is_base_of_v<KisDabCacheBase, Cache>);
    static_assert(std::is_constructible_v<Cache>);
    static_assert(std::is_destructible_v<Cache>);
    ASSERT_DAB_JOB_SIGNATURE(Cache,
                             getDabType,
                             void (Cache::*)(bool,
                                             Resources *,
                                             const KisDabCacheUtils::DabRequestInfo &,
                                             KisDabCacheUtils::DabGenerationInfo *,
                                             bool *));
    ASSERT_DAB_JOB_SIGNATURE(Cache, hasSeparateOriginal, bool (Cache::*)(Resources *) const);

    QVERIFY(true);
}

void KisDabRenderingJobSchemaContractTest::dabRenderingExecutorSchemaRemainStable()
{
    using Executor = KisDabRenderingExecutor;

    static_assert(std::is_class_v<Executor>);
    static_assert(std::is_constructible_v<Executor,
                                          const KoColorSpace *,
                                          KisDabCacheUtils::ResourcesFactory,
                                          KisRunnableStrokeJobsInterface *,
                                          KisMirrorOption *,
                                          KisPrecisionOption *>);
    static_assert(std::is_destructible_v<Executor>);
    ASSERT_DAB_JOB_SIGNATURE(Executor,
                             addDab,
                             void (Executor::*)(const KisDabCacheUtils::DabRequestInfo &, qreal, qreal));
    ASSERT_DAB_JOB_SIGNATURE(Executor, takeReadyDabs, QList<KisRenderedDab> (Executor::*)(bool, int, bool *));
    ASSERT_DAB_JOB_SIGNATURE(Executor, hasPreparedDabs, bool (Executor::*)() const);
    ASSERT_DAB_JOB_SIGNATURE(Executor, averageDabRenderingTime, qreal (Executor::*)() const);
    ASSERT_DAB_JOB_SIGNATURE(Executor, averageDabSize, int (Executor::*)() const);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisDabRenderingJobSchemaContractTest)

#include "KisDabRenderingJobSchemaContractTest.moc"
