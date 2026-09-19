/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KisStorageModel.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KisTagResourceModel.h>

#include <QTest>

#include <array>
#include <utility>

class KisResourceModelEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnValuesRemainStable();
    void resourceFilterValuesRemainStable();
    void storageFilterValuesRemainStable();
    void storageTypeLabelSchemaRemainsStable();
    void storageVersionIteratorValueSchemaRemainsStable();
    void storagePluginFactorySchemaRemainsStable();
};

void KisResourceModelEnumContractTest::columnValuesRemainStable()
{
    const std::array<KisAbstractResourceModel::Columns, 18> columns {{
        KisAbstractResourceModel::Id,
        KisAbstractResourceModel::StorageId,
        KisAbstractResourceModel::Name,
        KisAbstractResourceModel::Filename,
        KisAbstractResourceModel::Tooltip,
        KisAbstractResourceModel::Thumbnail,
        KisAbstractResourceModel::Status,
        KisAbstractResourceModel::Location,
        KisAbstractResourceModel::ResourceType,
        KisAbstractResourceModel::Tags,
        KisAbstractResourceModel::MD5,
        KisAbstractResourceModel::LargeThumbnail,
        KisAbstractResourceModel::Dirty,
        KisAbstractResourceModel::MetaData,
        KisAbstractResourceModel::ResourceActive,
        KisAbstractResourceModel::StorageActive,
        KisAbstractResourceModel::BrokenStatus,
        KisAbstractResourceModel::BrokenStatusMessage,
    }};

    for (std::size_t index = 0; index < columns.size(); ++index) {
        QCOMPARE(int(columns[index]), int(index));
    }
}

void KisResourceModelEnumContractTest::resourceFilterValuesRemainStable()
{
    const std::array<KisAbstractResourceFilterInterface::ResourceFilter, 3> filters {{
        KisAbstractResourceFilterInterface::ShowInactiveResources,
        KisAbstractResourceFilterInterface::ShowActiveResources,
        KisAbstractResourceFilterInterface::ShowAllResources,
    }};

    for (std::size_t index = 0; index < filters.size(); ++index) {
        QCOMPARE(int(filters[index]), int(index));
    }
}

void KisResourceModelEnumContractTest::storageFilterValuesRemainStable()
{
    const std::array<KisAbstractResourceFilterInterface::StorageFilter, 3> filters {{
        KisAbstractResourceFilterInterface::ShowInactiveStorages,
        KisAbstractResourceFilterInterface::ShowActiveStorages,
        KisAbstractResourceFilterInterface::ShowAllStorages,
    }};

    for (std::size_t index = 0; index < filters.size(); ++index) {
        QCOMPARE(int(filters[index]), int(index));
    }
}

void KisResourceModelEnumContractTest::storageTypeLabelSchemaRemainsStable()
{
    using Storage = KisResourceStorage;
    using StorageType = Storage::StorageType;


    const std::array<std::pair<StorageType, const char *>, 7> labels{{
        {StorageType::Unknown, "Unknown"},
        {StorageType::Folder, "Folder"},
        {StorageType::Bundle, "Bundle"},
        {StorageType::AdobeBrushLibrary, "Adobe Brush Library"},
        {StorageType::AdobeStyleLibrary, "Adobe Style Library"},
        {StorageType::Memory, "Memory"},
        {StorageType::FontStorage, "Font Storage"},
    }};

    for (const auto &[storageType, expectedLabel] : labels) {
        QCOMPARE(Storage::storageTypeToUntranslatedString(storageType), QString::fromLatin1(expectedLabel));
    }
}

void KisResourceModelEnumContractTest::storageVersionIteratorValueSchemaRemainsStable()
{
    using Iterator = KisVersionedStorageIterator;

    struct ResourceImplAccess : Iterator {
        using Iterator::resourceImpl;
    };

}

void KisResourceModelEnumContractTest::storagePluginFactorySchemaRemainsStable()
{
    struct PluginType;

}

QTEST_GUILESS_MAIN(KisResourceModelEnumContractTest)

#include "KisResourceModelEnumContractTest.moc"
