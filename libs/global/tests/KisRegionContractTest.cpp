/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRegion.h"

#include <QRegion>
#include <QTest>

#include <iterator>
#include <utility>

namespace
{
QRegion regionForRects(const QVector<QRect> &rects)
{
    QRegion region;
    for (const QRect &rect : rects) {
        region |= rect;
    }
    return region;
}
}

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisRegionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorsMergeSparseRectsAndReportGeometry();
    void mergeSparseRectsMergesHorizontalAndVerticalNeighbors();
    void overlappingApproximationAndGridUniquenessCoverInputs();
    void qRegionRoundTripAndIntersectionPreserveArea();
    void translationReturnsCopyAndMutatesInPlace();
};

void KisRegionContractTest::constructorsMergeSparseRectsAndReportGeometry()
{
    KisRegion empty;
    QVERIFY(empty.isEmpty());
    QCOMPARE(empty.rectCount(), 0);
    QCOMPARE(empty.rects(), QVector<QRect>());
    QCOMPARE(empty.boundingRect(), QRect());

    const QRect left(2, 3, 4, 5);
    KisRegion single(left);
    QVERIFY(!single.isEmpty());
    QCOMPARE(single.rects(), QVector<QRect>{left});

    const QRect right(6, 3, 4, 5);
    KisRegion listed {left, right};
    QCOMPARE(listed.rects(), (QVector<QRect>{left, right}));
    QCOMPARE(listed.boundingRect(), QRect(2, 3, 8, 5));

    const QVector<QRect> constRects {right, left};
    KisRegion merged(constRects);
    QCOMPARE(merged.rects(), QVector<QRect>{QRect(2, 3, 8, 5)});

    QVector<QRect> movableRects {right, left};
    KisRegion moved(std::move(movableRects));
    QCOMPARE(moved, merged);

    KisRegion copied(merged);
    QCOMPARE(copied, merged);

    KisRegion assigned;
    KisRegion *assignmentResult = &(assigned = copied);
    QCOMPARE(assignmentResult, &assigned);
    QCOMPARE(assigned, copied);
}

void KisRegionContractTest::mergeSparseRectsMergesHorizontalAndVerticalNeighbors()
{
    QVector<QRect> empty;
    QCOMPARE(KisRegion::mergeSparseRects(empty.begin(), empty.end()), empty.end());

    QVector<QRect> tiles {
        QRect(10, 10, 10, 10),
        QRect(0, 0, 10, 10),
        QRect(0, 10, 10, 10),
        QRect(10, 0, 10, 10),
    };
    const auto end = KisRegion::mergeSparseRects(tiles.begin(), tiles.end());
    tiles.erase(end, tiles.end());

    QCOMPARE(tiles, QVector<QRect>{QRect(0, 0, 20, 20)});
}

void KisRegionContractTest::overlappingApproximationAndGridUniquenessCoverInputs()
{
    QVector<QRect> gridLike {
        QRect(4, 0, 4, 4),
        QRect(0, 0, 4, 4),
        QRect(4, 0, 4, 4),
    };
    KisRegion::makeGridLikeRectsUnique(gridLike);
    QCOMPARE(gridLike, (QVector<QRect>{QRect(0, 0, 4, 4), QRect(4, 0, 4, 4)}));

    const QVector<QRect> overlapping {QRect(0, 0, 8, 8), QRect(4, 4, 8, 8)};
    QVector<QRect> approximated = overlapping;
    KisRegion::approximateOverlappingRects(approximated, 4);
    QVERIFY(!approximated.isEmpty());
    for (auto first = approximated.constBegin(); first != approximated.constEnd(); ++first) {
        for (auto second = std::next(first); second != approximated.constEnd(); ++second) {
            QVERIFY(!first->intersects(*second));
        }
    }

    const QRegion sourceArea = regionForRects(overlapping);
    const KisRegion region = KisRegion::fromOverlappingRects(overlapping, 4);
    QCOMPARE(region.toQRegion() & sourceArea, sourceArea);
    QCOMPARE(region.toQRegion(), regionForRects(approximated));
}

void KisRegionContractTest::qRegionRoundTripAndIntersectionPreserveArea()
{
    QRegion source(QRect(0, 0, 8, 6));
    source |= QRect(12, 3, 5, 7);

    KisRegion region = KisRegion::fromQRegion(source);
    QCOMPARE(region.toQRegion(), source);

    const QRect clip(4, 2, 10, 5);
    KisRegion *intersectionResult = &(region &= clip);
    QCOMPARE(intersectionResult, &region);
    QCOMPARE(region.toQRegion(), source & clip);
}

void KisRegionContractTest::translationReturnsCopyAndMutatesInPlace()
{
    const QVector<QRect> sourceRects {QRect(0, 0, 4, 5), QRect(8, 2, 3, 6)};
    KisRegion source(sourceRects);

    const KisRegion translated = source.translated(5, -3);
    QCOMPARE(source.toQRegion(), regionForRects(sourceRects));
    QCOMPARE(translated.toQRegion(), source.toQRegion().translated(5, -3));

    source.translate(5, -3);
    QCOMPARE(source, translated);
}

QTEST_GUILESS_MAIN(KisRegionContractTest)

#include "KisRegionContractTest.moc"
