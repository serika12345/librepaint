/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoGenericRegistry.h"
#include "KoGenericRegistryModel.h"

#include <QTest>

namespace
{

int safeAssertCount = 0;

struct RegistryItem {
    QString identifier;
    QString displayName;
    int *destructionCount = nullptr;

    ~RegistryItem()
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    QString id() const
    {
        return identifier;
    }

    QString name() const
    {
        return displayName;
    }
};

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KoGenericRegistryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyRegistryRejectsNullEntries();
    void itemsAliasesAndIterationExposeRegisteredValues();
    void duplicateIdRetainsPreviousEntryForDiagnosis();
    void registryDoesNotDestroyOwnedRawPointers();
    void modelReflectsRegistryRowsAndRoles();
    void modelReportsRootRowsForValidParent();
};

void KoGenericRegistryContractTest::emptyRegistryRejectsNullEntries()
{
    safeAssertCount = 0;
    KoGenericRegistry<RegistryItem *> registry;

    QCOMPARE(registry.count(), 0);
    QVERIFY(registry.keys().isEmpty());
    QVERIFY(registry.values().isEmpty());
    QVERIFY(registry.constBegin() == registry.constEnd());

    registry.add(nullptr);
    registry.add(QStringLiteral("explicit"), nullptr);

    QCOMPARE(registry.count(), 0);
    QCOMPARE(safeAssertCount, 2);
}

void KoGenericRegistryContractTest::itemsAliasesAndIterationExposeRegisteredValues()
{
    RegistryItem alpha{QStringLiteral("alpha"), QStringLiteral("Alpha")};
    RegistryItem beta{QStringLiteral("unused"), QStringLiteral("Beta")};
    KoGenericRegistry<RegistryItem *> registry;

    registry.add(&alpha);
    registry.add(QStringLiteral("beta"), &beta);

    QCOMPARE(registry.count(), 2);
    QVERIFY(registry.contains(QStringLiteral("alpha")));
    QCOMPARE(registry.get(QStringLiteral("alpha")), &alpha);
    QCOMPARE(registry.value(QStringLiteral("beta")), &beta);
    QVERIFY(registry.keys().contains(QStringLiteral("alpha")));
    QVERIFY(registry.keys().contains(QStringLiteral("beta")));
    QVERIFY(registry.values().contains(&alpha));
    QVERIFY(registry.values().contains(&beta));

    QList<RegistryItem *> iterated;
    for (auto it = registry.constBegin(); it != registry.constEnd(); ++it) {
        iterated.append(it.value());
    }
    QCOMPARE(iterated.size(), 2);
    QVERIFY(iterated.contains(&alpha));
    QVERIFY(iterated.contains(&beta));

    registry.addAlias(QStringLiteral("alternate"), QStringLiteral("alpha"));
    QVERIFY(registry.contains(QStringLiteral("alternate")));
    QCOMPARE(registry.get(QStringLiteral("alternate")), &alpha);
    registry.removeAlias(QStringLiteral("alternate"));
    QVERIFY(!registry.contains(QStringLiteral("alternate")));

    registry.remove(QStringLiteral("beta"));
    QCOMPARE(registry.count(), 1);
    QVERIFY(!registry.contains(QStringLiteral("beta")));
}

void KoGenericRegistryContractTest::duplicateIdRetainsPreviousEntryForDiagnosis()
{
    RegistryItem previous{QStringLiteral("same"), QStringLiteral("Previous")};
    RegistryItem replacement{QStringLiteral("same"), QStringLiteral("Replacement")};
    KoGenericRegistry<RegistryItem *> registry;

    registry.add(&previous);
    registry.add(&replacement);

    QCOMPARE(registry.count(), 1);
    QCOMPARE(registry.get(QStringLiteral("same")), &replacement);
    QCOMPARE(registry.doubleEntries(), QList<RegistryItem *>({&previous}));
}

void KoGenericRegistryContractTest::registryDoesNotDestroyOwnedRawPointers()
{
    int destructionCount = 0;
    auto *item = new RegistryItem{QStringLiteral("owned"), QStringLiteral("Owned"), &destructionCount};

    {
        KoGenericRegistry<RegistryItem *> registry;
        registry.add(item);
    }

    const int destructionCountAfterRegistry = destructionCount;
    delete item;
    QCOMPARE(destructionCountAfterRegistry, 0);
}

void KoGenericRegistryContractTest::modelReflectsRegistryRowsAndRoles()
{
    RegistryItem alpha{QStringLiteral("alpha"), QStringLiteral("Alpha")};
    RegistryItem beta{QStringLiteral("beta"), QStringLiteral("Beta")};
    KoGenericRegistry<RegistryItem *> registry;
    registry.add(&alpha);
    registry.add(&beta);

    KoGenericRegistryModel<RegistryItem *> model(&registry);
    QCOMPARE(model.rowCount(), 2);

    const QModelIndex first = model.index(0, 0);
    QVERIFY(first.isValid());
    const RegistryItem *firstItem = registry.get(registry.keys().at(0));
    QCOMPARE(model.get(first), firstItem);
    QCOMPARE(model.data(first).toString(), firstItem->name());
    QCOMPARE(model.data(first, Qt::EditRole).toString(), firstItem->name());
    QVERIFY(!model.data(first, Qt::UserRole).isValid());
    QVERIFY(!model.data(QModelIndex()).isValid());
}

void KoGenericRegistryContractTest::modelReportsRootRowsForValidParent()
{
    RegistryItem alpha{QStringLiteral("alpha"), QStringLiteral("Alpha")};
    RegistryItem beta{QStringLiteral("beta"), QStringLiteral("Beta")};
    KoGenericRegistry<RegistryItem *> registry;
    registry.add(&alpha);
    registry.add(&beta);

    KoGenericRegistryModel<RegistryItem *> model(&registry);
    QCOMPARE(model.rowCount(model.index(0, 0)), 2);
}

QTEST_GUILESS_MAIN(KoGenericRegistryContractTest)

#include "KoGenericRegistryContractTest.moc"
