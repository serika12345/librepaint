/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_opengl_update_info.h"

KisOpenGLUpdateInfo::KisOpenGLUpdateInfo() = default;

QRect KisOpenGLUpdateInfo::dirtyViewportRect()
{
    qFatal("Not implemented yet!");
    return QRect();
}

void KisOpenGLUpdateInfo::assignDirtyImageRect(const QRect &rect)
{
    m_dirtyImageRect = rect;
}

void KisOpenGLUpdateInfo::assignLevelOfDetail(int lod)
{
    m_levelOfDetail = lod;
}

QRect KisOpenGLUpdateInfo::dirtyImageRect() const
{
    return m_dirtyImageRect;
}

int KisOpenGLUpdateInfo::levelOfDetail() const
{
    return m_levelOfDetail;
}

bool KisOpenGLUpdateInfo::tryMergeWith(const KisOpenGLUpdateInfo &rhs)
{
    if (m_levelOfDetail != rhs.m_levelOfDetail) {
        return false;
    }

    m_dirtyImageRect |= rhs.m_dirtyImageRect;
    tileList.append(rhs.tileList);
    return true;
}
