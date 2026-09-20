/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisScreenMigrationTracker.h"

#include <QApplication>
#include <QByteArray>
#include <QMetaObject>
#include <QPointer>
#include <QScreen>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>
#include <QWindow>

namespace
{

int safeAssertCount = 0;
QByteArray lastSafeAssertion;

void resetSafeAssertDiagnostic()
{
    safeAssertCount = 0;
    lastSafeAssertion.clear();
}

void showTrackedWidget(QWidget *widget)
{
    widget->resize(160, 120);
    widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(widget));
    QTRY_VERIFY(widget->windowHandle());
}

QScreen *spyScreen(const QSignalSpy &spy)
{
    return qvariant_cast<QScreen *>(spy.first().first());
}

} // namespace

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    ++safeAssertCount;
    lastSafeAssertion = assertion;
}

class KisScreenMigrationTrackerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void hiddenWidgetUsesDefaultScreenAndRetainsBorrowedLifetime();
    void visibleWidgetConnectsAndForwardsScreenChanges();
    void resolutionChangesAreCoalesced();
};

void KisScreenMigrationTrackerContractTest::initTestCase()
{
    qRegisterMetaType<QScreen *>("QScreen*");
}

void KisScreenMigrationTrackerContractTest::hiddenWidgetUsesDefaultScreenAndRetainsBorrowedLifetime()
{
    resetSafeAssertDiagnostic();
    QPointer<QWidget> trackedWidget = new QWidget;
    auto *owner = new QObject;
    QPointer<KisScreenMigrationTracker> tracker = new KisScreenMigrationTracker(trackedWidget, owner);
    QScreen *defaultScreen = qApp->screens().first();

    QCOMPARE(tracker->parent(), owner);
    QCOMPARE(tracker->trackedWidget(), trackedWidget.data());
    QCOMPARE(tracker->currentScreenSafe(), defaultScreen);
    QCOMPARE(safeAssertCount, 0);

    QCOMPARE(tracker->currentScreen(), defaultScreen);
    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(lastSafeAssertion, QByteArray("m_connectedTopLevelWindow"));

    delete owner;
    QVERIFY(tracker.isNull());
    QVERIFY(!trackedWidget.isNull());
    delete trackedWidget;
}

void KisScreenMigrationTrackerContractTest::visibleWidgetConnectsAndForwardsScreenChanges()
{
    resetSafeAssertDiagnostic();
    QWidget trackedWidget;
    KisScreenMigrationTracker tracker(&trackedWidget);
    QSignalSpy screenChangedSpy(&tracker, &KisScreenMigrationTracker::sigScreenChanged);
    QSignalSpy screenOrResolutionChangedSpy(&tracker, &KisScreenMigrationTracker::sigScreenOrResolutionChanged);

    showTrackedWidget(&trackedWidget);
    QTRY_COMPARE(screenChangedSpy.count(), 1);
    QTRY_COMPARE(screenOrResolutionChangedSpy.count(), 1);

    QScreen *screen = trackedWidget.windowHandle()->screen();
    QCOMPARE(tracker.currentScreen(), screen);
    QCOMPARE(tracker.currentScreenSafe(), screen);
    QCOMPARE(safeAssertCount, 0);
    QCOMPARE(spyScreen(screenChangedSpy), screen);
    QCOMPARE(spyScreen(screenOrResolutionChangedSpy), screen);

    screenChangedSpy.clear();
    screenOrResolutionChangedSpy.clear();
    QVERIFY(QMetaObject::invokeMethod(trackedWidget.windowHandle(),
                                      "screenChanged",
                                      Qt::DirectConnection,
                                      Q_ARG(QScreen *, screen)));

    QCOMPARE(screenChangedSpy.count(), 1);
    QCOMPARE(screenOrResolutionChangedSpy.count(), 1);
    QCOMPARE(spyScreen(screenChangedSpy), screen);
    QCOMPARE(spyScreen(screenOrResolutionChangedSpy), screen);
}

void KisScreenMigrationTrackerContractTest::resolutionChangesAreCoalesced()
{
    resetSafeAssertDiagnostic();
    QWidget trackedWidget;
    KisScreenMigrationTracker tracker(&trackedWidget);
    QSignalSpy screenChangedSpy(&tracker, &KisScreenMigrationTracker::sigScreenChanged);
    QSignalSpy screenOrResolutionChangedSpy(&tracker, &KisScreenMigrationTracker::sigScreenOrResolutionChanged);

    showTrackedWidget(&trackedWidget);
    QTRY_COMPARE(screenChangedSpy.count(), 1);
    QTRY_COMPARE(screenOrResolutionChangedSpy.count(), 1);
    screenChangedSpy.clear();
    screenOrResolutionChangedSpy.clear();

    QScreen *screen = trackedWidget.windowHandle()->screen();
    QVERIFY(QMetaObject::invokeMethod(screen,
                                      "physicalDotsPerInchChanged",
                                      Qt::DirectConnection,
                                      Q_ARG(qreal, screen->physicalDotsPerInch())));
    QVERIFY(QMetaObject::invokeMethod(screen,
                                      "logicalDotsPerInchChanged",
                                      Qt::DirectConnection,
                                      Q_ARG(qreal, screen->logicalDotsPerInch())));

    QTest::qWait(50);
    QCOMPARE(screenOrResolutionChangedSpy.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(screenOrResolutionChangedSpy.count(), 1, 1000);
    QCOMPARE(screenChangedSpy.count(), 0);
    QCOMPARE(spyScreen(screenOrResolutionChangedSpy), screen);
    QCOMPARE(safeAssertCount, 0);
}

QTEST_MAIN(KisScreenMigrationTrackerContractTest)

#include "KisScreenMigrationTrackerContractTest.moc"
