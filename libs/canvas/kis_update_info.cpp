/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_update_info.h"

#include <KisStaticInitializer.h>

KIS_DECLARE_STATIC_INITIALIZER {
    qRegisterMetaType<KisUpdateInfoSP>("KisUpdateInfoSP");
}

KisUpdateInfo::KisUpdateInfo() = default;

KisUpdateInfo::~KisUpdateInfo() = default;

QRect KisUpdateInfo::dirtyViewportRect()
{
    return QRect();
}

bool KisUpdateInfo::canBeCompressed() const
{
    return true;
}

KisMarkerUpdateInfo::KisMarkerUpdateInfo(Type type, const QRect &dirtyImageRect)
    : m_type(type)
    , m_dirtyImageRect(dirtyImageRect)
{
}

KisMarkerUpdateInfo::Type KisMarkerUpdateInfo::type() const
{
    return m_type;
}

QRect KisMarkerUpdateInfo::dirtyImageRect() const
{
    return m_dirtyImageRect;
}

int KisMarkerUpdateInfo::levelOfDetail() const
{
    return -1 - static_cast<int>(m_type);
}

bool KisMarkerUpdateInfo::canBeCompressed() const
{
    return false;
}
