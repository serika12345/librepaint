/*
 * SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgText.h"

namespace KoSvgText
{

WritingMode parseWritingMode(const QString &value)
{
    return (value == "tb-rl" || value == "tb" || value == "vertical-rl") ? VerticalRL
        : (value == "vertical-lr")                                       ? VerticalLR
                                                                         : HorizontalTB;
}

Direction parseDirection(const QString &value)
{
    return value == "rtl" ? DirectionRightToLeft : DirectionLeftToRight;
}

UnicodeBidi parseUnicodeBidi(const QString &value)
{
    return value == "embed"           ? BidiEmbed
        : value == "bidi-override"    ? BidiOverride
        : value == "isolate"          ? BidiIsolate
        : value == "isolate-override" ? BidiIsolateOverride
        : value == "plaintext"        ? BidiPlainText
                                      : BidiNormal;
}

TextOrientation parseTextOrientation(const QString &value)
{
    return value == "upright" ? OrientationUpright : value == "sideways" ? OrientationSideWays : OrientationMixed;
}

TextOrientation parseTextOrientationFromGlyphOrientation(AutoValue value)
{
    if (value.isAuto) {
        return OrientationMixed;
    } else if (value.customValue == 0) {
        return OrientationUpright;
    } else if (value.customValue == 90) {
        return OrientationSideWays;
    } else {
        return OrientationMixed;
    }
}

TextAnchor parseTextAnchor(const QString &value)
{
    return value == "middle" ? AnchorMiddle : value == "end" ? AnchorEnd : AnchorStart;
}

LengthAdjust parseLengthAdjust(const QString &value)
{
    return value == "spacingAndGlyphs" ? LengthAdjustSpacingAndGlyphs : LengthAdjustSpacing;
}

QString writeWritingMode(WritingMode value, bool svg1_1)
{
    if (svg1_1) {
        return value == VerticalRL ? "tb" : "lr";
    } else {
        return value == VerticalRL ? "vertical-rl" : value == VerticalLR ? "vertical-lr" : "horizontal-tb";
    }
}

QString writeDirection(Direction value)
{
    return value == DirectionRightToLeft ? "rtl" : "ltr";
}

QString writeUnicodeBidi(UnicodeBidi value)
{
    return value == BidiEmbed          ? "embed"
        : value == BidiOverride        ? "bidi-override"
        : value == BidiIsolate         ? "isolate"
        : value == BidiIsolateOverride ? "isolate-override"
        : value == BidiPlainText       ? "plaintext"
                                       : "normal";
}

QString writeTextOrientation(TextOrientation orientation)
{
    return orientation == OrientationUpright ? "upright" : orientation == OrientationSideWays ? "sideways" : "mixed";
}

QString writeTextAnchor(TextAnchor value)
{
    return value == AnchorEnd ? "end" : value == AnchorMiddle ? "middle" : "start";
}

QString writeLengthAdjust(LengthAdjust value)
{
    return value == LengthAdjustSpacingAndGlyphs ? "spacingAndGlyphs" : "spacing";
}

TextPathMethod parseTextPathMethod(const QString &value)
{
    return value == "stretch" ? TextPathStretch : TextPathAlign;
}

TextPathSpacing parseTextPathSpacing(const QString &value)
{
    return value == "auto" ? TextPathAuto : TextPathExact;
}

TextPathSide parseTextPathSide(const QString &value)
{
    return value == "left" ? TextPathSideLeft : TextPathSideRight;
}

QString writeTextPathMethod(TextPathMethod value)
{
    return value == TextPathAlign ? "align" : "stretch";
}

QString writeTextPathSpacing(TextPathSpacing value)
{
    return value == TextPathAuto ? "auto" : "exact";
}

QString writeTextPathSide(TextPathSide value)
{
    return value == TextPathSideLeft ? "left" : "right";
}

WordBreak parseWordBreak(const QString &value)
{
    return value == "keep-all" ? WordBreakKeepAll : value == "break-all" ? WordBreakBreakAll : WordBreakNormal;
}

LineBreak parseLineBreak(const QString &value)
{
    return value == "loose"   ? LineBreakLoose
        : value == "normal"   ? LineBreakNormal
        : value == "strict"   ? LineBreakStrict
        : value == "anywhere" ? LineBreakAnywhere
                              : LineBreakAuto;
}

TextAlign parseTextAlign(const QString &value)
{
    return value == "end"         ? AlignEnd
        : value == "left"         ? AlignLeft
        : value == "right"        ? AlignRight
        : value == "center"       ? AlignCenter
        : value == "justify"      ? AlignJustify
        : value == "justify-all"  ? AlignJustify
        : value == "match-parent" ? AlignMatchParent
        : value == "auto"         ? AlignLastAuto
                                  : AlignStart;
}

QString writeWordBreak(WordBreak value)
{
    return value == WordBreakKeepAll ? "keep-all" : value == WordBreakBreakAll ? "break-all" : "normal";
}

QString writeLineBreak(LineBreak value)
{
    return value == LineBreakLoose   ? "loose"
        : value == LineBreakNormal   ? "normal"
        : value == LineBreakStrict   ? "strict"
        : value == LineBreakAnywhere ? "anywhere"
                                     : "auto";
}

QString writeTextAlign(TextAlign value)
{
    return value == AlignEnd        ? "end"
        : value == AlignLeft        ? "left"
        : value == AlignRight       ? "right"
        : value == AlignCenter      ? "center"
        : value == AlignJustify     ? "justify"
        : value == AlignMatchParent ? "match-parent"
        : value == AlignLastAuto    ? "auto"
                                    : "start";
}

TextRendering parseTextRendering(const QString &value)
{
    if (value == "optimizeSpeed") {
        return RenderingOptimizeSpeed;
    } else if (value == "optimizeLegibility") {
        return RenderingOptimizeLegibility;
    } else if (value == "geometricPrecision") {
        return RenderingGeometricPrecision;
    }
    return RenderingAuto;
}

QString writeTextRendering(TextRendering value)
{
    if (value == RenderingOptimizeSpeed) {
        return "optimizeSpeed";
    } else if (value == RenderingOptimizeLegibility) {
        return "optimizeLegibility";
    } else if (value == RenderingGeometricPrecision) {
        return "geometricPrecision";
    } else {
        return "auto";
    }
}

} // namespace KoSvgText
