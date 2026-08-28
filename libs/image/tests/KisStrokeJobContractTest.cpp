/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFakeRunnableStrokeJobsExecutor.h"
#include "KisRunnableStrokeJobData.h"
#include "KisRunnableStrokeJobDataBase.h"
#include "KisRunnableStrokeJobsInterface.h"
#include "kis_stroke_job_strategy.h"

#include <QByteArray>
#include <QRunnable>
#include <QTest>

namespace
{

QVector<QByteArray> safeAssertConditions;

class CopyableJobData : public KisStrokeJobData
{
public:
    using KisStrokeJobData::KisStrokeJobData;

    CopyableJobData(const CopyableJobData &rhs)
        : KisStrokeJobData(rhs)
    {
    }
};

class DestructionJobData : public KisStrokeJobData
{
public:
    explicit DestructionJobData(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~DestructionJobData() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class RecordingStrategy : public KisStrokeJobStrategy
{
public:
    RecordingStrategy(int *runCount, int *destructionCount)
        : m_runCount(runCount)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingStrategy() override
    {
        ++*m_destructionCount;
    }

    void run(KisStrokeJobData *data) override
    {
        m_lastData = data;
        ++*m_runCount;
    }

    QString debugId() const override
    {
        return QStringLiteral("recording-strategy");
    }

    KisStrokeJobData *lastData() const
    {
        return m_lastData;
    }

private:
    int *m_runCount;
    int *m_destructionCount;
    KisStrokeJobData *m_lastData = nullptr;
};

class RunnableJobData : public KisRunnableStrokeJobDataBase
{
public:
    RunnableJobData(Sequentiality sequentiality = SEQUENTIAL, Exclusivity exclusivity = NORMAL)
        : KisRunnableStrokeJobDataBase(sequentiality, exclusivity)
    {
    }

    void run() override
    {
        ++runCount;
    }

    int runCount = 0;
};

class TrackedRunnableJobData : public KisRunnableStrokeJobDataBase
{
public:
    TrackedRunnableJobData(int id,
                           QVector<int> *runOrder,
                           int *destructionCount,
                           Sequentiality sequentiality = SEQUENTIAL,
                           Exclusivity exclusivity = NORMAL)
        : KisRunnableStrokeJobDataBase(sequentiality, exclusivity)
        , m_id(id)
        , m_runOrder(runOrder)
        , m_destructionCount(destructionCount)
    {
    }

    ~TrackedRunnableJobData() override
    {
        ++*m_destructionCount;
    }

    void run() override
    {
        m_runOrder->append(m_id);
    }

private:
    int m_id;
    QVector<int> *m_runOrder;
    int *m_destructionCount;
};

class RecordingJobsInterface : public KisRunnableStrokeJobsInterface
{
public:
    using KisRunnableStrokeJobsInterface::addRunnableJobs;

    explicit RecordingJobsInterface(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingJobsInterface() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void addRunnableJobs(const QVector<KisRunnableStrokeJobDataBase *> &list) override
    {
        batches.append(list);
    }

    QVector<QVector<KisRunnableStrokeJobDataBase *>> batches;

private:
    int *m_destructionCount;
};

class RecordingQRunnable : public QRunnable
{
public:
    RecordingQRunnable(int *runCount, int *destructionCount)
        : m_runCount(runCount)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingQRunnable() override
    {
        ++*m_destructionCount;
    }

    void run() override
    {
        ++*m_runCount;
    }

private:
    int *m_runCount;
    int *m_destructionCount;
};

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    safeAssertConditions.append(assertion);
}

class KisStrokeJobContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultJobDataHasSequentialNormalDefaults();
    void jobModesExposeEverySchedulingClassification();
    void jobMutatorsAndCopyPreserveConfiguration();
    void jobDataDeletesThroughBase();
    void strategyRunsNamedJobsAndDeletesThroughBase();
    void runnableJobUsesDefaultAndRequestedScheduling();
    void functionRunnableRunsWithRequestedScheduling();
    void autoDeleteRunnableRunsAndDeletesWithJob();
    void retainedRunnableSurvivesJobDestruction();
    void singleRunnableJobDelegatesAsOneElementBatch();
    void derivedRunnableJobListConvertsAndPreservesOrder();
    void jobsInterfaceDeletesThroughBase();
    void fakeExecutorFlagValuesDescribeBarrierOptIn();
    void defaultFakeExecutorRunsAndDeletesJobsInOrder();
    void barrierFlagAllowsBarrierJob();
    void defaultFakeExecutorReportsUnsupportedJobs();
};

void KisStrokeJobContractTest::defaultJobDataHasSequentialNormalDefaults()
{
    KisStrokeJobData data;

    QCOMPARE(data.sequentiality(), KisStrokeJobData::SEQUENTIAL);
    QCOMPARE(data.exclusivity(), KisStrokeJobData::NORMAL);
    QVERIFY(data.isSequential());
    QVERIFY(!data.isBarrier());
    QVERIFY(!data.isExclusive());
    QVERIFY(data.isCancellable());
    QCOMPARE(data.levelOfDetailOverride(), -1);
    QCOMPARE(data.createLodClone(3), nullptr);
}

void KisStrokeJobContractTest::jobModesExposeEverySchedulingClassification()
{
    KisStrokeJobData concurrent(KisStrokeJobData::CONCURRENT);
    KisStrokeJobData sequential(KisStrokeJobData::SEQUENTIAL);
    KisStrokeJobData barrier(KisStrokeJobData::BARRIER);
    KisStrokeJobData uniquelyConcurrent(KisStrokeJobData::UNIQUELY_CONCURRENT);
    KisStrokeJobData exclusive(KisStrokeJobData::CONCURRENT, KisStrokeJobData::EXCLUSIVE);

    QVERIFY(!concurrent.isSequential());
    QVERIFY(!concurrent.isBarrier());
    QVERIFY(sequential.isSequential());
    QVERIFY(!sequential.isBarrier());
    QVERIFY(!barrier.isSequential());
    QVERIFY(barrier.isBarrier());
    QVERIFY(!uniquelyConcurrent.isSequential());
    QVERIFY(!uniquelyConcurrent.isBarrier());
    QVERIFY(!concurrent.isExclusive());
    QVERIFY(exclusive.isExclusive());
}

void KisStrokeJobContractTest::jobMutatorsAndCopyPreserveConfiguration()
{
    CopyableJobData source(KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
    source.setCancellable(false);
    source.setLevelOfDetailOverride(4);

    CopyableJobData copy(source);
    QCOMPARE(copy.sequentiality(), KisStrokeJobData::BARRIER);
    QCOMPARE(copy.exclusivity(), KisStrokeJobData::EXCLUSIVE);
    QVERIFY(!copy.isCancellable());
    QCOMPARE(copy.levelOfDetailOverride(), 4);
}

void KisStrokeJobContractTest::jobDataDeletesThroughBase()
{
    int destructionCount = 0;
    KisStrokeJobData *data = new DestructionJobData(&destructionCount);

    delete data;
    QCOMPARE(destructionCount, 1);
}

void KisStrokeJobContractTest::strategyRunsNamedJobsAndDeletesThroughBase()
{
    int runCount = 0;
    int destructionCount = 0;
    auto *recording = new RecordingStrategy(&runCount, &destructionCount);
    KisStrokeJobStrategy *strategy = recording;
    KisStrokeJobData data;

    QCOMPARE(strategy->debugId(), QStringLiteral("recording-strategy"));
    strategy->run(&data);
    QCOMPARE(runCount, 1);
    QCOMPARE(recording->lastData(), &data);

    delete strategy;
    QCOMPARE(destructionCount, 1);
}

void KisStrokeJobContractTest::runnableJobUsesDefaultAndRequestedScheduling()
{
    RunnableJobData defaults;
    QCOMPARE(defaults.sequentiality(), KisStrokeJobData::SEQUENTIAL);
    QCOMPARE(defaults.exclusivity(), KisStrokeJobData::NORMAL);

    RunnableJobData barrier(KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
    KisRunnable *runnable = &barrier;
    QCOMPARE(barrier.sequentiality(), KisStrokeJobData::BARRIER);
    QCOMPARE(barrier.exclusivity(), KisStrokeJobData::EXCLUSIVE);
    runnable->run();
    QCOMPARE(barrier.runCount, 1);
}

void KisStrokeJobContractTest::functionRunnableRunsWithRequestedScheduling()
{
    int runCount = 0;
    KisRunnableStrokeJobData data(
        [&runCount] {
            ++runCount;
        },
        KisStrokeJobData::CONCURRENT,
        KisStrokeJobData::EXCLUSIVE);

    QCOMPARE(data.sequentiality(), KisStrokeJobData::CONCURRENT);
    QCOMPARE(data.exclusivity(), KisStrokeJobData::EXCLUSIVE);
    data.run();
    QCOMPARE(runCount, 1);

    KisRunnableStrokeJobData emptyFunction{std::function<void()>()};
    emptyFunction.run();
    QCOMPARE(runCount, 1);
}

void KisStrokeJobContractTest::autoDeleteRunnableRunsAndDeletesWithJob()
{
    int runCount = 0;
    int destructionCount = 0;

    {
        auto *runnable = new RecordingQRunnable(&runCount, &destructionCount);
        QVERIFY(runnable->autoDelete());
        KisRunnableStrokeJobData data(runnable, KisStrokeJobData::BARRIER);
        data.run();
        QCOMPARE(runCount, 1);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KisStrokeJobContractTest::retainedRunnableSurvivesJobDestruction()
{
    int runCount = 0;
    int destructionCount = 0;
    auto *runnable = new RecordingQRunnable(&runCount, &destructionCount);
    runnable->setAutoDelete(false);

    {
        KisRunnableStrokeJobData data(runnable);
        data.run();
    }

    QCOMPARE(runCount, 1);
    QCOMPARE(destructionCount, 0);
    delete runnable;
    QCOMPARE(destructionCount, 1);
}

void KisStrokeJobContractTest::singleRunnableJobDelegatesAsOneElementBatch()
{
    RecordingJobsInterface jobs;
    RunnableJobData job;

    jobs.addRunnableJob(&job);

    QCOMPARE(jobs.batches.size(), 1);
    QCOMPARE(jobs.batches.constFirst().size(), 1);
    QCOMPARE(jobs.batches.constFirst().constFirst(), &job);
}

void KisStrokeJobContractTest::derivedRunnableJobListConvertsAndPreservesOrder()
{
    RecordingJobsInterface jobs;
    RunnableJobData first;
    RunnableJobData second;
    const QVector<RunnableJobData *> input{&first, &second};

    jobs.addRunnableJobs(input);

    QCOMPARE(jobs.batches.size(), 1);
    QCOMPARE(jobs.batches.constFirst().size(), 2);
    QCOMPARE(jobs.batches.constFirst().at(0), &first);
    QCOMPARE(jobs.batches.constFirst().at(1), &second);
}

void KisStrokeJobContractTest::jobsInterfaceDeletesThroughBase()
{
    int destructionCount = 0;
    KisRunnableStrokeJobsInterface *jobs = new RecordingJobsInterface(&destructionCount);

    delete jobs;

    QCOMPARE(destructionCount, 1);
}

void KisStrokeJobContractTest::fakeExecutorFlagValuesDescribeBarrierOptIn()
{
    QCOMPARE(int(KisFakeRunnableStrokeJobsExecutor::None), 0);
    QCOMPARE(int(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs), 1);

    KisFakeRunnableStrokeJobsExecutor::Flags flags = KisFakeRunnableStrokeJobsExecutor::None;
    QVERIFY(!flags.testFlag(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs));
    flags.setFlag(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs);
    QVERIFY(flags.testFlag(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs));
}

void KisStrokeJobContractTest::defaultFakeExecutorRunsAndDeletesJobsInOrder()
{
    KisFakeRunnableStrokeJobsExecutor executor;
    QVector<int> runOrder;
    int destructionCount = 0;
    QVector<KisRunnableStrokeJobDataBase *> jobs{
        new TrackedRunnableJobData(1, &runOrder, &destructionCount),
        new TrackedRunnableJobData(2, &runOrder, &destructionCount, KisStrokeJobData::CONCURRENT)};
    safeAssertConditions.clear();

    executor.addRunnableJobs(jobs);

    QCOMPARE(runOrder, QVector<int>({1, 2}));
    QCOMPARE(destructionCount, 2);
    QVERIFY(safeAssertConditions.isEmpty());
}

void KisStrokeJobContractTest::barrierFlagAllowsBarrierJob()
{
    KisFakeRunnableStrokeJobsExecutor executor(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs);
    QVector<int> runOrder;
    int destructionCount = 0;
    QVector<KisRunnableStrokeJobDataBase *> jobs{
        new TrackedRunnableJobData(3, &runOrder, &destructionCount, KisStrokeJobData::BARRIER)};
    safeAssertConditions.clear();

    executor.addRunnableJobs(jobs);

    QCOMPARE(runOrder, QVector<int>({3}));
    QCOMPARE(destructionCount, 1);
    QVERIFY(safeAssertConditions.isEmpty());
}

void KisStrokeJobContractTest::defaultFakeExecutorReportsUnsupportedJobs()
{
    KisFakeRunnableStrokeJobsExecutor executor;
    QVector<int> runOrder;
    int destructionCount = 0;
    QVector<KisRunnableStrokeJobDataBase *> jobs{
        new TrackedRunnableJobData(4, &runOrder, &destructionCount, KisStrokeJobData::BARRIER),
        new TrackedRunnableJobData(5,
                                   &runOrder,
                                   &destructionCount,
                                   KisStrokeJobData::CONCURRENT,
                                   KisStrokeJobData::EXCLUSIVE)};
    safeAssertConditions.clear();

    executor.addRunnableJobs(jobs);

    QCOMPARE(runOrder, QVector<int>({4, 5}));
    QCOMPARE(destructionCount, 2);
    QCOMPARE(safeAssertConditions.size(), 2);
    QVERIFY(safeAssertConditions.at(0).contains("barrier jobs are not supported"));
    QVERIFY(safeAssertConditions.at(1).contains("exclusive jobs are not supported"));
}

QTEST_GUILESS_MAIN(KisStrokeJobContractTest)

#include "KisStrokeJobContractTest.moc"
