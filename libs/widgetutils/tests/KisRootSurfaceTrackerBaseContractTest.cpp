/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRootSurfaceTrackerBase.h"

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QTest>
#include <QWidget>
#include <QWindow>

namespace
{

class RootSurfaceTrackerProbe : public KisRootSurfaceTrackerBase
{
public:
    RootSurfaceTrackerProbe(QWidget *watched, QObject *parent = nullptr, int *destructionCount = nullptr)
        : KisRootSurfaceTrackerBase(watched, parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~RootSurfaceTrackerProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void startTracking()
    {
        initialize();
    }

    QVector<QPointer<QWindow>> connectedWindows;
    int disconnectionCount{0};

protected:
    void connectToNativeWindow(QWindow *nativeWindow) override
    {
        connectedWindows.append(nativeWindow);
    }

    void disconnectFromNativeWindow() override
    {
        ++disconnectionCount;
    }

private:
    int *m_destructionCount;
};

[[noreturn]] void unexpectedAssert(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

class KisRootSurfaceTrackerBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void retainsBorrowedWidgetAndQObjectLifetime();
    void hiddenHierarchyChangesRemainDisconnectedFromNativeWindows();
};

void KisRootSurfaceTrackerBaseContractTest::retainsBorrowedWidgetAndQObjectLifetime()
{
    QPointer<QWidget> watched = new QWidget;
    auto *owner = new QObject;
    int destructionCount = 0;
    QPointer<RootSurfaceTrackerProbe> tracker = new RootSurfaceTrackerProbe(watched, owner, &destructionCount);

    QCOMPARE(tracker->parent(), owner);
    QCOMPARE(tracker->trackedWidget(), watched.data());
    QVERIFY(tracker->connectedWindows.isEmpty());
    QCOMPARE(tracker->disconnectionCount, 0);

    QObject *base = tracker;
    QCOMPARE(base, tracker.data());
    delete owner;

    QVERIFY(tracker.isNull());
    QCOMPARE(destructionCount, 1);
    QVERIFY(!watched.isNull());

    delete watched;
}

void KisRootSurfaceTrackerBaseContractTest::hiddenHierarchyChangesRemainDisconnectedFromNativeWindows()
{
    QWidget firstRoot;
    QWidget secondRoot;
    QWidget watched(&firstRoot);
    RootSurfaceTrackerProbe tracker(&watched);

    tracker.startTracking();
    QCOMPARE(tracker.trackedWidget(), &watched);
    QVERIFY(tracker.connectedWindows.isEmpty());
    QCOMPARE(tracker.disconnectionCount, 1);

    watched.setParent(&secondRoot);
    QCoreApplication::processEvents();

    QVERIFY(tracker.connectedWindows.isEmpty());
    QCOMPARE(tracker.disconnectionCount, 2);

    QEvent unrelatedEvent(QEvent::User);
    QCoreApplication::sendEvent(&watched, &unrelatedEvent);
    QCOMPARE(tracker.disconnectionCount, 2);
}

QTEST_MAIN(KisRootSurfaceTrackerBaseContractTest)

#include "KisRootSurfaceTrackerBaseContractTest.moc"
