/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include "KisGpuTestDevice.h"
#include "KisGpuEditSession.h"

namespace {
using Session = KisGpuEditSession;
using Store = KisGpuTileStore;
const QRect clip(-64, -64, 128, 128);
Store::DabCommand dab(QPointF center, quint32 color)
{
    Store::DabCommand result;
    result.center = center;
    result.diameter = QSizeF(21, 17);
    result.fade = QSizeF(.5, .75);
    result.rgba = color;
    return result;
}
void settle(Session &session)
{
    QElapsedTimer timer;
    timer.start();
    while ((session.state() == Session::State::Committing || session.state() == Session::State::Restoring)
           && timer.elapsed() < 5000) {
        session.poll();
        QTest::qWait(1);
    }
    session.poll();
}
}

/** Selection-aware previews and their CPU command journal survive device replacement. */
class KisGpuSelectionSessionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() { m_gpu = std::make_unique<KisGpuTestDevice>(); }
    void cleanup() {
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        QCOMPARE(m_gpu->owner.errorCount(), 0);
    }
    void selectionIsFixedForAcceptedInputAndCommitAdoptsPreview();
    void replayRestoresSelectionsHistoryAndPendingCommit();
    void replayRestoresReplacementWithoutItsOldSelection();
    void rejectedSelectionsPreserveAcceptedWork();
    void recoveryCountsSharedSelectionOnceAndRejectsBeforeGpu();
    void replayReleasesSelectionResourcesWhilePruningHistory();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuSelectionSessionTest::selectionIsFixedForAcceptedInputAndCommitAdoptsPreview()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 4);
    auto selection = session.createSelection({dab(QPointF(0, 0), 0x80000000)}, clip);
    QCOMPARE(selection.result, Session::Result::Accepted);
    const auto token = session.begin();
    const QVector<Store::DabCommand> commands{dab(QPointF(0, 0), 0xFF0000FF), dab(QPointF(0, 0), 0xFFFF0000)};
    QCOMPARE(session.append(token, commands, clip, selection.selection), Session::Result::Accepted);
    selection = session.createSelection({dab(QPointF(40, 40), 0xFF000000)}, clip);
    QCOMPARE(selection.result, Session::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    const auto preview = session.preview();
    const auto pixels = m_gpu->read(preview, clip);
    const auto center = (64 * 128 + 64) * 4;
    QCOMPARE(quint8(pixels[center]), quint8(85));
    QCOMPARE(quint8(pixels[center + 2]), quint8(170));
    QCOMPARE(quint8(pixels[center + 3]), quint8(192));
    const auto before = store.statistics();
    QCOMPARE(session.commit(token), Session::Result::Accepted);
    settle(session);
    QVERIFY(session.head() == preview);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QVERIFY(session.undo());
    QCOMPARE(session.head().tileCount(), qsizetype(0));
    QVERIFY(session.redo());
    QCOMPARE(m_gpu->read(session.head(), clip), pixels);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuSelectionSessionTest::replayRestoresSelectionsHistoryAndPendingCommit()
{
    Session::Recovery saved;
    QByteArray committed, pending;
    {
        KisGpuTestDevice oldGpu;
        Store store(oldGpu.owner, 64 * Store::TileBytes);
        Session session(store, 2);
        auto selection = session.createSelection({dab(QPointF(0, 0), 0x80000000)}, clip).selection;
        auto token = session.begin();
        QCOMPARE(session.append(token, {dab(QPointF(0, 0), 0xFF0000FF)}, clip, selection), Session::Result::Accepted);
        session.commit(token);
        settle(session);
        committed = oldGpu.read(session.head(), clip);
        selection = session.createSelection({dab(QPointF(10, 0), 0xC0000000)}, clip).selection;
        token = session.begin();
        QCOMPARE(session.append(token, {dab(QPointF(10, 0), 0xFFFF0000)}, clip, selection), Session::Result::Accepted);
        wgpuDevicePoll(oldGpu.device, true, nullptr);
        session.poll();
        pending = oldGpu.read(session.preview(), clip);
        session.commit(token);
        oldGpu.owner.destroy();
        session.poll();
        QCOMPARE(session.state(), Session::State::Failed);
        saved = *session.recovery();
    }
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session restored(store, 2, saved);
    settle(restored);
    QCOMPARE(restored.state(), Session::State::Idle);
    QCOMPARE(m_gpu->read(restored.head(), clip), pending);
    QVERIFY(restored.undo());
    QCOMPARE(m_gpu->read(restored.head(), clip), committed);
    QVERIFY(restored.undo());
    QCOMPARE(restored.head().tileCount(), qsizetype(0));
    QVERIFY(restored.redo());
    QVERIFY(restored.redo());
    QCOMPARE(m_gpu->read(restored.head(), clip), pending);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuSelectionSessionTest::replayRestoresReplacementWithoutItsOldSelection()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 2);
    auto first = session.createSelection({dab(QPointF(-20, 0), 0xFF000000)}, clip).selection;
    auto second = session.createSelection({dab(QPointF(20, 0), 0x80000000)}, clip).selection;
    const auto token = session.begin();
    session.append(token, {dab(QPointF(-20, 0), 0xFF0000FF)}, clip, first);
    QCOMPARE(session.replace(token, {dab(QPointF(20, 0), 0xFFFF0000)}, clip, second), Session::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    const auto pixels = m_gpu->read(session.preview(), clip);
    auto saved = *session.recovery();
    QCOMPARE(saved.working->size(), qsizetype(1));
    Store destination(m_gpu->owner, 64 * Store::TileBytes);
    Session restored(destination, 2, saved);
    settle(restored);
    QCOMPARE(restored.state(), Session::State::Editing);
    QCOMPARE(m_gpu->read(restored.preview(), clip), pixels);
    restored.commit(restored.currentToken());
    settle(restored);
    QCOMPARE(m_gpu->read(restored.head(), clip), pixels);
}

void KisGpuSelectionSessionTest::rejectedSelectionsPreserveAcceptedWork()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 2);
    auto selection = session.createSelection({dab(QPointF(0, 0), 0x80000000)}, clip).selection;
    const auto token = session.begin();
    QCOMPARE(session.append(token, {dab(QPointF(0, 0), 0xFF0000FF)}, clip, selection), Session::Result::Accepted);
    Store foreign(m_gpu->owner, 64 * Store::TileBytes);
    Session other(foreign, 2);
    const auto wrong = other.createSelection({dab(QPointF(0, 0), 0xFF000000)}, clip).selection;
    const auto before = store.statistics().submissions;
    QCOMPARE(session.replace(token, {dab(QPointF(0, 0), 0xFFFF0000)}, clip, wrong), Session::Result::GpuRejected);
    QCOMPARE(session.lastGpuError(), Store::Error::InvalidVersion);
    QCOMPARE(session.replace(token, {}, clip, Session::Selection{}), Session::Result::GpuRejected);
    QCOMPARE(store.statistics().submissions, before);
    QCOMPARE(session.recovery()->working->size(), qsizetype(1));
    session.commit(token);
    settle(session);
    QCOMPARE(quint8(m_gpu->read(session.head(), clip)[(64 * 128 + 64) * 4 + 3]), quint8(128));
}

void KisGpuSelectionSessionTest::recoveryCountsSharedSelectionOnceAndRejectsBeforeGpu()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 2);
    QVector<Store::DabCommand> mask(50, dab(QPointF(0, 0), 0x01000000));
    const auto selection = session.createSelection(mask, clip).selection;
    const auto token = session.begin();
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(session.append(token, {dab(QPointF(0, 0), 0x800000FF)}, clip, selection), Session::Result::Accepted);
    }
    session.commit(token);
    settle(session);
    auto saved = *session.recovery();
    const quint64 budget = sizeof(Session::ReplaySelection) + 50 * sizeof(Store::DabCommand)
        + sizeof(Session::ReplayEdit) + 4 * (sizeof(Session::ReplayBatch) + sizeof(Store::DabCommand));
    Store destination(m_gpu->owner, 64 * Store::TileBytes);
    Session restored(destination, 2, saved, budget);
    settle(restored);
    QCOMPARE(restored.state(), Session::State::Idle);
    QCOMPARE(m_gpu->read(restored.head(), clip), m_gpu->read(session.head(), clip));
    const auto newSelection = restored.createSelection(mask, clip).selection;
    const auto newToken = restored.begin();
    const auto before = destination.statistics().submissions;
    QCOMPARE(restored.append(newToken, {dab(QPointF(0, 0), 0xFF0000FF)}, clip, newSelection), Session::Result::RecoveryBudgetExceeded);
    QCOMPARE(destination.statistics().submissions, before);
    QVERIFY(restored.recovery()->working->isEmpty());
    Session tiny(destination, 2, Session::Recovery{}, 1);
    settle(tiny);
    QCOMPARE(tiny.createSelection(mask, clip).result, Session::Result::RecoveryBudgetExceeded);
    QCOMPARE(destination.statistics().submissions, before);
}

void KisGpuSelectionSessionTest::replayReleasesSelectionResourcesWhilePruningHistory()
{
    Session::Recovery saved;
    QByteArray expected;
    const QRect bounds(0, 0, 64, 64);
    {
        Store store(m_gpu->owner, 8 * Store::TileBytes);
        Session session(store, 1);
        const auto selection = session.createSelection({dab(QPointF(16, 16), 0x80000000)}, bounds).selection;
        for (int i = 0; i < 20; ++i) {
            const auto token = session.begin();
            QCOMPARE(session.append(token, {dab(QPointF(16 + i % 5, 16), 0x800000FF)}, bounds, selection),
                     Session::Result::Accepted);
            session.commit(token);
            settle(session);
            QCOMPARE(session.state(), Session::State::Idle);
        }
        saved = *session.recovery();
        QCOMPARE(saved.firstRetained, qsizetype(19));
        expected = m_gpu->read(session.head(), bounds);
    }
    Store store(m_gpu->owner, 8 * Store::TileBytes);
    Session restored(store, 1, saved);
    QElapsedTimer timer;
    timer.start();
    while (restored.state() == Session::State::Restoring && timer.elapsed() < 5000) {
        const auto before = store.statistics().submissions;
        restored.poll();
        QVERIFY(store.statistics().submissions - before <= 1);
        QTest::qWait(1);
    }
    QCOMPARE(restored.state(), Session::State::Idle);
    QCOMPARE(m_gpu->read(restored.head(), bounds), expected);
    QCOMPARE(restored.historySize(), qsizetype(2));
    QCOMPARE(store.statistics().residentBytes, 2 * Store::TileBytes);
    QVERIFY(restored.undo());
    QVERIFY(!restored.undo());
    QVERIFY(restored.redo());
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

QTEST_GUILESS_MAIN(KisGpuSelectionSessionTest)
#include "KisGpuSelectionSessionTest.moc"
