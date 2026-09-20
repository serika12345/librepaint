/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperationsAddSource_p.h"

#include <QMessageBox>

#include <klocalizedstring.h>

#include "KisResourceCacheDb.h"
#include "KisResourceModel.h"
#include "KisResourceUserOperations.h"

bool KisResourceUserOperations::resourceFilenameIsAlreadyUsed(
    KoResourceSP resource,
    const QString &storageLocation)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);

    int resourceId = -1;
    const bool found = KisResourceCacheDb::getResourceIdFromVersionedFilename(
        resource->filename(),
        resource->resourceType().first,
        storageLocation,
        resourceId);
    if (found) {
        resourceModel.resourceForId(resourceId);
    }
    return found;
}

namespace KisResourceUserOperationsAddSource
{
bool userAllowsOverwrite(QWidget *widgetParent)
{
    return QMessageBox::question(
               widgetParent,
               i18nc("@title:window", "Overwrite the resource?"),
               i18nc("Question in a dialog/messagebox",
                     "This filename is already used for another resource. "
                     "Do you want to overwrite that resource?\n"
                     "(If you decline now, nothing will be done)."),
               QMessageBox::Yes | QMessageBox::Cancel,
               QMessageBox::Cancel)
        != QMessageBox::Cancel;
}

bool resourceNameIsAlreadyUsed(KoResourceSP resource)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    return KisResourceUserOperations::resourceNameIsAlreadyUsed(
        &resourceModel, resource->name());
}

bool userAllowsDuplicateName(QWidget *widgetParent)
{
    return QMessageBox::question(
               widgetParent,
               i18nc("@title:window", "Add the resource?"),
               i18nc("Question in a dialog/messagebox",
                     "This name is already used for another resource. "
                     "Do you want to use the same name for multiple resources? "
                     "(If you decline now, the resource won't be added)."),
               QMessageBox::Yes | QMessageBox::Cancel,
               QMessageBox::Cancel)
        != QMessageBox::Cancel;
}

bool addResource(KoResourceSP resource, const QString &storageLocation)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowAllResources);
    return resourceModel.addResource(resource, storageLocation);
}

void warnAddFailed(QWidget *widgetParent)
{
    QMessageBox::warning(widgetParent,
                         i18nc("@title:window", "Failed to add resource"),
                         i18nc("Warning message", "Failed to add the resource."));
}
}
