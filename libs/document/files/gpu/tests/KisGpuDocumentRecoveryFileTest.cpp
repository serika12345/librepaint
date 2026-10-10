/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentRecoveryFile.h"
#include "KisGpuDocumentReadback.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <future>
#include <QCryptographicHash>
#include <QtEndian>
#include <limits>

namespace {
using File = KisGpuDocumentRecoveryFile;
using Document = KisGpuLayerDocument;
using Store = KisGpuTileStore;
using Session = KisGpuEditSession;
const QRect Bounds(-16, -16, 32, 32);
bool settle(Document &document)
{
    QElapsedTimer timer;
    timer.start();
    while ((document.state() == Document::State::Restoring || document.state() == Document::State::Committing)
           && timer.elapsed() < 5000) {
        document.poll();
        QTest::qWait(1);
    }
    return document.state() != Document::State::Failed;
}
}

/** Persisted CPU values preserve the selected document version independently of GPU lifetime. */
class KisGpuDocumentRecoveryFileTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void emptyDocumentHasStableVersionedBytes();
    void backgroundSnapshotRestoresUnfinishedEditing_data();
    void backgroundSnapshotRestoresUnfinishedEditing();
    void versionOneFilePreservesRecordedImages();
    void frozenGpuPixelsPersistWithoutReplayCommands();
    void damagedFilesAndOversizedCountsAreRejected();
    void failedWritesPreserveThePreviousFile();
    void sharedCheckpointHistoryFitsTheOriginalGpuBudget();
    void successfulWriteFitsTheReadBudget();
};

void KisGpuDocumentRecoveryFileTest::emptyDocumentHasStableVersionedBytes()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    Document::Recovery saved;
    saved.history = {{}};
    QCOMPARE(File::write(path, saved), File::Error::None);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto actual = file.readAll();
    // Version 1: empty asset/raster/layer tables and one empty history frame.
    const auto payload = QByteArray::fromHex(
        "000000000000000000000000000000000000000100000000"
        "0000000000000000000000000000000100");
    QByteArray expected("LPGPUREC", 8);
    expected += QByteArray::fromHex("000000010000000000000029");
    expected += QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
    expected += payload;
    QCOMPARE(actual, expected);
    const auto loaded = File::read(path);
    QCOMPARE(loaded.error, File::Error::None);
    QVERIFY(loaded.recovery.has_value());
    QCOMPARE(loaded.recovery->history.size(), 1);
    QVERIFY(loaded.recovery->history[0].isEmpty());
    QCOMPARE(loaded.recovery->nextId, quint64(1));
}

void KisGpuDocumentRecoveryFileTest::backgroundSnapshotRestoresUnfinishedEditing_data()
{
    QTest::addColumn<int>("phase");
    QTest::addColumn<bool>("mask");
    for (int phase = 0; phase < 3; ++phase) for (bool mask : {false, true})
        QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(phase).arg(mask ? "mask" : "pixels"))) << phase << mask;
}

void KisGpuDocumentRecoveryFileTest::backgroundSnapshotRestoresUnfinishedEditing()
{
    QFETCH(int, phase);
    QFETCH(bool, mask);
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    QByteArray expectedHead, expectedPreview;
    Session::Token oldToken;
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        Document document(store, 16);
        const auto id = document.addLayer(QStringLiteral("雪のレイヤー")).id;
        Store::DabCommand command;
        command.center = {0, 0};
        command.diameter = {13, 11};
        command.fade = {.7, .8};
        command.rgba = 0xCC552211;
        auto edit = document.begin(id);
        QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::Accepted);
        document.commit(edit.token);
        QVERIFY(settle(document));
        edit = document.begin(id, Document::Target::Mask);
        QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::Accepted);
        document.commit(edit.token);
        QVERIFY(settle(document));
        document.renameLayer(id, QStringLiteral("Future"));
        QVERIFY(document.undo());
        edit = document.begin(id, mask ? Document::Target::Mask : Document::Target::Pixels);
        const auto texture = edit.session->createTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3});
        command.rgba = 0x80000000;
        const auto selection = edit.session->createSelection({command}, Bounds);
        command.center = {3, -2};
        command.rgba = 0xFF3399CC;
        command.textureStrength = 77;
        QCOMPARE(edit.session->replace(edit.token, {command}, Bounds, texture.texture, selection.selection), Session::Result::Accepted);
        wgpuDevicePoll(gpu.device, true, nullptr);
        document.poll();
        expectedHead = gpu.read(edit.session->head(), Bounds);
        expectedPreview = gpu.read(edit.session->preview(), Bounds);
        QVERIFY(expectedPreview != expectedHead);
        oldToken = edit.token;
        if (phase) {
            document.commit(edit.token);
            if (phase == 2) edit.session->poll();
        }
        const auto before = store->statistics();
        const auto saved = document.recovery();
        auto writing = std::async(std::launch::async, [path, saved] { return File::write(path, saved); });
        document.cancel(edit.token);
        document.renameLayer(id, QStringLiteral("Changed after snapshot"));
        QCOMPARE(writing.get(), File::Error::None);
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(store->statistics().pixelUploadBytes, before.pixelUploadBytes);
        QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    }
    const auto loaded = File::read(path);
    QCOMPARE(loaded.error, File::Error::None);
    QVERIFY(loaded.recovery.has_value());
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes, 1);
    Document restored(store, 16, *loaded.recovery);
    QVERIFY(settle(restored));
    QCOMPARE(restored.layers()[0].name, QStringLiteral("雪のレイヤー"));
    QCOMPARE(restored.commit(oldToken), Document::Result::Stale);
    if (!phase) {
        QCOMPARE(restored.state(), Document::State::Editing);
        const auto edit = restored.currentEditing();
        QCOMPARE(gpu.read(edit.session->head(), Bounds), expectedHead);
        QCOMPARE(gpu.read(edit.session->preview(), Bounds), expectedPreview);
        QCOMPARE(restored.cancel(edit.token), Document::Result::Accepted);
        QVERIFY(restored.redo());
        QCOMPARE(restored.layers()[0].name, QStringLiteral("Future"));
    } else {
        QCOMPARE(restored.state(), Document::State::Idle);
        QCOMPARE(gpu.read(mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels, Bounds), expectedPreview);
        QVERIFY(!restored.redo());
        QVERIFY(restored.undo());
        QCOMPARE(gpu.read(mask ? restored.layers()[0].paint.mask : restored.layers()[0].paint.pixels, Bounds), expectedHead);
    }
    QCOMPARE(gpu.owner.errorCount(), 0);
}


void KisGpuDocumentRecoveryFileTest::frozenGpuPixelsPersistWithoutReplayCommands()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("snapshot"));
    QByteArray expectedPixels, expectedMask;
    {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
        Document document(store, 8);
        const auto id = document.addLayer(QStringLiteral("Frozen")).id;
        Store::DabCommand command;
        command.center = {0, 0}; command.diameter = {13, 11}; command.rgba = 0xCC552211;
        for (const auto target : {Document::Target::Pixels, Document::Target::Mask}) {
            const auto edit = document.begin(id, target);
            QCOMPARE(edit.session->append(edit.token, {command}, Bounds), Session::Result::Accepted);
            document.commit(edit.token);
            QVERIFY(settle(document));
        }
        expectedPixels = gpu.read(document.layers()[0].paint.pixels, Bounds);
        expectedMask = gpu.read(document.layers()[0].paint.mask, Bounds);
        KisGpuDocumentReadback readback(store, document.layers(), document.recovery().nextId);
        QElapsedTimer timer; timer.start();
        while (readback.status() == Store::Status::Pending && timer.elapsed() < 5000) {
            readback.poll(); QTest::qWait(1);
        }
        QCOMPARE(readback.status(), Store::Status::Succeeded);
        const auto saved = *readback.result();
        QVERIFY(saved.history[0][0]->pixels->edits.isEmpty());
        QVERIFY(saved.history[0][0]->mask->edits.isEmpty());
        const auto before = store->statistics();
        auto writing = std::async(std::launch::async, [path, saved] { return File::write(path, saved); });
        document.renameLayer(id, QStringLiteral("Changed"));
        document.removeLayer(id);
        QCOMPARE(writing.get(), File::Error::None);
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    }
    const auto loaded = File::read(path);
    QCOMPARE(loaded.error, File::Error::None);
    QVERIFY(loaded.recovery);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 64 * Store::TileBytes);
    Document restored(store, 0, *loaded.recovery);
    QVERIFY(settle(restored));
    QCOMPARE(restored.layers()[0].name, QStringLiteral("Frozen"));
    QCOMPARE(gpu.read(restored.layers()[0].paint.pixels, Bounds), expectedPixels);
    QCOMPARE(gpu.read(restored.layers()[0].paint.mask, Bounds), expectedMask);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuDocumentRecoveryFileTest::damagedFilesAndOversizedCountsAreRejected()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    Document::Recovery saved;
    saved.history = {{}};
    QCOMPARE(File::write(path, saved), File::Error::None);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto good = file.readAll();
    file.close();
    const auto reject = [&](const QByteArray &bytes, File::Error expected = File::Error::InvalidData) {
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(bytes), bytes.size());
        file.close();
        const auto loaded = File::read(path);
        QCOMPARE(loaded.error, expected);
        QVERIFY(!loaded.recovery);
    };
    auto bad = good; bad[0] = '!'; reject(bad);
    bad = good; bad[11] = 2; reject(bad, File::Error::UnsupportedVersion);
    bad = good; bad[19] = 40; reject(bad);
    bad = good; bad[20] ^= 1; reject(bad);
    for (const auto length : {0, 8, 51, 52, int(good.size() - 1)}) reject(good.first(length));
    const auto changedPayload = [&](QByteArray payload) {
        QByteArray bytes = good.first(12);
        char length[8]; qToBigEndian(quint64(payload.size()), length);
        bytes.append(length, 8);
        bytes += QCryptographicHash::hash(payload, QCryptographicHash::Sha256);
        bytes += payload;
        return bytes;
    };
    for (const auto offset : {0, 4, 8, 12, 16, 20}) {
        auto payload = good.sliced(52);
        payload.replace(offset, 4, QByteArray(4, char(0xFF)));
        reject(changedPayload(payload));
    }
    reject(changedPayload(good.sliced(52) + 'x'));
    QCOMPARE(File::read(directory.filePath(QStringLiteral("absent"))).error, File::Error::Io);
}

void KisGpuDocumentRecoveryFileTest::failedWritesPreserveThePreviousFile()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    Document::Recovery saved;
    saved.history = {{}};
    QCOMPARE(File::write(path, saved), File::Error::None);
    const auto contents = [&] {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return QByteArray{};
        return file.readAll();
    };
    const auto good = contents();
    QCOMPARE(File::write(path, saved, 40), File::Error::BudgetExceeded);
    QCOMPARE(contents(), good);
    QCOMPARE(File::read(path, 40).error, File::Error::BudgetExceeded);
    QCOMPARE(File::write(directory.path(), saved), File::Error::Io);
    QCOMPARE(File::write(path, Document::Recovery{}), File::Error::InvalidData);
    QCOMPARE(contents(), good);
    auto raster = std::make_shared<Session::Recovery>();
    Store::DabCommand command;
    command.center = {std::numeric_limits<double>::quiet_NaN(), 0};
    raster->edits = {{{{command}, Bounds, {}, {}}}};
    raster->cursor = raster->firstRetained = 1;
    auto layer = std::make_shared<Document::RecoveryLayer>();
    layer->id = 1; layer->pixels = raster;
    saved.history = {{layer}}; saved.nextId = 2;
    QCOMPARE(File::write(path, saved), File::Error::InvalidData);
    QCOMPARE(contents(), good);
    raster->edits[0][0].commands[0].center = {0, 0};
    auto texture = std::make_shared<Session::ReplayTexture>();
    texture->size = {3, 2}; texture->alpha = QByteArray(1, '\0');
    raster->edits[0][0].texture = texture;
    QCOMPARE(File::write(path, saved), File::Error::InvalidData);
    QCOMPARE(contents(), good);
}

void KisGpuDocumentRecoveryFileTest::sharedCheckpointHistoryFitsTheOriginalGpuBudget()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    auto pixels = std::make_shared<Session::Recovery>();
    pixels->bounds = {0, 0, 64, 64};
    pixels->pixels = QByteArray(Store::TileBytes, '\0');
    pixels->pixels.replace(0, 4, QByteArray::fromHex("11223300"));
    auto mask = std::make_shared<Session::Recovery>();
    mask->tiles.push_back({{0, 0}, QByteArray(Store::TileBytes, char(0xFF))});
    Document::Recovery saved;
    saved.nextId = 2;
    for (int i = 0; i < 24; ++i) {
        auto layer = std::make_shared<Document::RecoveryLayer>();
        layer->id = 1; layer->name = QString::number(i);
        layer->opacity = 64 + i; layer->operation = Store::CompositeOp::Erase;
        layer->pixels = pixels; layer->mask = mask;
        saved.history.push_back({layer});
    }
    saved.cursor = 23;
    QCOMPARE(File::write(path, saved), File::Error::None);
    const auto loaded = File::read(path);
    QCOMPARE(loaded.error, File::Error::None);
    QVERIFY(loaded.recovery);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 3 * Store::TileBytes);
    Document restored(store, 23, *loaded.recovery);
    QVERIFY(settle(restored));
    QCOMPARE(restored.state(), Document::State::Idle);
    QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
    QCOMPARE(gpu.read(restored.layers()[0].paint.pixels, {0, 0, 1, 1}), QByteArray::fromHex("11223300"));
    QCOMPARE(gpu.read(restored.layers()[0].paint.mask, {0, 0, 1, 1}), QByteArray::fromHex("ffffffff"));
    for (int i = 0; i < 23; ++i) QVERIFY(restored.undo());
    QCOMPARE(restored.layers()[0].name, QStringLiteral("0"));
    QCOMPARE(restored.layers()[0].paint.opacity, quint8(64));
    QCOMPARE(restored.layers()[0].paint.operation, Store::CompositeOp::Erase);
    QCOMPARE(store->statistics().residentBytes, 2 * Store::TileBytes);
    QCOMPARE(gpu.owner.errorCount(), 0);
}


void KisGpuDocumentRecoveryFileTest::successfulWriteFitsTheReadBudget()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    auto layer = std::make_shared<Document::RecoveryLayer>();
    layer->id = 1;
    layer->mask = std::make_shared<const Session::Recovery>();
    Document::Recovery saved;
    saved.history = {{layer}}; saved.nextId = 2;
    quint64 low = 0, high = 4096;
    while (low + 1 < high) {
        const auto budget = (low + high) / 2;
        if (File::write(path, saved, budget) == File::Error::None) high = budget;
        else low = budget;
    }
    QCOMPARE(File::write(path, saved, high), File::Error::None);
    QCOMPARE(File::read(path, high).error, File::Error::None);
}


void KisGpuDocumentRecoveryFileTest::versionOneFilePreservesRecordedImages()
{
    auto selection = std::make_shared<Session::ReplaySelection>();
    Store::DabCommand select;
    select.center = {8, 8}; select.diameter = {32, 32}; select.rgba = 0x80000000;
    selection->commands = {select}; selection->clip = {0, 0, 16, 16};
    auto texture = std::make_shared<Session::ReplayTexture>();
    texture->size = {3, 2}; texture->origin = {-2, 3}; texture->alpha = QByteArray::fromHex("0080ff0140fe");
    Store::DabCommand command;
    command.center = {4.5, 4.5}; command.diameter = {5, 7}; command.fade = {.7, .8};
    command.rgba = 0xFF3399CC; command.opacity = 191; command.coverage = 127; command.textureStrength = 77;
    auto first = std::make_shared<Session::Recovery>();
    first->bounds = {0, 0, 1, 1}; first->pixels = QByteArray::fromHex("11223344");
    first->edits = {{{{command}, {0, 0, 16, 16}, selection, texture}}};
    first->cursor = first->firstRetained = 1;
    auto mask = std::make_shared<Session::Recovery>();
    mask->tiles.push_back({{0, 0}, QByteArray(Store::TileBytes, char(0xFF))});
    mask->tiles[0].pixels.replace(0, 4, QByteArray::fromHex("11223300"));
    auto layer = std::make_shared<Document::RecoveryLayer>();
    layer->id = 1; layer->name = QStringLiteral("雪") + QChar(0xD800) + QChar(0);
    layer->opacity = 77; layer->operation = Store::CompositeOp::Erase;
    layer->pixels = first; layer->mask = mask;
    auto later = std::make_shared<Document::RecoveryLayer>(*layer);
    auto laterPixels = std::make_shared<Session::Recovery>(*first);
    command.center = {4.5, 4.5}; command.operation = Store::CompositeOp::Erase;
    laterPixels->edits.push_back({{{command}, {0, 0, 16, 16}, selection, texture}});
    laterPixels->cursor = laterPixels->firstRetained = 2;
    later->name = QStringLiteral("Future"); later->pixels = laterPixels;
    Document::Recovery saved;
    saved.history = {{layer}, {later}}; saved.nextId = 2;
    auto working = *first;
    command.center = {11.5, 2.5}; command.operation = Store::CompositeOp::Over;
    working.working = Session::ReplayEdit{{{command}, {0, 0, 16, 16}, selection, texture}};
    saved.working = Document::RecoveryWorking{layer, Document::Target::Pixels, working, false};
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("recovery"));
    QCOMPARE(File::write(path, saved), File::Error::None);
    QFile actual(path);
    QVERIFY(actual.open(QIODevice::ReadOnly));
    const auto bytes = actual.readAll();
    const auto fixturePath = QFINDTESTDATA("data/gpu_recovery_v1.bin");
    QVERIFY(!fixturePath.isEmpty());
    QFile fixture(fixturePath);
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    QCOMPARE(bytes, fixture.readAll());
    const auto loaded = File::read(fixturePath);
    QCOMPARE(loaded.error, File::Error::None);
    QVERIFY(loaded.recovery);
    KisGpuTestDevice gpu;
    auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
    Document restored(store, 1, *loaded.recovery);
    QVERIFY(settle(restored));
    QCOMPARE(restored.state(), Document::State::Editing);
    QCOMPARE(restored.layers()[0].name, layer->name);
    const auto edit = restored.currentEditing();
    const auto hash = [&](const Store::Version &version) {
        return QCryptographicHash::hash(gpu.read(version, {0, 0, 16, 16}), QCryptographicHash::Sha256).toHex();
    };
    QCOMPARE(hash(edit.session->head()), QByteArray("de39337199ec898fbd76a1531ebcc7627070821f88ad90b805fba68a0efcc93d"));
    QCOMPARE(hash(edit.session->preview()), QByteArray("ce8304573b60698cb03631ff17dc77f9ea1c3d18ed5a04fd0e611338bddc825b"));
    QCOMPARE(hash(restored.layers()[0].paint.mask), QByteArray("f3c6cce210c3cfd5e76fbd5014aa949feaf976d4a8aae8c239bf72f9bc250bfd"));
    restored.cancel(edit.token);
    QVERIFY(restored.redo());
    QVERIFY(settle(restored));
    QCOMPARE(hash(restored.layers()[0].paint.pixels), QByteArray("5b88c2b7ef9bfaaf63b525b05df072b82ac04f33504eaff83a8309a8e8501050"));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuDocumentRecoveryFileTest)
#include "KisGpuDocumentRecoveryFileTest.moc"
