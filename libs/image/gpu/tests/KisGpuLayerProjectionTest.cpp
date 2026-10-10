/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerProjection.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
using Store = KisGpuTileStore;
using Projection = KisGpuLayerProjection;
const QRect Canvas(-128, -64, 320, 192);
bool finish(Projection &projection)
{
    QElapsedTimer timer;
    timer.start();
    while (projection.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        projection.poll();
        QTest::qWait(1);
    }
    projection.poll();
    return projection.status() == Store::Status::Succeeded;
}
}

/** Image consumers observe matching layer metadata and pixels, including pending replacements. */
class KisGpuLayerProjectionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void layerChangesPublishTogether();
    void unchangedTilesRemainShared();
    void layerOrderMasksOpacityAndRemovalMatchProjection();
    void latestAcceptedLayersSurviveQueueRefusal();
    void hiddenAndOutsideLayersAvoidGpuWork();
    void invalidInputsAndBudgetRefusalPreservePublishedState();
    void deviceLossKeepsThePublishedSnapshot();
    void canvasEdgesUseWideCoordinateArithmetic_data();
    void canvasEdgesUseWideCoordinateArithmetic();
    void snapshotOutlivesOwners();
};

void KisGpuLayerProjectionTest::layerChangesPublishTogether()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    Projection projection(store, Canvas);
    const auto red = store.fill(store.emptyVersion(), {-70, -2, 20, 8}, 0xFF0000FF);
    const auto blue = store.fill(store.emptyVersion(), {60, -2, 20, 8}, 0x80FF0000);
    const QVector<Store::Layer> layers{{red.version}, {blue.version}};
    const auto before = store.statistics();
    QCOMPARE(projection.setLayers(layers), Store::Error::None);
    QCOMPARE(projection.status(), Store::Status::Pending);
    QVERIFY(projection.snapshot().layers.isEmpty());
    QCOMPARE(projection.snapshot().pixels.tileCount(), qsizetype(0));
    QVERIFY(finish(projection));
    const auto published = projection.snapshot();
    QCOMPARE(published.layers.size(), qsizetype(2));
    QVERIFY(published.layers[0].pixels == red.version);
    QVERIFY(published.layers[1].pixels == blue.version);
    QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
    const auto expected = store.project(store.emptyVersion(), layers, Canvas);
    QCOMPARE(gpu.read(published.pixels, Canvas), gpu.read(expected.version, Canvas));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::unchangedTilesRemainShared()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    Projection projection(store, Canvas);
    const auto source = store.paint(store.emptyVersion(), {
        {{-120, 0, 1, 1}, 0xFF0000FF}, {{128, 64, 1, 1}, 0xFFFF0000}});
    QCOMPARE(projection.setLayers({{source.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    const auto previous = projection.snapshot();
    const auto before = store.statistics();
    QCOMPARE(projection.setLayers(previous.layers), Store::Error::None);
    QVERIFY(finish(projection));
    QVERIFY(projection.snapshot().pixels == previous.pixels);
    QCOMPARE(store.statistics().submissions, before.submissions);
    const auto edited = store.fill(source.version, {-120, 0, 1, 1}, 0xFF00FF00);
    QCOMPARE(projection.setLayers({{edited.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    const auto current = projection.snapshot();
    const auto oldTile = previous.pixels.tile({2, 1}), newTile = current.pixels.tile({2, 1});
    QVERIFY(oldTile.buffer);
    QCOMPARE(newTile.buffer, oldTile.buffer);
    QCOMPARE(newTile.offset, oldTile.offset);
    QCOMPARE(gpu.read(previous.pixels, {-120, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.read(current.pixels, {-120, 0, 1, 1}), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::layerOrderMasksOpacityAndRemovalMatchProjection()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    Projection projection(store, Canvas);
    const auto red = store.fill(store.emptyVersion(), {-70, -2, 40, 8}, 0xFF0000FF);
    const auto blue = store.fill(store.emptyVersion(), {-65, -2, 40, 8}, 0x80FF0000);
    const auto mask = store.fill(store.emptyVersion(), {-67, -2, 7, 8}, 0x77123456);
    QVector<Store::Layer> layers{{red.version}, {blue.version}};
    const auto check = [&] {
        QCOMPARE(projection.setLayers(layers), Store::Error::None);
        QVERIFY(finish(projection));
        const auto expected = store.project(store.emptyVersion(), layers, Canvas);
        QCOMPARE(gpu.read(projection.snapshot().pixels, Canvas), gpu.read(expected.version, Canvas));
    };
    check();
    std::swap(layers[0], layers[1]);
    check();
    layers[1].opacity = 113;
    check();
    layers[1].mask = mask.version;
    check();
    layers[1].operation = Store::CompositeOp::Erase;
    check();
    layers[0].opacity = 0;
    check();
    layers.removeLast();
    check();
    layers.clear();
    check();
    QCOMPARE(gpu.read(projection.snapshot().pixels, Canvas), QByteArray(Canvas.width() * Canvas.height() * 4, '\0'));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::latestAcceptedLayersSurviveQueueRefusal()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes, 1);
    const auto red = store.fill(store.emptyVersion(), {-1, -1, 2, 2}, 0xFF0000FF);
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    const auto blue = store.fill(store.emptyVersion(), {63, 0, 2, 2}, 0xFFFF0000);
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    Projection projection(store, Canvas);
    Projection anotherDocument(store, Canvas);
    QCOMPARE(projection.setLayers({{red.version}}), Store::Error::None);
    const auto beforeRefusal = store.statistics();
    QCOMPARE(projection.setLayers({{blue.version}}), Store::Error::QueueFull);
    QCOMPARE(store.statistics().submissions, beforeRefusal.submissions);
    QCOMPARE(store.statistics().residentBytes, beforeRefusal.residentBytes);
    QVERIFY(projection.snapshot().layers.isEmpty());
    // Collect GPU completion without publishing the superseded layer list.
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    QCOMPARE(projection.setLayers({{blue.version}}), Store::Error::None);
    QVERIFY(projection.snapshot().layers.isEmpty());
    QVERIFY(finish(projection));
    QVERIFY(projection.snapshot().layers[0].pixels == blue.version);
    QCOMPARE(gpu.read(projection.snapshot().pixels, {-1, -1, 2, 2}), QByteArray(16, '\0'));
    QCOMPARE(gpu.read(projection.snapshot().pixels, {63, 0, 1, 1}), QByteArray::fromHex("0000ffff"));
    anotherDocument.poll();
    QVERIFY(anotherDocument.snapshot().layers.isEmpty());
    QCOMPARE(anotherDocument.snapshot().pixels.tileCount(), qsizetype(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::hiddenAndOutsideLayersAvoidGpuWork()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    const auto visible = store.fill(store.emptyVersion(), {-1, -1, 2, 2}, 0xFF0000FF);
    const auto outside = store.fill(store.emptyVersion(), {10000, -10000, 1, 1}, 0xFFFF0000);
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    Projection projection(store, Canvas);
    QVector<Store::Layer> layers{{visible.version, {}, 0}, {outside.version}};
    const auto before = store.statistics();
    QCOMPARE(projection.setLayers(layers), Store::Error::None);
    QVERIFY(finish(projection));
    QCOMPARE(projection.snapshot().layers.size(), qsizetype(2));
    QCOMPARE(projection.snapshot().pixels.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    layers[0].opacity = 255;
    QCOMPARE(projection.setLayers(layers), Store::Error::None);
    QVERIFY(finish(projection));
    QCOMPARE(projection.snapshot().pixels.tileCount(), qsizetype(4));
    QCOMPARE(gpu.read(projection.snapshot().pixels, {-1, -1, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::invalidInputsAndBudgetRefusalPreservePublishedState()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    Store foreign(gpu.owner, 128 * Store::TileBytes);
    Projection projection(store, Canvas);
    const auto red = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF0000FF);
    QCOMPARE(projection.setLayers({{red.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    const auto published = projection.snapshot();
    const auto blue = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFFFF0000);
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    const auto before = store.statistics();
    QCOMPARE(projection.setLayers({{{}}}), Store::Error::InvalidVersion);
    QCOMPARE(projection.setLayers({{foreign.emptyVersion(), {}, 0}}), Store::Error::InvalidVersion);
    QCOMPARE(projection.setLayers({{red.version, foreign.emptyVersion()}}), Store::Error::InvalidVersion);
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    QVERIFY(bool(occupied));
    QCOMPARE(projection.setLayers({{blue.version}}), Store::Error::BudgetExceeded);
    projection.poll();
    QVERIFY(projection.snapshot().pixels == published.pixels);
    QVERIFY(projection.snapshot().layers[0].pixels == red.version);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    occupied = {};
    QCOMPARE(projection.setLayers({{blue.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    QCOMPARE(gpu.read(projection.snapshot().pixels, {0, 0, 1, 1}), QByteArray::fromHex("0000ffff"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::deviceLossKeepsThePublishedSnapshot()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 128 * Store::TileBytes);
    Projection projection(store, Canvas);
    const auto red = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF0000FF);
    QCOMPARE(projection.setLayers({{red.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    const auto published = projection.snapshot();
    QCOMPARE(projection.setLayers({}), Store::Error::None);
    gpu.owner.destroy();
    projection.poll();
    QCOMPARE(projection.status(), Store::Status::Failed);
    QVERIFY(projection.snapshot().pixels == published.pixels);
    QVERIFY(projection.snapshot().layers[0].pixels == red.version);
    QCOMPARE(projection.setLayers({}), Store::Error::DeviceLost);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::canvasEdgesUseWideCoordinateArithmetic_data()
{
    QTest::addColumn<int>("x");
    QTest::newRow("minimum") << std::numeric_limits<int>::min();
    QTest::newRow("maximum") << std::numeric_limits<int>::max();
}

void KisGpuLayerProjectionTest::canvasEdgesUseWideCoordinateArithmetic()
{
    QFETCH(int, x);
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 16 * Store::TileBytes);
    const QRect bounds(x, -1, 1, 1);
    Projection projection(store, bounds);
    const int opposite = x == std::numeric_limits<int>::min() ? std::numeric_limits<int>::max() : std::numeric_limits<int>::min();
    auto source = store.fill(store.emptyVersion(), {opposite, -1, 1, 1}, 0xFFFF0000);
    source = store.fill(source.version, bounds, 0xFF0000FF);
    QCOMPARE(projection.setLayers({{source.version}}), Store::Error::None);
    QVERIFY(finish(projection));
    QCOMPARE(projection.snapshot().pixels.tileCount(), qsizetype(1));
    QCOMPARE(gpu.read(projection.snapshot().pixels, bounds), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(projection.setLayers({}), Store::Error::None);
    QVERIFY(finish(projection));
    QCOMPARE(gpu.read(projection.snapshot().pixels, bounds), QByteArray(4, '\0'));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Projection(store, {}));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerProjectionTest::snapshotOutlivesOwners()
{
    KisGpuTestDevice gpu;
    Projection::Snapshot retained;
    {
        Store store(gpu.owner, 16 * Store::TileBytes);
        Projection projection(store, Canvas);
        const auto source = store.fill(store.emptyVersion(), {-1, -1, 1, 1}, 0xFF0000FF);
        QCOMPARE(projection.setLayers({{source.version}}), Store::Error::None);
        QVERIFY(finish(projection));
        retained = projection.snapshot();
        QCOMPARE(projection.setLayers({}), Store::Error::None);
    }
    QCOMPARE(gpu.read(retained.pixels, {-1, -1, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.read(retained.layers[0].pixels, {-1, -1, 1, 1}), QByteArray::fromHex("ff0000ff"));
    retained = {};
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuLayerProjectionTest)
#include "KisGpuLayerProjectionTest.moc"
