/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgText.h"

#include <QTest>

namespace
{
template<typename Enum>
void compareEnum(Enum actual, Enum expected)
{
    QCOMPARE(int(actual), int(expected));
}
} // namespace

class KoSvgTextEnumConversionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writingDirectionAndBidiTokensPreserveAliasesAndFallbacks();
    void orientationAnchoringAndAlignmentPreserveCanonicalValues();
    void breakingAndLengthAdjustmentPreserveCanonicalValues();
    void textPathTokensPreserveCanonicalDefaults();
    void textRenderingTokensPreserveCanonicalDefault();
    void fontFeatureTokensPreserveUpdatesSerializationAndRanges();
    void fontWeightAndStyleTokensPreserveBoundsAndAngles();
    void baselineTokensPreserveAliasesAndFallbacks();
    void whitespaceTokensPreserveShorthandsFallbacksAndTrimIndependence();
    void textTransformTokensPreserveOrderedResetAndWarnings();
};

void KoSvgTextEnumConversionContractTest::writingDirectionAndBidiTokensPreserveAliasesAndFallbacks()
{
    using namespace KoSvgText;

    compareEnum(parseWritingMode(QStringLiteral("tb-rl")), VerticalRL);
    compareEnum(parseWritingMode(QStringLiteral("tb")), VerticalRL);
    compareEnum(parseWritingMode(QStringLiteral("vertical-rl")), VerticalRL);
    compareEnum(parseWritingMode(QStringLiteral("vertical-lr")), VerticalLR);
    compareEnum(parseWritingMode(QStringLiteral("horizontal-tb")), HorizontalTB);
    compareEnum(parseWritingMode(QStringLiteral("unexpected")), HorizontalTB);

    QCOMPARE(writeWritingMode(HorizontalTB), QStringLiteral("horizontal-tb"));
    QCOMPARE(writeWritingMode(VerticalRL), QStringLiteral("vertical-rl"));
    QCOMPARE(writeWritingMode(VerticalLR), QStringLiteral("vertical-lr"));
    QCOMPARE(writeWritingMode(HorizontalTB, true), QStringLiteral("lr"));
    QCOMPARE(writeWritingMode(VerticalRL, true), QStringLiteral("tb"));
    QCOMPARE(writeWritingMode(VerticalLR, true), QStringLiteral("lr"));

    compareEnum(parseDirection(QStringLiteral("rtl")), DirectionRightToLeft);
    compareEnum(parseDirection(QStringLiteral("ltr")), DirectionLeftToRight);
    compareEnum(parseDirection(QStringLiteral("unexpected")), DirectionLeftToRight);
    QCOMPARE(writeDirection(DirectionLeftToRight), QStringLiteral("ltr"));
    QCOMPARE(writeDirection(DirectionRightToLeft), QStringLiteral("rtl"));

    compareEnum(parseUnicodeBidi(QStringLiteral("normal")), BidiNormal);
    compareEnum(parseUnicodeBidi(QStringLiteral("embed")), BidiEmbed);
    compareEnum(parseUnicodeBidi(QStringLiteral("bidi-override")), BidiOverride);
    compareEnum(parseUnicodeBidi(QStringLiteral("isolate")), BidiIsolate);
    compareEnum(parseUnicodeBidi(QStringLiteral("isolate-override")), BidiIsolateOverride);
    compareEnum(parseUnicodeBidi(QStringLiteral("plaintext")), BidiPlainText);
    compareEnum(parseUnicodeBidi(QStringLiteral("unexpected")), BidiNormal);
    QCOMPARE(writeUnicodeBidi(BidiNormal), QStringLiteral("normal"));
    QCOMPARE(writeUnicodeBidi(BidiEmbed), QStringLiteral("embed"));
    QCOMPARE(writeUnicodeBidi(BidiOverride), QStringLiteral("bidi-override"));
    QCOMPARE(writeUnicodeBidi(BidiIsolate), QStringLiteral("isolate"));
    QCOMPARE(writeUnicodeBidi(BidiIsolateOverride), QStringLiteral("isolate-override"));
    QCOMPARE(writeUnicodeBidi(BidiPlainText), QStringLiteral("plaintext"));
}

void KoSvgTextEnumConversionContractTest::orientationAnchoringAndAlignmentPreserveCanonicalValues()
{
    using namespace KoSvgText;

    compareEnum(parseTextOrientation(QStringLiteral("mixed")), OrientationMixed);
    compareEnum(parseTextOrientation(QStringLiteral("upright")), OrientationUpright);
    compareEnum(parseTextOrientation(QStringLiteral("sideways")), OrientationSideWays);
    compareEnum(parseTextOrientation(QStringLiteral("unexpected")), OrientationMixed);
    compareEnum(parseTextOrientationFromGlyphOrientation(AutoValue()), OrientationMixed);
    compareEnum(parseTextOrientationFromGlyphOrientation(AutoValue(0.0)), OrientationUpright);
    compareEnum(parseTextOrientationFromGlyphOrientation(AutoValue(90.0)), OrientationSideWays);
    compareEnum(parseTextOrientationFromGlyphOrientation(AutoValue(-90.0)), OrientationMixed);
    QCOMPARE(writeTextOrientation(OrientationMixed), QStringLiteral("mixed"));
    QCOMPARE(writeTextOrientation(OrientationUpright), QStringLiteral("upright"));
    QCOMPARE(writeTextOrientation(OrientationSideWays), QStringLiteral("sideways"));

    compareEnum(parseTextAnchor(QStringLiteral("start")), AnchorStart);
    compareEnum(parseTextAnchor(QStringLiteral("middle")), AnchorMiddle);
    compareEnum(parseTextAnchor(QStringLiteral("end")), AnchorEnd);
    compareEnum(parseTextAnchor(QStringLiteral("unexpected")), AnchorStart);
    QCOMPARE(writeTextAnchor(AnchorStart), QStringLiteral("start"));
    QCOMPARE(writeTextAnchor(AnchorMiddle), QStringLiteral("middle"));
    QCOMPARE(writeTextAnchor(AnchorEnd), QStringLiteral("end"));

    compareEnum(parseTextAlign(QStringLiteral("start")), AlignStart);
    compareEnum(parseTextAlign(QStringLiteral("end")), AlignEnd);
    compareEnum(parseTextAlign(QStringLiteral("left")), AlignLeft);
    compareEnum(parseTextAlign(QStringLiteral("right")), AlignRight);
    compareEnum(parseTextAlign(QStringLiteral("center")), AlignCenter);
    compareEnum(parseTextAlign(QStringLiteral("justify")), AlignJustify);
    compareEnum(parseTextAlign(QStringLiteral("justify-all")), AlignJustify);
    compareEnum(parseTextAlign(QStringLiteral("match-parent")), AlignMatchParent);
    compareEnum(parseTextAlign(QStringLiteral("auto")), AlignLastAuto);
    compareEnum(parseTextAlign(QStringLiteral("unexpected")), AlignStart);
    QCOMPARE(writeTextAlign(AlignLastAuto), QStringLiteral("auto"));
    QCOMPARE(writeTextAlign(AlignStart), QStringLiteral("start"));
    QCOMPARE(writeTextAlign(AlignEnd), QStringLiteral("end"));
    QCOMPARE(writeTextAlign(AlignLeft), QStringLiteral("left"));
    QCOMPARE(writeTextAlign(AlignRight), QStringLiteral("right"));
    QCOMPARE(writeTextAlign(AlignCenter), QStringLiteral("center"));
    QCOMPARE(writeTextAlign(AlignJustify), QStringLiteral("justify"));
    QCOMPARE(writeTextAlign(AlignMatchParent), QStringLiteral("match-parent"));
}

void KoSvgTextEnumConversionContractTest::breakingAndLengthAdjustmentPreserveCanonicalValues()
{
    using namespace KoSvgText;

    compareEnum(parseWordBreak(QStringLiteral("normal")), WordBreakNormal);
    compareEnum(parseWordBreak(QStringLiteral("keep-all")), WordBreakKeepAll);
    compareEnum(parseWordBreak(QStringLiteral("break-all")), WordBreakBreakAll);
    compareEnum(parseWordBreak(QStringLiteral("unexpected")), WordBreakNormal);
    QCOMPARE(writeWordBreak(WordBreakNormal), QStringLiteral("normal"));
    QCOMPARE(writeWordBreak(WordBreakKeepAll), QStringLiteral("keep-all"));
    QCOMPARE(writeWordBreak(WordBreakBreakAll), QStringLiteral("break-all"));

    compareEnum(parseLineBreak(QStringLiteral("auto")), LineBreakAuto);
    compareEnum(parseLineBreak(QStringLiteral("loose")), LineBreakLoose);
    compareEnum(parseLineBreak(QStringLiteral("normal")), LineBreakNormal);
    compareEnum(parseLineBreak(QStringLiteral("strict")), LineBreakStrict);
    compareEnum(parseLineBreak(QStringLiteral("anywhere")), LineBreakAnywhere);
    compareEnum(parseLineBreak(QStringLiteral("unexpected")), LineBreakAuto);
    QCOMPARE(writeLineBreak(LineBreakAuto), QStringLiteral("auto"));
    QCOMPARE(writeLineBreak(LineBreakLoose), QStringLiteral("loose"));
    QCOMPARE(writeLineBreak(LineBreakNormal), QStringLiteral("normal"));
    QCOMPARE(writeLineBreak(LineBreakStrict), QStringLiteral("strict"));
    QCOMPARE(writeLineBreak(LineBreakAnywhere), QStringLiteral("anywhere"));

    compareEnum(parseLengthAdjust(QStringLiteral("spacing")), LengthAdjustSpacing);
    compareEnum(parseLengthAdjust(QStringLiteral("spacingAndGlyphs")), LengthAdjustSpacingAndGlyphs);
    compareEnum(parseLengthAdjust(QStringLiteral("unexpected")), LengthAdjustSpacing);
    QCOMPARE(writeLengthAdjust(LengthAdjustSpacing), QStringLiteral("spacing"));
    QCOMPARE(writeLengthAdjust(LengthAdjustSpacingAndGlyphs), QStringLiteral("spacingAndGlyphs"));
}

void KoSvgTextEnumConversionContractTest::textPathTokensPreserveCanonicalDefaults()
{
    using namespace KoSvgText;

    compareEnum(parseTextPathMethod(QStringLiteral("align")), TextPathAlign);
    compareEnum(parseTextPathMethod(QStringLiteral("stretch")), TextPathStretch);
    compareEnum(parseTextPathMethod(QStringLiteral("unexpected")), TextPathAlign);
    QCOMPARE(writeTextPathMethod(TextPathAlign), QStringLiteral("align"));
    QCOMPARE(writeTextPathMethod(TextPathStretch), QStringLiteral("stretch"));

    compareEnum(parseTextPathSpacing(QStringLiteral("auto")), TextPathAuto);
    compareEnum(parseTextPathSpacing(QStringLiteral("exact")), TextPathExact);
    compareEnum(parseTextPathSpacing(QStringLiteral("unexpected")), TextPathExact);
    QCOMPARE(writeTextPathSpacing(TextPathAuto), QStringLiteral("auto"));
    QCOMPARE(writeTextPathSpacing(TextPathExact), QStringLiteral("exact"));

    compareEnum(parseTextPathSide(QStringLiteral("left")), TextPathSideLeft);
    compareEnum(parseTextPathSide(QStringLiteral("right")), TextPathSideRight);
    compareEnum(parseTextPathSide(QStringLiteral("unexpected")), TextPathSideRight);
    QCOMPARE(writeTextPathSide(TextPathSideLeft), QStringLiteral("left"));
    QCOMPARE(writeTextPathSide(TextPathSideRight), QStringLiteral("right"));
}

void KoSvgTextEnumConversionContractTest::textRenderingTokensPreserveCanonicalDefault()
{
    using namespace KoSvgText;

    compareEnum(parseTextRendering(QStringLiteral("auto")), RenderingAuto);
    compareEnum(parseTextRendering(QStringLiteral("optimizeSpeed")), RenderingOptimizeSpeed);
    compareEnum(parseTextRendering(QStringLiteral("optimizeLegibility")), RenderingOptimizeLegibility);
    compareEnum(parseTextRendering(QStringLiteral("geometricPrecision")), RenderingGeometricPrecision);
    compareEnum(parseTextRendering(QStringLiteral("unexpected")), RenderingAuto);
    QCOMPARE(writeTextRendering(RenderingAuto), QStringLiteral("auto"));
    QCOMPARE(writeTextRendering(RenderingOptimizeSpeed), QStringLiteral("optimizeSpeed"));
    QCOMPARE(writeTextRendering(RenderingOptimizeLegibility), QStringLiteral("optimizeLegibility"));
    QCOMPARE(writeTextRendering(RenderingGeometricPrecision), QStringLiteral("geometricPrecision"));
}

void KoSvgTextEnumConversionContractTest::fontFeatureTokensPreserveUpdatesSerializationAndRanges()
{
    using namespace KoSvgText;

    FontFeatureLigatures ligatures;
    ligatures = parseFontFeatureLigatures(QStringLiteral("no-common-ligatures"), ligatures);
    QVERIFY(!ligatures.commonLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("common-ligatures"), ligatures);
    QVERIFY(ligatures.commonLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("discretionary-ligatures"), ligatures);
    QVERIFY(ligatures.discretionaryLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("no-discretionary-ligatures"), ligatures);
    QVERIFY(!ligatures.discretionaryLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("historical-ligatures"), ligatures);
    QVERIFY(ligatures.historicalLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("no-historical-ligatures"), ligatures);
    QVERIFY(!ligatures.historicalLigatures);
    ligatures = parseFontFeatureLigatures(QStringLiteral("no-contextual"), ligatures);
    QVERIFY(!ligatures.contextualAlternates);
    ligatures = parseFontFeatureLigatures(QStringLiteral("contextual"), ligatures);
    QVERIFY(ligatures.contextualAlternates);

    FontFeatureLigatures unchangedLigatures;
    unchangedLigatures.discretionaryLigatures = true;
    unchangedLigatures.contextualAlternates = false;
    const FontFeatureLigatures expectedUnchangedLigatures = unchangedLigatures;
    unchangedLigatures = parseFontFeatureLigatures(QStringLiteral("unexpected"), unchangedLigatures);
    QVERIFY(unchangedLigatures == expectedUnchangedLigatures);
    unchangedLigatures = parseFontFeatureLigatures(QStringLiteral("normal"), unchangedLigatures);
    QVERIFY(unchangedLigatures == expectedUnchangedLigatures);
    unchangedLigatures = parseFontFeatureLigatures(QStringLiteral("none"), unchangedLigatures);
    QVERIFY(!unchangedLigatures.commonLigatures);
    QVERIFY(!unchangedLigatures.discretionaryLigatures);
    QVERIFY(!unchangedLigatures.historicalLigatures);
    QVERIFY(!unchangedLigatures.contextualAlternates);

    QCOMPARE(writeFontFeatureLigatures(FontFeatureLigatures()), QStringLiteral("normal"));
    FontFeatureLigatures normalDespiteContext;
    normalDespiteContext.contextualAlternates = false;
    QCOMPARE(writeFontFeatureLigatures(normalDespiteContext), QStringLiteral("normal"));
    FontFeatureLigatures noneDespiteContext;
    noneDespiteContext.commonLigatures = false;
    noneDespiteContext.contextualAlternates = true;
    QCOMPARE(writeFontFeatureLigatures(noneDespiteContext), QStringLiteral("none"));
    FontFeatureLigatures allLigatureTokens;
    allLigatureTokens.commonLigatures = false;
    allLigatureTokens.discretionaryLigatures = true;
    allLigatureTokens.historicalLigatures = true;
    allLigatureTokens.contextualAlternates = false;
    QCOMPARE(writeFontFeatureLigatures(allLigatureTokens),
             QStringLiteral("no-common-ligatures discretionary-ligatures historical-ligatures no-contextual"));

    FontFeatureNumeric numeric;
    numeric = parseFontFeatureNumeric(QStringLiteral("lining-nums"), numeric);
    compareEnum(numeric.style, NumericFigureStyleLining);
    numeric = parseFontFeatureNumeric(QStringLiteral("oldstyle-nums"), numeric);
    compareEnum(numeric.style, NumericFigureStyleOld);
    numeric = parseFontFeatureNumeric(QStringLiteral("proportional-nums"), numeric);
    compareEnum(numeric.spacing, NumericFigureSpacingProportional);
    numeric = parseFontFeatureNumeric(QStringLiteral("tabular-nums"), numeric);
    compareEnum(numeric.spacing, NumericFigureSpacingTabular);
    numeric = parseFontFeatureNumeric(QStringLiteral("diagonal-fractions"), numeric);
    compareEnum(numeric.fractions, NumericFractionsDiagonal);
    numeric = parseFontFeatureNumeric(QStringLiteral("stacked-fractions"), numeric);
    compareEnum(numeric.fractions, NumericFractionsStacked);
    numeric = parseFontFeatureNumeric(QStringLiteral("ordinal"), numeric);
    numeric = parseFontFeatureNumeric(QStringLiteral("slashed-zero"), numeric);
    QVERIFY(numeric.ordinals);
    QVERIFY(numeric.slashedZero);
    QCOMPARE(writeFontFeatureNumeric(numeric),
             QStringLiteral("oldstyle-nums tabular-nums stacked-fractions ordinal slashed-zero"));
    FontFeatureNumeric alternateNumeric;
    alternateNumeric.style = NumericFigureStyleLining;
    alternateNumeric.spacing = NumericFigureSpacingProportional;
    alternateNumeric.fractions = NumericFractionsDiagonal;
    QCOMPARE(writeFontFeatureNumeric(alternateNumeric),
             QStringLiteral("lining-nums proportional-nums diagonal-fractions"));
    QCOMPARE(writeFontFeatureNumeric(FontFeatureNumeric()), QStringLiteral("normal"));
    numeric = parseFontFeatureNumeric(QStringLiteral("normal"), numeric);
    QVERIFY(numeric == FontFeatureNumeric());
    numeric.ordinals = true;
    numeric = parseFontFeatureNumeric(QStringLiteral("unexpected"), numeric);
    QVERIFY(numeric == FontFeatureNumeric());

    struct EastAsianVariantCase {
        const char *token;
        EastAsianVariant variant;
    };
    const EastAsianVariantCase eastAsianVariants[] = {
        {"jis78", EastAsianJis78},
        {"jis83", EastAsianJis83},
        {"jis90", EastAsianJis90},
        {"jis04", EastAsianJis04},
        {"simplified", EastAsianSimplified},
        {"traditional", EastAsianTraditional},
    };
    for (const EastAsianVariantCase &variantCase : eastAsianVariants) {
        FontFeatureEastAsian feature;
        feature = parseFontFeatureEastAsian(QLatin1String(variantCase.token), feature);
        compareEnum(feature.variant, variantCase.variant);
        QCOMPARE(writeFontFeatureEastAsian(feature), QLatin1String(variantCase.token));
    }
    FontFeatureEastAsian eastAsian;
    eastAsian = parseFontFeatureEastAsian(QStringLiteral("full-width"), eastAsian);
    compareEnum(eastAsian.width, EastAsianFullWidth);
    QCOMPARE(writeFontFeatureEastAsian(eastAsian), QStringLiteral("full-width"));
    eastAsian = parseFontFeatureEastAsian(QStringLiteral("proportional-width"), eastAsian);
    compareEnum(eastAsian.width, EastAsianProportionalWidth);
    eastAsian = parseFontFeatureEastAsian(QStringLiteral("ruby"), eastAsian);
    QVERIFY(eastAsian.ruby);
    QCOMPARE(writeFontFeatureEastAsian(eastAsian), QStringLiteral("proportional-width ruby"));
    eastAsian = parseFontFeatureEastAsian(QStringLiteral("normal"), eastAsian);
    QVERIFY(eastAsian == FontFeatureEastAsian());
    eastAsian.ruby = true;
    eastAsian = parseFontFeatureEastAsian(QStringLiteral("unexpected"), eastAsian);
    QVERIFY(eastAsian == FontFeatureEastAsian());
    QCOMPARE(writeFontFeatureEastAsian(FontFeatureEastAsian()), QStringLiteral("normal"));

    compareEnum(parseFontFeaturePosition(QStringLiteral("super"), PositionNormal), PositionSuper);
    compareEnum(parseFontFeaturePosition(QStringLiteral("sub"), PositionNormal), PositionSub);
    compareEnum(parseFontFeaturePosition(QStringLiteral("normal"), PositionSuper), PositionNormal);
    compareEnum(parseFontFeaturePosition(QStringLiteral("unexpected"), PositionSub), PositionSub);
    QCOMPARE(writeFontFeaturePosition(PositionSuper), QStringLiteral("super"));
    QCOMPARE(writeFontFeaturePosition(PositionSub), QStringLiteral("sub"));
    QCOMPARE(writeFontFeaturePosition(static_cast<FontFeaturePosition>(-1)), QStringLiteral("normal"));
    QCOMPARE(fontFeaturesPosition(PositionSuper, -2, 7), QStringList({QStringLiteral("sups[-2:7]=1")}));
    QCOMPARE(fontFeaturesPosition(PositionSub, 8, 3), QStringList({QStringLiteral("subs[8:3]=1")}));
    QVERIFY(fontFeaturesPosition(PositionNormal, 1, 4).isEmpty());

    struct CapsCase {
        const char *token;
        FontFeatureCaps caps;
        QStringList featureTags;
    };
    const CapsCase capsCases[] = {
        {"small-caps", CapsSmall, {QStringLiteral("smcp[-1:5]=1")}},
        {"all-small-caps", CapsAllSmall, {QStringLiteral("smcp[-1:5]=1"), QStringLiteral("c2sc[-1:5]=1")}},
        {"petite-caps", CapsPetite, {QStringLiteral("pcap[-1:5]=1")}},
        {"all-petite-caps", CapsAllPetite, {QStringLiteral("pcap[-1:5]=1"), QStringLiteral("c2pc[-1:5]=1")}},
        {"unicase", CapsUnicase, {QStringLiteral("unic[-1:5]=1")}},
        {"titling-caps", CapsTitling, {QStringLiteral("titl[-1:5]=1")}},
    };
    for (const CapsCase &capsCase : capsCases) {
        compareEnum(parseFontFeatureCaps(QLatin1String(capsCase.token), CapsNormal), capsCase.caps);
        QCOMPARE(writeFontFeatureCaps(capsCase.caps), QLatin1String(capsCase.token));
        QCOMPARE(fontFeaturesCaps(capsCase.caps, -1, 5), capsCase.featureTags);
    }
    compareEnum(parseFontFeatureCaps(QStringLiteral("normal"), CapsSmall), CapsNormal);
    compareEnum(parseFontFeatureCaps(QStringLiteral("unexpected"), CapsUnicase), CapsUnicase);
    QCOMPARE(writeFontFeatureCaps(static_cast<FontFeatureCaps>(-1)), QStringLiteral("normal"));
    QVERIFY(fontFeaturesCaps(CapsNormal, 5, -1).isEmpty());
}

void KoSvgTextEnumConversionContractTest::fontWeightAndStyleTokensPreserveBoundsAndAngles()
{
    using namespace KoSvgText;

    QCOMPARE(parseCSSFontWeight(QStringLiteral("bold"), 400), 700);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("bolder"), 100), 200);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("bolder"), 450), 500);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("bolder"), 900), 900);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("bolder"), 950), 900);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("lighter"), 900), 800);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("lighter"), 450), 400);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("lighter"), 100), 100);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("lighter"), 50), 100);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("-50"), 400), 0);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("550"), 400), 550);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("1250"), 400), 1000);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("normal"), 700), 400);
    QCOMPARE(parseCSSFontWeight(QStringLiteral("unexpected"), 700), 400);

    compareEnum(parseFontStyle(QStringLiteral("italic")).style, QFont::StyleItalic);
    compareEnum(parseFontStyle(QStringLiteral("oblique")).style, QFont::StyleOblique);
    compareEnum(parseFontStyle(QStringLiteral("normal")).style, QFont::StyleNormal);
    compareEnum(parseFontStyle(QStringLiteral("unexpected")).style, QFont::StyleNormal);

    const CssFontStyleData angledOblique = parseFontStyle(QStringLiteral("oblique 12.5deg"));
    compareEnum(angledOblique.style, QFont::StyleOblique);
    QVERIFY(!angledOblique.slantValue.isAuto);
    QCOMPARE(angledOblique.slantValue.customValue, 12.5);
    QCOMPARE(writeFontStyle(angledOblique), QStringLiteral("oblique 12.5deg"));

    const CssFontStyleData invalidAngle = parseFontStyle(QStringLiteral("oblique invaliddeg"));
    QVERIFY(!invalidAngle.slantValue.isAuto);
    QCOMPARE(invalidAngle.slantValue.customValue, 0.0);
    QCOMPARE(writeFontStyle(invalidAngle), QStringLiteral("oblique 0deg"));

    const CssFontStyleData italicAngle = parseFontStyle(QStringLiteral("italic 8deg"));
    QVERIFY(!italicAngle.slantValue.isAuto);
    QCOMPARE(writeFontStyle(italicAngle), QStringLiteral("italic"));
    QCOMPARE(writeFontStyle(CssFontStyleData(QFont::StyleNormal)), QStringLiteral("normal"));
    QCOMPARE(writeFontStyle(CssFontStyleData(QFont::StyleItalic)), QStringLiteral("italic"));
    QCOMPARE(writeFontStyle(CssFontStyleData(QFont::StyleOblique)), QStringLiteral("oblique"));
}

void KoSvgTextEnumConversionContractTest::baselineTokensPreserveAliasesAndFallbacks()
{
    using namespace KoSvgText;

    struct BaselineParseCase {
        const char *token;
        Baseline baseline;
    };
    const BaselineParseCase baselineCases[] = {
        {"use-script", BaselineUseScript},
        {"no-change", BaselineNoChange},
        {"reset-size", BaselineResetSize},
        {"ideographic", BaselineIdeographic},
        {"alphabetic", BaselineAlphabetic},
        {"hanging", BaselineHanging},
        {"mathematical", BaselineMathematical},
        {"central", BaselineCentral},
        {"middle", BaselineMiddle},
        {"baseline", BaselineDominant},
        {"text-after-edge", BaselineTextBottom},
        {"after-edge", BaselineTextBottom},
        {"text-bottom", BaselineTextBottom},
        {"text-before-edge", BaselineTextTop},
        {"before-edge", BaselineTextTop},
        {"text-top", BaselineTextTop},
    };
    for (const BaselineParseCase &baselineCase : baselineCases) {
        compareEnum(parseBaseline(QLatin1String(baselineCase.token)), baselineCase.baseline);
    }
    compareEnum(parseBaseline(QStringLiteral("unexpected")), BaselineAuto);

    compareEnum(parseBaselineShiftMode(QStringLiteral("baseline")), ShiftNone);
    compareEnum(parseBaselineShiftMode(QStringLiteral("sub")), ShiftSub);
    compareEnum(parseBaselineShiftMode(QStringLiteral("super")), ShiftSuper);
    compareEnum(parseBaselineShiftMode(QStringLiteral("top")), ShiftLineTop);
    compareEnum(parseBaselineShiftMode(QStringLiteral("bottom")), ShiftLineBottom);
    compareEnum(parseBaselineShiftMode(QStringLiteral("unexpected")), ShiftLengthPercentage);

    struct BaselineWriteCase {
        Baseline baseline;
        const char *dominant;
        const char *alignment;
    };
    const BaselineWriteCase writeCases[] = {
        {BaselineAuto, "auto", "auto"},
        {BaselineUseScript, "use-script", "auto"},
        {BaselineDominant, "auto", "baseline"},
        {BaselineNoChange, "no-change", "auto"},
        {BaselineResetSize, "reset-size", "auto"},
        {BaselineIdeographic, "ideographic", "ideographic"},
        {BaselineAlphabetic, "alphabetic", "alphabetic"},
        {BaselineHanging, "hanging", "hanging"},
        {BaselineMathematical, "mathematical", "mathematical"},
        {BaselineCentral, "central", "central"},
        {BaselineMiddle, "middle", "middle"},
        {BaselineTextBottom, "text-bottom", "text-bottom"},
        {BaselineTextTop, "text-top", "text-top"},
    };
    for (const BaselineWriteCase &writeCase : writeCases) {
        QCOMPARE(writeDominantBaseline(writeCase.baseline), QLatin1String(writeCase.dominant));
        QCOMPARE(writeAlignmentBaseline(writeCase.baseline), QLatin1String(writeCase.alignment));
    }
    QCOMPARE(writeDominantBaseline(static_cast<Baseline>(-1)), QStringLiteral("auto"));
    QCOMPARE(writeAlignmentBaseline(static_cast<Baseline>(-1)), QStringLiteral("auto"));
}

void KoSvgTextEnumConversionContractTest::whitespaceTokensPreserveShorthandsFallbacksAndTrimIndependence()
{
    using namespace KoSvgText;

    const auto compareWhiteSpace =
        [](const QString &token, bool expectedResult, TextSpaceCollapse expectedCollapse, TextWrap expectedWrap) {
            TextSpaceCollapse collapse = Discard;
            TextWrap wrap = Balance;
            TextSpaceTrims trim = TrimInner | DiscardAfter;
            QCOMPARE(whiteSpaceValueToLongHands(token, collapse, wrap, trim), expectedResult);
            compareEnum(collapse, expectedCollapse);
            compareEnum(wrap, expectedWrap);
            QCOMPARE(int(trim), int(TextSpaceTrims(TrimNone)));
        };
    compareWhiteSpace(QStringLiteral("pre"), true, Preserve, NoWrap);
    compareWhiteSpace(QStringLiteral("nowrap"), true, Collapse, NoWrap);
    compareWhiteSpace(QStringLiteral("pre-wrap"), true, Preserve, Wrap);
    compareWhiteSpace(QStringLiteral("pre-line"), true, PreserveBreaks, Wrap);
    compareWhiteSpace(QStringLiteral("normal"), true, Collapse, Wrap);
    compareWhiteSpace(QStringLiteral("break-spaces"), false, Collapse, Wrap);
    compareWhiteSpace(QStringLiteral("unexpected"), false, Collapse, Wrap);

    QCOMPARE(writeWhiteSpaceValue(Preserve, Wrap, TrimNone), QStringLiteral("pre-wrap"));
    QCOMPARE(writeWhiteSpaceValue(PreserveBreaks, Wrap, TrimNone), QStringLiteral("pre-line"));
    QCOMPARE(writeWhiteSpaceValue(BreakSpaces, Wrap, TrimNone), QStringLiteral("break-spaces"));
    QCOMPARE(writeWhiteSpaceValue(Collapse, Wrap, TrimNone), QStringLiteral("normal"));
    QCOMPARE(writeWhiteSpaceValue(Discard, Stable, TrimNone), QStringLiteral("normal"));
    QCOMPARE(writeWhiteSpaceValue(Preserve, NoWrap, TrimNone), QStringLiteral("pre"));
    QCOMPARE(writeWhiteSpaceValue(BreakSpaces, NoWrap, TrimNone), QStringLiteral("nowrap"));
    QCOMPARE(writeWhiteSpaceValue(Preserve, Wrap, TrimInner | DiscardAfter),
             writeWhiteSpaceValue(Preserve, Wrap, TrimNone));

    TextSpaceCollapse xmlCollapse = Discard;
    QVERIFY(xmlSpaceToLongHands(QStringLiteral("preserve"), xmlCollapse));
    compareEnum(xmlCollapse, PreserveSpaces);
    QVERIFY(xmlSpaceToLongHands(QStringLiteral("default"), xmlCollapse));
    compareEnum(xmlCollapse, Collapse);
    QVERIFY(!xmlSpaceToLongHands(QStringLiteral("unexpected"), xmlCollapse));
    compareEnum(xmlCollapse, Collapse);
    QCOMPARE(writeXmlSpace(PreserveSpaces), QStringLiteral("preserve"));
    QCOMPARE(writeXmlSpace(Preserve), QStringLiteral("default"));
    QCOMPARE(writeXmlSpace(static_cast<TextSpaceCollapse>(-1)), QStringLiteral("default"));
}

void KoSvgTextEnumConversionContractTest::textTransformTokensPreserveOrderedResetAndWarnings()
{
    using namespace KoSvgText;

    compareEnum(parseTextTransform(QStringLiteral("capitalize")).capitals, TextTransformCapitalize);
    compareEnum(parseTextTransform(QStringLiteral("UPPERCASE")).capitals, TextTransformUppercase);
    compareEnum(parseTextTransform(QStringLiteral("lowercase")).capitals, TextTransformLowercase);
    QVERIFY(parseTextTransform(QStringLiteral("full-width")).fullWidth);
    QVERIFY(parseTextTransform(QStringLiteral("full-size-kana")).fullSizeKana);

    const TextTransformInfo combined = parseTextTransform(QStringLiteral("uppercase full-width full-size-kana"));
    compareEnum(combined.capitals, TextTransformUppercase);
    QVERIFY(combined.fullWidth);
    QVERIFY(combined.fullSizeKana);
    QCOMPARE(writeTextTransform(combined), QStringLiteral("uppercase full-width full-size-kana"));

    const TextTransformInfo resetThenSet =
        parseTextTransform(QStringLiteral("uppercase full-size-kana none full-width"));
    compareEnum(resetThenSet.capitals, TextTransformNone);
    QVERIFY(resetThenSet.fullWidth);
    QVERIFY(!resetThenSet.fullSizeKana);
    const TextTransformInfo lastCapitalWins = parseTextTransform(QStringLiteral("uppercase lowercase capitalize"));
    compareEnum(lastCapitalWins.capitals, TextTransformCapitalize);

    QTest::ignoreMessage(QtWarningMsg, "Unknown parameter in text-transform \"unexpected\"");
    const TextTransformInfo warningPreservesState =
        parseTextTransform(QStringLiteral("uppercase unexpected full-width"));
    compareEnum(warningPreservesState.capitals, TextTransformUppercase);
    QVERIFY(warningPreservesState.fullWidth);

    QCOMPARE(writeTextTransform(TextTransformInfo()), QStringLiteral("none"));
    TextTransformInfo lowercase;
    lowercase.capitals = TextTransformLowercase;
    QCOMPARE(writeTextTransform(lowercase), QStringLiteral("lowercase"));
    TextTransformInfo capitalize;
    capitalize.capitals = TextTransformCapitalize;
    QCOMPARE(writeTextTransform(capitalize), QStringLiteral("capitalize"));
    TextTransformInfo nonCanonical;
    nonCanonical.capitals = static_cast<TextTransform>(-1);
    QCOMPARE(writeTextTransform(nonCanonical), QString());
}

QTEST_GUILESS_MAIN(KoSvgTextEnumConversionContractTest)

#include "KoSvgTextEnumConversionContractTest.moc"
