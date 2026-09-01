/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "svg/SvgGraphicContext.h"
#include "text/KoSvgText.h"
#include "text/KoSvgTextProperties.h"

#include <QFlags>
#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

class KoSvgTextEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layoutEnumsRemainOrdered();
    void whitespaceAndDecorationEnumsRemainOrdered();
    void fontVariantEnumsRemainOrdered();
    void flagAliasesPreserveIndependentOptions();
    void fontKeywordTablesRemainStable();
    void propertyIdentifiersRemainContiguous();
    void autoValuesDistinguishAutomaticAndNumericValues();
    void autoLengthsPreserveUnitsAndValueSemantics();
    void fontStylesCompareSlantOnlyForOblique();
    void textTransformsOwnIndependentFlags();
    void textIndentOwnsLengthAndLineFlags();
    void cssLengthPercentageValueSchemaRemainsStable();
    void cssLengthPercentageResolutionAndFormattingSignaturesRemainStable();
    void lineHeightValueSchemaRemainsStable();
    void tabSizeValueSchemaRemainsStable();
    void lineHeightAndTabSizeConversionSignaturesRemainStable();
    void characterTransformationValueSchemaRemainsStable();
    void characterTransformationOperationSignaturesRemainStable();
    void resolutionHandlerValueSchemaRemainsStable();
    void resolutionHandlerConversionSignaturesRemainStable();
    void resolutionHandlerAdjustmentSignaturesRemainStable();
    void graphicsContextStyleOrdinalsRemainStable();
    void graphicsContextFillAndStrokeStateKeepsPublicTypes();
    void graphicsContextClipTransformAndColorStateKeepsPublicTypes();
    void graphicsContextSpatialVisibilityAndResolutionStateKeepsPublicTypes();
    void graphicsContextMarkerTextAndPaintStateKeepsPublicTypes();
    void textPropertyValueTypeAndLifetimeSchemaRemainsStable();
    void textPropertyValueMapSchemaRemainsStable();
    void textPropertyInheritanceAndScalingSchemaRemainsStable();
    void textPropertyInterchangeAndFontProjectionSchemaRemainsStable();
    void textPropertyMetricsAndSurfaceSchemaRemainsStable();
};

void KoSvgTextEnumContractTest::layoutEnumsRemainOrdered()
{
    QCOMPARE(int(KoSvgText::HorizontalTB), 0);
    QCOMPARE(int(KoSvgText::VerticalRL), 1);
    QCOMPARE(int(KoSvgText::VerticalLR), 2);

    QCOMPARE(int(KoSvgText::DirectionLeftToRight), 0);
    QCOMPARE(int(KoSvgText::DirectionRightToLeft), 1);

    QCOMPARE(int(KoSvgText::BidiNormal), 0);
    QCOMPARE(int(KoSvgText::BidiEmbed), 1);
    QCOMPARE(int(KoSvgText::BidiOverride), 2);
    QCOMPARE(int(KoSvgText::BidiIsolate), 3);
    QCOMPARE(int(KoSvgText::BidiIsolateOverride), 4);
    QCOMPARE(int(KoSvgText::BidiPlainText), 5);

    QCOMPARE(int(KoSvgText::OrientationMixed), 0);
    QCOMPARE(int(KoSvgText::OrientationUpright), 1);
    QCOMPARE(int(KoSvgText::OrientationSideWays), 2);

    QCOMPARE(int(KoSvgText::AnchorStart), 0);
    QCOMPARE(int(KoSvgText::AnchorMiddle), 1);
    QCOMPARE(int(KoSvgText::AnchorEnd), 2);

    QCOMPARE(int(KoSvgText::BaselineAuto), 0);
    QCOMPARE(int(KoSvgText::BaselineUseScript), 1);
    QCOMPARE(int(KoSvgText::BaselineDominant), 2);
    QCOMPARE(int(KoSvgText::BaselineNoChange), 3);
    QCOMPARE(int(KoSvgText::BaselineResetSize), 4);
    QCOMPARE(int(KoSvgText::BaselineIdeographic), 5);
    QCOMPARE(int(KoSvgText::BaselineAlphabetic), 6);
    QCOMPARE(int(KoSvgText::BaselineHanging), 7);
    QCOMPARE(int(KoSvgText::BaselineMathematical), 8);
    QCOMPARE(int(KoSvgText::BaselineCentral), 9);
    QCOMPARE(int(KoSvgText::BaselineMiddle), 10);
    QCOMPARE(int(KoSvgText::BaselineTextBottom), 11);
    QCOMPARE(int(KoSvgText::BaselineTextTop), 12);

    QCOMPARE(int(KoSvgText::ShiftNone), 0);
    QCOMPARE(int(KoSvgText::ShiftSub), 1);
    QCOMPARE(int(KoSvgText::ShiftSuper), 2);
    QCOMPARE(int(KoSvgText::ShiftLengthPercentage), 3);
    QCOMPARE(int(KoSvgText::ShiftLineTop), 4);
    QCOMPARE(int(KoSvgText::ShiftLineBottom), 5);

    QCOMPARE(int(KoSvgText::LengthAdjustSpacing), 0);
    QCOMPARE(int(KoSvgText::LengthAdjustSpacingAndGlyphs), 1);

    QCOMPARE(int(KoSvgText::RenderingAuto), 0);
    QCOMPARE(int(KoSvgText::RenderingOptimizeSpeed), 1);
    QCOMPARE(int(KoSvgText::RenderingOptimizeLegibility), 2);
    QCOMPARE(int(KoSvgText::RenderingGeometricPrecision), 3);
}

void KoSvgTextEnumContractTest::whitespaceAndDecorationEnumsRemainOrdered()
{
    QCOMPARE(int(KoSvgText::Collapse), 0);
    QCOMPARE(int(KoSvgText::Discard), 1);
    QCOMPARE(int(KoSvgText::Preserve), 2);
    QCOMPARE(int(KoSvgText::PreserveBreaks), 3);
    QCOMPARE(int(KoSvgText::PreserveSpaces), 4);
    QCOMPARE(int(KoSvgText::BreakSpaces), 5);

    QCOMPARE(int(KoSvgText::Wrap), 0);
    QCOMPARE(int(KoSvgText::NoWrap), 1);
    QCOMPARE(int(KoSvgText::Balance), 2);
    QCOMPARE(int(KoSvgText::Stable), 3);
    QCOMPARE(int(KoSvgText::Pretty), 4);

    QCOMPARE(int(KoSvgText::TrimNone), 0x0);
    QCOMPARE(int(KoSvgText::TrimInner), 0x1);
    QCOMPARE(int(KoSvgText::DiscardBefore), 0x2);
    QCOMPARE(int(KoSvgText::DiscardAfter), 0x4);

    QCOMPARE(int(KoSvgText::WordBreakNormal), 0);
    QCOMPARE(int(KoSvgText::WordBreakKeepAll), 1);
    QCOMPARE(int(KoSvgText::WordBreakBreakAll), 2);

    QCOMPARE(int(KoSvgText::LineBreakAuto), 0);
    QCOMPARE(int(KoSvgText::LineBreakLoose), 1);
    QCOMPARE(int(KoSvgText::LineBreakNormal), 2);
    QCOMPARE(int(KoSvgText::LineBreakStrict), 3);
    QCOMPARE(int(KoSvgText::LineBreakAnywhere), 4);

    QCOMPARE(int(KoSvgText::OverflowWrapNormal), 0);
    QCOMPARE(int(KoSvgText::OverflowWrapAnywhere), 1);
    QCOMPARE(int(KoSvgText::OverflowWrapBreakWord), 2);

    QCOMPARE(int(KoSvgText::AlignLastAuto), 0);
    QCOMPARE(int(KoSvgText::AlignStart), 1);
    QCOMPARE(int(KoSvgText::AlignEnd), 2);
    QCOMPARE(int(KoSvgText::AlignLeft), 3);
    QCOMPARE(int(KoSvgText::AlignRight), 4);
    QCOMPARE(int(KoSvgText::AlignCenter), 5);
    QCOMPARE(int(KoSvgText::AlignJustify), 6);
    QCOMPARE(int(KoSvgText::AlignMatchParent), 7);

    QCOMPARE(int(KoSvgText::TextTransformNone), 0x0);
    QCOMPARE(int(KoSvgText::TextTransformCapitalize), 0x1);
    QCOMPARE(int(KoSvgText::TextTransformUppercase), 0x2);
    QCOMPARE(int(KoSvgText::TextTransformLowercase), 0x4);

    QCOMPARE(int(KoSvgText::OverFlowVisible), 0);
    QCOMPARE(int(KoSvgText::OverFlowClip), 1);
    QCOMPARE(int(KoSvgText::OverFlowEllipse), 2);

    QCOMPARE(int(KoSvgText::HangNone), 0x0);
    QCOMPARE(int(KoSvgText::HangFirst), 0x1);
    QCOMPARE(int(KoSvgText::HangLast), 0x2);
    QCOMPARE(int(KoSvgText::HangEnd), 0x4);
    QCOMPARE(int(KoSvgText::HangForce), 0x8);

    QCOMPARE(int(KoSvgText::DecorationNone), 0x0);
    QCOMPARE(int(KoSvgText::DecorationUnderline), 0x1);
    QCOMPARE(int(KoSvgText::DecorationOverline), 0x2);
    QCOMPARE(int(KoSvgText::DecorationLineThrough), 0x4);

    QCOMPARE(int(KoSvgText::Solid), 0);
    QCOMPARE(int(KoSvgText::Double), 1);
    QCOMPARE(int(KoSvgText::Dotted), 2);
    QCOMPARE(int(KoSvgText::Dashed), 3);
    QCOMPARE(int(KoSvgText::Wavy), 4);

    QCOMPARE(int(KoSvgText::UnderlineAuto), 0);
    QCOMPARE(int(KoSvgText::UnderlineUnder), 1);
    QCOMPARE(int(KoSvgText::UnderlineLeft), 2);
    QCOMPARE(int(KoSvgText::UnderlineRight), 3);

    QCOMPARE(int(KoSvgText::TextPathAlign), 0);
    QCOMPARE(int(KoSvgText::TextPathStretch), 1);
    QCOMPARE(int(KoSvgText::TextPathAuto), 0);
    QCOMPARE(int(KoSvgText::TextPathExact), 1);
    QCOMPARE(int(KoSvgText::TextPathSideRight), 0);
    QCOMPARE(int(KoSvgText::TextPathSideLeft), 1);
}

void KoSvgTextEnumContractTest::fontVariantEnumsRemainOrdered()
{
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Absolute), 0);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Percentage), 1);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Em), 2);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Ex), 3);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Cap), 4);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Ch), 5);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Ic), 6);
    QCOMPARE(int(KoSvgText::CssLengthPercentage::Lh), 7);

    QCOMPARE(int(KoSvgText::UnknownFontType), 0);
    QCOMPARE(int(KoSvgText::BDFFontType), 1);
    QCOMPARE(int(KoSvgText::Type1FontType), 2);
    QCOMPARE(int(KoSvgText::OpenTypeFontType), 3);

    QCOMPARE(int(KoSvgText::PositionNormal), 0);
    QCOMPARE(int(KoSvgText::PositionSuper), 1);
    QCOMPARE(int(KoSvgText::PositionSub), 2);

    QCOMPARE(int(KoSvgText::CapsNormal), 0);
    QCOMPARE(int(KoSvgText::CapsSmall), 1);
    QCOMPARE(int(KoSvgText::CapsAllSmall), 2);
    QCOMPARE(int(KoSvgText::CapsPetite), 3);
    QCOMPARE(int(KoSvgText::CapsAllPetite), 4);
    QCOMPARE(int(KoSvgText::CapsUnicase), 5);
    QCOMPARE(int(KoSvgText::CapsTitling), 6);

    QCOMPARE(int(KoSvgText::NumericFigureStyleNormal), 0);
    QCOMPARE(int(KoSvgText::NumericFigureStyleLining), 1);
    QCOMPARE(int(KoSvgText::NumericFigureStyleOld), 2);

    QCOMPARE(int(KoSvgText::NumericFigureSpacingNormal), 0);
    QCOMPARE(int(KoSvgText::NumericFigureSpacingProportional), 1);
    QCOMPARE(int(KoSvgText::NumericFigureSpacingTabular), 2);

    QCOMPARE(int(KoSvgText::NumericFractionsNormal), 0);
    QCOMPARE(int(KoSvgText::NumericFractionsDiagonal), 1);
    QCOMPARE(int(KoSvgText::NumericFractionsStacked), 2);

    QCOMPARE(int(KoSvgText::EastAsianVariantNormal), 0);
    QCOMPARE(int(KoSvgText::EastAsianJis78), 1);
    QCOMPARE(int(KoSvgText::EastAsianJis83), 2);
    QCOMPARE(int(KoSvgText::EastAsianJis90), 3);
    QCOMPARE(int(KoSvgText::EastAsianJis04), 4);
    QCOMPARE(int(KoSvgText::EastAsianSimplified), 5);
    QCOMPARE(int(KoSvgText::EastAsianTraditional), 6);

    QCOMPARE(int(KoSvgText::EastAsiantNormalWidth), 0);
    QCOMPARE(int(KoSvgText::EastAsianFullWidth), 1);
    QCOMPARE(int(KoSvgText::EastAsianProportionalWidth), 2);
}

void KoSvgTextEnumContractTest::flagAliasesPreserveIndependentOptions()
{
    static_assert(std::is_same_v<KoSvgText::TextDecorations, QFlags<KoSvgText::TextDecoration>>);
    static_assert(std::is_same_v<KoSvgText::TextSpaceTrims, QFlags<KoSvgText::TextSpaceTrim>>);
    static_assert(std::is_same_v<KoSvgText::HangingPunctuations, QFlags<KoSvgText::HangingPunctuation>>);

    const KoSvgText::TextDecorations decorations = KoSvgText::DecorationUnderline | KoSvgText::DecorationLineThrough;
    QVERIFY(decorations.testFlag(KoSvgText::DecorationUnderline));
    QVERIFY(!decorations.testFlag(KoSvgText::DecorationOverline));
    QVERIFY(decorations.testFlag(KoSvgText::DecorationLineThrough));

    const KoSvgText::TextSpaceTrims trims = KoSvgText::DiscardBefore | KoSvgText::DiscardAfter;
    QVERIFY(!trims.testFlag(KoSvgText::TrimInner));
    QVERIFY(trims.testFlag(KoSvgText::DiscardBefore));
    QVERIFY(trims.testFlag(KoSvgText::DiscardAfter));

    const KoSvgText::HangingPunctuations hanging = KoSvgText::HangFirst | KoSvgText::HangEnd | KoSvgText::HangForce;
    QVERIFY(hanging.testFlag(KoSvgText::HangFirst));
    QVERIFY(!hanging.testFlag(KoSvgText::HangLast));
    QVERIFY(hanging.testFlag(KoSvgText::HangEnd));
    QVERIFY(hanging.testFlag(KoSvgText::HangForce));
}

void KoSvgTextEnumContractTest::fontKeywordTablesRemainStable()
{
    QCOMPARE(KoSvgText::fontStretchNames.size(), size_t(9));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[0]), QByteArray("ultra-condensed"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[1]), QByteArray("extra-condensed"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[2]), QByteArray("condensed"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[3]), QByteArray("semi-condensed"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[4]), QByteArray("normal"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[5]), QByteArray("semi-expanded"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[6]), QByteArray("expanded"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[7]), QByteArray("extra-expanded"));
    QCOMPARE(QByteArray(KoSvgText::fontStretchNames[8]), QByteArray("ultra-expanded"));

    QCOMPARE(KoSvgText::fontSizeNames.size(), size_t(7));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[0]), QByteArray("xx-small"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[1]), QByteArray("x-small"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[2]), QByteArray("small"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[3]), QByteArray("medium"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[4]), QByteArray("large"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[5]), QByteArray("x-large"));
    QCOMPARE(QByteArray(KoSvgText::fontSizeNames[6]), QByteArray("xx-large"));
}

void KoSvgTextEnumContractTest::propertyIdentifiersRemainContiguous()
{
    const std::array<int, 62> identifiers = {
        int(KoSvgTextProperties::WritingModeId),
        int(KoSvgTextProperties::DirectionId),
        int(KoSvgTextProperties::UnicodeBidiId),
        int(KoSvgTextProperties::TextAnchorId),
        int(KoSvgTextProperties::DominantBaselineId),
        int(KoSvgTextProperties::AlignmentBaselineId),
        int(KoSvgTextProperties::BaselineShiftModeId),
        int(KoSvgTextProperties::BaselineShiftValueId),
        int(KoSvgTextProperties::KerningId),
        int(KoSvgTextProperties::TextOrientationId),
        int(KoSvgTextProperties::LetterSpacingId),
        int(KoSvgTextProperties::WordSpacingId),
        int(KoSvgTextProperties::FontFamiliesId),
        int(KoSvgTextProperties::FontStyleId),
        int(KoSvgTextProperties::FontStretchId),
        int(KoSvgTextProperties::FontWeightId),
        int(KoSvgTextProperties::FontSizeId),
        int(KoSvgTextProperties::FontSizeAdjustId),
        int(KoSvgTextProperties::FontVariantLigatureId),
        int(KoSvgTextProperties::FontVariantPositionId),
        int(KoSvgTextProperties::FontVariantCapsId),
        int(KoSvgTextProperties::FontVariantNumericId),
        int(KoSvgTextProperties::FontVariantEastAsianId),
        int(KoSvgTextProperties::FontFeatureSettingsId),
        int(KoSvgTextProperties::FontOpticalSizingId),
        int(KoSvgTextProperties::FontVariationSettingsId),
        int(KoSvgTextProperties::TextDecorationLineId),
        int(KoSvgTextProperties::TextDecorationStyleId),
        int(KoSvgTextProperties::TextDecorationColorId),
        int(KoSvgTextProperties::TextDecorationPositionId),
        int(KoSvgTextProperties::FillId),
        int(KoSvgTextProperties::StrokeId),
        int(KoSvgTextProperties::Opacity),
        int(KoSvgTextProperties::PaintOrder),
        int(KoSvgTextProperties::Visibility),
        int(KoSvgTextProperties::TextLanguage),
        int(KoSvgTextProperties::TextCollapseId),
        int(KoSvgTextProperties::TextWrapId),
        int(KoSvgTextProperties::TextTrimId),
        int(KoSvgTextProperties::LineBreakId),
        int(KoSvgTextProperties::WordBreakId),
        int(KoSvgTextProperties::TextAlignAllId),
        int(KoSvgTextProperties::TextAlignLastId),
        int(KoSvgTextProperties::TextTransformId),
        int(KoSvgTextProperties::TextOverFlowId),
        int(KoSvgTextProperties::OverflowWrapId),
        int(KoSvgTextProperties::InlineSizeId),
        int(KoSvgTextProperties::LineHeightId),
        int(KoSvgTextProperties::TextIndentId),
        int(KoSvgTextProperties::HangingPunctuationId),
        int(KoSvgTextProperties::TabSizeId),
        int(KoSvgTextProperties::ShapePaddingId),
        int(KoSvgTextProperties::ShapeMarginId),
        int(KoSvgTextProperties::FontSynthesisBoldId),
        int(KoSvgTextProperties::FontSynthesisItalicId),
        int(KoSvgTextProperties::FontSynthesisSmallCapsId),
        int(KoSvgTextProperties::FontSynthesisSuperSubId),
        int(KoSvgTextProperties::TextRenderingId),
        int(KoSvgTextProperties::KraTextVersionId),
        int(KoSvgTextProperties::KraTextStyleType),
        int(KoSvgTextProperties::KraTextStyleResolution),
        int(KoSvgTextProperties::LastPropertyId),
    };

    for (size_t i = 0; i < identifiers.size(); ++i) {
        QCOMPARE(identifiers[i], int(i));
    }
}

void KoSvgTextEnumContractTest::autoValuesDistinguishAutomaticAndNumericValues()
{
    KoSvgText::AutoValue automatic;
    QVERIFY(automatic.isAuto);
    QCOMPARE(automatic.customValue, 0.0);

    KoSvgText::AutoValue anotherAutomatic;
    anotherAutomatic.customValue = 91.0;
    QVERIFY(automatic == anotherAutomatic);

    const KoSvgText::AutoValue numeric(7.5);
    QVERIFY(!numeric.isAuto);
    QCOMPARE(numeric.customValue, 7.5);
    QVERIFY(numeric != automatic);
    QVERIFY(numeric == KoSvgText::AutoValue(7.5));
}

void KoSvgTextEnumContractTest::autoLengthsPreserveUnitsAndValueSemantics()
{
    const KoSvgText::AutoLengthPercentage automatic;
    QVERIFY(automatic.isAuto);
    QCOMPARE(automatic.length.value, 0.0);
    QCOMPARE(int(automatic.length.unit), int(KoSvgText::CssLengthPercentage::Absolute));

    const KoSvgText::CssLengthPercentage percentage(0.25, KoSvgText::CssLengthPercentage::Percentage);
    const KoSvgText::AutoLengthPercentage copiedLength(percentage);
    QVERIFY(!copiedLength.isAuto);
    QVERIFY(copiedLength.length == percentage);

    const KoSvgText::AutoLengthPercentage emLength(2.5, KoSvgText::CssLengthPercentage::Em);
    QCOMPARE(emLength.length.value, 2.5);
    QCOMPARE(int(emLength.length.unit), int(KoSvgText::CssLengthPercentage::Em));
    QVERIFY(emLength == KoSvgText::AutoLengthPercentage(2.5, KoSvgText::CssLengthPercentage::Em));
    QVERIFY(emLength != copiedLength);
}

void KoSvgTextEnumContractTest::fontStylesCompareSlantOnlyForOblique()
{
    KoSvgText::CssFontStyleData normal;
    QCOMPARE(int(normal.style), int(QFont::StyleNormal));
    QVERIFY(normal.slantValue.isAuto);

    KoSvgText::CssFontStyleData anotherNormal;
    anotherNormal.slantValue = KoSvgText::AutoValue(12.0);
    QVERIFY(normal == anotherNormal);

    KoSvgText::CssFontStyleData italic(QFont::StyleItalic);
    QCOMPARE(int(italic.style), int(QFont::StyleItalic));
    QVERIFY(italic != normal);

    KoSvgText::CssFontStyleData oblique(QFont::StyleOblique);
    oblique.slantValue = KoSvgText::AutoValue(8.0);
    KoSvgText::CssFontStyleData otherOblique(QFont::StyleOblique);
    otherOblique.slantValue = KoSvgText::AutoValue(9.0);
    QVERIFY(oblique != otherOblique);
    otherOblique.slantValue = KoSvgText::AutoValue(8.0);
    QVERIFY(oblique == otherOblique);
}

void KoSvgTextEnumContractTest::textTransformsOwnIndependentFlags()
{
    const KoSvgText::TextTransformInfo defaults;
    QCOMPARE(int(defaults.capitals), int(KoSvgText::TextTransformNone));
    QVERIFY(!defaults.fullWidth);
    QVERIFY(!defaults.fullSizeKana);

    KoSvgText::TextTransformInfo transformed;
    transformed.capitals = KoSvgText::TextTransformUppercase;
    transformed.fullWidth = true;
    transformed.fullSizeKana = true;
    QCOMPARE(int(transformed.capitals), int(KoSvgText::TextTransformUppercase));
    QVERIFY(transformed.fullWidth);
    QVERIFY(transformed.fullSizeKana);
    QVERIFY(transformed != defaults);

    KoSvgText::TextTransformInfo copy = transformed;
    QVERIFY(copy == transformed);
    copy.fullWidth = false;
    QVERIFY(copy != transformed);
}

void KoSvgTextEnumContractTest::textIndentOwnsLengthAndLineFlags()
{
    const KoSvgText::TextIndentInfo defaults;
    QVERIFY(defaults.length == KoSvgText::CssLengthPercentage());
    QVERIFY(!defaults.hanging);
    QVERIFY(!defaults.eachLine);

    KoSvgText::TextIndentInfo indent;
    indent.length = KoSvgText::CssLengthPercentage(1.75, KoSvgText::CssLengthPercentage::Em);
    indent.hanging = true;
    indent.eachLine = true;
    QCOMPARE(indent.length.value, 1.75);
    QCOMPARE(int(indent.length.unit), int(KoSvgText::CssLengthPercentage::Em));
    QVERIFY(indent.hanging);
    QVERIFY(indent.eachLine);
    QVERIFY(indent != defaults);

    KoSvgText::TextIndentInfo copy = indent;
    QVERIFY(copy == indent);
    copy.eachLine = false;
    QVERIFY(copy != indent);
}

void KoSvgTextEnumContractTest::cssLengthPercentageValueSchemaRemainsStable()
{
    using Length = KoSvgText::CssLengthPercentage;

    static_assert(std::is_class_v<Length>);
    static_assert(std::is_same_v<decltype(&Length::value), qreal Length::*>);
    static_assert(std::is_same_v<decltype(&Length::unit), Length::UnitType Length::*>);
    static_assert(std::is_default_constructible_v<Length>);
    static_assert(std::is_constructible_v<Length, qreal, Length::UnitType>);
    static_assert(std::is_same_v<decltype(&Length::operator==), bool (Length::*)(const Length &) const>);

    const Length defaults;
    QCOMPARE(defaults.value, 0.0);
    QCOMPARE(int(defaults.unit), int(Length::Absolute));

    const Length emLength(2.5, Length::Em);
    QCOMPARE(emLength.value, 2.5);
    QCOMPARE(int(emLength.unit), int(Length::Em));
    QVERIFY(emLength == Length(2.5, Length::Em));
    QVERIFY(!(emLength == Length(2.5, Length::Ex)));
    QVERIFY(!(emLength == Length(3.5, Length::Em)));

    Length copy = emLength;
    copy.value = 7.0;
    copy.unit = Length::Cap;
    QCOMPARE(emLength.value, 2.5);
    QCOMPARE(int(emLength.unit), int(Length::Em));
}

void KoSvgTextEnumContractTest::cssLengthPercentageResolutionAndFormattingSignaturesRemainStable()
{
    using Length = KoSvgText::CssLengthPercentage;
    using ConversionSignature = void (Length::*)(KoSvgText::FontMetrics, qreal, Length::UnitType);
    using DebugSignature = QDebug (*)(QDebug, const Length &);
    using WriterSignature = QString (*)(const Length &, bool);

    static_assert(
        std::is_same_v<decltype(static_cast<ConversionSignature>(&Length::convertToAbsolute)), ConversionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<DebugSignature>(&KoSvgText::operator<<)), DebugSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<WriterSignature>(&KoSvgText::writeLengthPercentage)), WriterSignature>);
}

void KoSvgTextEnumContractTest::lineHeightValueSchemaRemainsStable()
{
    using Length = KoSvgText::CssLengthPercentage;
    using LineHeight = KoSvgText::LineHeightInfo;

    static_assert(std::is_class_v<LineHeight>);
    static_assert(std::is_same_v<decltype(&LineHeight::isNormal), bool LineHeight::*>);
    static_assert(std::is_same_v<decltype(&LineHeight::isNumber), bool LineHeight::*>);
    static_assert(std::is_same_v<decltype(&LineHeight::length), Length LineHeight::*>);
    static_assert(std::is_same_v<decltype(&LineHeight::value), qreal LineHeight::*>);
    static_assert(std::is_same_v<decltype(&LineHeight::operator==), bool (LineHeight::*)(const LineHeight &) const>);

    const LineHeight defaults;
    QCOMPARE(defaults.length.value, 0.0);
    QCOMPARE(int(defaults.length.unit), int(Length::Absolute));
    QCOMPARE(defaults.value, 1.0);
    QVERIFY(!defaults.isNumber);
    QVERIFY(defaults.isNormal);

    LineHeight numeric;
    numeric.isNormal = false;
    numeric.isNumber = true;
    numeric.value = 1.5;
    numeric.length = Length(2.0, Length::Em);
    LineHeight numericCopy = numeric;
    numericCopy.length = Length(4.0, Length::Cap);
    QVERIFY(numericCopy == numeric);
    numericCopy.value = 2.0;
    QVERIFY(!(numericCopy == numeric));
    QCOMPARE(numeric.value, 1.5);
    QCOMPARE(int(numeric.length.unit), int(Length::Em));

    LineHeight measured;
    measured.isNormal = false;
    measured.length = Length(1.75, Length::Lh);
    measured.value = 8.0;
    LineHeight measuredCopy = measured;
    measuredCopy.value = 9.0;
    QVERIFY(measuredCopy == measured);
    measuredCopy.length.value = 2.0;
    QVERIFY(!(measuredCopy == measured));
    measuredCopy = measured;
    measuredCopy.isNormal = true;
    QVERIFY(!(measuredCopy == measured));
}

void KoSvgTextEnumContractTest::tabSizeValueSchemaRemainsStable()
{
    using Length = KoSvgText::CssLengthPercentage;
    using TabSize = KoSvgText::TabSizeInfo;

    static_assert(std::is_class_v<TabSize>);
    static_assert(std::is_same_v<decltype(&TabSize::extraSpacing), qreal TabSize::*>);
    static_assert(std::is_same_v<decltype(&TabSize::isNumber), bool TabSize::*>);
    static_assert(std::is_same_v<decltype(&TabSize::length), Length TabSize::*>);
    static_assert(std::is_same_v<decltype(&TabSize::value), qreal TabSize::*>);
    static_assert(std::is_same_v<decltype(&TabSize::operator==), bool (TabSize::*)(const TabSize &) const>);

    const TabSize defaults;
    QCOMPARE(defaults.extraSpacing, 0.0);
    QVERIFY(defaults.isNumber);
    QCOMPARE(defaults.length.value, 0.0);
    QCOMPARE(int(defaults.length.unit), int(Length::Absolute));
    QCOMPARE(defaults.value, 8.0);

    TabSize numeric;
    numeric.value = 4.0;
    numeric.length = Length(2.0, Length::Em);
    TabSize numericCopy = numeric;
    numericCopy.extraSpacing = 3.0;
    numericCopy.length = Length(5.0, Length::Cap);
    QVERIFY(numericCopy == numeric);
    numericCopy.value = 6.0;
    QVERIFY(!(numericCopy == numeric));
    QCOMPARE(numeric.value, 4.0);
    QCOMPARE(numeric.extraSpacing, 0.0);

    TabSize measured;
    measured.isNumber = false;
    measured.value = 12.0;
    measured.length = Length(2.5, Length::Ch);
    TabSize measuredCopy = measured;
    measuredCopy.value = 15.0;
    measuredCopy.extraSpacing = 7.0;
    QVERIFY(measuredCopy == measured);
    measuredCopy.length.unit = Length::Ic;
    QVERIFY(!(measuredCopy == measured));
    measuredCopy = measured;
    measuredCopy.isNumber = true;
    QVERIFY(!(measuredCopy == measured));
}

void KoSvgTextEnumContractTest::lineHeightAndTabSizeConversionSignaturesRemainStable()
{
    using LineHeight = KoSvgText::LineHeightInfo;
    using TabSize = KoSvgText::TabSizeInfo;
    using LineHeightDebugSignature = QDebug (*)(QDebug, const LineHeight &);
    using LineHeightParserSignature = LineHeight (*)(const QString &, const SvgLoadingContext &);
    using LineHeightWriterSignature = QString (*)(LineHeight);
    using TabSizeDebugSignature = QDebug (*)(QDebug, const TabSize &);
    using TabSizeParserSignature = TabSize (*)(const QString &, const SvgLoadingContext &);
    using TabSizeWriterSignature = QString (*)(TabSize);

    static_assert(std::is_same_v<decltype(static_cast<LineHeightDebugSignature>(&KoSvgText::operator<<)),
                                 LineHeightDebugSignature>);
    static_assert(std::is_same_v<decltype(static_cast<LineHeightParserSignature>(&KoSvgText::parseLineHeight)),
                                 LineHeightParserSignature>);
    static_assert(std::is_same_v<decltype(static_cast<LineHeightWriterSignature>(&KoSvgText::writeLineHeight)),
                                 LineHeightWriterSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<TabSizeDebugSignature>(&KoSvgText::operator<<)), TabSizeDebugSignature>);
    static_assert(std::is_same_v<decltype(static_cast<TabSizeParserSignature>(&KoSvgText::parseTabSize)),
                                 TabSizeParserSignature>);
    static_assert(std::is_same_v<decltype(static_cast<TabSizeWriterSignature>(&KoSvgText::writeTabSize)),
                                 TabSizeWriterSignature>);
}

void KoSvgTextEnumContractTest::characterTransformationValueSchemaRemainsStable()
{
    using Transformation = KoSvgText::CharTransformation;
    using OptionalScalar = boost::optional<qreal>;

    static_assert(std::is_default_constructible_v<Transformation>);
    static_assert(std::is_copy_constructible_v<Transformation>);
    static_assert(std::is_copy_assignable_v<Transformation>);
    static_assert(std::is_same_v<decltype(Transformation::xPos), OptionalScalar>);
    static_assert(std::is_same_v<decltype(Transformation::yPos), OptionalScalar>);
    static_assert(std::is_same_v<decltype(Transformation::dxPos), OptionalScalar>);
    static_assert(std::is_same_v<decltype(Transformation::dyPos), OptionalScalar>);
    static_assert(std::is_same_v<decltype(Transformation::rotate), OptionalScalar>);

    Transformation empty;
    QVERIFY(!empty.xPos);
    QVERIFY(!empty.yPos);
    QVERIFY(!empty.dxPos);
    QVERIFY(!empty.dyPos);
    QVERIFY(!empty.rotate);

    Transformation source;
    source.xPos = 1.25;
    source.yPos = -2.5;
    source.dxPos = 3.75;
    source.dyPos = -4.5;
    source.rotate = 0.625;

    const Transformation copied(source);
    Transformation assigned;
    assigned = source;
    source.xPos = 9.0;
    assigned.dyPos = -11.0;

    QCOMPARE(*copied.xPos, 1.25);
    QCOMPARE(*copied.yPos, -2.5);
    QCOMPARE(*copied.dxPos, 3.75);
    QCOMPARE(*copied.dyPos, -4.5);
    QCOMPARE(*copied.rotate, 0.625);
    QCOMPARE(*assigned.xPos, 1.25);
    QCOMPARE(*assigned.dyPos, -11.0);
    QCOMPARE(*source.xPos, 9.0);
    QCOMPARE(*source.dyPos, -4.5);
}

void KoSvgTextEnumContractTest::characterTransformationOperationSignaturesRemainStable()
{
    using Transformation = KoSvgText::CharTransformation;
    using MergeSignature = void (Transformation::*)(const Transformation &);
    using PredicateSignature = bool (Transformation::*)() const;
    using PointSignature = QPointF (Transformation::*)() const;
    using EqualitySignature = bool (Transformation::*)(const Transformation &) const;
    using DebugSignature = QDebug (*)(QDebug, const Transformation &);

    static_assert(std::is_same_v<decltype(&Transformation::mergeInParentTransformation), MergeSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::isNull), PredicateSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::startsNewChunk), PredicateSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::hasRelativeOffset), PredicateSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::absolutePos), PointSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::relativeOffset), PointSignature>);
    static_assert(std::is_same_v<decltype(&Transformation::operator==), EqualitySignature>);
    static_assert(std::is_same_v<decltype(static_cast<DebugSignature>(&KoSvgText::operator<<)), DebugSignature>);
}

void KoSvgTextEnumContractTest::resolutionHandlerValueSchemaRemainsStable()
{
    using Handler = KoSvgText::ResolutionHandler;

    static_assert(std::is_copy_constructible_v<Handler>);
    static_assert(std::is_same_v<decltype(Handler::xRes), qreal>);
    static_assert(std::is_same_v<decltype(Handler::yRes), qreal>);
    static_assert(std::is_same_v<decltype(Handler::roundToPixelHorizontal), bool>);
    static_assert(std::is_same_v<decltype(Handler::roundToPixelVertical), bool>);
    static_assert(std::is_same_v<decltype(Handler::freeTypePixel), const qreal>);
    static_assert(std::is_same_v<decltype(Handler::pointInInch), const qreal>);

    const Handler defaults;
    QCOMPARE(defaults.xRes, 72.0);
    QCOMPARE(defaults.yRes, 72.0);
    QVERIFY(!defaults.roundToPixelHorizontal);
    QVERIFY(!defaults.roundToPixelVertical);
    QCOMPARE(defaults.freeTypePixel, 64.0);
    QCOMPARE(defaults.pointInInch, 72.0);

    Handler configured(144.0, 96.0, true, false);
    const Handler copied(configured);
    configured.xRes = 288.0;
    configured.roundToPixelVertical = true;

    QCOMPARE(copied.xRes, 144.0);
    QCOMPARE(copied.yRes, 96.0);
    QVERIFY(copied.roundToPixelHorizontal);
    QVERIFY(!copied.roundToPixelVertical);
    QCOMPARE(configured.xRes, 288.0);
    QVERIFY(configured.roundToPixelVertical);
}

void KoSvgTextEnumContractTest::resolutionHandlerConversionSignaturesRemainStable()
{
    using Handler = KoSvgText::ResolutionHandler;
    using FactorSignature = qreal (Handler::*)(bool) const;
    using TransformSignature = QTransform (Handler::*)() const;

    static_assert(std::is_default_constructible_v<Handler>);
    static_assert(std::is_constructible_v<Handler, qreal, qreal, bool, bool>);
    static_assert(std::is_same_v<decltype(&Handler::freeTypePixelToPointFactor), FactorSignature>);
    static_assert(std::is_same_v<decltype(&Handler::freeTypeToPixelTransform), TransformSignature>);
    static_assert(std::is_same_v<decltype(&Handler::freeTypeToPointTransform), TransformSignature>);
    static_assert(std::is_same_v<decltype(&Handler::pointToPixelFactor), FactorSignature>);
    static_assert(std::is_same_v<decltype(&Handler::pointToPixel), TransformSignature>);
    static_assert(std::is_same_v<decltype(&Handler::pixelToPointFactor), FactorSignature>);
    static_assert(std::is_same_v<decltype(&Handler::pixelToPoint), TransformSignature>);
    static_assert(std::is_same_v<decltype(std::declval<const Handler &>().freeTypePixelToPointFactor()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const Handler &>().pointToPixelFactor()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const Handler &>().pixelToPointFactor()), qreal>);
}

void KoSvgTextEnumContractTest::resolutionHandlerAdjustmentSignaturesRemainStable()
{
    using Handler = KoSvgText::ResolutionHandler;
    using PointSignature = QPointF (Handler::*)(QPointF) const;
    using RectSignature = QRectF (Handler::*)(QRectF) const;
    using OffsetSignature = QPointF (Handler::*)(QPointF, QPointF) const;

    static_assert(std::is_same_v<decltype(static_cast<PointSignature>(&Handler::adjust)), PointSignature>);
    static_assert(std::is_same_v<decltype(static_cast<RectSignature>(&Handler::adjust)), RectSignature>);
    static_assert(std::is_same_v<decltype(&Handler::adjustFloor), PointSignature>);
    static_assert(std::is_same_v<decltype(&Handler::adjustCeil), PointSignature>);
    static_assert(std::is_same_v<decltype(&Handler::adjustWithOffset), OffsetSignature>);
}

void KoSvgTextEnumContractTest::graphicsContextStyleOrdinalsRemainStable()
{
    static_assert(std::is_class_v<SvgGraphicsContext>);
    static_assert(std::is_enum_v<SvgGraphicsContext::StyleType>);

    QCOMPARE(int(SvgGraphicsContext::None), 0);
    QCOMPARE(int(SvgGraphicsContext::Solid), 1);
    QCOMPARE(int(SvgGraphicsContext::Complex), 2);
    QCOMPARE(int(SvgGraphicsContext::Inherit), 3);
}

void KoSvgTextEnumContractTest::graphicsContextFillAndStrokeStateKeepsPublicTypes()
{
    static_assert(
        std::is_same_v<decltype(&SvgGraphicsContext::fillType), SvgGraphicsContext::StyleType SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::fillRule), Qt::FillRule SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::fillColor), QColor SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::fillId), QString SvgGraphicsContext::*>);
    static_assert(
        std::is_same_v<decltype(&SvgGraphicsContext::strokeType), SvgGraphicsContext::StyleType SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::strokeId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::stroke), KoShapeStrokeSP SvgGraphicsContext::*>);
}

void KoSvgTextEnumContractTest::graphicsContextClipTransformAndColorStateKeepsPublicTypes()
{
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::filterId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::clipPathId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::clipMaskId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::clipRule), Qt::FillRule SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::opacity), qreal SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::matrix), QTransform SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::currentColor), QColor SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::xmlBaseDir), QString SvgGraphicsContext::*>);
}

void KoSvgTextEnumContractTest::graphicsContextSpatialVisibilityAndResolutionStateKeepsPublicTypes()
{
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::preserveWhitespace), bool SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::currentBoundingBox), QRectF SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::forcePercentage), bool SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::viewboxTransform), QTransform SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::display), bool SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::visible), bool SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::isResolutionFrame), bool SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::pixelsPerInch), qreal SvgGraphicsContext::*>);
}

void KoSvgTextEnumContractTest::graphicsContextMarkerTextAndPaintStateKeepsPublicTypes()
{
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::markerStartId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::markerMidId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::markerEndId), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::autoFillMarkers), bool SvgGraphicsContext::*>);
    static_assert(
        std::is_same_v<decltype(&SvgGraphicsContext::textProperties), KoSvgTextProperties SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::shapeInsideValue), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::shapeSubtractValue), QString SvgGraphicsContext::*>);
    static_assert(std::is_same_v<decltype(&SvgGraphicsContext::paintOrder), QString SvgGraphicsContext::*>);
}

void KoSvgTextEnumContractTest::textPropertyValueTypeAndLifetimeSchemaRemainsStable()
{
    using Properties = KoSvgTextProperties;
    using AssignmentSignature = Properties &(Properties::*)(const Properties &);
    using EqualitySignature = bool (Properties::*)(const Properties &) const;

    static_assert(std::is_class_v<Properties>);
    static_assert(std::is_default_constructible_v<Properties>);
    static_assert(std::is_copy_constructible_v<Properties>);
    static_assert(std::is_destructible_v<Properties>);
    static_assert(
        std::is_same_v<decltype(static_cast<AssignmentSignature>(&Properties::operator=)), AssignmentSignature>);
    static_assert(std::is_same_v<decltype(static_cast<EqualitySignature>(&Properties::operator==)), EqualitySignature>);

    QVERIFY(true);
}

void KoSvgTextEnumContractTest::textPropertyValueMapSchemaRemainsStable()
{
    using Properties = KoSvgTextProperties;
    using PropertyId = Properties::PropertyId;
    using SetPropertySignature = void (Properties::*)(PropertyId, const QVariant &);
    using HasPropertySignature = bool (Properties::*)(PropertyId) const;
    using PropertySignature = QVariant (Properties::*)(PropertyId, const QVariant &) const;
    using RemovePropertySignature = void (Properties::*)(PropertyId);
    using PropertyOrDefaultSignature = QVariant (Properties::*)(PropertyId) const;
    using PropertiesSignature = QList<PropertyId> (Properties::*)() const;
    using IsEmptySignature = bool (Properties::*)() const;
    using DefaultPropertiesSignature = const Properties &(*)();
    using PropertyIsBlockOnlySignature = bool (*)(PropertyId);
    using PropertyIsInheritableSignature = bool (Properties::*)(PropertyId) const;

    static_assert(
        std::is_same_v<decltype(static_cast<SetPropertySignature>(&Properties::setProperty)), SetPropertySignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<HasPropertySignature>(&Properties::hasProperty)), HasPropertySignature>);
    static_assert(std::is_same_v<decltype(static_cast<PropertySignature>(&Properties::property)), PropertySignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Properties &>().property(std::declval<PropertyId>())), QVariant>);
    static_assert(std::is_same_v<decltype(static_cast<RemovePropertySignature>(&Properties::removeProperty)),
                                 RemovePropertySignature>);
    static_assert(std::is_same_v<decltype(static_cast<PropertyOrDefaultSignature>(&Properties::propertyOrDefault)),
                                 PropertyOrDefaultSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<PropertiesSignature>(&Properties::properties)), PropertiesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<IsEmptySignature>(&Properties::isEmpty)), IsEmptySignature>);
    static_assert(std::is_same_v<decltype(static_cast<DefaultPropertiesSignature>(&Properties::defaultProperties)),
                                 DefaultPropertiesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PropertyIsBlockOnlySignature>(&Properties::propertyIsBlockOnly)),
                                 PropertyIsBlockOnlySignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<PropertyIsInheritableSignature>(&Properties::propertyIsInheritable)),
                       PropertyIsInheritableSignature>);

    QVERIFY(true);
}

void KoSvgTextEnumContractTest::textPropertyInheritanceAndScalingSchemaRemainsStable()
{
    using Properties = KoSvgTextProperties;
    using PropertyId = Properties::PropertyId;
    using ResetSignature = void (Properties::*)();
    using InheritSignature = void (Properties::*)(const Properties &, bool, bool);
    using ResolveSignature = void (Properties::*)(KoSvgText::FontMetrics, qreal, bool);
    using InheritsSignature = bool (Properties::*)(PropertyId, const Properties &) const;
    using PredicateSignature = bool (Properties::*)() const;
    using SetInheritedSignature = void (Properties::*)(const Properties &);
    using ScaleSignature = void (Properties::*)(double, double);
    using OwnPropertiesSignature = Properties (Properties::*)(const Properties &, bool) const;

    static_assert(std::is_same_v<decltype(static_cast<ResetSignature>(&Properties::resetNonInheritableToDefault)),
                                 ResetSignature>);
    static_assert(std::is_same_v<decltype(static_cast<InheritSignature>(&Properties::inheritFrom)), InheritSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<Properties &>().inheritFrom(std::declval<const Properties &>())), void>);
    static_assert(
        std::is_same_v<decltype(static_cast<ResolveSignature>(&Properties::resolveRelativeValues)), ResolveSignature>);
    static_assert(std::is_same_v<decltype(std::declval<Properties &>().resolveRelativeValues()), void>);
    static_assert(
        std::is_same_v<decltype(static_cast<InheritsSignature>(&Properties::inheritsProperty)), InheritsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PredicateSignature>(&Properties::hasNonInheritableProperties)),
                                 PredicateSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetInheritedSignature>(&Properties::setAllButNonInheritableProperties)),
                       SetInheritedSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScaleSignature>(&Properties::scaleAbsoluteValues)), ScaleSignature>);
    static_assert(std::is_same_v<decltype(std::declval<Properties &>().scaleAbsoluteValues()), void>);
    static_assert(std::is_same_v<decltype(static_cast<OwnPropertiesSignature>(&Properties::ownProperties)),
                                 OwnPropertiesSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Properties &>().ownProperties(std::declval<const Properties &>())),
                       Properties>);

    QVERIFY(true);
}

void KoSvgTextEnumContractTest::textPropertyInterchangeAndFontProjectionSchemaRemainsStable()
{
    using Properties = KoSvgTextProperties;
    using ParseSignature = void (Properties::*)(const SvgLoadingContext &, const QString &, const QString &);
    using ConvertSignature = QMap<QString, QString> (Properties::*)() const;
    using SupportedAttributesSignature = QStringList (*)();
    using FontFeaturesSignature = QStringList (Properties::*)(int, int) const;
    using CssFontInfoSignature = KoCSSFontInfo (Properties::*)() const;
    using GenerateFontSignature = QFont (Properties::*)() const;

    static_assert(
        std::is_same_v<decltype(static_cast<ParseSignature>(&Properties::parseSvgTextAttribute)), ParseSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ConvertSignature>(&Properties::convertToSvgTextAttributes)),
                                 ConvertSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ConvertSignature>(&Properties::convertParagraphProperties)),
                                 ConvertSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SupportedAttributesSignature>(&Properties::supportedXmlAttributes)),
                       SupportedAttributesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FontFeaturesSignature>(&Properties::fontFeaturesForText)),
                                 FontFeaturesSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<CssFontInfoSignature>(&Properties::cssFontInfo)), CssFontInfoSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<GenerateFontSignature>(&Properties::generateFont)), GenerateFontSignature>);

    QVERIFY(true);
}

void KoSvgTextEnumContractTest::textPropertyMetricsAndSurfaceSchemaRemainsStable()
{
    using Properties = KoSvgTextProperties;
    using XHeightSignature = qreal (Properties::*)() const;
    using MetricsSignature = KoSvgText::FontMetrics (Properties::*)(bool, bool) const;
    using ApplyLineHeightSignature = KoSvgText::FontMetrics (Properties::*)(KoSvgText::FontMetrics) const;
    using BackgroundSignature = QSharedPointer<KoShapeBackground> (Properties::*)() const;
    using StrokeSignature = KoShapeStrokeModelSP (Properties::*)() const;
    using FontSizeSignature = KoSvgText::CssLengthPercentage (Properties::*)() const;
    using SetFontSizeSignature = void (Properties::*)(KoSvgText::CssLengthPercentage);

    static_assert(std::is_same_v<decltype(static_cast<XHeightSignature>(&Properties::xHeight)), XHeightSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MetricsSignature>(&Properties::metrics)), MetricsSignature>);
    static_assert(std::is_same_v<decltype(std::declval<const Properties &>().metrics()), KoSvgText::FontMetrics>);
    static_assert(std::is_same_v<decltype(static_cast<ApplyLineHeightSignature>(&Properties::applyLineHeight)),
                                 ApplyLineHeightSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BackgroundSignature>(&Properties::background)), BackgroundSignature>);
    static_assert(std::is_same_v<decltype(static_cast<StrokeSignature>(&Properties::stroke)), StrokeSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FontSizeSignature>(&Properties::fontSize)), FontSizeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetFontSizeSignature>(&Properties::setFontSize)), SetFontSizeSignature>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KoSvgTextEnumContractTest)

#include "KoSvgTextEnumContractTest.moc"
