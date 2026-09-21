/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image.h"
#include "kis_node_manager.h"
#include "kis_types.h"
#include <qobject.h>

void KisNodeManager::createFromVisible()
{
    KisImage *const currentImage = LayerCreationAccess::image(this);
    LayerCreationAccess::createFromVisible(currentImage, LayerCreationAccess::rootLastChild(currentImage));
}

KisLayerSP KisNodeManager::createPaintLayer()
{
    return LayerCreationAccess::createPaintLayer(this, QStringLiteral("KisPaintLayer"));
}
