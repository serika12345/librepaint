/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRunnableStrokeJobData.h"
#include "KisRunnableStrokeJobDataBase.h"
#include "kis_stroke_job_strategy.h"

#include <QRunnable>
#include <QTest>

namespace
{

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

QTEST_GUILESS_MAIN(KisStrokeJobContractTest)

#include "KisStrokeJobContractTest.moc"
