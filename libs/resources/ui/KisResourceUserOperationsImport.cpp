/*
 * SPDX-FileCopyrightText: 2021 Agata Cacko <cacko.azh@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperations.h"

#include "KisResourceUserOperationsImportSource_p.h"

KoResourceSP KisResourceUserOperations::importResourceFileWithUserInput(
    QWidget *widgetParent,
    QString storageLocation,
    QString resourceType,
    QString resourceFilepath)
{
    auto attempt = KisResourceUserOperationsImportSource::importResourceFile(
        resourceType, resourceFilepath, storageLocation, false);

    if (attempt.resource.isNull() && storageLocation.isEmpty() && attempt.wouldOverwrite) {
        if (!userAllowsOverwrite(widgetParent, resourceFilepath)) {
            return nullptr;
        }
        attempt = KisResourceUserOperationsImportSource::importResourceFile(
            resourceType, resourceFilepath, storageLocation, true);
    }

    if (attempt.resource.isNull()) {
        KisResourceUserOperationsImportSource::warnImportFailed(widgetParent);
    }
    return attempt.resource;
}
