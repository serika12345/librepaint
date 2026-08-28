/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QAbstractItemView>
#include <QApplication>
#include <QMessageBox>

#include <QListView>

#include "KisResourceTypes.h"
#include "KisResourceModel.h"
#include "KisStorageChooserWidget.h"
#include "KisStorageModel.h"
#include "KisStorageFilterProxyModel.h"

KisStorageChooserWidget::KisStorageChooserWidget(const QString &resourceType, QWidget *parent)
    : KisPopupButton(parent)
    , m_resourceType(resourceType)
{
    QListView *view = new QListView(this);

    KisStorageFilterProxyModel *proxyModel = new KisStorageFilterProxyModel(this);

    proxyModel->setSourceModel(KisStorageModel::instance());

    QStringList filter;
    filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::Bundle);
    if (m_resourceType == ResourceType::Brushes) {
        filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::AdobeBrushLibrary);
    }
    if (m_resourceType == ResourceType::LayerStyles) {
        filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::AdobeStyleLibrary);
    }

    proxyModel->setFilter(KisStorageFilterProxyModel::ByStorageType, filter);
    view->setModel(proxyModel);
    view->setIconSize(QSize(64, 64));
    view->setItemDelegate(new KisStorageChooserDelegate(this));
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(view, SIGNAL(clicked(QModelIndex)), this, SLOT(activated(QModelIndex)));
    this->setPopupWidget(view);
}

void KisStorageChooserWidget::activated(const QModelIndex &index)
{
    if (!index.isValid()) return;

    bool active = index.data(Qt::UserRole + KisStorageModel::Active).value<bool>();
    KisStorageModel::instance()->setData(index, !active, Qt::CheckStateRole);

    KisStorageFilterProxyModel proxy;
    proxy.setSourceModel(KisStorageModel::instance());

    QStringList filter;
    filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::Bundle);
    if (m_resourceType == ResourceType::Brushes) {
        filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::AdobeBrushLibrary);
    }
    if (m_resourceType == ResourceType::LayerStyles) {
        filter << KisResourceStorage::storageTypeToUntranslatedString(KisResourceStorage::StorageType::AdobeStyleLibrary);
    }

    proxy.setFilter(KisStorageFilterProxyModel::ByStorageType, filter);

    QString warning;
    if (!proxy.rowCount()) {
        warning = i18n("All bundles have been deactivated.");
    }

    KisResourceModel resourceModel(m_resourceType);
    resourceModel.setResourceFilter(KisResourceModel::ShowActiveResources);
    if (!resourceModel.rowCount()) {
        warning += i18n("\nThere are no resources of type %1 available. Please enable at least one bundle.", ResourceName::resourceTypeToName(m_resourceType));

    }

    if (!warning.isEmpty()) {
        QMessageBox::critical(qApp->activeWindow(), i18nc("@title:window", "LibrePaint"), warning);
    }

    repaint(); // this helps the chooser widget update the active state checkbox. The first checkbox doesn't repaint correctly without it

}

KisStorageChooserWidget::~KisStorageChooserWidget()
{

}
