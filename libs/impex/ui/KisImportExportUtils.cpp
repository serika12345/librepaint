/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportUtils.h"

#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include "kis_image.h"
#include "KisImportUserFeedbackInterface.h"
#include "KisImportExportColorSpaceDialog.h"

namespace KritaUtils {

KisImportExportErrorCode workaroundUnsuitableImageColorSpace(KisImageSP image,
                                                             KisImportUserFeedbackInterface *feedbackInterface,
                                                             KisImageBarrierLock &lock)
{
    const KoColorSpace *replacementColorSpace = 0;
    KoColorConversionTransformation::Intent replacementColorSpaceIntent = KoColorConversionTransformation::internalRenderingIntent();
    KoColorConversionTransformation::ConversionFlags replacementColorSpaceConversionFlags = KoColorConversionTransformation::internalConversionFlags();

    const KoColorSpace *colorSpace = image->colorSpace();
    const KoColorProfile *profile = colorSpace->profile();

    if (profile && !profile->isSuitableForOutput()) {
        /// The profile has no reverse mapping into for the described color space,
        /// so we cannot use it in Krita. We need to ask the user to convert the image
        /// right on loading

        KIS_SAFE_ASSERT_RECOVER_NOOP(feedbackInterface);
        if (feedbackInterface) {
            KisImportUserFeedbackInterface::Result result =
                feedbackInterface->askUser([&] (QWidget *parent) {

                    return KisImportExportColorSpaceDialog::selectEditableColorSpace(
                        parent,
                        colorSpace,
                        &replacementColorSpace,
                        &replacementColorSpaceIntent,
                        &replacementColorSpaceConversionFlags);
                });

            if (result == KisImportUserFeedbackInterface::SuppressedByBatchMode) {
                return ImportExportCodes::FormatColorSpaceUnsupported;
            } else if (result == KisImportUserFeedbackInterface::UserCancelled) {
                return ImportExportCodes::Cancelled;
            }
        }
    }

    if (replacementColorSpace) {
        /**
         * Here is an extremely tricky part! First we start the conversion
         * stroke, and only **after that** we unlock the image. The point is
         * that KisDelayedUpdateNodeInterface-based nodes are forbidden to
         * start their update jobs while the image is locked, so this
         * guarantees that no stroke will be started before we actually convert
         * the image into something usable
         */
        image->convertImageColorSpace(replacementColorSpace,
                                        replacementColorSpaceIntent,
                                        replacementColorSpaceConversionFlags);
        lock.unlock();
        image->waitForDone();
    }

    return ImportExportCodes::OK;
}

}
