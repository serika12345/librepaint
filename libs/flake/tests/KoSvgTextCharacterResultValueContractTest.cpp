/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgTextShape_p.h"

#include <QTest>

#include <type_traits>

namespace
{

CharacterResult makePopulatedResult()
{
    CharacterResult result;

    result.finalPosition = QPointF(3.0, -5.0);
    result.rotate = 0.75;
    result.hidden = true;
    result.addressable = false;
    result.middle = true;
    result.anchored_chunk = true;
    result.visualIndex = 7;
    result.plaintTextIndex = 11;

    Glyph::Outline outline;
    outline.path.addRect(QRectF(1.0, 2.0, 3.0, 4.0));
    result.glyph = outline;
    result.inkBoundingBox = QRectF(-2.0, -3.0, 8.0, 9.0);
    result.isHorizontal = false;
    result.cssPosition = QPointF(13.0, 17.0);
    result.textLengthOffset = QPointF(19.0, 23.0);
    result.textPathAndAnchoringOffset = QPointF(29.0, 31.0);
    result.dominantBaselineOffset = QPointF(37.0, 41.0);
    result.baselineOffset = QPointF(43.0, 47.0);
    result.advance = QPointF(53.0, 59.0);

    result.breakType = BreakType::HardBreak;
    result.lineEnd = LineEdgeBehaviour::Collapse;
    result.lineStart = LineEdgeBehaviour::ForceHang;
    result.justifyBefore = true;
    result.justifyAfter = true;
    result.isHanging = true;
    result.textLengthApplied = true;
    result.overflowWrap = true;
    result.tabSize = 61.0;

    result.extraFontScaling = 1.25;
    result.fontHalfLeading = -2.5;
    result.metrics.fontSize = 67;
    result.metrics.ascender = 71;
    result.scaledHalfLeading = -3.5;
    result.scaledAscent = 73.0;
    result.scaledDescent = -79.0;
    result.fontStyle = QFont::StyleOblique;
    result.fontWeight = 650;

    result.cursorInfo.caret = QLineF(QPointF(2.0, 3.0), QPointF(5.0, 7.0));
    result.cursorInfo.graphemeIndices = {83, 89};
    result.cursorInfo.offsets = {QPointF(97.0, 101.0), QPointF(103.0, 107.0)};
    result.cursorInfo.rtl = true;
    result.cursorInfo.isWordBoundary = true;
    result.cursorInfo.color = QColor(109, 113, 127, 131);
    result.anchor = KoSvgText::AnchorEnd;
    result.direction = KoSvgText::DirectionRightToLeft;

    return result;
}

void compareCopiedValues(const CharacterResult &actual, const CharacterResult &expected)
{
    QCOMPARE(actual.finalPosition, expected.finalPosition);
    QCOMPARE(actual.rotate, expected.rotate);
    QCOMPARE(actual.hidden, expected.hidden);
    QCOMPARE(actual.addressable, expected.addressable);
    QCOMPARE(actual.visualIndex, expected.visualIndex);
    QCOMPARE(actual.plaintTextIndex, expected.plaintTextIndex);

    QCOMPARE(actual.glyph.index(), expected.glyph.index());
    QVERIFY(std::get<Glyph::Outline>(actual.glyph).path == std::get<Glyph::Outline>(expected.glyph).path);
    QCOMPARE(actual.inkBoundingBox, expected.inkBoundingBox);
    QCOMPARE(actual.advance, expected.advance);

    QCOMPARE(int(actual.breakType), int(expected.breakType));
    QCOMPARE(int(actual.lineEnd), int(expected.lineEnd));
    QCOMPARE(int(actual.lineStart), int(expected.lineStart));
    QCOMPARE(actual.tabSize.has_value(), expected.tabSize.has_value());
    QCOMPARE(*actual.tabSize, *expected.tabSize);

    QCOMPARE(actual.extraFontScaling, expected.extraFontScaling);
    QCOMPARE(actual.fontHalfLeading, expected.fontHalfLeading);
    QCOMPARE(actual.metrics.fontSize, expected.metrics.fontSize);
    QCOMPARE(actual.metrics.ascender, expected.metrics.ascender);
    QCOMPARE(actual.scaledAscent, expected.scaledAscent);
    QCOMPARE(actual.scaledDescent, expected.scaledDescent);

    QCOMPARE(actual.cursorInfo.caret, expected.cursorInfo.caret);
    QCOMPARE(actual.cursorInfo.graphemeIndices, expected.cursorInfo.graphemeIndices);
    QCOMPARE(actual.cursorInfo.offsets, expected.cursorInfo.offsets);
    QCOMPARE(actual.cursorInfo.rtl, expected.cursorInfo.rtl);
    QCOMPARE(actual.cursorInfo.isWordBoundary, expected.cursorInfo.isWordBoundary);
    QCOMPARE(actual.cursorInfo.color, expected.cursorInfo.color);
    QCOMPARE(int(actual.anchor), int(expected.anchor));
    QCOMPARE(int(actual.direction), int(expected.direction));
}

} // namespace

class KoSvgTextCharacterResultValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void positionVisibilityAndIndicesDefaultAndAssignIndependently();
    void glyphGeometryAndOffsetsDefaultAndAssignIndependently();
    void lineBreakAndJustificationStateDefaultAndAssignIndependently();
    void fontMetricsAndScaledValuesPreserveExplicitValues();
    void cursorDirectionAndCopiesOwnNestedValues();
};

void KoSvgTextCharacterResultValueContractTest::positionVisibilityAndIndicesDefaultAndAssignIndependently()
{
    static_assert(std::is_default_constructible_v<CharacterResult>);
    static_assert(std::is_copy_constructible_v<CharacterResult>);
    static_assert(std::is_copy_assignable_v<CharacterResult>);
    static_assert(std::is_destructible_v<CharacterResult>);
    static_assert(std::is_same_v<decltype(CharacterResult::finalPosition), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::rotate), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::hidden), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::addressable), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::middle), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::anchored_chunk), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::visualIndex), int>);
    static_assert(std::is_same_v<decltype(CharacterResult::plaintTextIndex), int>);

    CharacterResult result;
    QCOMPARE(result.finalPosition, QPointF());
    QCOMPARE(result.rotate, 0.0);
    QVERIFY(!result.hidden);
    QVERIFY(result.addressable);
    QVERIFY(!result.middle);
    QVERIFY(!result.anchored_chunk);
    QCOMPARE(result.visualIndex, -1);
    QCOMPARE(result.plaintTextIndex, -1);

    result.finalPosition = QPointF(2.0, -3.0);
    result.rotate = -0.5;
    result.hidden = true;
    result.addressable = false;
    result.middle = true;
    result.anchored_chunk = true;
    result.visualIndex = 5;
    result.plaintTextIndex = 7;

    QCOMPARE(result.finalPosition, QPointF(2.0, -3.0));
    QCOMPARE(result.rotate, -0.5);
    QVERIFY(result.hidden);
    QVERIFY(!result.addressable);
    QVERIFY(result.middle);
    QVERIFY(result.anchored_chunk);
    QCOMPARE(result.visualIndex, 5);
    QCOMPARE(result.plaintTextIndex, 7);
}

void KoSvgTextCharacterResultValueContractTest::glyphGeometryAndOffsetsDefaultAndAssignIndependently()
{
    static_assert(std::is_same_v<decltype(CharacterResult::glyph), Glyph::Variant>);
    static_assert(std::is_same_v<decltype(CharacterResult::inkBoundingBox), QRectF>);
    static_assert(std::is_same_v<decltype(CharacterResult::isHorizontal), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::cssPosition), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::textLengthOffset), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::textPathAndAnchoringOffset), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::dominantBaselineOffset), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::baselineOffset), QPointF>);
    static_assert(std::is_same_v<decltype(CharacterResult::advance), QPointF>);

    CharacterResult result;
    QVERIFY(std::holds_alternative<std::monostate>(result.glyph));
    QCOMPARE(result.inkBoundingBox, QRectF());
    QVERIFY(result.isHorizontal);
    QCOMPARE(result.cssPosition, QPointF());
    QCOMPARE(result.textLengthOffset, QPointF());
    QCOMPARE(result.textPathAndAnchoringOffset, QPointF());
    QCOMPARE(result.dominantBaselineOffset, QPointF());
    QCOMPARE(result.baselineOffset, QPointF());
    QCOMPARE(result.advance, QPointF());

    result.glyph = Glyph::Outline();
    result.inkBoundingBox = QRectF(1.0, 2.0, 3.0, 4.0);
    result.isHorizontal = false;
    result.cssPosition = QPointF(5.0, 7.0);
    result.textLengthOffset = QPointF(11.0, 13.0);
    result.textPathAndAnchoringOffset = QPointF(17.0, 19.0);
    result.dominantBaselineOffset = QPointF(23.0, 29.0);
    result.baselineOffset = QPointF(31.0, 37.0);
    result.advance = QPointF(41.0, 43.0);

    QVERIFY(std::holds_alternative<Glyph::Outline>(result.glyph));
    QCOMPARE(result.inkBoundingBox, QRectF(1.0, 2.0, 3.0, 4.0));
    QVERIFY(!result.isHorizontal);
    QCOMPARE(result.cssPosition, QPointF(5.0, 7.0));
    QCOMPARE(result.textLengthOffset, QPointF(11.0, 13.0));
    QCOMPARE(result.textPathAndAnchoringOffset, QPointF(17.0, 19.0));
    QCOMPARE(result.dominantBaselineOffset, QPointF(23.0, 29.0));
    QCOMPARE(result.baselineOffset, QPointF(31.0, 37.0));
    QCOMPARE(result.advance, QPointF(41.0, 43.0));
}

void KoSvgTextCharacterResultValueContractTest::lineBreakAndJustificationStateDefaultAndAssignIndependently()
{
    static_assert(std::is_same_v<decltype(CharacterResult::breakType), BreakType>);
    static_assert(std::is_same_v<decltype(CharacterResult::lineEnd), LineEdgeBehaviour>);
    static_assert(std::is_same_v<decltype(CharacterResult::lineStart), LineEdgeBehaviour>);
    static_assert(std::is_same_v<decltype(CharacterResult::justifyBefore), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::justifyAfter), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::isHanging), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::textLengthApplied), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::overflowWrap), bool>);
    static_assert(std::is_same_v<decltype(CharacterResult::tabSize), std::optional<qreal>>);

    CharacterResult result;
    QCOMPARE(int(result.breakType), int(BreakType::NoBreak));
    QCOMPARE(int(result.lineEnd), int(LineEdgeBehaviour::NoChange));
    QCOMPARE(int(result.lineStart), int(LineEdgeBehaviour::NoChange));
    QVERIFY(!result.justifyBefore);
    QVERIFY(!result.justifyAfter);
    QVERIFY(!result.isHanging);
    QVERIFY(!result.textLengthApplied);
    QVERIFY(!result.overflowWrap);
    QVERIFY(!result.tabSize.has_value());

    result.breakType = BreakType::SoftBreak;
    result.lineEnd = LineEdgeBehaviour::ConditionallyHang;
    result.lineStart = LineEdgeBehaviour::ForceHang;
    result.justifyBefore = true;
    result.justifyAfter = true;
    result.isHanging = true;
    result.textLengthApplied = true;
    result.overflowWrap = true;
    result.tabSize = -47.5;

    QCOMPARE(int(result.breakType), int(BreakType::SoftBreak));
    QCOMPARE(int(result.lineEnd), int(LineEdgeBehaviour::ConditionallyHang));
    QCOMPARE(int(result.lineStart), int(LineEdgeBehaviour::ForceHang));
    QVERIFY(result.justifyBefore);
    QVERIFY(result.justifyAfter);
    QVERIFY(result.isHanging);
    QVERIFY(result.textLengthApplied);
    QVERIFY(result.overflowWrap);
    QVERIFY(result.tabSize.has_value());
    QCOMPARE(*result.tabSize, -47.5);
}

void KoSvgTextCharacterResultValueContractTest::fontMetricsAndScaledValuesPreserveExplicitValues()
{
    static_assert(std::is_same_v<decltype(CharacterResult::extraFontScaling), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::fontHalfLeading), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::metrics), KoSvgText::FontMetrics>);
    static_assert(std::is_same_v<decltype(CharacterResult::scaledHalfLeading), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::scaledAscent), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::scaledDescent), qreal>);
    static_assert(std::is_same_v<decltype(CharacterResult::fontStyle), QFont::Style>);
    static_assert(std::is_same_v<decltype(CharacterResult::fontWeight), int>);

    CharacterResult result;
    QCOMPARE(result.extraFontScaling, 1.0);
    QCOMPARE(result.metrics.fontSize, 0);
    QCOMPARE(result.metrics.ascender, 0);
    QCOMPARE(result.scaledHalfLeading, 0.0);
    QCOMPARE(result.scaledAscent, 0.0);
    QCOMPARE(result.scaledDescent, 0.0);
    QCOMPARE(int(result.fontStyle), int(QFont::StyleNormal));
    QCOMPARE(result.fontWeight, 400);

    result.extraFontScaling = 1.75;
    result.fontHalfLeading = -2.25;
    result.metrics.fontSize = 53;
    result.metrics.ascender = -59;
    result.scaledHalfLeading = 3.5;
    result.scaledAscent = 61.0;
    result.scaledDescent = -67.0;
    result.fontStyle = QFont::StyleItalic;
    result.fontWeight = 725;

    QCOMPARE(result.extraFontScaling, 1.75);
    QCOMPARE(result.fontHalfLeading, -2.25);
    QCOMPARE(result.metrics.fontSize, 53);
    QCOMPARE(result.metrics.ascender, -59);
    QCOMPARE(result.scaledHalfLeading, 3.5);
    QCOMPARE(result.scaledAscent, 61.0);
    QCOMPARE(result.scaledDescent, -67.0);
    QCOMPARE(int(result.fontStyle), int(QFont::StyleItalic));
    QCOMPARE(result.fontWeight, 725);
}

void KoSvgTextCharacterResultValueContractTest::cursorDirectionAndCopiesOwnNestedValues()
{
    static_assert(std::is_same_v<decltype(CharacterResult::cursorInfo), CursorInfo>);
    static_assert(std::is_same_v<decltype(CharacterResult::anchor), KoSvgText::TextAnchor>);
    static_assert(std::is_same_v<decltype(CharacterResult::direction), KoSvgText::Direction>);

    CharacterResult defaults;
    QCOMPARE(defaults.cursorInfo.caret, QLineF());
    QVERIFY(defaults.cursorInfo.graphemeIndices.isEmpty());
    QVERIFY(defaults.cursorInfo.offsets.isEmpty());
    QVERIFY(!defaults.cursorInfo.rtl);
    QVERIFY(!defaults.cursorInfo.isWordBoundary);
    QVERIFY(!defaults.cursorInfo.color.isValid());
    QCOMPARE(int(defaults.anchor), int(KoSvgText::AnchorStart));
    QCOMPARE(int(defaults.direction), int(KoSvgText::DirectionLeftToRight));

    CharacterResult source = makePopulatedResult();
    const CharacterResult expected(source);
    const CharacterResult copied(source);
    CharacterResult assigned;
    assigned = source;

    source.finalPosition = QPointF(-137.0, 139.0);
    std::get<Glyph::Outline>(source.glyph).path.addEllipse(QRectF(2.0, 3.0, 5.0, 7.0));
    source.advance = QPointF(-149.0, 151.0);
    source.tabSize.reset();
    source.fontHalfLeading = 157.0;
    source.metrics.fontSize = -163;
    source.cursorInfo.caret.setP1(QPointF(167.0, 173.0));
    source.cursorInfo.graphemeIndices.append(179);
    source.cursorInfo.offsets[0] = QPointF(181.0, 191.0);
    source.cursorInfo.rtl = false;
    source.cursorInfo.isWordBoundary = false;
    source.cursorInfo.color = Qt::magenta;
    source.anchor = KoSvgText::AnchorMiddle;
    source.direction = KoSvgText::DirectionLeftToRight;

    compareCopiedValues(copied, expected);
    compareCopiedValues(assigned, expected);
    QVERIFY(source.finalPosition != copied.finalPosition);
    QVERIFY(std::get<Glyph::Outline>(source.glyph).path != std::get<Glyph::Outline>(copied.glyph).path);
    QVERIFY(!source.tabSize.has_value());
    QCOMPARE(source.metrics.fontSize, -163);
    QCOMPARE(source.cursorInfo.graphemeIndices, QVector<int>({83, 89, 179}));
    QCOMPARE(int(source.anchor), int(KoSvgText::AnchorMiddle));
    QCOMPARE(int(source.direction), int(KoSvgText::DirectionLeftToRight));
}

QTEST_GUILESS_MAIN(KoSvgTextCharacterResultValueContractTest)

#include "KoSvgTextCharacterResultValueContractTest.moc"
