/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisToolChangesTrackerData.h"

#include <memory>

#include <QMetaType>
#include <QTest>
#include <QVariant>
#include <QWeakPointer>

namespace
{

class DestructionProbe : public KisToolChangesTrackerData
{
public:
    explicit DestructionProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~DestructionProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

class KisToolChangesTrackerDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseCloneCreatesAnIndependentValue();
    void sharedAliasIsRegisteredAndOwnsVirtualLifetime();
};

void KisToolChangesTrackerDataContractTest::baseCloneCreatesAnIndependentValue()
{
    KisToolChangesTrackerData original;
    const std::unique_ptr<KisToolChangesTrackerData> clone(original.clone());

    QVERIFY(clone);
    QVERIFY(clone.get() != &original);
}

void KisToolChangesTrackerDataContractTest::sharedAliasIsRegisteredAndOwnsVirtualLifetime()
{
    const QMetaType registeredType = QMetaType::fromName("KisToolChangesTrackerDataSP");
    QVERIFY(registeredType.isValid());
    QCOMPARE(registeredType.id(), QMetaType::fromType<KisToolChangesTrackerDataSP>().id());

    int destructionCount = 0;
    KisToolChangesTrackerDataSP owner(new DestructionProbe(&destructionCount));
    const QWeakPointer<KisToolChangesTrackerData> observer(owner);
    QVariant storedOwner = QVariant::fromValue(owner);

    owner.clear();
    QCOMPARE(destructionCount, 0);
    QVERIFY(!observer.isNull());

    storedOwner.clear();
    QCOMPARE(destructionCount, 1);
    QVERIFY(observer.isNull());
}

QTEST_GUILESS_MAIN(KisToolChangesTrackerDataContractTest)

#include "KisToolChangesTrackerDataContractTest.moc"
