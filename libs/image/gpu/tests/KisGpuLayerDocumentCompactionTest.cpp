/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuTestDevice.h"
#include <QtTest>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
const QRect Clip(-640064, 0, 1280128, 64);
Store::DabCommand dab(int index)
{
    Store::DabCommand result;
    result.center = {index % 2 ? 640032.0 : -640032.0, 24};
    result.diameter = {12, 12}; result.rgba = 0x80654321;
    return result;
}
bool settle(Document &document)
{
    QElapsedTimer timer; timer.start();
    while ((document.state() == Document::State::Restoring || document.state() == Document::State::Committing)
           && timer.elapsed() < 5000) { document.poll(); QTest::qWait(1); }
    return document.state() == Document::State::Idle;
}
bool paint(Document &document, Document::LayerId id, int count, Document::Target target = Document::Target::Pixels)
{
    const auto editing = document.begin(id, target);
    if (editing.result != Document::Result::Accepted) return false;
    QVector<Store::DabCommand> commands;
    for (int i = 0; i < count; ++i) commands.push_back(dab(i));
    return editing.session->append(editing.token, commands, Clip) == Session::Result::Accepted
        && document.commit(editing.token) == Document::Result::Accepted && settle(document);
}
bool compacted(Document &document)
{
    QElapsedTimer timer; timer.start();
    while (document.recoveryCompactionStatus() == Store::Status::Pending && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    return document.recoveryCompactionStatus() == Store::Status::Succeeded;
}
Store::Version version(const Document &document, bool mask)
{
    const auto layer = document.layers()[0].paint;
    return mask ? layer.mask : layer.pixels;
}
Document::RasterRecovery raster(const Document::Recovery &saved, bool mask)
{
    const auto &layer = saved.history[saved.cursor][0];
    return mask ? layer->mask : layer->pixels;
}
QByteArray pixels(KisGpuTestDevice &gpu, const Store::Version &head)
{
    return gpu.read(head, {-640064, 0, 64, 64}) + gpu.read(head, {640000, 0, 64, 64});
}
}

/** Explicit sparse checkpoints bound replay work while retaining document history and views. */
class KisGpuLayerDocumentCompactionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void checkpointPreservesHistoryAndGpuIdentity_data();
    void checkpointPreservesHistoryAndGpuIdentity();
    void cpuRefusalPrecedesGpuWork();
    void gpuRefusalPreservesRecoveryAndCanRetry();
    void lossDuringPartialReadPreservesRecovery();
};

void KisGpuLayerDocumentCompactionTest::checkpointPreservesHistoryAndGpuIdentity_data()
{
    QTest::addColumn<bool>("mask");
    QTest::newRow("pixels") << false;
    QTest::newRow("mask") << true;
}

void KisGpuLayerDocumentCompactionTest::checkpointPreservesHistoryAndGpuIdentity()
{
    QFETCH(bool, mask);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes, 1);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 1024));
    QVERIFY(paint(document, id, 1024, Document::Target::Mask));
    QCOMPARE(document.renameLayer(id, QStringLiteral("Renamed")), Document::Result::Accepted);
    QCOMPARE(document.setLayerProperties(id, 117, Store::CompositeOp::Erase), Document::Result::Accepted);
    const auto before = document.recovery();
    const auto source = raster(before, mask);
    QVERIFY(!source->edits.isEmpty());
    const auto head = version(document, mask);
    const auto unaffected = version(document, !mask);
    const auto expected = pixels(gpu, head);
    const auto bytes = document.recoveryBytes();
    const auto statistics = store->statistics();
    const auto target = mask ? Document::Target::Mask : Document::Target::Pixels;
    QVERIFY(!document.recoveryCompactionStatus());
    QCOMPARE(document.compactRecovery(id, target), Document::Result::Accepted);
    QCOMPARE(document.state(), Document::State::Compacting);
    QCOMPARE(document.recoveryCompactionStatus(), Store::Status::Pending);
    QVERIFY(document.recoveryBytes() > bytes);
    QVERIFY(raster(document.recovery(), mask) == source);
    QVERIFY(version(document, mask) == head);
    QCOMPARE(document.begin(id).result, Document::Result::Busy);
    QCOMPARE(document.renameLayer(id, QStringLiteral("Busy")), Document::Result::Busy);
    QCOMPARE(document.evictHistory(), Document::Result::Busy);
    QCOMPARE(document.compactRecovery(id, target), Document::Result::Busy);
    QVERIFY(!document.clearHistory());
    QVERIFY(!document.undo());
    QVERIFY(!document.redo());
    quint64 readBytes = statistics.pixelReadbackBytes;
    QElapsedTimer timer; timer.start();
    while (document.recoveryCompactionStatus() == Store::Status::Pending && timer.elapsed() < 5000) {
        document.poll();
        const auto current = store->statistics().pixelReadbackBytes;
        QVERIFY(current - readBytes <= Store::TileBytes);
        readBytes = current;
        QTest::qWait(1);
    }
    QCOMPARE(document.recoveryCompactionStatus(), Store::Status::Succeeded);
    QCOMPARE(document.recoveryCompactionError(), Store::Error::None);
    QCOMPARE(document.state(), Document::State::Idle);
    QVERIFY(document.recoveryBytes() < bytes);
    QCOMPARE(readBytes - statistics.pixelReadbackBytes, 2 * Store::TileBytes);
    QCOMPARE(store->statistics().pixelUploadBytes, statistics.pixelUploadBytes);
    QVERIFY(version(document, mask) == head);
    QCOMPARE(pixels(gpu, head), expected);
    QVERIFY(version(document, !mask) == unaffected);
    const auto after = document.recovery();
    QCOMPARE(after.cursor, before.cursor);
    QCOMPARE(after.nextId, before.nextId);
    QCOMPARE(after.history.size(), before.history.size());
    const auto checkpoint = raster(after, mask);
    QVERIFY(checkpoint != source);
    QVERIFY(checkpoint->edits.isEmpty());
    QCOMPARE(checkpoint->cursor, qsizetype(0));
    QCOMPARE(checkpoint->firstRetained, qsizetype(0));
    QCOMPARE(checkpoint->tiles.size(), 2);
    QVERIFY(checkpoint->pixels.isEmpty());
    for (qsizetype i = 0; i < before.history.size(); ++i) {
        for (qsizetype j = 0; j < before.history[i].size(); ++j) {
            const auto &oldLayer = before.history[i][j];
            const auto &newLayer = after.history[i][j];
            QCOMPARE(newLayer->id, oldLayer->id);
            QCOMPARE(newLayer->name, oldLayer->name);
            QCOMPARE(newLayer->opacity, oldLayer->opacity);
            QCOMPARE(newLayer->operation, oldLayer->operation);
            QVERIFY((mask ? newLayer->pixels : newLayer->mask) == (mask ? oldLayer->pixels : oldLayer->mask));
            if ((mask ? oldLayer->mask : oldLayer->pixels) == source)
                QVERIFY((mask ? newLayer->mask : newLayer->pixels) == checkpoint);
        }
    }
    QVERIFY(!source->edits.isEmpty());
    const auto submitted = store->statistics().submissions;
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[0].paint.opacity, quint8(255));
    QVERIFY(document.redo());
    QCOMPARE(document.layers()[0].paint.opacity, quint8(117));
    QCOMPARE(store->statistics().submissions, submitted);
    QCOMPARE(document.compactRecovery(id, target), Document::Result::Accepted);
    QCOMPARE(document.recoveryCompactionStatus(), Store::Status::Succeeded);
    QCOMPARE(store->statistics().submissions, submitted);
    KisGpuTestDevice replacement;
    auto replacementStore = std::make_shared<Store>(replacement.owner, 64 * Store::TileBytes);
    Document restored(replacementStore, 8, after);
    QVERIFY(settle(restored));
    QCOMPARE(pixels(replacement, version(restored, mask)), expected);
    QVERIFY(paint(document, id, 1, target));
    QCOMPARE(raster(document.recovery(), mask)->edits.size(), 1);
    QCOMPARE(raster(document.recovery(), mask)->tiles.size(), 2);
    QCOMPARE(gpu.owner.errorCount(), 0);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

void KisGpuLayerDocumentCompactionTest::cpuRefusalPrecedesGpuWork()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2, 4096);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 2));
    const auto saved = document.recovery();
    const auto statistics = store->statistics();
    QCOMPARE(document.compactRecovery(id), Document::Result::RecoveryBudgetExceeded);
    QCOMPARE(document.state(), Document::State::Idle);
    QVERIFY(raster(document.recovery(), false) == raster(saved, false));
    QCOMPARE(store->statistics().submissions, statistics.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, statistics.pixelReadbackBytes);
    QVERIFY(!document.recoveryCompactionStatus());
    QCOMPARE(document.compactRecovery(id + 1), Document::Result::InvalidLayer);
    QCOMPARE(document.compactRecovery(id, Document::Target(99)), Document::Result::InvalidInput);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentCompactionTest::gpuRefusalPreservesRecoveryAndCanRetry()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 2));
    const auto saved = document.recovery();
    const auto bytes = document.recoveryBytes();
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    QCOMPARE(document.compactRecovery(id), Document::Result::Accepted);
    QVERIFY(!compacted(document));
    QCOMPARE(document.recoveryCompactionStatus(), Store::Status::Failed);
    QCOMPARE(document.recoveryCompactionError(), Store::Error::BudgetExceeded);
    QCOMPARE(document.state(), Document::State::Idle);
    QCOMPARE(document.recoveryBytes(), bytes);
    QVERIFY(raster(document.recovery(), false) == raster(saved, false));
    occupied = {};
    QCOMPARE(document.compactRecovery(id), Document::Result::Accepted);
    QVERIFY(compacted(document));
    QVERIFY(raster(document.recovery(), false)->edits.isEmpty());
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentCompactionTest::lossDuringPartialReadPreservesRecovery()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 2));
    const auto expected = pixels(gpu, version(document, false));
    const auto saved = document.recovery();
    const auto readBytes = store->statistics().pixelReadbackBytes;
    QCOMPARE(document.compactRecovery(id), Document::Result::Accepted);
    QElapsedTimer timer; timer.start();
    while (store->statistics().pixelReadbackBytes < readBytes + 2 * Store::TileBytes && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    QCOMPARE(store->statistics().pixelReadbackBytes, readBytes + 2 * Store::TileBytes);
    QCOMPARE(document.state(), Document::State::Compacting);
    gpu.owner.destroy();
    document.poll();
    QCOMPARE(document.recoveryCompactionStatus(), Store::Status::Failed);
    QCOMPARE(document.recoveryCompactionError(), Store::Error::DeviceLost);
    QVERIFY(raster(document.recovery(), false) == raster(saved, false));
    KisGpuTestDevice replacement;
    auto replacementStore = std::make_shared<Store>(replacement.owner, 64 * Store::TileBytes);
    Document restored(replacementStore, 2, document.recovery());
    QVERIFY(settle(restored));
    QCOMPARE(pixels(replacement, version(restored, false)), expected);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuLayerDocumentCompactionTest)
#include "KisGpuLayerDocumentCompactionTest.moc"
