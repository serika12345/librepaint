/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_tool_ellipse_base.h"

#include <QPainterPath>
#include <KoPointerEvent.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoViewConverter.h>


KisToolEllipseBase::KisToolEllipseBase(KoCanvasBase * canvas, KisToolEllipseBase::ToolType type, const QCursor & cursor)
    : KisToolRectangleBase(canvas, type, cursor)
{
}

void KisToolEllipseBase::paintRectangle(QPainter &gc, const QRectF &imageRect)
{
    KIS_ASSERT_RECOVER_RETURN(canvas());

    QRect viewRect = pixelToView(imageRect).toRect();

    QPainterPath path;
    path.addEllipse(viewRect);
    getRotatedPath(path, viewRect.center(), getRotationAngle());
    path.addPath(drawX(pixelToView(dragStart())));
    path.addPath(drawX(pixelToView(dragCenter())));
    paintToolOutline(&gc, path);
}

bool KisToolEllipseBase::showRoundCornersGUI() const
{
    return false;
}
