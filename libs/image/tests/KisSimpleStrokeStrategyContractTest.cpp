/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_simple_stroke_strategy.h"

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

namespace
{

class SimpleStrokeStrategyProbe final : public KisSimpleStrokeStrategy
{
public:
    SimpleStrokeStrategyProbe(QStringList *callbacks = nullptr,
                              KisStrokeJobData **lastData = nullptr,
                              int *destructionCount = nullptr)
        : KisSimpleStrokeStrategy(QLatin1String("simple-contract"), kundo2_noi18n(QStringLiteral("Simple Contract")))
        , m_callbacks(callbacks)
        , m_lastData(lastData)
        , m_destructionCount(destructionCount)
    {
    }

    ~SimpleStrokeStrategyProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void configureJob(JobType type,
                      bool enabled,
                      KisStrokeJobData::Sequentiality sequentiality = KisStrokeJobData::SEQUENTIAL,
                      KisStrokeJobData::Exclusivity exclusivity = KisStrokeJobData::NORMAL)
    {
        enableJob(type, enabled, sequentiality, exclusivity);
    }

    void initStrokeCallback() override
    {
        record(QStringLiteral("init"));
    }

    void finishStrokeCallback() override
    {
        record(QStringLiteral("finish"));
    }

    void cancelStrokeCallback() override
    {
        record(QStringLiteral("cancel"));
    }

    void doStrokeCallback(KisStrokeJobData *data) override
    {
        if (m_lastData) {
            *m_lastData = data;
        }
        record(QStringLiteral("dostroke"));
    }

    void suspendStrokeCallback() override
    {
        record(QStringLiteral("suspend"));
    }

    void resumeStrokeCallback() override
    {
        record(QStringLiteral("resume"));
    }

private:
    void record(const QString &callback)
    {
        if (m_callbacks) {
            m_callbacks->append(callback);
        }
    }

    QStringList *m_callbacks;
    KisStrokeJobData **m_lastData;
    int *m_destructionCount;
};

void compareScheduling(KisStrokeJobData &data,
                       KisStrokeJobData::Sequentiality sequentiality,
                       KisStrokeJobData::Exclusivity exclusivity)
{
    QCOMPARE(data.sequentiality(), sequentiality);
    QCOMPARE(data.exclusivity(), exclusivity);
}

} // namespace

class KisSimpleStrokeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void jobTypesAndIdentityRemainStable();
    void disabledJobsReturnNoStrategiesAndDefaultData();
    void dataFactoriesPreservePerJobScheduling();
    void enabledJobStrategiesDispatchCallbacksAndBorrowParent();
};

void KisSimpleStrokeStrategyContractTest::jobTypesAndIdentityRemainStable()
{
    SimpleStrokeStrategyProbe strategy;

    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_INIT), 0);
    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_CANCEL), 1);
    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_FINISH), 2);
    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_DOSTROKE), 3);
    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_SUSPEND), 4);
    QCOMPARE(int(KisSimpleStrokeStrategy::JOB_RESUME), 5);
    QCOMPARE(int(KisSimpleStrokeStrategy::NJOBS), 6);

    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_INIT)),
             QStringLiteral("init"));
    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_CANCEL)),
             QStringLiteral("cancel"));
    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_FINISH)),
             QStringLiteral("finish"));
    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_DOSTROKE)),
             QStringLiteral("dostroke"));
    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_SUSPEND)),
             QStringLiteral("suspend"));
    QCOMPARE(QString(KisSimpleStrokeStrategy::jobTypeToString(KisSimpleStrokeStrategy::JOB_RESUME)),
             QStringLiteral("resume"));

    QCOMPARE(strategy.id(), QStringLiteral("simple-contract"));
    QCOMPARE(strategy.name().toString(), QStringLiteral("Simple Contract"));
}

void KisSimpleStrokeStrategyContractTest::disabledJobsReturnNoStrategiesAndDefaultData()
{
    KisSimpleStrokeStrategy strategy(QLatin1String("disabled"));

    QCOMPARE(strategy.createInitStrategy(), nullptr);
    QCOMPARE(strategy.createFinishStrategy(), nullptr);
    QCOMPARE(strategy.createCancelStrategy(), nullptr);
    QCOMPARE(strategy.createDabStrategy(), nullptr);
    QCOMPARE(strategy.createSuspendStrategy(), nullptr);
    QCOMPARE(strategy.createResumeStrategy(), nullptr);

    std::unique_ptr<KisStrokeJobData> initData(strategy.createInitData());
    std::unique_ptr<KisStrokeJobData> finishData(strategy.createFinishData());
    std::unique_ptr<KisStrokeJobData> cancelData(strategy.createCancelData());
    std::unique_ptr<KisStrokeJobData> suspendData(strategy.createSuspendData());
    std::unique_ptr<KisStrokeJobData> resumeData(strategy.createResumeData());

    QVERIFY(initData);
    QVERIFY(finishData);
    QVERIFY(cancelData);
    QVERIFY(suspendData);
    QVERIFY(resumeData);
    compareScheduling(*initData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);
    compareScheduling(*finishData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);
    compareScheduling(*cancelData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);
    compareScheduling(*suspendData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);
    compareScheduling(*resumeData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);

    strategy.initStrokeCallback();
    strategy.finishStrokeCallback();
    strategy.cancelStrokeCallback();
    strategy.doStrokeCallback(initData.get());
    strategy.suspendStrokeCallback();
    strategy.resumeStrokeCallback();
}

void KisSimpleStrokeStrategyContractTest::dataFactoriesPreservePerJobScheduling()
{
    SimpleStrokeStrategyProbe strategy;
    strategy.configureJob(KisSimpleStrokeStrategy::JOB_INIT,
                          false,
                          KisStrokeJobData::BARRIER,
                          KisStrokeJobData::EXCLUSIVE);
    strategy.configureJob(KisSimpleStrokeStrategy::JOB_FINISH,
                          false,
                          KisStrokeJobData::CONCURRENT,
                          KisStrokeJobData::NORMAL);
    strategy.configureJob(KisSimpleStrokeStrategy::JOB_CANCEL,
                          false,
                          KisStrokeJobData::UNIQUELY_CONCURRENT,
                          KisStrokeJobData::EXCLUSIVE);
    strategy.configureJob(KisSimpleStrokeStrategy::JOB_SUSPEND,
                          false,
                          KisStrokeJobData::SEQUENTIAL,
                          KisStrokeJobData::EXCLUSIVE);
    strategy.configureJob(KisSimpleStrokeStrategy::JOB_RESUME,
                          false,
                          KisStrokeJobData::BARRIER,
                          KisStrokeJobData::NORMAL);

    std::unique_ptr<KisStrokeJobData> initData(strategy.createInitData());
    std::unique_ptr<KisStrokeJobData> finishData(strategy.createFinishData());
    std::unique_ptr<KisStrokeJobData> cancelData(strategy.createCancelData());
    std::unique_ptr<KisStrokeJobData> suspendData(strategy.createSuspendData());
    std::unique_ptr<KisStrokeJobData> resumeData(strategy.createResumeData());

    compareScheduling(*initData, KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
    compareScheduling(*finishData, KisStrokeJobData::CONCURRENT, KisStrokeJobData::NORMAL);
    compareScheduling(*cancelData, KisStrokeJobData::UNIQUELY_CONCURRENT, KisStrokeJobData::EXCLUSIVE);
    compareScheduling(*suspendData, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);
    compareScheduling(*resumeData, KisStrokeJobData::BARRIER, KisStrokeJobData::NORMAL);

    QCOMPARE(strategy.createInitStrategy(), nullptr);
    QCOMPARE(strategy.createFinishStrategy(), nullptr);
    QCOMPARE(strategy.createCancelStrategy(), nullptr);
    QCOMPARE(strategy.createSuspendStrategy(), nullptr);
    QCOMPARE(strategy.createResumeStrategy(), nullptr);
}

void KisSimpleStrokeStrategyContractTest::enabledJobStrategiesDispatchCallbacksAndBorrowParent()
{
    QStringList callbacks;
    KisStrokeJobData *lastData = nullptr;
    int parentDestructionCount = 0;
    auto parent = std::make_unique<SimpleStrokeStrategyProbe>(&callbacks, &lastData, &parentDestructionCount);

    parent->configureJob(KisSimpleStrokeStrategy::JOB_INIT, true);
    parent->configureJob(KisSimpleStrokeStrategy::JOB_FINISH, true);
    parent->configureJob(KisSimpleStrokeStrategy::JOB_CANCEL, true);
    parent->configureJob(KisSimpleStrokeStrategy::JOB_DOSTROKE, true);
    parent->configureJob(KisSimpleStrokeStrategy::JOB_SUSPEND, true);
    parent->configureJob(KisSimpleStrokeStrategy::JOB_RESUME, true);

    std::unique_ptr<KisStrokeJobStrategy> initStrategy(parent->createInitStrategy());
    std::unique_ptr<KisStrokeJobStrategy> finishStrategy(parent->createFinishStrategy());
    std::unique_ptr<KisStrokeJobStrategy> cancelStrategy(parent->createCancelStrategy());
    std::unique_ptr<KisStrokeJobStrategy> dabStrategy(parent->createDabStrategy());
    std::unique_ptr<KisStrokeJobStrategy> suspendStrategy(parent->createSuspendStrategy());
    std::unique_ptr<KisStrokeJobStrategy> resumeStrategy(parent->createResumeStrategy());

    QVERIFY(initStrategy);
    QVERIFY(finishStrategy);
    QVERIFY(cancelStrategy);
    QVERIFY(dabStrategy);
    QVERIFY(suspendStrategy);
    QVERIFY(resumeStrategy);
    QCOMPARE(initStrategy->debugId(), QStringLiteral("simple-contract/init"));
    QCOMPARE(finishStrategy->debugId(), QStringLiteral("simple-contract/finish"));
    QCOMPARE(cancelStrategy->debugId(), QStringLiteral("simple-contract/cancel"));
    QCOMPARE(dabStrategy->debugId(), QStringLiteral("simple-contract/dostroke"));
    QCOMPARE(suspendStrategy->debugId(), QStringLiteral("simple-contract/suspend"));
    QCOMPARE(resumeStrategy->debugId(), QStringLiteral("simple-contract/resume"));

    KisStrokeJobData dabData;
    initStrategy->run(nullptr);
    finishStrategy->run(nullptr);
    cancelStrategy->run(nullptr);
    dabStrategy->run(&dabData);
    suspendStrategy->run(nullptr);
    resumeStrategy->run(nullptr);

    QCOMPARE(callbacks,
             QStringList({QStringLiteral("init"),
                          QStringLiteral("finish"),
                          QStringLiteral("cancel"),
                          QStringLiteral("dostroke"),
                          QStringLiteral("suspend"),
                          QStringLiteral("resume")}));
    QCOMPARE(lastData, &dabData);

    initStrategy.reset();
    finishStrategy.reset();
    cancelStrategy.reset();
    dabStrategy.reset();
    suspendStrategy.reset();
    resumeStrategy.reset();
    QCOMPARE(parentDestructionCount, 0);
    parent.reset();
    QCOMPARE(parentDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KisSimpleStrokeStrategyContractTest)

#include "KisSimpleStrokeStrategyContractTest.moc"
