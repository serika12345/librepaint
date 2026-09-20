/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperationsImportSource_p.h"

#include <QMessageBox>

#include <klocalizedstring.h>

#include "KisResourceModel.h"

namespace KisResourceUserOperationsImportSource
{
ImportAttempt importResourceFile(const QString &resourceType,
                                 const QString &resourceFilepath,
                                 const QString &storageLocation,
                                 bool overwrite)
{
    KisResourceModel resourceModel(resourceType);
    resourceModel.setResourceFilter(KisResourceModel::ShowActiveResources);

    ImportAttempt attempt;
    attempt.resource = resourceModel.importResourceFile(resourceFilepath,
                                                        overwrite,
                                                        storageLocation);
    if (attempt.resource.isNull() && !overwrite && storageLocation.isEmpty()) {
        attempt.wouldOverwrite =
            resourceModel.importWillOverwriteResource(resourceFilepath,
                                                       storageLocation);
    }
    return attempt;
}

void warnImportFailed(QWidget *widgetParent)
{
    QMessageBox::warning(widgetParent,
                         i18nc("@title:window", "Failed to import the resource"),
                         i18nc("Warning message", "Failed to import the resource."));
}
}
