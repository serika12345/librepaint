/*
 * SPDX-FileCopyrightText: 2021 Agata Cacko <cacko.azh@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperations.h"

#include <kis_assert.h>

#include "KisResourceUserOperationsAddSource_p.h"

bool KisResourceUserOperations::addResourceWithUserInput(QWidget *widgetParent,
                                                          KoResourceSP resource,
                                                          QString storageLocation)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(resource, false);

    if (resourceFilenameIsAlreadyUsed(resource, storageLocation)) {
        if (!KisResourceUserOperationsAddSource::userAllowsOverwrite(widgetParent)) {
            return false;
        }
        return updateResourceWithUserInput(widgetParent, resource);
    }

    if (KisResourceUserOperationsAddSource::resourceNameIsAlreadyUsed(resource)
        && !KisResourceUserOperationsAddSource::userAllowsDuplicateName(widgetParent)) {
        return false;
    }

    const bool result = KisResourceUserOperationsAddSource::addResource(
        resource, storageLocation);
    if (!result) {
        KisResourceUserOperationsAddSource::warnAddFailed(widgetParent);
    }
    return result;
}
