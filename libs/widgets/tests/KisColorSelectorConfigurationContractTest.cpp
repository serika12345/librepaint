/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSelectorConfiguration.h"

#include <QTest>

class KisColorSelectorConfigurationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesRemainStable();
    void defaultsAndRoundTripPreserveFields();
    void stringConstructionAndFactoryHandleInput();
};

void KisColorSelectorConfigurationContractTest::enumValuesRemainStable()
{
    using Configuration = KisColorSelectorConfiguration;

    QCOMPARE(int(Configuration::Ring), 0);
    QCOMPARE(int(Configuration::Square), 1);
    QCOMPARE(int(Configuration::Wheel), 2);
    QCOMPARE(int(Configuration::Triangle), 3);
    QCOMPARE(int(Configuration::Slider), 4);

    QCOMPARE(int(Configuration::H), 0);
    QCOMPARE(int(Configuration::hsvS), 1);
    QCOMPARE(int(Configuration::V), 2);
    QCOMPARE(int(Configuration::hslS), 3);
    QCOMPARE(int(Configuration::L), 4);
    QCOMPARE(int(Configuration::SL), 5);
    QCOMPARE(int(Configuration::SV), 6);
    QCOMPARE(int(Configuration::SV2), 7);
    QCOMPARE(int(Configuration::hsvSH), 8);
    QCOMPARE(int(Configuration::hslSH), 9);
    QCOMPARE(int(Configuration::VH), 10);
    QCOMPARE(int(Configuration::LH), 11);
    QCOMPARE(int(Configuration::SI), 12);
    QCOMPARE(int(Configuration::SY), 13);
    QCOMPARE(int(Configuration::hsiSH), 14);
    QCOMPARE(int(Configuration::hsySH), 15);
    QCOMPARE(int(Configuration::I), 16);
    QCOMPARE(int(Configuration::Y), 17);
    QCOMPARE(int(Configuration::IH), 18);
    QCOMPARE(int(Configuration::YH), 19);
    QCOMPARE(int(Configuration::hsiS), 20);
    QCOMPARE(int(Configuration::hsyS), 21);
    QCOMPARE(int(Configuration::Hluma), 22);
}

void KisColorSelectorConfigurationContractTest::defaultsAndRoundTripPreserveFields()
{
    using Configuration = KisColorSelectorConfiguration;

    const Configuration defaults;
    QCOMPARE(defaults.mainType, Configuration::Triangle);
    QCOMPARE(defaults.subType, Configuration::Ring);
    QCOMPARE(defaults.mainTypeParameter, Configuration::SL);
    QCOMPARE(defaults.subTypeParameter, Configuration::H);
    QCOMPARE(defaults.toString(), QStringLiteral("3|0|5|0"));

    const Configuration configured(Configuration::Square,
                                   Configuration::Slider,
                                   Configuration::YH,
                                   Configuration::hsyS);
    QCOMPARE(configured.toString(), QStringLiteral("1|4|19|21"));
    QCOMPARE(Configuration(configured.toString()), configured);
    QVERIFY(!(defaults == configured));
}

void KisColorSelectorConfigurationContractTest::stringConstructionAndFactoryHandleInput()
{
    using Configuration = KisColorSelectorConfiguration;

    const Configuration constructed(QStringLiteral("4|3|22|21"));
    QCOMPARE(constructed.mainType, Configuration::Slider);
    QCOMPARE(constructed.subType, Configuration::Triangle);
    QCOMPARE(constructed.mainTypeParameter, Configuration::Hluma);
    QCOMPARE(constructed.subTypeParameter, Configuration::hsyS);

    QCOMPARE(Configuration::fromString(QStringLiteral("1|4|19|21")),
             Configuration(Configuration::Square,
                           Configuration::Slider,
                           Configuration::YH,
                           Configuration::hsyS));
    QCOMPARE(Configuration(QStringLiteral("invalid")), Configuration());
    QCOMPARE(Configuration::fromString(QStringLiteral("-1|0|0|0")), Configuration());

    Configuration updated;
    updated.readString(QStringLiteral("0|2|16|17"));
    QCOMPARE(updated, Configuration(Configuration::Ring,
                                    Configuration::Wheel,
                                    Configuration::I,
                                    Configuration::Y));

    const Configuration beforeInvalidInput = updated;
    updated.readString(QStringLiteral("5|0|0|0"));
    QCOMPARE(updated, beforeInvalidInput);
    updated.readString(QStringLiteral("1|2|3"));
    QCOMPARE(updated, beforeInvalidInput);
    updated.readString(QStringLiteral("abc|0|0|0"));
    QCOMPARE(updated, beforeInvalidInput);
    updated.readString(QStringLiteral("0|0|-1|0"));
    QCOMPARE(updated, beforeInvalidInput);
}

QTEST_GUILESS_MAIN(KisColorSelectorConfigurationContractTest)

#include "KisColorSelectorConfigurationContractTest.moc"
