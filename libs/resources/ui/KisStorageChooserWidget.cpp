/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QAbstractItemView>
#include <QListView>

#include "KisResourceStorage.h"
#include "KisResourceTypes.h"
#include "KisStorageChooserWidget.h"
#include "KisStorageChooserWidgetModelSource_p.h"
#include "KisStorageFilterProxyModel.h"

KisStorageChooserWidget::KisStorageChooserWidget(const QString &resourceType, QWidget *parent)
    : KisPopupButton(parent)
    , m_resourceType(resourceType)
{
    QListView *view = new QListView(this);

    KisStorageFilterProxyModel *proxyModel = new KisStorageFilterProxyModel(this);

    proxyModel->setSourceModel(KisStorageChooserWidgetModelSource::model());

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
    connect(view, &QListView::clicked, this, &KisStorageChooserWidget::activated);
    this->setPopupWidget(view);
}

KisStorageChooserWidget::~KisStorageChooserWidget()
{

}
