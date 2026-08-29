/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <QSizeF>
#include <QUrl>
#include <klocalizedstring.h>

void KisNodeManager::saveNodeAsImage()
{
    const KisNodeSP node = NodeExportAccess::activeNode(this);
    if (!node) {
        NodeExportAccess::reportNoActiveNode();
        return;
    }

    KisPaintDevice *const device = NodeExportAccess::projection(node);
    if (!device) {
        NodeExportAccess::showFloatingMessage(
            this,
            i18nc("warning message when trying to export a transform mask", "Layer has no pixel data"));
        return;
    }

    const QRect saveBounds = NodeExportAccess::imageBounds(this) | NodeExportAccess::nodeBounds(node);
    NodeExportAccess::saveDevice(this,
                                 device,
                                 NodeExportAccess::nodeName(node),
                                 saveBounds,
                                 NodeExportAccess::imageXResolution(this),
                                 NodeExportAccess::imageYResolution(this),
                                 NodeExportAccess::nodeOpacity(node));
}

void KisNodeManager::saveVectorLayerAsImage()
{
    KisShapeLayer *const layer = NodeExportAccess::shapeLayer(NodeExportAccess::activeNode(this));
    if (!layer) {
        return;
    }

    const QString filename = NodeExportAccess::chooseSvgFilename(this);
    if (filename.isEmpty() || QUrl::fromLocalFile(filename).isEmpty()) {
        return;
    }

    const QSizeF sizeInPixels = NodeExportAccess::imagePixelSize(this);
    const QSizeF sizeInPoints(sizeInPixels.width() / NodeExportAccess::imageXResolution(this),
                              sizeInPixels.height() / NodeExportAccess::imageYResolution(this));
    QList<KoShape *> shapes = NodeExportAccess::shapes(layer);
    NodeExportAccess::sortShapes(&shapes);
    if (!NodeExportAccess::saveSvg(filename, sizeInPoints, shapes)) {
        NodeExportAccess::showSvgFailure(filename);
    }
}
