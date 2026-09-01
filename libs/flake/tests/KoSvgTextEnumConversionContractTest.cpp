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

QTEST_GUILESS_MAIN(KoSvgTextEnumConversionContractTest)

#include "KoSvgTextEnumConversionContractTest.moc"
