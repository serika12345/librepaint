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
bool settle(Document &document)
{
    QElapsedTimer timer;
    timer.start();
    while ((document.state() == Document::State::Restoring || document.state() == Document::State::Committing)
           && timer.elapsed() < 5000) {
        document.poll();
        QTest::qWait(1);
    }
    return document.state() == Document::State::Idle;
}
bool paint(Document &document, Document::LayerId id, quint32 rgba, Document::Target target = Document::Target::Pixels)
{
    const auto edit = document.begin(id, target);
    Store::DabCommand command;
    command.center = {8, 8}; command.diameter = {5, 5}; command.rgba = rgba;
    return edit.result == Document::Result::Accepted
        && edit.session->append(edit.token, {command}, Bounds) == Session::Result::Accepted
        && document.commit(edit.token) == Document::Result::Accepted && settle(document);
}
Store::Version target(const Document &document, bool mask)
{
    return mask ? document.layers()[0].paint.mask : document.layers()[0].paint.pixels;
}
}

/** GPU pressure releases history heads while CPU values preserve asynchronous Undo/Redo. */
class KisGpuLayerDocumentResidencyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void evictionReleasesOnlyUnneededVersions();
    void evictionPreservesActiveInput_data();
    void evictionPreservesActiveInput();
    void evictedUndoKeepsCurrentFrame_data();
    void evictedUndoKeepsCurrentFrame();
    void recoveryLoadsOnlyTheSelectedFrame();
    void failedAndCancelledUndoKeepCurrentFrame();
    void pendingUndoSurvivesDeviceReplacement();
};

void KisGpuLayerDocumentResidencyTest::evictionReleasesOnlyUnneededVersions()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 32 * Store::TileBytes);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    auto retained = document.layers()[0].paint.pixels;
    const auto expectedRetained = gpu.read(retained, Bounds);
    QVERIFY(paint(document, id, 0xFF445566));
    QVERIFY(paint(document, id, 0xFF778899));
    const auto current = document.layers()[0].paint.pixels;
    QCOMPARE(store->statistics().residentBytes, 3 * Store::TileBytes);
    const auto bytes = document.recoveryBytes();
    const auto history = document.historySize();
    const auto before = store->statistics();
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(document.recoveryBytes(), bytes);
    QCOMPARE(document.historySize(), history);
    QVERIFY(document.layers()[0].paint.pixels == current);
    QCOMPARE(gpu.read(retained, Bounds), expectedRetained);
    retained = {};
    QCOMPARE(store->statistics().residentBytes, Store::TileBytes);
    const auto edit = document.begin(id);
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    QCOMPARE(document.cancel(edit.token), Document::Result::Accepted);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentResidencyTest::evictedUndoKeepsCurrentFrame_data()
{
    QTest::addColumn<bool>("mask");
    QTest::newRow("pixels") << false;
    QTest::newRow("mask") << true;
}

void KisGpuLayerDocumentResidencyTest::evictionPreservesActiveInput_data()
{
    QTest::addColumn<bool>("mask");
    QTest::addColumn<bool>("committing");
    QTest::newRow("pixels-editing") << false << false;
    QTest::newRow("mask-editing") << true << false;
    QTest::newRow("pixels-committing") << false << true;
    QTest::newRow("mask-committing") << true << true;
}

void KisGpuLayerDocumentResidencyTest::evictionPreservesActiveInput()
{
    QFETCH(bool, mask);
    QFETCH(bool, committing);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 8 * Store::TileBytes);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    QVERIFY(paint(document, id, 0xFF445566));
    const auto expectedEarlier = gpu.read(document.layers()[0].paint.pixels, Bounds);
    QVERIFY(paint(document, id, 0xFF778899));
    if (mask) QVERIFY(paint(document, id, 0x80123456, Document::Target::Mask));
    const auto edit = document.begin(id, mask ? Document::Target::Mask : Document::Target::Pixels);
    QCOMPARE(edit.result, Document::Result::Accepted);
    Store::DabCommand command;
    command.center = {20, 20}; command.diameter = {5, 5}; command.rgba = 0xCC99AABB;
    QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::Accepted);
    wgpuDevicePoll(gpu.device, true, nullptr);
    document.poll();
    const auto preview = edit.session->preview();
    const auto expectedPreview = gpu.read(preview, Bounds);
    const QRect firstFootprint(18, 18, 3, 3);
    const auto expectedFirstFootprint = gpu.read(preview, firstFootprint);
    const auto cursor = document.recovery().cursor;
    const auto history = document.historySize();
    const auto cpuBytes = document.recoveryBytes();
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    if (committing) {
        QCOMPARE(document.commit(edit.token), Document::Result::Accepted);
        QCOMPARE(document.state(), Document::State::Committing);
    } else {
        QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::GpuRejected);
        QCOMPARE(edit.session->lastGpuError(), Store::Error::BudgetExceeded);
        QCOMPARE(document.state(), Document::State::Editing);
    }
    const auto before = store->statistics();
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    QCOMPARE(store->statistics().residentBytes, before.residentBytes - 2 * Store::TileBytes);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(document.historySize(), history);
    QCOMPARE(document.recovery().cursor, cursor);
    QCOMPARE(document.recoveryBytes(), cpuBytes);
    QVERIFY(document.currentEditing().token == edit.token);
    QVERIFY(edit.session->preview() == preview);
    const auto projection = document.projectionLayers()[0];
    QVERIFY((mask ? projection.mask : projection.pixels) == preview);
    if (!committing) {
        command.center = {24, 24}; command.rgba = 0xFFBBCCDD;
        QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::Accepted);
        QCOMPARE(document.commit(edit.token), Document::Result::Accepted);
    }
    QVERIFY(settle(document));
    occupied = {};
    QCOMPARE(document.historySize(), history + 1);
    QCOMPARE(document.recovery().cursor, cursor + 1);
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    if (committing) QCOMPARE(gpu.read(target(document, mask), Bounds), expectedPreview);
    else {
        QVERIFY(gpu.read(target(document, mask), Bounds) != expectedPreview);
        QCOMPARE(gpu.read(target(document, mask), firstFootprint), expectedFirstFootprint);
    }
    QVERIFY(document.undo());
    QVERIFY(settle(document));
    if (mask) { QVERIFY(document.undo()); QVERIFY(settle(document)); }
    QVERIFY(document.undo());
    QCOMPARE(document.state(), Document::State::Restoring);
    QVERIFY(settle(document));
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, Bounds), expectedEarlier);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentResidencyTest::evictedUndoKeepsCurrentFrame()
{
    QFETCH(bool, mask);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 32 * Store::TileBytes, 1);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    QVERIFY(paint(document, id, 0x80123456, Document::Target::Mask));
    QVERIFY(document.clearHistory());
    const auto expectedPrevious = gpu.read(target(document, mask), Bounds);
    QVERIFY(paint(document, id, 0xCC7799AA, mask ? Document::Target::Mask : Document::Target::Pixels));
    const auto current = target(document, mask);
    const auto expectedCurrent = gpu.read(current, Bounds);
    QVERIFY(expectedPrevious != expectedCurrent);
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    const auto before = store->statistics();
    QVERIFY(document.undo());
    QCOMPARE(document.state(), Document::State::Restoring);
    QVERIFY(target(document, mask) == current);
    const auto projection = document.projectionLayers()[0];
    QVERIFY((mask ? projection.mask : projection.pixels) == current);
    QCOMPARE(document.begin(id).result, Document::Result::Busy);
    QCOMPARE(document.renameLayer(id, QStringLiteral("Busy")), Document::Result::Busy);
    QVERIFY(!document.clearHistory());
    QVERIFY(!document.undo());
    QVERIFY(!document.redo());
    QCOMPARE(document.recovery().cursor, qsizetype(0));
    QVERIFY(settle(document));
    QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.read(target(document, mask), Bounds), expectedPrevious);
    const auto submitted = store->statistics().submissions;
    QVERIFY(document.redo());
    QCOMPARE(document.state(), Document::State::Idle);
    QVERIFY(target(document, mask) == current);
    QCOMPARE(store->statistics().submissions, submitted);
    QVERIFY(document.undo());
    QCOMPARE(document.evictHistory(), Document::Result::Accepted);
    QVERIFY(document.redo());
    QCOMPARE(document.state(), Document::State::Restoring);
    QVERIFY(settle(document));
    QCOMPARE(gpu.read(target(document, mask), Bounds), expectedCurrent);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentResidencyTest::recoveryLoadsOnlyTheSelectedFrame()
{
    Document::Recovery saved;
    QVector<QByteArray> expected;
    {
        KisGpuTestDevice sourceGpu;
        auto sourceStore = std::make_shared<Store>(sourceGpu.owner, 32 * Store::TileBytes);
        Document source(sourceStore, 8);
        const auto id = source.addLayer(QStringLiteral("Paint")).id;
        for (const auto rgba : {0xFF112233U, 0xFF445566U, 0xFF778899U}) {
            QVERIFY(paint(source, id, rgba));
            expected.push_back(sourceGpu.read(source.layers()[0].paint.pixels, Bounds));
        }
        saved = source.recovery();
    }
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 3 * Store::TileBytes + 4096, 1);
    Document restored(store, 8, saved);
    QVERIFY(restored.layers().isEmpty());
    QVERIFY(settle(restored));
    QCOMPARE(store->statistics().residentBytes, Store::TileBytes);
    QCOMPARE(restored.historySize(), saved.history.size());
    QCOMPARE(gpu.read(restored.layers()[0].paint.pixels, Bounds), expected[2]);
    for (int i = 1; i >= 0; --i) {
        QCOMPARE(restored.evictHistory(), Document::Result::Accepted);
        QVERIFY(restored.undo());
        QVERIFY(settle(restored));
        QCOMPARE(gpu.read(restored.layers()[0].paint.pixels, Bounds), expected[i]);
        QCOMPARE(restored.evictHistory(), Document::Result::Accepted);
        QCOMPARE(store->statistics().residentBytes, Store::TileBytes);
    }
    QVERIFY(restored.undo());
    QVERIFY(settle(restored));
    QCOMPARE(restored.layers()[0].paint.pixels.tileCount(), qsizetype(0));
    QVERIFY(restored.undo());
    QVERIFY(restored.layers().isEmpty());
    QVERIFY(!restored.undo());
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentResidencyTest::failedAndCancelledUndoKeepCurrentFrame()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 32 * Store::TileBytes);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    QVERIFY(paint(document, id, 0xFF445566));
    const auto current = document.layers()[0].paint.pixels;
    const auto cursor = document.recovery().cursor;
    document.evictHistory();
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    QVERIFY(document.undo());
    QVERIFY(!settle(document));
    QCOMPARE(document.state(), Document::State::Failed);
    QCOMPARE(document.recoveryError(), Store::Error::BudgetExceeded);
    QVERIFY(document.layers()[0].paint.pixels == current);
    occupied = {};
    QVERIFY(document.cancelHistoryRestore());
    QCOMPARE(document.state(), Document::State::Idle);
    QCOMPARE(document.recovery().cursor, cursor);
    QVERIFY(document.layers()[0].paint.pixels == current);
    QVERIFY(!document.cancelHistoryRestore());
    const auto before = store->statistics().submissions;
    QVERIFY(document.undo());
    QElapsedTimer timer;
    timer.start();
    while (store->statistics().submissions == before && timer.elapsed() < 5000) document.poll();
    QVERIFY(store->statistics().submissions > before);
    QCOMPARE(document.state(), Document::State::Restoring);
    QVERIFY(document.cancelHistoryRestore());
    wgpuDevicePoll(gpu.device, true, nullptr);
    document.poll();
    QVERIFY(document.layers()[0].paint.pixels == current);
    QCOMPARE(document.recovery().cursor, cursor);
    QVERIFY(document.undo());
    QVERIFY(settle(document));
    QVERIFY(!(document.layers()[0].paint.pixels == current));
    QCOMPARE(document.recoveryError(), Store::Error::None);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentResidencyTest::pendingUndoSurvivesDeviceReplacement()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 32 * Store::TileBytes);
    Document document(store, 8);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    const auto previous = gpu.read(document.layers()[0].paint.pixels, Bounds);
    QVERIFY(paint(document, id, 0xFF445566));
    const auto current = gpu.read(document.layers()[0].paint.pixels, Bounds);
    document.evictHistory();
    QVERIFY(document.undo());
    QCOMPARE(document.state(), Document::State::Restoring);
    gpu.owner.destroy();
    document.poll();
    QCOMPARE(document.state(), Document::State::Failed);
    const auto pending = document.recovery();
    KisGpuTestDevice replacement;
    auto replacementStore = std::make_shared<Store>(replacement.owner, 32 * Store::TileBytes);
    Document restored(replacementStore, 8, pending);
    QVERIFY(settle(restored));
    QCOMPARE(replacement.read(restored.layers()[0].paint.pixels, Bounds), previous);
    QVERIFY(restored.redo());
    QVERIFY(settle(restored));
    QCOMPARE(replacement.read(restored.layers()[0].paint.pixels, Bounds), current);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuLayerDocumentResidencyTest)
#include "KisGpuLayerDocumentResidencyTest.moc"
