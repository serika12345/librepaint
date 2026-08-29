/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathPointData.h"

#include <QTest>

#include <array>
#include <functional>
#include <type_traits>

namespace
{
using ShapeStorage = std::aligned_storage_t<sizeof(KoPathShape), alignof(KoPathShape)>;

KoPathShape *shapePointer(ShapeStorage &storage)
{
    return reinterpret_cast<KoPathShape *>(&storage);
}
} // namespace

class KoPathPointDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesShapeAndPointIndex();
    void equalityRequiresMatchingShapeAndBothIndices();
    void orderingUsesShapeThenLexicographicPointIndex();
};

void KoPathPointDataContractTest::constructionPreservesShapeAndPointIndex()
{
    ShapeStorage storage;
    KoPathShape *const shape = shapePointer(storage);
    const KoPathPointIndex pointIndex(3, 7);

    const KoPathPointData data(shape, pointIndex);

    QCOMPARE(data.pathShape, shape);
    QCOMPARE(data.pointIndex, pointIndex);
}

void KoPathPointDataContractTest::equalityRequiresMatchingShapeAndBothIndices()
{
    std::array<ShapeStorage, 2> storage;
    KoPathShape *const firstShape = shapePointer(storage[0]);
    KoPathShape *const secondShape = shapePointer(storage[1]);
    const KoPathPointData data(firstShape, KoPathPointIndex(2, 5));

    QVERIFY(data == KoPathPointData(firstShape, KoPathPointIndex(2, 5)));
    QVERIFY(!(data == KoPathPointData(secondShape, KoPathPointIndex(2, 5))));
    QVERIFY(!(data == KoPathPointData(firstShape, KoPathPointIndex(3, 5))));
    QVERIFY(!(data == KoPathPointData(firstShape, KoPathPointIndex(2, 6))));
}

void KoPathPointDataContractTest::orderingUsesShapeThenLexicographicPointIndex()
{
    std::array<ShapeStorage, 2> storage;
    KoPathShape *const firstShape = shapePointer(storage[0]);
    KoPathShape *const secondShape = shapePointer(storage[1]);
    const std::less<KoPathShape *> less;
    KoPathShape *const lowerShape = less(firstShape, secondShape) ? firstShape : secondShape;
    KoPathShape *const higherShape = lowerShape == firstShape ? secondShape : firstShape;

    const KoPathPointData lowerShapeData(lowerShape, KoPathPointIndex(9, 9));
    const KoPathPointData higherShapeData(higherShape, KoPathPointIndex(0, 0));
    QVERIFY(lowerShapeData < higherShapeData);
    QVERIFY(!(higherShapeData < lowerShapeData));

    const KoPathPointData lowerSubpath(firstShape, KoPathPointIndex(1, 9));
    const KoPathPointData higherSubpath(firstShape, KoPathPointIndex(2, 0));
    QVERIFY(lowerSubpath < higherSubpath);
    QVERIFY(!(higherSubpath < lowerSubpath));

    const KoPathPointData lowerPoint(firstShape, KoPathPointIndex(2, 3));
    const KoPathPointData higherPoint(firstShape, KoPathPointIndex(2, 4));
    QVERIFY(lowerPoint < higherPoint);
    QVERIFY(!(higherPoint < lowerPoint));
    QVERIFY(!(lowerPoint < KoPathPointData(firstShape, KoPathPointIndex(2, 3))));
}

QTEST_GUILESS_MAIN(KoPathPointDataContractTest)

#include "KoPathPointDataContractTest.moc"
