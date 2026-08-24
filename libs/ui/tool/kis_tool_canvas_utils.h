/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_CANVAS_UTILS_H
#define KIS_TOOL_CANVAS_UTILS_H

#include <kritaui_export.h>
#include <QString>
#include <QtGlobal>

class KoCanvasBase;
class QPainterPath;
class QPointF;

namespace KisToolUtils
{
QPainterPath KRITAUI_EXPORT shapeHoverInfoCrossLayer(KoCanvasBase *canvas,
                                                     const QPointF &point,
                                                     QString &shapeType,
                                                     bool *isHorizontal = nullptr,
                                                     bool skipCurrentShapes = true);
bool KRITAUI_EXPORT selectShapeCrossLayer(KoCanvasBase *canvas,
                                          const QPointF &point,
                                          const QString &shapeType = QString(),
                                          bool skipCurrentShapes = true);
void KRITAUI_EXPORT showBrushSizeFloatingMessage(KoCanvasBase *canvas, qreal size);
}

#endif
