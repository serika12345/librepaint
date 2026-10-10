/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <set>
#include <stdexcept>
#include "KisGpuTestDevice.h"

namespace {
quint32 rgba(const QJsonArray &values)
{
    quint32 pixel = 0;
    for (int c = 0; c < 4; ++c) pixel |= quint32(values[c].toInt()) << (c * 8);
    return pixel;
}

QByteArray expected(QRect bounds, const QList<QPair<QRect, quint32>> &fills)
{
    QByteArray result(bounds.width() * bounds.height() * 4, '\0');
    for (const auto &fill : fills) {
        const QRect area = bounds.intersected(fill.first);
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                const int offset = ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4;
                for (int c = 0; c < 4; ++c) result[offset + c] = char(fill.second >> (c * 8));
            }
        }
    }
    return result;
}

bool finish(KisGpuTileStore &store, const KisGpuTileStore::Completion &completion)
{
    QElapsedTimer timer;
    timer.start();
    while (completion.status() == KisGpuTileStore::Status::Pending && timer.elapsed() < 5000) {
        store.poll();
        QTest::qWait(1);
    }
    store.poll();
    return completion.status() == KisGpuTileStore::Status::Succeeded;
}
}

/** Consumers are GPU edits, working versions and retained history versions. */
class KisGpuTileStoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void sparseSignedCoordinates();
    void copiesOnlyChangedTiles();
    void fullOverwriteAvoidsCopy();
    void orderedVersionsAndCancellation();
    void budgetRejectionIsAtomic();
    void pendingResourcesRemainBudgeted();
    void rejectsForeignVersionsAndAcceptsEmptyEdits();
    void emptyEditInheritsSourceCompletion();
    void versionOutlivesStore();
    void repeatedEditsReleaseOldVersions();
    void batchedPaintPreservesOrderAndCopiesTilesOnce();
    void batchFitsOneCopyBudget();
    void batchRejectionIsAtomic();
    void emptyBatchInheritsSourceCompletion();
    void groupedTilesRespectDeviceLimits();
    void sharedAllocationsStayBudgeted();
    void compositeVersionsPreservesLayerOrder();
    void imageCompositeRejectionAndEmptySource();
    void compositeRetainsPendingSource();
    void readbackPinsVersionAndReleasesStaging();
    void readbackRejectionIsAtomic();
    void readbackOutlivesStore();
    void compositing_data();
    void compositing();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuTileStoreTest::initTestCase()
{
    try {
        m_gpu = std::make_unique<KisGpuTestDevice>();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    qInfo() << "GPU:" << m_gpu->name;
}

void KisGpuTileStoreTest::cleanup()
{
    if (m_gpu) {
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        QCOMPARE(m_gpu->errors.load(), 0);
    }
}

void KisGpuTileStoreTest::cleanupTestCase() { m_gpu.reset(); }

void KisGpuTileStoreTest::readbackPinsVersionAndReleasesStaging()
{
    KisGpuTileStore store(m_gpu->device, 32 * KisGpuTileStore::TileBytes);
    const QRect bounds(-67, -3, 137, 9);
    auto original = store.fill(store.emptyVersion(), QRect(-65, -1, 131, 3), 0x80402010);
    const auto read = store.readback(original.version, bounds);
    QCOMPARE(read.error, KisGpuTileStore::Error::None);
    auto changed = store.fill(original.version, QRect(-65, -1, 131, 3), 0xFF998877);
    original = {};
    QVERIFY(finish(store, read.completion));
    QCOMPARE(read.bytes(), expected(bounds, {{QRect(-65, -1, 131, 3), 0x80402010}}));
    QVERIFY(finish(store, changed.completion));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(bounds.width() * bounds.height() * 4));
    const quint64 resident = store.statistics().residentBytes;
    changed = {};
    store.poll();
    QVERIFY(resident > 0);
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    // The CPU result remains usable without retaining GPU history or staging.
    QCOMPARE(read.bytes(), expected(bounds, {{QRect(-65, -1, 131, 3), 0x80402010}}));
}

void KisGpuTileStoreTest::readbackRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->device, 2 * KisGpuTileStore::TileBytes);
    auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto large = store.readback(base.version, QRect(0, 0, 128, 128));
    QCOMPARE(large.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(large.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(large.bytes().isEmpty());
    QCOMPARE(store.readback({}, QRect(0, 0, 1, 1)).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.readback(base.version, QRect(0, 0, INT_MAX, INT_MAX)).error,
             KisGpuTileStore::Error::BudgetExceeded);
    const auto empty = store.readback(base.version, QRect());
    QVERIFY(finish(store, empty.completion));
    QVERIFY(empty.bytes().isEmpty());
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuTileStoreTest::readbackOutlivesStore()
{
    KisGpuTileStore::Readback read;
    const QRect bounds(-1, -1, 3, 3);
    {
        KisGpuTileStore store(m_gpu->device, 8 * KisGpuTileStore::TileBytes);
        const auto pending = store.fill(store.emptyVersion(), bounds, 0xFFABCDEF);
        read = store.readback(pending.version, bounds);
        QCOMPARE(read.error, KisGpuTileStore::Error::None);
    }
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(read.bytes(), expected(bounds, {{bounds, 0xFFABCDEF}}));
}

void KisGpuTileStoreTest::sparseSignedCoordinates()
{
    KisGpuTileStore store(m_gpu->device, 8 * KisGpuTileStore::TileBytes);
    const auto empty = store.emptyVersion();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    const auto edit = store.fill(empty, QRect(-1, -1, 2, 2), 0x80402010);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QCOMPARE(edit.version.tileCount(), qsizetype(4));
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(edit.version, QRect(-65, -65, 130, 130)),
             expected(QRect(-65, -65, 130, 130), {{QRect(-1, -1, 2, 2), 0x80402010}}));
    QCOMPARE(empty.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, 4 * KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
}

void KisGpuTileStoreTest::copiesOnlyChangedTiles()
{
    KisGpuTileStore store(m_gpu->device, 3 * KisGpuTileStore::TileBytes + 1024);
    const QRect bounds(0, 0, 128, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, QRect(2, 3, 4, 5), 0x00406080);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(m_gpu->read(edit.version, bounds), expected(bounds, {{bounds, 0xFF102030}, {QRect(2, 3, 4, 5), 0x00406080}}));
    QCOMPARE(store.statistics().tileCopyBytes, KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::orderedVersionsAndCancellation()
{
    KisGpuTileStore store(m_gpu->device, 16 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, 0, 192, 64);
    const auto first = store.fill(store.emptyVersion(), QRect(-1, 2, 66, 4), 0xFF112233);
    const auto second = store.fill(first.version, QRect(63, 3, 3, 5), 0x80445566);
    QCOMPARE(second.error, KisGpuTileStore::Error::None);
    QVERIFY(second.completion.sequence() > first.completion.sequence());
    // Undo retains first; a cancelled working edit must also preserve second (Redo).
    {
        const auto working = store.fill(first.version, QRect(-62, 1, 2, 2), 0xFFABCDEF);
        QVERIFY(finish(store, working.completion));
    }
    QVERIFY(finish(store, first.completion));
    QVERIFY(finish(store, second.completion));
    QCOMPARE(m_gpu->read(first.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}}));
    QCOMPARE(m_gpu->read(second.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}, {QRect(63, 3, 3, 5), 0x80445566}}));
}

void KisGpuTileStoreTest::fullOverwriteAvoidsCopy()
{
    KisGpuTileStore store(m_gpu->device, 3 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, -64, 64, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, bounds, 0x00112233);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(m_gpu->read(edit.version, bounds), expected(bounds, {{bounds, 0x00112233}}));
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
    QCOMPARE(store.statistics().residentBytes, 2 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::budgetRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->device, 3 * KisGpuTileStore::TileBytes);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto rejected = store.fill(base.version, QRect(-64, 0, 192, 1), 0xFFABCDEF);
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(rejected.version.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(m_gpu->read(base.version, QRect(-1, -1, 3, 3)), expected(QRect(-1, -1, 3, 3), {{QRect(0, 0, 1, 1), 0xFF123456}}));
    const auto huge = store.fill(base.version, QRect(-1000000000, -1000000000, 2000000000, 2000000000), 0);
    QCOMPARE(huge.error, KisGpuTileStore::Error::BudgetExceeded);
}

void KisGpuTileStoreTest::pendingResourcesRemainBudgeted()
{
    KisGpuTileStore store(m_gpu->device, KisGpuTileStore::TileBytes + 256);
    KisGpuTileStore::Completion completion;
    {
        const auto edit = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
        completion = edit.completion;
    }
    const auto rejected = store.fill(store.emptyVersion(), QRect(64, 0, 1, 1), 0xFFABCDEF);
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QVERIFY(finish(store, completion));
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    const auto retried = store.fill(store.emptyVersion(), QRect(64, 0, 1, 1), 0xFFABCDEF);
    QCOMPARE(retried.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, retried.completion));
}

void KisGpuTileStoreTest::rejectsForeignVersionsAndAcceptsEmptyEdits()
{
    KisGpuTileStore store(m_gpu->device, 0);
    KisGpuTileStore other(m_gpu->device, 0);
    QCOMPARE(store.fill(other.emptyVersion(), QRect(0, 0, 1, 1), 0).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.fill({}, QRect(), 0).error, KisGpuTileStore::Error::InvalidVersion);
    const auto empty = store.fill(store.emptyVersion(), QRect(), 0);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(empty.completion.sequence(), quint64(0));
    QCOMPARE(store.statistics().submissions, quint64(0));
}

void KisGpuTileStoreTest::versionOutlivesStore()
{
    KisGpuTileStore::Version version;
    KisGpuTileStore::Completion completion;
    {
        KisGpuTileStore store(m_gpu->device, 2 * KisGpuTileStore::TileBytes);
        const auto edit = store.fill(store.emptyVersion(), QRect(5, 7, 2, 3), 0x00112233);
        version = edit.version;
        completion = edit.completion;
    }
    QCOMPARE(completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(m_gpu->read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(5, 7, 2, 3), 0x00112233}}));
}

void KisGpuTileStoreTest::emptyEditInheritsSourceCompletion()
{
    KisGpuTileStore store(m_gpu->device, 2 * KisGpuTileStore::TileBytes);
    const auto source = store.fill(store.emptyVersion(), QRect(1, 1, 2, 2), 0xFF123456);
    const auto empty = store.fill(source.version, QRect(), 0);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), source.completion.sequence());
    QVERIFY(finish(store, empty.completion));
    QCOMPARE(source.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(store.statistics().submissions, quint64(1));
    QCOMPARE(m_gpu->read(empty.version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 0xFF123456}}));
}

void KisGpuTileStoreTest::repeatedEditsReleaseOldVersions()
{
    KisGpuTileStore store(m_gpu->device, 2 * KisGpuTileStore::TileBytes + 256);
    auto version = store.emptyVersion();
    for (quint32 i = 1; i <= 64; ++i) {
        const auto edit = store.fill(version, QRect(1, 1, 2, 2), i);
        QCOMPARE(edit.error, KisGpuTileStore::Error::None);
        QVERIFY(finish(store, edit.completion));
        version = edit.version;
        QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    }
    QCOMPARE(m_gpu->read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 64}}));
    QCOMPARE(store.statistics().submissions, quint64(64));
}

void KisGpuTileStoreTest::batchedPaintPreservesOrderAndCopiesTilesOnce()
{
    KisGpuTileStore store(m_gpu->device, 32 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, -64, 128, 128);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    using Op = KisGpuTileStore::CompositeOp;
    const QVector<KisGpuTileStore::PaintCommand> commands {
        {QRect(-5, -5, 11, 11), 0x800000FF, Op::Over},
        {QRect(-2, -2, 5, 5), 0xC0FF0000, Op::Over, 192, 128},
        {QRect(0, -1, 3, 5), 0x80000000, Op::Erase, 128, 128},
        {QRect(-63, -63, 1, 1), 0xFFABCDEF, Op::Over},
        {QRect(58, 58, 5, 5), 0xFF00FF00, Op::Over}
    };
    auto sequential = base.version;
    for (const auto &command : commands) {
        const auto edit = store.paint(sequential, command.rectangle, command.rgba, command.operation,
                                      command.opacity, command.coverage);
        QVERIFY(finish(store, edit.completion));
        sequential = edit.version;
    }
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, commands);
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(m_gpu->read(batch.version, bounds), m_gpu->read(sequential, bounds));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(1));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, 4 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::batchFitsOneCopyBudget()
{
    // A stroke has room for the original tile, one changed tile and compact commands.
    KisGpuTileStore store(m_gpu->device, 2 * KisGpuTileStore::TileBytes + 8192);
    const QRect bounds(0, 0, 64, 64), area(2, 3, 4, 5);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    QVector<KisGpuTileStore::PaintCommand> commands;
    for (quint32 i = 0; i < 128; ++i) commands.push_back({area, 0xFF000000 | i});
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, commands);
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(store.statistics().residentBytes <= 2 * KisGpuTileStore::TileBytes + 8192);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(m_gpu->read(batch.version, bounds), expected(bounds, {{bounds, 0xFF102030}, {area, 0xFF00007F}}));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(store.statistics().residentBytes, 2 * KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, KisGpuTileStore::TileBytes);
    QVERIFY(store.statistics().commandUploadBytes - before.commandUploadBytes <= 8192);
}

void KisGpuTileStoreTest::batchRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->device, 3 * KisGpuTileStore::TileBytes + 256);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const QVector<QVector<KisGpuTileStore::PaintCommand>> rejectedCommands {
        {{QRect(0, 0, 1, 1), 0xFFABCDEF}, {QRect(64, 0, 128, 1), 0xFF112233}},
        {{QRect(0, 0, 1, 1), 0xFFABCDEF}, {QRect(-1000000000, -1000000000, 2000000000, 2000000000), 0xFF000000}},
        QVector<KisGpuTileStore::PaintCommand>(1024, {QRect(0, 0, 1, 1), 0xFFABCDEF})
    };
    for (const auto &commands : rejectedCommands) {
        const auto rejected = store.paint(base.version, commands);
        QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
        QCOMPARE(rejected.version.tileCount(), qsizetype(0));
        QCOMPARE(store.statistics().residentBytes, before.residentBytes);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
        QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes);
    }
    QCOMPARE(m_gpu->read(base.version, QRect(0, 0, 64, 64)),
             expected(QRect(0, 0, 64, 64), {{QRect(0, 0, 1, 1), 0xFF123456}}));
    QCOMPARE(store.paint({}, rejectedCommands[0]).error, KisGpuTileStore::Error::InvalidVersion);
    KisGpuTileStore other(m_gpu->device, 0);
    QCOMPARE(store.paint(other.emptyVersion(), rejectedCommands[0]).error, KisGpuTileStore::Error::InvalidVersion);
}

void KisGpuTileStoreTest::emptyBatchInheritsSourceCompletion()
{
    KisGpuTileStore store(m_gpu->device, KisGpuTileStore::TileBytes + 256);
    const auto source = store.fill(store.emptyVersion(), QRect(1, 1, 2, 2), 0xFF123456);
    const QRect huge(-1000000000, -1000000000, 2000000000, 2000000000);
    using Op = KisGpuTileStore::CompositeOp;
    const QVector<QVector<KisGpuTileStore::PaintCommand>> batches {
        {}, {{QRect(), 0xFF000000}, {huge, 0xFF000000, Op::Over, 0},
             {huge, 0xFF000000, Op::Erase, 255, 0}, {huge, 0x000000FF}}
    };
    for (const auto &commands : batches) {
        const auto empty = store.paint(source.version, commands);
        QCOMPARE(empty.error, KisGpuTileStore::Error::None);
        QCOMPARE(empty.completion.sequence(), source.completion.sequence());
        QVERIFY(finish(store, empty.completion));
        QCOMPARE(m_gpu->read(empty.version, QRect(0, 0, 64, 64)),
                 expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 0xFF123456}}));
    }
    QCOMPARE(store.statistics().submissions, quint64(1));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::groupedTilesRespectDeviceLimits()
{
    KisGpuTestDevice gpu(2 * KisGpuTileStore::TileBytes);
    KisGpuTileStore store(gpu.device, 16 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, 0, 320, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, {{bounds, 0xFFABCDEF}});
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(gpu.read(batch.version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
    QCOMPARE(gpu.read(base.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(3));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, 5 * KisGpuTileStore::TileBytes);
    const auto composed = store.composite(base.version, batch.version, bounds);
    QCOMPARE(composed.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, composed.completion));
    QCOMPARE(gpu.read(composed.version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(6));
    QCOMPARE(gpu.errors.load(), 0);
}

void KisGpuTileStoreTest::sharedAllocationsStayBudgeted()
{
    KisGpuTileStore store(m_gpu->device, 8 * KisGpuTileStore::TileBytes);
    const QRect bounds(0, 0, 192, 64);
    auto version = store.emptyVersion();
    {
        const auto base = store.fill(version, bounds, 0xFF123456);
        QVERIFY(finish(store, base.completion));
        version = base.version;
    }
    for (int x = 0; x < 3; ++x) {
        const auto edit = store.fill(version, QRect(x * 64, 0, 64, 64), 0xFFABCDEF);
        QVERIFY(finish(store, edit.completion));
        version = edit.version;
        // A shared GPU buffer remains fully budgeted while any live tile uses it.
        std::set<WGPUBuffer> buffers;
        for (int i = 0; i < 3; ++i) buffers.insert(version.tile(QPoint(i, 0)).buffer);
        quint64 allocated = 0;
        for (const auto buffer : buffers) allocated += wgpuBufferGetSize(buffer);
        QCOMPARE(store.statistics().residentBytes, allocated);
    }
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
    QCOMPARE(m_gpu->read(version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
}

void KisGpuTileStoreTest::compositeVersionsPreservesLayerOrder()
{
    KisGpuTileStore store(m_gpu->device, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds(-65, -65, 194, 194), redArea(-1, -1, 66, 66), patch(63, 63, 2, 2), greenArea(0, 0, 4, 4);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFFFF0000);
    const auto red = store.fill(store.emptyVersion(), redArea, 0xFF0000FF);
    const auto patched = store.fill(red.version, patch, 0xFF00FF00);
    const auto green = store.fill(store.emptyVersion(), greenArea, 0xFF00FF00);
    const auto first = store.composite(base.version, patched.version, bounds);
    const auto front = store.composite(first.version, green.version, bounds);
    const auto reverseFirst = store.composite(base.version, green.version, bounds);
    const auto reverse = store.composite(reverseFirst.version, patched.version, bounds);
    QVERIFY(finish(store, front.completion));
    QVERIFY(finish(store, reverse.completion));
    QCOMPARE(m_gpu->read(first.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {redArea, 0xFF0000FF}, {patch, 0xFF00FF00}}));
    QCOMPARE(m_gpu->read(front.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {redArea, 0xFF0000FF}, {patch, 0xFF00FF00}, {greenArea, 0xFF00FF00}}));
    QCOMPARE(m_gpu->read(reverse.version, bounds), m_gpu->read(first.version, bounds));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}}));
    QCOMPARE(m_gpu->read(red.version, bounds), expected(bounds, {{redArea, 0xFF0000FF}}));
    const QRect clip(0, 0, 2, 3);
    const auto clipped = store.composite(base.version, patched.version, clip);
    QVERIFY(finish(store, clipped.completion));
    QCOMPARE(m_gpu->read(clipped.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {clip, 0xFF0000FF}}));

    // Same half-transparent layer stack and fixed pixels as the CPU raster contract.
    const QRect area(-2, -2, 67, 67);
    const auto halfRed = store.fill(store.emptyVersion(), area, 0x800000FF);
    const auto halfGreen = store.fill(store.emptyVersion(), area, 0x8000FF00);
    const auto redFirst = store.composite(base.version, halfRed.version, area);
    const auto greenFront = store.composite(redFirst.version, halfGreen.version, area);
    const auto greenFirst = store.composite(base.version, halfGreen.version, area);
    const auto redFront = store.composite(greenFirst.version, halfRed.version, area);
    QVERIFY(finish(store, greenFront.completion));
    QVERIFY(finish(store, redFront.completion));
    QCOMPARE(m_gpu->read(greenFront.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F8040}}));
    QCOMPARE(m_gpu->read(redFront.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F4080}}));
}

void KisGpuTileStoreTest::imageCompositeRejectionAndEmptySource()
{
    KisGpuTileStore store(m_gpu->device, 4 * KisGpuTileStore::TileBytes + 256);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    const auto source = store.fill(store.emptyVersion(), QRect(0, 0, 128, 1), 0xFFABCDEF);
    QVERIFY(finish(store, source.completion));
    const auto before = store.statistics();
    const auto rejected = store.composite(base.version, source.version, QRect(0, 0, 128, 1));
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes);
    const QRect huge(-1000000000, -1000000000, 2000000000, 2000000000);
    const auto empty = store.composite(base.version, store.emptyVersion(), huge);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), base.completion.sequence());
    QCOMPARE(store.statistics().submissions, before.submissions);
    const auto invisible = store.composite(base.version, source.version, huge, KisGpuTileStore::CompositeOp::Over, 0);
    QCOMPARE(invisible.error, KisGpuTileStore::Error::None);
    QCOMPARE(invisible.completion.sequence(), base.completion.sequence());
    QCOMPARE(store.composite(base.version, {}, QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    KisGpuTileStore other(m_gpu->device, 0);
    QCOMPARE(store.composite(base.version, other.emptyVersion(), QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.composite(other.emptyVersion(), source.version, QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(m_gpu->read(base.version, QRect(0, 0, 64, 64)),
             expected(QRect(0, 0, 64, 64), {{QRect(0, 0, 1, 1), 0xFF123456}}));
}

void KisGpuTileStoreTest::compositeRetainsPendingSource()
{
    KisGpuTileStore store(m_gpu->device, 5 * KisGpuTileStore::TileBytes);
    KisGpuTileStore::Version version;
    KisGpuTileStore::Completion completion;
    const QRect area(-1, 1, 1, 1);
    {
        const auto source = store.fill(store.emptyVersion(), area, 0xFF123456);
        const auto edit = store.composite(store.emptyVersion(), source.version, area);
        QCOMPARE(edit.error, KisGpuTileStore::Error::None);
        version = edit.version;
        completion = edit.completion;
    }
    QVERIFY(finish(store, completion));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    QCOMPARE(m_gpu->read(version, QRect(-64, 0, 64, 64)), expected(QRect(-64, 0, 64, 64), {{area, 0xFF123456}}));
}

void KisGpuTileStoreTest::compositing_data()
{
    QFile file(QStringLiteral(RASTER_EDIT_FIXTURE));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    QJsonParseError error;
    const auto fixture = QJsonDocument::fromJson(file.readAll(), &error).object();
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(fixture["schema"].toInt(), 1);
    QCOMPARE(fixture["profile"].toString(), QStringLiteral("sRGB-elle-V2-srgbtrc.icc"));
    QTest::addColumn<QJsonObject>("input");
    QTest::addColumn<QRect>("rectangle");
    QTest::addColumn<bool>("imageSource");
    const QList<QRect> rectangles {QRect(-1, -1, 1, 1), QRect(-2, -2, 67, 67), QRect(62, 62, 131, 3)};
    const auto cases = fixture["cases"].toArray();
    QVERIFY(!cases.isEmpty());
    for (const auto &entry : cases) {
        const auto input = entry.toObject();
        for (int i = 0; i < rectangles.size(); ++i) {
            const QByteArray name = input["id"].toString().toUtf8() + '-' + QByteArray::number(i);
            const QByteArray solidName = name + "-solid", imageName = name + "-image";
            QTest::newRow(solidName.constData()) << input << rectangles[i] << false;
            QTest::newRow(imageName.constData()) << input << rectangles[i] << true;
        }
    }
}

void KisGpuTileStoreTest::compositing()
{
    QFETCH(QJsonObject, input);
    QFETCH(QRect, rectangle);
    QFETCH(bool, imageSource);
    KisGpuTileStore store(m_gpu->device, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds = rectangle.adjusted(-2, -2, 2, 2);
    const quint32 destination = rgba(input["dst"].toArray());
    const auto base = store.fill(store.emptyVersion(), bounds, destination);
    QRect paintedRect = rectangle;
    const int mask = input["mask"].toInt();
    if (mask >= 0 && rectangle.width() > 2 && rectangle.height() > 2) paintedRect.adjust(1, 1, -1, -1);
    const auto operation = input["op"].toString() == "erase"
        ? KisGpuTileStore::CompositeOp::Erase : KisGpuTileStore::CompositeOp::Over;
    KisGpuTileStore::Edit edit;
    if (imageSource) {
        const auto source = store.fill(store.emptyVersion(), paintedRect, rgba(input["src"].toArray()));
        edit = store.composite(base.version, source.version, paintedRect,
                               operation, quint8(input["opacity"].toInt()), quint8(mask < 0 ? 255 : mask));
    } else {
        edit = store.paint(base.version, paintedRect, rgba(input["src"].toArray()),
                           operation, quint8(input["opacity"].toInt()), quint8(mask < 0 ? 255 : mask));
    }
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    const QByteArray actual = m_gpu->read(edit.version, bounds);
    const QByteArray wanted = expected(bounds, {{bounds, destination}, {paintedRect, rgba(input["expected"].toArray())}});
    QCOMPARE(actual.size(), wanted.size());
    for (int offset = 0; offset < actual.size(); offset += 4) {
        QVERIFY2(actual.mid(offset, 4) == wanted.mid(offset, 4),
                 qPrintable(QStringLiteral("(%1,%2): expected RGBA %3, actual %4")
                     .arg(bounds.x() + offset / 4 % bounds.width()).arg(bounds.y() + offset / 4 / bounds.width())
                     .arg(QString::fromLatin1(wanted.mid(offset, 4).toHex()), QString::fromLatin1(actual.mid(offset, 4).toHex()))));
    }
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, destination}}));
}

QTEST_GUILESS_MAIN(KisGpuTileStoreTest)
#include "KisGpuTileStoreTest.moc"
