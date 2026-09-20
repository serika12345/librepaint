/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QAbstractItemModel>
#include <QApplication>
#include <QMessageBox>

#include "KisResourceModel.h"
#include "KisResourceStorage.h"
#include "KisResourceTypes.h"
#include "KisStorageChooserWidget.h"
#include "KisStorageChooserWidgetModelSource_p.h"
#include "KisStorageFilterProxyModel.h"
#include "KisStorageModel.h"

void KisStorageChooserWidget::activated(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const bool active = index.data(Qt::UserRole + KisStorageModel::Active).value<bool>();
    QAbstractItemModel *storageModel = KisStorageChooserWidgetModelSource::model();
    storageModel->setData(index, !active, Qt::CheckStateRole);

    KisStorageFilterProxyModel proxy;
    proxy.setSourceModel(storageModel);

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

    repaint();
}
