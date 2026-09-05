/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMemoryStorage.h"

#include <QTest>

#include <type_traits>

namespace
{
using Storage = KisMemoryStorage;

#define ASSERT_MEMORY_STORAGE_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Storage::method)), signature>)
} // namespace

class KisMemoryStorageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void memoryStorageTypeLifetimeAndConstructionSchemaRemainStable();
    void memoryStorageCopyAndResourceMutationSignaturesRemainStable();
    void memoryStorageResourceTransferAndLookupSignaturesRemainStable();
    void memoryStorageMetadataSignaturesRemainStable();
    void memoryStorageIteratorSignaturesRemainStable();
};

void KisMemoryStorageSchemaContractTest::memoryStorageTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Storage>);
    static_assert(std::is_base_of_v<KisStoragePlugin, Storage>);
    static_assert(std::is_copy_constructible_v<Storage>);
    static_assert(std::is_constructible_v<Storage, const QString &>);
    static_assert(std::is_default_constructible_v<Storage>);
    static_assert(std::has_virtual_destructor_v<Storage>);
}

void KisMemoryStorageSchemaContractTest::memoryStorageCopyAndResourceMutationSignaturesRemainStable()
{
    ASSERT_MEMORY_STORAGE_SIGNATURE(operator=, Storage & (Storage::*)(const Storage &));
    ASSERT_MEMORY_STORAGE_SIGNATURE(addResource, bool (Storage::*)(const QString &, KoResourceSP));
    ASSERT_MEMORY_STORAGE_SIGNATURE(loadVersionedResource, bool (Storage::*)(KoResourceSP));
    ASSERT_MEMORY_STORAGE_SIGNATURE(saveAsNewVersion, bool (Storage::*)(const QString &, KoResourceSP));
}

void KisMemoryStorageSchemaContractTest::memoryStorageResourceTransferAndLookupSignaturesRemainStable()
{
    ASSERT_MEMORY_STORAGE_SIGNATURE(exportResource, bool (Storage::*)(const QString &, QIODevice *));
    ASSERT_MEMORY_STORAGE_SIGNATURE(importResource, bool (Storage::*)(const QString &, QIODevice *));
    ASSERT_MEMORY_STORAGE_SIGNATURE(resourceItem, KisResourceStorage::ResourceItem (Storage::*)(const QString &));
    ASSERT_MEMORY_STORAGE_SIGNATURE(resourceMd5, QString (Storage::*)(const QString &));
}

void KisMemoryStorageSchemaContractTest::memoryStorageMetadataSignaturesRemainStable()
{
    ASSERT_MEMORY_STORAGE_SIGNATURE(metaData, QVariant (Storage::*)(const QString &) const);
    ASSERT_MEMORY_STORAGE_SIGNATURE(metaDataKeys, QStringList (Storage::*)() const);
    ASSERT_MEMORY_STORAGE_SIGNATURE(setMetaData, void (Storage::*)(const QString &, const QVariant &));
}

void KisMemoryStorageSchemaContractTest::memoryStorageIteratorSignaturesRemainStable()
{
    ASSERT_MEMORY_STORAGE_SIGNATURE(resources,
                                    QSharedPointer<KisResourceStorage::ResourceIterator> (Storage::*)(const QString &));
    ASSERT_MEMORY_STORAGE_SIGNATURE(tags,
                                    QSharedPointer<KisResourceStorage::TagIterator> (Storage::*)(const QString &));
}

QTEST_GUILESS_MAIN(KisMemoryStorageSchemaContractTest)

#include "KisMemoryStorageSchemaContractTest.moc"
