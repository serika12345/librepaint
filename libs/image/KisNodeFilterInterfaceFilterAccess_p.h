/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_NODE_FILTER_INTERFACE_FILTER_ACCESS_P_H
#define KIS_NODE_FILTER_INTERFACE_FILTER_ACCESS_P_H

class KisFilterConfiguration;

template<typename T>
class KisPinnedSharedPtr;

void kisSharedPtrAddReference(KisFilterConfiguration *configuration);
bool kisSharedPtrRelease(KisFilterConfiguration *configuration);

class KisNodeFilterInterfaceFilterAccess
{
public:
    static void acquire(KisFilterConfiguration *configuration);
    static bool release(KisFilterConfiguration *configuration);
    static bool hasLocalResourcesSnapshot(const KisFilterConfiguration *configuration);
    static KisPinnedSharedPtr<KisFilterConfiguration> clone(const KisFilterConfiguration *configuration);
};

#endif // KIS_NODE_FILTER_INTERFACE_FILTER_ACCESS_P_H
