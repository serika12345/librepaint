/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISUPDATEOUTLINEJOBSELECTIONACCESS_P_H
#define KISUPDATEOUTLINEJOBSELECTIONACCESS_P_H

class QColor;
class KisSelection;

namespace KisUpdateOutlineJobSelectionAccess
{

void recalculateOutlineCache(KisSelection *selection);
void recalculateThumbnailImage(KisSelection *selection, const QColor &maskColor);
void notifySelectionChanged(KisSelection *selection);

} // namespace KisUpdateOutlineJobSelectionAccess

#endif // KISUPDATEOUTLINEJOBSELECTIONACCESS_P_H
