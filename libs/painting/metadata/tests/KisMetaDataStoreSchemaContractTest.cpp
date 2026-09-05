/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_meta_data_entry.h>

#include <kis_meta_data_store.h>
#include <kis_meta_data_validator.h>

#include <QTest>

#include <type_traits>

namespace
{
using StoreConstIterator = QHash<QString, KisMetaData::Entry>::const_iterator;
using Entry = KisMetaData::Entry;

#define ASSERT_STORE_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMetaData::Store::method)), signature>)
#define ASSERT_ENTRY_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Entry::method)), signature>)
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
    void metaDataValidationReasonTypeSchemaRemainStable();
    void metaDataValidationReasonValueSemanticsSchemaRemainStable();
    void metaDataValidatorTypeAndLifetimeSchemaRemainStable();
    void metaDataValidatorCountSignaturesRemainStable();
    void metaDataValidatorEntryAndRevalidationSignaturesRemainStable();
    void metaDataEntryTypeLifetimeAndConstructionSchemaRemainStable();
    void metaDataEntryIdentityAndSchemaSignaturesRemainStable();
    void metaDataEntryValueAndAssignmentSignaturesRemainStable();
    void metaDataEntryValidationAndEqualitySignaturesRemainStable();
    void metaDataEntryDebugSignatureRemainsStable();
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

void KisMetaDataStoreSchemaContractTest::metaDataValidationReasonTypeSchemaRemainStable()
{
    using Reason = KisMetaData::Validator::Reason;
    using ReasonType = Reason::Type;

    static_assert(std::is_class_v<Reason>);
    static_assert(std::is_enum_v<ReasonType>);
    static_assert(Reason::UNKNOWN_REASON == 0);
    static_assert(Reason::UNKNOWN_ENTRY == 1);
    static_assert(Reason::INVALID_TYPE == 2);
    static_assert(Reason::INVALID_VALUE == 3);
}

void KisMetaDataStoreSchemaContractTest::metaDataValidationReasonValueSemanticsSchemaRemainStable()
{
    using Reason = KisMetaData::Validator::Reason;
    using ReasonType = Reason::Type;

    static_assert(std::is_default_constructible_v<Reason>);
    static_assert(std::is_constructible_v<Reason, ReasonType>);
    static_assert(std::is_copy_constructible_v<Reason>);
    static_assert(std::is_same_v<decltype(static_cast<Reason &(Reason::*)(const Reason &)>(&Reason::operator=)),
                                 Reason &(Reason::*)(const Reason &)>);
    static_assert(std::is_destructible_v<Reason>);
    static_assert(std::is_same_v<decltype(&Reason::type), ReasonType (Reason::*)() const>);
}

void KisMetaDataStoreSchemaContractTest::metaDataValidatorTypeAndLifetimeSchemaRemainStable()
{
    using Validator = KisMetaData::Validator;

    static_assert(std::is_class_v<Validator>);
    static_assert(std::is_constructible_v<Validator, const KisMetaData::Store *>);
    static_assert(std::is_destructible_v<Validator>);
}

void KisMetaDataStoreSchemaContractTest::metaDataValidatorCountSignaturesRemainStable()
{
    using Validator = KisMetaData::Validator;

    static_assert(std::is_same_v<decltype(&Validator::countInvalidEntries), int (Validator::*)() const>);
    static_assert(std::is_same_v<decltype(&Validator::countValidEntries), int (Validator::*)() const>);
}

void KisMetaDataStoreSchemaContractTest::metaDataValidatorEntryAndRevalidationSignaturesRemainStable()
{
    using InvalidEntries = QMap<QString, KisMetaData::Validator::Reason>;
    using Validator = KisMetaData::Validator;

    static_assert(std::is_same_v<decltype(&Validator::invalidEntries), const InvalidEntries &(Validator::*)() const>);
    static_assert(std::is_same_v<decltype(&Validator::revalidate), void (Validator::*)()>);
}

void KisMetaDataStoreSchemaContractTest::metaDataEntryTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Entry>);
    static_assert(std::is_default_constructible_v<Entry>);
    static_assert(std::is_copy_constructible_v<Entry>);
    static_assert(std::is_constructible_v<Entry, const KisMetaData::Schema *, QString, const KisMetaData::Value &>);
    static_assert(std::is_destructible_v<Entry>);
}

void KisMetaDataStoreSchemaContractTest::metaDataEntryIdentityAndSchemaSignaturesRemainStable()
{
    ASSERT_ENTRY_SIGNATURE(name, QString (Entry::*)() const);
    ASSERT_ENTRY_SIGNATURE(qualifiedName, QString (Entry::*)() const);
    ASSERT_ENTRY_SIGNATURE(schema, const KisMetaData::Schema *(Entry::*)() const);
}

void KisMetaDataStoreSchemaContractTest::metaDataEntryValueAndAssignmentSignaturesRemainStable()
{
    ASSERT_ENTRY_SIGNATURE(operator=, Entry & (Entry::*)(const Entry &));
    ASSERT_ENTRY_SIGNATURE(value, KisMetaData::Value & (Entry::*)());
    ASSERT_ENTRY_SIGNATURE(value, const KisMetaData::Value &(Entry::*)() const);
}

void KisMetaDataStoreSchemaContractTest::metaDataEntryValidationAndEqualitySignaturesRemainStable()
{
    ASSERT_ENTRY_SIGNATURE(isValid, bool (Entry::*)() const);
    ASSERT_ENTRY_SIGNATURE(isValidName, bool (*)(const QString &));
    ASSERT_ENTRY_SIGNATURE(operator==, bool (Entry::*)(const Entry &) const);
}

void KisMetaDataStoreSchemaContractTest::metaDataEntryDebugSignatureRemainsStable()
{
    using DebugSignature = QDebug (*)(QDebug, const Entry &);
    static_assert(std::is_same_v<decltype(static_cast<DebugSignature>(&operator<<)), DebugSignature>);
}

QTEST_APPLESS_MAIN(KisMetaDataStoreSchemaContractTest)

#include "KisMetaDataStoreSchemaContractTest.moc"
