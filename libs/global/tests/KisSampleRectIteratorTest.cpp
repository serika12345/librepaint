/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisSampleRectIterator.h>
#include <kis_assert.h>

#include <QTest>

#include <type_traits>

namespace
{

int safeAssertCount = 0;

void advance(KisSampleRectIterator &iterator, int count)
{
    for (int i = 0; i < count; ++i) {
        ++iterator;
    }
}

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    ++safeAssertCount;
}

class KisSampleRectIteratorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void defaultIteratorStartsAtEmptyRect();
    void samplesLandmarksThenHaltonSequence();
    void copiesPreserveIndependentPosition();
    void movesContinueAtTheTransferredPosition();
};

void KisSampleRectIteratorTest::init()
{
    safeAssertCount = 0;
}

void KisSampleRectIteratorTest::defaultIteratorStartsAtEmptyRect()
{
    KisSampleRectIterator iterator;

    QVERIFY(std::is_destructible_v<KisSampleRectIterator>);
    QCOMPARE(iterator.numSamples(), 1);
    QCOMPARE(*iterator, QPointF());
    QCOMPARE(safeAssertCount, 0);
}

void KisSampleRectIteratorTest::samplesLandmarksThenHaltonSequence()
{
    const QRectF rect(10.0, 20.0, 8.0, 6.0);
    KisSampleRectIterator iterator(rect);
    const QList<QPointF> expected{
        QPointF(10.0, 20.0),
        QPointF(18.0, 20.0),
        QPointF(18.0, 26.0),
        QPointF(10.0, 26.0),
        QPointF(10.0, 23.0),
        QPointF(14.0, 20.0),
        QPointF(18.0, 23.0),
        QPointF(14.0, 26.0),
        QPointF(14.0, 23.0),
        QPointF(14.0, 22.0),
    };

    for (qsizetype i = 0; i < expected.size(); ++i) {
        QCOMPARE(iterator.numSamples(), i + 1);
        QCOMPARE(*iterator, expected[i]);
        if (i + 1 < expected.size()) {
            ++iterator;
        }
    }
    QCOMPARE(safeAssertCount, 0);
}

void KisSampleRectIteratorTest::copiesPreserveIndependentPosition()
{
    const QRectF rect(10.0, 20.0, 8.0, 6.0);
    KisSampleRectIterator source(rect);
    advance(source, 10);

    KisSampleRectIterator copied(source);
    QCOMPARE(copied.numSamples(), source.numSamples());
    QCOMPARE(*copied, *source);

    ++copied;
    ++source;
    QCOMPARE(*copied, *source);

    KisSampleRectIterator assigned;
    assigned = source;
    QCOMPARE(assigned.numSamples(), source.numSamples());
    QCOMPARE(*assigned, *source);

    ++assigned;
    ++source;
    QCOMPARE(*assigned, *source);
    QCOMPARE(safeAssertCount, 0);
}

void KisSampleRectIteratorTest::movesContinueAtTheTransferredPosition()
{
    const QRectF rect(10.0, 20.0, 8.0, 6.0);
    KisSampleRectIterator constructionSource(rect);
    advance(constructionSource, 9);
    const QPointF constructionPoint = *constructionSource;
    const int constructionCount = constructionSource.numSamples();

    KisSampleRectIterator moved(std::move(constructionSource));
    QCOMPARE(moved.numSamples(), constructionCount);
    QCOMPARE(*moved, constructionPoint);

    KisSampleRectIterator assignmentSource(rect);
    advance(assignmentSource, 11);
    const QPointF assignmentPoint = *assignmentSource;
    const int assignmentCount = assignmentSource.numSamples();

    KisSampleRectIterator assigned;
    assigned = std::move(assignmentSource);
    QCOMPARE(assigned.numSamples(), assignmentCount);
    QCOMPARE(*assigned, assignmentPoint);
    QCOMPARE(safeAssertCount, 0);
}

QTEST_GUILESS_MAIN(KisSampleRectIteratorTest)

#include "KisSampleRectIteratorTest.moc"
