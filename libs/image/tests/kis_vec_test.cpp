/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_vec_test.h"

#include "kis_vec.h"
#include <QTest>

void KisVecTest::testCreation()
{
    const KisVector2D vector = KisVector2D::Zero();

    QCOMPARE(vector.x(), 0.0);
    QCOMPARE(vector.y(), 0.0);
}

void KisVecTest::pointsConvertToVectorsWithoutChangingCoordinates()
{
    const KisVector2D integerVector = toKisVector2D(QPoint(-3, 4));
    const KisVector2D floatingVector = toKisVector2D(QPointF(1.25, -2.5));

    QCOMPARE(integerVector.x(), -3.0);
    QCOMPARE(integerVector.y(), 4.0);
    QCOMPARE(floatingVector.x(), 1.25);
    QCOMPARE(floatingVector.y(), -2.5);
}

void KisVecTest::vectorExpressionsConvertToPoints()
{
    const KisVector2D vector(3.75, -2.125);

    const QPointF point = toQPointF(vector * 2.0);

    QCOMPARE(point, QPointF(7.5, -4.25));
}

QTEST_GUILESS_MAIN(KisVecTest)
