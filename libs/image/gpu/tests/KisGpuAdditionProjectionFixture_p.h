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
// Format36, generated before adding the new modes by source12a4c1d4ee3f.
// Lighten layers, a pending stroke and one reversible blend change.
// SHA256 a47eaafa767559c4fb2dee363a8e7d9d3a71e55787b8dee87682ffb36cc04702.
inline QByteArray lightenRecoveryV36()
{
    return qUncompress(QByteArray::fromBase64(
        "AAAEjnic8wnwD2JQYWBgdGBgYNjCAAGsUJoJSjMzIAAjElO8Ys7bw468DAw8zAt/mdf+9mNR2TJ11UgGr6r7r8k+BgUdI2Z4"
        "wfnoYnDAhKQIRv93UIFw7HfOBIFZ/2GqtXEZgwHA5jE6mEB4MNr+A9EGIDuQES21gAAzmjpGJDEwOyg1hZGRiZmFlY2dg5OL"
        "m4eXj1+AR8nC7z+SJkEhYRFRMXEJSSlpGVk5eQWQGD8DCwNqgIGcwOqZl5daxIhNA5KBjIpKyiqqauoamlraOrp6+gYQA5EB"
        "yI2s/qUlQMOwKUY2DFtwQFwHlkYPEbjEEAgRFiTDqBMif+BOQ3Ysa/43lfWbrq8T3bfBTze2e81q1mnbApTXNjZFChyOOWPL"
        "mQQAXif1bg=="));
}
}
#endif
