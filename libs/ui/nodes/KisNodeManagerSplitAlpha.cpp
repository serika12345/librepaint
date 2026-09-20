/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <kis_assert.h>
#include <klocalizedstring.h>

void KisNodeManager::slotSplitAlphaIntoMask()
{
    const KisNodeSP node = SplitAlphaAccess::activeNode(this);
    if (!SplitAlphaAccess::canModifyLayer(this, node)) {
        return;
    }

    KIS_ASSERT_RECOVER_RETURN(SplitAlphaAccess::hasEditablePaintDevice(node));

    const QString maskName =
        SplitAlphaAccess::createMaskName(this, node, QStringLiteral("KisTransparencyMask"), i18n("Transparency Mask"));
    SplitAlphaAccess::splitAlphaToMask(node, maskName);
}

void KisNodeManager::slotSplitAlphaWrite()
{
    SplitAlphaAccess::mergeTransparencyMaskAsAlpha(this, true);
}

void KisNodeManager::slotSplitAlphaSaveMerged()
{
    SplitAlphaAccess::mergeTransparencyMaskAsAlpha(this, false);
}
