/*
 * SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTextureOptionData.h"

#include <kis_paintop_lod_limitations.h>

KisPaintopLodLimitations KisTextureOptionData::lodLimitations() const
{
    KisPaintopLodLimitations limitations;
    if (isEnabled) {
        limitations.limitations << KoID(
            "texture-pattern",
            i18nc("PaintOp instant preview limitation", "Texture->Pattern (low quality preview)"));
    }
    return limitations;
}
