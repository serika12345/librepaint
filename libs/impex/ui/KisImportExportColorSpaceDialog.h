/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_COLOR_SPACE_DIALOG_H
#define KIS_IMPORT_EXPORT_COLOR_SPACE_DIALOG_H

#include <KoColorConversionTransformation.h>

#include <kritaui_export.h>

class KoColorSpace;
class QWidget;

namespace KisImportExportColorSpaceDialog
{
KRITAUI_EXPORT bool selectEditableColorSpace(
    QWidget *parent,
    const KoColorSpace *sourceColorSpace,
    const KoColorSpace **replacementColorSpace,
    KoColorConversionTransformation::Intent *intent,
    KoColorConversionTransformation::ConversionFlags *flags);
}

#endif
