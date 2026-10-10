/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <stdexcept>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
const QRect Bounds(-16, -16, 32, 32);
Store::DabCommand dab(QPointF point, quint32 rgba)
{
    Store::DabCommand command;
    command.center = point;
    command.diameter = QSizeF(5, 5);
    command.rgba = rgba;
    return command;
}
bool finish(KisGpuTestDevice &gpu, Document &document)
{
    QElapsedTimer timer;
    timer.start();
    while (document.state() == Document::State::Committing && timer.elapsed() < 5000) {
        document.poll();
        QTest::qWait(1);
    }
    wgpuDevicePoll(gpu.device, true, nullptr);
    document.poll();
    return document.state() == Document::State::Idle;
}
}

/** Document recovery restores immutable layer history and unfinished edits. */
class KisGpuLayerDocumentRecoveryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void recoveryRestoresLayerStructureAndGlobalHistory();
    void unfinishedEditing_data();
    void unfinishedEditing();
    void invalidHistoryIsRejectedBeforeGpu();
    void partialFailureKeepsCpuRecoveryAndReleasesGpu();
    void lostDeviceCanRetryTheWholeDocument();
    void metadataHistorySharesRebuiltPixelsAndMasks();
};

void KisGpuLayerDocumentRecoveryTest::recoveryRestoresLayerStructureAndGlobalHistory()
{
    KisGpuTestDevice originalGpu;
    auto originalStore = std::make_shared<Store>(originalGpu.owner, 256 * Store::TileBytes);
    Document original(originalStore, 16);
    const auto bottom = original.addLayer(QStringLiteral("Bottom"));
    const auto top = original.addLayer(QStringLiteral("Top"));
    for (const auto id : {bottom.id, top.id}) {
        const auto edit = original.begin(id);
        edit.session->append(edit.token, {dab({0, 0}, id == bottom.id ? 0xFF0000FF : 0xFFFF0000)}, Bounds);
        original.commit(edit.token);
        QVERIFY(finish(originalGpu, original));
    }
    original.renameLayer(bottom.id, QStringLiteral("Sketch"));
    original.moveLayer(bottom.id, 1);
    original.setLayerProperties(bottom.id, 77, Store::CompositeOp::Erase);
    {
        const auto edit = original.begin(bottom.id, Document::Target::Mask);
        edit.session->append(edit.token, {dab({0, 0}, 0x88123456)}, Bounds);
        original.commit(edit.token);
        QVERIFY(finish(originalGpu, original));
    }
    original.renameLayer(top.id, QStringLiteral("Later name"));
    QVERIFY(original.undo());
    const auto expected = original.layers();
    QVector<QByteArray> pixels, masks;
    for (const auto &layer : expected) {
        pixels.push_back(originalGpu.read(layer.paint.pixels, Bounds));
        masks.push_back(originalGpu.read(layer.paint.mask, Bounds));
    }
    const auto saved = original.recovery();
    QCOMPARE(saved.history.size(), original.historySize());
    originalGpu.owner.destroy();
    original.poll();
    KisGpuTestDevice replacement;
    auto store = std::make_shared<Store>(replacement.owner, 256 * Store::TileBytes, 1);
    Document restored(store, 16, saved);
    QVERIFY(restored.layers().isEmpty());
    QElapsedTimer timer;
    timer.start();
    while (restored.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        restored.poll();
        QTest::qWait(1);
    }
    QCOMPARE(restored.state(), Document::State::Idle);
    QCOMPARE(restored.historySize(), original.historySize());
    const auto actual = restored.layers();
    QCOMPARE(actual.size(), expected.size());
    for (qsizetype i = 0; i < actual.size(); ++i) {
        QCOMPARE(actual[i].id, expected[i].id);
        QCOMPARE(actual[i].name, expected[i].name);
        QCOMPARE(actual[i].paint.opacity, expected[i].paint.opacity);
        QCOMPARE(actual[i].paint.operation, expected[i].paint.operation);
        QCOMPARE(replacement.read(actual[i].paint.pixels, Bounds), pixels[i]);
        QCOMPARE(replacement.read(actual[i].paint.mask, Bounds), masks[i]);
    }
    const auto before = store->statistics();
    QVERIFY(restored.redo());
    QCOMPARE(restored.layers()[0].name, QStringLiteral("Later name"));
    QVERIFY(restored.undo());
    QCOMPARE(store->statistics().submissions, before.submissions);
    const auto added = restored.addLayer(QStringLiteral("New"));
    QVERIFY(added.id > top.id);
    QVERIFY(!restored.redo());
    QCOMPARE(replacement.owner.errorCount(), 0);
}


void KisGpuLayerDocumentRecoveryTest::unfinishedEditing_data()
{
    QTest::addColumn<int>("phase");
    QTest::addColumn<bool>("mask");
    for (int phase = 0; phase < 3; ++phase) {
        for (bool mask : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(phase).arg(mask ? "mask" : "pixels"))) << phase << mask;
        }
    }
}

void KisGpuLayerDocumentRecoveryTest::unfinishedEditing()
{
    QFETCH(int, phase);
    QFETCH(bool, mask);
    KisGpuTestDevice originalGpu;
    auto originalStore = std::make_shared<Store>(originalGpu.owner, 256 * Store::TileBytes);
    Document original(originalStore, 16);
    const auto first = original.addLayer(QStringLiteral("First")).id;
    original.addLayer(QStringLiteral("Second"));
    const auto target = mask ? Document::Target::Mask : Document::Target::Pixels;
    for (const auto part : {Document::Target::Pixels, Document::Target::Mask}) {
        const auto edit = original.begin(first, part);
        QCOMPARE(edit.result, Document::Result::Accepted);
        QCOMPARE(edit.session->append(edit.token, {dab({0, 0}, 0xC0554422)}, Bounds), Session::Result::Accepted);
        QCOMPARE(original.commit(edit.token), Document::Result::Accepted);
        QVERIFY(finish(originalGpu, original));
    }
    original.renameLayer(first, QStringLiteral("Future"));
    QVERIFY(original.undo());
    const auto edit = original.begin(first, target);
    const auto committed = mask ? original.layers()[0].paint.mask : original.layers()[0].paint.pixels;
    const auto expectedCommitted = originalGpu.read(committed, Bounds);
    // The document's base record uses commands while the editing lease uses a
    // compacted checkpoint for the same head. Restoration reuses that GPU head.
    QCOMPARE(edit.session->compactRecovery(), Session::Result::Accepted);
    QElapsedTimer timer;
    timer.start();
    while (edit.session->recoveryCompactionStatus() == Store::Status::Pending && timer.elapsed() < 5000) {
        original.poll();
        QTest::qWait(1);
    }
    QCOMPARE(edit.session->recoveryCompactionStatus(), Store::Status::Succeeded);
    QVERIFY(!edit.session->recovery()->tiles.isEmpty());
    const auto beforeTexture = originalStore->statistics().pixelUploadBytes;
    const auto texture = edit.session->createTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3});
    QCOMPARE(texture.result, Session::Result::Accepted);
    const auto textureBytes = originalStore->statistics().pixelUploadBytes - beforeTexture;
    auto selectionDab = dab({0, 0}, 0x80000000);
    selectionDab.diameter = {64, 64};
    const auto selection = edit.session->createSelection({selectionDab}, Bounds);
    QCOMPARE(selection.result, Session::Result::Accepted);
    auto command = dab({-4, 0}, 0xFF3399CC);
    command.textureStrength = 77;
    QCOMPARE(edit.session->append(edit.token, {command}, Bounds, texture.texture, selection.selection), Session::Result::Accepted);
    command.center = {4, 0};
    QCOMPARE(edit.session->replace(edit.token, {command}, Bounds, texture.texture, selection.selection), Session::Result::Accepted);
    wgpuDevicePoll(originalGpu.device, true, nullptr);
    original.poll();
    const auto expectedPreview = originalGpu.read(edit.session->preview(), Bounds);
    QVERIFY(expectedPreview != expectedCommitted);
    if (phase) {
        QCOMPARE(original.commit(edit.token), Document::Result::Accepted);
        if (phase == 2) {
            edit.session->poll();
            QCOMPARE(edit.session->state(), Session::State::Idle);
        }
    }
    const auto saved = original.recovery();
    QVERIFY(saved.working.has_value());
    QCOMPARE(saved.working->completed, phase == 2);
    originalGpu.owner.destroy();
    original.poll();
    KisGpuTestDevice replacement;
    auto store = std::make_shared<Store>(replacement.owner, 256 * Store::TileBytes, 1);
    Document restored(store, 16, saved);
    QCOMPARE(restored.commit(edit.token), Document::Result::Stale);
    timer.restart();
    while (restored.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        QVERIFY(restored.layers().isEmpty());
        QVERIFY(restored.projectionLayers().isEmpty());
        restored.poll();
        QTest::qWait(1);
    }
    if (!phase) {
        QCOMPARE(restored.state(), Document::State::Editing);
        const auto current = restored.currentEditing();
        QCOMPARE(current.result, Document::Result::Accepted);
        QVERIFY(current.token != edit.token);
        QCOMPARE(current.session->append(edit.token, {command}, Bounds), Session::Result::Stale);
        QCOMPARE(replacement.read(current.session->head(), Bounds), expectedCommitted);
        QCOMPARE(replacement.read(current.session->preview(), Bounds), expectedPreview);
        const auto previewLayer = restored.projectionLayers()[0];
        QCOMPARE(replacement.read(mask ? previewLayer.mask : previewLayer.pixels, Bounds), expectedPreview);
        QCOMPARE(restored.cancel(current.token), Document::Result::Accepted);
        const auto actual = mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels;
        QCOMPARE(replacement.read(actual, Bounds), expectedCommitted);
        QVERIFY(restored.redo());
        QCOMPARE(restored.layers()[0].name, QStringLiteral("Future"));
    } else {
        QVERIFY(finish(replacement, restored));
        QCOMPARE(restored.lastPublicationResult(), Document::Result::Accepted);
        QVERIFY(!restored.redo());
        const auto actual = mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels;
        QCOMPARE(replacement.read(actual, Bounds), expectedPreview);
        const auto before = store->statistics();
        QVERIFY(restored.undo());
        QCOMPARE(store->statistics().submissions, before.submissions);
        const auto undone = mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels;
        QCOMPARE(replacement.read(undone, Bounds), expectedCommitted);
        const auto beforeRedo = store->statistics().submissions;
        QVERIFY(restored.redo());
        QCOMPARE(store->statistics().submissions, beforeRedo);
        const auto redone = mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels;
        QCOMPARE(replacement.read(redone, Bounds), expectedPreview);
    }
    if (phase != 2) QCOMPARE(store->statistics().pixelUploadBytes, textureBytes);
    QCOMPARE(replacement.owner.errorCount(), 0);
}


void KisGpuLayerDocumentRecoveryTest::invalidHistoryIsRejectedBeforeGpu()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 256 * Store::TileBytes);
    auto layer = std::make_shared<Document::RecoveryLayer>();
    layer->id = 1;
    Document::Recovery valid;
    valid.history = {{}, {layer}, {layer}};
    valid.cursor = 2;
    valid.nextId = 2;
    const auto rejects = [&](Document::Recovery request, qsizetype retained = 2, quint64 maximum = 64 * 1024 * 1024) {
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Document(store, retained, request, maximum));
    };
    auto bad = valid;
    bad.history.clear();
    rejects(bad);
    bad = valid; bad.cursor = -1; rejects(bad);
    bad = valid; bad.cursor = 3; rejects(bad);
    rejects(valid, 1);
    bad = valid; bad.nextId = 0; rejects(bad);
    bad = valid; bad.nextId = 1; rejects(bad);
    bad = valid; bad.history.back().push_back({}); rejects(bad);
    bad = valid; bad.history.back().push_back(layer); rejects(bad);
    auto corrupt = std::make_shared<Document::RecoveryLayer>(*layer);
    corrupt->id = 0;
    bad = valid; bad.history.back() = {corrupt}; rejects(bad);
    corrupt->id = 1;
    corrupt->operation = Store::CompositeOp(-1); rejects(bad);
    corrupt->operation = Store::CompositeOp::Over;
    auto corruptPixels = std::make_shared<Session::Recovery>();
    corrupt->pixels = corruptPixels;
    corruptPixels->bounds = {0, 0, 1, 1};
    corruptPixels->pixels = QByteArray(3, '\0'); rejects(bad);
    bad = valid;
    Session::Recovery working;
    working.working.emplace();
    bad.working = Document::RecoveryWorking{layer, Document::Target::Pixels, working, false};
    bad.working->base = std::make_shared<const Document::RecoveryLayer>(*layer); rejects(bad);
    bad.working->base = layer;
    bad.working->target = Document::Target(-1); rejects(bad);
    bad.working->target = Document::Target::Pixels;
    bad.working->data.working.reset(); rejects(bad);
    bad.working->completed = true;
    bad.working->data.commitRequested = true; rejects(bad);
    Document sample(store, 2, valid);
    rejects(valid, 2, sample.recoveryBytes() - 1);
    bad = valid;
    bad.working = Document::RecoveryWorking{layer, Document::Target::Pixels, working, false};
    rejects(bad, 2, sample.recoveryBytes());
    QCOMPARE(store->statistics().submissions, quint64(0));
}

void KisGpuLayerDocumentRecoveryTest::partialFailureKeepsCpuRecoveryAndReleasesGpu()
{
    KisGpuTestDevice gpu;
    auto record = std::make_shared<Document::RecoveryLayer>();
    record->id = 1;
    auto raster = std::make_shared<Session::Recovery>();
    record->pixels = raster;
    raster->edits = {{{{dab({8, 8}, 0xFF112233)}, {0, 0, 64, 64}, {}, {}}}};
    raster->cursor = raster->firstRetained = 1;
    auto second = std::make_shared<Document::RecoveryLayer>(*record);
    second->id = 2;
    second->pixels = std::make_shared<const Session::Recovery>(*raster);
    Document::Recovery saved;
    saved.history = {{record, second}};
    saved.nextId = 3;
    auto small = std::make_shared<Store>(gpu.owner, 2 * Store::TileBytes);
    Document restored(small, 0, saved);
    QVERIFY(!restored.clearHistory());
    QCOMPARE(restored.begin(1).result, Document::Result::Busy);
    QCOMPARE(restored.renameLayer(1, QStringLiteral("Busy")), Document::Result::Busy);
    QVERIFY(!restored.undo());
    QElapsedTimer timer;
    timer.start();
    while (restored.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        QVERIFY(restored.layers().isEmpty());
        restored.poll();
        QTest::qWait(1);
    }
    QCOMPARE(restored.state(), Document::State::Failed);
    QVERIFY(restored.layers().isEmpty());
    QCOMPARE(restored.recoveryError(), Store::Error::BudgetExceeded);
    QCOMPARE(restored.recovery().history, saved.history);
    wgpuDevicePoll(gpu.device, true, nullptr);
    restored.poll();
    QCOMPARE(small->statistics().residentBytes, quint64(0));
    auto sufficient = std::make_shared<Store>(gpu.owner, 16 * Store::TileBytes);
    Document retried(sufficient, 0, restored.recovery());
    timer.restart();
    while (retried.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        retried.poll();
        QTest::qWait(1);
    }
    QCOMPARE(retried.state(), Document::State::Idle);
    QCOMPARE(retried.layers().size(), 2);
    const auto pixels = gpu.read(retried.layers()[0].paint.pixels, {0, 0, 64, 64});
    QCOMPARE(gpu.read(retried.layers()[1].paint.pixels, {0, 0, 64, 64}), pixels);
    QCOMPARE(pixels.mid((8 * 64 + 8) * 4, 4), QByteArray::fromHex("332211ff"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentRecoveryTest::lostDeviceCanRetryTheWholeDocument()
{
    Document::Recovery saved;
    QByteArray expected;
    {
        KisGpuTestDevice sourceGpu;
        auto sourceStore = std::make_shared<Store>(sourceGpu.owner, 64 * Store::TileBytes);
        Document source(sourceStore, 8);
        const auto id = source.addLayer(QStringLiteral("Saved")).id;
        const auto edit = source.begin(id);
        edit.session->append(edit.token, {dab({0, 0}, 0xCC994433)}, Bounds);
        source.commit(edit.token);
        QVERIFY(finish(sourceGpu, source));
        saved = source.recovery();
        expected = sourceGpu.read(source.layers()[0].paint.pixels, Bounds);
    }
    KisGpuTestDevice lostGpu;
    auto lostStore = std::make_shared<Store>(lostGpu.owner, 64 * Store::TileBytes);
    Document lost(lostStore, 8, saved);
    QElapsedTimer timer;
    timer.start();
    while (!lostStore->statistics().submissions && timer.elapsed() < 5000) lost.poll();
    QVERIFY(lostStore->statistics().submissions > 0);
    lostGpu.owner.destroy();
    lost.poll();
    QCOMPARE(lost.state(), Document::State::Failed);
    QCOMPARE(lost.recoveryError(), Store::Error::DeviceLost);
    QVERIFY(lost.layers().isEmpty());
    QCOMPARE(lost.recovery().history, saved.history);
    KisGpuTestDevice replacement;
    auto replacementStore = std::make_shared<Store>(replacement.owner, 64 * Store::TileBytes);
    Document restored(replacementStore, 8, lost.recovery());
    timer.restart();
    while (restored.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        restored.poll();
        QTest::qWait(1);
    }
    QCOMPARE(restored.state(), Document::State::Idle);
    QCOMPARE(replacement.read(restored.layers()[0].paint.pixels, Bounds), expected);
    QCOMPARE(replacement.owner.errorCount(), 0);
}


void KisGpuLayerDocumentRecoveryTest::metadataHistorySharesRebuiltPixelsAndMasks()
{
    Document::Recovery saved;
    QByteArray pixels, mask;
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 16 * Store::TileBytes);
        Document source(store, 32);
        const auto id = source.addLayer(QStringLiteral("First")).id;
        for (const auto target : {Document::Target::Pixels, Document::Target::Mask}) {
            const auto edit = source.begin(id, target);
            edit.session->append(edit.token, {dab({8, 8}, 0xAA112233)}, {0, 0, 64, 64});
            source.commit(edit.token);
            QVERIFY(finish(gpu, source));
        }
        for (int i = 0; i < 12; ++i) {
            QCOMPARE(source.renameLayer(id, QString::number(i)), Document::Result::Accepted);
            QCOMPARE(source.setLayerProperties(id, 64 + i, Store::CompositeOp::Over), Document::Result::Accepted);
        }
        QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
        pixels = gpu.read(source.layers()[0].paint.pixels, {0, 0, 64, 64});
        mask = gpu.read(source.layers()[0].paint.mask, {0, 0, 64, 64});
        saved = source.recovery();
    }
    KisGpuTestDevice replacement;
    auto store = std::make_shared<Store>(replacement.owner, 3 * Store::TileBytes);
    Document restored(store, 32, saved);
    QElapsedTimer timer;
    timer.start();
    while (restored.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        restored.poll();
        QTest::qWait(1);
    }
    QCOMPARE(restored.state(), Document::State::Idle);
    QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
    // Inspect one pixel at a time so explicit read staging fits the same budget.
    QCOMPARE(replacement.read(restored.layers()[0].paint.pixels, {8, 8, 1, 1}), pixels.mid((8 * 64 + 8) * 4, 4));
    QCOMPARE(replacement.read(restored.layers()[0].paint.mask, {8, 8, 1, 1}), mask.mid((8 * 64 + 8) * 4, 4));
    for (int i = 0; i < 24; ++i) QVERIFY(restored.undo());
    QCOMPARE(restored.layers()[0].name, QStringLiteral("First"));
    for (int i = 0; i < 24; ++i) QVERIFY(restored.redo());
    QCOMPARE(restored.layers()[0].name, QStringLiteral("11"));
    QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuLayerDocumentRecoveryTest)
#include "KisGpuLayerDocumentRecoveryTest.moc"
