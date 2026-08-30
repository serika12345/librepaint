/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISUPDATESELECTIONJOBSELECTIONACCESS_P_H
#define KISUPDATESELECTIONJOBSELECTIONACCESS_P_H

#include "kis_types.h"

class QRect;

namespace KisUpdateSelectionJobSelectionAccess
{

KisNodeSP parentNode(KisSelection *selection);
QRect extent(KisNode *node);
void updateProjection(KisSelection *selection);
void updateProjection(KisSelection *selection, const QRect &rect);
void notifySelectionChanged(KisSelection *selection);
bool isOverlayProjectionLeaf(KisNode *node);
void setDirty(KisNode *node, const QRect &rect);

} // namespace KisUpdateSelectionJobSelectionAccess

#endif // KISUPDATESELECTIONJOBSELECTIONACCESS_P_H
