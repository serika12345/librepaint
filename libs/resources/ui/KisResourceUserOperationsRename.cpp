/*
 * SPDX-FileCopyrightText: 2021 Agata Cacko <cacko.azh@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperations.h"

#include <kis_assert.h>

#include "KisResourceUserOperationsRenameSource_p.h"

bool KisResourceUserOperations::renameResourceWithUserInput(QWidget *widgetParent,
                                                             KoResourceSP resource,
                                                             QString resourceName)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(resource, false);

    if (KisResourceUserOperationsRenameSource::resourceNameIsAlreadyUsed(
            resource, resourceName)
        && !KisResourceUserOperationsRenameSource::userAllowsDuplicateName(
            widgetParent)) {
        return false;
    }

    const bool result = KisResourceUserOperationsRenameSource::renameResource(
        resource, resourceName);
    if (!result) {
        KisResourceUserOperationsRenameSource::warnRenameFailed(widgetParent);
    }
    return result;
}
