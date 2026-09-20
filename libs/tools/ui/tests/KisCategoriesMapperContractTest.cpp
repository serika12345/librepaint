/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_categories_mapper.h"

#include <QMetaMethod>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

namespace {

struct StringEntryName
{
    QString operator()(const QString &entry) const
    {
        return QStringLiteral("entry:") + entry;
    }
};

using StringMapper = KisCategoriesMapper<QString, StringEntryName>;

}

class KisCategoriesMapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionCategoryAndEntryIdentityRemainStable();
    void itemStateDefaultsMutationsAndNotificationsRemainStable();
    void categoryEntryInsertionAndLookupRemainStable();
    void rowMappingRemovalAndExpansionRemainStable();
    void rowSignalProtocolRemainsStable();
};

void KisCategoriesMapperContractTest::constructionCategoryAndEntryIdentityRemainStable()
{
    StringMapper mapper;
    __CategoriesSignalsBase *const signalsBase = &mapper;
    QCOMPARE(signalsBase, static_cast<__CategoriesSignalsBase *>(&mapper));

    StringMapper::DataItem *const category = mapper.addCategory(QStringLiteral("drawing"));
    QVERIFY(category);
    QVERIFY(category->isCategory());
    QCOMPARE(category->name(), QStringLiteral("drawing"));
    QCOMPARE(category->data(), nullptr);
    QCOMPARE(category->parentCategory(), nullptr);

    QString originalEntry = QStringLiteral("ink");
    StringMapper::DataItem *const entry = mapper.addEntry(QStringLiteral("drawing"), originalEntry);
    QVERIFY(entry);
    QVERIFY(!entry->isCategory());
    QCOMPARE(entry->name(), QStringLiteral("entry:ink"));
    QCOMPARE(entry->parentCategory(), category);
    QVERIFY(entry->data());
    QCOMPARE(*entry->data(), QStringLiteral("ink"));

    originalEntry = QStringLiteral("changed-after-insertion");
    QCOMPARE(*entry->data(), QStringLiteral("ink"));

    QPointer<__CategoriesSignalsBase> lifetime;
    lifetime = new StringMapper;
    QVERIFY(lifetime);
    delete lifetime.data();
    QVERIFY(lifetime.isNull());
}

void KisCategoriesMapperContractTest::itemStateDefaultsMutationsAndNotificationsRemainStable()
{
    StringMapper mapper;
    StringMapper::DataItem *const category = mapper.addCategory(QStringLiteral("drawing"));
    StringMapper::DataItem *const entry = mapper.addEntry(QStringLiteral("drawing"), QStringLiteral("ink"));
    QSignalSpy changedSpy(&mapper, &__CategoriesSignalsBase::rowChanged);

    QVERIFY(!entry->isExpanded());
    QVERIFY(entry->isEnabled());
    QVERIFY(!entry->isCheckable());
    QVERIFY(!entry->isChecked());
    QVERIFY(!entry->isLocked());
    QVERIFY(!entry->isLockable());
    QVERIFY(!entry->isToggled());

    entry->setEnabled(false);
    QVERIFY(!entry->isEnabled());
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).at(0).toInt(), 1);
    entry->setEnabled(false);
    QCOMPARE(changedSpy.count(), 1);

    entry->setCheckable(true);
    QVERIFY(entry->isCheckable());
    QCOMPARE(changedSpy.count(), 2);
    entry->setCheckable(true);
    QCOMPARE(changedSpy.count(), 2);

    entry->setChecked(true);
    QVERIFY(entry->isChecked());
    QVERIFY(entry->isToggled());
    QCOMPARE(changedSpy.count(), 3);
    entry->setChecked(false);
    QVERIFY(!entry->isChecked());

    // Maintained characterization: every actual checked-state transition sets
    // toggled, including a transition back to the unchecked state.
    QVERIFY(entry->isToggled());
    QCOMPARE(changedSpy.count(), 4);
    entry->setChecked(false);
    QCOMPARE(changedSpy.count(), 4);

    entry->setLocked(true);
    entry->setLockable(true);
    entry->setToggled(false);
    QVERIFY(entry->isLocked());
    QVERIFY(entry->isLockable());
    QVERIFY(!entry->isToggled());

    // Maintained characterization: these three state setters do not notify.
    QCOMPARE(changedSpy.count(), 4);

    category->setExpanded(true);
    QVERIFY(category->isExpanded());
    QCOMPARE(changedSpy.count(), 6);
    QCOMPARE(changedSpy.at(4).at(0).toInt(), 0);
    QCOMPARE(changedSpy.at(5).at(0).toInt(), 1);
    category->setExpanded(true);
    QCOMPARE(changedSpy.count(), 6);
}

void KisCategoriesMapperContractTest::categoryEntryInsertionAndLookupRemainStable()
{
    StringMapper mapper;
    QSignalSpy beginInsertSpy(&mapper, &__CategoriesSignalsBase::beginInsertRow);
    QSignalSpy endInsertSpy(&mapper, &__CategoriesSignalsBase::endInsertRow);

    StringMapper::DataItem *const alphaOne =
        mapper.addEntry(QStringLiteral("alpha"), QStringLiteral("one"));
    QVERIFY(alphaOne);
    QCOMPARE(mapper.rowCount(), 2);
    QCOMPARE(beginInsertSpy.count(), 2);
    QCOMPARE(beginInsertSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(beginInsertSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(endInsertSpy.count(), 2);

    StringMapper::DataItem *const alpha = mapper.fetchCategory(QStringLiteral("alpha"));
    QVERIFY(alpha);
    QCOMPARE(alphaOne, mapper.fetchEntry(QStringLiteral("alpha"), QStringLiteral("one")));

    beginInsertSpy.clear();
    endInsertSpy.clear();
    QCOMPARE(mapper.addCategory(QStringLiteral("alpha")), nullptr);
    QCOMPARE(beginInsertSpy.count(), 0);
    QCOMPARE(endInsertSpy.count(), 0);

    StringMapper::DataItem *const betaOne =
        mapper.addEntry(QStringLiteral("beta"), QStringLiteral("one"));
    StringMapper::DataItem *const alphaTwo =
        mapper.addEntry(QStringLiteral("alpha"), QStringLiteral("two"));
    QVERIFY(betaOne);
    QVERIFY(alphaTwo);

    QCOMPARE(mapper.fetchEntry(QStringLiteral("beta"), QStringLiteral("one")), betaOne);
    QCOMPARE(mapper.fetchOneEntry(QStringLiteral("one")), alphaOne);
    QCOMPARE(mapper.fetchEntry(QStringLiteral("alpha"), QStringLiteral("missing")), nullptr);
    QCOMPARE(mapper.fetchCategory(QStringLiteral("missing")), nullptr);

    const QVector<StringMapper::DataItem *> alphaItems = mapper.itemsForCategory(QStringLiteral("alpha"));
    QCOMPARE(alphaItems, QVector<StringMapper::DataItem *>({alphaOne, alphaTwo}));
    QCOMPARE(mapper.itemsForCategory(QStringLiteral("missing")).size(), 0);
    QCOMPARE(mapper.rowCount(), 5);
}

void KisCategoriesMapperContractTest::rowMappingRemovalAndExpansionRemainStable()
{
    StringMapper mapper;
    StringMapper::DataItem *const alphaOne =
        mapper.addEntry(QStringLiteral("alpha"), QStringLiteral("one"));
    StringMapper::DataItem *const alphaTwo =
        mapper.addEntry(QStringLiteral("alpha"), QStringLiteral("two"));
    StringMapper::DataItem *const betaThree =
        mapper.addEntry(QStringLiteral("beta"), QStringLiteral("three"));
    QVERIFY(alphaOne);

    for (int row = 0; row < mapper.rowCount(); ++row) {
        QCOMPARE(mapper.rowFromItem(mapper.itemFromRow(row)), row);
    }

    StringMapper::DataItem unlistedCategory(QStringLiteral("unlisted"), &mapper);
    QCOMPARE(mapper.rowFromItem(&unlistedCategory), -1);

    QSignalSpy changedSpy(&mapper, &__CategoriesSignalsBase::rowChanged);
    mapper.expandAllCategories();
    QCOMPARE(changedSpy.count(), 5);
    for (int row = 0; row < changedSpy.count(); ++row) {
        QCOMPARE(changedSpy.at(row).at(0).toInt(), row);
    }
    QVERIFY(mapper.fetchCategory(QStringLiteral("alpha"))->isExpanded());
    QVERIFY(mapper.fetchCategory(QStringLiteral("beta"))->isExpanded());
    mapper.expandAllCategories();
    QCOMPARE(changedSpy.count(), 5);

    QSignalSpy beginRemoveSpy(&mapper, &__CategoriesSignalsBase::beginRemoveRow);
    QSignalSpy endRemoveSpy(&mapper, &__CategoriesSignalsBase::endRemoveRow);
    mapper.removeEntry(QStringLiteral("alpha"), QStringLiteral("one"));
    QCOMPARE(beginRemoveSpy.count(), 1);
    QCOMPARE(beginRemoveSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(endRemoveSpy.count(), 1);
    QCOMPARE(mapper.fetchOneEntry(QStringLiteral("one")), nullptr);
    QCOMPARE(mapper.rowFromItem(alphaTwo), 1);
    QCOMPARE(mapper.rowFromItem(betaThree), 3);

    mapper.removeEntry(QStringLiteral("alpha"), QStringLiteral("missing"));
    QCOMPARE(beginRemoveSpy.count(), 1);
    QCOMPARE(endRemoveSpy.count(), 1);

    mapper.removeCategory(QStringLiteral("alpha"));
    QCOMPARE(beginRemoveSpy.count(), 3);
    QCOMPARE(beginRemoveSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(beginRemoveSpy.at(2).at(0).toInt(), 0);
    QCOMPARE(endRemoveSpy.count(), 3);
    QCOMPARE(mapper.fetchCategory(QStringLiteral("alpha")), nullptr);
    QCOMPARE(mapper.rowCount(), 2);

    mapper.removeCategory(QStringLiteral("missing"));
    QCOMPARE(beginRemoveSpy.count(), 3);
    QCOMPARE(endRemoveSpy.count(), 3);

    // itemFromRow() intentionally has no invalid-row guard; only valid rows are
    // part of this contract.
    QCOMPARE(mapper.itemFromRow(0), mapper.fetchCategory(QStringLiteral("beta")));
}

void KisCategoriesMapperContractTest::rowSignalProtocolRemainsStable()
{
    const QMetaMethod rowChanged = QMetaMethod::fromSignal(&__CategoriesSignalsBase::rowChanged);
    const QMetaMethod beginInsert = QMetaMethod::fromSignal(&__CategoriesSignalsBase::beginInsertRow);
    const QMetaMethod endInsert = QMetaMethod::fromSignal(&__CategoriesSignalsBase::endInsertRow);
    const QMetaMethod beginRemove = QMetaMethod::fromSignal(&__CategoriesSignalsBase::beginRemoveRow);
    const QMetaMethod endRemove = QMetaMethod::fromSignal(&__CategoriesSignalsBase::endRemoveRow);

    QCOMPARE(rowChanged.methodSignature(), QByteArray("rowChanged(int)"));
    QCOMPARE(beginInsert.methodSignature(), QByteArray("beginInsertRow(int)"));
    QCOMPARE(endInsert.methodSignature(), QByteArray("endInsertRow()"));
    QCOMPARE(beginRemove.methodSignature(), QByteArray("beginRemoveRow(int)"));
    QCOMPARE(endRemove.methodSignature(), QByteArray("endRemoveRow()"));

    StringMapper mapper;
    QStringList events;
    connect(&mapper, &__CategoriesSignalsBase::beginInsertRow, &mapper, [&events](int row) {
        events.append(QStringLiteral("begin-insert:%1").arg(row));
    });
    connect(&mapper, &__CategoriesSignalsBase::endInsertRow, &mapper, [&events]() {
        events.append(QStringLiteral("end-insert"));
    });
    connect(&mapper, &__CategoriesSignalsBase::beginRemoveRow, &mapper, [&events](int row) {
        events.append(QStringLiteral("begin-remove:%1").arg(row));
    });
    connect(&mapper, &__CategoriesSignalsBase::endRemoveRow, &mapper, [&events]() {
        events.append(QStringLiteral("end-remove"));
    });
    connect(&mapper, &__CategoriesSignalsBase::rowChanged, &mapper, [&events](int row) {
        events.append(QStringLiteral("changed:%1").arg(row));
    });

    StringMapper::DataItem *const entry =
        mapper.addEntry(QStringLiteral("alpha"), QStringLiteral("one"));
    entry->setEnabled(false);
    mapper.removeEntry(QStringLiteral("alpha"), QStringLiteral("one"));

    QCOMPARE(events,
             QStringList({QStringLiteral("begin-insert:0"),
                          QStringLiteral("end-insert"),
                          QStringLiteral("begin-insert:1"),
                          QStringLiteral("end-insert"),
                          QStringLiteral("changed:1"),
                          QStringLiteral("begin-remove:1"),
                          QStringLiteral("end-remove")}));
}

QTEST_GUILESS_MAIN(KisCategoriesMapperContractTest)

#include "KisCategoriesMapperContractTest.moc"
