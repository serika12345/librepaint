/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgText.h"

#include <QTest>

#include <type_traits>

namespace
{

using Axis = KoSvgText::FontFamilyAxis;
using EastAsian = KoSvgText::FontFeatureEastAsian;
using Ligatures = KoSvgText::FontFeatureLigatures;
using Numeric = KoSvgText::FontFeatureNumeric;
using Style = KoSvgText::FontFamilyStyleInfo;

} // namespace

class KoSvgTextFontSelectionValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontFamilyAxesPreserveDefaultsFactoriesAndOwnedValues();
    void fontFamilyStylesOwnLabelsCoordinatesAndFlags();
    void ligaturesGenerateFeaturesFromIndependentFlags();
    void numericVariantsGenerateFeaturesInStableOrder();
    void eastAsianVariantsGenerateFeaturesInStableOrder();
};

void KoSvgTextFontSelectionValueContractTest::fontFamilyAxesPreserveDefaultsFactoriesAndOwnedValues()
{
    static_assert(std::is_default_constructible_v<Axis>);
    static_assert(std::is_copy_constructible_v<Axis>);
    static_assert(std::is_copy_assignable_v<Axis>);
    static_assert(std::is_same_v<decltype(Axis::tag), QString>);
    static_assert(std::is_same_v<decltype(Axis::localizedLabels), QHash<QLocale, QString>>);
    static_assert(std::is_same_v<decltype(Axis::min), qreal>);
    static_assert(std::is_same_v<decltype(Axis::max), qreal>);
    static_assert(std::is_same_v<decltype(Axis::value), qreal>);
    static_assert(std::is_same_v<decltype(Axis::defaultValue), qreal>);
    static_assert(std::is_same_v<decltype(Axis::variableAxis), bool>);
    static_assert(std::is_same_v<decltype(Axis::axisHidden), bool>);

    Axis defaults;
    QVERIFY(defaults.tag.isEmpty());
    QVERIFY(defaults.localizedLabels.isEmpty());
    QCOMPARE(defaults.min, -1.0);
    QCOMPARE(defaults.max, -1.0);
    QCOMPARE(defaults.value, 0.0);
    QCOMPARE(defaults.defaultValue, 0.0);
    QVERIFY(!defaults.variableAxis);
    QVERIFY(!defaults.axisHidden);

    Axis parameterized(QStringLiteral("opsz"), 17.5);
    QCOMPARE(parameterized.tag, QStringLiteral("opsz"));
    QCOMPARE(parameterized.value, 17.5);
    QCOMPARE(parameterized.min, -1.0);
    QCOMPARE(parameterized.max, -1.0);
    QCOMPARE(parameterized.defaultValue, 0.0);
    QVERIFY(!parameterized.variableAxis);
    QVERIFY(!parameterized.axisHidden);

    const Axis weight = Axis::weightAxis(725.0);
    QCOMPARE(weight.tag, QStringLiteral("wght"));
    QCOMPARE(weight.min, 725.0);
    QCOMPARE(weight.max, 725.0);
    QCOMPARE(weight.value, 725.0);
    QCOMPARE(weight.defaultValue, 400.0);

    const Axis width = Axis::widthAxis(87.5);
    QCOMPARE(width.tag, QStringLiteral("wdth"));
    QCOMPARE(width.min, 87.5);
    QCOMPARE(width.max, 87.5);
    QCOMPARE(width.value, 87.5);
    QCOMPARE(width.defaultValue, 100.0);

    const Axis slant = Axis::slantAxis(-12.0);
    QCOMPARE(slant.tag, QStringLiteral("slnt"));
    QCOMPARE(slant.min, -12.0);
    QCOMPARE(slant.max, -12.0);
    QCOMPARE(slant.value, -12.0);
    QCOMPARE(slant.defaultValue, 0.0);

    parameterized.localizedLabels.insert(QLocale(QLocale::English), QStringLiteral("Optical size"));
    parameterized.variableAxis = true;
    parameterized.axisHidden = true;
    const Axis copied(parameterized);
    Axis assigned;
    assigned = parameterized;

    parameterized.tag = QStringLiteral("changed");
    parameterized.localizedLabels[QLocale(QLocale::English)] = QStringLiteral("Changed");
    parameterized.value = -23.0;
    parameterized.variableAxis = false;
    parameterized.axisHidden = false;

    QCOMPARE(copied.tag, QStringLiteral("opsz"));
    QCOMPARE(copied.localizedLabels.value(QLocale(QLocale::English)), QStringLiteral("Optical size"));
    QCOMPARE(copied.value, 17.5);
    QVERIFY(copied.variableAxis);
    QVERIFY(copied.axisHidden);
    QCOMPARE(assigned.tag, QStringLiteral("opsz"));
    QCOMPARE(assigned.localizedLabels.value(QLocale(QLocale::English)), QStringLiteral("Optical size"));
    QCOMPARE(assigned.value, 17.5);
    QVERIFY(assigned.variableAxis);
    QVERIFY(assigned.axisHidden);
}

void KoSvgTextFontSelectionValueContractTest::fontFamilyStylesOwnLabelsCoordinatesAndFlags()
{
    static_assert(std::is_default_constructible_v<Style>);
    static_assert(std::is_copy_constructible_v<Style>);
    static_assert(std::is_copy_assignable_v<Style>);
    static_assert(std::is_same_v<decltype(Style::localizedLabels), QHash<QLocale, QString>>);
    static_assert(std::is_same_v<decltype(Style::instanceCoords), QHash<QString, float>>);
    static_assert(std::is_same_v<decltype(Style::isItalic), bool>);
    static_assert(std::is_same_v<decltype(Style::isOblique), bool>);

    Style source;
    QVERIFY(source.localizedLabels.isEmpty());
    QVERIFY(source.instanceCoords.isEmpty());
    QVERIFY(!source.isItalic);
    QVERIFY(!source.isOblique);

    source.localizedLabels.insert(QLocale(QLocale::English), QStringLiteral("Display Italic"));
    source.instanceCoords.insert(QStringLiteral("wght"), 625.0f);
    source.instanceCoords.insert(QStringLiteral("wdth"), 91.0f);
    source.isItalic = true;
    source.isOblique = true;

    const Style copied(source);
    Style assigned;
    assigned = source;

    source.localizedLabels[QLocale(QLocale::English)] = QStringLiteral("Changed");
    source.instanceCoords[QStringLiteral("wght")] = -1.0f;
    source.instanceCoords.remove(QStringLiteral("wdth"));
    source.isItalic = false;
    source.isOblique = false;

    QCOMPARE(copied.localizedLabels.value(QLocale(QLocale::English)), QStringLiteral("Display Italic"));
    QCOMPARE(copied.instanceCoords.value(QStringLiteral("wght")), 625.0f);
    QCOMPARE(copied.instanceCoords.value(QStringLiteral("wdth")), 91.0f);
    QVERIFY(copied.isItalic);
    QVERIFY(copied.isOblique);
    QCOMPARE(assigned.localizedLabels.value(QLocale(QLocale::English)), QStringLiteral("Display Italic"));
    QCOMPARE(assigned.instanceCoords.value(QStringLiteral("wght")), 625.0f);
    QCOMPARE(assigned.instanceCoords.value(QStringLiteral("wdth")), 91.0f);
    QVERIFY(assigned.isItalic);
    QVERIFY(assigned.isOblique);
}

void KoSvgTextFontSelectionValueContractTest::ligaturesGenerateFeaturesFromIndependentFlags()
{
    static_assert(std::is_default_constructible_v<Ligatures>);
    static_assert(std::is_same_v<decltype(Ligatures::commonLigatures), bool>);
    static_assert(std::is_same_v<decltype(Ligatures::discretionaryLigatures), bool>);
    static_assert(std::is_same_v<decltype(Ligatures::historicalLigatures), bool>);
    static_assert(std::is_same_v<decltype(Ligatures::contextualAlternates), bool>);

    Ligatures defaults;
    QVERIFY(defaults.commonLigatures);
    QVERIFY(!defaults.discretionaryLigatures);
    QVERIFY(!defaults.historicalLigatures);
    QVERIFY(defaults.contextualAlternates);
    QVERIFY(defaults.fontFeatures(3, 11).isEmpty());

    const Ligatures anotherDefault;
    QVERIFY(defaults == anotherDefault);

    Ligatures features;
    features.commonLigatures = false;
    features.discretionaryLigatures = true;
    features.historicalLigatures = true;
    features.contextualAlternates = false;

    const QStringList expected = {
        QStringLiteral("clig[3:11]=0"),
        QStringLiteral("liga[3:11]=0"),
        QStringLiteral("dlig[3:11]=1"),
        QStringLiteral("hlig[3:11]=1"),
        QStringLiteral("calt[3:11]=0"),
    };
    QCOMPARE(features.fontFeatures(3, 11), expected);
    QVERIFY(!(features == defaults));
    QVERIFY(!features.commonLigatures);
    QVERIFY(features.discretionaryLigatures);
    QVERIFY(features.historicalLigatures);
    QVERIFY(!features.contextualAlternates);
}

void KoSvgTextFontSelectionValueContractTest::numericVariantsGenerateFeaturesInStableOrder()
{
    static_assert(std::is_default_constructible_v<Numeric>);
    static_assert(std::is_same_v<decltype(Numeric::style), KoSvgText::NumericFigureStyle>);
    static_assert(std::is_same_v<decltype(Numeric::spacing), KoSvgText::NumericFigureSpacing>);
    static_assert(std::is_same_v<decltype(Numeric::fractions), KoSvgText::NumericFractions>);
    static_assert(std::is_same_v<decltype(Numeric::ordinals), bool>);
    static_assert(std::is_same_v<decltype(Numeric::slashedZero), bool>);

    Numeric defaults;
    QCOMPARE(int(defaults.style), int(KoSvgText::NumericFigureStyleNormal));
    QCOMPARE(int(defaults.spacing), int(KoSvgText::NumericFigureSpacingNormal));
    QCOMPARE(int(defaults.fractions), int(KoSvgText::NumericFractionsNormal));
    QVERIFY(!defaults.ordinals);
    QVERIFY(!defaults.slashedZero);
    QVERIFY(defaults.fontFeatures(5, 17).isEmpty());

    const Numeric anotherDefault;
    QVERIFY(defaults == anotherDefault);

    Numeric oldTabularStacked;
    oldTabularStacked.style = KoSvgText::NumericFigureStyleOld;
    oldTabularStacked.spacing = KoSvgText::NumericFigureSpacingTabular;
    oldTabularStacked.fractions = KoSvgText::NumericFractionsStacked;
    oldTabularStacked.ordinals = true;
    oldTabularStacked.slashedZero = true;
    const QStringList oldExpected = {
        QStringLiteral("onum[5:17]=1"),
        QStringLiteral("tnum[5:17]=1"),
        QStringLiteral("afrc[5:17]=1"),
        QStringLiteral("ordn[5:17]=1"),
        QStringLiteral("zero[5:17]=1"),
    };
    QCOMPARE(oldTabularStacked.fontFeatures(5, 17), oldExpected);
    QVERIFY(!(oldTabularStacked == defaults));

    Numeric liningProportionalDiagonal;
    liningProportionalDiagonal.style = KoSvgText::NumericFigureStyleLining;
    liningProportionalDiagonal.spacing = KoSvgText::NumericFigureSpacingProportional;
    liningProportionalDiagonal.fractions = KoSvgText::NumericFractionsDiagonal;
    const QStringList liningExpected = {
        QStringLiteral("lnum[5:17]=1"),
        QStringLiteral("pnum[5:17]=1"),
        QStringLiteral("frac[5:17]=1"),
    };
    QCOMPARE(liningProportionalDiagonal.fontFeatures(5, 17), liningExpected);
}

void KoSvgTextFontSelectionValueContractTest::eastAsianVariantsGenerateFeaturesInStableOrder()
{
    static_assert(std::is_default_constructible_v<EastAsian>);
    static_assert(std::is_same_v<decltype(EastAsian::variant), KoSvgText::EastAsianVariant>);
    static_assert(std::is_same_v<decltype(EastAsian::width), KoSvgText::EastAsianWidth>);
    static_assert(std::is_same_v<decltype(EastAsian::ruby), bool>);

    EastAsian defaults;
    QCOMPARE(int(defaults.variant), int(KoSvgText::EastAsianVariantNormal));
    QCOMPARE(int(defaults.width), int(KoSvgText::EastAsiantNormalWidth));
    QVERIFY(!defaults.ruby);
    QVERIFY(defaults.fontFeatures(7, 19).isEmpty());

    const EastAsian anotherDefault;
    QVERIFY(defaults == anotherDefault);

    const QList<QPair<KoSvgText::EastAsianVariant, QString>> variants = {
        {KoSvgText::EastAsianJis78, QStringLiteral("jp78[7:19]=1")},
        {KoSvgText::EastAsianJis83, QStringLiteral("jp83[7:19]=1")},
        {KoSvgText::EastAsianJis90, QStringLiteral("jp90[7:19]=1")},
        {KoSvgText::EastAsianJis04, QStringLiteral("jp04[7:19]=1")},
        {KoSvgText::EastAsianSimplified, QStringLiteral("smpl[7:19]=1")},
        {KoSvgText::EastAsianTraditional, QStringLiteral("trad[7:19]=1")},
    };
    for (const auto &entry : variants) {
        EastAsian feature;
        feature.variant = entry.first;
        QCOMPARE(feature.fontFeatures(7, 19), QStringList({entry.second}));
    }

    EastAsian fullWidth;
    fullWidth.width = KoSvgText::EastAsianFullWidth;
    QCOMPARE(fullWidth.fontFeatures(7, 19), QStringList({QStringLiteral("fwid[7:19]=1")}));

    EastAsian combined;
    combined.variant = KoSvgText::EastAsianTraditional;
    combined.width = KoSvgText::EastAsianProportionalWidth;
    combined.ruby = true;
    const QStringList combinedExpected = {
        QStringLiteral("trad[7:19]=1"),
        QStringLiteral("pwid[7:19]=1"),
        QStringLiteral("ruby[7:19]=1"),
    };
    QCOMPARE(combined.fontFeatures(7, 19), combinedExpected);
    QVERIFY(!(combined == defaults));
}

QTEST_GUILESS_MAIN(KoSvgTextFontSelectionValueContractTest)

#include "KoSvgTextFontSelectionValueContractTest.moc"
