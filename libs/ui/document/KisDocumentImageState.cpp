/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "document/KisDocument.h"

#include <kis_image.h>
#include <kis_image_animation_interface.h>

bool KisDocument::hasImage() const
{
    return bool(image());
}

QString KisDocument::imageObjectName() const
{
    KisImageWSP documentImage = image();
    return documentImage ? documentImage->objectName() : QString();
}

int KisDocument::animationLength() const
{
    KisImageWSP documentImage = image();
    if (!documentImage) {
        return 0;
    }

    return documentImage->animationInterface()->totalLength();
}

void KisDocument::setAnimationRange(int firstFrame, int lastFrame)
{
    KisImageWSP documentImage = image();
    if (!documentImage) {
        return;
    }

    KisImageAnimationInterface *animation = documentImage->animationInterface();
    animation->setDocumentRangeStartFrame(firstFrame);
    animation->setDocumentRangeEndFrame(lastFrame);
}

void KisDocument::setAnimationTiming(int frameRate, int firstFrame, int lastFrame)
{
    KisImageWSP documentImage = image();
    if (!documentImage) {
        return;
    }

    KisImageAnimationInterface *animation = documentImage->animationInterface();
    animation->setFramerate(frameRate);
    animation->setDocumentRangeStartFrame(firstFrame);
    animation->setDocumentRangeEndFrame(lastFrame);
}

void KisDocument::refreshProjectionAndWait()
{
    KisImageWSP documentImage = image();
    if (!documentImage) {
        return;
    }

    documentImage->refreshGraphAsync();
    documentImage->waitForDone();
}
