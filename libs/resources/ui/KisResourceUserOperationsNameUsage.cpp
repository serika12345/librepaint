/*
 * SPDX-FileCopyrightText: 2021 Agata Cacko <cacko.azh@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperations.h"

#include <algorithm>

#include "KisResourceUserOperationsNameSource_p.h"

bool KisResourceUserOperations::resourceNameIsAlreadyUsed(KisResourceModel *resourceModel,
                                                           QString resourceName,
                                                           int resourceIdToIgnore)
{
    const auto containsOtherResource = [resourceIdToIgnore](const QVector<int> &resourceIds) {
        if (resourceIdToIgnore < 0) {
            return !resourceIds.isEmpty();
        }

        return std::any_of(resourceIds.cbegin(),
                           resourceIds.cend(),
                           [resourceIdToIgnore](int resourceId) {
                               return resourceId != resourceIdToIgnore;
                           });
    };

    if (containsOtherResource(
            KisResourceUserOperationsNameSource::resourceIdsForName(resourceModel,
                                                                     resourceName))) {
        return true;
    }

    resourceName.replace(QLatin1Char(' '), QLatin1Char('_'));
    return containsOtherResource(
        KisResourceUserOperationsNameSource::resourceIdsForName(resourceModel,
                                                                 resourceName));
}
