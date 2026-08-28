/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisStorageFilterProxyModel.h>

#include <KisResourceModel.h>
#include <KisStorageFilterProxyModelSource_p.h>

#include <QPointer>
#include <QStandardItemModel>
#include <QTest>

namespace
{
QAbstractItemModel *capturedSourceModel = nullptr;
QPersistentModelIndex capturedSourceIndex;

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

namespace KisStorageFilterProxyModelSource
{
KisResourceStorageSP storageForIndex(QAbstractItemModel *model,
                                     const QModelIndex &index)
{
    capturedSourceModel = model;
    capturedSourceIndex = index;
    return KisResourceStorageSP();
}
}

class KisStorageFilterProxyModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterTypeValuesAreStable();
    void constructorUsesParentOwnership();
    void initialFiltersUseConfiguredCriterion();
    void changingFilterDoesNotInvalidateEvaluatedRows();
    void storageLookupMapsProxyIndexToSource();
};

void KisStorageFilterProxyModelContractTest::filterTypeValuesAreStable()
{
    QCOMPARE(int(KisStorageFilterProxyModel::ByFileName), 0);
    QCOMPARE(int(KisStorageFilterProxyModel::ByStorageType), 1);
    QCOMPARE(int(KisStorageFilterProxyModel::ByActive), 2);
}

void KisStorageFilterProxyModelContractTest::constructorUsesParentOwnership()
{
    QObject parent;
    QPointer<KisStorageFilterProxyModel> proxy = new KisStorageFilterProxyModel(&parent);

    QCOMPARE(proxy->parent(), &parent);
    delete proxy.data();
    QVERIFY(proxy.isNull());
}

void KisStorageFilterProxyModelContractTest::initialFiltersUseConfiguredCriterion()
{
    QStandardItemModel model;
    configureStorageModel(model);

    {
        KisStorageFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilter(KisStorageFilterProxyModel::ByFileName,
                        QStringLiteral("default.asl"));
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
                 QStringLiteral("Style Library"));
    }

    {
        KisStorageFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilter(KisStorageFilterProxyModel::ByStorageType,
                        QStringList{QStringLiteral("Bundle"), QStringLiteral("Folder")});
        QCOMPARE(proxy.rowCount(), 2);
    }

    {
        KisStorageFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilter(KisStorageFilterProxyModel::ByActive, false);
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, KisAbstractResourceModel::Name).data(Qt::DisplayRole).toString(),
                 QStringLiteral("Style Library"));
    }

    {
        KisStorageFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilter(KisStorageFilterProxyModel::ByActive, QVariant());
        QCOMPARE(proxy.rowCount(), 3);
    }
}

void KisStorageFilterProxyModelContractTest::changingFilterDoesNotInvalidateEvaluatedRows()
{
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
             QStringLiteral("Brush Bundle"));
}

void KisStorageFilterProxyModelContractTest::storageLookupMapsProxyIndexToSource()
{
    QStandardItemModel model;
    configureStorageModel(model);
    KisStorageFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    const QModelIndex proxyIndex = proxy.index(1, KisAbstractResourceModel::Name);
    capturedSourceModel = nullptr;
    capturedSourceIndex = QModelIndex();

    const KisResourceStorageSP storage = proxy.storageForIndex(proxyIndex);

    QVERIFY(storage.isNull());
    QCOMPARE(capturedSourceModel, &model);
    QCOMPARE(capturedSourceIndex, model.index(1, KisAbstractResourceModel::Name));
}

QTEST_GUILESS_MAIN(KisStorageFilterProxyModelContractTest)

#include "KisStorageFilterProxyModelContractTest.moc"
