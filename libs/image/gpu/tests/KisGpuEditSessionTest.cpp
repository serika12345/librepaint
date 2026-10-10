/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include "KisGpuTestDevice.h"
#include "KisGpuEditSession.h"
#include <stdexcept>

namespace {
KisGpuTileStore::DabCommand dab(QPointF center, quint32 color)
{
    KisGpuTileStore::DabCommand result;
    result.center = center;
    result.diameter = QSizeF(15, 11);
    result.fade = QSizeF(.5, .75);
    result.rgba = color;
    return result;
}
bool settle(KisGpuEditSession &session)
{
    QElapsedTimer timer;
    timer.start();
    while ((session.state() == KisGpuEditSession::State::Committing
            || session.state() == KisGpuEditSession::State::Restoring) && timer.elapsed() < 5000) {
        session.poll();
        QTest::qWait(1);
    }
    session.poll();
    return session.state() == KisGpuEditSession::State::Idle;
}
}

/** GPU raster editing consumes tentative previews, commit/cancel and immutable history. */
class KisGpuEditSessionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void replacementCommitsExactlyThePreview();
    void cancellationPreservesRedoAndRejectsStaleInput();
    void historyLimitReleasesGpuVersions();
    void rejectedUpdatesPreserveTheWorkingEdit();
    void inputFromAnotherDocumentIsRejected();
    void deviceLossNeverCommitsTentativePixels();
    void importedVersionStartsAnEditableDocument();
    void replayRestoresPrunedHistoryAndTentativeReplacement();
    void replayRestoresLoadedPixelsAndPendingCommit();
    void recoveryBudgetAndInvalidInputsPreserveAcceptedWork();
    void replayUsesBoundedGpuHistoryAndPreservesNewBranches();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuEditSessionTest::initTestCase() { m_gpu = std::make_unique<KisGpuTestDevice>(); }

void KisGpuEditSessionTest::replayRestoresPrunedHistoryAndTentativeReplacement()
{
    KisGpuTestDevice oldGpu;
    KisGpuTileStore oldStore(oldGpu.owner, 256 * KisGpuTileStore::TileBytes);
    KisGpuEditSession old(oldStore, 2);
    const QRect clip(-64, -64, 128, 128);
    for (int i = 0; i < 4; ++i) {
        auto token = old.begin();
        QCOMPARE(old.append(token, {dab(QPointF(i * 10, 0), 0x800000FF)}, clip), KisGpuEditSession::Result::Accepted);
        old.commit(token);
        QVERIFY(settle(old));
    }
    const auto redoPixels = oldGpu.read(old.head(), clip);
    QVERIFY(old.undo());
    const auto headPixels = oldGpu.read(old.head(), clip);
    const auto token = old.begin();
    old.append(token, {dab(QPointF(-30, 0), 0xFF00FF00)}, clip);
    old.replace(token, {dab(QPointF(0, 20), 0xFF00FF00)}, clip);
    old.append(token, {dab(QPointF(20, 20), 0xFF00FF00)}, clip);
    wgpuDevicePoll(oldGpu.device, true, nullptr);
    old.poll();
    const auto previewPixels = oldGpu.read(old.preview(), clip);
    oldGpu.owner.destroy();
    old.poll();
    const auto saved = old.recovery();
    QVERIFY(saved.has_value());
    QCOMPARE(saved->firstRetained, qsizetype(2));
    QCOMPARE(saved->cursor, qsizetype(3));
    QVERIFY(saved->working.has_value());
    QCOMPARE(saved->working->size(), qsizetype(2));
    KisGpuTileStore store(m_gpu->owner, 256 * KisGpuTileStore::TileBytes);
    KisGpuEditSession restored(store, 2, *saved);
    settle(restored);
    QCOMPARE(restored.state(), KisGpuEditSession::State::Editing);
    QCOMPARE(m_gpu->read(restored.head(), clip), headPixels);
    QCOMPARE(m_gpu->read(restored.preview(), clip), previewPixels);
    QCOMPARE(restored.historySize(), qsizetype(3));
    QCOMPARE(restored.append(token, {}, clip), KisGpuEditSession::Result::Stale);
    const auto replacementToken = restored.currentToken();
    QVERIFY(bool(replacementToken));
    QVERIFY(replacementToken != token);
    QCOMPARE(restored.cancel(replacementToken), KisGpuEditSession::Result::Accepted);
    QVERIFY(restored.redo());
    QCOMPARE(m_gpu->read(restored.head(), clip), redoPixels);
    QVERIFY(restored.undo());
    QVERIFY(restored.undo());
    QVERIFY(!restored.undo());
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

void KisGpuEditSessionTest::replayRestoresLoadedPixelsAndPendingCommit()
{
    KisGpuEditSession::Recovery initial;
    initial.bounds = QRect(-1, -1, 2, 2);
    initial.pixels = QByteArray::fromHex("1122330044556680778899ffaabbcc40");
    KisGpuTestDevice oldGpu;
    KisGpuTileStore oldStore(oldGpu.owner, 128 * KisGpuTileStore::TileBytes);
    KisGpuEditSession old(oldStore, 4, initial);
    QVERIFY(settle(old));
    QCOMPARE(oldGpu.read(old.head(), initial.bounds), initial.pixels);
    const QRect clip(-32, -32, 64, 64);
    const auto input = dab(QPointF(0, 0), 0x8000FF00);
    auto token = old.begin();
    QCOMPARE(old.append(token, {input}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(old.commit(token), KisGpuEditSession::Result::Accepted);
    oldGpu.owner.destroy();
    old.poll();
    QCOMPARE(old.state(), KisGpuEditSession::State::Failed);
    auto saved = old.recovery();
    QVERIFY(saved.has_value());
    QVERIFY(saved->commitRequested);
    KisGpuTileStore store(m_gpu->owner, 128 * KisGpuTileStore::TileBytes);
    auto reference = store.upload(store.emptyVersion(), initial.bounds, initial.pixels);
    auto painted = store.paintDabs(reference.version, {input}, clip);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    store.poll();
    KisGpuEditSession restored(store, 4, *saved);
    QVERIFY(settle(restored));
    QCOMPARE(m_gpu->read(restored.head(), clip), m_gpu->read(painted.version, clip));
    QVERIFY(restored.undo());
    QCOMPARE(m_gpu->read(restored.head(), initial.bounds), initial.pixels);
    QVERIFY(restored.redo());
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(initial.pixels.size() * 2));
}

void KisGpuEditSessionTest::recoveryBudgetAndInvalidInputsPreserveAcceptedWork()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 2, KisGpuEditSession::Recovery{}, 1);
    QVERIFY(settle(session));
    const auto token = session.begin();
    const auto before = store.statistics();
    QCOMPARE(session.append(token, {dab(QPointF(0, 0), 0xFFFFFFFF)}, QRect(-10, -10, 20, 20)),
             KisGpuEditSession::Result::RecoveryBudgetExceeded);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QVERIFY(session.preview() == session.head());
    QCOMPARE(session.append(token, {dab(QPointF(0, 0), 0xFFFFFFFF)}, QRect()), KisGpuEditSession::Result::Accepted);
    session.commit(token);
    QVERIFY(settle(session));
    QCOMPARE(session.historySize(), qsizetype(1));
    auto invalid = KisGpuEditSession::Recovery{};
    invalid.cursor = 1;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, KisGpuEditSession(store, 2, invalid));
    invalid.cursor = 0;
    invalid.bounds = QRect(0, 0, 2, 2);
    invalid.pixels = QByteArray(4, '\0');
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, KisGpuEditSession(store, 2, invalid));
    QCOMPARE(store.statistics().submissions, before.submissions);
}

void KisGpuEditSessionTest::replayUsesBoundedGpuHistoryAndPreservesNewBranches()
{
    const QRect clip(0, 0, 64, 64);
    KisGpuEditSession::Recovery saved;
    QByteArray accepted, branchBase;
    {
        KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
        KisGpuEditSession session(store, 2);
        for (int i = 0; i < 30; ++i) {
            auto token = session.begin();
            QCOMPARE(session.append(token, {dab(QPointF(16 + i % 16, 16), 0x80ABCDEF)}, clip),
                     KisGpuEditSession::Result::Accepted);
            session.commit(token);
            QVERIFY(settle(session));
        }
        accepted = m_gpu->read(session.head(), clip);
        saved = *session.recovery();
    }
    {
        KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
        KisGpuEditSession session(store, 2, saved);
        QVERIFY(settle(session));
        QCOMPARE(m_gpu->read(session.head(), clip), accepted);
        QCOMPARE(session.historySize(), qsizetype(3));
        QVERIFY(session.undo());
        branchBase = m_gpu->read(session.head(), clip);
        QVERIFY(session.clearHistory());
        const auto token = session.begin();
        session.append(token, {dab(QPointF(32, 32), 0xFF123456)}, clip);
        session.commit(token);
        QVERIFY(settle(session));
        QVERIFY(!session.redo());
        accepted = m_gpu->read(session.head(), clip);
        saved = *session.recovery();
    }
    KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 1, saved);
    QVERIFY(settle(session));
    QCOMPARE(m_gpu->read(session.head(), clip), accepted);
    QVERIFY(session.undo());
    QCOMPARE(m_gpu->read(session.head(), clip), branchBase);
    QVERIFY(!session.undo());
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}
void KisGpuEditSessionTest::cleanup()
{
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    QCOMPARE(m_gpu->owner.errorCount(), 0);
}

void KisGpuEditSessionTest::replacementCommitsExactlyThePreview()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 8);
    const QRect clip(-64, -64, 128, 128);
    const auto old = dab(QPointF(-20.25, -10.25), 0x80FFFFFF);
    const auto replacement = dab(QPointF(20.25, 10.25), 0x80FFFFFF);
    const auto token = session.begin();
    QVERIFY(bool(token));
    QCOMPARE(session.replace(token, {old}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.replace(token, {replacement}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
    QVERIFY(settle(session));
    const auto accepted = m_gpu->read(session.head(), clip);
    const auto direct = store.paintDabs(store.emptyVersion(), {replacement}, clip);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    store.poll();
    QCOMPARE(accepted, m_gpu->read(direct.version, clip));
    QCOMPARE(accepted, m_gpu->read(session.preview(), clip));
    QCOMPARE(session.historySize(), qsizetype(2));
    QVERIFY(session.undo());
    QCOMPARE(session.head().tileCount(), qsizetype(0));
    QVERIFY(session.redo());
    QCOMPARE(m_gpu->read(session.head(), clip), accepted);
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

void KisGpuEditSessionTest::cancellationPreservesRedoAndRejectsStaleInput()
{
    KisGpuTileStore store(m_gpu->owner, 64 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 8);
    const QRect clip(0, 0, 128, 64);
    for (const auto center : {QPointF(16.25, 16.25), QPointF(80.25, 16.25)}) {
        const auto token = session.begin();
        QCOMPARE(session.append(token, {dab(center, 0xFF123456)}, clip), KisGpuEditSession::Result::Accepted);
        QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
        QVERIFY(settle(session));
    }
    const auto painted = m_gpu->read(session.head(), clip);
    QVERIFY(session.undo());
    const auto cancelled = session.begin();
    QCOMPARE(session.replace(cancelled, {dab(QPointF(40, 40), 0xFFABCDEF)}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.commit(cancelled), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.cancel(cancelled), KisGpuEditSession::Result::Accepted);
    const auto active = session.begin();
    QVERIFY(active != cancelled);
    const auto before = store.statistics().submissions;
    QCOMPARE(session.append(cancelled, {dab(QPointF(16, 16), 0xFFFFFFFF)}, clip), KisGpuEditSession::Result::Stale);
    QCOMPARE(store.statistics().submissions, before);
    session.poll();
    QCOMPARE(session.cancel(active), KisGpuEditSession::Result::Accepted);
    QVERIFY(session.redo());
    QCOMPARE(m_gpu->read(session.head(), clip), painted);
    // An empty committed operation also preserves the redo branch.
    QVERIFY(session.undo());
    const auto empty = session.begin();
    QCOMPARE(session.append(empty, {}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.commit(empty), KisGpuEditSession::Result::Accepted);
    QVERIFY(settle(session));
    QVERIFY(session.redo());
    QCOMPARE(m_gpu->read(session.head(), clip), painted);
}

void KisGpuEditSessionTest::historyLimitReleasesGpuVersions()
{
    KisGpuTileStore store(m_gpu->owner, 16 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 2);
    const QRect clip(0, 0, 64, 64);
    for (int i = 0; i < 10; ++i) {
        const auto token = session.begin();
        QCOMPARE(session.append(token, {dab(QPointF(16.25, 16.25), 0x80402010)}, clip), KisGpuEditSession::Result::Accepted);
        QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
        QVERIFY(settle(session));
    }
    QCOMPARE(session.historySize(), qsizetype(3));
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
    QVERIFY(session.undo());
    QVERIFY(session.undo());
    QVERIFY(!session.undo());
    QVERIFY(session.redo());
    QVERIFY(session.clearHistory());
    QCOMPARE(session.historySize(), qsizetype(1));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
}

void KisGpuEditSessionTest::rejectedUpdatesPreserveTheWorkingEdit()
{
    KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes, 1);
    KisGpuEditSession session(store, 8);
    const QRect clip(0, 0, 64, 64);
    const auto token = session.begin();
    QVERIFY(!session.begin());
    const auto first = dab(QPointF(16.25, 16.25), 0xFFABCDEF);
    QCOMPARE(session.append(token, {first}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.replace(token, {dab(QPointF(32.25, 16.25), 0xFF123456)}, clip), KisGpuEditSession::Result::GpuRejected);
    QCOMPARE(session.lastGpuError(), KisGpuTileStore::Error::QueueFull);
    QVERIFY(!session.undo());
    QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
    QVERIFY(settle(session));
    const auto accepted = m_gpu->read(session.head(), clip);
    const auto direct = store.paintDabs(store.emptyVersion(), {first}, clip);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    store.poll();
    QCOMPARE(accepted, m_gpu->read(direct.version, clip));
}

void KisGpuEditSessionTest::inputFromAnotherDocumentIsRejected()
{
    KisGpuTileStore store(m_gpu->owner, 16 * KisGpuTileStore::TileBytes);
    KisGpuEditSession first(store, 8), second(store, 8);
    const auto oldDocument = first.begin();
    const auto currentDocument = second.begin();
    const auto before = store.statistics().submissions;
    QCOMPARE(second.append(oldDocument, {dab(QPointF(16, 16), 0xFFABCDEF)}, QRect(0, 0, 64, 64)),
             KisGpuEditSession::Result::Stale);
    QCOMPARE(store.statistics().submissions, before);
    QCOMPARE(second.cancel(currentDocument), KisGpuEditSession::Result::Accepted);
    QCOMPARE(first.cancel(oldDocument), KisGpuEditSession::Result::Accepted);
    KisGpuEditSession::Token expired;
    {
        KisGpuEditSession closed(store, 8);
        expired = closed.begin();
    }
    QVERIFY(!expired);
    const auto reopened = second.begin();
    QCOMPARE(second.commit(expired), KisGpuEditSession::Result::Stale);
    QCOMPARE(second.cancel(reopened), KisGpuEditSession::Result::Accepted);
}

void KisGpuEditSessionTest::deviceLossNeverCommitsTentativePixels()
{
    KisGpuTestDevice isolatedGpu;
    KisGpuTileStore store(isolatedGpu.owner, 16 * KisGpuTileStore::TileBytes);
    KisGpuEditSession session(store, 8);
    const QRect clip(0, 0, 64, 64);
    const auto token = session.begin();
    QCOMPARE(session.append(token, {dab(QPointF(16, 16), 0xFFABCDEF)}, clip), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
    store.invalidateDevice();
    session.poll();
    QCOMPARE(session.state(), KisGpuEditSession::State::Failed);
    QCOMPARE(session.head().tileCount(), qsizetype(0));
    QCOMPARE(session.preview().tileCount(), qsizetype(0));
    QCOMPARE(session.historySize(), qsizetype(1));
    QCOMPARE(session.cancel(token), KisGpuEditSession::Result::Accepted);
    QVERIFY(!session.begin());
}

void KisGpuEditSessionTest::importedVersionStartsAnEditableDocument()
{
    KisGpuTileStore store(m_gpu->owner, 16 * KisGpuTileStore::TileBytes);
    const QRect bounds(0, 0, 64, 64);
    const QByteArray original(64 * 64 * 4, '\xFF');
    const auto loaded = store.upload(store.emptyVersion(), bounds, original);
    QCOMPARE(loaded.error, KisGpuTileStore::Error::None);
    QVERIFY_EXCEPTION_THROWN(KisGpuEditSession(store, 2, loaded.version), std::invalid_argument);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    store.poll();
    KisGpuEditSession session(store, 2, loaded.version);
    QCOMPARE(m_gpu->read(session.head(), bounds), original);
    QVERIFY(!session.undo());
    const auto token = session.begin();
    QCOMPARE(session.append(token, {dab(QPointF(16.25, 16.25), 0xFF123456)}, bounds), KisGpuEditSession::Result::Accepted);
    QCOMPARE(session.commit(token), KisGpuEditSession::Result::Accepted);
    QVERIFY(settle(session));
    QVERIFY(m_gpu->read(session.head(), bounds) != original);
    QVERIFY(session.undo());
    QCOMPARE(m_gpu->read(session.head(), bounds), original);
    QVERIFY(!session.undo());
    KisGpuTileStore foreign(m_gpu->owner, 16 * KisGpuTileStore::TileBytes);
    QVERIFY_EXCEPTION_THROWN(KisGpuEditSession(store, 2, foreign.emptyVersion()), std::invalid_argument);
}

QTEST_GUILESS_MAIN(KisGpuEditSessionTest)
#include "KisGpuEditSessionTest.moc"
