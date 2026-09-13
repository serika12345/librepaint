/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceModel.h>
#include <KisResourceModelProvider.h>
#include <KisResourceTypeModel.h>
#include <KisTagModelProvider.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisAbstractResourceModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterTypeLifetimeAndConfigurationSignaturesRemainStable();
    void modelTypeLifetimeAndLookupSignaturesRemainStable();
    void resourceActivationAndUpdateSignaturesRemainStable();
    void resourceImportAndExportSignaturesRemainStable();
    void resourceAdditionNameAndMetadataSignaturesRemainStable();
    void resourceTypeModelTypeColumnsAndLifetimeSchemaRemainStable();
    void resourceTypeModelColumnOrdinalsRemainStable();
    void resourceTypeModelDataSignaturesRemainStable();
    void resourceModelProviderTypeConstructionAndLifetimeSchemaRemainStable();
    void resourceModelProviderAccessAndTestingSignaturesRemainStable();
    void tagModelProviderTypeConstructionLifetimeAndAccessSchemaRemainStable();
};

#define ASSERT_RESOURCE_MODEL_SIGNATURE(Type, Method, Signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Type::Method)), Signature>)

void KisAbstractResourceModelSchemaContractTest::filterTypeLifetimeAndConfigurationSignaturesRemainStable()
{
    using Filter = KisAbstractResourceFilterInterface;
    using ResourceFilterSetter = void (Filter::*)(Filter::ResourceFilter);
    using StorageFilterSetter = void (Filter::*)(Filter::StorageFilter);

    static_assert(std::is_class_v<Filter>);
    static_assert(std::has_virtual_destructor_v<Filter>);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Filter, setResourceFilter, ResourceFilterSetter);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Filter, setStorageFilter, StorageFilterSetter);
}

void KisAbstractResourceModelSchemaContractTest::modelTypeLifetimeAndLookupSignaturesRemainStable()
{
    using Model = KisAbstractResourceModel;
    using IndexForId = QModelIndex (Model::*)(int) const;
    using IndexForResource = QModelIndex (Model::*)(KoResourceSP) const;
    using ResourceForIndex = KoResourceSP (Model::*)(QModelIndex) const;

    static_assert(std::is_class_v<Model>);
    static_assert(std::has_virtual_destructor_v<Model>);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, resourceForIndex, ResourceForIndex);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, indexForResource, IndexForResource);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, indexForResourceId, IndexForId);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().resourceForIndex()), KoResourceSP>);
}

void KisAbstractResourceModelSchemaContractTest::resourceActivationAndUpdateSignaturesRemainStable()
{
    using Model = KisAbstractResourceModel;
    using ActiveSetter = bool (Model::*)(const QModelIndex &, bool);
    using InactiveSetter = bool (Model::*)(const QModelIndex &);
    using ResourceMutation = bool (Model::*)(KoResourceSP);

    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, setResourceActive, ActiveSetter);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, setResourceInactive, InactiveSetter);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, updateResource, ResourceMutation);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, reloadResource, ResourceMutation);
}

void KisAbstractResourceModelSchemaContractTest::resourceImportAndExportSignaturesRemainStable()
{
    using Model = KisAbstractResourceModel;
    using Export = bool (Model::*)(KoResourceSP, QIODevice *);
    using Import = KoResourceSP (Model::*)(const QString &, QIODevice *, bool, const QString &);
    using ImportFile = KoResourceSP (Model::*)(const QString &, bool, const QString &);
    using WillOverwrite = bool (Model::*)(const QString &, const QString &) const;

    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, importResourceFile, ImportFile);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, importResource, Import);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, importWillOverwriteResource, WillOverwrite);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, exportResource, Export);
}

void KisAbstractResourceModelSchemaContractTest::resourceAdditionNameAndMetadataSignaturesRemainStable()
{
    using Model = KisAbstractResourceModel;
    using Add = bool (Model::*)(KoResourceSP, const QString &);
    using Metadata = bool (Model::*)(KoResourceSP, QMap<QString, QVariant>);

    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, addResource, Add);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, addResourceDeduplicateFileName, Add);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, renameResource, Add);
    ASSERT_RESOURCE_MODEL_SIGNATURE(Model, setResourceMetaData, Metadata);
}

void KisAbstractResourceModelSchemaContractTest::resourceTypeModelTypeColumnsAndLifetimeSchemaRemainStable()
{
    using Model = KisResourceTypeModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<QAbstractTableModel, Model>);
    static_assert(std::is_enum_v<Model::Columns>);
    static_assert(std::is_default_constructible_v<Model>);
    static_assert(std::is_constructible_v<Model, QObject *>);
    static_assert(std::has_virtual_destructor_v<Model>);
}

void KisAbstractResourceModelSchemaContractTest::resourceTypeModelColumnOrdinalsRemainStable()
{
    using Columns = KisResourceTypeModel::Columns;

    static_assert(Columns::Id == 0);
    static_assert(Columns::ResourceType == 1);
    static_assert(Columns::Name == 2);
}

void KisAbstractResourceModelSchemaContractTest::resourceTypeModelDataSignaturesRemainStable()
{
    using Model = KisResourceTypeModel;

    static_assert(std::is_same_v<decltype(&Model::rowCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::columnCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::data), QVariant (Model::*)(const QModelIndex &, int) const>);
}

void KisAbstractResourceModelSchemaContractTest::resourceModelProviderTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Provider = KisResourceModelProvider;

    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_default_constructible_v<Provider>);
    static_assert(std::is_destructible_v<Provider>);
    static_assert(!std::is_copy_constructible_v<Provider>);
    static_assert(!std::is_copy_assignable_v<Provider>);
}

void KisAbstractResourceModelSchemaContractTest::resourceModelProviderAccessAndTestingSignaturesRemainStable()
{
    using Provider = KisResourceModelProvider;
    using ResourceModel = KisAllResourcesModel *(*)(const QString &);
    using TagModel = KisAllTagsModel *(*)(const QString &);
    using TagResourceModel = KisAllTagResourceModel *(*)(const QString &);
    using MetadataModel = KisResourceMetaDataModel *(*)();
    using TestingControl = void (*)();

    static_assert(std::is_same_v<decltype(&Provider::resourceModel), ResourceModel>);
    static_assert(std::is_same_v<decltype(&Provider::tagModel), TagModel>);
    static_assert(std::is_same_v<decltype(&Provider::tagResourceModel), TagResourceModel>);
    static_assert(std::is_same_v<decltype(&Provider::resourceMetadataModel), MetadataModel>);
    static_assert(std::is_same_v<decltype(&Provider::testingResetAllModels), TestingControl>);
    static_assert(std::is_same_v<decltype(&Provider::testingCloseAllQueries), TestingControl>);
}

void KisAbstractResourceModelSchemaContractTest::tagModelProviderTypeConstructionLifetimeAndAccessSchemaRemainStable()
{
    using Provider = KisTagModelProvider;
    using TagModel = KisTagModel *(*)(const QString &);
    using TagResourceModel = KisTagResourceModel *(*)(const QString &);

    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_base_of_v<QObject, Provider>);
    static_assert(std::is_default_constructible_v<Provider>);
    static_assert(std::has_virtual_destructor_v<Provider>);
    static_assert(std::is_same_v<decltype(&Provider::tagModel), TagModel>);
    static_assert(std::is_same_v<decltype(&Provider::tagResourceModel), TagResourceModel>);
}

#undef ASSERT_RESOURCE_MODEL_SIGNATURE

QTEST_APPLESS_MAIN(KisAbstractResourceModelSchemaContractTest)

#include "KisAbstractResourceModelSchemaContractTest.moc"
