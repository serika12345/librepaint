/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "PerspectiveBasedAssistantHelper.h"

#include <QTest>

#include <cmath>

namespace
{

constexpr qreal tolerance = 1e-9;

void compareValue(qreal actual, qreal expected)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance, "perspective value differs");
}

} // namespace

class PerspectiveBasedAssistantHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cacheDefaultsRepresentNoPerspective();
    void projectiveScaleAndPerpendicularProductUseFixedGeometry();
    void vanishingPointsPopulateCacheAndNormalizeDistance();
};

void PerspectiveBasedAssistantHelperContractTest::cacheDefaultsRepresentNoPerspective()
{
    PerspectiveBasedAssistantHelper::CacheData cache;

    QVERIFY(!cache.vanishingPoint1);
    QVERIFY(!cache.vanishingPoint2);
    QVERIFY(cache.distancesFromPoints.isEmpty());
    QCOMPARE(cache.maxDistanceFromPoint, 0.0);
    QCOMPARE(cache.horizon, QLineF());
    QVERIFY(cache.polygon.isEmpty());
    QCOMPARE(cache.type, PerspectiveBasedAssistantHelper::CacheData::None);
    QCOMPARE(int(PerspectiveBasedAssistantHelper::CacheData::None), 0);
    QCOMPARE(int(PerspectiveBasedAssistantHelper::CacheData::OneVp), 1);
    QCOMPARE(int(PerspectiveBasedAssistantHelper::CacheData::TwoVps), 2);
}

void PerspectiveBasedAssistantHelperContractTest::projectiveScaleAndPerpendicularProductUseFixedGeometry()
{
    QCOMPARE(PerspectiveBasedAssistantHelper::pdot(QPointF(3.0, 4.0), QPointF(5.0, 6.0)), -2.0);

    const QTransform identity;
    compareValue(PerspectiveBasedAssistantHelper::localScale(identity, QPointF(0.25, 0.75)), 1.0);
    compareValue(PerspectiveBasedAssistantHelper::inverseMaxLocalScale(identity), 1.0);

    const QTransform projective(1.0, 0.0, 0.1, 0.0, 1.0, 0.2, 0.0, 0.0, 1.0);
    compareValue(PerspectiveBasedAssistantHelper::localScale(projective, QPointF(0.0, 0.0)), 1.32);
    compareValue(PerspectiveBasedAssistantHelper::inverseMaxLocalScale(projective), 1.0 / 1.32);
}

void PerspectiveBasedAssistantHelperContractTest::vanishingPointsPopulateCacheAndNormalizeDistance()
{
    PerspectiveBasedAssistantHelper::CacheData cache;
    const QPolygonF rectangle{QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(4.0, 2.0), QPointF(0.0, 2.0)};

    boost::optional<QPointF> first;
    boost::optional<QPointF> second;
    QVERIFY(!PerspectiveBasedAssistantHelper::getVanishingPointsOptional(rectangle, first, second));
    PerspectiveBasedAssistantHelper::updateCacheData(cache, rectangle);
    QCOMPARE(cache.type, PerspectiveBasedAssistantHelper::CacheData::None);
    QCOMPARE(PerspectiveBasedAssistantHelper::distanceInGrid(cache, QPointF(2.0, 1.0)), 1.0);

    const QPolygonF onePoint{QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(3.0, 2.0), QPointF(1.0, 2.0)};
    PerspectiveBasedAssistantHelper::updateCacheData(cache, onePoint);
    QCOMPARE(cache.type, PerspectiveBasedAssistantHelper::CacheData::OneVp);
    QCOMPARE(cache.polygon, onePoint);
    QCOMPARE(cache.distancesFromPoints.size(), 4);
    compareValue(cache.maxDistanceFromPoint, 4.0);
    compareValue(PerspectiveBasedAssistantHelper::distanceInGrid(cache, QPointF(2.0, 2.0)), 0.5);

    const QPolygonF twoPoints{QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(3.0, 2.0), QPointF(0.0, 3.0)};
    PerspectiveBasedAssistantHelper::updateCacheData(cache, twoPoints);
    QCOMPARE(cache.type, PerspectiveBasedAssistantHelper::CacheData::TwoVps);
    QVERIFY(cache.vanishingPoint1);
    QVERIFY(cache.vanishingPoint2);
    QVERIFY(cache.maxDistanceFromPoint > 0.0);
    compareValue(PerspectiveBasedAssistantHelper::distanceInGrid(cache, cache.horizon.pointAt(0.5)), 0.0);
}

QTEST_GUILESS_MAIN(PerspectiveBasedAssistantHelperContractTest)

#include "PerspectiveBasedAssistantHelperContractTest.moc"
