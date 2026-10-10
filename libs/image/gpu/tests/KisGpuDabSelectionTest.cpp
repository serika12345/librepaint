/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDevice.h"
#include "KisGpuTileStore.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
namespace {
using Store = KisGpuTileStore;
bool finish(Store &store, const Store::Completion &completion) {
    QElapsedTimer timer;
    timer.start();
    while (completion.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        store.poll();
        QTest::qWait(1);
    }
    return completion.status() == Store::Status::Succeeded;
}
QVector<Store::DabCommand> colors() {
    Store::DabCommand red, blue;
    red.center = blue.center = QPointF(0, 0);
    red.diameter = blue.diameter = QSizeF(1024, 1024);
    red.rgba = 0xFF0000FF;
    blue.rgba = 0xFFFF0000;
    return {red, blue};
}
}
/** Brush consumers require selection at each dab, immutable input and bounded work. */
class KisGpuDabSelectionTest : public QObject
{
    Q_OBJECT
    std::unique_ptr<KisGpuTestDevice> gpu;
private Q_SLOTS:
    void initTestCase() { gpu = std::make_unique<KisGpuTestDevice>(); }
    void cleanup() {
        wgpuDevicePoll(gpu->device, true, nullptr);
        QCOMPARE(gpu->owner.errorCount(), 0);
    }
    void selectionIsAppliedToEveryOverlappingDab() {
        Store store(gpu->owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        const auto base = store.fill(store.emptyVersion(), pixel, 0xFF000000);
        const auto mask = store.fill(store.emptyVersion(), pixel, 0x80112233);
        const auto selected = store.paintDabs(base.version, colors(), pixel, mask.version);
        QCOMPARE(selected.error, Store::Error::None);
        QVERIFY(finish(store, selected.completion));
        QCOMPARE(gpu->read(selected.version, pixel), QByteArray::fromHex("400080ff"));
        const auto whole = store.paintDabs(base.version, colors(), pixel);
        const auto deferred = store.compositeMasked(base.version, whole.version, mask.version, pixel);
        QVERIFY(finish(store, deferred.completion));
        QCOMPARE(gpu->read(deferred.version, pixel), QByteArray::fromHex("000080ff"));
        QCOMPARE(gpu->read(base.version, pixel), QByteArray::fromHex("000000ff"));
    }
    void coverageCrossesNegativeAndPositiveTileBoundaries() {
        Store store(gpu->owner, 4 * 1024 * 1024);
        const QRect bounds(-65, 0, 194, 1);
        const int coordinates[] = {-65, -64, -1, 0, 63, 64, 128};
        const quint8 coverage[] = {0, 1, 64, 127, 128, 254, 255};
        const quint8 remainingRed[] = {0, 1, 48, 64, 64, 1, 0};
        auto mask = store.emptyVersion();
        for (int i = 0; i < 7; ++i) {
            mask = store.fill(mask, {coordinates[i], 0, 1, 1}, (quint32(coverage[i]) << 24) | 0x123456).version;
        }
        const auto base = store.fill(store.emptyVersion(), bounds, 0xFF000000);
        const auto before = store.statistics();
        const auto result = store.paintDabs(base.version, colors(), bounds, mask);
        QVERIFY(finish(store, result.completion));
        const auto pixels = gpu->read(result.version, bounds);
        for (int x = bounds.x(); x < bounds.x() + bounds.width(); ++x) {
            QByteArray expected = QByteArray::fromHex("000000ff");
            for (int i = 0; i < 7; ++i) if (x == coordinates[i]) {
                expected[0] = char(remainingRed[i]);
                expected[2] = char(coverage[i]);
            }
            QCOMPARE(pixels.mid((x - bounds.x()) * 4, 4), expected);
        }
        QCOMPARE(store.statistics().submissions, before.submissions + 1);
        QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes + 5 * Store::TileBytes);
        QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
        QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    }
    void erasingAndUniformCoverageUseTheSameSelection() {
        Store store(gpu->owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        const auto mask = store.fill(store.emptyVersion(), pixel, 0x80000000);
        auto erase = colors().first();
        erase.operation = Store::CompositeOp::Erase;
        const auto base = store.fill(store.emptyVersion(), pixel, 0x80224466);
        const auto erased = store.paintDabs(base.version, {erase, erase}, pixel, mask.version);
        QVERIFY(finish(store, erased.completion));
        QCOMPARE(gpu->read(erased.version, pixel), QByteArray::fromHex("66442220"));
        auto paint = colors().first();
        paint.coverage = 128;
        erase.coverage = 128;
        erase.opacity = 128;
        const auto result = store.paintDabs(store.emptyVersion(), {paint, erase}, pixel, mask.version);
        QVERIFY(finish(store, result.completion));
        QCOMPARE(gpu->read(result.version, pixel), QByteArray::fromHex("ff000038"));
    }
    void sparseSelectionBoundsTheWorkOfAWideDab() {
        Store store(gpu->owner, 4 * Store::TileBytes);
        const auto mask = store.fill(store.emptyVersion(), {-32, 9, 1, 1}, 0xFF000000);
        auto dab = colors().first();
        dab.diameter = QSizeF(1000000, 1000000);
        const auto result = store.paintDabs(store.emptyVersion(), {dab}, {-1000000, -1000000, 2000000, 2000000}, mask.version);
        QCOMPARE(result.error, Store::Error::None);
        QVERIFY(finish(store, result.completion));
        QCOMPARE(result.version.tileCount(), qsizetype(1));
        QCOMPARE(gpu->read(result.version, {-32, 9, 2, 1}), QByteArray::fromHex("ff0000ff00000000"));
    }
    void pendingSelectionIsPinnedUntilCollection() {
        Store store(gpu->owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        auto mask = store.fill(store.emptyVersion(), pixel, 0x80000000);
        QCOMPARE(mask.completion.status(), Store::Status::Pending);
        const auto result = store.paintDabs(store.emptyVersion(), {colors().first()}, pixel, mask.version);
        mask = {};
        QVERIFY(store.statistics().residentBytes > 2 * Store::TileBytes);
        QVERIFY(finish(store, result.completion));
        QCOMPARE(store.statistics().residentBytes, Store::TileBytes);
        QCOMPARE(gpu->read(result.version, pixel), QByteArray::fromHex("ff000080"));
    }
    void laterSelectionChangesKeepEarlierBatchesIndependent() {
        Store store(gpu->owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        auto mask = store.fill(store.emptyVersion(), pixel, 0x80000000);
        const auto earlier = store.paintDabs(store.emptyVersion(), {colors().first()}, pixel, mask.version);
        auto changed = store.fill(mask.version, pixel, 0xFF000000);
        const auto later = store.paintDabs(store.emptyVersion(), {colors().last()}, pixel, changed.version);
        mask = {};
        changed = {};
        QVERIFY(finish(store, later.completion));
        QCOMPARE(earlier.completion.status(), Store::Status::Succeeded);
        QCOMPARE(gpu->read(earlier.version, pixel), QByteArray::fromHex("ff000080"));
        QCOMPARE(gpu->read(later.version, pixel), QByteArray::fromHex("0000ffff"));
    }
    void deviceLossFailsAnAcceptedSelectedBatch() {
        KisGpuTestDevice local;
        Store store(local.owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        const auto mask = store.fill(store.emptyVersion(), pixel, 0x80000000);
        const auto result = store.paintDabs(store.emptyVersion(), colors(), pixel, mask.version);
        QCOMPARE(result.error, Store::Error::None);
        QCOMPARE(result.completion.status(), Store::Status::Pending);
        local.owner.destroy();
        store.poll();
        QCOMPARE(result.completion.status(), Store::Status::Failed);
        QCOMPARE(store.paintDabs(store.emptyVersion(), colors(), pixel, mask.version).error, Store::Error::DeviceLost);
    }
    void rejectionAndEmptySelectionPreserveTheQueue() {
        Store store(gpu->owner, 1024 * 1024, 1), foreign(gpu->owner, 1024 * 1024);
        const QRect pixel(0, 0, 1, 1);
        auto mask = store.fill(store.emptyVersion(), pixel, 0x80000000);
        const auto before = store.statistics();
        QCOMPARE(store.paintDabs(store.emptyVersion(), colors(), pixel, {}).error, Store::Error::InvalidVersion);
        QCOMPARE(store.paintDabs(store.emptyVersion(), colors(), pixel, foreign.emptyVersion()).error, Store::Error::InvalidVersion);
        QCOMPARE(store.paintDabs(store.emptyVersion(), colors(), pixel, mask.version).error, Store::Error::QueueFull);
        const auto unchanged = store.paintDabs(mask.version, colors(), pixel, store.emptyVersion());
        QCOMPARE(unchanged.error, Store::Error::None);
        QVERIFY(unchanged.version == mask.version);
        QCOMPARE(store.statistics().residentBytes, before.residentBytes);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QVERIFY(finish(store, mask.completion));
        Store small(gpu->owner, 2 * Store::TileBytes);
        const auto smallMask = small.fill(small.emptyVersion(), pixel, 0x80000000);
        QVERIFY(finish(small, smallMask.completion));
        const auto resident = small.statistics().residentBytes;
        QCOMPARE(small.paintDabs(small.emptyVersion(), {colors().first()}, pixel, smallMask.version).error, Store::Error::BudgetExceeded);
        QCOMPARE(small.statistics().residentBytes, resident);
        QCOMPARE(small.statistics().submissions, quint64(1));
    }
};
QTEST_GUILESS_MAIN(KisGpuDabSelectionTest)
#include "KisGpuDabSelectionTest.moc"
