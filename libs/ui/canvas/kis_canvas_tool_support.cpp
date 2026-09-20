/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_canvas2.h"

#include <QPainter>

#include <KisOptimizedBrushOutline.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/kis_config.h>
#include <kis_config_notifier.h>
#include <kis_cursor.h>
#include <kis_icon.h>
#include <kis_image.h>
#include <nodes/kis_node_manager.h>
#include <kis_selection.h>

#include "opengl/kis_opengl_canvas2.h"

KisSelectionSP KisCanvas2::currentSelectionForTool() const
{
    return viewManager()->selection();
}

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

bool KisCanvas2::selectionModifierMappingSwapsCtrlAndAltForTool() const
{
    return KisConfig(true).switchSelectionCtrlAlt();
}

QCursor KisCanvas2::moveSelectionCursorForTool() const
{
    return KisCursor::moveSelectionCursor();
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

void KisCanvas2::setInputEventFilterConnection(std::function<void(QObject *, bool, int)> connection)
{
    m_inputEventFilterConnection = std::move(connection);
}

void KisCanvas2::attachPriorityEventFilterForTool(QObject *filter, int priority)
{
    if (m_inputEventFilterConnection) {
        m_inputEventFilterConnection(filter, true, priority);
    }
}

void KisCanvas2::detachPriorityEventFilterForTool(QObject *filter)
{
    if (m_inputEventFilterConnection) {
        m_inputEventFilterConnection(filter, false, 0);
    }
}

void KisCanvas2::requestStrokeEndForTool()
{
    image()->requestStrokeEnd();
}

void KisCanvas2::requestStrokeCancellationForTool()
{
    image()->requestStrokeCancellation();
}

void KisCanvas2::setInputCanvasWidgetChangedCallback(std::function<void()> callback)
{
    m_inputCanvasWidgetChangedCallback = std::move(callback);
}
