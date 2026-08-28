/*
 * SPDX-FileCopyrightText: 2021 Agata Cacko <cacko.azh@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperations.h"

#include <kis_assert.h>

#include "KisResourceUserOperationsUpdateSource_p.h"

bool KisResourceUserOperations::updateResourceWithUserInput(QWidget *widgetParent,
                                                             KoResourceSP resource)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(resource, false);

    if (KisResourceUserOperationsUpdateSource::isExternalResource(resource)) {
        resource = prepareExternalResource(resource);
        if (resource.isNull()) {
            KisResourceUserOperationsUpdateSource::warnUpdateFailed(widgetParent);
            return false;
        }
    }

    if (KisResourceUserOperationsUpdateSource::resourceNameChanged(resource)
        && KisResourceUserOperationsUpdateSource::resourceNameIsAlreadyUsed(resource)
        && !KisResourceUserOperationsUpdateSource::userAllowsDuplicateName(
            widgetParent)) {
        return false;
    }

    const bool result =
        KisResourceUserOperationsUpdateSource::updateResource(resource);
    if (!result) {
        KisResourceUserOperationsUpdateSource::warnUpdateFailed(widgetParent);
    }
    return result;
}
