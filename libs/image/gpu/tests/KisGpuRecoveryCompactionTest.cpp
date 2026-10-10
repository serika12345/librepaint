/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include "KisGpuEditSession.h"
#include "KisGpuTestDevice.h"
#include <limits>

namespace {
using Store = KisGpuTileStore;
using Session = KisGpuEditSession;
const QRect clip(-640064, 0, 1280128, 64);
Store::DabCommand dab(int index) {
    Store::DabCommand result;
    result.center = {index % 2 ? 640032.0 : -640032.0, 24.0 + index};
    result.diameter = {16, 16};
    result.rgba = 0x80443322 + index;
    return result;
}
bool settle(Session &session) {
    QElapsedTimer timer; timer.start();
    while ((session.state() == Session::State::Committing || session.state() == Session::State::Restoring)
           && timer.elapsed() < 5000) { session.poll(); QTest::qWait(1); }
    return session.state() == Session::State::Idle;
}
bool commit(Session &session, int index) {
    const auto token = session.begin();
    return session.append(token, {dab(index)}, clip) == Session::Result::Accepted
        && session.commit(token) == Session::Result::Accepted && settle(session);
}
bool compacted(Session &session) {
    QElapsedTimer timer; timer.start();
    while (session.recoveryCompactionStatus() == Store::Status::Pending && timer.elapsed() < 5000) {
        session.poll(); QTest::qWait(1);
    }
    return session.recoveryCompactionStatus() == Store::Status::Succeeded;
}
QByteArray pixels(KisGpuTestDevice &gpu, const Store::Version &version) {
    return gpu.read(version, {-640064, 0, 64, 64}) + gpu.read(version, {640000, 0, 64, 64});
}
}

/** Explicit maintenance folds discarded commands into sparse pixels without changing retained edits. */
class KisGpuRecoveryCompactionTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void sparseCheckpointPreservesWorkingEditUndoAndRedo() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session session(store, 2);
        for (int i = 0; i < 5; ++i) QVERIFY(commit(session, i));
        const auto redo = pixels(gpu, session.head());
        QVERIFY(session.undo());
        const auto head = pixels(gpu, session.head());
        const auto token = session.begin();
        QCOMPARE(session.append(token, {dab(6)}, clip), Session::Result::Accepted);
        const auto before = session.recovery();
        QCOMPARE(before->firstRetained, qsizetype(3));
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        QCOMPARE(session.recoveryCompactionStatus(), Store::Status::Pending);
        QCOMPARE(session.recovery()->edits.size(), before->edits.size());
        QCOMPARE(session.compactRecovery(), Session::Result::Busy);
        QCOMPARE(session.append(token, {dab(7)}, clip), Session::Result::Accepted);
        QVERIFY(compacted(session));
        QCOMPARE(session.state(), Session::State::Editing);
        QVERIFY(session.currentToken() == token);
        QCOMPARE(pixels(gpu, session.head()), head);
        QCOMPARE(session.recovery()->working->size(), qsizetype(2));
        QCOMPARE(session.cancel(token), Session::Result::Accepted);
        const auto saved = *session.recovery();
        QCOMPARE(saved.firstRetained, qsizetype(0));
        QCOMPARE(saved.cursor, qsizetype(1));
        QCOMPARE(saved.edits.size(), qsizetype(2));
        QCOMPARE(saved.tiles.size(), qsizetype(2));
        QVERIFY(saved.pixels.isEmpty());
        QCOMPARE(store.statistics().pixelReadbackBytes, 2 * Store::TileBytes);
        QVERIFY(session.redo());
        QCOMPARE(pixels(gpu, session.head()), redo);
        gpu.owner.destroy();
        KisGpuTestDevice replacement;
        Store restoredStore(replacement.owner, 64 * Store::TileBytes);
        Session restored(restoredStore, 2, saved);
        QVERIFY(settle(restored));
        QCOMPARE(pixels(replacement, restored.head()), head);
        QVERIFY(restored.redo());
        QCOMPARE(pixels(replacement, restored.head()), redo);
        QVERIFY(restored.undo());
        QVERIFY(restored.undo());
        QVERIFY(!restored.undo());
        QCOMPARE(restoredStore.statistics().pixelUploadBytes, 2 * Store::TileBytes);
        QCOMPARE(restoredStore.statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(replacement.owner.errorCount(), 0);
    }
    void concurrentBranchAndHistoryClearRemainRecoverable() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session session(store, 2);
        for (int i = 0; i < 4; ++i) QVERIFY(commit(session, i));
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        QVERIFY(session.undo());
        const auto branchBase = pixels(gpu, session.head());
        QVERIFY(session.clearHistory());
        QVERIFY(commit(session, 8));
        QVERIFY(compacted(session));
        const auto accepted = pixels(gpu, session.head());
        Session restored(store, 2, *session.recovery());
        QVERIFY(settle(restored));
        QCOMPARE(pixels(gpu, restored.head()), accepted);
        QVERIFY(restored.undo());
        QCOMPARE(pixels(gpu, restored.head()), branchBase);
        QVERIFY(!restored.undo());
        QVERIFY(restored.redo());
        QVERIFY(!restored.redo());
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void cpuAndGpuRefusalsPreserveRecoveryAndCanBeRetried() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session small(store, 1, Session::Recovery{}, 2048);
        QVERIFY(settle(small));
        QVERIFY(commit(small, 0));
        QVERIFY(commit(small, 1));
        auto before = store.statistics();
        QCOMPARE(small.compactRecovery(), Session::Result::RecoveryBudgetExceeded);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QCOMPARE(small.recovery()->edits.size(), qsizetype(2));
        Session session(store, 1);
        QVERIFY(commit(session, 0));
        QVERIFY(commit(session, 1));
        const auto saved = *session.recovery();
        const auto head = pixels(gpu, session.head());
        auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        session.poll();
        QCOMPARE(session.recoveryCompactionStatus(), Store::Status::Failed);
        QCOMPARE(session.lastGpuError(), Store::Error::BudgetExceeded);
        QCOMPARE(session.recovery()->edits.size(), saved.edits.size());
        QCOMPARE(session.recovery()->firstRetained, saved.firstRetained);
        QVERIFY(session.recovery()->tiles.isEmpty());
        occupied = {};
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        QVERIFY(compacted(session));
        QCOMPARE(pixels(gpu, session.head()), head);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void pendingCheckpointReservesCpuCapacityForNewInput() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session::Recovery initial;
        initial.tiles.push_back({{-10001, 0}, QByteArray(Store::TileBytes, '\0')});
        Session session(store, 2, initial, 2 * (Store::TileBytes + sizeof(Session::RecoveryTile)) + 1);
        QVERIFY(settle(session));
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        const auto token = session.begin();
        const auto before = store.statistics();
        QCOMPARE(session.append(token, {dab(0)}, clip), Session::Result::RecoveryBudgetExceeded);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QVERIFY(compacted(session));
        QCOMPARE(session.append(token, {dab(0)}, clip), Session::Result::Accepted);
        QCOMPARE(session.commit(token), Session::Result::Accepted);
        QVERIFY(settle(session));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void lossDuringPartialReadKeepsThePreviousRecovery() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session session(store, 1);
        for (int i = 0; i < 3; ++i) QVERIFY(commit(session, i));
        const auto accepted = pixels(gpu, session.head());
        const auto saved = *session.recovery();
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        QElapsedTimer timer; timer.start();
        while (store.statistics().pixelReadbackBytes < 2 * Store::TileBytes && timer.elapsed() < 5000) {
            session.poll(); QTest::qWait(1);
        }
        QCOMPARE(store.statistics().pixelReadbackBytes, 2 * Store::TileBytes);
        gpu.owner.destroy();
        session.poll();
        QCOMPARE(session.recoveryCompactionStatus(), Store::Status::Failed);
        QCOMPARE(session.recovery()->edits.size(), saved.edits.size());
        QCOMPARE(session.recovery()->firstRetained, saved.firstRetained);
        QVERIFY(session.recovery()->tiles.isEmpty());
        KisGpuTestDevice replacement;
        Store restoredStore(replacement.owner, 64 * Store::TileBytes);
        Session restored(restoredStore, 1, *session.recovery());
        QVERIFY(settle(restored));
        QCOMPARE(pixels(replacement, restored.head()), accepted);
        QVERIFY(restored.undo());
        QVERIFY(!restored.undo());
        QCOMPARE(replacement.owner.errorCount(), 0);
    }
    void sparseCheckpointKeepsCoordinateLimitsAndTransparentRgb() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session::Recovery initial;
        for (int coordinate : {std::numeric_limits<int>::min() / 64, -1, std::numeric_limits<int>::max() / 64}) {
            QByteArray data(Store::TileBytes, '\0');
            data.replace(0, 4, QByteArray::fromHex("11223300"));
            initial.tiles.push_back({{coordinate, coordinate}, data});
        }
        Session session(store, 1, initial);
        QVERIFY(settle(session));
        QCOMPARE(session.compactRecovery(), Session::Result::Accepted);
        QVERIFY(compacted(session));
        const auto saved = *session.recovery();
        QCOMPARE(saved.tiles.size(), initial.tiles.size());
        for (qsizetype i = 0; i < initial.tiles.size(); ++i) {
            QCOMPARE(saved.tiles[i].coordinate, initial.tiles[i].coordinate);
            QCOMPARE(saved.tiles[i].pixels, initial.tiles[i].pixels);
        }
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void invalidSparseCheckpointsAreRejectedBeforeSubmission() {
        KisGpuTestDevice gpu;
        Store store(gpu.owner, 64 * Store::TileBytes);
        Session::Recovery invalid;
        invalid.tiles.push_back({{-1, 0}, QByteArray(4, '\0')});
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Session(store, 2, invalid));
        invalid.tiles.front().pixels = QByteArray(Store::TileBytes, '\0');
        invalid.tiles.push_back(invalid.tiles.front());
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Session(store, 2, invalid));
        invalid.tiles.pop_back();
        invalid.tiles.front().coordinate.setX(std::numeric_limits<int>::max());
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Session(store, 2, invalid));
        invalid.tiles.front().coordinate = {-1, 0};
        invalid.bounds = {0, 0, 1, 1};
        invalid.pixels = QByteArray(4, '\0');
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Session(store, 2, invalid));
        QCOMPARE(store.statistics().submissions, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
};
QTEST_GUILESS_MAIN(KisGpuRecoveryCompactionTest)
#include "KisGpuRecoveryCompactionTest.moc"
