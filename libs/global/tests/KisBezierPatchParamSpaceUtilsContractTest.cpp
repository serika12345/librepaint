/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierPatchParamSpaceUtils.h"

#include <QDebug>
#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisBezierPatchParamSpaceUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rangeGeometryReportsEndpointsLengthAndMidpoint();
    void containsRequiresFuzzyEqualityWithBothEnds();
    void squeezingMapsRelativeEndpoints();
    void forwardDistanceDistinguishesLeftRightAndOverlap();
    void rectangleConversionsPreserveAxes();
    void debugOutputReportsEndpoints();
    void tightRangeSearchBoundsTheRequestedSourceInterval();
};

void KisBezierPatchParamSpaceUtilsContractTest::rangeGeometryReportsEndpointsLengthAndMidpoint()
{
    const KisBezierUtils::Range range{2.0, 6.0};

    QCOMPARE(range.start, 2.0);
    QCOMPARE(range.end, 6.0);
    QCOMPARE(range.length(), 4.0);
    QCOMPARE(range.mid(), 4.0);
    QVERIFY(!range.isEmpty());
    const KisBezierUtils::Range emptyRange{3.0, 3.0};
    QVERIFY(emptyRange.isEmpty());
}

void KisBezierPatchParamSpaceUtilsContractTest::containsRequiresFuzzyEqualityWithBothEnds()
{
    const KisBezierUtils::Range ordinaryRange{0.0, 10.0};
    QVERIFY(!ordinaryRange.contains(5.0));

    const KisBezierUtils::Range fuzzyRange{1.0e12, 1.0e12 + 1.0};
    QVERIFY(fuzzyRange.contains(1.0e12 + 0.5));
    QVERIFY(!fuzzyRange.contains(fuzzyRange.start));
    QVERIFY(!fuzzyRange.contains(fuzzyRange.end));
}

void KisBezierPatchParamSpaceUtilsContractTest::squeezingMapsRelativeEndpoints()
{
    const KisBezierUtils::Range source{10.0, 30.0};
    const KisBezierUtils::Range alpha{0.25, 0.75};

    QCOMPARE(source.squeezedRange(alpha).start, 15.0);
    QCOMPARE(source.squeezedRange(alpha).end, 25.0);

    KisBezierUtils::Range inPlace = source;
    inPlace.squeezeRange(alpha);
    QCOMPARE(inPlace.start, 15.0);
    QCOMPARE(inPlace.end, 25.0);
}

void KisBezierPatchParamSpaceUtilsContractTest::forwardDistanceDistinguishesLeftRightAndOverlap()
{
    KisBezierUtils::Range source{10.0, 20.0};

    QCOMPARE(source.forwardDistanceTo({25.0, 30.0}), std::optional<qreal>(5.0));
    QCOMPARE(source.forwardDistanceTo({0.0, 5.0}), std::optional<qreal>(-5.0));
    QCOMPARE(source.forwardDistanceTo({20.0, 25.0}), std::nullopt);
    QCOMPARE(source.forwardDistanceTo({15.0, 25.0}), std::nullopt);
}

void KisBezierPatchParamSpaceUtilsContractTest::rectangleConversionsPreserveAxes()
{
    const QRectF rectangle(2.0, 3.0, 5.0, 7.0);
    const KisBezierUtils::Range xRange = KisBezierUtils::Range::fromRectX(rectangle);
    const KisBezierUtils::Range yRange = KisBezierUtils::Range::fromRectY(rectangle);

    QCOMPARE(xRange.start, 2.0);
    QCOMPARE(xRange.end, 7.0);
    QCOMPARE(yRange.start, 3.0);
    QCOMPARE(yRange.end, 10.0);
    QCOMPARE(KisBezierUtils::Range::makeRectF(xRange, yRange), rectangle);
}

void KisBezierPatchParamSpaceUtilsContractTest::debugOutputReportsEndpoints()
{
    QString output;
    {
        QDebug debug(&output);
        debug << KisBezierUtils::Range{2.0, 6.0};
    }

    QCOMPARE(output, QStringLiteral("Range(2, 6) "));
}

void KisBezierPatchParamSpaceUtilsContractTest::tightRangeSearchBoundsTheRequestedSourceInterval()
{
    using KisBezierUtils::Range;

    const auto sourceRangeAt = [](qreal parameter) {
        const qreal center = parameter * 100.0;
        return Range{std::max(0.0, center - 3.0), std::min(center + 3.0, 100.0)};
    };

    const auto [externalRange, internalRange] = KisBezierUtils::calcTightSrcRectRangeInParamSpace1D(
        Range{0.0, 1.0}, Range{0.0, 100.0}, Range{60.0, 70.0}, sourceRangeAt, 0.001);

    QVERIFY(externalRange.start < internalRange.start);
    QVERIFY(externalRange.end > internalRange.end);
    QVERIFY(qAbs(externalRange.start - 0.569992) < 0.000001);
    QVERIFY(qAbs(externalRange.end - 0.730003) < 0.000001);
    QVERIFY(qAbs(internalRange.start - 0.570007) < 0.000001);
    QVERIFY(qAbs(internalRange.end - 0.729996) < 0.000001);
}

QTEST_GUILESS_MAIN(KisBezierPatchParamSpaceUtilsContractTest)

#include "KisBezierPatchParamSpaceUtilsContractTest.moc"
