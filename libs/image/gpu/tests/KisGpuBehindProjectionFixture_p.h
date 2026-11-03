/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BEHIND_PROJECTION_FIXTURE_P_H
#define KIS_GPU_BEHIND_PROJECTION_FIXTURE_P_H

#include <QByteArray>

namespace LibrePaint::Image::Gpu::Test {
// CPU KoCompositeOpBehind<RGBA8, KoAdditiveBlendingPolicy> at 2fcef58e95,
// Linux x86_64, GCC 15.3.0, Qt 6.11.1; transparent bottom normalizes RGB to zero.
// Two uniform layers: normal (37,89,151,A), behind (201,41,93,A).
// Row order: destination alpha0/173/255, source alpha0/137/255,
// opacity137/255 and RGBA mask15/9/6/0. Alpha locking follows KoCompositeOpBase.
inline QByteArray behindProjectionReferences()
{
    return QByteArray::fromHex(
        "0000000000000000000000000000000000000000000000000000000000000000c9295d4ac900004a00295d0000000000"
        "c9295d89c900008900295d0000000000c9295d89c900008900295d0000000000c9295dffc90000ff00295d0000000000"
        "255997ad255997ad255997ad255997ad255997ad255997ad255997ad255997ad395390c5395997c5255390ad255997ad"
        "474f8cd9475997d9254f8cad255997ad474f8cd9475997d9254f8cad255997ad5a4a84ff5a5997ff254a84ad255997ad"
        "255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff"
        "255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff255997ff");
}
}
#endif
