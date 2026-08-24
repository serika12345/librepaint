/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "canvas/kis_canvas2.h"

#include <kis_image.h>

void KisCanvas2::connectViewImageSignals()
{
    KisImageSP currentImage = image();

    connect(currentImage.data(),
            &KisImage::sigSizeChanged,
            this,
            &KisCanvas2::sigViewImageSizeChanged);
    connect(currentImage.data(),
            &KisImage::sigResolutionChanged,
            this,
            [this](double, double) {
                Q_EMIT sigViewImageResolutionChanged();
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
            &KisCanvas2::sigViewImageColorSpaceChanged);
    connect(currentImage.data(),
            &KisImage::sigProfileChanged,
            this,
            &KisCanvas2::sigViewImageProfileChanged);
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
