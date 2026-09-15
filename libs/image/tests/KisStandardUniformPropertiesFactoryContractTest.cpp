/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/KisStandardUniformPropertyDefinition.h"
#include "brushengine/kis_standard_uniform_properties_factory.h"

#include <QTest>

#include <type_traits>

class KisStandardUniformPropertiesFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardFactoryEntryPointsSelectStableSliderDefinitions();
};

void KisStandardUniformPropertiesFactoryContractTest::standardFactoryEntryPointsSelectStableSliderDefinitions()
{
    using KoIdFactory =
        KisUniformPaintOpPropertySP (*)(const KoID &, KisPaintOpSettingsRestrictedSP, KisPaintOpPresetUpdateProxy *);
    using StringFactory =
        KisUniformPaintOpPropertySP (*)(const QString &, KisPaintOpSettingsRestrictedSP, KisPaintOpPresetUpdateProxy *);

    static_assert(
        std::is_same_v<decltype(static_cast<KoIdFactory>(&KisStandardUniformPropertiesFactory::createProperty)),
                       KoIdFactory>);
    static_assert(
        std::is_same_v<decltype(static_cast<StringFactory>(&KisStandardUniformPropertiesFactory::createProperty)),
                       StringFactory>);

    const auto size = standardUniformPropertyDefinition(QStringLiteral("size"));
    QVERIFY(size);
    QCOMPARE(size->kind, KisStandardUniformPropertyDefinition::Kind::Size);
    QCOMPARE(size->minimum, 0.0);
    QCOMPARE(size->singleStep, 1.0);
    QCOMPARE(size->exponentRatio, 3.0);
    QCOMPARE(size->decimals, 2);
    QVERIFY(size->usesBrushSizeMaximum);
    QVERIFY(size->usesPixelSuffix);

    const auto opacity = standardUniformPropertyDefinition(QStringLiteral("opacity"));
    QVERIFY(opacity);
    QCOMPARE(opacity->kind, KisStandardUniformPropertyDefinition::Kind::Opacity);
    QCOMPARE(opacity->minimum, 0.0);
    QCOMPARE(opacity->maximum, 1.0);
    QCOMPARE(opacity->singleStep, 0.01);
    QCOMPARE(opacity->exponentRatio, 1.0);
    QCOMPARE(opacity->decimals, 2);
    QVERIFY(!opacity->usesBrushSizeMaximum);
    QVERIFY(!opacity->usesPixelSuffix);

    const auto flow = standardUniformPropertyDefinition(QStringLiteral("flow"));
    QVERIFY(flow);
    QCOMPARE(flow->kind, KisStandardUniformPropertyDefinition::Kind::Flow);
    QCOMPARE(flow->minimum, 0.0);
    QCOMPARE(flow->maximum, 1.0);
    QCOMPARE(flow->singleStep, 0.01);
    QCOMPARE(flow->exponentRatio, 1.0);
    QCOMPARE(flow->decimals, 2);
    QVERIFY(!flow->usesBrushSizeMaximum);
    QVERIFY(!flow->usesPixelSuffix);

    QVERIFY(!standardUniformPropertyDefinition(QStringLiteral("angle")));
    QVERIFY(!standardUniformPropertyDefinition(QStringLiteral("spacing")));
    QVERIFY(!standardUniformPropertyDefinition(QStringLiteral("unknown")));
}

QTEST_APPLESS_MAIN(KisStandardUniformPropertiesFactoryContractTest)

#include "KisStandardUniformPropertiesFactoryContractTest.moc"
