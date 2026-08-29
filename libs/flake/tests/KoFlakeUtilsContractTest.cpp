/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoFlakeUtils.h"

#include <QHash>
#include <QTest>

#include <array>
#include <type_traits>

namespace
{
using ShapeStorage = std::aligned_storage_t<sizeof(KoShape), alignof(KoShape)>;

KoShape *shapePointer(ShapeStorage &storage)
{
    return reinterpret_cast<KoShape *>(&storage);
}

struct Property {
    int value;
};

struct PropertyPolicy {
    using PointerType = const Property *;

    PointerType getProperty(KoShape *shape) const
    {
        ++fetchCount;
        return properties.value(shape);
    }

    bool compareTo(PointerType first, PointerType second) const
    {
        return first->value == second->value;
    }

    QHash<KoShape *, PointerType> properties;
    mutable int fetchCount = 0;
};

struct DefaultPropertyPolicy {
    using PointerType = const Property *;

    DefaultPropertyPolicy()
    {
        ++constructionCount;
    }

    PointerType getProperty(KoShape *shape) const
    {
        return shape == firstShape ? &firstProperty : &secondProperty;
    }

    bool compareTo(PointerType first, PointerType second) const
    {
        return first->value == second->value;
    }

    static void reset(KoShape *first, KoShape *second, int firstValue, int secondValue)
    {
        firstShape = first;
        secondShape = second;
        firstProperty.value = firstValue;
        secondProperty.value = secondValue;
        constructionCount = 0;
    }

    inline static KoShape *firstShape = nullptr;
    inline static KoShape *secondShape = nullptr;
    inline static Property firstProperty{0};
    inline static Property secondProperty{0};
    inline static int constructionCount = 0;
};
} // namespace

class KoFlakeUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyAndSingleShapePropertiesAreEqualWithoutFetching();
    void multipleShapePropertiesRespectNullAndValueEquality();
    void defaultPolicyEntryConstructsAndUsesThePolicy();
    void emptyStrokeModificationReturnsNullWithoutCallingModifier();
};

void KoFlakeUtilsContractTest::emptyAndSingleShapePropertiesAreEqualWithoutFetching()
{
    ShapeStorage storage;
    KoShape *const shape = shapePointer(storage);
    PropertyPolicy policy;

    QVERIFY(KoFlake::compareShapePropertiesEqual(QList<KoShape *>(), policy));
    QVERIFY(KoFlake::compareShapePropertiesEqual(QList<KoShape *>{shape}, policy));
    QCOMPARE(policy.fetchCount, 0);
}

void KoFlakeUtilsContractTest::multipleShapePropertiesRespectNullAndValueEquality()
{
    std::array<ShapeStorage, 3> storage;
    KoShape *const firstShape = shapePointer(storage[0]);
    KoShape *const secondShape = shapePointer(storage[1]);
    KoShape *const thirdShape = shapePointer(storage[2]);
    const QList<KoShape *> shapes{firstShape, secondShape, thirdShape};
    const Property firstProperty{7};
    const Property equalProperty{7};
    const Property differentProperty{9};
    PropertyPolicy policy;

    QVERIFY(KoFlake::compareShapePropertiesEqual(shapes, policy));

    policy.properties.insert(firstShape, &firstProperty);
    QVERIFY(!KoFlake::compareShapePropertiesEqual(shapes, policy));

    policy.properties.clear();
    policy.properties.insert(secondShape, &equalProperty);
    QVERIFY(!KoFlake::compareShapePropertiesEqual(shapes, policy));

    policy.properties.insert(firstShape, &firstProperty);
    policy.properties.insert(thirdShape, &equalProperty);
    QVERIFY(KoFlake::compareShapePropertiesEqual(shapes, policy));

    policy.properties.insert(thirdShape, &differentProperty);
    QVERIFY(!KoFlake::compareShapePropertiesEqual(shapes, policy));
}

void KoFlakeUtilsContractTest::defaultPolicyEntryConstructsAndUsesThePolicy()
{
    std::array<ShapeStorage, 2> storage;
    KoShape *const firstShape = shapePointer(storage[0]);
    KoShape *const secondShape = shapePointer(storage[1]);
    const QList<KoShape *> shapes{firstShape, secondShape};

    DefaultPropertyPolicy::reset(firstShape, secondShape, 11, 11);
    QVERIFY(KoFlake::compareShapePropertiesEqual<DefaultPropertyPolicy>(shapes));
    QCOMPARE(DefaultPropertyPolicy::constructionCount, 1);

    DefaultPropertyPolicy::reset(firstShape, secondShape, 11, 12);
    QVERIFY(!KoFlake::compareShapePropertiesEqual<DefaultPropertyPolicy>(shapes));
    QCOMPARE(DefaultPropertyPolicy::constructionCount, 1);
}

void KoFlakeUtilsContractTest::emptyStrokeModificationReturnsNullWithoutCallingModifier()
{
    int modificationCount = 0;
    KUndo2Command *const command =
        KoFlake::modifyShapesStrokes(QList<KoShape *>(), [&modificationCount](KoShapeStrokeSP) {
            ++modificationCount;
        });

    QCOMPARE(command, nullptr);
    QCOMPARE(modificationCount, 0);
}

QTEST_GUILESS_MAIN(KoFlakeUtilsContractTest)

#include "KoFlakeUtilsContractTest.moc"
