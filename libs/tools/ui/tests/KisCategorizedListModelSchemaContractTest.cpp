/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_categorized_list_model.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

struct EntryConverter {
    QString operator()(const QString &entry) const
    {
        return entry;
    }
};

using Model = KisCategorizedListModel<QString, EntryConverter>;
using Mapper = KisCategoriesMapper<QString, EntryConverter>;
using SortedModel = KisSortedCategorizedListModel<Model>;

class CategorizedListModelBaseProbe : public __CategorizedListModelBase
{
public:
    using __CategorizedListModelBase::__CategorizedListModelBase;

    int rowCount(const QModelIndex &) const override
    {
        return 0;
    }

    QVariant data(const QModelIndex &, int) const override
    {
        return {};
    }
};

} // namespace

class KisCategorizedListModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void categorizedListModelIdentityAndAliasSchemaRemainsStable();
    void categorizedListModelAdditionalRoleValuesRemainStable();
    void categorizedListModelLifecycleSignaturesRemainStable();
    void categorizedListModelAccessSignaturesRemainStable();
    void sortedCategorizedListModelAccessSignaturesRemainStable();
};

void KisCategorizedListModelSchemaContractTest::categorizedListModelIdentityAndAliasSchemaRemainsStable()
{
    static_assert(std::is_class_v<__CategorizedListModelBase>);
    static_assert(std::is_class_v<Model>);
    static_assert(std::is_class_v<SortedModel>);
    static_assert(std::is_same_v<Model::Entry_Type, QString>);
    static_assert(std::is_same_v<Model::SpecificCategoriesMapper, Mapper>);
    static_assert(std::is_same_v<Model::DataItem, Mapper::DataItem>);
}

void KisCategorizedListModelSchemaContractTest::categorizedListModelAdditionalRoleValuesRemainStable()
{
    static_assert(std::is_enum_v<__CategorizedListModelBase::AdditionalRoles>);
    static_assert(__CategorizedListModelBase::IsHeaderRole == Qt::UserRole + 1);
    static_assert(__CategorizedListModelBase::ExpandCategoryRole == Qt::UserRole + 2);
    static_assert(__CategorizedListModelBase::SortRole == Qt::UserRole + 3);
    static_assert(__CategorizedListModelBase::isLockedRole == Qt::UserRole + 4);
    static_assert(__CategorizedListModelBase::isLockableRole == Qt::UserRole + 5);
    static_assert(__CategorizedListModelBase::isToggledRole == Qt::UserRole + 6);
}

void KisCategorizedListModelSchemaContractTest::categorizedListModelLifecycleSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<CategorizedListModelBaseProbe, QObject *>);
    static_assert(std::is_destructible_v<__CategorizedListModelBase>);
    static_assert(std::is_constructible_v<Model>);
    static_assert(std::is_constructible_v<Model, QObject *>);
}

void KisCategorizedListModelSchemaContractTest::categorizedListModelAccessSignaturesRemainStable()
{
    using MutableMapper = Mapper *(Model::*)();
    using ConstMapper = const Mapper *(Model::*)() const;
    using Data = QVariant (Model::*)(const QModelIndex &, int) const;
    using EntryAt = bool (Model::*)(QString &, QModelIndex) const;
    using Flags = Qt::ItemFlags (Model::*)(const QModelIndex &) const;
    using IndexOf = QModelIndex (Model::*)(const QString &) const;
    using RowCount = int (Model::*)(const QModelIndex &) const;
    using SetData = bool (Model::*)(const QModelIndex &, const QVariant &, int);

    static_assert(std::is_same_v<decltype(static_cast<MutableMapper>(&Model::categoriesMapper)), MutableMapper>);
    static_assert(std::is_same_v<decltype(static_cast<ConstMapper>(&Model::categoriesMapper)), ConstMapper>);
    static_assert(std::is_same_v<decltype(static_cast<Data>(&Model::data)), Data>);
    static_assert(std::is_same_v<decltype(static_cast<EntryAt>(&Model::entryAt)), EntryAt>);
    static_assert(std::is_same_v<decltype(static_cast<Flags>(&Model::flags)), Flags>);
    static_assert(std::is_same_v<decltype(static_cast<IndexOf>(&Model::indexOf)), IndexOf>);
    static_assert(std::is_same_v<decltype(static_cast<RowCount>(&Model::rowCount)), RowCount>);
    static_assert(std::is_same_v<decltype(static_cast<SetData>(&Model::setData)), SetData>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Model &>().data(std::declval<const QModelIndex &>())), QVariant>);
    static_assert(std::is_same_v<decltype(std::declval<Model &>().setData(std::declval<const QModelIndex &>(),
                                                                          std::declval<const QVariant &>())),
                                 bool>);
}

void KisCategorizedListModelSchemaContractTest::sortedCategorizedListModelAccessSignaturesRemainStable()
{
    using EntryAt = bool (SortedModel::*)(QString &, QModelIndex) const;
    using IndexOf = QModelIndex (SortedModel::*)(const QString &) const;

    static_assert(std::is_constructible_v<SortedModel, QObject *>);
    static_assert(std::is_same_v<decltype(static_cast<EntryAt>(&SortedModel::entryAt)), EntryAt>);
    static_assert(std::is_same_v<decltype(static_cast<IndexOf>(&SortedModel::indexOf)), IndexOf>);
}

QTEST_APPLESS_MAIN(KisCategorizedListModelSchemaContractTest)

#include "KisCategorizedListModelSchemaContractTest.moc"
