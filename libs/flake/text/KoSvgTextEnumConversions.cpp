/*
 * SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgText.h"

#include <QDebug>

#include <algorithm>
#include <array>
#include <functional>

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

FontFeatureLigatures parseFontFeatureLigatures(const QString &value, FontFeatureLigatures features)
{
    if (value == "common-ligatures") {
        features.commonLigatures = true;
    } else if (value == "no-common-ligatures") {
        features.commonLigatures = false;
    } else if (value == "discretionary-ligatures") {
        features.discretionaryLigatures = true;
    } else if (value == "no-discretionary-ligatures") {
        features.discretionaryLigatures = false;
    } else if (value == "historical-ligatures") {
        features.historicalLigatures = true;
    } else if (value == "no-historical-ligatures") {
        features.historicalLigatures = false;
    } else if (value == "contextual") {
        features.contextualAlternates = true;
    } else if (value == "no-contextual") {
        features.contextualAlternates = false;
    } else if (value == "none") {
        features.commonLigatures = false;
        features.discretionaryLigatures = false;
        features.historicalLigatures = false;
        features.contextualAlternates = false;
    }
    return features;
}

QString writeFontFeatureLigatures(const FontFeatureLigatures &feature)
{
    if (feature.commonLigatures && !feature.discretionaryLigatures && !feature.historicalLigatures
        && feature.commonLigatures) {
        return "normal";
    }
    if (!feature.commonLigatures && !feature.discretionaryLigatures && !feature.historicalLigatures
        && !feature.commonLigatures) {
        return "none";
    }
    QStringList list;
    if (!feature.commonLigatures) {
        list << "no-common-ligatures";
    }
    if (feature.discretionaryLigatures) {
        list << "discretionary-ligatures";
    }
    if (feature.historicalLigatures) {
        list << "historical-ligatures";
    }
    if (!feature.contextualAlternates) {
        list << "no-contextual";
    }
    return list.join(" ");
}

FontFeatureNumeric parseFontFeatureNumeric(const QString &value, FontFeatureNumeric features)
{
    if (value == "lining-nums") {
        features.style = NumericFigureStyleLining;
    } else if (value == "oldstyle-nums") {
        features.style = NumericFigureStyleOld;
    } else if (value == "proportional-nums") {
        features.spacing = NumericFigureSpacingProportional;
    } else if (value == "tabular-nums") {
        features.spacing = NumericFigureSpacingTabular;
    } else if (value == "diagonal-fractions") {
        features.fractions = NumericFractionsDiagonal;
    } else if (value == "stacked-fractions") {
        features.fractions = NumericFractionsStacked;
    } else if (value == "ordinal") {
        features.ordinals = true;
    } else if (value == "slashed-zero") {
        features.slashedZero = true;
    } else {
        features = FontFeatureNumeric();
    }
    return features;
}

QString writeFontFeatureNumeric(const FontFeatureNumeric &feature)
{
    if (feature == FontFeatureNumeric()) {
        return "normal";
    }
    QStringList list;

    if (feature.style == NumericFigureStyleLining) {
        list << "lining-nums";
    } else if (feature.style == NumericFigureStyleOld) {
        list << "oldstyle-nums";
    }

    if (feature.spacing == NumericFigureSpacingProportional) {
        list << "proportional-nums";
    } else if (feature.spacing == NumericFigureSpacingTabular) {
        list << "tabular-nums";
    }

    if (feature.fractions == NumericFractionsDiagonal) {
        list << "diagonal-fractions";
    } else if (feature.fractions == NumericFractionsStacked) {
        list << "stacked-fractions";
    }

    if (feature.ordinals) {
        list << "ordinal";
    }

    if (feature.slashedZero) {
        list << "slashed-zero";
    }

    return list.join(" ");
}

FontFeatureEastAsian parseFontFeatureEastAsian(const QString &value, FontFeatureEastAsian features)
{
    if (value == "jis78") {
        features.variant = EastAsianJis78;
    } else if (value == "jis83") {
        features.variant = EastAsianJis83;
    } else if (value == "jis90") {
        features.variant = EastAsianJis90;
    } else if (value == "jis04") {
        features.variant = EastAsianJis04;
    } else if (value == "simplified") {
        features.variant = EastAsianSimplified;
    } else if (value == "traditional") {
        features.variant = EastAsianTraditional;
    } else if (value == "full-width") {
        features.width = EastAsianFullWidth;
    } else if (value == "proportional-width") {
        features.width = EastAsianProportionalWidth;
    } else if (value == "ruby") {
        features.ruby = true;
    } else {
        features = FontFeatureEastAsian();
    }
    return features;
}

QString writeFontFeatureEastAsian(const FontFeatureEastAsian &feature)
{
    if (feature == FontFeatureEastAsian()) {
        return "normal";
    }
    QStringList list;

    if (feature.variant == EastAsianJis78) {
        list << "jis78";
    } else if (feature.variant == EastAsianJis83) {
        list << "jis83";
    } else if (feature.variant == EastAsianJis90) {
        list << "jis90";
    } else if (feature.variant == EastAsianJis04) {
        list << "jis04";
    } else if (feature.variant == EastAsianSimplified) {
        list << "simplified";
    } else if (feature.variant == EastAsianTraditional) {
        list << "traditional";
    }

    if (feature.width == EastAsianFullWidth) {
        list << "full-width";
    } else if (feature.width == EastAsianProportionalWidth) {
        list << "proportional-width";
    }

    if (feature.ruby) {
        list << "ruby";
    }

    return list.join(" ");
}

FontFeaturePosition parseFontFeaturePosition(const QString &value, FontFeaturePosition feature)
{
    return value == "super" ? PositionSuper
        : value == "sub"    ? PositionSub
        : value == "normal" ? PositionNormal
                            : feature;
}

QString writeFontFeaturePosition(const FontFeaturePosition &value)
{
    return value == PositionSuper ? "super" : value == PositionSub ? "sub" : "normal";
}

FontFeatureCaps parseFontFeatureCaps(const QString &value, FontFeatureCaps feature)
{
    return value == "small-caps"     ? CapsSmall
        : value == "all-small-caps"  ? CapsAllSmall
        : value == "petite-caps"     ? CapsPetite
        : value == "all-petite-caps" ? CapsAllPetite
        : value == "unicase"         ? CapsUnicase
        : value == "titling-caps"    ? CapsTitling
        : value == "normal"          ? CapsNormal
                                     : feature;
}

QString writeFontFeatureCaps(const FontFeatureCaps &value)
{
    return value == CapsSmall    ? "small-caps"
        : value == CapsAllSmall  ? "all-small-caps"
        : value == CapsPetite    ? "petite-caps"
        : value == CapsAllPetite ? "all-petite-caps"
        : value == CapsUnicase   ? "unicase"
        : value == CapsTitling   ? "titling-caps"
                                 : "normal";
}

QStringList fontFeaturesPosition(const FontFeaturePosition &feature, const int start, const int end)
{
    const QString length = QString("[%1:%2]").arg(start).arg(end);
    QString tag = feature == PositionSuper ? "sups" : feature == PositionSub ? "subs" : QString();
    if (!tag.isEmpty()) {
        tag += length;
        tag += "=1";
    }
    return tag.isEmpty() ? QStringList() : QStringList(tag);
}

QStringList fontFeaturesCaps(const FontFeatureCaps &feature, const int start, const int end)
{
    QStringList list;
    const QString length = QString("[%1:%2]").arg(start).arg(end);

    switch (feature) {
    case CapsSmall:
        list << "smcp" + length + "=1";
        break;
    case CapsAllSmall:
        list << "smcp" + length + "=1";
        list << "c2sc" + length + "=1";
        break;
    case CapsPetite:
        list << "pcap" + length + "=1";
        break;
    case CapsAllPetite:
        list << "pcap" + length + "=1";
        list << "c2pc" + length + "=1";
        break;
    case CapsUnicase:
        list << "unic" + length + "=1";
        break;
    case CapsTitling:
        list << "titl" + length + "=1";
        break;
    default:
        break;
    }

    return list;
}

int parseCSSFontWeight(const QString &value, int currentWeight)
{
    int weight = 400;

    // map svg weight to qt weight
    // svg value        qt value
    // 100,200,300      1, 17, 33
    // 400              50          (normal)
    // 500,600          58,66
    // 700              75          (bold)
    // 800,900          87,99
    static constexpr std::array<int, 9> svgFontWeights = {100, 200, 300, 400, 500, 600, 700, 800, 900};

    if (value == "bold")
        weight = 700;
    else if (value == "bolder") {
        const auto it = std::upper_bound(svgFontWeights.begin(), svgFontWeights.end(), currentWeight);

        weight = it != svgFontWeights.end() ? *it : svgFontWeights.back();
    } else if (value == "lighter") {
        const auto it =
            std::upper_bound(svgFontWeights.rbegin(), svgFontWeights.rend(), currentWeight, std::greater<int>());

        weight = it != svgFontWeights.rend() ? *it : svgFontWeights.front();
    } else {
        bool ok = false;

        // try to read numerical weight value
        const int parsed = value.toInt(&ok, 10);
        if (ok) {
            weight = qBound(0, parsed, 1000);
        }
    }
    return weight;
}

CssFontStyleData parseFontStyle(const QString &value)
{
    CssFontStyleData slant;
    QStringList params = value.split(" ");
    if (!params.isEmpty()) {
        QString style = params.first();
        slant.style = style == "italic" ? QFont::StyleItalic
            : style == "oblique"        ? QFont::StyleOblique
                                        : QFont::StyleNormal;
    }
    if (params.size() > 1) {
        QString angle = params.last();
        if (angle.endsWith("deg")) {
            angle.chop(3);
            slant.slantValue.isAuto = false;
            slant.slantValue.customValue = angle.toDouble();
        }
    }
    return slant;
}

QString writeFontStyle(CssFontStyleData value)
{
    QString style = value.style == QFont::StyleItalic ? "italic"
        : value.style == QFont::StyleOblique          ? "oblique"
                                                      : "normal";
    if (value.style == QFont::StyleOblique && !value.slantValue.isAuto) {
        style.append(QString(" ") + QString::number(value.slantValue.customValue) + QString("deg"));
    }
    return style;
}

Baseline parseBaseline(const QString &value)
{
    return value == "use-script"                                                          ? BaselineUseScript
        : value == "no-change"                                                            ? BaselineNoChange
        : value == "reset-size"                                                           ? BaselineResetSize
        : value == "ideographic"                                                          ? BaselineIdeographic
        : value == "alphabetic"                                                           ? BaselineAlphabetic
        : value == "hanging"                                                              ? BaselineHanging
        : value == "mathematical"                                                         ? BaselineMathematical
        : value == "central"                                                              ? BaselineCentral
        : value == "middle"                                                               ? BaselineMiddle
        : value == "baseline"                                                             ? BaselineDominant
        : (value == "text-after-edge" || value == "after-edge" || value == "text-bottom") ? BaselineTextBottom
        : (value == "text-before-edge" || value == "before-edge" || value == "text-top")  ? BaselineTextTop
                                                                                          : BaselineAuto;
}

BaselineShiftMode parseBaselineShiftMode(const QString &value)
{
    return value == "baseline" ? ShiftNone
        : value == "sub"       ? ShiftSub
        : value == "super"     ? ShiftSuper
        : value == "top"       ? ShiftLineTop
        : value == "bottom"    ? ShiftLineBottom
                               : ShiftLengthPercentage;
}

QString writeDominantBaseline(Baseline value)
{
    return value == BaselineUseScript   ? "use-script"
        : value == BaselineNoChange     ? "no-change"
        : value == BaselineResetSize    ? "reset-size"
        : value == BaselineIdeographic  ? "ideographic"
        : value == BaselineAlphabetic   ? "alphabetic"
        : value == BaselineHanging      ? "hanging"
        : value == BaselineMathematical ? "mathematical"
        : value == BaselineCentral      ? "central"
        : value == BaselineMiddle       ? "middle"
        : value == BaselineTextBottom   ? "text-bottom"
                                        : // text-after-edge in svg 1.1
        value == BaselineTextTop ? "text-top"
                                   : // text-before-edge in svg 1.1
        "auto";
}

QString writeAlignmentBaseline(Baseline value)
{
    return value == BaselineDominant    ? "baseline"
        : value == BaselineIdeographic  ? "ideographic"
        : value == BaselineAlphabetic   ? "alphabetic"
        : value == BaselineHanging      ? "hanging"
        : value == BaselineMathematical ? "mathematical"
        : value == BaselineCentral      ? "central"
        : value == BaselineMiddle       ? "middle"
        : value == BaselineTextBottom   ? "text-bottom"
                                        : // text-after-edge in svg 1.1
        value == BaselineTextTop ? "text-top"
                                   : // text-before-edge in svg 1.1
        "auto";
}

bool whiteSpaceValueToLongHands(const QString &value,
                                TextSpaceCollapse &collapseMethod,
                                TextWrap &wrapMethod,
                                TextSpaceTrims &trimMethod)
{
    bool result = true;
    if (value == "pre") {
        collapseMethod = Preserve;
        wrapMethod = NoWrap;
        trimMethod = TrimNone;
    } else if (value == "nowrap") {
        collapseMethod = Collapse;
        wrapMethod = NoWrap;
        trimMethod = TrimNone;
    } else if (value == "pre-wrap") {
        collapseMethod = Preserve;
        wrapMethod = Wrap;
        trimMethod = TrimNone;
    } else if (value == "pre-wrap") {
        collapseMethod = BreakSpaces;
        wrapMethod = Wrap;
        trimMethod = TrimNone;
    } else if (value == "pre-line") {
        collapseMethod = PreserveBreaks;
        wrapMethod = Wrap;
        trimMethod = TrimNone;
    } else { // "normal"
        if (value != "normal") {
            result = false;
        }
        collapseMethod = Collapse;
        wrapMethod = Wrap;
        trimMethod = TrimNone;
    }
    return result;
}

bool xmlSpaceToLongHands(const QString &value, TextSpaceCollapse &collapseMethod)
{
    bool result = true;

    if (value == "preserve") {
        /*
         * "When xml:space="preserve", the SVG user agent will do the following
         * using a copy of the original character data content. It will convert
         * all newline and tab characters into space characters. Then, it will
         * draw all space characters, including leading, trailing and multiple
         * contiguous space characters."
         */
        collapseMethod = PreserveSpaces;
    } else {
        /*
         * "When xml:space="default", the SVG user agent will do the following
         * using a copy of the original character data content. First, it will
         * remove all newline characters. Then it will convert all tab
         * characters into space characters. Then, it will strip off all leading
         * and trailing space characters. Then, all contiguous space characters
         * will be consolidated."
         */
        if (value != "default") {
            result = false;
        }
        collapseMethod = Collapse;
    }

    return result;
}

QString writeWhiteSpaceValue(TextSpaceCollapse collapseMethod, TextWrap wrapMethod, TextSpaceTrims trimMethod)
{
    Q_UNUSED(trimMethod);
    if (wrapMethod != NoWrap) {
        if (collapseMethod == Preserve) {
            return "pre-wrap";
        } else if (collapseMethod == PreserveBreaks) {
            return "pre-line";
        } else if (collapseMethod == BreakSpaces) {
            return "break-spaces";
        } else {
            return "normal";
        }

    } else {
        if (collapseMethod == Preserve) {
            return "pre";
        } else {
            return "nowrap";
        }
    }
}

QString writeXmlSpace(TextSpaceCollapse collapseMethod)
{
    return collapseMethod == PreserveSpaces ? "preserve" : "default";
}

TextTransformInfo parseTextTransform(const QString &value)
{
    TextTransformInfo textTransform;
    const QStringList values = value.toLower().split(" ");
    Q_FOREACH (const QString &param, values) {
        if (param == "capitalize") {
            textTransform.capitals = TextTransformCapitalize;
        } else if (param == "uppercase") {
            textTransform.capitals = TextTransformUppercase;
        } else if (param == "lowercase") {
            textTransform.capitals = TextTransformLowercase;
        } else if (param == "full-width") {
            textTransform.fullWidth = true;
        } else if (param == "full-size-kana") {
            textTransform.fullSizeKana = true;
        } else if (param == "none") {
            textTransform.capitals = TextTransformNone;
            textTransform.fullWidth = false;
            textTransform.fullSizeKana = false;
        } else {
            qWarning() << "Unknown parameter in text-transform" << param;
        }
    }
    return textTransform;
}

QString writeTextTransform(const TextTransformInfo textTransform)
{
    QStringList values;
    if (textTransform.capitals == TextTransformNone && !textTransform.fullWidth && !textTransform.fullSizeKana) {
        values.append("none");
    } else {
        if (textTransform.capitals == TextTransformLowercase) {
            values.append("lowercase");
        } else if (textTransform.capitals == TextTransformUppercase) {
            values.append("uppercase");
        } else if (textTransform.capitals == TextTransformCapitalize) {
            values.append("capitalize");
        }
        if (textTransform.fullWidth) {
            values.append("full-width");
        }
        if (textTransform.fullSizeKana) {
            values.append("full-size-kana");
        }
    }
    return values.join(" ");
}

} // namespace KoSvgText
