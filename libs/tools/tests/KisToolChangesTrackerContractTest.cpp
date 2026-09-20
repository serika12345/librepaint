/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisToolChangesTracker.h"

#include <memory>

#include <QSignalSpy>
#include <QTest>
#include <QWeakPointer>

namespace
{

class TrackerProbe : public KisToolChangesTracker
{
public:
    explicit TrackerProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~TrackerProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

KisToolChangesTrackerDataSP makeState()
{
    return KisToolChangesTrackerDataSP::create();
}

} // namespace

class KisToolChangesTrackerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsEmptyAndCommitsWithoutNotification();
    void undoAndRedoExposeTheCurrentState();
    void commitAfterUndoClearsRedoHistory();
    void undoWithOnlyOneStateMovesItToRedoWithoutNotification();
    void resetClearsHistoryAndOwnership();
    void qObjectOwnershipDestroysDerivedTracker();
};

void KisToolChangesTrackerContractTest::startsEmptyAndCommitsWithoutNotification()
{
    KisToolChangesTracker tracker;
    QSignalSpy changes(&tracker, &KisToolChangesTracker::sigConfigChanged);

    QVERIFY(tracker.isEmpty());
    QVERIFY(!tracker.canUndo());
    QVERIFY(!tracker.canRedo());
    QVERIFY(tracker.lastState().isNull());

    const KisToolChangesTrackerDataSP first = makeState();
    tracker.commitConfig(first);

    QVERIFY(!tracker.isEmpty());
    QVERIFY(!tracker.canUndo());
    QVERIFY(!tracker.canRedo());
    QCOMPARE(tracker.lastState().data(), first.data());
    QCOMPARE(changes.count(), 0);
}

void KisToolChangesTrackerContractTest::undoAndRedoExposeTheCurrentState()
{
    KisToolChangesTracker tracker;
    const KisToolChangesTrackerDataSP first = makeState();
    const KisToolChangesTrackerDataSP second = makeState();
    tracker.commitConfig(first);
    tracker.commitConfig(second);
    QSignalSpy changes(&tracker, &KisToolChangesTracker::sigConfigChanged);

    QVERIFY(tracker.canUndo());
    tracker.requestUndo();

    QVERIFY(!tracker.canUndo());
    QVERIFY(tracker.canRedo());
    QCOMPARE(tracker.lastState().data(), first.data());
    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.takeFirst().at(0).value<KisToolChangesTrackerDataSP>().data(), first.data());

    tracker.requestRedo();

    QVERIFY(tracker.canUndo());
    QVERIFY(!tracker.canRedo());
    QCOMPARE(tracker.lastState().data(), second.data());
    QCOMPARE(changes.count(), 1);
    QCOMPARE(changes.takeFirst().at(0).value<KisToolChangesTrackerDataSP>().data(), second.data());
}

void KisToolChangesTrackerContractTest::commitAfterUndoClearsRedoHistory()
{
    KisToolChangesTracker tracker;
    const KisToolChangesTrackerDataSP first = makeState();
    const KisToolChangesTrackerDataSP second = makeState();
    const KisToolChangesTrackerDataSP replacement = makeState();
    tracker.commitConfig(first);
    tracker.commitConfig(second);
    tracker.requestUndo();
    QVERIFY(tracker.canRedo());

    tracker.commitConfig(replacement);

    QVERIFY(tracker.canUndo());
    QVERIFY(!tracker.canRedo());
    QCOMPARE(tracker.lastState().data(), replacement.data());
}

void KisToolChangesTrackerContractTest::undoWithOnlyOneStateMovesItToRedoWithoutNotification()
{
    KisToolChangesTracker tracker;
    const KisToolChangesTrackerDataSP onlyState = makeState();
    tracker.commitConfig(onlyState);
    QSignalSpy changes(&tracker, &KisToolChangesTracker::sigConfigChanged);
    QVERIFY(!tracker.canUndo());

    tracker.requestUndo();

    QVERIFY(tracker.lastState().isNull());
    QVERIFY(tracker.canRedo());
    QVERIFY(!tracker.isEmpty());
    QCOMPARE(changes.count(), 0);
}

void KisToolChangesTrackerContractTest::resetClearsHistoryAndOwnership()
{
    KisToolChangesTracker tracker;
    QWeakPointer<KisToolChangesTrackerData> observer;
    {
        const KisToolChangesTrackerDataSP state = makeState();
        observer = state;
        tracker.commitConfig(state);
    }
    QVERIFY(!observer.isNull());
    QSignalSpy changes(&tracker, &KisToolChangesTracker::sigConfigChanged);

    tracker.reset();

    QVERIFY(tracker.isEmpty());
    QVERIFY(!tracker.canUndo());
    QVERIFY(!tracker.canRedo());
    QVERIFY(tracker.lastState().isNull());
    QVERIFY(observer.isNull());
    QCOMPARE(changes.count(), 0);
}

void KisToolChangesTrackerContractTest::qObjectOwnershipDestroysDerivedTracker()
{
    int destructionCount = 0;

    {
        std::unique_ptr<QObject> tracker = std::make_unique<TrackerProbe>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisToolChangesTrackerContractTest)

#include "KisToolChangesTrackerContractTest.moc"
