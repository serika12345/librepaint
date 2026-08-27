/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_runnable_with_debug_name.h"

#include <QTest>

namespace
{

class NamedRunnable : public KisRunnableWithDebugName
{
public:
    NamedRunnable(int *runCount, int *destructionCount)
        : m_runCount(runCount)
        , m_destructionCount(destructionCount)
    {
    }

    ~NamedRunnable() override
    {
        ++*m_destructionCount;
    }

    void run() override
    {
        ++*m_runCount;
    }

    QString debugName() const override
    {
        return QStringLiteral("named-runnable");
    }

private:
    int *m_runCount;
    int *m_destructionCount;
};

} // namespace

class KisRunnableContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namedRunnableRunsAndDeletesThroughBase();
};

void KisRunnableContractTest::namedRunnableRunsAndDeletesThroughBase()
{
    int runCount = 0;
    int destructionCount = 0;
    auto *named = new NamedRunnable(&runCount, &destructionCount);
    KisRunnable *runnable = named;

    QCOMPARE(named->debugName(), QStringLiteral("named-runnable"));
    runnable->run();
    QCOMPARE(runCount, 1);

    delete runnable;
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisRunnableContractTest)

#include "KisRunnableContractTest.moc"
