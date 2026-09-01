/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTagModel.h>

#include <QTest>

#include <array>
#include <type_traits>

class KisTagModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void modelTypesDescribeOwnershipAndHierarchy();
    void columnsRemainStable();
    void pseudoRowsKeepStableIdsAndUrls();
    void tagFilterValuesRemainStable();
    void storageFilterValuesRemainStable();
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

QTEST_GUILESS_MAIN(KisTagModelSchemaContractTest)

#include "KisTagModelSchemaContractTest.moc"
