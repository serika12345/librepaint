/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISBATCHNODEUPDATENODEACCESS_P_H
#define KISBATCHNODEUPDATENODEACCESS_P_H

#include "kis_types.h"

namespace KisBatchNodeUpdateNodeAccess
{

KisNodeList sortAndFilterMergeableNodes(KisNodeList nodes);
bool isChildOf(KisNodeSP node, KisNodeSP parent);

} // namespace KisBatchNodeUpdateNodeAccess

#endif // KISBATCHNODEUPDATENODEACCESS_P_H
