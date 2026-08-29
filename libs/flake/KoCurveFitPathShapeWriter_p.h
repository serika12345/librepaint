/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2026 LibrePaint contributors

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOCURVEFITPATHSHAPEWRITER_P_H
#define KOCURVEFITPATHSHAPEWRITER_P_H

class KoPathShape;
class QPointF;

namespace KoCurveFitPathShapeWriter
{

KoPathShape *createPath();
void moveTo(KoPathShape *path, const QPointF &point);
void curveTo(KoPathShape *path, const QPointF &controlPoint1, const QPointF &controlPoint2, const QPointF &endPoint);

} // namespace KoCurveFitPathShapeWriter

#endif
