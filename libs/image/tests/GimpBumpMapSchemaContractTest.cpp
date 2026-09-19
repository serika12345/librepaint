/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/gimp_bump_map.h"

#include <QTest>


class GimpBumpMapSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bumpMapPlacementDefaultsRemainStable();
    void bumpMapLightingDefaultsRemainStable();
    void bumpMapFlagsAndTypeDefaultsRemainStable();
};

void GimpBumpMapSchemaContractTest::bumpMapPlacementDefaultsRemainStable()
{

    const bumpmap_vals_t values;
    QCOMPARE(values.bumpmap_id, 0);
    QCOMPARE(values.xofs, 0);
    QCOMPARE(values.yofs, 0);
    QCOMPARE(values.waterlevel, 0);
    QCOMPARE(values.tiled, false);
}

void GimpBumpMapSchemaContractTest::bumpMapLightingDefaultsRemainStable()
{

    const bumpmap_vals_t values;
    QCOMPARE(values.azimuth, 0.0);
    QCOMPARE(values.elevation, 30.0);
    QCOMPARE(values.depth, 50);
    QCOMPARE(values.ambient, 10);
}

void GimpBumpMapSchemaContractTest::bumpMapFlagsAndTypeDefaultsRemainStable()
{

    const bumpmap_vals_t values;
    QCOMPARE(values.compensate, true);
    QCOMPARE(values.invert, false);
    QCOMPARE(values.type, 0);
}

QTEST_APPLESS_MAIN(GimpBumpMapSchemaContractTest)

#include "GimpBumpMapSchemaContractTest.moc"
