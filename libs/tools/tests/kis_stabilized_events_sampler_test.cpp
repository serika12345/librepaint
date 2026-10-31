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

SIMPLE_TEST_MAIN(KisStabilizedEventsSamplerTest)
