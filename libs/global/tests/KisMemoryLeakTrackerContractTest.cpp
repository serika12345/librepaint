/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_memory_leak_tracker.h"

#include <QStringList>
#include <QTest>

namespace
{
QStringList *capturedMessages = nullptr;

void captureMessage(QtMsgType, const QMessageLogContext &, const QString &message)
{
    if (capturedMessages) {
        capturedMessages->append(message);
    }
}
}

class KisMemoryLeakTrackerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singletonReturnsStableInstance();
    void referencePathsFollowConfiguredTrackingMode();
};

void KisMemoryLeakTrackerContractTest::singletonReturnsStableInstance()
{
    KisMemoryLeakTracker *first = KisMemoryLeakTracker::instance();
    QVERIFY(first);
    QCOMPARE(KisMemoryLeakTracker::instance(), first);
}

void KisMemoryLeakTrackerContractTest::referencePathsFollowConfiguredTrackingMode()
{
    QStringList messages;
    capturedMessages = &messages;
    QtMessageHandler previousHandler = qInstallMessageHandler(captureMessage);

    {
        KisMemoryLeakTracker tracker;
        int tracked = 0;
        int owner = 0;

        tracker.reference(static_cast<const void *>(&tracked), &owner, "raw-int");
        tracker.dumpReferences(static_cast<const void *>(&tracked));
        tracker.dumpReferences();
        tracker.dereference(static_cast<const void *>(&tracked), &owner);

        tracker.reference(&tracked, &owner);
        tracker.dereference(&tracked, &owner);
    }

    qInstallMessageHandler(previousHandler);
    capturedMessages = nullptr;

#ifdef HAVE_MEMORY_LEAK_TRACKER
    QVERIFY(!messages.isEmpty());
#else
    QVERIFY(messages.isEmpty());
#endif
}

QTEST_GUILESS_MAIN(KisMemoryLeakTrackerContractTest)

#include "KisMemoryLeakTrackerContractTest.moc"
