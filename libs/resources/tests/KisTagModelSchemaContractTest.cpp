/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTagModel.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

class KisTagModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void modelTypesDescribeOwnershipAndHierarchy();
    void columnsRemainStable();
    void pseudoRowsKeepStableIdsAndUrls();
    void tagFilterValuesRemainStable();
    void storageFilterValuesRemainStable();
    void abstractTagModelOperationSignaturesRemainStable();
    void allTagsModelViewSignaturesRemainStable();
    void allTagsModelTagOperationSignaturesRemainStable();
    void filteredTagModelTypeFilterAndLookupSignaturesRemainStable();
    void filteredTagModelMutationSignaturesRemainStable();
};

void KisTagModelSchemaContractTest::modelTypesDescribeOwnershipAndHierarchy()
{
    static_assert(std::is_abstract_v<KisAbstractTagModel>);
    static_assert(std::is_base_of_v<QAbstractTableModel, KisAllTagsModel>);
    static_assert(std::is_base_of_v<KisAbstractTagModel, KisAllTagsModel>);
    static_assert(std::is_base_of_v<QSortFilterProxyModel, KisTagModel>);
    static_assert(std::is_base_of_v<KisAbstractTagModel, KisTagModel>);
    static_assert(std::is_same_v<KisAllTagsModelSP, QSharedPointer<KisAllTagsModel>>);

    const KisAllTagsModelSP model;
    QVERIFY(model.isNull());
}

void KisTagModelSchemaContractTest::columnsRemainStable()
{
    const std::array<KisAllTagsModel::Columns, 7> columns {{
        KisAllTagsModel::Id,
        KisAllTagsModel::Url,
        KisAllTagsModel::Name,
        KisAllTagsModel::Comment,
        KisAllTagsModel::ResourceType,
        KisAllTagsModel::Active,
        KisAllTagsModel::KisTagRole,
    }};

    for (std::size_t index = 0; index < columns.size(); ++index) {
        QCOMPARE(int(columns[index]), int(index));
    }
}

void KisTagModelSchemaContractTest::pseudoRowsKeepStableIdsAndUrls()
{
    QCOMPARE(int(KisAllTagsModel::All), -2);
    QCOMPARE(int(KisAllTagsModel::AllUntagged), -1);
    QCOMPARE(KisAllTagsModel::urlAll(), QStringLiteral("All"));
    QCOMPARE(KisAllTagsModel::urlAllUntagged(), QStringLiteral("All untagged"));
    QVERIFY(KisAllTagsModel::urlAll() != KisAllTagsModel::urlAllUntagged());
}

void KisTagModelSchemaContractTest::tagFilterValuesRemainStable()
{
    const std::array<KisTagModel::TagFilter, 3> filters {{
        KisTagModel::ShowInactiveTags,
        KisTagModel::ShowActiveTags,
        KisTagModel::ShowAllTags,
    }};

    for (std::size_t index = 0; index < filters.size(); ++index) {
        QCOMPARE(int(filters[index]), int(index));
    }
}

void KisTagModelSchemaContractTest::storageFilterValuesRemainStable()
{
    const std::array<KisTagModel::StorageFilter, 3> filters {{
        KisTagModel::ShowInactiveStorages,
        KisTagModel::ShowActiveStorages,
        KisTagModel::ShowAllStorages,
    }};

    for (std::size_t index = 0; index < filters.size(); ++index) {
        QCOMPARE(int(filters[index]), int(index));
    }
}

void KisTagModelSchemaContractTest::abstractTagModelOperationSignaturesRemainStable()
{
    using IndexForTag = QModelIndex (KisAbstractTagModel::*)(KisTagSP) const;
    using TagForIndex = KisTagSP (KisAbstractTagModel::*)(QModelIndex) const;
    using TagForUrl = KisTagSP (KisAbstractTagModel::*)(const QString &) const;
    using AddTagByName = KisTagSP (KisAbstractTagModel::*)(const QString &, const bool, QVector<KoResourceSP>);
    using AddTag = bool (KisAbstractTagModel::*)(const KisTagSP, const bool, QVector<KoResourceSP>);
    using SetTagState = bool (KisAbstractTagModel::*)(const KisTagSP);
    using RenameTag = bool (KisAbstractTagModel::*)(const KisTagSP, const QString &, const bool);
    using ChangeTagActive = bool (KisAbstractTagModel::*)(const KisTagSP, bool);

    static_assert(std::is_destructible_v<KisAbstractTagModel>);
    static_assert(std::has_virtual_destructor_v<KisAbstractTagModel>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::indexForTag), IndexForTag>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::tagForIndex), TagForIndex>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::tagForUrl), TagForUrl>);
    static_assert(std::is_same_v<decltype(static_cast<AddTagByName>(&KisAbstractTagModel::addTag)), AddTagByName>);
    static_assert(std::is_same_v<decltype(static_cast<AddTag>(&KisAbstractTagModel::addTag)), AddTag>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::setTagActive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::setTagInactive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::renameTag), RenameTag>);
    static_assert(std::is_same_v<decltype(&KisAbstractTagModel::changeTagActive), ChangeTagActive>);
    static_assert(std::is_same_v<decltype(std::declval<const KisAbstractTagModel &>().tagForIndex()), KisTagSP>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisAbstractTagModel &>().addTag(std::declval<KisTagSP>(), false)), bool>);
}

void KisTagModelSchemaContractTest::allTagsModelViewSignaturesRemainStable()
{
    using RowCount = int (KisAllTagsModel::*)(const QModelIndex &) const;
    using ColumnCount = int (KisAllTagsModel::*)(const QModelIndex &) const;
    using Data = QVariant (KisAllTagsModel::*)(const QModelIndex &, int) const;
    using SetData = bool (KisAllTagsModel::*)(const QModelIndex &, const QVariant &, int);
    using Flags = Qt::ItemFlags (KisAllTagsModel::*)(const QModelIndex &) const;

    static_assert(std::is_destructible_v<KisAllTagsModel>);
    static_assert(std::has_virtual_destructor_v<KisAllTagsModel>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::rowCount), RowCount>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::columnCount), ColumnCount>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::data), Data>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::setData), SetData>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::flags), Flags>);
    static_assert(std::is_same_v<decltype(std::declval<const KisAllTagsModel &>().rowCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisAllTagsModel &>().columnCount()), int>);
}

void KisTagModelSchemaContractTest::allTagsModelTagOperationSignaturesRemainStable()
{
    using IndexForTag = QModelIndex (KisAllTagsModel::*)(KisTagSP) const;
    using TagForIndex = KisTagSP (KisAllTagsModel::*)(QModelIndex) const;
    using TagForUrl = KisTagSP (KisAllTagsModel::*)(const QString &) const;
    using AddTagByName = KisTagSP (KisAllTagsModel::*)(const QString &, const bool, QVector<KoResourceSP>);
    using AddTag = bool (KisAllTagsModel::*)(const KisTagSP, const bool, QVector<KoResourceSP>);
    using SetTagState = bool (KisAllTagsModel::*)(const KisTagSP);
    using RenameTag = bool (KisAllTagsModel::*)(const KisTagSP, const QString &, const bool);
    using ChangeTagActive = bool (KisAllTagsModel::*)(const KisTagSP, bool);

    static_assert(std::is_same_v<decltype(&KisAllTagsModel::indexForTag), IndexForTag>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::tagForIndex), TagForIndex>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::tagForUrl), TagForUrl>);
    static_assert(std::is_same_v<decltype(static_cast<AddTagByName>(&KisAllTagsModel::addTag)), AddTagByName>);
    static_assert(std::is_same_v<decltype(static_cast<AddTag>(&KisAllTagsModel::addTag)), AddTag>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::setTagActive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::setTagInactive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::renameTag), RenameTag>);
    static_assert(std::is_same_v<decltype(&KisAllTagsModel::changeTagActive), ChangeTagActive>);
    static_assert(std::is_same_v<decltype(std::declval<const KisAllTagsModel &>().tagForIndex()), KisTagSP>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisAllTagsModel &>().addTag(std::declval<KisTagSP>(), false)), bool>);
}

void KisTagModelSchemaContractTest::filteredTagModelTypeFilterAndLookupSignaturesRemainStable()
{
    using SetTagFilter = void (KisTagModel::*)(KisTagModel::TagFilter);
    using SetStorageFilter = void (KisTagModel::*)(KisTagModel::StorageFilter);
    using IndexForTag = QModelIndex (KisTagModel::*)(KisTagSP) const;
    using TagForIndex = KisTagSP (KisTagModel::*)(QModelIndex) const;
    using TagForUrl = KisTagSP (KisTagModel::*)(const QString &) const;

    static_assert(std::is_constructible_v<KisTagModel, const QString &>);
    static_assert(std::is_constructible_v<KisTagModel, const QString &, QObject *>);
    static_assert(std::is_destructible_v<KisTagModel>);
    static_assert(std::has_virtual_destructor_v<KisTagModel>);
    static_assert(std::is_same_v<decltype(&KisTagModel::setTagFilter), SetTagFilter>);
    static_assert(std::is_same_v<decltype(&KisTagModel::setStorageFilter), SetStorageFilter>);
    static_assert(std::is_same_v<decltype(&KisTagModel::indexForTag), IndexForTag>);
    static_assert(std::is_same_v<decltype(&KisTagModel::tagForIndex), TagForIndex>);
    static_assert(std::is_same_v<decltype(&KisTagModel::tagForUrl), TagForUrl>);
    static_assert(std::is_same_v<decltype(std::declval<const KisTagModel &>().tagForIndex()), KisTagSP>);
}

void KisTagModelSchemaContractTest::filteredTagModelMutationSignaturesRemainStable()
{
    using AddTagByName = KisTagSP (KisTagModel::*)(const QString &, const bool, QVector<KoResourceSP>);
    using AddTag = bool (KisTagModel::*)(const KisTagSP, const bool, QVector<KoResourceSP>);
    using SetTagState = bool (KisTagModel::*)(const KisTagSP);
    using RenameTag = bool (KisTagModel::*)(const KisTagSP, const QString &, const bool);
    using ChangeTagActive = bool (KisTagModel::*)(const KisTagSP, bool);

    static_assert(std::is_same_v<decltype(static_cast<AddTagByName>(&KisTagModel::addTag)), AddTagByName>);
    static_assert(std::is_same_v<decltype(static_cast<AddTag>(&KisTagModel::addTag)), AddTag>);
    static_assert(std::is_same_v<decltype(&KisTagModel::setTagInactive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisTagModel::setTagActive), SetTagState>);
    static_assert(std::is_same_v<decltype(&KisTagModel::renameTag), RenameTag>);
    static_assert(std::is_same_v<decltype(&KisTagModel::changeTagActive), ChangeTagActive>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisTagModel &>().addTag(std::declval<KisTagSP>(), false)), bool>);
}

QTEST_GUILESS_MAIN(KisTagModelSchemaContractTest)

#include "KisTagModelSchemaContractTest.moc"
