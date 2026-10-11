/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuApplicationState.h"
#include "KisGpuDocumentDevice.h"
#include <QtTest>
#ifdef KIS_GPU_TEST_MEMORY_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif

/** Application and memory notifications preserve documents across shared GPU replacement. */
class KisGpuApplicationStateTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
#ifdef KIS_GPU_TEST_MEMORY_DBUS
    void nativeMemorySignalUsesTheIsolatedSystemBus() {
        if (!qEnvironmentVariableIsSet("KIS_GPU_TEST_ISOLATED_BUS"))
            QSKIP("Run with an isolated D-Bus system address to exercise native memory signals");
        auto bus = QDBusConnection::systemBus();
        QVERIFY(bus.registerService(QStringLiteral("org.freedesktop.LowMemoryMonitor")));
        KisGpuDocumentDevice documents;
        {
            KisGpuApplicationState state(documents);
            state.applicationStateChanged(Qt::ApplicationActive);
            const auto id = documents.createDocument({});
            auto model = documents.document(id);
            QVERIFY(model);
            QCOMPARE(model->addLayer(QStringLiteral("Native notification")).result, KisGpuLayerDocument::Result::Accepted);
            const auto notify = [&](uchar level) {
                auto signal = QDBusMessage::createSignal(QStringLiteral("/org/freedesktop/LowMemoryMonitor"),
                    QStringLiteral("org.freedesktop.LowMemoryMonitor"), QStringLiteral("LowMemoryWarning"));
                signal << QVariant::fromValue(level);
                return bus.send(signal);
            };
            QVERIFY(notify(255));
            QTRY_VERIFY(documents.isSuspended());
            QVERIFY(!documents.device());
            QCOMPARE(model->state(), KisGpuLayerDocument::State::Suspended);
            QVERIFY(notify(0));
            QTRY_VERIFY(!documents.isSuspended());
            QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        }
        QVERIFY(documents.isSuspended());
        QVERIFY(bus.unregisterService(QStringLiteral("org.freedesktop.LowMemoryMonitor")));
    }
#endif
    void emptyApplicationKeepsGpuCreationLazy() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState state(documents);
        QVERIFY(QMetaObject::invokeMethod(qApp, "applicationStateChanged", Qt::DirectConnection,
            Q_ARG(Qt::ApplicationState, Qt::ApplicationSuspended)));
        QVERIFY(documents.isSuspended());
        QVERIFY(!documents.device());
        QVERIFY(QMetaObject::invokeMethod(qApp, "applicationStateChanged", Qt::DirectConnection,
            Q_ARG(Qt::ApplicationState, Qt::ApplicationActive)));
        QVERIFY(!documents.isSuspended());
        QVERIFY(!documents.device());
        QVERIFY(!state.isPolling());
    }
    void notificationsKeepAcceptedEditingAndInvalidateOldTokens() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState state(documents);
        state.applicationStateChanged(Qt::ApplicationActive);
        const auto id = documents.createDocument({});
        auto model = documents.document(id);
        QVERIFY(model);
        model->addLayer(QStringLiteral("Retained"));
        auto edit = model->begin(1);
        KisGpuTileStore::DabCommand dab;
        dab.center = {16, 16}; dab.diameter = {8, 8}; dab.rgba = 0xFF123456;
        QCOMPARE(edit.session->append(edit.token, {dab}, {0, 0, 64, 64}), KisGpuEditSession::Result::Accepted);
        const auto accepted = model->recovery().working->data.working->size();
        auto store = documents.store(id);
        const auto before = store->statistics();
        QVERIFY(state.isPolling());
        state.applicationStateChanged(Qt::ApplicationInactive);
        QVERIFY(documents.device());
        state.memoryPressure(50);
        QTRY_VERIFY(!documents.historyEvictionPending());
        QCOMPARE(model->state(), KisGpuLayerDocument::State::Editing);
        state.memoryPressure(255);
        QVERIFY(documents.isSuspended());
        QVERIFY(!documents.device());
        QVERIFY(!state.isPolling());
        QCOMPARE(model->recovery().working->data.working->size(), accepted);
        QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
        state.memoryPressure(0);
        QVERIFY(documents.isSuspended());
        QVERIFY(!state.resumeForInput());
        state.applicationStateChanged(Qt::ApplicationActive);
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Editing);
        QCOMPARE(model->commit(edit.token), KisGpuLayerDocument::Result::Stale);
        const auto resumed = model->currentEditing();
        QCOMPARE(model->commit(resumed.token), KisGpuLayerDocument::Result::Accepted);
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        QCOMPARE(documents.store(id)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(documents.store(id)->statistics().pixelReadbackBytes, quint64(0));
        QVERIFY(documents.closeDocument(id));
        QVERIFY(!state.isPolling());
        QVERIFY(!documents.device());
    }
    void criticalPressureRetriesOnInputAndDeviceLossRestoresOnce() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState state(documents);
        state.applicationStateChanged(Qt::ApplicationActive);
        const auto id = documents.createDocument({});
        auto model = documents.document(id);
        QVERIFY(model);
        model->addLayer(QStringLiteral("Restored"));
        state.memoryPressure(255);
        QVERIFY(documents.isSuspended());
        QVERIFY(state.resumeForInput());
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        QSignalSpy restored(&documents, &KisGpuDocumentDevice::gpuRestored);
        documents.device()->destroy();
        QTRY_VERIFY(documents.device() && documents.device()->available());
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        QCOMPARE(restored.count(), 1);
        QCOMPARE(model->layers().first().name, QStringLiteral("Restored"));
        state.applicationStateChanged(Qt::ApplicationSuspended);
        state.memoryPressure(0);
        QVERIFY(documents.isSuspended());
        state.applicationStateChanged(Qt::ApplicationActive);
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        state.shutdown();
        QVERIFY(documents.isSuspended());
        state.poll();
        QVERIFY(documents.isSuspended());
        QVERIFY(!state.resumeForInput());
        state.applicationStateChanged(Qt::ApplicationActive);
        QVERIFY(documents.isSuspended());
        QVERIFY(!state.isPolling());
    }
};
QTEST_MAIN(KisGpuApplicationStateTest)
#include "KisGpuApplicationStateTest.moc"
