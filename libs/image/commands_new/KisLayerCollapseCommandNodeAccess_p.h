/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISLAYERCOLLAPSECOMMANDNODEACCESS_P_H
#define KISLAYERCOLLAPSECOMMANDNODEACCESS_P_H

class KisNode;
class QString;

namespace KisLayerCollapseCommandNodeAccess
{

QString name(KisNode *node);
bool collapsed(KisNode *node);
void setCollapsed(KisNode *node, bool value);

} // namespace KisLayerCollapseCommandNodeAccess

#endif // KISLAYERCOLLAPSECOMMANDNODEACCESS_P_H
