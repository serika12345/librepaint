/*
 *  SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_update_outline_job.h"
#include "KisUpdateOutlineJobSelectionAccess_p.h"

KisUpdateOutlineJob::KisUpdateOutlineJob(KisSelectionSP selection, bool updateThumbnail, const QColor &maskColor)
    : m_selection(selection)
    , m_updateThumbnail(updateThumbnail)
    , m_maskColor(maskColor)
{
}

bool KisUpdateOutlineJob::overrides(const KisSpontaneousJob *otherJob)
{
    return dynamic_cast<const KisUpdateOutlineJob *>(otherJob);
}

void KisUpdateOutlineJob::run()
{
    KisUpdateOutlineJobSelectionAccess::recalculateOutlineCache(m_selection.data());
    if (m_updateThumbnail) {
        KisUpdateOutlineJobSelectionAccess::recalculateThumbnailImage(m_selection.data(), m_maskColor);
    }
    KisUpdateOutlineJobSelectionAccess::notifySelectionChanged(m_selection.data());
}

int KisUpdateOutlineJob::levelOfDetail() const
{
    return 0;
}

QString KisUpdateOutlineJob::debugName() const
{
    return "KisUpdateOutlineJob";
}
