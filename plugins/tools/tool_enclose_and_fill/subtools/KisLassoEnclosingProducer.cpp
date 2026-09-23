/*
 * KDE. Krita Project.
 *
 * SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpoint.h>
#include <QtGlobal>

#include "KisLassoEnclosingProducer.h"
#include "KisQStringListFwd.h"
#include "KisToolOutlineBase.h"
#include "kis_cursor.h"
#include "kis_painter.h"
#include "kis_pixel_selection.h"
#include "kis_types.h"
#include "subtools/KisDynamicDelegatedTool.h"

KisLassoEnclosingProducer::KisLassoEnclosingProducer(KoCanvasBase * canvas)
    : KisDynamicDelegateTool<KisToolOutlineBase>(canvas, KisToolOutlineBase::PAINT, KisCursor::loadWithSize("tool_outline_selection_cursor.svg", 32, 32, 5, 5))
{
    setObjectName("enclosing_tool_lasso");
    setSupportOutline(true);
    setOutlineEnabled(false);

    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas);

    connect(kritaCanvas->viewManager()->canvasResourceProvider(), SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(resetCursorStyle()));
}

KisLassoEnclosingProducer::~KisLassoEnclosingProducer()
{}

void  KisLassoEnclosingProducer::resetCursorStyle()
{
    if (isEraser()) {
        useCursor(KisCursor::loadWithSize("tool_outline_selection_enclose_eraser_cursor.svg", 32, 32, 5, 5));
    } else {
        KisDynamicDelegateTool::resetCursorStyle();
    }

    overrideCursorIfNotEditable();
}

void KisLassoEnclosingProducer::finishOutline(const QVector<QPointF> &points)
{
    if (points.size() < 3) {
        return;
    }
    
    KisPixelSelectionSP enclosingMask = new KisPixelSelection();

    KisPainter painter(enclosingMask);
    painter.setPaintColor(KoColor(Qt::white, enclosingMask->colorSpace()));
    painter.setAntiAliasPolygonFill(false);
    painter.setFillStyle(KisPainter::FillStyleForegroundColor);
    painter.setStrokeStyle(KisPainter::StrokeStyleNone);

    painter.paintPolygon(points);

    Q_EMIT enclosingMaskProduced(enclosingMask);
}

bool KisLassoEnclosingProducer::hasUserInteractionRunning() const
{
    return m_hasUserInteractionRunning;
}

void KisLassoEnclosingProducer::beginShape()
{
    m_hasUserInteractionRunning = true;
}

void KisLassoEnclosingProducer::endShape()
{
    m_hasUserInteractionRunning = false;
}
