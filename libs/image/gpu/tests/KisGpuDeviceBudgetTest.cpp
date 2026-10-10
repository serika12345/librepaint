/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include "KisGpuTestDevice.h"
#include <limits>

namespace {
using Store = KisGpuTileStore;
bool finish(Store &store, const Store::Completion &completion) {
    QElapsedTimer timer; timer.start();
    while (completion.status() == Store::Status::Pending && timer.elapsed() < 5000) { store.poll(); QTest::qWait(1); }
    return completion.status() == Store::Status::Succeeded;
}
}
/** All document owners on one GPU share a finite logical allocation budget. */
class KisGpuDeviceBudgetTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void explicitTransfersPreflightSharedCapacity_data() {
        QTest::addColumn<bool>("reading");
        QTest::newRow("readback") << true;
        QTest::newRow("upload") << false;
    }
    void explicitTransfersPreflightSharedCapacity() {
        QFETCH(bool, reading);
        KisGpuTestDevice gpu(0, false, 2 * Store::TileBytes + 64);
        Store store(gpu.owner, 8 * Store::TileBytes);
        auto base = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF332211);
        QVERIFY(finish(store, base.completion));
        auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory() - 3);
        const auto before = store.statistics();
        const auto reserved = gpu.owner.memoryStatistics().reservedBytes;
        const auto error = reading ? store.readback(base.version, {0, 0, 1, 1}).error
                                  : store.upload(base.version, {0, 0, 1, 1}, QByteArray::fromHex("aabbccff")).error;
        QCOMPARE(error, Store::Error::BudgetExceeded);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, reserved);
        occupied = {};
        if (reading) {
            const auto retried = store.readback(base.version, {0, 0, 1, 1});
            QCOMPARE(retried.error, Store::Error::None);
            QVERIFY(finish(store, retried.completion));
            QCOMPARE(retried.bytes(), QByteArray::fromHex("112233ff"));
        } else {
            const auto retried = store.upload(base.version, {0, 0, 1, 1}, QByteArray::fromHex("aabbccff"));
            QCOMPARE(retried.error, Store::Error::None);
            QVERIFY(finish(store, retried.completion));
            QCOMPARE(gpu.read(retried.version, {0, 0, 1, 1}), QByteArray::fromHex("aabbccff"));
            QCOMPARE(gpu.read(base.version, {0, 0, 1, 1}), QByteArray::fromHex("112233ff"));
        }
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void pendingWorkAndRetainedVersionsShareOneLimit() {
        KisGpuTestDevice gpu(0, false, 2 * Store::TileBytes + 64);
        Store first(gpu.owner, 8 * Store::TileBytes), second(gpu.owner, 8 * Store::TileBytes);
        auto a = first.fill(first.emptyVersion(), {0, 0, 64, 64}, 0xFF0000FF);
        QCOMPARE(a.error, Store::Error::None);
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes + 64);
        const auto refused = second.fill(second.emptyVersion(), {0, 0, 1, 1}, 0xFFFF0000);
        QCOMPARE(refused.error, Store::Error::BudgetExceeded);
        QCOMPARE(second.statistics().submissions, quint64(0));
        QCOMPARE(second.statistics().residentBytes, quint64(0));
        QVERIFY(finish(first, a.completion));
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes);
        auto b = second.fill(second.emptyVersion(), {0, 0, 1, 1}, 0xFFFF0000);
        QCOMPARE(b.error, Store::Error::None);
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, 2 * Store::TileBytes + 64);
        QVERIFY(finish(second, b.completion));
        QCOMPARE(first.fill(a.version, {0, 0, 1, 1}, 0xFF00FF00).error, Store::Error::BudgetExceeded);
        a = {};
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes);
        const auto retried = second.fill(b.version, {0, 0, 1, 1}, 0xFF00FF00);
        QCOMPARE(retried.error, Store::Error::None);
        QVERIFY(finish(second, retried.completion));
        QCOMPARE(gpu.read(b.version, {0, 0, 1, 1}), QByteArray::fromHex("0000ffff"));
        QCOMPARE(gpu.read(retried.version, {0, 0, 1, 1}), QByteArray::fromHex("00ff00ff"));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void reservationsResizeMoveAndSurviveTheirDeviceOwner() {
        KisGpuDevice::MemoryReservation retained;
        {
            KisGpuDevice gpu(0, false, 100);
            auto memory = gpu.reserveMemory(64);
            QVERIFY(bool(memory));
            QCOMPARE(memory.bytes(), quint64(64));
            QVERIFY(!gpu.reserveMemory(std::numeric_limits<quint64>::max()));
            QVERIFY(!memory.tryResize(101));
            auto other = gpu.reserveMemory(36);
            QVERIFY(bool(other));
            QCOMPARE(gpu.memoryStatistics().reservedBytes, quint64(100));
            QVERIFY(!memory.tryResize(65));
            QVERIFY(other.tryResize(0));
            QVERIFY(memory.tryResize(100));
            retained = std::move(memory);
            QVERIFY(!memory);
            QCOMPARE(retained.bytes(), quint64(100));
            gpu.destroy();
            QVERIFY(!retained.tryResize(101));
            QVERIFY(retained.tryResize(50));
            QCOMPARE(gpu.memoryStatistics().reservedBytes, quint64(50));
            QCOMPARE(gpu.availableMemory(), quint64(0));
        }
        QCOMPARE(retained.bytes(), quint64(50));
        QVERIFY(!retained.tryResize(51));
        QVERIFY(retained.tryResize(0));
        retained = {};
    }
    void displayedTexturesAndPatternsRetainTheirReservation() {
        KisGpuTestDevice gpu(0, false, 2 * Store::TileBytes + 64);
        Store::TextureSnapshot retained;
        {
            Store transient(gpu.owner, 8 * Store::TileBytes);
            auto painted = transient.fill(transient.emptyVersion(), {0, 0, 64, 64}, 0xFF0000FF);
            QVERIFY(finish(transient, painted.completion));
            retained = transient.textureSnapshot(painted.version, {0, 0, 64, 64});
            QVERIFY(finish(transient, retained.completion));
            QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, 2 * Store::TileBytes);
        }
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes);
        Store store(gpu.owner, 8 * Store::TileBytes);
        auto painted = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF00FF00);
        QVERIFY(finish(store, painted.completion));
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, 2 * Store::TileBytes);
        QCOMPARE(store.uploadBrushTexture({65, 1}, QByteArray(65, 'x')).error, Store::Error::BudgetExceeded);
        retained = {};
        auto texture = store.uploadBrushTexture({65, 1}, QByteArray(65, 'x'));
        QCOMPARE(texture.error, Store::Error::None);
        QVERIFY(finish(store, texture.completion));
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes + 68);
        texture = {};
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, Store::TileBytes);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void profilingStorageParticipatesInGlobalPreflight() {
        KisGpuTestDevice gpu(0, true, Store::TileBytes + 64);
        Store store(gpu.owner, 8 * Store::TileBytes);
        QCOMPARE(store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF0000FF).error, Store::Error::BudgetExceeded);
        QCOMPARE(store.statistics().submissions, quint64(0));
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
};
QTEST_GUILESS_MAIN(KisGpuDeviceBudgetTest)
#include "KisGpuDeviceBudgetTest.moc"
