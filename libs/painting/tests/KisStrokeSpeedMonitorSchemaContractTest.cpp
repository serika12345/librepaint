/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokeSpeedMonitor.h"

#include <QTest>

#include <type_traits>

#define ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisStrokeSpeedMonitor::method)), signature>)

class KisStrokeSpeedMonitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokeSpeedMonitorTypeLifetimeAndAccessSchemaRemainStable();
    void strokeSpeedMonitorMeasurementStateAndRecordingSignaturesRemainStable();
    void strokeSpeedMonitorLastStrokeSignaturesRemainStable();
    void strokeSpeedMonitorAverageMetricSignaturesRemainStable();
    void strokeSpeedMonitorNotificationSignatureRemainsStable();
};

void KisStrokeSpeedMonitorSchemaContractTest::strokeSpeedMonitorTypeLifetimeAndAccessSchemaRemainStable()
{
    static_assert(std::is_class_v<KisStrokeSpeedMonitor>);
    static_assert(std::is_base_of_v<QObject, KisStrokeSpeedMonitor>);
    static_assert(std::is_default_constructible_v<KisStrokeSpeedMonitor>);
    static_assert(std::is_destructible_v<KisStrokeSpeedMonitor>);
    static_assert(std::has_virtual_destructor_v<KisStrokeSpeedMonitor>);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(instance, KisStrokeSpeedMonitor * (*)());
}

void KisStrokeSpeedMonitorSchemaContractTest::strokeSpeedMonitorMeasurementStateAndRecordingSignaturesRemainStable()
{
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(haveStrokeSpeedMeasurement, bool (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(notifyStrokeFinished,
                                          void (KisStrokeSpeedMonitor::*)(qreal, qreal, qreal, KisPaintOpPresetSP));
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(setHaveStrokeSpeedMeasurement, void (KisStrokeSpeedMonitor::*)(bool));
}

void KisStrokeSpeedMonitorSchemaContractTest::strokeSpeedMonitorLastStrokeSignaturesRemainStable()
{
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastCursorSpeed, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastFps, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastPresetName, QString (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastPresetSize, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastRenderingSpeed, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(lastStrokeSaturated, bool (KisStrokeSpeedMonitor::*)() const);
}

void KisStrokeSpeedMonitorSchemaContractTest::strokeSpeedMonitorAverageMetricSignaturesRemainStable()
{
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(avgCursorSpeed, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(avgFps, qreal (KisStrokeSpeedMonitor::*)() const);
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(avgRenderingSpeed, qreal (KisStrokeSpeedMonitor::*)() const);
}

void KisStrokeSpeedMonitorSchemaContractTest::strokeSpeedMonitorNotificationSignatureRemainsStable()
{
    ASSERT_STROKE_SPEED_MONITOR_SIGNATURE(sigStatsUpdated, void (KisStrokeSpeedMonitor::*)());
}

#undef ASSERT_STROKE_SPEED_MONITOR_SIGNATURE

QTEST_GUILESS_MAIN(KisStrokeSpeedMonitorSchemaContractTest)
#include "KisStrokeSpeedMonitorSchemaContractTest.moc"
