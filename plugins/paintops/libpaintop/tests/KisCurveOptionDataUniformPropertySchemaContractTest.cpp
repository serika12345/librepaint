/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCurveOptionDataUniformProperty.h>

#include <QTest>

#include <type_traits>

class KisCurveOptionDataUniformPropertySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curveOptionDataUniformPropertySchemaRemainStable();
};

void KisCurveOptionDataUniformPropertySchemaContractTest::curveOptionDataUniformPropertySchemaRemainStable()
{
    using Property = KisCurveOptionDataUniformProperty;

    static_assert(std::is_class_v<Property>);
    static_assert(std::is_base_of_v<KisDoubleSliderBasedPaintOpProperty, Property>);
    static_assert(
        std::is_constructible_v<Property, const KisCurveOptionData &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<Property,
                                          const KisCurveOptionData &,
                                          const QString &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(std::is_destructible_v<Property>);
    static_assert(std::is_same_v<decltype(&Property::readValueImpl), void (Property::*)()>);
    static_assert(std::is_same_v<decltype(&Property::writeValueImpl), void (Property::*)()>);
    static_assert(std::is_same_v<decltype(&Property::isVisible), bool (Property::*)() const>);
}

QTEST_GUILESS_MAIN(KisCurveOptionDataUniformPropertySchemaContractTest)

#include "KisCurveOptionDataUniformPropertySchemaContractTest.moc"
