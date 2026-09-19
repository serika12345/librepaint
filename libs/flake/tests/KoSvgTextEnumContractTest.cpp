/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "svg/SvgUtil.h"
#include "text/KoSvgText.h"

#include <QFlags>
#include <QTest>

#include <array>
#include <utility>

class KoSvgTextEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void flagAliasesPreserveIndependentOptions();
    void fontKeywordTablesRemainStable();
    void autoValuesDistinguishAutomaticAndNumericValues();
    void autoLengthsPreserveUnitsAndValueSemantics();
    void fontStylesCompareSlantOnlyForOblique();
    void textTransformsOwnIndependentFlags();
    void textIndentOwnsLengthAndLineFlags();
    void cssLengthPercentageDefaultsAndValues();
    void lineHeightDefaultsAndValues();
    void tabSizeDefaultsAndValues();
    void characterTransformationDefaultsAndValues();
    void resolutionHandlerDefaultsAndValues();
};

void KoSvgTextEnumContractTest::flagAliasesPreserveIndependentOptions()
{
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

void KoSvgTextEnumContractTest::cssLengthPercentageDefaultsAndValues()
{
    using Length = KoSvgText::CssLengthPercentage;

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

void KoSvgTextEnumContractTest::lineHeightDefaultsAndValues()
{
    using Length = KoSvgText::CssLengthPercentage;
    using LineHeight = KoSvgText::LineHeightInfo;

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

void KoSvgTextEnumContractTest::tabSizeDefaultsAndValues()
{
    using Length = KoSvgText::CssLengthPercentage;
    using TabSize = KoSvgText::TabSizeInfo;

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

void KoSvgTextEnumContractTest::characterTransformationDefaultsAndValues()
{
    using Transformation = KoSvgText::CharTransformation;

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

void KoSvgTextEnumContractTest::resolutionHandlerDefaultsAndValues()
{
    using Handler = KoSvgText::ResolutionHandler;

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

QTEST_GUILESS_MAIN(KoSvgTextEnumContractTest)

#include "KoSvgTextEnumContractTest.moc"
