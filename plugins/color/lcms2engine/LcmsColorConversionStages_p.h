/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include <KoColorConversionStages.h>
#include <NativeLabConversion.h>
#include <lcms2.h>
class KoColorSpace;
KoColorConversionStagesResult lcmsDisplayStages(cmsHTRANSFORM transform,
    const KoColorSpace *source, const KoColorSpace *destination);

LibrePaint::Pigment::Color::NativeLabConversionResult lcmsNativeLabConversion(cmsHTRANSFORM transform,
    const KoColorSpace *source, const KoColorSpace *destination);
