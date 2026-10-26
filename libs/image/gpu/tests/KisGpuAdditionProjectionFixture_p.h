/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_ADDITION_PROJECTION_FIXTURE_P_H
#define KIS_GPU_ADDITION_PROJECTION_FIXTURE_P_H

#include <QByteArray>

namespace LibrePaint::Image::Gpu::Test {
// RGBA8/sRGB CPU projection from 572d3317f6 with coefficient-owner source
// 12a4c1d4ee3f. Both stable IDs add/linear_dodge produce identical bytes.
// Two uniform 256x256 layers: normal (37,89,151,A), addition (208,224,240,A).
// Stored and default pixels were independently read through Document.pixelData.
// Row order: destination alpha0/173/255, source alpha0/137/255,
// opacity137/255 and RGBA mask15/9/6/0 (including inherited alpha).
// Native CPU channel flags are mapped from BGRA to this RGBA mask.
inline QByteArray additionProjectionReferences()
{
    return QByteArray::fromHex(
        "0000000000000000000000000000000000000000000000000000000000000000d0e0f04ad000004a0000000000000000"
        "d0e0f089d00000890000000000000000d0e0f089d00000890000000000000000d0e0f0ffd00000ff0000000000000000"
        "255997ad255997ad255997ad255997ad255997ad255997ad255997ad255997ad6e94bcc56e5997c52589b5ad255997ad"
        "a1bcd5d9a15997d925b2cfad255997ada1bcd5d9a15997d925b2cfad255997ade9f5faffe95997ff25ffffad255997ad"
        "255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff6189b5ff615997ff2589b5ff255997ff"
        "95b2cfff955997ff25b2cfff255997ff95b2cfff955997ff25b2cfff255997fff5fffffff55997ff25ffffff255997ff");
}
}
#endif
