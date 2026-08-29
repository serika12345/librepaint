/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2026 LibrePaint contributors

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoCurveFitPathShapeWriter_p.h"

#include <KoPathShape.h>

namespace KoCurveFitPathShapeWriter
{

KoPathShape *createPath()
{
    return new KoPathShape();
}

void moveTo(KoPathShape *path, const QPointF &point)
{
    path->moveTo(point);
}

void curveTo(KoPathShape *path, const QPointF &controlPoint1, const QPointF &controlPoint2, const QPointF &endPoint)
{
    path->curveTo(controlPoint1, controlPoint2, endPoint);
}

} // namespace KoCurveFitPathShapeWriter
