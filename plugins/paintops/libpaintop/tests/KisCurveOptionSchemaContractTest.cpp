/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCurveOption.h"
#include "KisStandardOptionData.h"
#include "KisStandardOptions.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_CURVE_OPTION_SIGNATURE(function, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCurveOption::function)), signature>)
#define ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(function)                                                      \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<KisCurveOptionWidget *(*)()>(&KisPaintOpOptionWidgetUtils::function)),     \
                       KisCurveOptionWidget *(*)()>)

template<template<typename> class Option>
struct StandardOptionTemplateProbe {
};

} // namespace

class KisCurveOptionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curveOptionOwnershipAndPolicySchemaRemainsStable();
    void curveOptionValueComponentStorageRemainsStable();
    void curveOptionValueCompositionSignaturesRemainStable();
    void curveOptionStrengthSignaturesRemainStable();
    void curveOptionStatePolicySignaturesRemainStable();
    void standardOptionTypesAndAliasesSchemaRemainStable();
    void standardOptionConstructionAndEvaluationSignaturesRemainStable();
    void standardOptionCoreWidgetFactorySignaturesRemainStable();
    void standardOptionColorAndStrengthWidgetFactorySignaturesRemainStable();
    void standardOptionMaskingWidgetFactorySignaturesRemainStable();
};

void KisCurveOptionSchemaContractTest::curveOptionOwnershipAndPolicySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisCurveOption>);
    static_assert(std::is_class_v<KisCurveOption::ValueComponents>);
    static_assert(std::is_constructible_v<KisCurveOption, const KisCurveOptionData &>);
    static_assert(!std::is_constructible_v<KisCurveOption, KisCurveOption &>);
    static_assert(!std::is_assignable_v<KisCurveOption &, KisCurveOption &>);
}

void KisCurveOptionSchemaContractTest::curveOptionValueComponentStorageRemainsStable()
{
    using Components = KisCurveOption::ValueComponents;

    static_assert(std::is_same_v<decltype(Components::constant), qreal>);
    static_assert(std::is_same_v<decltype(Components::scaling), qreal>);
    static_assert(std::is_same_v<decltype(Components::additive), qreal>);
    static_assert(std::is_same_v<decltype(Components::absoluteOffset), qreal>);
    static_assert(std::is_same_v<decltype(Components::hasAbsoluteOffset), bool>);
    static_assert(std::is_same_v<decltype(Components::hasScaling), bool>);
    static_assert(std::is_same_v<decltype(Components::hasAdditive), bool>);
    static_assert(std::is_same_v<decltype(Components::minSizeLikeValue), qreal>);
    static_assert(std::is_same_v<decltype(Components::maxSizeLikeValue), qreal>);

    const Components defaults;
    QCOMPARE(defaults.constant, 1.0);
    QCOMPARE(defaults.scaling, 1.0);
    QCOMPARE(defaults.additive, 0.0);
    QCOMPARE(defaults.absoluteOffset, 0.0);
    QVERIFY(!defaults.hasAbsoluteOffset);
    QVERIFY(!defaults.hasScaling);
    QVERIFY(!defaults.hasAdditive);
    QCOMPARE(defaults.minSizeLikeValue, 0.0);
    QCOMPARE(defaults.maxSizeLikeValue, 0.0);
}

void KisCurveOptionSchemaContractTest::curveOptionValueCompositionSignaturesRemainStable()
{
    using Components = KisCurveOption::ValueComponents;
    using RotationValueSignature = qreal (Components::*)(qreal, bool, qreal, bool) const;
    using SizeValueSignature = qreal (Components::*)() const;
    using ComputeComponentsSignature = Components (KisCurveOption::*)(const KisPaintInformation &, bool) const;
    using ComputeSizeSignature = qreal (KisCurveOption::*)(const KisPaintInformation &, bool) const;
    using ComputeRotationSignature =
        qreal (KisCurveOption::*)(const KisPaintInformation &, qreal, bool, qreal, bool) const;

    static_assert(std::is_same_v<decltype(static_cast<RotationValueSignature>(&Components::rotationLikeValue)),
                                 RotationValueSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SizeValueSignature>(&Components::sizeLikeValue)), SizeValueSignature>);
    ASSERT_CURVE_OPTION_SIGNATURE(computeValueComponents, ComputeComponentsSignature);
    ASSERT_CURVE_OPTION_SIGNATURE(computeSizeLikeValue, ComputeSizeSignature);
    ASSERT_CURVE_OPTION_SIGNATURE(computeRotationLikeValue, ComputeRotationSignature);
    static_assert(std::is_same_v<decltype(std::declval<const KisCurveOption &>().computeSizeLikeValue(
                                     std::declval<const KisPaintInformation &>())),
                                 qreal>);
}

void KisCurveOptionSchemaContractTest::curveOptionStrengthSignaturesRemainStable()
{
    using GetterSignature = qreal (KisCurveOption::*)() const;

    ASSERT_CURVE_OPTION_SIGNATURE(strengthValue, GetterSignature);
    ASSERT_CURVE_OPTION_SIGNATURE(strengthMinValue, GetterSignature);
    ASSERT_CURVE_OPTION_SIGNATURE(strengthMaxValue, GetterSignature);
}

void KisCurveOptionSchemaContractTest::curveOptionStatePolicySignaturesRemainStable()
{
    using GetterSignature = bool (KisCurveOption::*)() const;

    ASSERT_CURVE_OPTION_SIGNATURE(isChecked, GetterSignature);
    ASSERT_CURVE_OPTION_SIGNATURE(isRandom, GetterSignature);
}

void KisCurveOptionSchemaContractTest::standardOptionTypesAndAliasesSchemaRemainStable()
{
    static_assert(std::is_class_v<StandardOptionTemplateProbe<KisStandardOption>>);
    static_assert(std::is_class_v<StandardOptionTemplateProbe<KisStandardOptionNoApply>>);
    static_assert(std::is_same_v<KisFlowOption, KisStandardOption<KisFlowOptionData>>);
    static_assert(std::is_same_v<KisLightnessStrengthOption, KisStandardOption<KisLightnessStrengthOptionData>>);
    static_assert(std::is_same_v<KisMixOption, KisStandardOption<KisMixOptionData>>);
    static_assert(std::is_same_v<KisRateOption, KisStandardOption<KisRateOptionData>>);
    static_assert(std::is_same_v<KisRatioOption, KisStandardOption<KisRatioOptionData>>);
    static_assert(std::is_same_v<KisSizeOption, KisStandardOption<KisSizeOptionData>>);
    static_assert(std::is_same_v<KisSoftnessOption, KisStandardOption<KisSoftnessOptionData>>);
    static_assert(std::is_same_v<KisStrengthOption, KisStandardOption<KisStrengthOptionData>>);
}

void KisCurveOptionSchemaContractTest::standardOptionConstructionAndEvaluationSignaturesRemainStable()
{
    using StandardOption = KisStandardOption<KisFlowOptionData>;
    using StandardOptionNoApply = KisStandardOptionNoApply<KisFlowOptionData>;
    using ApplySignature = qreal (StandardOption::*)(const KisPaintInformation &) const;

    static_assert(std::is_constructible_v<StandardOption, const KisPropertiesConfiguration *>);
    static_assert(std::is_same_v<decltype(static_cast<ApplySignature>(&StandardOption::apply)), ApplySignature>);
    static_assert(std::is_constructible_v<StandardOptionNoApply, const KisPropertiesConfiguration *>);
}

void KisCurveOptionSchemaContractTest::standardOptionCoreWidgetFactorySignaturesRemainStable()
{
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createFlowOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMixOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createOpacityOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createRateOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createRatioOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createRotationOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createSoftnessOptionWidget);
}

void KisCurveOptionSchemaContractTest::standardOptionColorAndStrengthWidgetFactorySignaturesRemainStable()
{
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createDarkenOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createHueOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createSaturationOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createStrengthOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createValueOptionWidget);
}

void KisCurveOptionSchemaContractTest::standardOptionMaskingWidgetFactorySignaturesRemainStable()
{
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingFlowOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingMirrorOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingOpacityOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingRatioOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingRotationOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingScatterOptionWidget);
    ASSERT_STANDARD_OPTION_WIDGET_FACTORY_SIGNATURE(createMaskingSizeOptionWidget);
}

QTEST_GUILESS_MAIN(KisCurveOptionSchemaContractTest)

#include "KisCurveOptionSchemaContractTest.moc"
