/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "layerstyles/gimp_bump_map.h"

#include <QTest>

#include <type_traits>

class GimpBumpMapSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bumpMapModeValuesRemainStable();
    void bumpMapPlacementDefaultsRemainStable();
    void bumpMapLightingDefaultsRemainStable();
    void bumpMapFlagsAndTypeDefaultsRemainStable();
    void bumpMapFunctionSignatureRemainsStable();
};

void GimpBumpMapSchemaContractTest::bumpMapModeValuesRemainStable()
{
    static_assert(std::is_enum_v<BumpmapType>);
    static_assert(LINEAR == 0);
    static_assert(SPHERICAL == 1);
    static_assert(SINUSOIDAL == 2);
}

void GimpBumpMapSchemaContractTest::bumpMapPlacementDefaultsRemainStable()
{
    static_assert(std::is_class_v<bumpmap_vals_t>);
    static_assert(std::is_default_constructible_v<bumpmap_vals_t>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::bumpmap_id), const qint32>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::xofs), const int>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::yofs), const int>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::waterlevel), const int>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::tiled), const bool>);

    const bumpmap_vals_t values;
    QCOMPARE(values.bumpmap_id, 0);
    QCOMPARE(values.xofs, 0);
    QCOMPARE(values.yofs, 0);
    QCOMPARE(values.waterlevel, 0);
    QCOMPARE(values.tiled, false);
}

void GimpBumpMapSchemaContractTest::bumpMapLightingDefaultsRemainStable()
{
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::azimuth), double>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::elevation), double>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::depth), int>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::ambient), int>);

    const bumpmap_vals_t values;
    QCOMPARE(values.azimuth, 0.0);
    QCOMPARE(values.elevation, 30.0);
    QCOMPARE(values.depth, 50);
    QCOMPARE(values.ambient, 10);
}

void GimpBumpMapSchemaContractTest::bumpMapFlagsAndTypeDefaultsRemainStable()
{
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::compensate), bool>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::invert), bool>);
    static_assert(std::is_same_v<decltype(bumpmap_vals_t::type), int>);

    const bumpmap_vals_t values;
    QCOMPARE(values.compensate, true);
    QCOMPARE(values.invert, false);
    QCOMPARE(values.type, 0);
}

void GimpBumpMapSchemaContractTest::bumpMapFunctionSignatureRemainsStable()
{
    using BumpMapSignature = void (*)(KisPixelSelectionSP, const QRect &, const bumpmap_vals_t &);

    static_assert(std::is_same_v<decltype(static_cast<BumpMapSignature>(&bumpmap)), BumpMapSignature>);
}

QTEST_APPLESS_MAIN(GimpBumpMapSchemaContractTest)

#include "GimpBumpMapSchemaContractTest.moc"
