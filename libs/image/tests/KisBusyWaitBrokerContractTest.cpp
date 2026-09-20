/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBusyWaitBroker.h"

#include <QTest>

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

QTEST_MAIN(KisBusyWaitBrokerContractTest)

#include "KisBusyWaitBrokerContractTest.moc"
