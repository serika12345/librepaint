/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_canvas_utils.h"

#include <QIcon>
#include <QPainterPath>

#include <klocalizedstring.h>

#include <KoCanvasBase.h>
#include <KoPathShape.h>
#include <KoSelection.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoSvgTextProperties.h>
#include <KoSvgTextShape.h>

#include <application/ui/workspace/KisViewManager.h>
#include <kis_assert.h>
#include <kis_floating_message.h>
#include <kis_layer_utils.h>
#include <nodes/kis_node_manager.h>
#include <kis_shape_layer.h>

#include "kis_canvas2.h"

namespace KisToolUtils
{
namespace
{
QList<KisShapeLayerSP> findShapeLayers(KisNodeSP root, const QPointF &point, bool editableOnly)
{
    QList<KisShapeLayerSP> foundNodes;
    KisLayerUtils::recursiveApplyNodes(root, [&](KisNodeSP node) {
        if ((node->isEditable(true) && editableOnly) || !editableOnly) {
            KisShapeLayerSP shapeLayer = dynamic_cast<KisShapeLayer *>(node.data());
            if (shapeLayer && shapeLayer->isEditable() && shapeLayer->shapeManager()->shapeAt(point)) {
                foundNodes.append(shapeLayer);
            }
        }
    });
    return foundNodes;
}
}

void showBrushSizeFloatingMessage(KoCanvasBase *canvas, qreal size)
{
    KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2 *>(canvas);
    KIS_SAFE_ASSERT_RECOVER_RETURN(kisCanvas);
    kisCanvas->viewManager()->showFloatingMessage(i18n("Brush Size: %1 px", size),
                                                  QIcon(),
                                                  1000,
                                                  KisFloatingMessage::High,
                                                  Qt::AlignLeft | Qt::TextWordWrap | Qt::AlignVCenter);
}

QPainterPath shapeHoverInfoCrossLayer(KoCanvasBase *canvas,
                                      const QPointF &point,
                                      QString &shapeType,
                                      bool *isHorizontal,
                                      bool skipCurrentShapes)
{
    QPainterPath result;
    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas);
    if (!canvas2) {
        return result;
    }

    const QList<KoShape *> currentShapes = canvas->shapeManager()->selection()->selectedShapes();
    const QList<KisShapeLayerSP> candidates = findShapeLayers(canvas2->image()->root(), point, true);
    KisShapeLayerSP shapeLayer = candidates.isEmpty() ? nullptr : candidates.last();

    if (!shapeLayer) {
        return result;
    }

    KoShape *shape = shapeLayer->shapeManager()->shapeAt(point);
    if (!shape || (currentShapes.contains(shape) && skipCurrentShapes)) {
        return result;
    }

    shapeType = shape->shapeId();
    KoSvgTextShape *textShape = dynamic_cast<KoSvgTextShape *>(shape);
    if (!textShape) {
        return shape->absoluteTransformation().map(shape->outline());
    }

    result.addRect(textShape->boundingRect());
    if (isHorizontal) {
        *isHorizontal = textShape->writingMode() == KoSvgText::HorizontalTB;
    }

    QPainterPath innerPaths;
    Q_FOREACH (KoShape *innerShape, textShape->shapesInside()) {
        KoPathShape *path = dynamic_cast<KoPathShape *>(innerShape);
        if (path) {
            innerPaths.addPath(path->absoluteTransformation().map(path->outline()));
        }
    }
    return innerPaths.isEmpty() ? result : innerPaths;
}

bool selectShapeCrossLayer(KoCanvasBase *canvas,
                           const QPointF &point,
                           const QString &shapeType,
                           bool skipCurrentShapes)
{
    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas);
    if (!canvas2) {
        return false;
    }

    const QList<KoShape *> currentShapes = canvas->shapeManager()->selection()->selectedShapes();
    const QList<KisShapeLayerSP> candidates = findShapeLayers(canvas2->image()->root(), point, true);
    KisShapeLayerSP shapeLayer = candidates.isEmpty() ? nullptr : candidates.last();
    if (!shapeLayer) {
        return false;
    }

    KoShape *shape = shapeLayer->shapeManager()->shapeAt(point);
    if (!shape || (currentShapes.contains(shape) && skipCurrentShapes) ||
        (!shapeType.isEmpty() && shapeType != shape->shapeId())) {
        return false;
    }

    canvas2->viewManager()->nodeManager()->slotNonUiActivatedNode(shapeLayer);
    canvas2->shapeManager()->selection()->deselectAll();
    canvas2->shapeManager()->selection()->select(shape);
    return true;
}
}
