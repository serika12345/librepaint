/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_VEC_TEST_H
#define KIS_VEC_TEST_H

#include <QObject>

class KisVecTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCreation();
    void pointsConvertToVectorsWithoutChangingCoordinates();
    void vectorExpressionsConvertToPoints();
};

#endif
