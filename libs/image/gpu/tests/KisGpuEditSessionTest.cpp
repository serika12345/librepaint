/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include "KisGpuTestDevice.h"
#include "KisGpuEditSession.h"

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
    while (session.state() == KisGpuEditSession::State::Committing && timer.elapsed() < 5000) {
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
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuEditSessionTest::initTestCase() { m_gpu = std::make_unique<KisGpuTestDevice>(); }
void KisGpuEditSessionTest::cleanup()
{
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    QCOMPARE(m_gpu->errors.load(), 0);
}

void KisGpuEditSessionTest::replacementCommitsExactlyThePreview()
{
    KisGpuTileStore store(m_gpu->device, 32 * KisGpuTileStore::TileBytes);
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
}

void KisGpuEditSessionTest::cancellationPreservesRedoAndRejectsStaleInput()
{
    KisGpuTileStore store(m_gpu->device, 64 * KisGpuTileStore::TileBytes);
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
    KisGpuTileStore store(m_gpu->device, 16 * KisGpuTileStore::TileBytes);
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
    KisGpuTileStore store(m_gpu->device, 8 * KisGpuTileStore::TileBytes, 1);
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
    KisGpuTileStore store(m_gpu->device, 16 * KisGpuTileStore::TileBytes);
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
    KisGpuTileStore store(m_gpu->device, 16 * KisGpuTileStore::TileBytes);
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

QTEST_GUILESS_MAIN(KisGpuEditSessionTest)
#include "KisGpuEditSessionTest.moc"
