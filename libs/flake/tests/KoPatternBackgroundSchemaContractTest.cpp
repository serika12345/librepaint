/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPatternBackground.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PATTERN_BACKGROUND_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPatternBackground::method)), signature>)
} // namespace

class KoPatternBackgroundSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void patternBackgroundTypeAndValueLifetimeSchemaRemainStable();
    void patternBackgroundRepeatAndReferenceOrdinalsRemainStable();
    void patternImageAndDisplayGeometrySignaturesRemainStable();
    void patternReferencePlacementSignaturesRemainStable();
    void patternTileAndTransformSignaturesRemainStable();
};

void KoPatternBackgroundSchemaContractTest::patternBackgroundTypeAndValueLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPatternBackground>);
    static_assert(std::is_base_of_v<KoShapeBackground, KoPatternBackground>);
    static_assert(std::is_default_constructible_v<KoPatternBackground>);
    static_assert(std::is_copy_constructible_v<KoPatternBackground>);
    static_assert(std::is_copy_assignable_v<KoPatternBackground>);
    static_assert(std::is_destructible_v<KoPatternBackground>);
    static_assert(std::has_virtual_destructor_v<KoPatternBackground>);

    ASSERT_PATTERN_BACKGROUND_SIGNATURE(compareTo, bool (KoPatternBackground::*)(const KoShapeBackground *) const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(operator=,
                                        KoPatternBackground & (KoPatternBackground::*)(const KoPatternBackground &));
}

void KoPatternBackgroundSchemaContractTest::patternBackgroundRepeatAndReferenceOrdinalsRemainStable()
{
    static_assert(std::is_enum_v<KoPatternBackground::PatternRepeat>);
    static_assert(KoPatternBackground::Original == 0);
    static_assert(KoPatternBackground::Tiled == 1);
    static_assert(KoPatternBackground::Stretched == 2);

    static_assert(std::is_enum_v<KoPatternBackground::ReferencePoint>);
    static_assert(KoPatternBackground::TopLeft == 0);
    static_assert(KoPatternBackground::Top == 1);
    static_assert(KoPatternBackground::TopRight == 2);
    static_assert(KoPatternBackground::Left == 3);
    static_assert(KoPatternBackground::Center == 4);
    static_assert(KoPatternBackground::Right == 5);
    static_assert(KoPatternBackground::BottomLeft == 6);
    static_assert(KoPatternBackground::Bottom == 7);
    static_assert(KoPatternBackground::BottomRight == 8);
}

void KoPatternBackgroundSchemaContractTest::patternImageAndDisplayGeometrySignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(pattern, QImage (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternDisplaySize, QSizeF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternOriginalSize, QSizeF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(patternRectFromFillSize, QRectF (KoPatternBackground::*)(const QSizeF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(repeat, KoPatternBackground::PatternRepeat (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setPattern, void (KoPatternBackground::*)(const QImage &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setPatternDisplaySize, void (KoPatternBackground::*)(const QSizeF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setRepeat, void (KoPatternBackground::*)(KoPatternBackground::PatternRepeat));
}

void KoPatternBackgroundSchemaContractTest::patternReferencePlacementSignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(referencePoint,
                                        KoPatternBackground::ReferencePoint (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(referencePointOffset, QPointF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setReferencePoint,
                                        void (KoPatternBackground::*)(KoPatternBackground::ReferencePoint));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setReferencePointOffset, void (KoPatternBackground::*)(const QPointF &));
}

void KoPatternBackgroundSchemaContractTest::patternTileAndTransformSignaturesRemainStable()
{
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setTileRepeatOffset, void (KoPatternBackground::*)(const QPointF &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(setTransform, void (KoPatternBackground::*)(const QTransform &));
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(tileRepeatOffset, QPointF (KoPatternBackground::*)() const);
    ASSERT_PATTERN_BACKGROUND_SIGNATURE(transform, QTransform (KoPatternBackground::*)() const);
}

QTEST_GUILESS_MAIN(KoPatternBackgroundSchemaContractTest)

#include "KoPatternBackgroundSchemaContractTest.moc"
