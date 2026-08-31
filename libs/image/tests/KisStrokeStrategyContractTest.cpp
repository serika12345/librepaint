/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokesQueueMutatedJobInterface.h"
#include "kis_stroke_strategy.h"

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

class StrokeStrategyProbe final : public KisStrokeStrategy
{
public:
    explicit StrokeStrategyProbe(int *destructionCount = nullptr)
        : KisStrokeStrategy(QLatin1String("contract-stroke"), kundo2_noi18n(QStringLiteral("Contract Stroke")))
        , m_destructionCount(destructionCount)
    {
    }

    ~StrokeStrategyProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void configureNonDefaultFlags()
    {
        setExclusive(true);
        setSupportsWrapAroundMode(true);
        setClearsRedoOnStart(false);
        setRequestsOtherStrokesToEnd(false);
        setCanForgetAboutMe(true);
        setAsynchronouslyCancellable(false);
        setNeedsExplicitCancel(true);
        setBalancingRatioOverride(2.5);
    }

private:
    int *m_destructionCount;
};

class MutatedJobsInterfaceProbe final : public KisStrokesQueueMutatedJobInterface
{
public:
    MutatedJobsInterfaceProbe(int *lodPreferenceCalls, int *destructionCount)
        : m_lodPreferenceCalls(lodPreferenceCalls)
        , m_destructionCount(destructionCount)
    {
    }

    ~MutatedJobsInterfaceProbe() override
    {
        ++*m_destructionCount;
    }

    void addMutatedJobs(KisStrokeId, const QVector<KisStrokeJobData *>) override
    {
        qFatal("The public LOD preference query must not add mutated jobs");
    }

    KisLodPreferences lodPreferences() const override
    {
        ++*m_lodPreferenceCalls;
        return KisLodPreferences(KisLodPreferences::LodSupported | KisLodPreferences::LodPreferred, 4);
    }

private:
    int *m_lodPreferenceCalls;
    int *m_destructionCount;
};

} // namespace

class KisStrokeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identityDefaultsAndConfigurableFlagsPreserveValues();
    void defaultLifecycleHooksAndFactoriesRemainNullNoops();
    void mutatedJobsInterfaceIsBorrowedAndSuppliesLodPreferences();
    void virtualBaseDestructionReleasesDerivedOnce();
};

void KisStrokeStrategyContractTest::identityDefaultsAndConfigurableFlagsPreserveValues()
{
    StrokeStrategyProbe strategy;

    QCOMPARE(strategy.id(), QStringLiteral("contract-stroke"));
    QCOMPARE(strategy.name().toString(), QStringLiteral("Contract Stroke"));
    QVERIFY(!strategy.isExclusive());
    QVERIFY(!strategy.supportsWrapAroundMode());
    QVERIFY(strategy.clearsRedoOnStart());
    QVERIFY(strategy.requestsOtherStrokesToEnd());
    QVERIFY(!strategy.canForgetAboutMe());
    QVERIFY(strategy.isAsynchronouslyCancellable());
    QVERIFY(!strategy.needsExplicitCancel());
    QVERIFY(!strategy.forceLodModeIfPossible());
    QCOMPARE(strategy.balancingRatioOverride(), -1.0);

    strategy.configureNonDefaultFlags();
    strategy.setForceLodModeIfPossible(true);

    QVERIFY(strategy.isExclusive());
    QVERIFY(strategy.supportsWrapAroundMode());
    QVERIFY(!strategy.clearsRedoOnStart());
    QVERIFY(!strategy.requestsOtherStrokesToEnd());
    QVERIFY(strategy.canForgetAboutMe());
    QVERIFY(!strategy.isAsynchronouslyCancellable());
    QVERIFY(strategy.needsExplicitCancel());
    QVERIFY(strategy.forceLodModeIfPossible());
    QCOMPARE(strategy.balancingRatioOverride(), 2.5);
}

void KisStrokeStrategyContractTest::defaultLifecycleHooksAndFactoriesRemainNullNoops()
{
    KisStrokeStrategy strategy(QLatin1String("default-lifecycle"));
    KisStrokeStrategy *const base = &strategy;

    base->notifyUserStartedStroke();
    base->notifyUserEndedStroke();
    base->tryCancelCurrentStrokeJobAsync();

    QVERIFY(base->createInitStrategy() == nullptr);
    QVERIFY(base->createFinishStrategy() == nullptr);
    QVERIFY(base->createCancelStrategy() == nullptr);
    QVERIFY(base->createDabStrategy() == nullptr);
    QVERIFY(base->createSuspendStrategy() == nullptr);
    QVERIFY(base->createResumeStrategy() == nullptr);

    QVERIFY(base->createInitData() == nullptr);
    QVERIFY(base->createFinishData() == nullptr);
    QVERIFY(base->createCancelData() == nullptr);
    QVERIFY(base->createSuspendData() == nullptr);
    QVERIFY(base->createResumeData() == nullptr);

    QVERIFY(base->createLodClone(3) == nullptr);
}

void KisStrokeStrategyContractTest::mutatedJobsInterfaceIsBorrowedAndSuppliesLodPreferences()
{
    int lodPreferenceCalls = 0;
    int interfaceDestructions = 0;
    std::unique_ptr<MutatedJobsInterfaceProbe> interface(
        new MutatedJobsInterfaceProbe(&lodPreferenceCalls, &interfaceDestructions));

    {
        StrokeStrategyProbe strategy;
        strategy.setMutatedJobsInterface(interface.get(), KisStrokeId());

        const KisLodPreferences preferences = strategy.currentLodPreferences();
        QCOMPARE(lodPreferenceCalls, 1);
        QVERIFY(preferences.lodSupported());
        QVERIFY(preferences.lodPreferred());
        QCOMPARE(preferences.desiredLevelOfDetail(), 4);
    }

    QCOMPARE(interfaceDestructions, 0);
    interface.reset();
    QCOMPARE(interfaceDestructions, 1);
}

void KisStrokeStrategyContractTest::virtualBaseDestructionReleasesDerivedOnce()
{
    int destructionCount = 0;
    std::unique_ptr<KisStrokeStrategy> strategy(new StrokeStrategyProbe(&destructionCount));

    QCOMPARE(destructionCount, 0);
    strategy.reset();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisStrokeStrategyContractTest)

#include "KisStrokeStrategyContractTest.moc"
