/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <memory>

#include <QImage>
#include <klocalizedstring.h>

void KisNodeManager::createReferenceImage(bool fromLayer)
{
    KisPaintDevice *const device =
        fromLayer ? ReferenceImageAccess::activeLayerProjection(this) : ReferenceImageAccess::visibleProjection(this);
    const QImage image = ReferenceImageAccess::convertToImage(device);
    std::unique_ptr<KisReferenceImage, void (*)(KisReferenceImage *)> reference(
        ReferenceImageAccess::createReferenceImage(this, image),
        &ReferenceImageAccess::deleteReferenceImage);

    if (reference) {
        const int existingReferenceCount = ReferenceImageAccess::referenceImageCount(this);
        if (existingReferenceCount >= 0) {
            ReferenceImageAccess::setZIndex(reference.get(), existingReferenceCount);
        }
        ReferenceImageAccess::addReferenceImage(this, reference.release());
        ReferenceImageAccess::switchTool(QStringLiteral("ToolReferenceImages"));
    } else if (ReferenceImageAccess::hasCanvasWidget(this)) {
        const QString message = fromLayer
            ? i18nc("error dialog from the reference tool", "Could not create a reference image from the active layer.")
            : i18nc("error dialog from the reference tool",
                    "Could not create a reference image from the visible canvas.");
        ReferenceImageAccess::showFloatingMessage(this, message, 5000, true, true);
    }
}

void KisNodeManager::createReferenceImageFromLayer()
{
    createReferenceImage(true);
}

void KisNodeManager::createReferenceImageFromVisible()
{
    createReferenceImage(false);
}
