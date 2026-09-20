/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSynchronizedConnection.h"

#include <QCoreApplication>
#include <QTest>

#include <memory>
#include <utility>
#include <vector>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class SynchronizedSender : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void valuesReady(int number, const QString &text);
};

class SynchronizedReceiver
{
public:
    void receive(int number, const QString &text)
    {
        calls++;
        lastNumber = number;
        lastText = text;
    }

    int calls {0};
    int lastNumber {0};
    QString lastText;
};

class KisSynchronizedConnectionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void eventCopiesTypeAndTracksDestinationLifetime();
    void callbackConstructionQueuesAndDeliversInOrder();
    void inputAndOutputConnectionsForwardArguments();
    void synchronizedConnectionWiresBothEnds();
    void baseConfigurationAndBarrierControlDelivery();
};

void KisSynchronizedConnectionContractTest::init()
{
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
}

void KisSynchronizedConnectionContractTest::cleanup()
{
    QCoreApplication::sendPostedEvents(
        nullptr, KisSynchronizedConnectionBase::eventType());
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
}

void KisSynchronizedConnectionContractTest::eventCopiesTypeAndTracksDestinationLifetime()
{
    auto destination = std::make_unique<QObject>();
    KisSynchronizedConnectionEvent event(destination.get());
    KisSynchronizedConnectionEvent copy(event);

    QCOMPARE(static_cast<int>(event.type()), KisSynchronizedConnectionBase::eventType());
    QCOMPARE(copy.type(), event.type());
    QCOMPARE(event.destination.data(), destination.get());
    QCOMPARE(copy.destination.data(), destination.get());

    destination.reset();
    QVERIFY(event.destination.isNull());
    QVERIFY(copy.destination.isNull());
}

void KisSynchronizedConnectionContractTest::callbackConstructionQueuesAndDeliversInOrder()
{
    using Connection = KisSynchronizedConnection<int, QString>;
    Connection::ArgsTuple representativeArguments(3, QStringLiteral("tuple"));
    QCOMPARE(std::get<0>(representativeArguments), 3);
    QCOMPARE(std::get<1>(representativeArguments), QStringLiteral("tuple"));

    std::vector<std::pair<int, QString>> delivered;
    Connection::CallbackFunction callback = [&delivered](int number, const QString &text) {
        delivered.emplace_back(number, text);
    };

    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(true);
    Connection immediate(callback);
    immediate.start(1, QStringLiteral("immediate"));
    QVERIFY(!immediate.hasPendingSignals());
    QCOMPARE(delivered.size(), std::size_t(1));

    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    Connection queued;
    queued.setCallback(callback);
    queued.start(2, QStringLiteral("first-queued"));
    queued.start(3, QStringLiteral("second-queued"));
    QVERIFY(queued.hasPendingSignals());

    QCoreApplication::sendPostedEvents(
        &queued, KisSynchronizedConnectionBase::eventType());

    QVERIFY(!queued.hasPendingSignals());
    QCOMPARE(delivered.size(), std::size_t(3));
    QCOMPARE(delivered[1].first, 2);
    QCOMPARE(delivered[1].second, QStringLiteral("first-queued"));
    QCOMPARE(delivered[2].first, 3);
    QCOMPARE(delivered[2].second, QStringLiteral("second-queued"));
}

void KisSynchronizedConnectionContractTest::inputAndOutputConnectionsForwardArguments()
{
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(true);
    SynchronizedSender sender;
    SynchronizedReceiver receiver;
    KisSynchronizedConnection<int, QString> connection;
    connection.connectInputSignal(&sender, &SynchronizedSender::valuesReady);
    connection.connectOutputSlot(&receiver, &SynchronizedReceiver::receive);

    Q_EMIT sender.valuesReady(11, QStringLiteral("separate"));

    QCOMPARE(receiver.calls, 1);
    QCOMPARE(receiver.lastNumber, 11);
    QCOMPARE(receiver.lastText, QStringLiteral("separate"));
}

void KisSynchronizedConnectionContractTest::synchronizedConnectionWiresBothEnds()
{
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(true);
    SynchronizedSender sender;
    SynchronizedReceiver receiver;
    KisSynchronizedConnection<int, QString> connection;
    connection.connectSync(&sender,
                           &SynchronizedSender::valuesReady,
                           &receiver,
                           &SynchronizedReceiver::receive);

    Q_EMIT sender.valuesReady(19, QStringLiteral("combined"));

    QCOMPARE(receiver.calls, 1);
    QCOMPARE(receiver.lastNumber, 19);
    QCOMPARE(receiver.lastText, QStringLiteral("combined"));
}

void KisSynchronizedConnectionContractTest::baseConfigurationAndBarrierControlDelivery()
{
    int barrierCalls = 0;
    KisSynchronizedConnectionBase::registerSynchronizedEventBarrier([&barrierCalls] {
        barrierCalls++;
    });
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(true);
    QVERIFY(KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled());

    KisSynchronizedConnectionBase::forceDeliverAllSynchronizedEvents();

    QCOMPARE(barrierCalls, 1);
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    QVERIFY(!KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled());
}

QTEST_GUILESS_MAIN(KisSynchronizedConnectionContractTest)

#include "KisSynchronizedConnectionContractTest.moc"
