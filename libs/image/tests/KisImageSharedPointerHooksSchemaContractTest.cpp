/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_idle_watcher.h"
#include "kis_onion_skin_compositor.h"
#include "kis_types.h"
#include "kis_update_time_monitor.h"

#include <QTest>

#include <cstddef>
#include <functional>
#include <type_traits>

#define ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(member, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisUpdateTimeMonitor::member)), signature>)

#define ASSERT_IDLE_WATCHER_SIGNATURE(member, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisIdleWatcher::member)), signature>)

#define ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(member, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisOnionSkinCompositor::member)), signature>)

class KisImageSharedPointerHooksSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSharedPointerNodeAndLayerHooksRemainStable();
    void imageSharedPointerConfigurationHooksRemainStable();
    void imageSharedPointerSelectionHooksRemainStable();
    void imageSharedPointerQtHashSignaturesRemainStable();
    void imageSharedPointerStandardHashSchemaRemainStable();
    void updateTimeMonitorTypeLifetimeAndSingletonSchemaRemainStable();
    void updateTimeMonitorStrokeMeasurementSignaturesRemainStable();
    void updateTimeMonitorInteractionAndUpdateSignaturesRemainStable();
    void updateTimeMonitorJobSignaturesRemainStable();
    void idleWatcherTypeLifetimeAndConstructionSchemaRemainStable();
    void idleWatcherStateAndImageTrackingSignaturesRemainStable();
    void idleWatcherMemoryAndCountdownSignaturesRemainStable();
    void idleWatcherNotificationSignaturesRemainStable();
    void onionSkinCompositorTypeLifetimeAndSingletonSchemaRemainStable();
    void onionSkinCompositorCompositeAndFullExtentSignaturesRemainStable();
    void onionSkinCompositorExtentSignaturesRemainStable();
    void onionSkinCompositorConfigAndSignalsRemainStable();
};

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerNodeAndLayerHooksRemainStable()
{
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(KisNode *)>(&kisSharedPtrAddReference)), void (*)(KisNode *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<bool (*)(KisNode *)>(&kisSharedPtrRelease)), bool (*)(KisNode *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisGroupLayer *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisGroupLayer *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisFilterMask *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisFilterMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisFilterMask *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisFilterMask *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerConfigurationHooksRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisPaintOpSettings *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisPaintOpSettings *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisPaintOpSettings *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisPaintOpSettings *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(KisPropertiesConfiguration *)>(&kisSharedPtrAddReference)),
                       void (*)(KisPropertiesConfiguration *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisPropertiesConfiguration *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisPropertiesConfiguration *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerSelectionHooksRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisSelection *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisSelection *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisSelection *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisSelection *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisSelectionMask *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisSelectionMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisSelectionMask *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisSelectionMask *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerQtHashSignaturesRemainStable()
{
    using StrongHashSignature = uint (*)(KisSharedPtr<KisNode>);
    using WeakHashSignature = uint (*)(KisWeakSharedPtr<KisNode>);

    static_assert(std::is_same_v<decltype(static_cast<StrongHashSignature>(&qHash<KisNode>)), StrongHashSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WeakHashSignature>(&qHash<KisNode>)), WeakHashSignature>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerStandardHashSchemaRemainStable()
{
    using StrongHasher = std::hash<KisSharedPtr<KisNode>>;
    using WeakHasher = std::hash<KisWeakSharedPtr<KisNode>>;
    using StrongHashSignature = std::size_t (StrongHasher::*)(const KisSharedPtr<KisNode> &) const;
    using WeakHashSignature = std::size_t (WeakHasher::*)(const KisWeakSharedPtr<KisNode> &) const;

    static_assert(std::is_class_v<StrongHasher>);
    static_assert(
        std::is_same_v<decltype(static_cast<StrongHashSignature>(&StrongHasher::operator())), StrongHashSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WeakHashSignature>(&WeakHasher::operator())), WeakHashSignature>);
}

void KisImageSharedPointerHooksSchemaContractTest::updateTimeMonitorTypeLifetimeAndSingletonSchemaRemainStable()
{
    static_assert(std::is_class_v<KisUpdateTimeMonitor>);
    static_assert(std::is_default_constructible_v<KisUpdateTimeMonitor>);
    static_assert(std::is_destructible_v<KisUpdateTimeMonitor>);
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(instance, KisUpdateTimeMonitor * (*)());
}

void KisImageSharedPointerHooksSchemaContractTest::updateTimeMonitorStrokeMeasurementSignaturesRemainStable()
{
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(startStrokeMeasure, void (KisUpdateTimeMonitor::*)());
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(endStrokeMeasure, void (KisUpdateTimeMonitor::*)());
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(reportPaintOpPreset, void (KisUpdateTimeMonitor::*)(KisPaintOpPresetSP));
}

void KisImageSharedPointerHooksSchemaContractTest::updateTimeMonitorInteractionAndUpdateSignaturesRemainStable()
{
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(reportMouseMove, void (KisUpdateTimeMonitor::*)(const QPointF &));
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(printValues, void (KisUpdateTimeMonitor::*)());
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(reportUpdateFinished, void (KisUpdateTimeMonitor::*)(const QRect &));
}

void KisImageSharedPointerHooksSchemaContractTest::updateTimeMonitorJobSignaturesRemainStable()
{
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(reportJobStarted, void (KisUpdateTimeMonitor::*)(void *));
    ASSERT_UPDATE_TIME_MONITOR_SIGNATURE(reportJobFinished,
                                         void (KisUpdateTimeMonitor::*)(void *, const QVector<QRect> &));
}

void KisImageSharedPointerHooksSchemaContractTest::idleWatcherTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisIdleWatcher>);
    static_assert(std::is_base_of_v<QObject, KisIdleWatcher>);
    static_assert(std::is_constructible_v<KisIdleWatcher, int, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisIdleWatcher>);
}

void KisImageSharedPointerHooksSchemaContractTest::idleWatcherStateAndImageTrackingSignaturesRemainStable()
{
    ASSERT_IDLE_WATCHER_SIGNATURE(isIdle, bool (KisIdleWatcher::*)() const);
    ASSERT_IDLE_WATCHER_SIGNATURE(isCounting, bool (KisIdleWatcher::*)() const);
    ASSERT_IDLE_WATCHER_SIGNATURE(setTrackedImages, void (KisIdleWatcher::*)(const QVector<KisImageSP> &));
    ASSERT_IDLE_WATCHER_SIGNATURE(setTrackedImage, void (KisIdleWatcher::*)(KisImageSP));
}

void KisImageSharedPointerHooksSchemaContractTest::idleWatcherMemoryAndCountdownSignaturesRemainStable()
{
    ASSERT_IDLE_WATCHER_SIGNATURE(connectMemoryStatisticsUpdates, void (KisIdleWatcher::*)());
    ASSERT_IDLE_WATCHER_SIGNATURE(forceImageModified, void (KisIdleWatcher::*)());
    ASSERT_IDLE_WATCHER_SIGNATURE(restartCountdown, void (KisIdleWatcher::*)());
    ASSERT_IDLE_WATCHER_SIGNATURE(triggerCountdownNoDelay, void (KisIdleWatcher::*)());
}

void KisImageSharedPointerHooksSchemaContractTest::idleWatcherNotificationSignaturesRemainStable()
{
    ASSERT_IDLE_WATCHER_SIGNATURE(startedIdleMode, void (KisIdleWatcher::*)());
    ASSERT_IDLE_WATCHER_SIGNATURE(imageModified, void (KisIdleWatcher::*)());
}

void KisImageSharedPointerHooksSchemaContractTest::onionSkinCompositorTypeLifetimeAndSingletonSchemaRemainStable()
{
    static_assert(std::is_class_v<KisOnionSkinCompositor>);
    static_assert(std::is_base_of_v<QObject, KisOnionSkinCompositor>);
    static_assert(std::is_default_constructible_v<KisOnionSkinCompositor>);
    static_assert(std::has_virtual_destructor_v<KisOnionSkinCompositor>);
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(instance, KisOnionSkinCompositor * (*)());
}

void KisImageSharedPointerHooksSchemaContractTest::onionSkinCompositorCompositeAndFullExtentSignaturesRemainStable()
{
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(
        composite,
        void (KisOnionSkinCompositor::*)(KisPaintDeviceSP, KisPaintDeviceSP, const QRect &));
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(calculateFullExtent, QRect (KisOnionSkinCompositor::*)(KisPaintDeviceSP));
}

void KisImageSharedPointerHooksSchemaContractTest::onionSkinCompositorExtentSignaturesRemainStable()
{
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(calculateExtent, QRect (KisOnionSkinCompositor::*)(KisPaintDeviceSP));
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(calculateExtent, QRect (KisOnionSkinCompositor::*)(KisPaintDeviceSP, int));
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(updateExtentOnAddition,
                                           QRect (KisOnionSkinCompositor::*)(KisPaintDeviceSP, int));
}

void KisImageSharedPointerHooksSchemaContractTest::onionSkinCompositorConfigAndSignalsRemainStable()
{
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(configSeqNo, int (KisOnionSkinCompositor::*)() const);
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(setColorLabelFilter, void (KisOnionSkinCompositor::*)(QSet<int>));
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(colorLabelFilter, QSet<int> (KisOnionSkinCompositor::*)());
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(configChanged, void (KisOnionSkinCompositor::*)());
    ASSERT_ONION_SKIN_COMPOSITOR_SIGNATURE(sigOnionSkinChanged, void (KisOnionSkinCompositor::*)());
}

QTEST_APPLESS_MAIN(KisImageSharedPointerHooksSchemaContractTest)

#include "KisImageSharedPointerHooksSchemaContractTest.moc"
