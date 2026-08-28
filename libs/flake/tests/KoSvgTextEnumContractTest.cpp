/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgText.h"
#include "text/KoSvgTextProperties.h"

#include <QFlags>
#include <QTest>

#include <array>
#include <type_traits>

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

    const KoSvgText::TextDecorations decorations =
        KoSvgText::DecorationUnderline | KoSvgText::DecorationLineThrough;
    QVERIFY(decorations.testFlag(KoSvgText::DecorationUnderline));
    QVERIFY(!decorations.testFlag(KoSvgText::DecorationOverline));
    QVERIFY(decorations.testFlag(KoSvgText::DecorationLineThrough));

    const KoSvgText::TextSpaceTrims trims =
        KoSvgText::DiscardBefore | KoSvgText::DiscardAfter;
    QVERIFY(!trims.testFlag(KoSvgText::TrimInner));
    QVERIFY(trims.testFlag(KoSvgText::DiscardBefore));
    QVERIFY(trims.testFlag(KoSvgText::DiscardAfter));

    const KoSvgText::HangingPunctuations hanging =
        KoSvgText::HangFirst | KoSvgText::HangEnd | KoSvgText::HangForce;
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

QTEST_GUILESS_MAIN(KoSvgTextEnumContractTest)

#include "KoSvgTextEnumContractTest.moc"
