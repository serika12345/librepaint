/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_meta_data_entry.h>
#include <kis_meta_data_store.h>

#include <QTest>

#include <type_traits>

namespace
{
using StoreConstIterator = QHash<QString, KisMetaData::Entry>::const_iterator;

#define ASSERT_STORE_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMetaData::Store::method)), signature>)
} // namespace

class KisMetaDataStoreSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void metaDataStoreIdentityAndLifecycleSignaturesRemainStable();
    void metaDataStoreEntryMutationSignaturesRemainStable();
    void metaDataStoreLookupSignaturesRemainStable();
    void metaDataStoreQuerySignaturesRemainStable();
    void metaDataStoreIterationFilterAndDebugSignaturesRemainStable();
};

void KisMetaDataStoreSchemaContractTest::metaDataStoreIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KisMetaData::Store>);
    static_assert(std::is_default_constructible_v<KisMetaData::Store>);
    static_assert(std::is_copy_constructible_v<KisMetaData::Store>);
    static_assert(std::is_destructible_v<KisMetaData::Store>);
}

void KisMetaDataStoreSchemaContractTest::metaDataStoreEntryMutationSignaturesRemainStable()
{
    ASSERT_STORE_SIGNATURE(addEntry, bool (KisMetaData::Store::*)(const KisMetaData::Entry &));
    ASSERT_STORE_SIGNATURE(copyFrom, void (KisMetaData::Store::*)(const KisMetaData::Store *));
    ASSERT_STORE_SIGNATURE(removeEntry, void (KisMetaData::Store::*)(const KisMetaData::Schema *, const QString &));
    ASSERT_STORE_SIGNATURE(removeEntry, void (KisMetaData::Store::*)(const QString &));
    ASSERT_STORE_SIGNATURE(removeEntry, void (KisMetaData::Store::*)(const QString &, const QString &));
}

void KisMetaDataStoreSchemaContractTest::metaDataStoreLookupSignaturesRemainStable()
{
    ASSERT_STORE_SIGNATURE(getEntry,
                           KisMetaData::Entry & (KisMetaData::Store::*)(const KisMetaData::Schema *, const QString &));
    ASSERT_STORE_SIGNATURE(
        getEntry,
        const KisMetaData::Entry &(KisMetaData::Store::*)(const KisMetaData::Schema *, const QString &) const);
    ASSERT_STORE_SIGNATURE(getEntry, KisMetaData::Entry & (KisMetaData::Store::*)(const QString &));
    ASSERT_STORE_SIGNATURE(getEntry, const KisMetaData::Entry &(KisMetaData::Store::*)(const QString &) const);
    ASSERT_STORE_SIGNATURE(getEntry, KisMetaData::Entry & (KisMetaData::Store::*)(const QString &, const QString &));
    ASSERT_STORE_SIGNATURE(getEntry,
                           const KisMetaData::Entry &(KisMetaData::Store::*)(const QString &, const QString &) const);
    ASSERT_STORE_SIGNATURE(getValue,
                           const KisMetaData::Value &(KisMetaData::Store::*)(const QString &, const QString &) const);
}

void KisMetaDataStoreSchemaContractTest::metaDataStoreQuerySignaturesRemainStable()
{
    ASSERT_STORE_SIGNATURE(containsEntry,
                           bool (KisMetaData::Store::*)(const KisMetaData::Schema *, const QString &) const);
    ASSERT_STORE_SIGNATURE(containsEntry, bool (KisMetaData::Store::*)(const QString &) const);
    ASSERT_STORE_SIGNATURE(containsEntry, bool (KisMetaData::Store::*)(const QString &, const QString &) const);
    ASSERT_STORE_SIGNATURE(empty, bool (KisMetaData::Store::*)() const);
    ASSERT_STORE_SIGNATURE(isEmpty, bool (KisMetaData::Store::*)() const);
    ASSERT_STORE_SIGNATURE(entries, QList<KisMetaData::Entry> (KisMetaData::Store::*)() const);
    ASSERT_STORE_SIGNATURE(keys, QList<QString> (KisMetaData::Store::*)() const);
}

void KisMetaDataStoreSchemaContractTest::metaDataStoreIterationFilterAndDebugSignaturesRemainStable()
{
    ASSERT_STORE_SIGNATURE(applyFilters, void (KisMetaData::Store::*)(const QList<const KisMetaData::Filter *> &));
    ASSERT_STORE_SIGNATURE(begin, StoreConstIterator (KisMetaData::Store::*)() const);
    ASSERT_STORE_SIGNATURE(debugDump, void (KisMetaData::Store::*)() const);
    ASSERT_STORE_SIGNATURE(end, StoreConstIterator (KisMetaData::Store::*)() const);
}

QTEST_APPLESS_MAIN(KisMetaDataStoreSchemaContractTest)

#include "KisMetaDataStoreSchemaContractTest.moc"
