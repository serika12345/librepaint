/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef BUILTIN_LCMS_PROFILE_P_H
#define BUILTIN_LCMS_PROFILE_P_H

#include <lcms2.h>

class IccColorProfile;

/** Consumes the native handle of a built-in standard definition. The caller
 * owns the returned profile. Its creation date, portable platform and ICC ID
 * describe the fixed definition; invalid native data yields an invalid profile.
 */
IccColorProfile *createBuiltinLcmsProfile(cmsHPROFILE profile);

#endif
