/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoResourceUpdateMediator.h"

#include <QList>
#include <QSignalSpy>
#include <QTest>
#include <QVariant>

namespace
{
class MediatorProbe : public KoResourceUpdateMediator
{
public:
    explicit MediatorProbe(int key, int *destructionCount = nullptr)
        : KoResourceUpdateMediator(key)
        , m_destructionCount(destructionCount)
    {
    }

    ~MediatorProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void connectResource(QVariant sourceResource) override
    {
        connectedResources.append(sourceResource);
    }

    void emitResourceChanged(int key)
    {
        Q_EMIT sigResourceChanged(key);
    }

    QList<QVariant> connectedResources;

private:
    int *m_destructionCount;
};
} // namespace

class KoResourceUpdateMediatorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesKeyAndDispatchesResourceConnections();
    void deliversChangeSignalArgumentsInOrder();
    void sharedAliasKeepsVirtualLifetime();
};

void KoResourceUpdateMediatorContractTest::storesKeyAndDispatchesResourceConnections()
{
    MediatorProbe mediator(-19);
    KoResourceUpdateMediator *interface = &mediator;

    QVERIFY(!interface->parent());
    QCOMPARE(interface->key(), -19);

    interface->connectResource(QStringLiteral("source"));
    interface->connectResource(42);

    QCOMPARE(mediator.connectedResources, QList<QVariant>({QVariant(QStringLiteral("source")), QVariant(42)}));
}

void KoResourceUpdateMediatorContractTest::deliversChangeSignalArgumentsInOrder()
{
    MediatorProbe mediator(73);
    QSignalSpy spy(&mediator, &KoResourceUpdateMediator::sigResourceChanged);

    mediator.emitResourceChanged(mediator.key());
    mediator.emitResourceChanged(2048);

    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.at(0).at(0), QVariant(73));
    QCOMPARE(spy.at(1).at(0), QVariant(2048));
}

void KoResourceUpdateMediatorContractTest::sharedAliasKeepsVirtualLifetime()
{
    int destructionCount = 0;
    KoResourceUpdateMediatorSP owner(new MediatorProbe(5, &destructionCount));
    KoResourceUpdateMediatorSP copy = owner;

    QCOMPARE(copy->key(), 5);
    owner.clear();
    QCOMPARE(destructionCount, 0);

    copy->connectResource(QStringLiteral("retained"));
    QCOMPARE(static_cast<MediatorProbe *>(copy.data())->connectedResources,
             QList<QVariant>({QVariant(QStringLiteral("retained"))}));

    copy.clear();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoResourceUpdateMediatorContractTest)

#include "KoResourceUpdateMediatorContractTest.moc"
