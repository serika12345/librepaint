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

QTEST_GUILESS_MAIN(KoSvgTextPropertiesModelSchemaContractTest)

#include "KoSvgTextPropertiesModelSchemaContractTest.moc"
