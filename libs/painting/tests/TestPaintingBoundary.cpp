/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KoColorSpaceRegistry.h>
#include <KisStrokeSpeedMonitor.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_resources_snapshot.h>

class TestPaintingBoundary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapshotOwnsStrokeState();
    void measurementStateIsSetByTheCaller();
};

void TestPaintingBoundary::snapshotOwnsStrokeState()
{
    KisImageSP image = new KisImage(nullptr,
                                    64,
                                    48,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("painting-boundary"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("paint"), OPACITY_OPAQUE_U8);
    image->addNode(layer);

    KisResourcesSnapshot snapshot(image, layer);

    QCOMPARE(snapshot.image().data(), image.data());
    QCOMPARE(snapshot.currentNode().data(), layer.data());
    QCOMPARE(snapshot.selectedNodes(), KisNodeList());
    QCOMPARE(snapshot.opacity(), OPACITY_OPAQUE_F);

    snapshot.setOpacity(0.25);
    QCOMPARE(snapshot.opacity(), 0.25);
}

void TestPaintingBoundary::measurementStateIsSetByTheCaller()
{
    KisStrokeSpeedMonitor *monitor = KisStrokeSpeedMonitor::instance();

    monitor->setHaveStrokeSpeedMeasurement(false);
    QVERIFY(!monitor->haveStrokeSpeedMeasurement());

    monitor->setHaveStrokeSpeedMeasurement(true);
    QVERIFY(monitor->haveStrokeSpeedMeasurement());

    monitor->setHaveStrokeSpeedMeasurement(false);
}

QTEST_MAIN(TestPaintingBoundary)

#include "TestPaintingBoundary.moc"
