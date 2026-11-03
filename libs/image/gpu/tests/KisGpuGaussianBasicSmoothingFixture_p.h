/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GAUSSIAN_BASIC_SMOOTHING_FIXTURE_P_H
#define KIS_GPU_GAUSSIAN_BASIC_SMOOTHING_FIXTURE_P_H

#include "../KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Test {
struct GaussianBasicCase
{
    const char *name;
    QList<InputSample> input;
    qsizetype flushAt;
    QByteArray rgbaSha256;
};

inline QList<GaussianBasicCase> gaussianBasicCases()
{
    // Normal CPU Basic tool, shipped Basic-5 preset, full mouse pressure,
    // 128x128 RGBA8 sRGB transparent canvas, foreground (17,34,51),
    // flow .5, canvas opacity .57 and four workers. Only channel order changes.
    // Curve/closed/idle/two/return pixels originate at 62cfadd521. Current
    // tap/stationary and duplicate-first observations follow c75fece's repair.
    // The supplemental first time is uniquely inferred from a monotonic [0,0]
    // interval after PI construction; it was not directly intercepted.
    // For two/return, first time 0 is a fixed test input, not an old observation:
    // that time is unused by their emitted final POINT/LINE respectively.
    // Old stationary blank is a corrected defect; 2/3/8 equal the current tap.
    return {
        {"tap", {{{12.25, 13.75}, 1, 0, 0, 0, 0}}, 0, QByteArrayLiteral("aac1e2aa27458aafbfd956b779f37e97e6127ae7fe1722714a180dcb9160f69b")},
        {"two", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 18}}, 0, QByteArrayLiteral("f96ad27f722038183057b7f17ad46428ce853c13585c666198b823382be42fba")},
        {"curve", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 24}, {{105.5, 91.25}, 1, 0, 0, 0, 48}}, 0, QByteArrayLiteral("ed56e863942109c60a0705bc13ecd869ca4e9982ba25494a48368e6f3f2e0ee3")},
        {"closed", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 24}, {{105.5, 91.25}, 1, 0, 0, 0, 48}, {{12.25, 13.75}, 1, 0, 0, 0, 72}}, 0, QByteArrayLiteral("435f2f2c857dfbeaa68b938a61c30dd8d3329698ecc35bbc9652ce41054e87c1")},
        {"stationary3", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{12.25, 13.75}, 1, 0, 0, 0, 20}, {{12.25, 13.75}, 1, 0, 0, 0, 40}}, 0, QByteArrayLiteral("aac1e2aa27458aafbfd956b779f37e97e6127ae7fe1722714a180dcb9160f69b")},
        {"duplicate-first", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{12.25, 13.75}, 1, 0, 0, 0, 21}, {{105.5, 91.25}, 1, 0, 0, 0, 41}}, 0, QByteArrayLiteral("5e7750d4cb069d164846a505e12c576b65386716353fb3aab33fbb53a616c998")},
        {"return", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 24}, {{12.25, 13.75}, 1, 0, 0, 0, 48}}, 0, QByteArrayLiteral("fd70eb9cd2528fea3af606222e5a777c1ab66e5e546e1b4cf3f971b3ec16c2f7")},
        {"idle-two-continue", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 24}, {{105.5, 91.25}, 1, 0, 0, 0, 48}, {{40.5, 102.25}, 1, 0, 0, 0, 72}}, 2, QByteArrayLiteral("0a8d384b47eef3559cd76eacb87a07873e834c8f86db9b4bf0d22b6a19c4387c")},
        {"idle-curve-continue", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{60.125, 31.5}, 1, 0, 0, 0, 24}, {{105.5, 91.25}, 1, 0, 0, 0, 46}, {{40.5, 102.25}, 1, 0, 0, 0, 70}}, 3, QByteArrayLiteral("97738bd7b15fea9bc54a628192c16a2a4fd8aa246727bed8a3e4dfe12395e2f2")},
        {"stationary2", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{12.25, 13.75}, 1, 0, 0, 0, 16}}, 0, QByteArrayLiteral("aac1e2aa27458aafbfd956b779f37e97e6127ae7fe1722714a180dcb9160f69b")},
        {"stationary8", {{{12.25, 13.75}, 1, 0, 0, 0, 0}, {{12.25, 13.75}, 1, 0, 0, 0, 20}, {{12.25, 13.75}, 1, 0, 0, 0, 40}, {{12.25, 13.75}, 1, 0, 0, 0, 58}, {{12.25, 13.75}, 1, 0, 0, 0, 75}, {{12.25, 13.75}, 1, 0, 0, 0, 92}, {{12.25, 13.75}, 1, 0, 0, 0, 112}, {{12.25, 13.75}, 1, 0, 0, 0, 132}}, 0, QByteArrayLiteral("aac1e2aa27458aafbfd956b779f37e97e6127ae7fe1722714a180dcb9160f69b")},
    };
}
}
#endif
