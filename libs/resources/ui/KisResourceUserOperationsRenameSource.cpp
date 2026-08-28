/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperationsRenameSource_p.h"

#include <QMessageBox>

#include <klocalizedstring.h>

#include "KisResourceModel.h"
#include "KisResourceUserOperations.h"

namespace KisResourceUserOperationsRenameSource
{
bool resourceNameIsAlreadyUsed(KoResourceSP resource, const QString &resourceName)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowActiveResources);
    return KisResourceUserOperations::resourceNameIsAlreadyUsed(
        &resourceModel, resourceName, resource->resourceId());
}

bool userAllowsDuplicateName(QWidget *widgetParent)
{
    return QMessageBox::question(
               widgetParent,
               i18nc("@title:window", "Rename the resource?"),
               i18nc("Question in a dialog/messagebox",
                     "This name is already used for another resource. "
                     "Do you want to use the same name for multiple resources?"
                     "(If you decline now, the resource won't be renamed)."),
               QMessageBox::Yes | QMessageBox::Cancel,
               QMessageBox::Cancel)
        != QMessageBox::Cancel;
}

bool renameResource(KoResourceSP resource, const QString &resourceName)
{
    KisResourceModel resourceModel(resource->resourceType().first);
    resourceModel.setResourceFilter(KisResourceModel::ShowActiveResources);
    return resourceModel.renameResource(resource, resourceName);
}

void warnRenameFailed(QWidget *widgetParent)
{
    QMessageBox::warning(widgetParent,
                         i18nc("@title:window", "Failed to rename the resource"),
                         i18nc("Warning message", "Failed to rename the resource."));
}
}
