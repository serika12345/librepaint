/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisBrushModel.h>

#include <QGuiApplication>
#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

template<typename Data, typename Mutator>
void verifyIndependentMember(const Data &baseline, Mutator mutate)
{
    Data copy = baseline;
    QVERIFY(copy == baseline);

    mutate(copy);
    QVERIFY(copy != baseline);

    copy = baseline;
    QVERIFY(copy == baseline);
}

} // namespace

class KisBrushModelValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesAreStable();
    void commonDataDefaultsAndMembersAreIndependent();
    void autoBrushGeneratorDefaultsAndMembersAreIndependent();
    void autoBrushDefaultsAndNestedGeneratorAreIndependent();
    void textBrushDefaultsAndMembersAreIndependent();
    void predefinedBrushResourceAndSizingSchemaRemainsStable();
    void predefinedBrushAdjustmentSchemaRemainsStable();
    void brushAggregateSchemaRemainsStable();
    void brushPersistenceSignaturesRemainStable();
    void brushSizingAndPolicySignaturesRemainStable();
};

void KisBrushModelValuesContractTest::enumValuesAreStable()
{
    QCOMPARE(int(KisBrushModel::Circle), 0);
    QCOMPARE(int(KisBrushModel::Rectangle), 1);

    QCOMPARE(int(KisBrushModel::Default), 0);
    QCOMPARE(int(KisBrushModel::Soft), 1);
    QCOMPARE(int(KisBrushModel::Gaussian), 2);

    QCOMPARE(int(KisBrushModel::Auto), 0);
    QCOMPARE(int(KisBrushModel::Predefined), 1);
    QCOMPARE(int(KisBrushModel::Text), 2);
}

void KisBrushModelValuesContractTest::commonDataDefaultsAndMembersAreIndependent()
{
    const KisBrushModel::CommonData baseline;
    QCOMPARE(baseline.angle, 0.0);
    QCOMPARE(baseline.spacing, 0.05);
    QCOMPARE(baseline.useAutoSpacing, false);
    QCOMPARE(baseline.autoSpacingCoeff, 1.0);

    verifyIndependentMember(baseline, [](auto &data) {
        data.angle = 0.25;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.spacing = 0.5;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.useAutoSpacing = true;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.autoSpacingCoeff = 2.0;
    });
}

void KisBrushModelValuesContractTest::autoBrushGeneratorDefaultsAndMembersAreIndependent()
{
    const KisBrushModel::AutoBrushGeneratorData baseline;
    QCOMPARE(baseline.diameter, 42.0);
    QCOMPARE(baseline.ratio, 1.0);
    QCOMPARE(baseline.horizontalFade, 1.0);
    QCOMPARE(baseline.verticalFade, 1.0);
    QCOMPARE(baseline.spikes, 2);
    QCOMPARE(baseline.antialiasEdges, true);
    QCOMPARE(baseline.shape, KisBrushModel::Circle);
    QCOMPARE(baseline.type, KisBrushModel::Default);
    QVERIFY(baseline.curveString.isEmpty());

    verifyIndependentMember(baseline, [](auto &data) {
        data.diameter = 84.0;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.ratio = 0.5;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.horizontalFade = 0.25;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.verticalFade = 0.75;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.spikes = 7;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.antialiasEdges = false;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.shape = KisBrushModel::Rectangle;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.type = KisBrushModel::Gaussian;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.curveString = QStringLiteral("curve");
    });
}

void KisBrushModelValuesContractTest::autoBrushDefaultsAndNestedGeneratorAreIndependent()
{
    const KisBrushModel::AutoBrushData baseline;
    QCOMPARE(baseline.randomness, 0.0);
    QCOMPARE(baseline.density, 1.0);
    QCOMPARE(baseline.generator, KisBrushModel::AutoBrushGeneratorData());

    verifyIndependentMember(baseline, [](auto &data) {
        data.randomness = 0.5;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.density = 0.25;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.generator.diameter = 12.0;
    });
}

void KisBrushModelValuesContractTest::textBrushDefaultsAndMembersAreIndependent()
{
    const KisBrushModel::TextBrushData baseline;
    QCOMPARE(baseline.baseSize, QSize(42, 42));
    QCOMPARE(baseline.scale, 1.0);
    QCOMPARE(baseline.text, QStringLiteral("The quick brown fox ate your text"));
    QCOMPARE(baseline.font, QGuiApplication::font().toString());
    QCOMPARE(baseline.usePipeMode, false);

    verifyIndependentMember(baseline, [](auto &data) {
        data.baseSize = QSize(21, 84);
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.scale = 2.0;
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.text = QStringLiteral("replacement");
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.font = QStringLiteral("replacement-font");
    });
    verifyIndependentMember(baseline, [](auto &data) {
        data.usePipeMode = true;
    });
}

void KisBrushModelValuesContractTest::predefinedBrushResourceAndSizingSchemaRemainsStable()
{
    using Data = KisBrushModel::PredefinedBrushData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_same_v<decltype(&Data::resourceSignature), KoResourceSignature Data::*>);
    static_assert(std::is_same_v<decltype(&Data::subtype), QString Data::*>);
    static_assert(std::is_same_v<decltype(&Data::baseSize), QSize Data::*>);
    static_assert(std::is_same_v<decltype(&Data::scale), qreal Data::*>);
    static_assert(std::is_same_v<decltype(&Data::application), enumBrushApplication Data::*>);
    static_assert(std::is_same_v<decltype(&Data::brushType), enumBrushType Data::*>);
}

void KisBrushModelValuesContractTest::predefinedBrushAdjustmentSchemaRemainsStable()
{
    using Data = KisBrushModel::PredefinedBrushData;
    static_assert(std::is_same_v<decltype(&Data::hasColorAndTransparency), bool Data::*>);
    static_assert(std::is_same_v<decltype(&Data::autoAdjustMidPoint), bool Data::*>);
    static_assert(std::is_same_v<decltype(&Data::adjustmentMidPoint), quint8 Data::*>);
    static_assert(std::is_same_v<decltype(&Data::brightnessAdjustment), qreal Data::*>);
    static_assert(std::is_same_v<decltype(&Data::contrastAdjustment), qreal Data::*>);
    static_assert(std::is_same_v<decltype(&Data::parasiteSelection), QString Data::*>);
    static_assert(std::is_same_v<decltype(std::declval<const Data &>() == std::declval<const Data &>()), bool>);
}

void KisBrushModelValuesContractTest::brushAggregateSchemaRemainsStable()
{
    using Data = KisBrushModel::BrushData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_same_v<decltype(&Data::common), KisBrushModel::CommonData Data::*>);
    static_assert(std::is_same_v<decltype(&Data::type), KisBrushModel::BrushType Data::*>);
    static_assert(std::is_same_v<decltype(&Data::autoBrush), KisBrushModel::AutoBrushData Data::*>);
    static_assert(std::is_same_v<decltype(&Data::predefinedBrush), KisBrushModel::PredefinedBrushData Data::*>);
    static_assert(std::is_same_v<decltype(&Data::textBrush), KisBrushModel::TextBrushData Data::*>);
}

void KisBrushModelValuesContractTest::brushPersistenceSignaturesRemainStable()
{
    using Data = KisBrushModel::BrushData;
    using Read = std::optional<Data> (*)(const KisPropertiesConfiguration *, KisResourcesInterfaceSP);

    static_assert(std::is_same_v<decltype(&Data::write), void (Data::*)(KisPropertiesConfiguration *) const>);
    static_assert(std::is_same_v<decltype(&Data::read), Read>);
}

void KisBrushModelValuesContractTest::brushSizingAndPolicySignaturesRemainStable()
{
    using Data = KisBrushModel::BrushData;
    using AutoData = KisBrushModel::AutoBrushData;
    using PredefinedData = KisBrushModel::PredefinedBrushData;
    using TextData = KisBrushModel::TextBrushData;
    using BrushType = KisBrushModel::BrushType;

    using LodLimitations = KisPaintopLodLimitations (*)(const Data &);
    using EffectiveSize = qreal (*)(BrushType, const AutoData &, const PredefinedData &, const TextData &);
    using AggregateEffectiveSize = qreal (*)(const Data &);
    using SetEffectiveSize = void (*)(BrushType, AutoData &, PredefinedData &, TextData &, qreal);
    using LightnessMode = qreal (*)(BrushType, const PredefinedData &);

    static_assert(std::is_same_v<decltype(&KisBrushModel::brushLodLimitations), LodLimitations>);
    static_assert(
        std::is_same_v<decltype(static_cast<EffectiveSize>(&KisBrushModel::effectiveSizeForBrush)), EffectiveSize>);
    static_assert(std::is_same_v<decltype(static_cast<AggregateEffectiveSize>(&KisBrushModel::effectiveSizeForBrush)),
                                 AggregateEffectiveSize>);
    static_assert(std::is_same_v<decltype(&KisBrushModel::setEffectiveSizeForBrush), SetEffectiveSize>);
    static_assert(std::is_same_v<decltype(&KisBrushModel::lightnessModeActivated), LightnessMode>);
    static_assert(std::is_same_v<decltype(std::declval<const Data &>() == std::declval<const Data &>()), bool>);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisBrushModelValuesContractTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "KisBrushModelValuesContractTest.moc"
