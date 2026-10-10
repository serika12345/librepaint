/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <limits>

namespace {
using Store = KisGpuTileStore;
bool finish(Store &store, const Store::Completion &completion)
{
    QElapsedTimer timer;
    timer.start();
    while (completion.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        store.poll();
        QTest::qWait(1);
    }
    store.poll();
    return completion.status() == Store::Status::Succeeded;
}
}

/** Display consumers need immutable, budgeted GPU images without CPU pixel traffic. */
class KisGpuTileTextureTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void sparsePixelsAndPendingVersion();
    void exclusiveSnapshotUpdatesOnlyChangedTiles_data();
    void exclusiveSnapshotUpdatesOnlyChangedTiles();
    void sharedSnapshotRemainsImmutableDuringReuse();
    void pendingSnapshotUsesFreshStorage();
    void reuseRefusalPreservesPreviousImage();
    void emptyPixelsAndCoordinateLimits();
    void unallocatedGapsStayTransparent();
    void snapshotOutlivesStore();
    void droppedSnapshotRemainsBudgetedUntilCollection();
    void rejectedSnapshotsLeaveQueueAndBudgetUnchanged();
    void deviceLossRejectsSnapshots();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuTileTextureTest::initTestCase()
{
    m_gpu = std::make_unique<KisGpuTestDevice>();
    qInfo() << "GPU:" << m_gpu->name;
}

void KisGpuTileTextureTest::cleanup()
{
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    QCOMPARE(m_gpu->owner.errorCount(), 0);
}

void KisGpuTileTextureTest::sparsePixelsAndPendingVersion()
{
    Store store(m_gpu->owner, 32 * Store::TileBytes);
    const QRect bounds(-67, -5, 139, 11), painted(-65, -2, 131, 5);
    auto original = store.fill(store.emptyVersion(), painted, 0x00402010);
    auto snapshot = store.textureSnapshot(original.version, bounds);
    QCOMPARE(snapshot.error, Store::Error::None);
    QVERIFY(snapshot.texture());
    QCOMPARE(snapshot.bounds(), bounds);
    auto changed = store.fill(original.version, painted, 0xFF998877);
    original = {};
    QVERIFY(finish(store, snapshot.completion));
    QVERIFY(finish(store, changed.completion));
    QByteArray expected(bounds.width() * bounds.height() * 4, '\0');
    for (int y = painted.top(); y <= painted.bottom(); ++y) {
        for (int x = painted.left(); x <= painted.right(); ++x) {
            const int offset = ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4;
            expected[offset] = 0x10;
            expected[offset + 1] = 0x20;
            expected[offset + 2] = 0x40;
        }
    }
    QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), expected);
    const auto statistics = store.statistics();
    QCOMPARE(statistics.textureCopyBytes, quint64(4 * 11 * bounds.width()));
    QCOMPARE(statistics.pixelReadbackBytes, quint64(0));
    QCOMPARE(statistics.pixelUploadBytes, quint64(0));
    QVERIFY(!snapshot.completion.gpuComputeNanoseconds());
    changed = {};
    store.poll();
    QCOMPARE(store.statistics().residentBytes, quint64(expected.size()));
    snapshot = {};
    QCOMPARE(store.statistics().residentBytes, quint64(0));
}

void KisGpuTileTextureTest::emptyPixelsAndCoordinateLimits()
{
    Store store(m_gpu->owner, Store::TileBytes);
    const QVector<QRect> rectangles{
        QRect(-1, -1, 3, 3),
        QRect(std::numeric_limits<int>::max() - 4, std::numeric_limits<int>::min(), 5, 3)};
    for (QRect bounds : rectangles) {
        auto snapshot = store.textureSnapshot(store.emptyVersion(), bounds);
        QCOMPARE(snapshot.error, Store::Error::None);
        QVERIFY(finish(store, snapshot.completion));
        QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), QByteArray(bounds.width() * bounds.height() * 4, '\0'));
    }
    QCOMPARE(store.statistics().textureCopyBytes, quint64(0));
}

void KisGpuTileTextureTest::exclusiveSnapshotUpdatesOnlyChangedTiles_data()
{
    QTest::addColumn<QRect>("bounds");
    QTest::newRow("whole-tiles") << QRect(0, 0, 128, 64);
    QTest::newRow("negative-partial-tiles") << QRect(-67, -5, 139, 11);
}

void KisGpuTileTextureTest::exclusiveSnapshotUpdatesOnlyChangedTiles()
{
    QFETCH(QRect, bounds);
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    const auto original = store.fill(store.emptyVersion(), bounds, 0xFF332211);
    auto snapshot = store.textureSnapshot(original.version, bounds);
    QVERIFY(finish(store, snapshot.completion));
    const auto changed = store.fill(original.version, {1, 1, 4, 3}, 0xCC998877);
    QVERIFY(finish(store, changed.completion));
    const auto expected = m_gpu->read(changed.version, bounds);
    auto occupied = m_gpu->owner.reserveMemory(m_gpu->owner.availableMemory());
    const auto before = store.statistics();
    const auto reserved = m_gpu->owner.memoryStatistics().reservedBytes;
    QCOMPARE(before.imageTextureAllocationBytes, quint64(bounds.width()) * bounds.height() * 4);
    auto update = store.textureSnapshot(changed.version, bounds, std::move(snapshot));
    QCOMPARE(update.error, Store::Error::None);
    QVERIFY(!snapshot.texture());
    QCOMPARE(m_gpu->owner.memoryStatistics().reservedBytes, reserved);
    QVERIFY(finish(store, update.completion));
    QCOMPARE(m_gpu->read(update.texture(), bounds.size()), expected);
    QCOMPARE(store.statistics().imageTextureAllocationBytes, before.imageTextureAllocationBytes);
    const auto copied = bounds.intersected(QRect(0, 0, 64, 64));
    QCOMPARE(store.statistics().textureCopyBytes - before.textureCopyBytes,
             quint64(copied.width()) * copied.height() * 4);
    const auto updated = store.statistics();
    snapshot = store.textureSnapshot(changed.version, bounds, std::move(update));
    QCOMPARE(snapshot.error, Store::Error::None);
    QCOMPARE(snapshot.completion.status(), Store::Status::Succeeded);
    QCOMPARE(store.statistics().submissions, updated.submissions);
    QCOMPARE(store.statistics().textureCopyBytes, updated.textureCopyBytes);
    const auto refused = store.textureSnapshot(store.emptyVersion(), bounds, std::move(snapshot));
    QCOMPARE(refused.error, Store::Error::BudgetExceeded);
    QVERIFY(snapshot.texture());
    QCOMPARE(snapshot.completion.status(), Store::Status::Succeeded);
    QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), expected);
    QCOMPARE(store.statistics().submissions, updated.submissions);
    occupied = {};
    auto cleared = store.textureSnapshot(store.emptyVersion(), bounds, std::move(snapshot));
    QCOMPARE(cleared.error, Store::Error::None);
    QVERIFY(finish(store, cleared.completion));
    QCOMPARE(m_gpu->read(cleared.texture(), bounds.size()), QByteArray(bounds.width() * bounds.height() * 4, '\0'));
    QCOMPARE(store.statistics().textureCopyBytes - updated.textureCopyBytes,
             quint64(bounds.width()) * bounds.height() * 4);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuTileTextureTest::sharedSnapshotRemainsImmutableDuringReuse()
{
    Store store(m_gpu->owner, 16 * Store::TileBytes);
    const QRect bounds(0, 0, 128, 64);
    const auto original = store.fill(store.emptyVersion(), bounds, 0xFF332211);
    auto snapshot = store.textureSnapshot(original.version, bounds);
    QVERIFY(finish(store, snapshot.completion));
    const auto held = snapshot;
    const auto expectedOriginal = m_gpu->read(held.texture(), bounds.size());
    const auto changed = store.fill(original.version, {1, 1, 4, 3}, 0xCC998877);
    QVERIFY(finish(store, changed.completion));
    auto occupied = m_gpu->owner.reserveMemory(m_gpu->owner.availableMemory());
    const auto before = store.statistics();
    const auto refused = store.textureSnapshot(changed.version, bounds, std::move(snapshot));
    QCOMPARE(refused.error, Store::Error::BudgetExceeded);
    QVERIFY(snapshot.texture());
    QCOMPARE(store.statistics().submissions, before.submissions);
    occupied = {};
    auto update = store.textureSnapshot(changed.version, bounds, std::move(snapshot));
    QCOMPARE(update.error, Store::Error::None);
    QVERIFY(!snapshot.texture());
    QVERIFY(finish(store, update.completion));
    QCOMPARE(m_gpu->read(update.texture(), bounds.size()), m_gpu->read(changed.version, bounds));
    QCOMPARE(m_gpu->read(held.texture(), bounds.size()), expectedOriginal);
    QCOMPARE(store.statistics().textureCopyBytes - before.textureCopyBytes, 2 * Store::TileBytes);
    QCOMPARE(store.statistics().imageTextureAllocationBytes - before.imageTextureAllocationBytes, 2 * Store::TileBytes);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuTileTextureTest::snapshotOutlivesStore()
{
    Store::TextureSnapshot snapshot;
    const QRect bounds(62, -2, 5, 3);
    {
        Store store(m_gpu->owner, 8 * Store::TileBytes);
        auto edit = store.fill(store.emptyVersion(), bounds, 0x80706050);
        snapshot = store.textureSnapshot(edit.version, bounds);
        QCOMPARE(snapshot.error, Store::Error::None);
    }
    QCOMPARE(snapshot.completion.status(), Store::Status::Succeeded);
    QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), QByteArray("\x50\x60\x70\x80", 4).repeated(15));
}

void KisGpuTileTextureTest::pendingSnapshotUsesFreshStorage()
{
    Store store(m_gpu->owner, 16 * Store::TileBytes);
    const QRect bounds(0, 0, 128, 64);
    const auto original = store.fill(store.emptyVersion(), bounds, 0xFF332211);
    auto snapshot = store.textureSnapshot(original.version, bounds);
    const auto previousCompletion = snapshot.completion;
    QCOMPARE(previousCompletion.status(), Store::Status::Pending);
    const auto changed = store.fill(original.version, {1, 1, 4, 3}, 0xCC998877);
    const auto before = store.statistics();
    auto update = store.textureSnapshot(changed.version, bounds, std::move(snapshot));
    QCOMPARE(update.error, Store::Error::None);
    QVERIFY(!snapshot.texture());
    QCOMPARE(store.statistics().residentBytes, before.residentBytes + 2 * Store::TileBytes);
    QVERIFY(finish(store, update.completion));
    QCOMPARE(previousCompletion.status(), Store::Status::Succeeded);
    QCOMPARE(m_gpu->read(update.texture(), bounds.size()), m_gpu->read(changed.version, bounds));
    QCOMPARE(store.statistics().textureCopyBytes - before.textureCopyBytes, 2 * Store::TileBytes);
}

void KisGpuTileTextureTest::reuseRefusalPreservesPreviousImage()
{
    Store store(m_gpu->owner, 8 * Store::TileBytes, 1), other(m_gpu->owner, Store::TileBytes);
    const QRect bounds(0, 0, 64, 64);
    const auto original = store.fill(store.emptyVersion(), bounds, 0xFF332211);
    QVERIFY(finish(store, original.completion));
    auto snapshot = store.textureSnapshot(original.version, bounds);
    QVERIFY(finish(store, snapshot.completion));
    const auto expected = m_gpu->read(snapshot.texture(), bounds.size());
    const auto before = store.statistics();
    QCOMPARE(store.textureSnapshot(other.emptyVersion(), bounds, std::move(snapshot)).error, Store::Error::InvalidVersion);
    QCOMPARE(store.textureSnapshot(original.version, {}, std::move(snapshot)).error, Store::Error::InvalidCommand);
    QCOMPARE(store.textureSnapshot(original.version, {0, 0, 1024, 1024}, std::move(snapshot)).error, Store::Error::BudgetExceeded);
    QCOMPARE(store.statistics().submissions, before.submissions);
    const auto changed = store.fill(original.version, {1, 1, 4, 3}, 0xCC998877);
    QCOMPARE(store.textureSnapshot(changed.version, bounds, std::move(snapshot)).error, Store::Error::QueueFull);
    QVERIFY(snapshot.texture());
    QCOMPARE(snapshot.completion.status(), Store::Status::Succeeded);
    QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), expected);
    QVERIFY(finish(store, changed.completion));
    auto retry = store.textureSnapshot(changed.version, bounds, std::move(snapshot));
    QCOMPARE(retry.error, Store::Error::None);
    QVERIFY(finish(store, retry.completion));
    QCOMPARE(m_gpu->read(retry.texture(), bounds.size()), m_gpu->read(changed.version, bounds));
}

void KisGpuTileTextureTest::unallocatedGapsStayTransparent()
{
    Store store(m_gpu->owner, 8 * Store::TileBytes);
    const QRect bounds(-130, 0, 261, 3);
    auto left = store.fill(store.emptyVersion(), QRect(-130, 0, 2, 3), 0xFF302010);
    auto right = store.fill(left.version, QRect(129, 0, 2, 3), 0xFF605040);
    auto snapshot = store.textureSnapshot(right.version, bounds);
    QCOMPARE(snapshot.error, Store::Error::None);
    QVERIFY(finish(store, snapshot.completion));
    QByteArray expected;
    for (int y = 0; y < 3; ++y) {
        expected += QByteArray("\x10\x20\x30\xFF", 4).repeated(2);
        expected += QByteArray(257 * 4, '\0');
        expected += QByteArray("\x40\x50\x60\xFF", 4).repeated(2);
    }
    QCOMPARE(m_gpu->read(snapshot.texture(), bounds.size()), expected);
    QCOMPARE(store.statistics().textureCopyBytes, quint64(5 * 3 * 4));
}

void KisGpuTileTextureTest::droppedSnapshotRemainsBudgetedUntilCollection()
{
    Store store(m_gpu->owner, Store::TileBytes);
    auto snapshot = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 64, 64));
    QCOMPARE(snapshot.error, Store::Error::None);
    const auto completion = snapshot.completion;
    snapshot = {};
    QCOMPARE(store.statistics().residentBytes, Store::TileBytes);
    auto rejected = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QCOMPARE(rejected.error, Store::Error::BudgetExceeded);
    QVERIFY(finish(store, completion));
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    auto retry = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 64, 64));
    QCOMPARE(retry.error, Store::Error::None);
    QVERIFY(finish(store, retry.completion));
}

void KisGpuTileTextureTest::rejectedSnapshotsLeaveQueueAndBudgetUnchanged()
{
    Store store(m_gpu->owner, Store::TileBytes, 1), other(m_gpu->owner, Store::TileBytes);
    QCOMPARE(store.textureSnapshot({}, QRect(0, 0, 1, 1)).error, Store::Error::InvalidVersion);
    QCOMPARE(store.textureSnapshot(other.emptyVersion(), QRect(0, 0, 1, 1)).error, Store::Error::InvalidVersion);
    QCOMPARE(store.textureSnapshot(store.emptyVersion(), {}).error, Store::Error::InvalidCommand);
    QCOMPARE(store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 65, 64)).error, Store::Error::BudgetExceeded);
    QCOMPARE(store.textureSnapshot(store.emptyVersion(), QRect(0, 0, std::numeric_limits<int>::max(), 1)).error,
             Store::Error::BudgetExceeded);
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    QCOMPARE(store.statistics().submissions, quint64(0));
    auto pending = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QCOMPARE(pending.error, Store::Error::None);
    QCOMPARE(store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1)).error, Store::Error::QueueFull);
    QCOMPARE(store.statistics().residentBytes, quint64(4));
    QCOMPARE(store.statistics().submissions, quint64(1));
    QVERIFY(finish(store, pending.completion));
    auto retry = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QCOMPARE(retry.error, Store::Error::None);
}

void KisGpuTileTextureTest::deviceLossRejectsSnapshots()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, Store::TileBytes);
    auto snapshot = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QCOMPARE(snapshot.error, Store::Error::None);
    store.invalidateDevice();
    QCOMPARE(snapshot.completion.status(), Store::Status::Failed);
    QCOMPARE(store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1)).error, Store::Error::DeviceLost);
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    QCOMPARE(snapshot.completion.status(), Store::Status::Failed);
    snapshot = {};
    QCOMPARE(store.statistics().residentBytes, quint64(0));
}

QTEST_GUILESS_MAIN(KisGpuTileTextureTest)
#include "KisGpuTileTextureTest.moc"
