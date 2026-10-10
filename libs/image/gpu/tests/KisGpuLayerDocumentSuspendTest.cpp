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
const QRect Bounds(0, 0, 32, 32);
Store::DabCommand dab(quint32 rgba)
{
    Store::DabCommand command;
    command.center = {8, 8}; command.diameter = {12, 12}; command.rgba = rgba;
    return command;
}
bool settle(Document &document)
{
    QElapsedTimer timer; timer.start();
    while ((document.state() == Document::State::Restoring || document.state() == Document::State::Committing)
           && timer.elapsed() < 5000) { document.poll(); QTest::qWait(1); }
    return document.state() == Document::State::Idle || document.state() == Document::State::Editing;
}
bool paint(Document &document, Document::LayerId id, quint32 rgba,
           Document::Target target = Document::Target::Pixels)
{
    const auto edit = document.begin(id, target);
    return edit.result == Document::Result::Accepted
        && edit.session->append(edit.token, {dab(rgba)}, Bounds) == Session::Result::Accepted
        && document.commit(edit.token) == Document::Result::Accepted && settle(document);
}
}

/** One document retains CPU editing state while replacing its GPU ownership. */
class KisGpuLayerDocumentSuspendTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void preservesDocumentAcrossDeviceReplacement();
    void unfinishedInput_data();
    void unfinishedInput();
    void pendingUndoAndFailedResumeKeepCpuState();
    void suspensionCancelsMaintenanceAndSurvivesLoss();
};
void KisGpuLayerDocumentSuspendTest::preservesDocumentAcrossDeviceReplacement()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 16);
    document.addLayer(QStringLiteral("Paint"));
    document.addLayer(QStringLiteral("Top"));
    QVERIFY(paint(document, 1, 0xFF112233));
    QVERIFY(paint(document, 1, 0x80123456, Document::Target::Mask));
    QVERIFY(paint(document, 2, 0xCC778899));
    document.setLayerProperties(1, 177, Store::CompositeOp::Erase);
    document.setLayerLocked(1, true);
    document.setLayerVisible(2, false);
    document.renameLayer(1, QStringLiteral("Later"));
    QVERIFY(document.undo());
    const auto expectedPixels = gpu.read(document.layers()[0].paint.pixels, Bounds);
    const auto expectedMask = gpu.read(document.layers()[0].paint.mask, Bounds);
    const auto recovery = document.recovery();
    const auto before = store->statistics();
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    QCOMPARE(document.state(), Document::State::Suspended);
    QCOMPARE(document.historySize(), recovery.history.size());
    QCOMPARE(document.recovery().cursor, recovery.cursor);
    QCOMPARE(document.recovery().nextId, recovery.nextId);
    QVERIFY(document.layers().isEmpty());
    QVERIFY(document.projectionLayers().isEmpty());
    QCOMPARE(document.begin(1).result, Document::Result::Busy);
    QCOMPARE(document.renameLayer(1, QStringLiteral("Rejected")), Document::Result::Busy);
    QVERIFY(!document.undo()); QVERIFY(!document.redo()); QVERIFY(!document.clearHistory());
    document.poll();
    QCOMPARE(document.recoveryError(), Store::Error::None);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().residentBytes, quint64(0));
    QCOMPARE(document.resume({}), Document::Result::InvalidInput);
    QCOMPARE(document.state(), Document::State::Suspended);
    gpu.owner.destroy();
    KisGpuTestDevice replacement;
    auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
    QCOMPARE(document.resume(nextStore), Document::Result::Accepted);
    QCOMPARE(document.state(), Document::State::Restoring);
    QVERIFY(document.layers().isEmpty());
    QVERIFY(settle(document));
    QCOMPARE(document.state(), Document::State::Idle);
    QCOMPARE(document.layers()[0].name, QStringLiteral("Paint"));
    QCOMPARE(document.layers()[0].paint.opacity, quint8(177));
    QCOMPARE(document.layers()[0].paint.operation, Store::CompositeOp::Erase);
    QVERIFY(document.layers()[0].locked);
    QVERIFY(!document.layers()[1].visible);
    QCOMPARE(document.projectionLayers().size(), 1);
    QCOMPARE(document.begin(1).result, Document::Result::LayerNotEditable);
    QCOMPARE(replacement.read(document.layers()[0].paint.pixels, Bounds), expectedPixels);
    QCOMPARE(replacement.read(document.layers()[0].paint.mask, Bounds), expectedMask);
    QVERIFY(document.redo());
    QVERIFY(settle(document));
    QCOMPARE(document.layers()[0].name, QStringLiteral("Later"));
    QCOMPARE(document.addLayer(QStringLiteral("New")).id, quint64(3));
    QCOMPARE(document.resume(nextStore), Document::Result::Busy);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

void KisGpuLayerDocumentSuspendTest::unfinishedInput_data()
{
    QTest::addColumn<bool>("mask");
    QTest::addColumn<int>("phase");
    for (bool mask : {false, true}) for (int phase = 0; phase < 3; ++phase) {
        const QByteArray name = QByteArray(mask ? "mask-" : "pixels-") + QByteArray::number(phase);
        QTest::newRow(name.constData()) << mask << phase;
    }
}

void KisGpuLayerDocumentSuspendTest::unfinishedInput()
{
    QFETCH(bool, mask);
    QFETCH(int, phase);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 16);
    document.addLayer(QStringLiteral("Paint"));
    const auto target = mask ? Document::Target::Mask : Document::Target::Pixels;
    QVERIFY(paint(document, 1, 0xFF112233, target));
    const auto committed = gpu.read(mask ? document.layers()[0].paint.mask : document.layers()[0].paint.pixels, Bounds);
    auto edit = document.begin(1, target);
    QCOMPARE(edit.session->append(edit.token, {dab(0x80123456)}, Bounds), Session::Result::Accepted);
    wgpuDevicePoll(gpu.device, true, nullptr);
    edit.session->poll();
    const auto preview = gpu.read(edit.session->preview(), Bounds);
    if (phase) QCOMPARE(document.commit(edit.token), Document::Result::Accepted);
    if (phase == 2) {
        QElapsedTimer timer; timer.start();
        while (edit.session->state() == Session::State::Committing && timer.elapsed() < 5000) {
            edit.session->poll(); QTest::qWait(1);
        }
        QCOMPARE(edit.session->state(), Session::State::Idle);
    }
    const auto recovery = document.recovery();
    QVERIFY(recovery.working);
    const auto before = store->statistics();
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    QCOMPARE(document.state(), Document::State::Suspended);
    QCOMPARE(edit.session->append(edit.token, {dab(0xFF000000)}, Bounds), Session::Result::Stale);
    QCOMPARE(document.commit(edit.token), Document::Result::Stale);
    QCOMPARE(document.cancel(edit.token), Document::Result::Stale);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(document.recovery().working->completed, recovery.working->completed);
    edit.session.reset();
    gpu.owner.destroy();
    KisGpuTestDevice replacement;
    auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
    QCOMPARE(document.resume(nextStore), Document::Result::Accepted);
    QVERIFY(settle(document));
    if (!phase) {
        QCOMPARE(document.state(), Document::State::Editing);
        const auto current = document.currentEditing();
        QVERIFY(current.token != edit.token);
        QCOMPARE(replacement.read(current.session->head(), Bounds), committed);
        QCOMPARE(replacement.read(current.session->preview(), Bounds), preview);
        QCOMPARE(document.cancel(current.token), Document::Result::Accepted);
    } else {
        QCOMPARE(document.state(), Document::State::Idle);
        QCOMPARE(replacement.read(mask ? document.layers()[0].paint.mask : document.layers()[0].paint.pixels, Bounds), preview);
        QVERIFY(document.undo());
        QVERIFY(settle(document));
    }
    QCOMPARE(replacement.read(mask ? document.layers()[0].paint.mask : document.layers()[0].paint.pixels, Bounds), committed);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

void KisGpuLayerDocumentSuspendTest::pendingUndoAndFailedResumeKeepCpuState()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 16);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto earlier = gpu.read(document.layers()[0].paint.pixels, Bounds);
    QVERIFY(paint(document, 1, 0x80123456));
    const auto later = gpu.read(document.layers()[0].paint.pixels, Bounds);
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    QVERIFY(document.undo());
    QCOMPARE(document.state(), Document::State::Restoring);
    const auto cursor = document.recovery().cursor;
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    gpu.owner.destroy();
    KisGpuTestDevice replacement;
    auto tight = std::make_shared<Store>(replacement.owner, 4096);
    QCOMPARE(document.resume(tight), Document::Result::Accepted);
    QVERIFY(!settle(document));
    QCOMPARE(document.state(), Document::State::Failed);
    QCOMPARE(document.recoveryError(), Store::Error::BudgetExceeded);
    QVERIFY(document.layers().isEmpty());
    QCOMPARE(document.recovery().cursor, cursor);
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
    QCOMPARE(document.resume(nextStore), Document::Result::Accepted);
    QVERIFY(settle(document));
    QCOMPARE(replacement.read(document.layers()[0].paint.pixels, Bounds), earlier);
    QVERIFY(document.redo());
    QVERIFY(settle(document));
    QCOMPARE(replacement.read(document.layers()[0].paint.pixels, Bounds), later);
}

void KisGpuLayerDocumentSuspendTest::suspensionCancelsMaintenanceAndSurvivesLoss()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 16);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    QVERIFY(paint(document, 1, 0x80123456));
    const auto expected = gpu.read(document.layers()[0].paint.pixels, Bounds);
    const auto recovery = document.recovery();
    QCOMPARE(document.compactRecovery(1), Document::Result::Accepted);
    document.poll();
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    QVERIFY(!document.recoveryCompactionStatus());
    QCOMPARE(document.recovery().history[recovery.cursor][0]->pixels, recovery.history[recovery.cursor][0]->pixels);
    QCOMPARE(document.resume(store), Document::Result::Accepted);
    QVERIFY(settle(document));
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, Bounds), expected);
    gpu.owner.destroy();
    QCOMPARE(document.state(), Document::State::Failed);
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    document.poll();
    KisGpuTestDevice replacement;
    auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
    QCOMPARE(document.resume(nextStore), Document::Result::Accepted);
    QVERIFY(settle(document));
    QCOMPARE(replacement.read(document.layers()[0].paint.pixels, Bounds), expected);
}
QTEST_GUILESS_MAIN(KisGpuLayerDocumentSuspendTest)
#include "KisGpuLayerDocumentSuspendTest.moc"
