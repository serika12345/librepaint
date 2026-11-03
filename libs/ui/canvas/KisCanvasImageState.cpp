/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "canvas/kis_canvas2.h"
#include "kis_types.h"

#include <KoColorProfile.h>
#include <kis_image.h>
#include <qnamespace.h>
#include <QtGlobal>

void KisCanvas2::connectViewImageSignals()
{
    KisImageSP currentImage = image();
    const KisImageWSP source = currentImage;

    connect(currentImage.data(),
            &KisImage::sigSizeChanged,
            this,
            [this, source](const QPointF &oldStillPoint, const QPointF &newStillPoint) {
                if (source && source.data() == image().data())
                    Q_EMIT sigViewImageSizeChanged(oldStillPoint, newStillPoint);
            });
    connect(currentImage.data(),
            &KisImage::sigResolutionChanged,
            this,
            [this, source](double, double) {
                if (source && source.data() == image().data()) Q_EMIT sigViewImageResolutionChanged();
            });
    connect(currentImage.data(),
            &KisImage::sigNodeAddedAsync,
            this,
            &KisCanvas2::sigViewNodeAddedAsync,
            Qt::DirectConnection);
    connect(currentImage.data(),
            &KisImage::sigRemoveNodeAsync,
            this,
            &KisCanvas2::sigViewNodeRemovedAsync,
            Qt::DirectConnection);
    connect(currentImage.data(),
            &KisImage::sigColorSpaceChanged,
            this,
            [this, source](const KoColorSpace *colorSpace) {
                if (source && source.data() == image().data()) Q_EMIT sigViewImageColorSpaceChanged(colorSpace);
            });
    connect(currentImage.data(),
            &KisImage::sigProfileChanged,
            this,
            [this, source](const KoColorProfile *profile) {
                if (source && source.data() == image().data()) Q_EMIT sigViewImageProfileChanged(profile);
            });
}

void KisCanvas2::prepareImageForDisplay()
{
    KisImageSP currentImage = image();
    if (currentImage->locked()) {
        currentImage->blockSignals(false);
        currentImage->unlock();
    }

    initializeImage();
}

bool KisCanvas2::imageUsesFloatingPointColorDepth() const
{
    return image()->colorSpace()->colorDepthId().id().contains("F");
}
