/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceModel.h>

#include <QTest>

#include <array>

class KisResourceModelEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void columnValuesRemainStable();
    void resourceFilterValuesRemainStable();
    void storageFilterValuesRemainStable();
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

QTEST_GUILESS_MAIN(KisResourceModelEnumContractTest)

#include "KisResourceModelEnumContractTest.moc"
