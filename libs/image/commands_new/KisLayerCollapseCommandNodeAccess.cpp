/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLayerCollapseCommandNodeAccess_p.h"

#include "kis_node.h"

QString KisLayerCollapseCommandNodeAccess::name(KisNode *node)
{
    return node->name();
}

bool KisLayerCollapseCommandNodeAccess::collapsed(KisNode *node)
{
    return node->collapsed();
}

void KisLayerCollapseCommandNodeAccess::setCollapsed(KisNode *node, bool value)
{
    node->setCollapsed(value);
}
