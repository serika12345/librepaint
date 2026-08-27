/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_signals_blocker.h>

#include <QObject>
#include <QTest>

class KisSignalsBlockerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singleObjectRestoresPriorState();
    void multipleObjectsRestoreIndividualStates();
};

void KisSignalsBlockerTest::singleObjectRestoresPriorState()
{
    QObject object;
    QVERIFY(!object.signalsBlocked());

    {
        KisSignalsBlocker blocker(&object);
        QVERIFY(object.signalsBlocked());
    }
    QVERIFY(!object.signalsBlocked());

    object.blockSignals(true);
    {
        KisSignalsBlocker blocker(&object);
        QVERIFY(object.signalsBlocked());
    }
    QVERIFY(object.signalsBlocked());
}

void KisSignalsBlockerTest::multipleObjectsRestoreIndividualStates()
{
    QObject first;
    QObject second;
    QObject third;
    QObject fourth;
    QObject fifth;
    QObject sixth;
    second.blockSignals(true);
    fifth.blockSignals(true);

    {
        KisSignalsBlocker blocker(&first, &second, &third, &fourth, &fifth, &sixth);
        QVERIFY(first.signalsBlocked());
        QVERIFY(second.signalsBlocked());
        QVERIFY(third.signalsBlocked());
        QVERIFY(fourth.signalsBlocked());
        QVERIFY(fifth.signalsBlocked());
        QVERIFY(sixth.signalsBlocked());
    }

    QVERIFY(!first.signalsBlocked());
    QVERIFY(second.signalsBlocked());
    QVERIFY(!third.signalsBlocked());
    QVERIFY(!fourth.signalsBlocked());
    QVERIFY(fifth.signalsBlocked());
    QVERIFY(!sixth.signalsBlocked());
}

QTEST_GUILESS_MAIN(KisSignalsBlockerTest)

#include "KisSignalsBlockerTest.moc"
