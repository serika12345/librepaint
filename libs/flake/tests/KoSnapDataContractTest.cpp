/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoSnapData.h>

#include <QTest>

namespace
{
int pathSegmentPayloadCount = 0;

KoPathPoint *pointToken(quintptr value)
{
    return reinterpret_cast<KoPathPoint *>(value);
}

void compareSegment(const KoPathSegment &segment, KoPathPoint *expectedFirst, KoPathPoint *expectedSecond)
{
    QCOMPARE(segment.first(), expectedFirst);
    QCOMPARE(segment.second(), expectedSecond);
}
} // namespace

class KoPathSegment::Private
{
public:
    Private(KoPathPoint *first, KoPathPoint *second)
        : first(first)
        , second(second)
    {
        ++pathSegmentPayloadCount;
    }

    ~Private()
    {
        --pathSegmentPayloadCount;
    }

    KoPathPoint *first;
    KoPathPoint *second;
};

KoPathSegment::KoPathSegment(KoPathPoint *first, KoPathPoint *second)
    : d(new Private(first, second))
{
}

KoPathSegment::KoPathSegment(const KoPathSegment &segment)
    : d(new Private(segment.first(), segment.second()))
{
}

KoPathSegment &KoPathSegment::operator=(const KoPathSegment &other)
{
    if (this != &other) {
        d->first = other.first();
        d->second = other.second();
    }
    return *this;
}

KoPathSegment::~KoPathSegment()
{
    delete d;
}

KoPathPoint *KoPathSegment::first() const
{
    return d->first;
}

void KoPathSegment::setFirst(KoPathPoint *first)
{
    d->first = first;
}

KoPathPoint *KoPathSegment::second() const
{
    return d->second;
}

void KoPathSegment::setSecond(KoPathPoint *second)
{
    d->second = second;
}

class KoSnapDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsToEmptyListsAndReleasesOwnedValuesAtDestruction();
    void pointListsPreserveOrderReplaceAndDetachFromCopies();
    void segmentListsPreserveOrderReplaceAndDetachFromCopies();
};

void KoSnapDataContractTest::defaultsToEmptyListsAndReleasesOwnedValuesAtDestruction()
{
    QCOMPARE(pathSegmentPayloadCount, 0);

    auto *data = new KoSnapData;
    QVERIFY(data->snapPoints().isEmpty());
    QVERIFY(data->snapSegments().isEmpty());

    QList<KoPathSegment> segments{
        KoPathSegment(pointToken(0x11), pointToken(0x12)),
    };
    data->setSnapSegments(segments);
    segments.clear();
    QCOMPARE(pathSegmentPayloadCount, 1);

    delete data;
    QCOMPARE(pathSegmentPayloadCount, 0);
}

void KoSnapDataContractTest::pointListsPreserveOrderReplaceAndDetachFromCopies()
{
    KoSnapData data;
    QList<QPointF> points{
        QPointF(-3.5, 7.25),
        QPointF(11.0, -13.0),
    };

    data.setSnapPoints(points);
    points[0] = QPointF(101.0, 103.0);

    QList<QPointF> stored = data.snapPoints();
    QCOMPARE(stored.size(), 2);
    QCOMPARE(stored.at(0), QPointF(-3.5, 7.25));
    QCOMPARE(stored.at(1), QPointF(11.0, -13.0));

    stored.removeFirst();
    QCOMPARE(data.snapPoints().size(), 2);
    QCOMPARE(data.snapPoints().at(0), QPointF(-3.5, 7.25));

    data.setSnapPoints({QPointF(17.0, 19.0)});
    QCOMPARE(data.snapPoints(), QList<QPointF>({QPointF(17.0, 19.0)}));
}

void KoSnapDataContractTest::segmentListsPreserveOrderReplaceAndDetachFromCopies()
{
    KoSnapData data;
    QList<KoPathSegment> segments{
        KoPathSegment(pointToken(0x21), pointToken(0x22)),
        KoPathSegment(pointToken(0x31), pointToken(0x32)),
    };

    data.setSnapSegments(segments);
    segments[0].setFirst(pointToken(0x99));

    QList<KoPathSegment> stored = data.snapSegments();
    QCOMPARE(stored.size(), 2);
    compareSegment(stored.at(0), pointToken(0x21), pointToken(0x22));
    compareSegment(stored.at(1), pointToken(0x31), pointToken(0x32));

    stored[1].setSecond(pointToken(0x98));
    const QList<KoPathSegment> unchanged = data.snapSegments();
    compareSegment(unchanged.at(1), pointToken(0x31), pointToken(0x32));

    data.setSnapSegments({KoPathSegment(pointToken(0x41), pointToken(0x42))});
    const QList<KoPathSegment> replacement = data.snapSegments();
    QCOMPARE(replacement.size(), 1);
    compareSegment(replacement.at(0), pointToken(0x41), pointToken(0x42));
}

QTEST_GUILESS_MAIN(KoSnapDataContractTest)

#include "KoSnapDataContractTest.moc"
