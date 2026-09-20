/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_green_coordinates_math.h"

#include <QTest>

namespace
{

void comparePoint(const QPointF &actual, const QPointF &expected)
{
    constexpr qreal tolerance = 1e-9;
    QVERIFY(qAbs(actual.x() - expected.x()) < tolerance);
    QVERIFY(qAbs(actual.y() - expected.y()) < tolerance);
}

const QVector<QPointF> originalCage{QPointF(0.0, 0.0), QPointF(100.0, 0.0), QPointF(100.0, 100.0), QPointF(0.0, 100.0)};
const QVector<QPointF> originalPoints{QPointF(25.0, 40.0), QPointF(75.0, 60.0)};

} // namespace

class KisGreenCoordinatesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identityCageKeepsPointPositions();
    void translatedCageTranslatesPointPositions();
};

void KisGreenCoordinatesContractTest::identityCageKeepsPointPositions()
{
    KisGreenCoordinatesMath coordinates;
    coordinates.precalculateGreenCoordinates(originalCage, originalPoints);
    coordinates.generateTransformedCageNormals(originalCage);

    comparePoint(coordinates.transformedPoint(0, originalCage), originalPoints.at(0));
    comparePoint(coordinates.transformedPoint(1, originalCage), originalPoints.at(1));
}

void KisGreenCoordinatesContractTest::translatedCageTranslatesPointPositions()
{
    KisGreenCoordinatesMath coordinates;
    coordinates.precalculateGreenCoordinates(originalCage, originalPoints);
    const QPointF offset(12.5, -7.25);
    QVector<QPointF> translatedCage;
    translatedCage.reserve(originalCage.size());
    for (const QPointF &point : originalCage) {
        translatedCage.append(point + offset);
    }
    coordinates.generateTransformedCageNormals(translatedCage);

    comparePoint(coordinates.transformedPoint(0, translatedCage), originalPoints.at(0) + offset);
    comparePoint(coordinates.transformedPoint(1, translatedCage), originalPoints.at(1) + offset);
}

QTEST_GUILESS_MAIN(KisGreenCoordinatesContractTest)

#include "KisGreenCoordinatesContractTest.moc"
