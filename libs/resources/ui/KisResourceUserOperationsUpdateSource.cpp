/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperationsUpdateSource_p.h"

#include <QBuffer>
#include <QFileInfo>
#include <QMessageBox>

#include <klocalizedstring.h>

#include "KisGlobalResourcesInterface.h"
#include "KisResourceCacheDb.h"
#include "KisResourceLocator.h"
#include "KisResourceModel.h"
#include "KisResourceUserOperations.h"

KoResourceSP KisResourceUserOperations::prepareExternalResource(
    KoResourceSP resource)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);

    if (QFileInfo(resource->storageLocation()).isRelative()) {
        resource->setStorageLocation(
            KisResourceLocator::instance()->makeStorageLocationAbsolute(
                resource->storageLocation()));
    }

    int resourceId = -1;
    const bool found = KisResourceCacheDb::getResourceIdFromVersionedFilename(
        resource->filename(),
        resource->resourceType().first,
        KisResourceLocator::instance()->makeStorageLocationRelative(
            resource->storageLocation()),
        resourceId);
    if (!found) {
        qWarning() << "Could not get resource id from versioned filename"
                   << resource->filename()
                   << resource->resourceType().first;
    }

    KoResourceSP cachedResource;
    if (resourceId >= 0) {
        cachedResource = resourceModel.resourceForId(resourceId);
    }
    if (cachedResource.isNull() || !resource->isSerializable()
        || !cachedResource->isSerializable()) {
        return nullptr;
    }

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    resource->saveToDevice(&buffer);
    buffer.close();
    buffer.open(QIODevice::ReadWrite);
    cachedResource->loadFromDevice(&buffer,
                                   KisGlobalResourcesInterface::instance());
    buffer.close();
    return cachedResource;
}

namespace KisResourceUserOperationsUpdateSource
{
bool isExternalResource(KoResourceSP resource)
{
    return resource->resourceId() < 0;
}

bool resourceNameChanged(KoResourceSP resource)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    const QString oldName = resourceModel
                                .data(resourceModel.indexForResourceId(
                                          resource->resourceId()),
                                      Qt::UserRole + KisAllResourcesModel::Name)
                                .toString();
    return resource->name() != oldName;
}

bool resourceNameIsAlreadyUsed(KoResourceSP resource)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    return KisResourceUserOperations::resourceNameIsAlreadyUsed(
        &resourceModel, resource->name(), resource->resourceId());
}

bool userAllowsDuplicateName(QWidget *widgetParent)
{
    return QMessageBox::question(
               widgetParent,
               i18nc("@title:window", "Rename the resource?"),
               i18nc("Question in a dialog/messagebox",
                     "This name is already used for another resource. Do you want to overwrite "
                     "and use the same name for multiple resources?"
                     "\nIf you cancel, your changes won't be saved."),
               QMessageBox::Yes | QMessageBox::Cancel,
               QMessageBox::Cancel)
        != QMessageBox::Cancel;
}

bool updateResource(KoResourceSP resource)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    return resourceModel.updateResource(resource);
}

void warnUpdateFailed(QWidget *widgetParent)
{
    QMessageBox::warning(widgetParent,
                         i18nc("@title:window", "Failed to overwrite the resource"),
                         i18nc("Warning message", "Failed to overwrite the resource."));
}
}
