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

/** Raster edits and layer changes share a document-wide observable history. */
class KisGpuLayerDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void differentLayersShareOneUndoSequence();
    void layerStructureAndPropertiesUseTheSameHistory();
    void previewCancellationPreservesRedoAndRejectsOldInput();
    void masksHaveIndependentPixelsAndSharedDocumentHistory();
    void cpuBudgetRefusesInputBeforeGpuWork();
    void historyLimitReleasesDiscardedGpuPixels();
    void closingDocumentInvalidatesInputAndLeaseKeepsStoreAlive();
    void deviceLossPreservesTheCompletedDocument();
    void undoKeepsTheRecoverySourceForEachLayerAndMask();
};

void KisGpuLayerDocumentTest::differentLayersShareOneUndoSequence()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 8);
    const auto bottom = document.addLayer(QStringLiteral("Bottom"));
    const auto top = document.addLayer(QStringLiteral("Top"));
    QCOMPARE(bottom.result, Document::Result::Accepted);
    QCOMPARE(top.result, Document::Result::Accepted);
    for (const auto id : {bottom.id, top.id}) {
        const auto edit = document.begin(id);
        QCOMPARE(edit.result, Document::Result::Accepted);
        QCOMPARE(edit.session->append(edit.token, {dab({0, 0}, id == bottom.id ? 0xFF0000FF : 0xFFFF0000)}, Bounds),
                 Session::Result::Accepted);
        QCOMPARE(document.commit(edit.token), Document::Result::Accepted);
        wgpuDevicePoll(gpu.device, true, nullptr);
        document.poll();
    }
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.read(document.layers()[1].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("0000ffff"));
    const auto beforeUndo = store->statistics();
    QVERIFY(document.undo());
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(document.layers()[1].paint.pixels.tileCount(), qsizetype(0));
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[0].paint.pixels.tileCount(), qsizetype(0));
    QCOMPARE(store->statistics().submissions, beforeUndo.submissions);
    QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::layerStructureAndPropertiesUseTheSameHistory()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 16 * Store::TileBytes);
    Document document(store, 16);
    const auto bottom = document.addLayer(QStringLiteral("Bottom"));
    const auto top = document.addLayer(QStringLiteral("Top"));
    const auto middle = document.addLayer(QStringLiteral("Middle"), 1);
    QCOMPARE(document.layers()[0].id, bottom.id);
    QCOMPARE(document.layers()[1].id, middle.id);
    QCOMPARE(document.layers()[2].id, top.id);
    QCOMPARE(document.moveLayer(middle.id, 2), Document::Result::Accepted);
    QCOMPARE(document.layers()[2].id, middle.id);
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[1].id, middle.id);
    QCOMPARE(document.renameLayer(top.id, QStringLiteral("Foreground")), Document::Result::Accepted);
    QCOMPARE(document.setLayerProperties(top.id, 77, Store::CompositeOp::Erase), Document::Result::Accepted);
    QCOMPARE(document.layers()[2].paint.opacity, quint8(77));
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[2].name, QStringLiteral("Foreground"));
    QCOMPARE(document.layers()[2].paint.opacity, quint8(255));
    QCOMPARE(document.layers()[2].paint.operation, Store::CompositeOp::Over);
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[2].name, QStringLiteral("Top"));
    QCOMPARE(document.removeLayer(middle.id), Document::Result::Accepted);
    QCOMPARE(document.layers().size(), qsizetype(2));
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[1].id, middle.id);
    const auto size = document.historySize();
    QCOMPARE(document.moveLayer(middle.id, 1), Document::Result::Accepted);
    QCOMPARE(document.renameLayer(top.id, QStringLiteral("Top")), Document::Result::Accepted);
    QCOMPARE(document.setLayerProperties(top.id, 255, Store::CompositeOp::Over), Document::Result::Accepted);
    QCOMPARE(document.clearMask(top.id), Document::Result::Accepted);
    QCOMPARE(document.addLayer(QStringLiteral("Bad"), -2).result, Document::Result::InvalidInput);
    QCOMPARE(document.removeLayer(0), Document::Result::InvalidLayer);
    QCOMPARE(document.moveLayer(top.id, 3), Document::Result::InvalidInput);
    QCOMPARE(document.setLayerProperties(top.id, 255, Store::CompositeOp(99)), Document::Result::InvalidInput);
    QCOMPARE(document.historySize(), size);
    QVERIFY(document.redo());
    const auto replacement = document.addLayer(QStringLiteral("Replacement"));
    QVERIFY(replacement.id > middle.id);
    QCOMPARE(store->statistics().submissions, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::previewCancellationPreservesRedoAndRejectsOldInput()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 8), otherDocument(store, 8);
    const auto layer = document.addLayer(QStringLiteral("Paint"));
    auto first = document.begin(layer.id);
    QCOMPARE(first.session->append(first.token, {dab({0, 0}, 0xFF0000FF)}, Bounds), Session::Result::Accepted);
    document.commit(first.token);
    QVERIFY(finish(gpu, document));
    const auto committed = document.layers()[0].paint.pixels;
    const auto secondLayer = document.addLayer(QStringLiteral("Later"));
    QVERIFY(document.undo());
    const auto beforeBegin = store->statistics();
    auto working = document.begin(layer.id);
    QCOMPARE(working.result, Document::Result::Accepted);
    QCOMPARE(store->statistics().submissions, beforeBegin.submissions);
    QCOMPARE(working.session->append(working.token, {dab({8, 0}, 0xFF00FF00)}, Bounds), Session::Result::Accepted);
    wgpuDevicePoll(gpu.device, true, nullptr);
    document.poll();
    QVERIFY(document.layers()[0].paint.pixels == committed);
    QVERIFY(!(document.projectionLayers()[0].pixels == committed));
    QVERIFY(document.projectionLayers(false)[0].pixels == committed);
    QCOMPARE(document.removeLayer(layer.id), Document::Result::Busy);
    QVERIFY(!document.undo());
    const auto otherLayer = otherDocument.addLayer(QStringLiteral("Other"));
    const auto foreign = otherDocument.begin(otherLayer.id);
    QCOMPARE(document.commit(foreign.token), Document::Result::Stale);
    QCOMPARE(document.cancel(working.token), Document::Result::Accepted);
    QCOMPARE(working.session->append(working.token, {dab({0, 0}, 0xFFFF0000)}, Bounds), Session::Result::Stale);
    QVERIFY(document.redo());
    QCOMPARE(document.layers()[1].id, secondLayer.id);
    QVERIFY(document.undo());
    const auto replacement = document.begin(layer.id);
    QCOMPARE(replacement.session->append(working.token, {}, Bounds), Session::Result::Stale);
    replacement.session->append(replacement.token, {dab({8, 0}, 0xFF00FF00)}, Bounds);
    document.commit(replacement.token);
    QVERIFY(finish(gpu, document));
    QVERIFY(!document.redo());
    QCOMPARE(document.layers().size(), qsizetype(1));
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, {8, 0, 1, 1}), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::masksHaveIndependentPixelsAndSharedDocumentHistory()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 8);
    const auto layer = document.addLayer(QStringLiteral("Paint"));
    auto paint = document.begin(layer.id);
    paint.session->append(paint.token, {dab({0, 0}, 0xFF0000FF)}, Bounds);
    document.commit(paint.token);
    QVERIFY(finish(gpu, document));
    const auto committed = document.layers()[0].paint;
    auto mask = document.begin(layer.id, Document::Target::Mask);
    QCOMPARE(mask.result, Document::Result::Accepted);
    QVERIFY(document.projectionLayers()[0].mask == committed.mask);
    auto point = dab({0, 0}, 0xFFFFFFFF);
    point.diameter = QSizeF(.5, .5);
    mask.session->append(mask.token, {point}, Bounds);
    wgpuDevicePoll(gpu.device, true, nullptr);
    document.poll();
    QVERIFY(document.layers()[0].paint.mask == committed.mask);
    QCOMPARE(gpu.read(document.projectionLayers()[0].mask, {0, 0, 1, 1}), QByteArray::fromHex("ffffffff"));
    QCOMPARE(gpu.read(document.projectionLayers()[0].mask, {1, 0, 1, 1}), QByteArray(4, '\0'));
    document.cancel(mask.token);
    QVERIFY(document.projectionLayers()[0].mask == committed.mask);
    mask = document.begin(layer.id, Document::Target::Mask);
    mask.session->append(mask.token, {point}, Bounds);
    document.commit(mask.token);
    QVERIFY(finish(gpu, document));
    const auto maskVersion = document.layers()[0].paint.mask;
    QCOMPARE(gpu.read(maskVersion, {0, 0, 1, 1}), QByteArray::fromHex("ffffffff"));
    QVERIFY(document.layers()[0].paint.pixels == committed.pixels);
    QVERIFY(document.undo());
    QVERIFY(document.layers()[0].paint.mask == committed.mask);
    QVERIFY(document.redo());
    QVERIFY(document.layers()[0].paint.mask == maskVersion);
    QCOMPARE(document.clearMask(layer.id), Document::Result::Accepted);
    QVERIFY(document.layers()[0].paint.mask == committed.mask);
    QVERIFY(document.undo());
    QVERIFY(document.layers()[0].paint.mask == maskVersion);
    const auto projected = store->project(store->emptyVersion(), document.projectionLayers(), Bounds);
    QCOMPARE(gpu.read(projected.version, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(gpu.read(projected.version, {1, 0, 1, 1}), QByteArray(4, '\0'));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::cpuBudgetRefusesInputBeforeGpuWork()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Document(store, 8, 0));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Document({}, 8));
    Document document(store, 8, 4096);
    QCOMPARE(document.addLayer(QString(4096, QLatin1Char('x'))).result, Document::Result::RecoveryBudgetExceeded);
    QVERIFY(document.layers().isEmpty());
    const auto layer = document.addLayer(QStringLiteral("Paint"));
    const auto edit = document.begin(layer.id);
    QCOMPARE(edit.result, Document::Result::Accepted);
    const auto before = store->statistics();
    const auto bytes = document.recoveryBytes();
    QCOMPARE(edit.session->append(edit.token, QVector<Store::DabCommand>(10000, dab({0, 0}, 0xFF0000FF)), Bounds),
             Session::Result::RecoveryBudgetExceeded);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().residentBytes, before.residentBytes);
    QCOMPARE(document.recoveryBytes(), bytes);
    QCOMPARE(edit.session->append(edit.token, {dab({0, 0}, 0xFF0000FF)}, Bounds), Session::Result::Accepted);
    QVERIFY(document.recoveryBytes() <= 4096);
    document.clearHistory();
    QCOMPARE(document.historySize(), qsizetype(1));
    QCOMPARE(document.state(), Document::State::Editing);
    document.commit(edit.token);
    QVERIFY(finish(gpu, document));
    QCOMPARE(document.historySize(), qsizetype(2));
    QVERIFY(document.undo());
    QCOMPARE(document.layers()[0].paint.pixels.tileCount(), qsizetype(0));
    QVERIFY(document.redo());
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
    QVERIFY(document.recoveryBytes() <= 4096);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::historyLimitReleasesDiscardedGpuPixels()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 0);
    const auto layer = document.addLayer(QStringLiteral("Paint"));
    for (int i = 0; i < 8; ++i) {
        const auto edit = document.begin(layer.id);
        QCOMPARE(edit.result, Document::Result::Accepted);
        // Keep the footprint inside one tile so obsolete versions consume extra bytes.
        auto point = dab({8, 8}, 0xFF000000 | quint32(i));
        point.diameter = QSizeF(.5, .5);
        edit.session->append(edit.token, {point}, Bounds);
        document.commit(edit.token);
        QVERIFY(finish(gpu, document));
    }
    QCOMPARE(document.historySize(), qsizetype(1));
    QVERIFY(!document.undo());
    QCOMPARE(store->statistics().residentBytes, Store::TileBytes);
    QCOMPARE(document.removeLayer(layer.id), Document::Result::Accepted);
    QCOMPARE(store->statistics().residentBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::closingDocumentInvalidatesInputAndLeaseKeepsStoreAlive()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    std::weak_ptr<Store> storeLifetime = store;
    auto document = std::make_unique<Document>(store, 2);
    const auto layer = document->addLayer(QStringLiteral("Paint"));
    auto edit = document->begin(layer.id);
    edit.session->append(edit.token, {dab({0, 0}, 0xFF0000FF)}, Bounds);
    store.reset();
    document.reset();
    QVERIFY(!storeLifetime.expired());
    QCOMPARE(edit.session->append(edit.token, {}, Bounds), Session::Result::Stale);
    edit.session.reset();
    QVERIFY(storeLifetime.expired());
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::deviceLossPreservesTheCompletedDocument()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 8);
    const auto layer = document.addLayer(QStringLiteral("Paint"));
    auto first = document.begin(layer.id);
    first.session->append(first.token, {dab({0, 0}, 0xFF0000FF)}, Bounds);
    document.commit(first.token);
    QVERIFY(finish(gpu, document));
    const auto committed = document.layers()[0].paint.pixels;
    auto second = document.begin(layer.id);
    second.session->append(second.token, {dab({8, 0}, 0xFF00FF00)}, Bounds);
    document.commit(second.token);
    gpu.owner.destroy();
    document.poll();
    QCOMPARE(document.state(), Document::State::Failed);
    QVERIFY(document.layers()[0].paint.pixels == committed);
    QCOMPARE(document.begin(layer.id).result, Document::Result::GpuRejected);
    QVERIFY(!document.undo());
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuLayerDocumentTest::undoKeepsTheRecoverySourceForEachLayerAndMask()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document document(store, 8);
    const auto bottom = document.addLayer(QStringLiteral("Bottom"));
    const auto top = document.addLayer(QStringLiteral("Top"));
    for (const auto id : {bottom.id, top.id}) {
        const auto edit = document.begin(id);
        edit.session->append(edit.token, {dab({0, 0}, id == bottom.id ? 0xFF0000FF : 0xFFFF0000)}, Bounds);
        document.commit(edit.token);
        QVERIFY(finish(gpu, document));
    }
    {
        const auto edit = document.begin(bottom.id, Document::Target::Mask);
        edit.session->append(edit.token, {dab({0, 0}, 0x77123456)}, Bounds);
        document.commit(edit.token);
        QVERIFY(finish(gpu, document));
    }
    {
        const auto edit = document.begin(bottom.id);
        edit.session->append(edit.token, {dab({8, 0}, 0xFF00FF00)}, Bounds);
        document.commit(edit.token);
        QVERIFY(finish(gpu, document));
    }
    QVERIFY(document.undo());
    const auto expectedLayers = document.layers();
    KisGpuTestDevice replacement;
    Store restoredStore(replacement.owner, 128 * Store::TileBytes);
    for (const auto target : {Document::Target::Pixels, Document::Target::Mask}) {
        for (const auto &layer : expectedLayers) {
            if (target == Document::Target::Mask && layer.id == top.id) continue;
            const auto before = store->statistics();
            auto edit = document.begin(layer.id, target);
            QCOMPARE(edit.result, Document::Result::Accepted);
            QCOMPARE(store->statistics().submissions, before.submissions);
            document.cancel(edit.token);
            const auto recovery = edit.session->recovery();
            QVERIFY(recovery.has_value());
            Session restored(restoredStore, 0, *recovery);
            QElapsedTimer timer;
            timer.start();
            while (restored.state() == Session::State::Restoring && timer.elapsed() < 5000) {
                restored.poll();
                QTest::qWait(1);
            }
            QCOMPARE(restored.state(), Session::State::Idle);
            const auto expected = target == Document::Target::Pixels ? layer.paint.pixels : layer.paint.mask;
            QCOMPARE(replacement.read(restored.head(), Bounds), gpu.read(expected, Bounds));
        }
    }
    QVERIFY(document.redo());
    QCOMPARE(gpu.read(document.layers()[0].paint.pixels, {8, 0, 1, 1}), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(replacement.owner.errorCount(), 0);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuLayerDocumentTest)
#include "KisGpuLayerDocumentTest.moc"
