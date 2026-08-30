/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define Q_OS_ANDROID
#include "kis_config_notifier.h"
#undef Q_OS_ANDROID

#include <QSignalSpy>
#include <QTest>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisConfigNotifierAndroidContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pageNavigationEmulationNotificationForwardsBothValues();
    void highFunctionKeyEmulationNotificationForwardsBothValues();
    void historicTabletEventNotificationForwardsBothValues();
};

void KisConfigNotifierAndroidContractTest::pageNavigationEmulationNotificationForwardsBothValues()
{
    KisConfigNotifier notifier;
    QSignalSpy spy(&notifier, &KisConfigNotifier::sigUsePageUpDownMouseButtonEmulationWorkaroundChanged);

    notifier.notifyUsePageUpDownMouseButtonEmulationWorkaroundChanged(false);
    notifier.notifyUsePageUpDownMouseButtonEmulationWorkaroundChanged(true);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QCOMPARE(spy.at(1).at(0).toBool(), true);
}

void KisConfigNotifierAndroidContractTest::highFunctionKeyEmulationNotificationForwardsBothValues()
{
    KisConfigNotifier notifier;
    QSignalSpy spy(&notifier, &KisConfigNotifier::sigUseHighFunctionKeyMouseButtonEmulationWorkaroundChanged);

    notifier.notifyUseHighFunctionKeyMouseButtonEmulationWorkaroundChanged(false);
    notifier.notifyUseHighFunctionKeyMouseButtonEmulationWorkaroundChanged(true);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QCOMPARE(spy.at(1).at(0).toBool(), true);
}

void KisConfigNotifierAndroidContractTest::historicTabletEventNotificationForwardsBothValues()
{
    KisConfigNotifier notifier;
    QSignalSpy spy(&notifier, &KisConfigNotifier::sigUseIgnoreHistoricTabletEventsWorkaroundChanged);

    notifier.notifyUseIgnoreHistoricTabletEventsWorkaroundChanged(false);
    notifier.notifyUseIgnoreHistoricTabletEventsWorkaroundChanged(true);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QCOMPARE(spy.at(1).at(0).toBool(), true);
}

QTEST_GUILESS_MAIN(KisConfigNotifierAndroidContractTest)

#include "KisConfigNotifierAndroidContractTest.moc"
