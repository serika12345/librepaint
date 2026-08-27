/*
 *  SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_fast_math_test.h"

#include <cmath>

#include <QPointF>

#include "kis_fast_math.h"

void KisFastMathTest::testAtan2()
{
    const QList<QPointF> representativeCoordinates{
        QPointF(0.0, 0.0),
        QPointF(1.0, 0.0),
        QPointF(-1.0, 0.0),
        QPointF(0.0, 1.0),
        QPointF(0.0, -1.0),
        QPointF(3.0, 7.0),
        QPointF(-3.0, 7.0),
        QPointF(-3.0, -7.0),
        QPointF(3.0, -7.0),
    };

    for (const QPointF &coordinate : representativeCoordinates) {
        const qreal expected = std::atan2(coordinate.y(), coordinate.x());
        const qreal actual = KisFastMath::atan2(coordinate.y(), coordinate.x());
        QVERIFY(std::abs(expected - actual) < 0.0001);
    }

    const int COUNT = 1000;

    for (int i = 0; i < COUNT; ++i) {
        const double x = i;
        for (int j = 0; j < COUNT; ++j) {
            const double y = j;
            const double expected = std::atan2(y, x);
            const double actual = KisFastMath::atan2(y, x);

            QVERIFY(std::abs(expected - actual) < 0.0001);
        }
    }
}

QTEST_GUILESS_MAIN(KisFastMathTest)
