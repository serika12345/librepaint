/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNodeRenameCommandNodeAccess_p.h"

#include "kis_node.h"

void KisNodeRenameCommandNodeAccess::setName(KisNode *node, const QString &name)
{
    node->setName(name);
}
