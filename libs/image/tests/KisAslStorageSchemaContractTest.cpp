/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAslStorage.h"

#include <QTest>

#include <type_traits>

#define ASSERT_ASL_STORAGE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAslStorage::method)), signature>)

class KisAslStorageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aslStorageTypeOwnershipAndLifetimeSchemaRemainStable();
    void aslStorageResourceLookupAndVersioningSchemaRemainStable();
    void aslStorageIterationSignaturesRemainStable();
    void aslStorageMutationAndValiditySignaturesRemainStable();
};

void KisAslStorageSchemaContractTest::aslStorageTypeOwnershipAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAslStorage>);
    static_assert(std::is_base_of_v<KisStoragePlugin, KisAslStorage>);
    static_assert(std::is_constructible_v<KisAslStorage, const QString &>);
    static_assert(std::has_virtual_destructor_v<KisAslStorage>);
    static_assert(std::is_same_v<decltype(&KisAslStorage::m_aslSerializer),
                                 QSharedPointer<KisAslLayerStyleSerializer> KisAslStorage::*>);
}

void KisAslStorageSchemaContractTest::aslStorageResourceLookupAndVersioningSchemaRemainStable()
{
    ASSERT_ASL_STORAGE_SIGNATURE(resourceItem, KisResourceStorage::ResourceItem (KisAslStorage::*)(const QString &));
    ASSERT_ASL_STORAGE_SIGNATURE(resource, KoResourceSP (KisAslStorage::*)(const QString &));
    ASSERT_ASL_STORAGE_SIGNATURE(loadVersionedResource, bool (KisAslStorage::*)(KoResourceSP));
    ASSERT_ASL_STORAGE_SIGNATURE(supportsVersioning, bool (KisAslStorage::*)() const);
}

void KisAslStorageSchemaContractTest::aslStorageIterationSignaturesRemainStable()
{
    ASSERT_ASL_STORAGE_SIGNATURE(
        resources,
        QSharedPointer<KisResourceStorage::ResourceIterator> (KisAslStorage::*)(const QString &));
    ASSERT_ASL_STORAGE_SIGNATURE(tags,
                                 QSharedPointer<KisResourceStorage::TagIterator> (KisAslStorage::*)(const QString &));
}

void KisAslStorageSchemaContractTest::aslStorageMutationAndValiditySignaturesRemainStable()
{
    ASSERT_ASL_STORAGE_SIGNATURE(saveAsNewVersion, bool (KisAslStorage::*)(const QString &, KoResourceSP));
    ASSERT_ASL_STORAGE_SIGNATURE(addResource, bool (KisAslStorage::*)(const QString &, KoResourceSP));
    ASSERT_ASL_STORAGE_SIGNATURE(isValid, bool (KisAslStorage::*)() const);
}

#undef ASSERT_ASL_STORAGE_SIGNATURE

QTEST_GUILESS_MAIN(KisAslStorageSchemaContractTest)

#include "KisAslStorageSchemaContractTest.moc"
