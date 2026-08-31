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

void kisNodeFilterInterfaceAcquireFilter(KisFilterConfiguration *configuration);
bool kisNodeFilterInterfaceReleaseFilter(KisFilterConfiguration *configuration);
bool kisNodeFilterInterfaceHasLocalResourcesSnapshot(const KisFilterConfiguration *configuration);
KisPinnedSharedPtr<KisFilterConfiguration>
kisNodeFilterInterfaceCloneFilter(const KisFilterConfiguration *configuration);

#endif // KIS_NODE_FILTER_INTERFACE_FILTER_ACCESS_P_H
