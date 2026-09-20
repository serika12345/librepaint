/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISNODERENAMECOMMANDNODEACCESS_P_H
#define KISNODERENAMECOMMANDNODEACCESS_P_H

class KisNode;
class QString;

namespace KisNodeRenameCommandNodeAccess
{

void setName(KisNode *node, const QString &name);

}

#endif // KISNODERENAMECOMMANDNODEACCESS_P_H
