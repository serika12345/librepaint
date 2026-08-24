/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_canvas2.h"

#include <QPainter>

#include <KisOptimizedBrushOutline.h>
#include <KisViewManager.h>
#include <kis_config_notifier.h>
#include <kis_icon.h>
#include <kis_node_manager.h>

#include "opengl/kis_opengl_canvas2.h"

KisNodeList KisCanvas2::selectedNodesForTool() const
{
    return viewManager()->nodeManager()->selectedNodes();
}

bool KisCanvas2::blockUntilOperationsFinishedForTool(KisImageSP image)
{
    return viewManager()->blockUntilOperationsFinished(image);
}

void KisCanvas2::blockUntilOperationsFinishedForToolForced(KisImageSP image)
{
    viewManager()->blockUntilOperationsFinishedForced(image);
}

bool KisCanvas2::selectionEditableForTool() const
{
    return viewManager()->selectionEditable();
}

void KisCanvas2::showToolMessage(const QString &message, const QString &iconName)
{
    viewManager()->showFloatingMessage(message, KisIconUtils::loadIcon(iconName));
}

void KisCanvas2::drawToolOutline(QPainter *painter,
                                 const KisOptimizedBrushOutline &path,
                                 int thickness)
{
    KisOpenGLCanvas2 *openGLCanvas = dynamic_cast<KisOpenGLCanvas2 *>(canvasWidget());
    if (openGLCanvas) {
        painter->beginNativePainting();
        openGLCanvas->paintToolOutline(path, thickness);
        painter->endNativePainting();
        return;
    }

    painter->save();
    painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    QPen pen = QColor(128, 255, 128);
    pen.setCosmetic(true);
    pen.setWidth(thickness);
    painter->setPen(pen);
    for (auto it = path.begin(); it != path.end(); ++it) {
        painter->drawPolyline(*it);
    }
    painter->restore();
}

QObject *KisCanvas2::toolConfigNotifier() const
{
    return KisConfigNotifier::instance();
}
