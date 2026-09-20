/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_spontaneous_job.h"

#include <QTest>

#include <utility>

namespace
{

class TestSpontaneousJob final : public KisSpontaneousJob
{
public:
    TestSpontaneousJob(QString name, int lod)
        : m_name(std::move(name))
        , m_lod(lod)
    {
    }

    void run() override
    {
        ++runCount;
    }

    QString debugName() const override
    {
        return m_name;
    }

    bool overrides(const KisSpontaneousJob *otherJob) override
    {
        lastOverrideCandidate = otherJob;
        return otherJob == overrideTarget;
    }

    int levelOfDetail() const override
    {
        return m_lod;
    }

    void configureExclusive(bool value)
    {
        setExclusive(value);
    }

    const KisSpontaneousJob *overrideTarget{nullptr};
    const KisSpontaneousJob *lastOverrideCandidate{nullptr};
    int runCount{0};

private:
    QString m_name;
    int m_lod;
};

} // namespace

class KisSpontaneousJobContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void runsThroughTheNamedJobBoundary();
    void protectedSetterControlsExclusiveState();
    void dispatchesOverrideAndLodPolicies();
};

void KisSpontaneousJobContractTest::runsThroughTheNamedJobBoundary()
{
    TestSpontaneousJob job(QStringLiteral("projection-refresh"), 3);
    KisRunnableWithDebugName *runnable = &job;

    QCOMPARE(runnable->debugName(), QStringLiteral("projection-refresh"));
    runnable->run();
    QCOMPARE(job.runCount, 1);
    QVERIFY(!job.isExclusive());
}

void KisSpontaneousJobContractTest::protectedSetterControlsExclusiveState()
{
    TestSpontaneousJob job(QStringLiteral("exclusive-refresh"), 1);

    QVERIFY(!job.isExclusive());
    job.configureExclusive(true);
    QVERIFY(job.isExclusive());
    job.configureExclusive(false);
    QVERIFY(!job.isExclusive());
}

void KisSpontaneousJobContractTest::dispatchesOverrideAndLodPolicies()
{
    TestSpontaneousJob replacedJob(QStringLiteral("replaced"), 2);
    TestSpontaneousJob unrelatedJob(QStringLiteral("unrelated"), 5);
    TestSpontaneousJob replacementJob(QStringLiteral("replacement"), 7);
    replacementJob.overrideTarget = &replacedJob;
    KisSpontaneousJob *job = &replacementJob;

    QVERIFY(job->overrides(&replacedJob));
    QCOMPARE(replacementJob.lastOverrideCandidate, &replacedJob);
    QVERIFY(!job->overrides(&unrelatedJob));
    QCOMPARE(replacementJob.lastOverrideCandidate, &unrelatedJob);
    QCOMPARE(job->levelOfDetail(), 7);
}

QTEST_GUILESS_MAIN(KisSpontaneousJobContractTest)

#include "KisSpontaneousJobContractTest.moc"
