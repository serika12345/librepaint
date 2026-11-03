/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBusyWaitBroker.h"

#include <QApplication>
#include <QProcess>
#include <QTest>

#include <cstdio>
#include <cstring>
#include <functional>
#include <thread>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisBusyWaitBrokerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singletonAndInitialWaitState();
    void generalWaitNestingTracksGuiThreadState();
    void workerThreadGeneralWaitNotificationsAreIgnored();
    void imageWaitNestingTracksGuiThreadState();
    void notificationsOutsideApplicationLifetimeAreIgnored_data();
    void notificationsOutsideApplicationLifetimeAreIgnored();
};

void KisBusyWaitBrokerContractTest::singletonAndInitialWaitState()
{
    KisBusyWaitBroker independentBroker;
    QVERIFY(!independentBroker.guiThreadIsWaitingForBetterWeather());

    KisBusyWaitBroker *firstInstance = KisBusyWaitBroker::instance();
    QVERIFY(firstInstance);
    QCOMPARE(KisBusyWaitBroker::instance(), firstInstance);
    QVERIFY(!firstInstance->guiThreadIsWaitingForBetterWeather());
}

void KisBusyWaitBrokerContractTest::generalWaitNestingTracksGuiThreadState()
{
    KisBusyWaitBroker broker;
    QVERIFY(!broker.guiThreadIsWaitingForBetterWeather());

    broker.notifyGeneralWaitStarted();
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());
    broker.notifyGeneralWaitStarted();
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());

    broker.notifyGeneralWaitEnded();
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());
    broker.notifyGeneralWaitEnded();
    QVERIFY(!broker.guiThreadIsWaitingForBetterWeather());
}

void KisBusyWaitBrokerContractTest::workerThreadGeneralWaitNotificationsAreIgnored()
{
    KisBusyWaitBroker broker;

    std::thread workerStart([&broker] {
        broker.notifyGeneralWaitStarted();
    });
    workerStart.join();
    QVERIFY(!broker.guiThreadIsWaitingForBetterWeather());

    broker.notifyGeneralWaitStarted();
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());

    std::thread workerEnd([&broker] {
        broker.notifyGeneralWaitEnded();
    });
    workerEnd.join();
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());

    broker.notifyGeneralWaitEnded();
    QVERIFY(!broker.guiThreadIsWaitingForBetterWeather());
}

void KisBusyWaitBrokerContractTest::imageWaitNestingTracksGuiThreadState()
{
    KisBusyWaitBroker broker;
    KisImage *const imageToken = nullptr;

    broker.notifyWaitOnImageStarted(imageToken);
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());
    broker.notifyWaitOnImageStarted(imageToken);
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());

    broker.notifyWaitOnImageEnded(imageToken);
    QVERIFY(broker.guiThreadIsWaitingForBetterWeather());
    broker.notifyWaitOnImageEnded(imageToken);
    QVERIFY(!broker.guiThreadIsWaitingForBetterWeather());
}

void KisBusyWaitBrokerContractTest::notificationsOutsideApplicationLifetimeAreIgnored_data()
{
    QTest::addColumn<QString>("probe");
    QTest::newRow("before-application") << QStringLiteral("--probe-before-application");
    QTest::newRow("after-application") << QStringLiteral("--probe-after-application");
}

void KisBusyWaitBrokerContractTest::notificationsOutsideApplicationLifetimeAreIgnored()
{
    QFETCH(QString, probe);
    QProcess process;
    process.start(QCoreApplication::applicationFilePath(), {probe});
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(process.readAllStandardOutput(), QByteArray("wait notifications ignored"));
}

static int probeWithoutApplication(int argc, char **argv, bool destroyApplication)
{
    KisBusyWaitBroker broker;
    if (destroyApplication) {
        QApplication application(argc, argv);
    }

    broker.notifyWaitOnImageStarted(nullptr);
    if (broker.guiThreadIsWaitingForBetterWeather()) return 1;
    broker.notifyWaitOnImageEnded(nullptr);
    if (broker.guiThreadIsWaitingForBetterWeather()) return 2;
    broker.notifyGeneralWaitStarted();
    if (broker.guiThreadIsWaitingForBetterWeather()) return 3;
    broker.notifyGeneralWaitEnded();
    if (broker.guiThreadIsWaitingForBetterWeather()) return 4;
    std::fputs("wait notifications ignored", stdout);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--probe-before-application") == 0) {
        return probeWithoutApplication(argc, argv, false);
    }
    if (argc == 2 && std::strcmp(argv[1], "--probe-after-application") == 0) {
        return probeWithoutApplication(argc, argv, true);
    }
    QApplication application(argc, argv);
    KisBusyWaitBrokerContractTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "KisBusyWaitBrokerContractTest.moc"
