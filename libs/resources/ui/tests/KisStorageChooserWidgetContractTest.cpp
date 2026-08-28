/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisStorageChooserWidget.h>
#include <KisStorageChooserWidgetModelSource_p.h>

#include <KisResourceModel.h>
#include <KisResourceTypes.h>
#include <KisStorageFilterProxyModel.h>
#include <KisStorageFilterProxyModelSource_p.h>
#include <KisStorageModel.h>

#include <QApplication>
#include <QIcon>
#include <QListView>
#include <QPointer>
#include <QStandardItemModel>
#include <QTest>

namespace
{
QStandardItemModel sourceModel;
QPointer<KisStorageChooserWidget> activatedWidget;
QString activatedResourceType;
QPersistentModelIndex activatedIndex;

void configureSourceModel()
{
    sourceModel.clear();
    sourceModel.setRowCount(4);
    sourceModel.setColumnCount(KisAbstractResourceModel::Name + 1);

    const QStringList storageTypes {
        QStringLiteral("Bundle"),
        QStringLiteral("Adobe Brush Library"),
        QStringLiteral("Adobe Style Library"),
        QStringLiteral("Folder")
    };
    for (int row = 0; row < storageTypes.size(); ++row) {
        const QModelIndex index = sourceModel.index(row, KisAbstractResourceModel::Name);
        sourceModel.setData(index,
                            storageTypes.at(row),
                            Qt::UserRole + KisStorageModel::StorageType);
    }
}

QListView *viewForModel(QAbstractItemModel *model)
{
    const auto widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
        auto *view = qobject_cast<QListView *>(widget);
        if (view && view->model() == model) {
            return view;
        }
    }
    return nullptr;
}
}

namespace KisStorageChooserWidgetModelSource
{
QAbstractItemModel *model()
{
    return &sourceModel;
}
}

void KisStorageChooserWidget::activated(const QModelIndex &index)
{
    activatedWidget = this;
    activatedResourceType = m_resourceType;
    activatedIndex = index;
}

namespace KisStorageFilterProxyModelSource
{
KisResourceStorageSP storageForIndex(QAbstractItemModel *, const QModelIndex &)
{
    return KisResourceStorageSP();
}
}

namespace KisIconUtils
{
QIcon loadIcon(const QString &)
{
    return QIcon();
}
}

class KisStorageChooserWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorBuildsSingleSelectionPopup();
    void resourceTypeSelectsCompatibleStorages();
    void destructorReleasesOwnedPresentation();
};

void KisStorageChooserWidgetContractTest::constructorBuildsSingleSelectionPopup()
{
    configureSourceModel();
    QWidget parent;
    KisStorageChooserWidget widget(ResourceType::Brushes, &parent);
    auto *proxy = widget.findChild<KisStorageFilterProxyModel *>();
    auto *delegate = widget.findChild<KisStorageChooserDelegate *>();

    QVERIFY(proxy);
    QVERIFY(delegate);
    QCOMPARE(widget.parentWidget(), &parent);
    QCOMPARE(proxy->parent(), &widget);
    QCOMPARE(delegate->parent(), &widget);

    QListView *view = viewForModel(proxy);
    QVERIFY(view);
    QCOMPARE(view->iconSize(), QSize(64, 64));
    QCOMPARE(view->selectionMode(), QAbstractItemView::SingleSelection);
    QCOMPARE(view->itemDelegate(), delegate);

    activatedWidget = nullptr;
    activatedResourceType.clear();
    activatedIndex = QModelIndex();
    const QModelIndex firstIndex = proxy->index(0, KisAbstractResourceModel::Name);
    Q_EMIT view->clicked(firstIndex);
    QCOMPARE(activatedWidget, &widget);
    QCOMPARE(activatedResourceType, ResourceType::Brushes);
    QCOMPARE(activatedIndex, firstIndex);
}

void KisStorageChooserWidgetContractTest::resourceTypeSelectsCompatibleStorages()
{
    configureSourceModel();

    KisStorageChooserWidget brushes(ResourceType::Brushes);
    auto *brushProxy = brushes.findChild<KisStorageFilterProxyModel *>();
    QVERIFY(brushProxy);
    QCOMPARE(brushProxy->rowCount(), 2);

    KisStorageChooserWidget layerStyles(ResourceType::LayerStyles);
    auto *styleProxy = layerStyles.findChild<KisStorageFilterProxyModel *>();
    QVERIFY(styleProxy);
    QCOMPARE(styleProxy->rowCount(), 2);

    KisStorageChooserWidget patterns(ResourceType::Patterns);
    auto *patternProxy = patterns.findChild<KisStorageFilterProxyModel *>();
    QVERIFY(patternProxy);
    QCOMPARE(patternProxy->rowCount(), 1);
}

void KisStorageChooserWidgetContractTest::destructorReleasesOwnedPresentation()
{
    configureSourceModel();
    QPointer<KisStorageChooserWidget> widget =
        new KisStorageChooserWidget(ResourceType::Patterns);
    QPointer<KisStorageFilterProxyModel> proxy =
        widget->findChild<KisStorageFilterProxyModel *>();
    QPointer<KisStorageChooserDelegate> delegate =
        widget->findChild<KisStorageChooserDelegate *>();
    QPointer<QListView> view = viewForModel(proxy);

    QVERIFY(proxy);
    QVERIFY(delegate);
    QVERIFY(view);
    delete widget.data();

    QVERIFY(widget.isNull());
    QVERIFY(proxy.isNull());
    QVERIFY(delegate.isNull());
    QVERIFY(view.isNull());
}

QTEST_MAIN(KisStorageChooserWidgetContractTest)

#include "KisStorageChooserWidgetContractTest.moc"
