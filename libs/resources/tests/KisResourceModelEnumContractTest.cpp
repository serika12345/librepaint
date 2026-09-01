/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <KisStorageModel.h>
#include <KisTagResourceModel.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

class KisResourceModelEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnValuesRemainStable();
    void resourceFilterValuesRemainStable();
    void storageFilterValuesRemainStable();
    void allResourcesModelTableSchemaRemainsStable();
    void allResourcesModelLookupSchemaRemainsStable();
    void allResourcesModelTransferSchemaRemainsStable();
    void allResourcesModelPersistenceSchemaRemainsStable();
    void allResourcesModelStateSchemaRemainsStable();
    void resourceModelTypeAndFilterSchemaRemainsStable();
    void resourceModelLookupSchemaRemainsStable();
    void resourceModelTransferSchemaRemainsStable();
    void resourceModelPersistenceSchemaRemainsStable();
    void resourceModelStateSchemaRemainsStable();
    void tagResourceModelTypeAndViewSchemaRemainsStable();
    void tagResourceModelFilterSchemaRemainsStable();
    void tagResourceModelRelationsSchemaRemainsStable();
    void tagResourceModelTransferSchemaRemainsStable();
    void tagResourceModelMutationSchemaRemainsStable();
    void storageTypeIdentitySchemaRemainsStable();
    void storageLifetimeSchemaRemainsStable();
    void storageTypeLabelSchemaRemainsStable();
    void storageIdentityQuerySchemaRemainsStable();
    void storageStateQuerySchemaRemainsStable();
    void storageResourceItemSchemaRemainsStable();
    void storageResourceIteratorNavigationSchemaRemainsStable();
    void storageResourceIteratorValueSchemaRemainsStable();
    void storageTagIteratorSchemaRemainsStable();
    void storageLookupAndEnumerationSchemaRemainsStable();
    void storageModelTypeAndEnumSchemaRemainsStable();
    void storageModelTableSchemaRemainsStable();
    void storageModelLookupSchemaRemainsStable();
    void storageModelImportSchemaRemainsStable();
    void storageModelNotificationSchemaRemainsStable();
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

void KisResourceModelEnumContractTest::allResourcesModelTableSchemaRemainsStable()
{
    using Model = KisAllResourcesModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::has_virtual_destructor_v<Model>);

    static_assert(std::is_same_v<decltype(&Model::rowCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::columnCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::data), QVariant (Model::*)(const QModelIndex &, int) const>);
    static_assert(std::is_same_v<decltype(&Model::headerData), QVariant (Model::*)(int, Qt::Orientation, int) const>);
    static_assert(
        std::is_same_v<decltype(&Model::setData), bool (Model::*)(const QModelIndex &, const QVariant &, int)>);
    static_assert(std::is_same_v<decltype(&Model::flags), Qt::ItemFlags (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::roleNames), QHash<int, QByteArray> (Model::*)() const>);

    static_assert(std::is_same_v<decltype(std::declval<const Model &>().rowCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().columnCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().headerData(0, Qt::Horizontal)), QVariant>);
}

void KisResourceModelEnumContractTest::allResourcesModelLookupSchemaRemainsStable()
{
    using Model = KisAllResourcesModel;

    static_assert(std::is_same_v<decltype(&Model::resourceForIndex), KoResourceSP (Model::*)(QModelIndex) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResource), QModelIndex (Model::*)(KoResourceSP) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResourceId), QModelIndex (Model::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Model::resourceForId), KoResourceSP (Model::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Model::resourceExists),
                                 bool (Model::*)(const QString &, const QString &, const QString &)>);
    static_assert(
        std::is_same_v<decltype(&Model::resourcesForFilename), QVector<KoResourceSP> (Model::*)(QString) const>);
    static_assert(
        std::is_same_v<decltype(&Model::resourcesForName), QVector<KoResourceSP> (Model::*)(const QString &) const>);
    static_assert(
        std::is_same_v<decltype(&Model::resourcesForMD5), QVector<KoResourceSP> (Model::*)(const QString &) const>);
    static_assert(std::is_same_v<decltype(&Model::tagsForResource), QVector<KisTagSP> (Model::*)(int) const>);

    static_assert(std::is_same_v<decltype(std::declval<const Model &>().resourceForIndex()), KoResourceSP>);
}

void KisResourceModelEnumContractTest::allResourcesModelTransferSchemaRemainsStable()
{
    using Model = KisAllResourcesModel;

    static_assert(std::is_same_v<decltype(&Model::importResourceFile),
                                 KoResourceSP (Model::*)(const QString &, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importResource),
                                 KoResourceSP (Model::*)(const QString &, QIODevice *, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importWillOverwriteResource),
                                 bool (Model::*)(const QString &, const QString &) const>);
    static_assert(std::is_same_v<decltype(&Model::exportResource), bool (Model::*)(KoResourceSP, QIODevice *)>);

    static_assert(
        std::is_same_v<decltype(std::declval<Model &>().importResourceFile(std::declval<const QString &>(), false)),
                       KoResourceSP>);
    static_assert(std::is_same_v<decltype(std::declval<Model &>().importResource(std::declval<const QString &>(),
                                                                                 static_cast<QIODevice *>(nullptr),
                                                                                 false)),
                                 KoResourceSP>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().importWillOverwriteResource(
                                     std::declval<const QString &>())),
                                 bool>);
}

void KisResourceModelEnumContractTest::allResourcesModelPersistenceSchemaRemainsStable()
{
    using Model = KisAllResourcesModel;

    static_assert(std::is_same_v<decltype(&Model::addResource), bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::addResourceDeduplicateFileName),
                                 bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::updateResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::reloadResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::renameResource), bool (Model::*)(KoResourceSP, const QString &)>);

    static_assert(std::is_same_v<decltype(std::declval<Model &>().addResource(std::declval<KoResourceSP>())), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<Model &>().addResourceDeduplicateFileName(std::declval<KoResourceSP>())),
                       bool>);
}

void KisResourceModelEnumContractTest::allResourcesModelStateSchemaRemainsStable()
{
    using Model = KisAllResourcesModel;

    static_assert(std::is_same_v<decltype(&Model::setResourceActive), bool (Model::*)(const QModelIndex &, bool)>);
    static_assert(
        std::is_same_v<decltype(&Model::setResourceMetaData), bool (Model::*)(KoResourceSP, QMap<QString, QVariant>)>);
}

void KisResourceModelEnumContractTest::resourceModelTypeAndFilterSchemaRemainsStable()
{
    using Model = KisResourceModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::has_virtual_destructor_v<Model>);
    static_assert(std::is_constructible_v<Model, const QString &>);
    static_assert(std::is_constructible_v<Model, const QString &, QObject *>);

    static_assert(std::is_same_v<decltype(&Model::setResourceFilter), void (Model::*)(Model::ResourceFilter)>);
    static_assert(std::is_same_v<decltype(&Model::setStorageFilter), void (Model::*)(Model::StorageFilter)>);
    static_assert(std::is_same_v<decltype(&Model::showOnlyUntaggedResources), void (Model::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Model::roleNames), QHash<int, QByteArray> (Model::*)() const>);
}

void KisResourceModelEnumContractTest::resourceModelLookupSchemaRemainsStable()
{
    using Model = KisResourceModel;

    static_assert(std::is_same_v<decltype(&Model::resourceForIndex), KoResourceSP (Model::*)(QModelIndex) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResource), QModelIndex (Model::*)(KoResourceSP) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResourceId), QModelIndex (Model::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Model::resourceForId), KoResourceSP (Model::*)(int) const>);
    static_assert(
        std::is_same_v<decltype(&Model::resourcesForFilename), QVector<KoResourceSP> (Model::*)(QString) const>);
    static_assert(std::is_same_v<decltype(&Model::resourcesForName), QVector<KoResourceSP> (Model::*)(QString) const>);
    static_assert(std::is_same_v<decltype(&Model::resourcesForMD5), QVector<KoResourceSP> (Model::*)(QString) const>);
    static_assert(std::is_same_v<decltype(&Model::tagsForResource), QVector<KisTagSP> (Model::*)(int) const>);

    static_assert(std::is_same_v<decltype(std::declval<const Model &>().resourceForIndex()), KoResourceSP>);
}

void KisResourceModelEnumContractTest::resourceModelTransferSchemaRemainsStable()
{
    using Model = KisResourceModel;

    static_assert(std::is_same_v<decltype(&Model::importResourceFile),
                                 KoResourceSP (Model::*)(const QString &, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importResource),
                                 KoResourceSP (Model::*)(const QString &, QIODevice *, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importWillOverwriteResource),
                                 bool (Model::*)(const QString &, const QString &) const>);
    static_assert(std::is_same_v<decltype(&Model::exportResource), bool (Model::*)(KoResourceSP, QIODevice *)>);

    static_assert(
        std::is_same_v<decltype(std::declval<Model &>().importResourceFile(std::declval<const QString &>(), false)),
                       KoResourceSP>);
    static_assert(std::is_same_v<decltype(std::declval<Model &>().importResource(std::declval<const QString &>(),
                                                                                 static_cast<QIODevice *>(nullptr),
                                                                                 false)),
                                 KoResourceSP>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().importWillOverwriteResource(
                                     std::declval<const QString &>())),
                                 bool>);
}

void KisResourceModelEnumContractTest::resourceModelPersistenceSchemaRemainsStable()
{
    using Model = KisResourceModel;

    static_assert(std::is_same_v<decltype(&Model::addResource), bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::addResourceDeduplicateFileName),
                                 bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::updateResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::reloadResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::renameResource), bool (Model::*)(KoResourceSP, const QString &)>);

    static_assert(std::is_same_v<decltype(std::declval<Model &>().addResource(std::declval<KoResourceSP>())), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<Model &>().addResourceDeduplicateFileName(std::declval<KoResourceSP>())),
                       bool>);
}

void KisResourceModelEnumContractTest::resourceModelStateSchemaRemainsStable()
{
    using Model = KisResourceModel;

    static_assert(std::is_same_v<decltype(&Model::setResourceActive), bool (Model::*)(const QModelIndex &, bool)>);
    static_assert(
        std::is_same_v<decltype(&Model::setResourceMetaData), bool (Model::*)(KoResourceSP, QMap<QString, QVariant>)>);
}

void KisResourceModelEnumContractTest::tagResourceModelTypeAndViewSchemaRemainsStable()
{
    using Model = KisTagResourceModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::has_virtual_destructor_v<Model>);
    static_assert(std::is_constructible_v<Model, const QString &>);
    static_assert(std::is_constructible_v<Model, const QString &, QObject *>);

    static_assert(int(Model::ShowInactiveTags) == 0);
    static_assert(int(Model::ShowActiveTags) == 1);
    static_assert(int(Model::ShowAllTags) == 2);

    static_assert(std::is_same_v<decltype(&Model::headerData), QVariant (Model::*)(int, Qt::Orientation, int) const>);
    static_assert(std::is_same_v<decltype(&Model::roleNames), QHash<int, QByteArray> (Model::*)() const>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().headerData(0, Qt::Horizontal)), QVariant>);
}

void KisResourceModelEnumContractTest::tagResourceModelFilterSchemaRemainsStable()
{
    using Model = KisTagResourceModel;

    static_assert(std::is_same_v<decltype(&Model::setTagFilter), void (Model::*)(Model::TagFilter)>);
    static_assert(std::is_same_v<decltype(&Model::setResourceFilter), void (Model::*)(Model::ResourceFilter)>);
    static_assert(std::is_same_v<decltype(&Model::setStorageFilter), void (Model::*)(Model::StorageFilter)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Model::*)(QVector<int>)>(&Model::setTagsFilter)),
                                 void (Model::*)(QVector<int>)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Model::*)(QVector<KisTagSP>)>(&Model::setTagsFilter)),
                                 void (Model::*)(QVector<KisTagSP>)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Model::*)(QVector<int>)>(&Model::setResourcesFilter)),
                                 void (Model::*)(QVector<int>)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Model::*)(QVector<KoResourceSP>)>(&Model::setResourcesFilter)),
                       void (Model::*)(QVector<KoResourceSP>)>);
}

void KisResourceModelEnumContractTest::tagResourceModelRelationsSchemaRemainsStable()
{
    using Model = KisTagResourceModel;

    static_assert(std::is_same_v<decltype(&Model::tagResources), bool (Model::*)(KisTagSP, const QVector<int> &)>);
    static_assert(std::is_same_v<decltype(&Model::untagResources), bool (Model::*)(KisTagSP, const QVector<int> &)>);
    static_assert(std::is_same_v<decltype(&Model::isResourceTagged), int (Model::*)(KisTagSP, int)>);
    static_assert(std::is_same_v<decltype(&Model::resourceForIndex), KoResourceSP (Model::*)(QModelIndex) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResource), QModelIndex (Model::*)(KoResourceSP) const>);
    static_assert(std::is_same_v<decltype(&Model::indexForResourceId), QModelIndex (Model::*)(int) const>);
}

void KisResourceModelEnumContractTest::tagResourceModelTransferSchemaRemainsStable()
{
    using Model = KisTagResourceModel;

    static_assert(std::is_same_v<decltype(&Model::importResourceFile),
                                 KoResourceSP (Model::*)(const QString &, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importResource),
                                 KoResourceSP (Model::*)(const QString &, QIODevice *, bool, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::importWillOverwriteResource),
                                 bool (Model::*)(const QString &, const QString &) const>);
    static_assert(std::is_same_v<decltype(&Model::exportResource), bool (Model::*)(KoResourceSP, QIODevice *)>);

    static_assert(
        std::is_same_v<decltype(std::declval<Model &>().importResourceFile(std::declval<const QString &>(), false)),
                       KoResourceSP>);
    static_assert(std::is_same_v<decltype(std::declval<Model &>().importResource(std::declval<const QString &>(),
                                                                                 static_cast<QIODevice *>(nullptr),
                                                                                 false)),
                                 KoResourceSP>);
}

void KisResourceModelEnumContractTest::tagResourceModelMutationSchemaRemainsStable()
{
    using Model = KisTagResourceModel;

    static_assert(std::is_same_v<decltype(&Model::addResource), bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::addResourceDeduplicateFileName),
                                 bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::updateResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::reloadResource), bool (Model::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Model::renameResource), bool (Model::*)(KoResourceSP, const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::setResourceActive), bool (Model::*)(const QModelIndex &, bool)>);
    static_assert(
        std::is_same_v<decltype(&Model::setResourceMetaData), bool (Model::*)(KoResourceSP, QMap<QString, QVariant>)>);
}

void KisResourceModelEnumContractTest::storageTypeIdentitySchemaRemainsStable()
{
    using Storage = KisResourceStorage;
    using StorageType = Storage::StorageType;

    static_assert(std::is_same_v<KisResourceStorageSP, QSharedPointer<Storage>>);
    static_assert(std::is_class_v<Storage>);
    static_assert(std::is_enum_v<StorageType>);
    static_assert(std::is_same_v<std::underlying_type_t<StorageType>, int>);

    static_assert(int(StorageType::Unknown) == 1);
    static_assert(int(StorageType::Folder) == 2);
    static_assert(int(StorageType::Bundle) == 3);
    static_assert(int(StorageType::AdobeBrushLibrary) == 4);
    static_assert(int(StorageType::AdobeStyleLibrary) == 5);
    static_assert(int(StorageType::Memory) == 6);
    static_assert(int(StorageType::FontStorage) == 7);
}

void KisResourceModelEnumContractTest::storageLifetimeSchemaRemainsStable()
{
    using Storage = KisResourceStorage;

    static_assert(std::is_constructible_v<Storage, const QString &, Storage::StorageType>);
    static_assert(std::is_constructible_v<Storage, const QString &>);
    static_assert(std::is_copy_constructible_v<Storage>);
    static_assert(std::is_destructible_v<Storage>);
    static_assert(std::is_copy_assignable_v<Storage>);
    static_assert(std::is_same_v<decltype(&Storage::operator=), Storage &(Storage::*)(const Storage &)>);
    static_assert(std::is_same_v<decltype(&Storage::clone), KisResourceStorageSP (Storage::*)() const>);
}

void KisResourceModelEnumContractTest::storageTypeLabelSchemaRemainsStable()
{
    using Storage = KisResourceStorage;
    using StorageType = Storage::StorageType;
    using TypeLabelFunction = QString (*)(StorageType);

    static_assert(std::is_same_v<decltype(&Storage::storageTypeToString), TypeLabelFunction>);
    static_assert(std::is_same_v<decltype(&Storage::storageTypeToUntranslatedString), TypeLabelFunction>);

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

void KisResourceModelEnumContractTest::storageIdentityQuerySchemaRemainsStable()
{
    using Storage = KisResourceStorage;

    static_assert(std::is_same_v<decltype(&Storage::name), QString (Storage::*)() const>);
    static_assert(std::is_same_v<decltype(&Storage::location), QString (Storage::*)() const>);
    static_assert(std::is_same_v<decltype(&Storage::type), Storage::StorageType (Storage::*)() const>);
}

void KisResourceModelEnumContractTest::storageStateQuerySchemaRemainsStable()
{
    using Storage = KisResourceStorage;

    static_assert(std::is_same_v<decltype(&Storage::valid), bool (Storage::*)() const>);
    static_assert(std::is_same_v<decltype(&Storage::thumbnail), QImage (Storage::*)() const>);
    static_assert(std::is_same_v<decltype(&Storage::timestamp), QDateTime (Storage::*)() const>);
    static_assert(std::is_same_v<decltype(&Storage::timeStampForResource),
                                 QDateTime (Storage::*)(const QString &, const QString &) const>);
}

void KisResourceModelEnumContractTest::storageResourceItemSchemaRemainsStable()
{
    using Item = KisResourceStorage::ResourceItem;

    static_assert(std::is_class_v<Item>);
    static_assert(std::is_destructible_v<Item>);
    static_assert(std::has_virtual_destructor_v<Item>);
    static_assert(std::is_same_v<decltype(&Item::url), QString Item::*>);
    static_assert(std::is_same_v<decltype(&Item::folder), QString Item::*>);
    static_assert(std::is_same_v<decltype(&Item::resourceType), QString Item::*>);
    static_assert(std::is_same_v<decltype(&Item::lastModified), QDateTime Item::*>);
}

void KisResourceModelEnumContractTest::storageResourceIteratorNavigationSchemaRemainsStable()
{
    using Iterator = KisResourceStorage::ResourceIterator;

    static_assert(std::is_class_v<Iterator>);
    static_assert(std::is_abstract_v<Iterator>);
    static_assert(std::is_destructible_v<Iterator>);
    static_assert(std::has_virtual_destructor_v<Iterator>);
    static_assert(std::is_same_v<decltype(&Iterator::hasNext), bool (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::next), void (Iterator::*)()>);
    static_assert(std::is_same_v<decltype(&Iterator::url), QString (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::type), QString (Iterator::*)() const>);
}

void KisResourceModelEnumContractTest::storageResourceIteratorValueSchemaRemainsStable()
{
    using Iterator = KisResourceStorage::ResourceIterator;

    static_assert(std::is_same_v<decltype(&Iterator::lastModified), QDateTime (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::guessedVersion), int (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::versions), QSharedPointer<Iterator> (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::resource), KoResourceSP (Iterator::*)() const>);
}

void KisResourceModelEnumContractTest::storageTagIteratorSchemaRemainsStable()
{
    using Iterator = KisResourceStorage::TagIterator;

    static_assert(std::is_class_v<Iterator>);
    static_assert(std::is_abstract_v<Iterator>);
    static_assert(std::is_destructible_v<Iterator>);
    static_assert(std::has_virtual_destructor_v<Iterator>);
    static_assert(std::is_same_v<decltype(&Iterator::hasNext), bool (Iterator::*)() const>);
    static_assert(std::is_same_v<decltype(&Iterator::next), void (Iterator::*)()>);
    static_assert(std::is_same_v<decltype(&Iterator::tag), KisTagSP (Iterator::*)() const>);
}

void KisResourceModelEnumContractTest::storageLookupAndEnumerationSchemaRemainsStable()
{
    using Storage = KisResourceStorage;

    static_assert(
        std::is_same_v<decltype(&Storage::resourceItem), Storage::ResourceItem (Storage::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Storage::resource), KoResourceSP (Storage::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Storage::resourceMd5), QString (Storage::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Storage::resourceFilePath), QString (Storage::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Storage::resources),
                                 QSharedPointer<Storage::ResourceIterator> (Storage::*)(const QString &) const>);
    static_assert(std::is_same_v<decltype(&Storage::tags),
                                 QSharedPointer<Storage::TagIterator> (Storage::*)(const QString &) const>);
}

void KisResourceModelEnumContractTest::storageModelTypeAndEnumSchemaRemainsStable()
{
    using Model = KisStorageModel;
    using Columns = Model::Columns;
    using ImportOption = Model::StorageImportOption;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<QAbstractTableModel, Model>);
    static_assert(std::is_enum_v<Columns>);
    static_assert(int(Model::Id) == 0);
    static_assert(int(Model::StorageType) == 1);
    static_assert(int(Model::Location) == 2);
    static_assert(int(Model::TimeStamp) == 3);
    static_assert(int(Model::PreInstalled) == 4);
    static_assert(int(Model::Active) == 5);
    static_assert(int(Model::Thumbnail) == 6);
    static_assert(int(Model::DisplayName) == 7);
    static_assert(int(Model::MetaData) == 8);

    static_assert(std::is_enum_v<ImportOption>);
    static_assert(int(Model::None) == 0);
    static_assert(int(Model::Overwrite) == 1);
    static_assert(int(Model::Rename) == 2);
}

void KisResourceModelEnumContractTest::storageModelTableSchemaRemainsStable()
{
    using Model = KisStorageModel;

    static_assert(std::is_constructible_v<Model>);
    static_assert(std::is_constructible_v<Model, QObject *>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::has_virtual_destructor_v<Model>);
    static_assert(std::is_same_v<decltype(&Model::rowCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::columnCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::data), QVariant (Model::*)(const QModelIndex &, int) const>);
    static_assert(
        std::is_same_v<decltype(&Model::setData), bool (Model::*)(const QModelIndex &, const QVariant &, int)>);
    static_assert(std::is_same_v<decltype(&Model::flags), Qt::ItemFlags (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::headerData), QVariant (Model::*)(int, Qt::Orientation, int) const>);

    static_assert(std::is_same_v<decltype(std::declval<const Model &>().rowCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().columnCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().headerData(0, Qt::Horizontal)), QVariant>);
}

void KisResourceModelEnumContractTest::storageModelLookupSchemaRemainsStable()
{
    using Model = KisStorageModel;

    static_assert(std::is_same_v<decltype(&Model::instance), Model *(*)()>);
    static_assert(
        std::is_same_v<decltype(&Model::storageForIndex), KisResourceStorageSP (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::storageForId), KisResourceStorageSP (Model::*)(int) const>);
}

void KisResourceModelEnumContractTest::storageModelImportSchemaRemainsStable()
{
    using Model = KisStorageModel;
    using ImportOption = Model::StorageImportOption;

    static_assert(
        std::is_same_v<decltype(&Model::importStorage), bool (Model::*)(const QString &, ImportOption) const>);
    static_assert(std::is_same_v<decltype(&Model::importStorageData),
                                 bool (Model::*)(const QString &, ImportOption, const QByteArray &) const>);
    static_assert(std::is_same_v<decltype(&Model::canImportStorage), bool (Model::*)(const QString &) const>);
}

void KisResourceModelEnumContractTest::storageModelNotificationSchemaRemainsStable()
{
    using Model = KisStorageModel;

    static_assert(std::is_same_v<decltype(&Model::storageEnabled), void (Model::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::storageDisabled), void (Model::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Model::storageResynchronized), void (Model::*)(const QString &, bool)>);
    static_assert(std::is_same_v<decltype(&Model::storagesBulkSynchronizationFinished), void (Model::*)()>);
}

QTEST_GUILESS_MAIN(KisResourceModelEnumContractTest)

#include "KisResourceModelEnumContractTest.moc"
