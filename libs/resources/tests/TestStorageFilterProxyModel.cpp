/*
 * SPDX-FileCopyrightText: 2019 boud <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "TestStorageFilterProxyModel.h"

#include <simpletest.h>

#include <KisResourceModel.h>
#include <KisStorageFilterProxyModel.h>
#include <KisStorageModel.h>

#include <QStandardItemModel>

namespace
{
void setStorageRow(QStandardItemModel &model,
                   int row,
                   const QString &name,
                   const QString &location,
                   const QString &storageType,
                   bool active)
{
    const QModelIndex index = model.index(row, KisAbstractResourceModel::Name);
    model.setData(index, name, Qt::DisplayRole);
    model.setData(index, name, Qt::UserRole + KisAbstractResourceModel::Name);
    model.setData(index, location, Qt::UserRole + KisStorageModel::Location);
    model.setData(index, storageType, Qt::UserRole + KisStorageModel::StorageType);
    model.setData(index, active, Qt::UserRole + KisStorageModel::Active);
}

void configureStorageModel(QStandardItemModel &model)
{
    model.setRowCount(3);
    model.setColumnCount(KisAbstractResourceModel::Name + 1);
    setStorageRow(model,
                  0,
                  QStringLiteral("Brush Bundle"),
                  QStringLiteral("/brushes/default.bundle"),
                  QStringLiteral("Bundle"),
                  true);
    setStorageRow(model,
                  1,
                  QStringLiteral("Style Library"),
                  QStringLiteral("/styles/default.asl"),
                  QStringLiteral("Adobe Style Library"),
                  false);
    setStorageRow(model,
                  2,
                  QStringLiteral("Pattern Folder"),
                  QStringLiteral("/home/patterns"),
                  QStringLiteral("Folder"),
                  true);
}
}

void TestStorageFilterProxyModel::filtersStorageRowsForChooser()
{
    // Consumer: the resource chooser and bundle manager views.
    // Operation: Select storages by filename, supported storage type, or active state.
    // Observable result: The view contains only the matching storage rows, or every row when no active-state filter is selected.
    // Failure impact: A user cannot find a resource source or is offered a storage that cannot provide the selected resource type.
    QStandardItemModel model;
    configureStorageModel(model);

    KisStorageFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setFilter(KisStorageFilterProxyModel::ByFileName,
                    QStringLiteral("default.asl"));
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
             QStringLiteral("Style Library"));

    proxy.setFilter(KisStorageFilterProxyModel::ByStorageType,
                    QStringList{QStringLiteral("Bundle"), QStringLiteral("Folder")});
    QCOMPARE(proxy.rowCount(), 2);

    proxy.setFilter(KisStorageFilterProxyModel::ByActive, false);
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
             QStringLiteral("Style Library"));

    proxy.setFilter(KisStorageFilterProxyModel::ByActive, QVariant());
    QCOMPARE(proxy.rowCount(), 3);
}

void TestStorageFilterProxyModel::updatesVisibleRowsWhenFilterChanges()
{
    // Consumer: the resource chooser and bundle manager views.
    // Operation: Change the storage-type filter after the view has displayed a previous result.
    // Observable result: The visible row changes to the storage matching the new type.
    // Failure impact: A user selects a stale resource source or receives an incorrect missing-resource warning.
    QStandardItemModel model;
    configureStorageModel(model);
    KisStorageFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setFilter(KisStorageFilterProxyModel::ByStorageType,
                    QStringList{QStringLiteral("Bundle")});
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
             QStringLiteral("Brush Bundle"));

    proxy.setFilter(KisStorageFilterProxyModel::ByStorageType,
                    QStringList{QStringLiteral("Adobe Style Library")});

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
             QStringLiteral("Style Library"));
}

SIMPLE_TEST_MAIN(TestStorageFilterProxyModel)
