/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentReadback.h"
#include "KisGpuTestDevice.h"
#include <QtTest>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Readback = KisGpuDocumentReadback;
const QRect Clip(-640064, 0, 1280128, 64);
bool settle(Document &document)
{
    QElapsedTimer timer; timer.start();
    while ((document.state() == Document::State::Restoring || document.state() == Document::State::Committing)
           && timer.elapsed() < 5000) { document.poll(); QTest::qWait(1); }
    return document.state() == Document::State::Idle;
}
bool paint(Document &document, Document::LayerId id, quint32 rgba, Document::Target target = Document::Target::Pixels)
{
    const auto editing = document.begin(id, target);
    if (editing.result != Document::Result::Accepted) return false;
    Store::DabCommand command;
    command.center = {-640032, 24}; command.diameter = {12, 12}; command.rgba = rgba;
    auto second = command; second.center.setX(640032);
    return editing.session->append(editing.token, {command, second}, Clip) == Session::Result::Accepted
        && document.commit(editing.token) == Document::Result::Accepted && settle(document);
}
bool finish(Readback &readback)
{
    QElapsedTimer timer; timer.start();
    while (readback.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        readback.poll(); QTest::qWait(1);
    }
    return readback.status() == Store::Status::Succeeded;
}
QByteArray pixels(KisGpuTestDevice &gpu, const Store::Version &version)
{
    return gpu.read(version, {-640064, 0, 64, 64}) + gpu.read(version, {640000, 0, 64, 64});
}
}

/** Native-save consumers receive complete CPU pixels from the requested immutable versions. */
class KisGpuDocumentReadbackTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void frozenLayersSurviveEditingAndDocumentClose();
    void sharedVersionsAreReadOnce();
    void cpuAndInputRefusalsPrecedeTransfers();
    void queuePressureRetriesAndGpuRefusalCanRestart();
    void partialLossPublishesNoResult();
    void emptyDocumentCompletesWithoutGpuWork();
};

void KisGpuDocumentReadbackTest::frozenLayersSurviveEditingAndDocumentClose()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes, 4);
    const std::weak_ptr<Store> weakStore = store;
    auto document = std::make_unique<Document>(store, 8);
    const auto first = document->addLayer(QStringLiteral("Paint 雪")).id;
    const auto second = document->addLayer(QStringLiteral("Top")).id;
    QVERIFY(paint(*document, first, 0xCC112233));
    QVERIFY(paint(*document, first, 0x80123456, Document::Target::Mask));
    QVERIFY(paint(*document, second, 0xFF778899));
    document->setLayerProperties(first, 77, Store::CompositeOp::Erase);
    document->setLayerVisible(second, false);
    document->setLayerLocked(second, true);
    auto layers = document->layers();
    const auto expectedFirst = pixels(gpu, layers[0].paint.pixels);
    const auto expectedMask = pixels(gpu, layers[0].paint.mask);
    const auto expectedSecond = pixels(gpu, layers[1].paint.pixels);
    const auto nextId = document->recovery().nextId;
    const auto before = store->statistics();
    auto job = std::make_unique<Readback>(store, layers, nextId);
    QCOMPARE(job->status(), Store::Status::Pending);
    QVERIFY(!job->result());
    job->poll();
    QVERIFY(!job->result());
    QVERIFY(paint(*document, first, 0xFFABCDEF));
    document->renameLayer(first, QStringLiteral("Changed"));
    document->removeLayer(second);
    document.reset();
    layers.clear();
    store.reset();
    QVERIFY(!weakStore.expired());
    quint64 readBytes = weakStore.lock()->statistics().pixelReadbackBytes;
    QElapsedTimer timer; timer.start();
    while (job->status() == Store::Status::Pending && timer.elapsed() < 5000) {
        job->poll();
        const auto current = weakStore.lock()->statistics().pixelReadbackBytes;
        QVERIFY(current - readBytes <= Store::TileBytes);
        readBytes = current;
        if (job->status() == Store::Status::Pending) QVERIFY(!job->result());
        QTest::qWait(1);
    }
    QCOMPARE(job->status(), Store::Status::Succeeded);
    QCOMPARE(job->error(), Readback::Error::None);
    QCOMPARE(readBytes - before.pixelReadbackBytes, 6 * Store::TileBytes);
    QCOMPARE(weakStore.lock()->statistics().pixelUploadBytes, before.pixelUploadBytes);
    const auto saved = job->result();
    QVERIFY(saved);
    QCOMPARE(saved->history.size(), 1);
    QCOMPARE(saved->cursor, qsizetype(0));
    QCOMPARE(saved->nextId, nextId);
    QVERIFY(!saved->working);
    QVERIFY(Document::validRecovery(*saved, 0, job->reservedRecoveryBytes()));
    job.reset();
    QVERIFY(weakStore.expired());
    gpu.owner.destroy();
    KisGpuTestDevice replacement;
    auto restoredStore = std::make_shared<Store>(replacement.owner, 64 * Store::TileBytes);
    Document restored(restoredStore, 0, *saved);
    QVERIFY(settle(restored));
    const auto actual = restored.layers();
    QCOMPARE(actual.size(), 2);
    QCOMPARE(actual[0].id, first);
    QCOMPARE(actual[0].name, QStringLiteral("Paint 雪"));
    QCOMPARE(actual[0].paint.opacity, quint8(77));
    QCOMPARE(actual[0].paint.operation, Store::CompositeOp::Erase);
    QCOMPARE(actual[1].id, second);
    QCOMPARE(actual[1].name, QStringLiteral("Top"));
    QVERIFY(!actual[1].visible);
    QVERIFY(actual[1].locked);
    QCOMPARE(restored.projectionLayers().size(), 1);
    QCOMPARE(restored.begin(second).result, Document::Result::LayerNotEditable);
    QVERIFY(actual[1].paint.mask == Store::Version{});
    QCOMPARE(pixels(replacement, actual[0].paint.pixels), expectedFirst);
    QCOMPARE(pixels(replacement, actual[0].paint.mask), expectedMask);
    QCOMPARE(pixels(replacement, actual[1].paint.pixels), expectedSecond);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

void KisGpuDocumentReadbackTest::sharedVersionsAreReadOnce()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    const auto id = document.addLayer(QStringLiteral("Paint")).id;
    QVERIFY(paint(document, id, 0xFF112233));
    auto layers = document.layers();
    auto alias = layers[0]; alias.id = 2; alias.name = QStringLiteral("Alias");
    alias.paint.mask = alias.paint.pixels;
    layers.push_back(alias);
    const auto before = store->statistics();
    Readback job(store, layers, 3);
    QVERIFY(finish(job));
    QCOMPARE(store->statistics().pixelReadbackBytes - before.pixelReadbackBytes, 2 * Store::TileBytes);
    const auto frame = job.result()->history[0];
    QVERIFY(frame[0]->pixels == frame[1]->pixels);
    QVERIFY(frame[1]->pixels == frame[1]->mask);
    QVERIFY(frame[0]->pixels->edits.isEmpty());
    QCOMPARE(frame[0]->pixels->tiles.size(), 2);
    const auto submitted = store->statistics().submissions;
    gpu.owner.destroy();
    job.poll();
    QCOMPARE(job.status(), Store::Status::Succeeded);
    QCOMPARE(store->statistics().submissions, submitted);
    QVERIFY(job.result());
}

void KisGpuDocumentReadbackTest::cpuAndInputRefusalsPrecedeTransfers()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    const auto before = store->statistics();
    Readback small(store, document.layers(), 2, Store::TileBytes - 1);
    QCOMPARE(small.status(), Store::Status::Failed);
    QCOMPARE(small.error(), Readback::Error::BudgetExceeded);
    QVERIFY(!small.result());
    QCOMPARE(small.reservedRecoveryBytes(), quint64(0));
    auto invalid = document.layers();
    invalid.push_back(invalid[0]);
    Readback duplicate(store, invalid, 2);
    QCOMPARE(duplicate.error(), Readback::Error::InvalidDocument);
    Readback reusedId(store, document.layers(), 1);
    QCOMPARE(reusedId.error(), Readback::Error::InvalidDocument);
    auto foreignStore = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Readback foreign(foreignStore, document.layers(), 2);
    QCOMPARE(foreign.error(), Readback::Error::InvalidVersion);
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuDocumentReadbackTest::queuePressureRetriesAndGpuRefusalCanRestart()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes, 1);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    Readback refused(store, document.layers(), 2);
    refused.poll();
    QCOMPARE(refused.status(), Store::Status::Failed);
    QCOMPARE(refused.error(), Readback::Error::BudgetExceeded);
    QVERIFY(!refused.result());
    QCOMPARE(refused.reservedRecoveryBytes(), quint64(0));
    occupied = {};
    Readback first(store, document.layers(), 2);
    Readback second(store, document.layers(), 2);
    first.poll(); second.poll();
    QElapsedTimer timer; timer.start();
    while ((first.status() == Store::Status::Pending || second.status() == Store::Status::Pending)
           && timer.elapsed() < 5000) { first.poll(); second.poll(); QTest::qWait(1); }
    QCOMPARE(first.status(), Store::Status::Succeeded);
    QCOMPARE(second.status(), Store::Status::Succeeded);
    QCOMPARE(first.result()->history[0][0]->pixels->tiles[0].pixels, second.result()->history[0][0]->pixels->tiles[0].pixels);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuDocumentReadbackTest::partialLossPublishesNoResult()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document document(store, 2);
    document.addLayer(QStringLiteral("Paint"));
    QVERIFY(paint(document, 1, 0xFF112233));
    Readback job(store, document.layers(), 2);
    const auto before = store->statistics().pixelReadbackBytes;
    QElapsedTimer timer; timer.start();
    while (store->statistics().pixelReadbackBytes < before + 2 * Store::TileBytes && timer.elapsed() < 5000) {
        job.poll(); QTest::qWait(1);
    }
    QCOMPARE(store->statistics().pixelReadbackBytes, before + 2 * Store::TileBytes);
    QCOMPARE(job.status(), Store::Status::Pending);
    QVERIFY(!job.result());
    gpu.owner.destroy();
    job.poll();
    QCOMPARE(job.status(), Store::Status::Failed);
    QCOMPARE(job.error(), Readback::Error::DeviceLost);
    QVERIFY(!job.result());
    QCOMPARE(job.reservedRecoveryBytes(), quint64(0));
}

void KisGpuDocumentReadbackTest::emptyDocumentCompletesWithoutGpuWork()
{
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    const auto before = store->statistics();
    Readback job(store, {}, 1);
    QVERIFY(finish(job));
    QVERIFY(job.result()->history[0].isEmpty());
    QVERIFY(Document::validRecovery(*job.result(), 0, job.reservedRecoveryBytes()));
    QCOMPARE(store->statistics().submissions, before.submissions);
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
}

QTEST_GUILESS_MAIN(KisGpuDocumentReadbackTest)
#include "KisGpuDocumentReadbackTest.moc"
