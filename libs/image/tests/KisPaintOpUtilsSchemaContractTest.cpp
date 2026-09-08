/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paintop_utils.h"

#include <QTest>

#include <type_traits>

namespace
{

struct PaintOpProbe {
};

#define ASSERT_PAINTOP_UTILS_FUNCTION(function, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpUtils::function)), signature>)

} // namespace

class KisPaintOpUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void positionHistoryTypeAndUpdateBehaviorRemainStable();
    void sizeAndAutomaticSpacingBehaviorRemainStable();
    void effectiveSpacingAndTimingSignaturesRemainStable();
    void dabRectangleSplittingSignaturesRemainStable();
    void fanAndLinePaintingTemplateSignaturesRemainStable();
};

// clang-format off
void KisPaintOpUtilsSchemaContractTest::positionHistoryTypeAndUpdateBehaviorRemainStable()
// clang-format on
{
    using History = KisPaintOpUtils::PositionHistory;

    static_assert(std::is_class_v<History>);
    static_assert(std::is_default_constructible_v<History>);
    static_assert(std::is_same_v<decltype(&History::reset), void (History::*)(const QPointF &)>);
    static_assert(std::is_same_v<decltype(&History::pushThroughHistory), QPointF (History::*)(const QPointF &, qreal)>);

    History history;
    const QPointF initialPoint(10.0, 20.0);
    const QPointF secondPoint(20.0, 30.0);
    const QPointF thirdPoint(30.0, 40.0);

    history.reset(initialPoint);
    QCOMPARE(history.pushThroughHistory(initialPoint, 1.0), initialPoint);
    QCOMPARE(history.pushThroughHistory(secondPoint, 1.0), initialPoint);
    QCOMPARE(history.pushThroughHistory(thirdPoint, 1.0), secondPoint);
}

// clang-format off
void KisPaintOpUtilsSchemaContractTest::sizeAndAutomaticSpacingBehaviorRemainStable()
// clang-format on
{
    ASSERT_PAINTOP_UTILS_FUNCTION(checkSizeTooSmall, bool (*)(qreal, qreal, qreal));
    ASSERT_PAINTOP_UTILS_FUNCTION(calcAutoSpacing, qreal (*)(qreal, qreal));
    ASSERT_PAINTOP_UTILS_FUNCTION(calcAutoSpacing, QPointF (*)(const QPointF &, qreal, qreal));

    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 0.009, 10.0));
    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 10.0, 0.009));
    QVERIFY(!KisPaintOpUtils::checkSizeTooSmall(1.0, 0.01, 0.01));
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(0.25, 0.5), 0.125);
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(4.0, 0.5), 1.0);
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(QPointF(4.0, 9.0), 0.5, 1.0), QPointF(1.0, 1.5));
}

// clang-format off
void KisPaintOpUtilsSchemaContractTest::effectiveSpacingAndTimingSignaturesRemainStable()
// clang-format on
{
    using SpacingSignature =
        KisSpacingInformation (*)(qreal, qreal, qreal, bool, bool, qreal, bool, qreal, bool, qreal, qreal);
    using TimingSignature = KisTimingInformation (*)(bool, qreal, qreal);

    ASSERT_PAINTOP_UTILS_FUNCTION(effectiveSpacing, SpacingSignature);
    ASSERT_PAINTOP_UTILS_FUNCTION(effectiveTiming, TimingSignature);
}

// clang-format off
void KisPaintOpUtilsSchemaContractTest::dabRectangleSplittingSignaturesRemainStable()
// clang-format on
{
    using FilterSignature = QVector<QRect> (*)(const QRect &, const QVector<QRect> &, int);
    using SplitSignature = QVector<QRect> (*)(const QVector<QRect> &, int, int, qreal);

    ASSERT_PAINTOP_UTILS_FUNCTION(splitAndFilterDabRect, FilterSignature);
    ASSERT_PAINTOP_UTILS_FUNCTION(splitDabsIntoRects, SplitSignature);
}

// clang-format off
void KisPaintOpUtilsSchemaContractTest::fanAndLinePaintingTemplateSignaturesRemainStable()
// clang-format on
{
    using FanSignature = bool (*)(PaintOpProbe &,
                                  const KisPaintInformation &,
                                  const KisPaintInformation &,
                                  KisDistanceInformation *,
                                  qreal);
    using LineSignature = void (*)(PaintOpProbe &,
                                   const KisPaintInformation &,
                                   const KisPaintInformation &,
                                   KisDistanceInformation *,
                                   bool,
                                   qreal);

    ASSERT_PAINTOP_UTILS_FUNCTION(paintFan<PaintOpProbe>, FanSignature);
    ASSERT_PAINTOP_UTILS_FUNCTION(paintLine<PaintOpProbe>, LineSignature);
}

#undef ASSERT_PAINTOP_UTILS_FUNCTION

QTEST_GUILESS_MAIN(KisPaintOpUtilsSchemaContractTest)

#include "KisPaintOpUtilsSchemaContractTest.moc"
