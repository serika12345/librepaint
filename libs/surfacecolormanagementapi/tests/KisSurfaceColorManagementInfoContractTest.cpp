/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <surfacecolormanagement/KisSurfaceColorManagementInfo.h>

#include <QPromise>
#include <QTest>

#include <memory>
#include <utility>

class SurfaceColorManagementInfoProbe final : public KisSurfaceColorManagementInfo
{
public:
    SurfaceColorManagementInfoProbe(bool managedByOperatingSystem, QString report, bool *destroyed, QObject *parent)
        : KisSurfaceColorManagementInfo(parent)
        , m_managedByOperatingSystem(managedByOperatingSystem)
        , m_report(std::move(report))
        , m_destroyed(destroyed)
    {
    }

    ~SurfaceColorManagementInfoProbe() override
    {
        *m_destroyed = true;
    }

    bool surfaceColorManagedByOS() override
    {
        ++managedByOperatingSystemCallCount;
        return m_managedByOperatingSystem;
    }

    QFuture<QString> debugReport() override
    {
        ++debugReportCallCount;
        m_reportPromise = std::make_shared<QPromise<QString>>();
        m_reportPromise->start();
        return m_reportPromise->future();
    }

    void completeDebugReport()
    {
        m_reportPromise->addResult(m_report);
        m_reportPromise->finish();
    }

    int managedByOperatingSystemCallCount{0};
    int debugReportCallCount{0};

private:
    bool m_managedByOperatingSystem;
    QString m_report;
    bool *m_destroyed;
    std::shared_ptr<QPromise<QString>> m_reportPromise;
};

class KisSurfaceColorManagementInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesParentOwnershipAndVirtualAsynchronousBehavior();
};

void KisSurfaceColorManagementInfoContractTest::preservesParentOwnershipAndVirtualAsynchronousBehavior()
{
    bool destroyed = false;

    {
        QObject owner;
        auto *probe = new SurfaceColorManagementInfoProbe(true,
                                                          QStringLiteral("surface color management report"),
                                                          &destroyed,
                                                          &owner);
        KisSurfaceColorManagementInfo *interface = probe;

        QCOMPARE(interface->parent(), &owner);
        QCOMPARE(owner.children(), QObjectList{interface});

        QVERIFY(interface->surfaceColorManagedByOS());
        QCOMPARE(probe->managedByOperatingSystemCallCount, 1);

        QFuture<QString> report = interface->debugReport();
        QCOMPARE(probe->debugReportCallCount, 1);
        QVERIFY(report.isStarted());
        QVERIFY(!report.isFinished());

        probe->completeDebugReport();
        report.waitForFinished();
        QVERIFY(report.isFinished());
        QCOMPARE(report.resultCount(), 1);
        QCOMPARE(report.result(), QStringLiteral("surface color management report"));
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisSurfaceColorManagementInfoContractTest)

#include "KisSurfaceColorManagementInfoContractTest.moc"
