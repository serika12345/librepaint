/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgText.h"

#include <QTest>

#include <type_traits>

namespace
{

using FontMetrics = KoSvgText::FontMetrics;

void assignDistinctValues(FontMetrics &metrics)
{
    metrics.isVertical = true;
    metrics.fontSize = 101;
    metrics.zeroAdvance = 102;
    metrics.spaceAdvance = 103;
    metrics.ideographicAdvance = 104;
    metrics.xHeight = 105;
    metrics.capHeight = 106;
    metrics.subScriptOffset = qMakePair<qint32, qint32>(-107, 108);
    metrics.superScriptOffset = qMakePair<qint32, qint32>(109, -110);
    metrics.ascender = 111;
    metrics.descender = -112;
    metrics.lineGap = 113;
    metrics.alphabeticBaseline = 114;
    metrics.mathematicalBaseline = -115;
    metrics.ideographicUnderBaseline = 116;
    metrics.ideographicCenterBaseline = -117;
    metrics.ideographicOverBaseline = 118;
    metrics.ideographicFaceUnderBaseline = -119;
    metrics.ideographicFaceOverBaseline = 120;
    metrics.hangingBaseline = -121;
    metrics.underlineOffset = 122;
    metrics.underlineThickness = 123;
    metrics.lineThroughOffset = -124;
    metrics.lineThroughThickness = 125;
    metrics.caretRun = -126;
    metrics.caretRise = 127;
    metrics.caretOffset = -128;
}

void compareMembers(const FontMetrics &actual, const FontMetrics &expected)
{
    QCOMPARE(actual.isVertical, expected.isVertical);
    QCOMPARE(actual.fontSize, expected.fontSize);
    QCOMPARE(actual.zeroAdvance, expected.zeroAdvance);
    QCOMPARE(actual.spaceAdvance, expected.spaceAdvance);
    QCOMPARE(actual.ideographicAdvance, expected.ideographicAdvance);
    QCOMPARE(actual.xHeight, expected.xHeight);
    QCOMPARE(actual.capHeight, expected.capHeight);
    QCOMPARE(actual.subScriptOffset.first, expected.subScriptOffset.first);
    QCOMPARE(actual.subScriptOffset.second, expected.subScriptOffset.second);
    QCOMPARE(actual.superScriptOffset.first, expected.superScriptOffset.first);
    QCOMPARE(actual.superScriptOffset.second, expected.superScriptOffset.second);
    QCOMPARE(actual.ascender, expected.ascender);
    QCOMPARE(actual.descender, expected.descender);
    QCOMPARE(actual.lineGap, expected.lineGap);
    QCOMPARE(actual.alphabeticBaseline, expected.alphabeticBaseline);
    QCOMPARE(actual.mathematicalBaseline, expected.mathematicalBaseline);
    QCOMPARE(actual.ideographicUnderBaseline, expected.ideographicUnderBaseline);
    QCOMPARE(actual.ideographicCenterBaseline, expected.ideographicCenterBaseline);
    QCOMPARE(actual.ideographicOverBaseline, expected.ideographicOverBaseline);
    QCOMPARE(actual.ideographicFaceUnderBaseline, expected.ideographicFaceUnderBaseline);
    QCOMPARE(actual.ideographicFaceOverBaseline, expected.ideographicFaceOverBaseline);
    QCOMPARE(actual.hangingBaseline, expected.hangingBaseline);
    QCOMPARE(actual.underlineOffset, expected.underlineOffset);
    QCOMPARE(actual.underlineThickness, expected.underlineThickness);
    QCOMPARE(actual.lineThroughOffset, expected.lineThroughOffset);
    QCOMPARE(actual.lineThroughThickness, expected.lineThroughThickness);
    QCOMPARE(actual.caretRun, expected.caretRun);
    QCOMPARE(actual.caretRise, expected.caretRise);
    QCOMPARE(actual.caretOffset, expected.caretOffset);
}

} // namespace

class KoSvgTextFontMetricsValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void orientationAndAdvancesDefaultToIndependentScalarValues();
    void heightsAndScriptOffsetsPreserveSignedValues();
    void baselinesPreserveIndependentSignedValues();
    void decorationMetricsPreserveIndependentSignedValues();
    void caretMetricsAndCopiesOwnAllValues();
};

void KoSvgTextFontMetricsValueContractTest::orientationAndAdvancesDefaultToIndependentScalarValues()
{
    static_assert(std::is_default_constructible_v<FontMetrics>);
    static_assert(std::is_same_v<decltype(FontMetrics::isVertical), bool>);
    static_assert(std::is_same_v<decltype(FontMetrics::fontSize), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::zeroAdvance), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::spaceAdvance), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicAdvance), qint32>);

    FontMetrics metrics;
    QVERIFY(!metrics.isVertical);
    QCOMPARE(metrics.fontSize, 0);
    QCOMPARE(metrics.zeroAdvance, 0);
    QCOMPARE(metrics.spaceAdvance, 0);
    QCOMPARE(metrics.ideographicAdvance, 0);

    metrics.isVertical = true;
    metrics.fontSize = -17;
    metrics.zeroAdvance = 23;
    metrics.spaceAdvance = -31;
    metrics.ideographicAdvance = 47;

    QVERIFY(metrics.isVertical);
    QCOMPARE(metrics.fontSize, -17);
    QCOMPARE(metrics.zeroAdvance, 23);
    QCOMPARE(metrics.spaceAdvance, -31);
    QCOMPARE(metrics.ideographicAdvance, 47);
}

void KoSvgTextFontMetricsValueContractTest::heightsAndScriptOffsetsPreserveSignedValues()
{
    static_assert(std::is_same_v<decltype(FontMetrics::xHeight), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::capHeight), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::subScriptOffset), QPair<qint32, qint32>>);
    static_assert(std::is_same_v<decltype(FontMetrics::superScriptOffset), QPair<qint32, qint32>>);
    static_assert(std::is_same_v<decltype(FontMetrics::ascender), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::descender), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::lineGap), qint32>);

    FontMetrics metrics;
    QCOMPARE(metrics.xHeight, 0);
    QCOMPARE(metrics.capHeight, 0);
    QCOMPARE(metrics.subScriptOffset.first, 0);
    QCOMPARE(metrics.subScriptOffset.second, 0);
    QCOMPARE(metrics.superScriptOffset.first, 0);
    QCOMPARE(metrics.superScriptOffset.second, 0);
    QCOMPARE(metrics.ascender, 0);
    QCOMPARE(metrics.descender, 0);
    QCOMPARE(metrics.lineGap, 0);

    metrics.xHeight = 29;
    metrics.capHeight = 43;
    metrics.subScriptOffset = qMakePair<qint32, qint32>(-53, 59);
    metrics.superScriptOffset = qMakePair<qint32, qint32>(61, -67);
    metrics.ascender = 71;
    metrics.descender = -73;
    metrics.lineGap = 79;

    QCOMPARE(metrics.xHeight, 29);
    QCOMPARE(metrics.capHeight, 43);
    QCOMPARE(metrics.subScriptOffset.first, -53);
    QCOMPARE(metrics.subScriptOffset.second, 59);
    QCOMPARE(metrics.superScriptOffset.first, 61);
    QCOMPARE(metrics.superScriptOffset.second, -67);
    QCOMPARE(metrics.ascender, 71);
    QCOMPARE(metrics.descender, -73);
    QCOMPARE(metrics.lineGap, 79);
}

void KoSvgTextFontMetricsValueContractTest::baselinesPreserveIndependentSignedValues()
{
    static_assert(std::is_same_v<decltype(FontMetrics::alphabeticBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::mathematicalBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicUnderBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicCenterBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicOverBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicFaceUnderBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::ideographicFaceOverBaseline), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::hangingBaseline), qint32>);

    FontMetrics metrics;
    QCOMPARE(metrics.alphabeticBaseline, 0);
    QCOMPARE(metrics.mathematicalBaseline, 0);
    QCOMPARE(metrics.ideographicUnderBaseline, 0);
    QCOMPARE(metrics.ideographicCenterBaseline, 0);
    QCOMPARE(metrics.ideographicOverBaseline, 0);
    QCOMPARE(metrics.ideographicFaceUnderBaseline, 0);
    QCOMPARE(metrics.ideographicFaceOverBaseline, 0);
    QCOMPARE(metrics.hangingBaseline, 0);

    metrics.alphabeticBaseline = -83;
    metrics.mathematicalBaseline = 89;
    metrics.ideographicUnderBaseline = -97;
    metrics.ideographicCenterBaseline = 101;
    metrics.ideographicOverBaseline = -103;
    metrics.ideographicFaceUnderBaseline = 107;
    metrics.ideographicFaceOverBaseline = -109;
    metrics.hangingBaseline = 113;

    QCOMPARE(metrics.alphabeticBaseline, -83);
    QCOMPARE(metrics.mathematicalBaseline, 89);
    QCOMPARE(metrics.ideographicUnderBaseline, -97);
    QCOMPARE(metrics.ideographicCenterBaseline, 101);
    QCOMPARE(metrics.ideographicOverBaseline, -103);
    QCOMPARE(metrics.ideographicFaceUnderBaseline, 107);
    QCOMPARE(metrics.ideographicFaceOverBaseline, -109);
    QCOMPARE(metrics.hangingBaseline, 113);
}

void KoSvgTextFontMetricsValueContractTest::decorationMetricsPreserveIndependentSignedValues()
{
    static_assert(std::is_same_v<decltype(FontMetrics::underlineOffset), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::underlineThickness), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::lineThroughOffset), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::lineThroughThickness), qint32>);

    FontMetrics metrics;
    QCOMPARE(metrics.underlineOffset, 0);
    QCOMPARE(metrics.underlineThickness, 0);
    QCOMPARE(metrics.lineThroughOffset, 0);
    QCOMPARE(metrics.lineThroughThickness, 0);

    metrics.underlineOffset = -127;
    metrics.underlineThickness = 131;
    metrics.lineThroughOffset = -137;
    metrics.lineThroughThickness = 139;

    QCOMPARE(metrics.underlineOffset, -127);
    QCOMPARE(metrics.underlineThickness, 131);
    QCOMPARE(metrics.lineThroughOffset, -137);
    QCOMPARE(metrics.lineThroughThickness, 139);
}

void KoSvgTextFontMetricsValueContractTest::caretMetricsAndCopiesOwnAllValues()
{
    static_assert(std::is_same_v<decltype(FontMetrics::caretRun), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::caretRise), qint32>);
    static_assert(std::is_same_v<decltype(FontMetrics::caretOffset), qint32>);

    FontMetrics source;
    QCOMPARE(source.caretRun, 0);
    QCOMPARE(source.caretRise, 0);
    QCOMPARE(source.caretOffset, 0);
    assignDistinctValues(source);

    const FontMetrics expected(source);
    const FontMetrics copied(source);
    FontMetrics assigned;
    assigned = source;

    source.isVertical = false;
    source.fontSize = -201;
    source.subScriptOffset.first = 202;
    source.alphabeticBaseline = -203;
    source.underlineThickness = 204;
    source.caretRun = 211;
    source.caretRise = -223;
    source.caretOffset = 227;

    compareMembers(copied, expected);
    compareMembers(assigned, expected);

    assigned.caretOffset = 229;
    QCOMPARE(expected.caretOffset, -128);
    QCOMPARE(copied.caretOffset, -128);
    QCOMPARE(source.caretRun, 211);
    QCOMPARE(source.caretRise, -223);
    QCOMPARE(source.caretOffset, 227);
}

QTEST_GUILESS_MAIN(KoSvgTextFontMetricsValueContractTest)

#include "KoSvgTextFontMetricsValueContractTest.moc"
