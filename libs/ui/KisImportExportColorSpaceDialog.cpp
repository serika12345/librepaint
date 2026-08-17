/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisImportExportColorSpaceDialog.h>

#include <QDialog>

#include <KLocalizedString>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_image.h>

#include "dialogs/KisColorSpaceConversionDialog.h"

bool KisImportExportColorSpaceDialog::selectEditableColorSpace(
    QWidget *parent,
    const KoColorSpace *sourceColorSpace,
    const KoColorSpace **replacementColorSpace,
    KoColorConversionTransformation::Intent *intent,
    KoColorConversionTransformation::ConversionFlags *flags)
{
    KisColorSpaceConversionDialog dialog(parent, "ColorSpaceConversion");
    const KoColorSpace *fallbackColorSpace = KoColorSpaceRegistry::instance()->colorSpace(
        sourceColorSpace->colorModelId().id(),
        sourceColorSpace->colorDepthId().id(),
        nullptr);

    dialog.setCaption(i18n("Convert image color space on import"));
    dialog.m_page->lblHeadlineWarning->setText(
        i18nc("the argument is the ICC profile name",
              "The image has a profile attached that LibrePaint cannot edit images in (\"%1\"), "
              "please select a space to convert to for editing: \n",
              sourceColorSpace->profile()->name()));
    dialog.m_page->lblHeadlineWarning->setVisible(true);
    dialog.setInitialColorSpace(fallbackColorSpace, 0);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    *replacementColorSpace = dialog.colorSpace();
    *intent = dialog.conversionIntent();
    *flags = dialog.conversionFlags();
    return true;
}
