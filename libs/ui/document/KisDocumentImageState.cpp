/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "document/KisDocument.h"

#include <kis_image.h>
#include <kis_image_animation_interface.h>
#include <kis_composite_progress_proxy.h>
#include <KisImageBarrierLock.h>
#include <kis_memory_statistics_server.h>

bool KisDocument::hasImage() const
{
    return bool(image());
}

QString KisDocument::imageObjectName() const
{
    KisImageWSP documentImage = image();
    return documentImage ? documentImage->objectName() : QString();
}

qint64 KisDocument::imageMemorySize() const
{
    KisImageSP documentImage = image();
    return KisMemoryStatisticsServer::instance()
        ->fetchMemoryStatistics(documentImage)
        .imageSize;
}

void KisDocument::connectImageMemoryStatisticsUpdates(QObject *receiver, const char *method) const
{
    QObject::connect(KisMemoryStatisticsServer::instance(),
                     SIGNAL(sigUpdateMemoryStatistics()),
                     receiver,
                     method);
}

void KisDocument::addImageProgressProxy(KoProgressProxy *proxy) const
{
    image()->compositeProgressProxy()->addProxy(proxy);
}

void KisDocument::removeImageProgressProxy(KoProgressProxy *proxy) const
{
    image()->compositeProgressProxy()->removeProxy(proxy);
}

KisUndoAdapter *KisDocument::imageUndoAdapter() const
{
    KisImageWSP documentImage = image();
    Q_ASSERT(documentImage);
    return documentImage->undoAdapter();
}

KisDocument *KisDocument::cloneWithImageReadLock()
{
    KisImageReadOnlyBarrierLock lock(image());
    return clone(true);
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
