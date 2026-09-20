/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNodeUuidInfoNodeAccess_p.h"

#include "kis_node.h"

QUuid KisNodeUuidInfoNodeAccess::uuid(KisNode *node)
{
    return node->uuid();
}

QString KisNodeUuidInfoNodeAccess::name(KisNode *node)
{
    return node->name();
}

KisNodeSP KisNodeUuidInfoNodeAccess::firstChild(KisNode *node)
{
    return node->firstChild();
}

KisNodeSP KisNodeUuidInfoNodeAccess::nextSibling(KisNode *node)
{
    return node->nextSibling();
}
