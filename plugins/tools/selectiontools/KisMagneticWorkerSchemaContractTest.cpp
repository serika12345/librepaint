/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMagneticWorker.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_MAGNETIC_LAZY_TILES_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMagneticLazyTiles::method)), signature>)
#define ASSERT_MAGNETIC_WORKER_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMagneticWorker::method)), signature>)
} // namespace

class KisMagneticWorkerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void magneticLazyTilesTypeAndFilterSchemaRemainStable();
    void magneticLazyTilesAccessorsAndWorkerConstructionSchemaRemainStable();
    void magneticWorkerSearchAndOutputSchemaRemainStable();
};

void KisMagneticWorkerSchemaContractTest::magneticLazyTilesTypeAndFilterSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMagneticLazyTiles>);
    static_assert(std::is_constructible_v<KisMagneticLazyTiles, KisPaintDeviceSP>);
    ASSERT_MAGNETIC_LAZY_TILES_SIGNATURE(filter, void (KisMagneticLazyTiles::*)(qreal, QRect &));

    QVERIFY(true);
}

void KisMagneticWorkerSchemaContractTest::magneticLazyTilesAccessorsAndWorkerConstructionSchemaRemainStable()
{
    ASSERT_MAGNETIC_LAZY_TILES_SIGNATURE(device, KisPaintDeviceSP (KisMagneticLazyTiles::*)());
    ASSERT_MAGNETIC_LAZY_TILES_SIGNATURE(tiles, QVector<QRect> (KisMagneticLazyTiles::*)());
    static_assert(std::is_class_v<KisMagneticWorker>);
    static_assert(std::is_constructible_v<KisMagneticWorker, const KisPaintDeviceSP &>);

    QVERIFY(true);
}

void KisMagneticWorkerSchemaContractTest::magneticWorkerSearchAndOutputSchemaRemainStable()
{
    ASSERT_MAGNETIC_WORKER_SIGNATURE(computeEdge, QVector<QPointF> (KisMagneticWorker::*)(int, QPoint, QPoint, qreal));
    ASSERT_MAGNETIC_WORKER_SIGNATURE(intensity, qreal (KisMagneticWorker::*)(QPoint));
    ASSERT_MAGNETIC_WORKER_SIGNATURE(saveTheImage, void (KisMagneticWorker::*)(vQPointF));

    QVERIFY(true);
}

#undef ASSERT_MAGNETIC_LAZY_TILES_SIGNATURE
#undef ASSERT_MAGNETIC_WORKER_SIGNATURE

QTEST_APPLESS_MAIN(KisMagneticWorkerSchemaContractTest)

#include "KisMagneticWorkerSchemaContractTest.moc"
