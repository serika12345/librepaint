/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDabRenderingJob.h"

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

QTEST_GUILESS_MAIN(KisDabRenderingJobSchemaContractTest)

#include "KisDabRenderingJobSchemaContractTest.moc"
