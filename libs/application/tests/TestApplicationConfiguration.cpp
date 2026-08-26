/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QTest>

#include <application/kis_config.h>
#include <application/kis_snap_config.h>

class TestApplicationConfiguration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapDefaultsRemainStable();
};

void TestApplicationConfiguration::snapDefaultsRemainStable()
{
    KisSnapConfig snapConfig(false);
    snapConfig.setOrthogonal(true);
    snapConfig.setNode(true);
    snapConfig.setExtension(true);
    snapConfig.setIntersection(true);
    snapConfig.setBoundingBox(true);
    snapConfig.setImageBounds(false);
    snapConfig.setImageCenter(true);
    snapConfig.setToPixel(true);

    KisConfig config(true);
    config.loadSnapConfig(&snapConfig, true);

    QVERIFY(!snapConfig.orthogonal());
    QVERIFY(!snapConfig.node());
    QVERIFY(!snapConfig.extension());
    QVERIFY(!snapConfig.intersection());
    QVERIFY(!snapConfig.boundingBox());
    QVERIFY(snapConfig.imageBounds());
    QVERIFY(!snapConfig.imageCenter());
    QVERIFY(!snapConfig.toPixel());
}

QTEST_MAIN(TestApplicationConfiguration)

#include "TestApplicationConfiguration.moc"
