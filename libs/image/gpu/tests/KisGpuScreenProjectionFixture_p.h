/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_SCREEN_PROJECTION_FIXTURE_P_H
#define KIS_GPU_SCREEN_PROJECTION_FIXTURE_P_H

#include <QByteArray>

namespace LibrePaint::Image::Gpu::Test {
// RGBA8/sRGB CPU projection at 34f82e48cd, equal on ARM64 and x86_64.
// Two uniform 256x256 layers: normal (37,89,151,A), screen (201,41,93,A).
// Row order: destination alpha0/173/255, source alpha0/137/255,
// opacity137/255 and RGBA mask15/9/6/0.
inline QByteArray screenProjectionReferences()
{
    return QByteArray::fromHex(
        "0000000000000000000000000000000000000000000000000000000000000000c9295d4ac900004a0000000000000000"
        "c9295d89c90000890000000000000000c9295d89c90000890000000000000000c9295dffc90000ff0000000000000000"
        "255997ad255997ad255997ad255997ad255997ad255997ad255997ad255997ad655b9ac5655997c52561a2ad255997ad"
        "915a9bd9915997d92568abad255997ad915a9bd9915997d92568abad255997adce5c9effce5997ff2574bdad255997ad"
        "255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff5761a2ff575997ff2561a2ff255997ff"
        "8168abff815997ff2568abff255997ff8168abff815997ff2568abff255997ffd174bdffd15997ff2574bdff255997ff");
}
}
#endif
