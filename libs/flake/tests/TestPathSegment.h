/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef TESTPATHSEGMENT_H
#define TESTPATHSEGMENT_H

#include <QObject>

class TestPathSegment : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void constructsCopiesAssignsAndOwnsDetachedEndpoints();
    void evaluatesStableLineQuadraticAndCubicGeometry();
    void transformsConvertsSplitsAndInterpolatesWithoutMutatingSource();
    void measuresLengthAndMapsArcParameters();
    void findsIntersectionsAndNearestParameters();
    void classifiesFlatnessAndReportsNormalizedDirection();
};

#endif // TESTPATHSEGMENT_H
