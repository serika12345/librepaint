/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_projection_update_info.h"

KisProjectionUpdateInfo::KisProjectionUpdateInfo(const QRect &dirtyImageRect)
    : m_dirtyImageRect(dirtyImageRect)
{
}

QRect KisProjectionUpdateInfo::dirtyViewportRect()
{
    return viewportRect.toAlignedRect();
}

QRect KisProjectionUpdateInfo::dirtyImageRect() const
{
    return m_dirtyImageRect;
}

int KisProjectionUpdateInfo::levelOfDetail() const
{
    return 0;
}
