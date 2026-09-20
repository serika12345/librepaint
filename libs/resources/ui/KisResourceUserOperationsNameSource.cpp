/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceUserOperationsNameSource_p.h"

#include <algorithm>
#include <iterator>

#include "KisResourceModel.h"

namespace KisResourceUserOperationsNameSource
{
QVector<int> resourceIdsForName(KisResourceModel *resourceModel,
                                const QString &resourceName)
{
    const QVector<KoResourceSP> resources = resourceModel->resourcesForName(resourceName);
    QVector<int> resourceIds;
    resourceIds.reserve(resources.size());
    std::transform(resources.cbegin(),
                   resources.cend(),
                   std::back_inserter(resourceIds),
                   [](const KoResourceSP &resource) {
                       return resource->resourceId();
                   });
    return resourceIds;
}
}
