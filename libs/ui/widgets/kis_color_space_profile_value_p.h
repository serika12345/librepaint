/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_COLOR_SPACE_PROFILE_VALUE_P_H
#define KIS_COLOR_SPACE_PROFILE_VALUE_P_H

#include <QMetaType>
#include <QSharedPointer>
#include <KoColorProfile.h>

// Selection rows retain their value across profile unregistration.
using KisColorSpaceProfileValue = QSharedPointer<const KoColorProfile>;
Q_DECLARE_METATYPE(KisColorSpaceProfileValue)

inline bool kisSameColorSpaceProfile(const KisColorSpaceProfileValue &left,
                                    const KisColorSpaceProfileValue &right)
{
    return left && right && left->name() == right->name() && left->rawData() == right->rawData();
}

#endif
