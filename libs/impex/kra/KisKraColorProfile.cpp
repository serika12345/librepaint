/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisKraRasterCodec.h"
#include <KoColorProfile.h>
#include <KoColorProfileQuery.h>
#include <KoColorSpaceEngine.h>
#include <KoColorSpaceRegistry.h>

namespace LibrePaint::ImportExport::Kra {
QHash<QString, QString> RasterCodec::profileNameAliases()
{
    QHash<QString, QString> aliases;
    KoColorProfileQuery query(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0,
                              TRC_ITU_R_BT_2100_0_PQ, 80.0);
    const auto profile = KoColorSpaceRegistry::instance()->profileFor(query, false);
    if (profile) {
        aliases.insert(QStringLiteral("High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF"),
                       profile->name());
    }
    query.hdrReferenceWhite = 180.0;
    const auto engine = KoColorSpaceEngineRegistry::instance()->get(QStringLiteral("icc"));
    auto uncommon = KoColorSpaceRegistry::instance()->profileFor(query, false);
    if (!uncommon && engine) uncommon = engine->getProfile(query);
    if (uncommon) aliases.insert(QStringLiteral("Krita Rec. 2100 Perceptual Quantizer 180nits"), uncommon->name());
    return aliases;
}
}
