/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisAbrStorage.h>
#include <KisBrushTypeMetaDataFixup.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ABR_STORAGE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAbrStorage::method)), signature>)
} // namespace

class KisBrushStorageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abrStorageTypeOwnershipAndLifetimeSchemaRemainStable();
    void abrStorageResourceLookupAndVersioningSchemaRemainStable();
    void abrStorageIterationAndThumbnailSchemaRemainStable();
    void metadataFixupSchemaRemainsStable();
};

void KisBrushStorageSchemaContractTest::abrStorageTypeOwnershipAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAbrStorage>);
    static_assert(std::is_constructible_v<KisAbrStorage, const QString &>);
    static_assert(std::has_virtual_destructor_v<KisAbrStorage>);
    static_assert(
        std::is_same_v<decltype(&KisAbrStorage::m_brushCollection), KisAbrBrushCollectionSP KisAbrStorage::*>);
}

void KisBrushStorageSchemaContractTest::abrStorageResourceLookupAndVersioningSchemaRemainStable()
{
    ASSERT_ABR_STORAGE_SIGNATURE(resourceItem, KisResourceStorage::ResourceItem (KisAbrStorage::*)(const QString &));
    ASSERT_ABR_STORAGE_SIGNATURE(resource, KoResourceSP (KisAbrStorage::*)(const QString &));
    ASSERT_ABR_STORAGE_SIGNATURE(loadVersionedResource, bool (KisAbrStorage::*)(KoResourceSP));
    ASSERT_ABR_STORAGE_SIGNATURE(supportsVersioning, bool (KisAbrStorage::*)() const);
}

void KisBrushStorageSchemaContractTest::abrStorageIterationAndThumbnailSchemaRemainStable()
{
    ASSERT_ABR_STORAGE_SIGNATURE(
        resources,
        QSharedPointer<KisResourceStorage::ResourceIterator> (KisAbrStorage::*)(const QString &));
    ASSERT_ABR_STORAGE_SIGNATURE(tags,
                                 QSharedPointer<KisResourceStorage::TagIterator> (KisAbrStorage::*)(const QString &));
    ASSERT_ABR_STORAGE_SIGNATURE(thumbnail, QImage (KisAbrStorage::*)() const);
}

void KisBrushStorageSchemaContractTest::metadataFixupSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisBrushTypeMetaDataFixup>);
    static_assert(
        std::is_same_v<decltype(&KisBrushTypeMetaDataFixup::executeFix), QStringList (KisBrushTypeMetaDataFixup::*)()>);
}

QTEST_GUILESS_MAIN(KisBrushStorageSchemaContractTest)

#include "KisBrushStorageSchemaContractTest.moc"
