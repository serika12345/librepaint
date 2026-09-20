/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRunnableBasedStrokeStrategy.h"

#include "KisRunnableStrokeJobDataBase.h"
#include "KisRunnableStrokeJobsInterface.h"
#include "KisStrokesQueueMutatedJobInterface.h"

#include <QTest>

#include <memory>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

KisRunnableStrokeJobDataBase::KisRunnableStrokeJobDataBase(KisStrokeJobData::Sequentiality sequentiality,
                                                           KisStrokeJobData::Exclusivity exclusivity)
    : KisStrokeJobData(sequentiality, exclusivity)
{
}

KisRunnableStrokeJobsInterface::~KisRunnableStrokeJobsInterface() = default;

namespace
{

class RunnableData final : public KisRunnableStrokeJobDataBase
{
public:
    RunnableData(int *runCount, int *destructionCount = nullptr)
        : m_runCount(runCount)
        , m_destructionCount(destructionCount)
    {
    }

    ~RunnableData() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void run() override
    {
        ++*m_runCount;
    }

private:
    int *m_runCount;
    int *m_destructionCount;
};

class MutatedJobsProbe final : public KisStrokesQueueMutatedJobInterface
{
public:
    void addMutatedJobs(KisStrokeId strokeId, const QVector<KisStrokeJobData *> list) override
    {
        receivedStrokeId = strokeId;
        receivedJobs = list;
        ++deliveryCount;
    }

    KisLodPreferences lodPreferences() const override
    {
        return {};
    }

    KisStrokeId receivedStrokeId;
    QVector<KisStrokeJobData *> receivedJobs;
    int deliveryCount = 0;
};

class CountingStrategy final : public KisRunnableBasedStrokeStrategy
{
public:
    explicit CountingStrategy(int *destructionCount)
        : KisRunnableBasedStrokeStrategy(QLatin1String("counting"))
        , m_destructionCount(destructionCount)
    {
    }

    ~CountingStrategy() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

class KisRunnableBasedStrokeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesIdentityAndCreatesJobsInterface();
    void copyRebindsIndependentJobsInterface();
    void strokeCallbackRunsOnlyRunnableData();
    void jobsInterfaceForwardsListsInOrder();
    void strategyDeletesThroughBaseAndOwnsItsInterface();
};

void KisRunnableBasedStrokeStrategyContractTest::constructionPreservesIdentityAndCreatesJobsInterface()
{
    KisRunnableBasedStrokeStrategy strategy(QLatin1String("runnable-contract"),
                                            kundo2_noi18n(QStringLiteral("Runnable Contract")));

    QCOMPARE(strategy.id(), QStringLiteral("runnable-contract"));
    QCOMPARE(strategy.name().toString(), QStringLiteral("Runnable Contract"));
    QVERIFY(strategy.runnableJobsInterface());
}

void KisRunnableBasedStrokeStrategyContractTest::copyRebindsIndependentJobsInterface()
{
    KisRunnableBasedStrokeStrategy original(QLatin1String("copy-contract"),
                                            kundo2_noi18n(QStringLiteral("Copy Contract")));
    KisRunnableBasedStrokeStrategy copy(original);

    QCOMPARE(copy.id(), original.id());
    QCOMPARE(copy.name().toString(), original.name().toString());
    QVERIFY(copy.runnableJobsInterface());
    QVERIFY(copy.runnableJobsInterface() != original.runnableJobsInterface());
}

void KisRunnableBasedStrokeStrategyContractTest::strokeCallbackRunsOnlyRunnableData()
{
    KisRunnableBasedStrokeStrategy strategy(QLatin1String("dispatch-contract"));
    KisStrokeJobData ordinaryData;
    int runCount = 0;
    int destructionCount = 0;

    strategy.doStrokeCallback(nullptr);
    strategy.doStrokeCallback(&ordinaryData);
    {
        RunnableData runnable(&runCount, &destructionCount);
        strategy.doStrokeCallback(&runnable);
        QCOMPARE(runCount, 1);
        QCOMPARE(destructionCount, 0);
    }
    QCOMPARE(destructionCount, 1);
}

void KisRunnableBasedStrokeStrategyContractTest::jobsInterfaceForwardsListsInOrder()
{
    KisRunnableBasedStrokeStrategy strategy(QLatin1String("jobs-contract"));
    MutatedJobsProbe probe;
    KisStrokeSP stroke(reinterpret_cast<KisStroke *>(quintptr(1)), [](KisStroke *) { });
    strategy.setMutatedJobsInterface(&probe, stroke);

    int firstRuns = 0;
    int secondRuns = 0;
    RunnableData first(&firstRuns);
    RunnableData second(&secondRuns);
    QVector<KisRunnableStrokeJobDataBase *> jobs{&first, &second};

    strategy.runnableJobsInterface()->addRunnableJobs(jobs);

    QCOMPARE(probe.deliveryCount, 1);
    QVERIFY(probe.receivedStrokeId == KisStrokeId(stroke));
    QCOMPARE(probe.receivedJobs.size(), 2);
    QCOMPARE(probe.receivedJobs.at(0), static_cast<KisStrokeJobData *>(&first));
    QCOMPARE(probe.receivedJobs.at(1), static_cast<KisStrokeJobData *>(&second));
    QCOMPARE(firstRuns, 0);
    QCOMPARE(secondRuns, 0);
}

void KisRunnableBasedStrokeStrategyContractTest::strategyDeletesThroughBaseAndOwnsItsInterface()
{
    int destructionCount = 0;
    std::unique_ptr<KisStrokeStrategy> strategy(new CountingStrategy(&destructionCount));

    QVERIFY(static_cast<CountingStrategy *>(strategy.get())->runnableJobsInterface());
    strategy.reset();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisRunnableBasedStrokeStrategyContractTest)

#include "KisRunnableBasedStrokeStrategyContractTest.moc"
