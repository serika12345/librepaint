/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoSimpleColorSpaceFactory.h"

#include <colorprofiles/KoDummyColorProfile.h>

bool KoSimpleColorSpaceFactory::profileIsCompatible(const KoColorProfile *profile) const
{
    return dynamic_cast<const KoDummyColorProfile *>(profile);
}
