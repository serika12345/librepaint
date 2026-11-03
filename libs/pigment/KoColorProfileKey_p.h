/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef KOCOLORPROFILEKEY_P_H
#define KOCOLORPROFILEKEY_P_H

#include "KoColorProfile.h"

#include <QPair>

using KoColorProfileKey = QPair<QString, QByteArray>;

inline KoColorProfileKey koColorProfileKey(const KoColorProfile *profile)
{
    return {profile->name(), profile->rawData()};
}

#endif
