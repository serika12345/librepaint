/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_acyclic_signal_connector.h"

#include <QPointer>
#include <QTest>
#include <QVariant>

class KoColor
{
public:
    explicit KoColor(int marker = 0)
        : marker(marker)
    {
    }

    int marker;
};

class SignalEndpoint : public QObject
{
    Q_OBJECT

public:
    int doubleCalls {0};
    int intCalls {0};
    int boolCalls {0};
    int voidCalls {0};
    int variantCalls {0};
    int resourcePairCalls {0};
    int colorCalls {0};
    double doubleValue {0.0};
    int intValue {0};
    bool boolValue {false};
    QVariant variantValue;
    int resourceKey {0};
    QVariant resourceValue;
    int colorMarker {0};

public Q_SLOTS:
    void receiveDouble(double value)
    {
        doubleCalls++;
        doubleValue = value;
    }

    void receiveInt(int value)
    {
        intCalls++;
        intValue = value;
    }

    void receiveBool(bool value)
    {
        boolCalls++;
        boolValue = value;
    }

    void receiveVoid()
    {
        voidCalls++;
    }

    void receiveVariant(const QVariant &value)
    {
        variantCalls++;
        variantValue = value;
    }

    void receiveResourcePair(int key, const QVariant &value)
    {
        resourcePairCalls++;
        resourceKey = key;
        resourceValue = value;
    }

    void receiveColor(const KoColor &value)
    {
        colorCalls++;
        colorMarker = value.marker;
    }

Q_SIGNALS:
    void doubleSent(double value);
    void intSent(int value);
    void boolSent(bool value);
    void voidSent();
    void variantSent(const QVariant &value);
    void resourcePairSent(int key, const QVariant &value);
    void colorSent(const KoColor &value);
};

class KisAcyclicSignalConnectorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void forwardsEverySupportedPayloadWithoutDuplicateConnections();
    void backwardsEverySupportedPayload();
    void manualAndScopedLocksSuppressDelivery();
    void coordinatedConnectorsShareLocksAndLifetime();
};

void KisAcyclicSignalConnectorContractTest::forwardsEverySupportedPayloadWithoutDuplicateConnections()
{
    KisAcyclicSignalConnector connector;
    SignalEndpoint sender;
    SignalEndpoint receiver;

    connector.connectForwardDouble(&sender, SIGNAL(doubleSent(double)), &receiver, SLOT(receiveDouble(double)));
    connector.connectForwardDouble(&sender, SIGNAL(doubleSent(double)), &receiver, SLOT(receiveDouble(double)));
    connector.connectForwardInt(&sender, SIGNAL(intSent(int)), &receiver, SLOT(receiveInt(int)));
    connector.connectForwardBool(&sender, SIGNAL(boolSent(bool)), &receiver, SLOT(receiveBool(bool)));
    connector.connectForwardVoid(&sender, SIGNAL(voidSent()), &receiver, SLOT(receiveVoid()));
    connector.connectForwardVariant(&sender, SIGNAL(variantSent(QVariant)), &receiver, SLOT(receiveVariant(QVariant)));
    connector.connectForwardResourcePair(&sender, SIGNAL(resourcePairSent(int,QVariant)), &receiver, SLOT(receiveResourcePair(int,QVariant)));
    connector.connectForwardKoColor(&sender, SIGNAL(colorSent(KoColor)), &receiver, SLOT(receiveColor(KoColor)));

    Q_EMIT sender.doubleSent(1.25);
    Q_EMIT sender.intSent(42);
    Q_EMIT sender.boolSent(true);
    Q_EMIT sender.voidSent();
    Q_EMIT sender.variantSent(QStringLiteral("forward"));
    Q_EMIT sender.resourcePairSent(7, QByteArrayLiteral("resource"));
    Q_EMIT sender.colorSent(KoColor(91));

    QCOMPARE(receiver.doubleCalls, 1);
    QCOMPARE(receiver.doubleValue, 1.25);
    QCOMPARE(receiver.intCalls, 1);
    QCOMPARE(receiver.intValue, 42);
    QCOMPARE(receiver.boolCalls, 1);
    QVERIFY(receiver.boolValue);
    QCOMPARE(receiver.voidCalls, 1);
    QCOMPARE(receiver.variantCalls, 1);
    QCOMPARE(receiver.variantValue.toString(), QStringLiteral("forward"));
    QCOMPARE(receiver.resourcePairCalls, 1);
    QCOMPARE(receiver.resourceKey, 7);
    QCOMPARE(receiver.resourceValue.toByteArray(), QByteArrayLiteral("resource"));
    QCOMPARE(receiver.colorCalls, 1);
    QCOMPARE(receiver.colorMarker, 91);
}

void KisAcyclicSignalConnectorContractTest::backwardsEverySupportedPayload()
{
    KisAcyclicSignalConnector connector;
    SignalEndpoint sender;
    SignalEndpoint receiver;

    connector.connectBackwardDouble(&sender, SIGNAL(doubleSent(double)), &receiver, SLOT(receiveDouble(double)));
    connector.connectBackwardInt(&sender, SIGNAL(intSent(int)), &receiver, SLOT(receiveInt(int)));
    connector.connectBackwardBool(&sender, SIGNAL(boolSent(bool)), &receiver, SLOT(receiveBool(bool)));
    connector.connectBackwardVoid(&sender, SIGNAL(voidSent()), &receiver, SLOT(receiveVoid()));
    connector.connectBackwardVariant(&sender, SIGNAL(variantSent(QVariant)), &receiver, SLOT(receiveVariant(QVariant)));
    connector.connectBackwardResourcePair(&sender, SIGNAL(resourcePairSent(int,QVariant)), &receiver, SLOT(receiveResourcePair(int,QVariant)));
    connector.connectBackwardKoColor(&sender, SIGNAL(colorSent(KoColor)), &receiver, SLOT(receiveColor(KoColor)));

    Q_EMIT sender.doubleSent(2.5);
    Q_EMIT sender.intSent(-12);
    Q_EMIT sender.boolSent(true);
    Q_EMIT sender.voidSent();
    Q_EMIT sender.variantSent(QStringLiteral("backward"));
    Q_EMIT sender.resourcePairSent(11, QByteArrayLiteral("pair"));
    Q_EMIT sender.colorSent(KoColor(37));

    QCOMPARE(receiver.doubleCalls, 1);
    QCOMPARE(receiver.doubleValue, 2.5);
    QCOMPARE(receiver.intCalls, 1);
    QCOMPARE(receiver.intValue, -12);
    QCOMPARE(receiver.boolCalls, 1);
    QVERIFY(receiver.boolValue);
    QCOMPARE(receiver.voidCalls, 1);
    QCOMPARE(receiver.variantCalls, 1);
    QCOMPARE(receiver.variantValue.toString(), QStringLiteral("backward"));
    QCOMPARE(receiver.resourcePairCalls, 1);
    QCOMPARE(receiver.resourceKey, 11);
    QCOMPARE(receiver.resourceValue.toByteArray(), QByteArrayLiteral("pair"));
    QCOMPARE(receiver.colorCalls, 1);
    QCOMPARE(receiver.colorMarker, 37);
}

void KisAcyclicSignalConnectorContractTest::manualAndScopedLocksSuppressDelivery()
{
    KisAcyclicSignalConnector connector;
    SignalEndpoint sender;
    SignalEndpoint receiver;
    connector.connectForwardInt(&sender, SIGNAL(intSent(int)), &receiver, SLOT(receiveInt(int)));

    connector.lock();
    connector.lock();
    QVERIFY(connector.isLocked());
    Q_EMIT sender.intSent(1);
    QCOMPARE(receiver.intCalls, 0);

    connector.unlock();
    QVERIFY(connector.isLocked());
    Q_EMIT sender.intSent(2);
    QCOMPARE(receiver.intCalls, 0);

    connector.unlock();
    QVERIFY(!connector.isLocked());
    Q_EMIT sender.intSent(3);
    QCOMPARE(receiver.intCalls, 1);
    QCOMPARE(receiver.intValue, 3);

    {
        KisAcyclicSignalConnector::Blocker blocker(connector);
        QVERIFY(connector.isLocked());
        Q_EMIT sender.intSent(4);
        QCOMPARE(receiver.intCalls, 1);
    }
    QVERIFY(!connector.isLocked());
}

void KisAcyclicSignalConnectorContractTest::coordinatedConnectorsShareLocksAndLifetime()
{
    QObject owner;
    auto *connector = new KisAcyclicSignalConnector(&owner);
    QPointer<KisAcyclicSignalConnector> connectorPointer(connector);
    KisAcyclicSignalConnector *coordinated = connector->createCoordinatedConnector();
    QPointer<KisAcyclicSignalConnector> coordinatedPointer(coordinated);
    QCOMPARE(connector->parent(), &owner);
    QCOMPARE(coordinated->parent(), connector);

    SignalEndpoint parentSender;
    SignalEndpoint childSender;
    SignalEndpoint parentReceiver;
    SignalEndpoint childReceiver;
    connector->connectForwardInt(&parentSender, SIGNAL(intSent(int)), &parentReceiver, SLOT(receiveInt(int)));
    coordinated->connectForwardDouble(&childSender, SIGNAL(doubleSent(double)), &childReceiver, SLOT(receiveDouble(double)));

    coordinated->lock();
    QVERIFY(connector->isLocked());
    QVERIFY(coordinated->isLocked());
    Q_EMIT parentSender.intSent(5);
    Q_EMIT childSender.doubleSent(6.0);
    QCOMPARE(parentReceiver.intCalls, 0);
    QCOMPARE(childReceiver.doubleCalls, 0);

    coordinated->unlock();
    QVERIFY(!connector->isLocked());
    QVERIFY(!coordinated->isLocked());
    Q_EMIT parentSender.intSent(7);
    Q_EMIT childSender.doubleSent(8.0);
    QCOMPARE(parentReceiver.intValue, 7);
    QCOMPARE(childReceiver.doubleValue, 8.0);

    delete connector;
    QVERIFY(connectorPointer.isNull());
    QVERIFY(coordinatedPointer.isNull());
}

QTEST_GUILESS_MAIN(KisAcyclicSignalConnectorContractTest)

#include "KisAcyclicSignalConnectorContractTest.moc"
