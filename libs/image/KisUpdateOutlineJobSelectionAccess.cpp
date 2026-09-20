/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisUpdateOutlineJobSelectionAccess_p.h"

#include "kis_selection.h"

void KisUpdateOutlineJobSelectionAccess::recalculateOutlineCache(KisSelection *selection)
{
    selection->recalculateOutlineCache();
}

void KisUpdateOutlineJobSelectionAccess::recalculateThumbnailImage(KisSelection *selection, const QColor &maskColor)
{
    selection->recalculateThumbnailImage(maskColor);
}

void KisUpdateOutlineJobSelectionAccess::notifySelectionChanged(KisSelection *selection)
{
    selection->notifySelectionChanged();
}
