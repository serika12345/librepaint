/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

#include "kis_shared_ptr.h"

bool KisNodeModel::belongsToIsolatedGroup(KisImageSP image, KisNodeSP node, KisDummiesFacadeBase *dummiesFacade)
{
    KisNodeSP isolatedRoot = IsolationMembershipAccess::isolationRoot(image);
    if (!isolatedRoot) {
        return true;
    }

    KisNodeDummy *isolatedRootDummy = IsolationMembershipAccess::dummyForNode(dummiesFacade, isolatedRoot);
    KisNodeDummy *dummy = IsolationMembershipAccess::dummyForNode(dummiesFacade, node);

    while (dummy) {
        if (dummy == isolatedRootDummy) {
            return true;
        }
        dummy = IsolationMembershipAccess::parentDummy(dummy);
    }

    return false;
}
