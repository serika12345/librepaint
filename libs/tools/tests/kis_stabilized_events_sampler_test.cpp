/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_stabilized_events_sampler_test.h"

#include <KisStabilizerDelayedPaintHelper.h>
#include <kis_stabilized_events_sampler.h>

#include <kis_paint_information.h>
#include <QPointer>
#include <new>

void KisStabilizedEventsSamplerTest::test()
{
    KisStabilizedEventsSampler sampler(20);

    KisPaintInformation pi1(QPoint(10,10));
    KisPaintInformation pi2(QPoint(20,20));

    sampler.addEvent(pi1);

    QTest::qSleep(50);

    sampler.addEvent(pi2);

    QTest::qSleep(70);

    KisStabilizedEventsSampler::iterator it;
    KisStabilizedEventsSampler::iterator end;
    std::tie(it, end) = sampler.range();


    int numTotal = 0;
    int num1 = 0;
    int num2 = 0;

    for (; it != end; ++it) {
        numTotal++;
        if (it->pos().x() == 10) {
            num1++;
        } else if (it->pos().x() == 20) {
            num2++;
        }

        qDebug() << ppVar(it->pos());
    }

    QVERIFY(numTotal >= 6);
    QVERIFY(num1 >= 3);
    QVERIFY(num2 >= 3);
}

void KisStabilizedEventsSamplerTest::delayedPaintPreservesInputOrder()
{
    KisPaintInformation first(QPointF(10.0, 20.0));
    KisPaintInformation second(QPointF(30.0, 40.0));
    KisPaintInformation third(QPointF(50.0, 60.0));
    QVector<QPair<QPointF, QPointF>> paintedSegments;
    QVector<QPair<QPointF, QPointF>> expectedSegments;
    expectedSegments.append(qMakePair(first.pos(), second.pos()));
    expectedSegments.append(qMakePair(second.pos(), third.pos()));

    KisStabilizerDelayedPaintHelper helper;
    helper.setPaintLineCallback(
        [&paintedSegments](const KisPaintInformation &from,
                           const KisPaintInformation &to) {
            paintedSegments.append(qMakePair(from.pos(), to.pos()));
        });
    helper.setUpdateOutlineCallback([]() {});

    helper.start(first);
    QVERIFY(helper.running());
    QVERIFY(helper.hasLastPaintInformation());
    QCOMPARE(helper.lastPaintInformation().pos(), first.pos());

    helper.update({second, third});
    helper.end();

    QVERIFY(!helper.running());
    QVERIFY(!helper.hasLastPaintInformation());
    QCOMPARE(paintedSegments, expectedSegments);

    helper.start(first);
    helper.update({second});
    helper.cancel();

    QVERIFY(!helper.running());
    QVERIFY(!helper.hasLastPaintInformation());
    QCOMPARE(paintedSegments.size(), 2);
}

void KisStabilizedEventsSamplerTest::delayedPaintAllowsCancellationAndDestruction()
{
    // A rejected GPU append can cancel or close the document synchronously.
    for (bool finish : {false, true}) for (bool destroy : {false, true}) {
        QPointer<KisStabilizerDelayedPaintHelper> helper = new KisStabilizerDelayedPaintHelper;
        int painted = 0;
        helper->setPaintLineCallback([&](const KisPaintInformation &, const KisPaintInformation &) {
            ++painted;
            if (destroy) delete helper.data();
            else helper->cancel();
        });
        helper->setUpdateOutlineCallback([] {});
        helper->start(KisPaintInformation(QPointF(10, 10)));
        helper->update({KisPaintInformation(QPointF(20, 20)), KisPaintInformation(QPointF(30, 30))});
        if (finish) helper->end();
        else helper->paintSome();
        QCOMPARE(painted, 1);
        if (destroy) QVERIFY(!helper);
        else {
            QVERIFY(!helper->running());
            QVERIFY(!helper->hasLastPaintInformation());
            delete helper.data();
        }
    }
}

void KisStabilizedEventsSamplerTest::delayedPaintBatchesDueLinesAndRetainsTheNextStart()
{
    using Lines = KisStabilizerDelayedPaintHelper::PaintLines;
    const KisPaintInformation first(QPointF(10, 20)), second(QPointF(30, 40)), third(QPointF(50, 60));
    for (bool finish : {false, true}) for (bool destroy : {false, true}) {
        QPointer<KisStabilizerDelayedPaintHelper> helper = new KisStabilizerDelayedPaintHelper;
        Lines received;
        int calls = 0;
        helper->setPaintLineCallback([](const KisPaintInformation &, const KisPaintInformation &) { QFAIL("Batch consumer was bypassed"); });
        helper->setUpdateOutlineCallback([] {});
        helper->setPaintLinesCallback([&](const Lines &lines) {
            ++calls;
            received = lines;
            if (destroy) delete helper.data();
        });
        helper->start(first);
        helper->update({second, third});
        if (finish) helper->end();
        else helper->paintSome();
        QCOMPARE(calls, 1);
        QCOMPARE(received.size(), 2);
        QCOMPARE(received[0].first.pos(), first.pos());
        QCOMPARE(received[0].second.pos(), second.pos());
        QCOMPARE(received[1].first.pos(), second.pos());
        QCOMPARE(received[1].second.pos(), third.pos());
        if (destroy) { QVERIFY(!helper); continue; }
        if (finish) QVERIFY(!helper->hasLastPaintInformation());
        else {
            QCOMPARE(helper->lastPaintInformation().pos(), third.pos());
            helper->update({first});
            helper->end();
            QCOMPARE(calls, 2);
            QCOMPARE(received.size(), 1);
            QCOMPARE(received[0].first.pos(), third.pos());
            QCOMPARE(received[0].second.pos(), first.pos());
        }
        helper->cancel();
        QVERIFY(!helper->running());
        QVERIFY(!helper->hasLastPaintInformation());
        delete helper.data();
    }

    // A cold GPU preview can leave many resampled lines due at once. Deliver
    // their complete sensor input in one call, retaining every line endpoint.
    KisStabilizerDelayedPaintHelper helper;
    QVector<KisPaintInformation> input;
    for (int i = 0; i <= 1000; ++i)
        input.append(KisPaintInformation(QPointF(i, i % 17), qreal(i % 11) / 10, 0, 0, 0, 0, 1, i, 0));
    int calls = 0;
    helper.setPaintLinesCallback([&](const Lines &lines) {
        ++calls;
        QCOMPARE(lines.size(), input.size() - 1);
        for (qsizetype i = 0; i < lines.size(); ++i) {
            QCOMPARE(lines[i].first.pos(), input[i].pos());
            QCOMPARE(lines[i].second.pos(), input[i + 1].pos());
            QCOMPARE(lines[i].first.pressure(), input[i].pressure());
            QCOMPARE(lines[i].second.currentTime(), input[i + 1].currentTime());
        }
    });
    helper.setUpdateOutlineCallback([] {});
    helper.start(input.first());
    helper.update(input.sliced(1));
    helper.end();
    QCOMPARE(calls, 1);
    QVERIFY(!helper.hasLastPaintInformation());
}

void KisStabilizedEventsSamplerTest::delayedBatchFailureCancelsBeforeNotifying()
{
    for (bool finish : {false, true}) for (bool destroy : {false, true}) {
        QPointer<KisStabilizerDelayedPaintHelper> helper = new KisStabilizerDelayedPaintHelper;
        int notified = 0;
        helper->setPaintLinesCallback([](const KisStabilizerDelayedPaintHelper::PaintLines &) {
            throw std::bad_alloc();
        }, [&] {
            ++notified;
            QVERIFY(!helper->running());
            QVERIFY(!helper->hasLastPaintInformation());
            if (destroy) delete helper.data();
        });
        helper->setUpdateOutlineCallback([] {});
        helper->start(KisPaintInformation(QPointF(10, 10)));
        helper->update({KisPaintInformation(QPointF(20, 20)), KisPaintInformation(QPointF(30, 30))});
        if (finish) helper->end();
        else helper->paintSome();
        QCOMPARE(notified, 1);
        if (destroy) QVERIFY(!helper);
        else delete helper.data();
    }
}

SIMPLE_TEST_MAIN(KisStabilizedEventsSamplerTest)
