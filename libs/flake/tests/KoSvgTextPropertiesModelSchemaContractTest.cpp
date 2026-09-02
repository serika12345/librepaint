/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/lager/KoSvgTextPropertiesModel.h"

#include <QTest>

#include <type_traits>

namespace
{

using Model = KoSvgTextPropertiesModel;
using PropertyState = Model::PropertyState;
using HangComma = Model::HangComma;

#define ASSERT_PROPERTY_CURSOR(type, name)                                                                             \
    static_assert(std::is_same_v<decltype(Model::LAGER_QT(name)), lager::cursor<type>>);                               \
    static_assert(std::is_same_v<decltype(static_cast<const type &(Model::*)() const>(&Model::name)),                  \
                                 const type &(Model::*)() const>);                                                     \
    static_assert(std::is_same_v<decltype(static_cast<void (Model::*)(const type &)>(&Model::set##name)),              \
                                 void (Model::*)(const type &)>);                                                      \
    static_assert(std::is_same_v<decltype(static_cast<void (Model::*)(const type &)>(&Model::name##Changed)),          \
                                 void (Model::*)(const type &)>)

} // namespace

class KoSvgTextPropertiesModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void propertyModelFontFoundationCursorSchemaRemainsStable();
    void propertyModelFontFeatureCursorSchemaRemainsStable();
    void propertyModelWritingAndLayoutCursorSchemaRemainsStable();
    void propertyModelDecorationAndBaselineCursorSchemaRemainsStable();
    void propertyModelFlowAndSurfaceCursorSchemaRemainsStable();
    void propertyModelIdentityEnumAndResolutionSchemaRemainsStable();
    void propertyModelCursorDataMemberSchemaRemainsStable();
    void propertyModelChildModelMemberSchemaRemainsStable();
    void propertyModelChildModelAccessorSignaturesRemainStable();
    void propertyModelSurfaceSignalSignaturesRemainStable();
};

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelFontFoundationCursorSchemaRemainsStable()
{
    ASSERT_PROPERTY_CURSOR(PropertyState, fontSizeState);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontStyleState);
    ASSERT_PROPERTY_CURSOR(int, fontWeight);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontWeightState);
    ASSERT_PROPERTY_CURSOR(int, fontWidth);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontWidthState);
    ASSERT_PROPERTY_CURSOR(bool, fontOpticalSizeLink);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontOpticalSizeLinkState);
    ASSERT_PROPERTY_CURSOR(QStringList, fontFamilies);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontFamiliesState);
    ASSERT_PROPERTY_CURSOR(QVariantMap, axisValues);
    ASSERT_PROPERTY_CURSOR(PropertyState, axisValuesState);
    ASSERT_PROPERTY_CURSOR(qreal, fontSizeAdjust);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontSizeAdjustState);
    ASSERT_PROPERTY_CURSOR(QString, language);
    ASSERT_PROPERTY_CURSOR(PropertyState, languageState);
    ASSERT_PROPERTY_CURSOR(PropertyState, letterSpacingState);
    ASSERT_PROPERTY_CURSOR(PropertyState, wordSpacingState);
    ASSERT_PROPERTY_CURSOR(PropertyState, lineHeightState);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelFontFeatureCursorSchemaRemainsStable()
{
    ASSERT_PROPERTY_CURSOR(QVariantMap, fontFeatureSettings);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontFeatureSettingsState);
    ASSERT_PROPERTY_CURSOR(bool, fontKerning);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontKerningState);
    ASSERT_PROPERTY_CURSOR(bool, fontSynthesisStyle);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontSynthesisStyleState);
    ASSERT_PROPERTY_CURSOR(bool, fontSynthesisWeight);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontSynthesisWeightState);
    ASSERT_PROPERTY_CURSOR(int, fontVariantCaps);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontVariantCapsState);
    ASSERT_PROPERTY_CURSOR(int, fontVariantPosition);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontVariantPositionState);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontVariantEastAsianState);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontVariantLigaturesState);
    ASSERT_PROPERTY_CURSOR(PropertyState, fontVariantNumericState);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelWritingAndLayoutCursorSchemaRemainsStable()
{
    ASSERT_PROPERTY_CURSOR(int, writingMode);
    ASSERT_PROPERTY_CURSOR(PropertyState, writingModeState);
    ASSERT_PROPERTY_CURSOR(int, direction);
    ASSERT_PROPERTY_CURSOR(PropertyState, directionState);
    ASSERT_PROPERTY_CURSOR(int, unicodeBidi);
    ASSERT_PROPERTY_CURSOR(PropertyState, unicodeBidiState);
    ASSERT_PROPERTY_CURSOR(int, textAlignAll);
    ASSERT_PROPERTY_CURSOR(PropertyState, textAlignAllState);
    ASSERT_PROPERTY_CURSOR(int, textAlignLast);
    ASSERT_PROPERTY_CURSOR(PropertyState, textAlignLastState);
    ASSERT_PROPERTY_CURSOR(int, textAnchor);
    ASSERT_PROPERTY_CURSOR(PropertyState, textAnchorState);
    ASSERT_PROPERTY_CURSOR(int, lineBreak);
    ASSERT_PROPERTY_CURSOR(PropertyState, lineBreakState);
    ASSERT_PROPERTY_CURSOR(int, wordBreak);
    ASSERT_PROPERTY_CURSOR(PropertyState, wordBreakState);
    ASSERT_PROPERTY_CURSOR(PropertyState, textIndentState);
    ASSERT_PROPERTY_CURSOR(PropertyState, tabSizeState);
    ASSERT_PROPERTY_CURSOR(PropertyState, textTransformState);
    ASSERT_PROPERTY_CURSOR(int, textRendering);
    ASSERT_PROPERTY_CURSOR(PropertyState, textRenderingState);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelDecorationAndBaselineCursorSchemaRemainsStable()
{
    ASSERT_PROPERTY_CURSOR(bool, textDecorationUnderline);
    ASSERT_PROPERTY_CURSOR(bool, textDecorationOverline);
    ASSERT_PROPERTY_CURSOR(bool, textDecorationLineThrough);
    ASSERT_PROPERTY_CURSOR(PropertyState, textDecorationLineState);
    ASSERT_PROPERTY_CURSOR(int, textDecorationStyle);
    ASSERT_PROPERTY_CURSOR(PropertyState, textDecorationStyleState);
    ASSERT_PROPERTY_CURSOR(QColor, textDecorationColor);
    ASSERT_PROPERTY_CURSOR(PropertyState, textDecorationColorState);
    ASSERT_PROPERTY_CURSOR(int, textDecorationUnderlinePosHorizontal);
    ASSERT_PROPERTY_CURSOR(int, textDecorationUnderlinePosVertical);
    ASSERT_PROPERTY_CURSOR(PropertyState, textDecorationUnderlinePositionState);
    ASSERT_PROPERTY_CURSOR(int, alignmentBaseline);
    ASSERT_PROPERTY_CURSOR(PropertyState, alignmentBaselineState);
    ASSERT_PROPERTY_CURSOR(int, dominantBaseline);
    ASSERT_PROPERTY_CURSOR(PropertyState, dominantBaselineState);
    ASSERT_PROPERTY_CURSOR(int, baselineShiftMode);
    ASSERT_PROPERTY_CURSOR(PropertyState, baselineShiftState);
    ASSERT_PROPERTY_CURSOR(HangComma, hangingPunctuationComma);
    ASSERT_PROPERTY_CURSOR(bool, hangingPunctuationLast);
    ASSERT_PROPERTY_CURSOR(PropertyState, hangingPunctuationState);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelFlowAndSurfaceCursorSchemaRemainsStable()
{
    ASSERT_PROPERTY_CURSOR(int, textCollapse);
    ASSERT_PROPERTY_CURSOR(PropertyState, textCollapseState);
    ASSERT_PROPERTY_CURSOR(int, textWrap);
    ASSERT_PROPERTY_CURSOR(PropertyState, textWrapState);
    ASSERT_PROPERTY_CURSOR(PropertyState, shapeMarginState);
    ASSERT_PROPERTY_CURSOR(PropertyState, shapePaddingState);
    ASSERT_PROPERTY_CURSOR(bool, spanSelection);
    ASSERT_PROPERTY_CURSOR(bool, enabled);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelIdentityEnumAndResolutionSchemaRemainsStable()
{
    static_assert(std::is_class_v<Model>);
    static_assert(std::is_enum_v<PropertyState>);
    static_assert(std::is_enum_v<HangComma>);
    static_assert(Model::PropertyUnset == 0);
    static_assert(Model::PropertySet == 1);
    static_assert(Model::PropertyTriState == 2);
    static_assert(Model::PropertyInherited == 3);
    static_assert(Model::NoHang == 0);
    static_assert(Model::AllowHang == 1);
    static_assert(Model::ForceHang == 2);
    static_assert(std::is_constructible_v<Model, lager::cursor<KoSvgTextPropertyData>>);
    static_assert(std::is_same_v<decltype(static_cast<qreal (Model::*)(bool)>(&Model::resolvedFontSize)),
                                 qreal (Model::*)(bool)>);
    static_assert(
        std::is_same_v<decltype(static_cast<qreal (Model::*)(bool)>(&Model::resolvedXHeight)), qreal (Model::*)(bool)>);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelCursorDataMemberSchemaRemainsStable()
{
    static_assert(
        std::is_same_v<decltype(Model::baselineShiftValueData), lager::cursor<KoSvgText::CssLengthPercentage>>);
    static_assert(std::is_same_v<decltype(Model::commonProperties), lager::cursor<KoSvgTextProperties>>);
    static_assert(std::is_same_v<decltype(Model::cssFontStyleData), lager::cursor<KoSvgText::CssFontStyleData>>);
    static_assert(std::is_same_v<decltype(Model::fontSizeData), lager::cursor<KoSvgText::CssLengthPercentage>>);
    static_assert(
        std::is_same_v<decltype(Model::fontVariantEastAsianData), lager::cursor<KoSvgText::FontFeatureEastAsian>>);
    static_assert(
        std::is_same_v<decltype(Model::fontVariantLigaturesData), lager::cursor<KoSvgText::FontFeatureLigatures>>);
    static_assert(
        std::is_same_v<decltype(Model::fontVariantNumericData), lager::cursor<KoSvgText::FontFeatureNumeric>>);
    static_assert(std::is_same_v<decltype(Model::letterSpacingData), lager::cursor<KoSvgText::CssLengthPercentage>>);
    static_assert(std::is_same_v<decltype(Model::lineHeightData), lager::cursor<KoSvgText::LineHeightInfo>>);
    static_assert(std::is_same_v<decltype(Model::shapeMarginData), lager::cursor<KoSvgText::CssLengthPercentage>>);
    static_assert(std::is_same_v<decltype(Model::shapePaddingData), lager::cursor<KoSvgText::CssLengthPercentage>>);
    static_assert(std::is_same_v<decltype(Model::tabSizeData), lager::cursor<KoSvgText::TabSizeInfo>>);
    static_assert(std::is_same_v<decltype(Model::textData), lager::cursor<KoSvgTextPropertyData>>);
    static_assert(std::is_same_v<decltype(Model::textIndentData), lager::cursor<KoSvgText::TextIndentInfo>>);
    static_assert(std::is_same_v<decltype(Model::textTransformData), lager::cursor<KoSvgText::TextTransformInfo>>);
    static_assert(std::is_same_v<decltype(Model::wordSpacingData), lager::cursor<KoSvgText::CssLengthPercentage>>);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelChildModelMemberSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(Model::baselineShiftValueModel), CssLengthPercentageModel>);
    static_assert(std::is_same_v<decltype(Model::cssFontStyleModel), CssFontStyleModel>);
    static_assert(std::is_same_v<decltype(Model::fontSizeModel), CssLengthPercentageModel>);
    static_assert(std::is_same_v<decltype(Model::fontVariantEastAsianModel), FontVariantEastAsianModel>);
    static_assert(std::is_same_v<decltype(Model::fontVariantLigaturesModel), FontVariantLigaturesModel>);
    static_assert(std::is_same_v<decltype(Model::fontVariantNumericModel), FontVariantNumericModel>);
    static_assert(std::is_same_v<decltype(Model::letterSpacingModel), CssLengthPercentageModel>);
    static_assert(std::is_same_v<decltype(Model::lineHeightModel), LineHeightModel>);
    static_assert(std::is_same_v<decltype(Model::shapeMarginModel), CssLengthPercentageModel>);
    static_assert(std::is_same_v<decltype(Model::shapePaddingModel), CssLengthPercentageModel>);
    static_assert(std::is_same_v<decltype(Model::tabSizeModel), TabSizeModel>);
    static_assert(std::is_same_v<decltype(Model::textIndentModel), TextIndentModel>);
    static_assert(std::is_same_v<decltype(Model::textTransformModel), TextTransformModel>);
    static_assert(std::is_same_v<decltype(Model::wordSpacingModel), CssLengthPercentageModel>);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelChildModelAccessorSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&Model::baselineShiftValue), CssLengthPercentageModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::fontStyle), CssFontStyleModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::fontVariantEastAsian), FontVariantEastAsianModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::fontVariantLigatures), FontVariantLigaturesModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::fontVariantNumeric), FontVariantNumericModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::letterSpacing), CssLengthPercentageModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::lineHeight), LineHeightModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::shapeMargin), CssLengthPercentageModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::shapePadding), CssLengthPercentageModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::tabSize), TabSizeModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::textIndent), TextIndentModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::textTransform), TextTransformModel *(Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::wordSpacing), CssLengthPercentageModel *(Model::*)()>);
}

void KoSvgTextPropertiesModelSchemaContractTest::propertyModelSurfaceSignalSignaturesRemainStable()
{
#define ASSERT_VOID_SIGNAL(name) static_assert(std::is_same_v<decltype(&Model::name), void (Model::*)()>)
    ASSERT_VOID_SIGNAL(baselineShiftValueChanged);
    ASSERT_VOID_SIGNAL(fontSizeChanged);
    ASSERT_VOID_SIGNAL(fontStyleChanged);
    ASSERT_VOID_SIGNAL(fontVariantEastAsianChanged);
    ASSERT_VOID_SIGNAL(fontVariantLigaturesChanged);
    ASSERT_VOID_SIGNAL(fontVariantNumericChanged);
    ASSERT_VOID_SIGNAL(letterSpacingChanged);
    ASSERT_VOID_SIGNAL(lineHeightChanged);
    ASSERT_VOID_SIGNAL(shapeMarginChanged);
    ASSERT_VOID_SIGNAL(shapePaddingChanged);
    ASSERT_VOID_SIGNAL(tabSizeChanged);
    ASSERT_VOID_SIGNAL(textIndentChanged);
    ASSERT_VOID_SIGNAL(textPropertyChanged);
    ASSERT_VOID_SIGNAL(textTransformChanged);
    ASSERT_VOID_SIGNAL(wordSpacingChanged);
#undef ASSERT_VOID_SIGNAL
}

QTEST_GUILESS_MAIN(KoSvgTextPropertiesModelSchemaContractTest)

#include "KoSvgTextPropertiesModelSchemaContractTest.moc"
