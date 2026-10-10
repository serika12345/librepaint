/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <set>
#include <stdexcept>
#include <limits>
#include <cstring>
#include "KisGpuTestDevice.h"
#include "KisGpuDevice.h"

namespace {
quint32 rgba(const QJsonArray &values)
{
    quint32 pixel = 0;
    for (int c = 0; c < 4; ++c) pixel |= quint32(values[c].toInt()) << (c * 8);
    return pixel;
}

QByteArray expected(QRect bounds, const QList<QPair<QRect, quint32>> &fills)
{
    QByteArray result(bounds.width() * bounds.height() * 4, '\0');
    for (const auto &fill : fills) {
        const QRect area = bounds.intersected(fill.first);
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                const int offset = ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4;
                for (int c = 0; c < 4; ++c) result[offset + c] = char(fill.second >> (c * 8));
            }
        }
    }
    return result;
}

bool finish(KisGpuTileStore &store, const KisGpuTileStore::Completion &completion)
{
    QElapsedTimer timer;
    timer.start();
    while (completion.status() == KisGpuTileStore::Status::Pending && timer.elapsed() < 5000) {
        store.poll();
        QTest::qWait(1);
    }
    store.poll();
    return completion.status() == KisGpuTileStore::Status::Succeeded;
}
}

/** Consumers are GPU edits, working versions and retained history versions. */
class KisGpuTileStoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void timestampsMeasureExecutionWithoutReadingPixels();
    void timedBatchesRetainAndRecycleQueries();
    void cleanupTestCase();
    void cleanup();
    void sparseSignedCoordinates();
    void copiesOnlyChangedTiles();
    void fullOverwriteAvoidsCopy();
    void orderedVersionsAndCancellation();
    void budgetRejectionIsAtomic();
    void pendingResourcesRemainBudgeted();
    void rejectsForeignVersionsAndAcceptsEmptyEdits();
    void emptyEditInheritsSourceCompletion();
    void versionOutlivesStore();
    void repeatedEditsReleaseOldVersions();
    void batchedPaintPreservesOrderAndCopiesTilesOnce();
    void batchFitsOneCopyBudget();
    void batchRejectionIsAtomic();
    void emptyBatchInheritsSourceCompletion();
    void groupedTilesRespectDeviceLimits();
    void sharedAllocationsStayBudgeted();
    void compositeVersionsPreservesLayerOrder();
    void imageCompositeRejectionAndEmptySource();
    void compositeRetainsPendingSource();
    void readbackPinsVersionAndReleasesStaging();
    void readbackRejectionIsAtomic();
    void readbackOutlivesStore();
    void generatedDabsMatchBrushMasks();
    void generatedDabsPreserveOrderAndRejectInvalidInput();
    void boundedSubmissionsResumeAfterPoll();
    void deviceLossRejectsReadsAndEditsWithoutPrematureRelease();
    void destroyedDeviceReleasesPendingReadback();
    void uploadPreservesPixelsAndSourceVersions();
    void uploadRejectionIsAtomic();
    void savedPixelsRestoreOnAnotherDevice();
    void maskedCompositeUsesCoveragePixels();
    void maskedCompositeRejectsForeignMasks();
    void projectionUpdatesOnlyDamageInOneSubmission();
    void projectionMasksAndRejections();
    void projectionKeepsSparseGapsAcrossAllocationGroups();
    void projectionPreservesEveryLayerRounding();
    void projectionFitsBoundedCommandMemory();
    void deviceOwnerInvalidatesBeforeDestroyingPendingReads();
    void nativeDeviceErrorStopsAllDocuments();
    void compositing_data();
    void compositing();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuTileStoreTest::initTestCase()
{
    try {
        m_gpu = std::make_unique<KisGpuTestDevice>();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    qInfo() << "GPU:" << m_gpu->name;
}

void KisGpuTileStoreTest::cleanup()
{
    if (m_gpu) {
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        QCOMPARE(m_gpu->owner.errorCount(), 0);
    }
}

void KisGpuTileStoreTest::cleanupTestCase() { m_gpu.reset(); }

void KisGpuTileStoreTest::readbackPinsVersionAndReleasesStaging()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    const QRect bounds(-67, -3, 137, 9);
    auto original = store.fill(store.emptyVersion(), QRect(-65, -1, 131, 3), 0x80402010);
    const auto read = store.readback(original.version, bounds);
    QCOMPARE(read.error, KisGpuTileStore::Error::None);
    auto changed = store.fill(original.version, QRect(-65, -1, 131, 3), 0xFF998877);
    original = {};
    QVERIFY(finish(store, read.completion));
    QCOMPARE(read.bytes(), expected(bounds, {{QRect(-65, -1, 131, 3), 0x80402010}}));
    QVERIFY(finish(store, changed.completion));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(bounds.width() * bounds.height() * 4));
    const quint64 resident = store.statistics().residentBytes;
    changed = {};
    store.poll();
    QVERIFY(resident > 0);
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    // The CPU result remains usable without retaining GPU history or staging.
    QCOMPARE(read.bytes(), expected(bounds, {{QRect(-65, -1, 131, 3), 0x80402010}}));
}

void KisGpuTileStoreTest::readbackRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes);
    auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto large = store.readback(base.version, QRect(0, 0, 128, 128));
    QCOMPARE(large.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(large.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(large.bytes().isEmpty());
    QCOMPARE(store.readback({}, QRect(0, 0, 1, 1)).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.readback(base.version, QRect(0, 0, INT_MAX, INT_MAX)).error,
             KisGpuTileStore::Error::BudgetExceeded);
    const auto empty = store.readback(base.version, QRect());
    QVERIFY(finish(store, empty.completion));
    QVERIFY(empty.bytes().isEmpty());
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuTileStoreTest::readbackOutlivesStore()
{
    KisGpuTileStore::Readback read;
    const QRect bounds(-1, -1, 3, 3);
    {
        KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
        const auto pending = store.fill(store.emptyVersion(), bounds, 0xFFABCDEF);
        read = store.readback(pending.version, bounds);
        QCOMPARE(read.error, KisGpuTileStore::Error::None);
    }
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(read.bytes(), expected(bounds, {{bounds, 0xFFABCDEF}}));
}

void KisGpuTileStoreTest::generatedDabsMatchBrushMasks()
{
    QFile file(QFileInfo(QStringLiteral(RASTER_EDIT_FIXTURE)).dir().filePath("brush_mask_contract.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto cases = QJsonDocument::fromJson(file.readAll()).object()["cases"].toArray();
    QVERIFY(!cases.isEmpty());
    for (const auto &value : cases) {
        const auto entry = value.toObject();
        const auto center = entry["center"].toArray();
        const auto diameter = entry["diameter"].toArray();
        const auto fade = entry["fade"].toArray();
        const auto bounds = entry["bounds"].toArray();
        const auto rows = entry["alphaRows"].toArray();
        for (const QPoint offset : {QPoint(-67, -3), QPoint(60, 61)}) {
            const QRect clip(offset, QSize(bounds[2].toInt(), bounds[3].toInt()));
            KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
            KisGpuTileStore::DabCommand dab;
            dab.center = QPointF(center[0].toDouble(), center[1].toDouble()) + offset;
            dab.diameter = QSizeF(diameter[0].toDouble(), diameter[1].toDouble());
            dab.fade = QSizeF(fade[0].toDouble(), fade[1].toDouble());
            dab.rgba = 0xFF204080;
            const auto edit = store.paintDabs(store.emptyVersion(), {dab}, clip);
            QCOMPARE(edit.error, KisGpuTileStore::Error::None);
            QVERIFY(finish(store, edit.completion));
            const auto actual = m_gpu->read(edit.version, clip.adjusted(-1, -1, 1, 1));
            const int stride = (clip.width() + 2) * 4;
            for (int y = -1; y <= clip.height(); ++y) {
                const auto mask = y >= 0 && y < rows.size() ? QByteArray::fromHex(rows[y].toString().toLatin1()) : QByteArray();
                for (int x = -1; x <= clip.width(); ++x) {
                    const int alpha = x >= 0 && x < mask.size() ? quint8(mask[x]) : 0;
                    const int index = (y + 1) * stride + (x + 1) * 4;
                    const quint32 color = alpha ? (dab.rgba & 0xFFFFFF) | quint32(alpha) << 24 : 0;
                    for (int c = 0; c < 4; ++c) {
                        QVERIFY2(qAbs(int(quint8(actual[index + c])) - int((color >> (c * 8)) & 255)) <= 1,
                            qPrintable(QStringLiteral("%1 (%2,%3) channel %4").arg(entry["id"].toString()).arg(x).arg(y).arg(c)));
                    }
                }
            }
            QCOMPARE(store.statistics().submissions, quint64(1));
            QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
            QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
        }
    }
}

void KisGpuTileStoreTest::generatedDabsPreserveOrderAndRejectInvalidInput()
{
    KisGpuTileStore store(m_gpu->owner, 64 * KisGpuTileStore::TileBytes);
    const QRect clip(-64, -64, 128, 128);
    KisGpuTileStore::DabCommand red, erase;
    red.center = QPointF(-.25, .25);
    red.diameter = QSizeF(33, 21);
    red.fade = QSizeF(.5, .75);
    red.rgba = 0x80FFFFFF;
    red.opacity = 192;
    red.coverage = 128;
    erase = red;
    erase.center += QPointF(3, 2);
    erase.operation = KisGpuTileStore::CompositeOp::Erase;
    const auto first = store.paintDabs(store.emptyVersion(), {red}, clip);
    const auto sequential = store.paintDabs(first.version, {erase}, clip);
    const auto batch = store.paintDabs(store.emptyVersion(), {red, erase}, clip);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(m_gpu->read(batch.version, clip), m_gpu->read(sequential.version, clip));
    const auto before = store.statistics();
    auto invalid = red;
    invalid.center.setX(std::numeric_limits<double>::quiet_NaN());
    QCOMPARE(store.paintDabs(batch.version, {red, invalid}, clip).error, KisGpuTileStore::Error::InvalidCommand);
    invalid = red;
    invalid.diameter.setWidth(-1);
    QCOMPARE(store.paintDabs(batch.version, {invalid}, clip).error, KisGpuTileStore::Error::InvalidCommand);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
}

void KisGpuTileStoreTest::boundedSubmissionsResumeAfterPoll()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes, 1);
    const auto first = store.fill(store.emptyVersion(), QRect(0, 0, 64, 64), 0xFF123456);
    QCOMPARE(first.error, KisGpuTileStore::Error::None);
    const auto before = store.statistics();
    QCOMPARE(store.fill(first.version, QRect(0, 0, 1, 1), 0xFFABCDEF).error,
             KisGpuTileStore::Error::QueueFull);
    QCOMPARE(store.readback(first.version, QRect(0, 0, 1, 1)).error, KisGpuTileStore::Error::QueueFull);
    QCOMPARE(store.composite(first.version, first.version, QRect(0, 0, 1, 1)).error,
             KisGpuTileStore::Error::QueueFull);
    const auto empty = store.paint(first.version, QRect(), 0xFFFFFFFF);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), first.completion.sequence());
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QVERIFY(finish(store, first.completion));
    const auto second = store.fill(first.version, QRect(0, 0, 1, 1), 0xFFABCDEF);
    QCOMPARE(second.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, second.completion));
    QCOMPARE(m_gpu->read(second.version, QRect(0, 0, 2, 1)),
             expected(QRect(0, 0, 2, 1), {{QRect(0, 0, 2, 1), 0xFF123456}, {QRect(0, 0, 1, 1), 0xFFABCDEF}}));
}

void KisGpuTileStoreTest::deviceLossRejectsReadsAndEditsWithoutPrematureRelease()
{
    KisGpuTestDevice isolatedGpu;
    KisGpuTileStore store(isolatedGpu.owner, 32 * KisGpuTileStore::TileBytes);
    auto pending = store.fill(store.emptyVersion(), QRect(-1, -1, 2, 2), 0xFFABCDEF);
    auto dependent = store.paint(pending.version, QRect(0, 0, 1, 1), 0x80FFFFFF);
    const auto read = store.readback(dependent.version, QRect(-2, -2, 4, 4));
    const auto before = store.statistics();
    store.invalidateDevice();
    QVERIFY(!store.deviceAvailable());
    QCOMPARE(pending.completion.status(), KisGpuTileStore::Status::Failed);
    QCOMPARE(dependent.completion.status(), KisGpuTileStore::Status::Failed);
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(read.bytes().isEmpty());
    QCOMPARE(store.fill(pending.version, QRect(0, 0, 1, 1), 0xFFFFFFFF).error, KisGpuTileStore::Error::DeviceLost);
    QCOMPARE(store.readback(pending.version, QRect(0, 0, 1, 1)).error, KisGpuTileStore::Error::DeviceLost);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    const auto firstCompletion = pending.completion;
    pending = {};
    dependent = {};
    wgpuDevicePoll(isolatedGpu.device, true, nullptr);
    store.poll();
    QCOMPARE(firstCompletion.status(), KisGpuTileStore::Status::Failed);
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Failed);
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    store.invalidateDevice();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
}

void KisGpuTileStoreTest::destroyedDeviceReleasesPendingReadback()
{
    KisGpuTestDevice gpu;
    KisGpuTileStore store(gpu.owner, 64 * KisGpuTileStore::TileBytes);
    auto edit = store.fill(store.emptyVersion(), QRect(0, 0, 256, 256), 0xFFABCDEF);
    const auto read = store.readback(edit.version, QRect(0, 0, 256, 256));
    QCOMPARE(read.error, KisGpuTileStore::Error::None);
    store.invalidateDevice();
    wgpuDeviceDestroy(gpu.device);
    edit = {};
    wgpuDevicePoll(gpu.device, true, nullptr);
    store.poll();
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(read.bytes().isEmpty());
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::uploadPreservesPixelsAndSourceVersions()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    const QRect canvas(-64, -64, 192, 192), bounds(-3, -2, 70, 67);
    const auto base = store.fill(store.emptyVersion(), canvas, 0xFF102030);
    QByteArray pixels(bounds.width() * bounds.height() * 4, '\0');
    for (int i = 0; i < pixels.size(); ++i) pixels[i] = char(i * 37 % 256);
    for (int i = 0; i < pixels.size() / 4; i += 17) pixels[i * 4 + 3] = '\0';
    const QByteArray original = pixels;
    const auto before = store.statistics();
    const auto imported = store.upload(base.version, bounds, pixels);
    QCOMPARE(imported.error, KisGpuTileStore::Error::None);
    pixels.fill('\0');
    QVERIFY(finish(store, imported.completion));
    auto wanted = expected(canvas, {{canvas, 0xFF102030}});
    for (int y = 0; y < bounds.height(); ++y) {
        std::memcpy(wanted.data() + ((bounds.y() + y - canvas.y()) * canvas.width() + bounds.x() - canvas.x()) * 4,
                    original.constData() + y * bounds.width() * 4, bounds.width() * 4);
    }
    QCOMPARE(m_gpu->read(imported.version, canvas), wanted);
    QCOMPARE(m_gpu->read(base.version, canvas), expected(canvas, {{canvas, 0xFF102030}}));
    QCOMPARE(store.statistics().pixelUploadBytes - before.pixelUploadBytes, quint64(original.size()));
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
}

void KisGpuTileStoreTest::uploadRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes, 1);
    auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    const auto before = store.statistics();
    QCOMPARE(store.upload(base.version, QRect(0, 0, 1, 1), QByteArray(4, '\xFF')).error, KisGpuTileStore::Error::QueueFull);
    QVERIFY(finish(store, base.completion));
    QCOMPARE(store.upload(base.version, QRect(0, 0, 1, 1), QByteArray(3, '\0')).error, KisGpuTileStore::Error::InvalidCommand);
    QCOMPARE(store.upload(base.version, QRect(0, 0, 128, 128), QByteArray(128 * 128 * 4, '\0')).error,
             KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(store.upload({}, QRect(0, 0, 1, 1), QByteArray(4, '\0')).error, KisGpuTileStore::Error::InvalidVersion);
    const auto empty = store.upload(base.version, {}, {});
    QVERIFY(finish(store, empty.completion));
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::savedPixelsRestoreOnAnotherDevice()
{
    const QRect bounds(-3, -2, 70, 67);
    KisGpuTileStore::Readback saved;
    {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 32 * KisGpuTileStore::TileBytes);
        const auto base = store.fill(store.emptyVersion(), bounds, 0x80402010);
        const auto edited = store.paint(base.version, QRect(-1, -1, 3, 3), 0xFF123456);
        saved = store.readback(edited.version, bounds);
        QVERIFY(finish(store, saved.completion));
        store.invalidateDevice();
        wgpuDeviceDestroy(gpu.device);
    }
    QCOMPARE(saved.completion.status(), KisGpuTileStore::Status::Succeeded);
    const auto pixels = saved.bytes();
    QVERIFY(!pixels.isEmpty());
    KisGpuTestDevice nextGpu;
    KisGpuTileStore restored(nextGpu.owner, 32 * KisGpuTileStore::TileBytes);
    const auto loaded = restored.upload(restored.emptyVersion(), bounds, pixels);
    QCOMPARE(loaded.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(restored, loaded.completion));
    QCOMPARE(nextGpu.read(loaded.version, bounds), pixels);
    QCOMPARE(restored.statistics().pixelUploadBytes, quint64(pixels.size()));
    QCOMPARE(nextGpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::sparseSignedCoordinates()
{
    KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
    const auto empty = store.emptyVersion();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    const auto edit = store.fill(empty, QRect(-1, -1, 2, 2), 0x80402010);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QCOMPARE(edit.version.tileCount(), qsizetype(4));
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(edit.version, QRect(-65, -65, 130, 130)),
             expected(QRect(-65, -65, 130, 130), {{QRect(-1, -1, 2, 2), 0x80402010}}));
    QCOMPARE(empty.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, 4 * KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
}

void KisGpuTileStoreTest::copiesOnlyChangedTiles()
{
    KisGpuTileStore store(m_gpu->owner, 3 * KisGpuTileStore::TileBytes + 1024);
    const QRect bounds(0, 0, 128, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, QRect(2, 3, 4, 5), 0x00406080);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(m_gpu->read(edit.version, bounds), expected(bounds, {{bounds, 0xFF102030}, {QRect(2, 3, 4, 5), 0x00406080}}));
    QCOMPARE(store.statistics().tileCopyBytes, KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::orderedVersionsAndCancellation()
{
    KisGpuTileStore store(m_gpu->owner, 16 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, 0, 192, 64);
    const auto first = store.fill(store.emptyVersion(), QRect(-1, 2, 66, 4), 0xFF112233);
    const auto second = store.fill(first.version, QRect(63, 3, 3, 5), 0x80445566);
    QCOMPARE(second.error, KisGpuTileStore::Error::None);
    QVERIFY(second.completion.sequence() > first.completion.sequence());
    // Undo retains first; a cancelled working edit must also preserve second (Redo).
    {
        const auto working = store.fill(first.version, QRect(-62, 1, 2, 2), 0xFFABCDEF);
        QVERIFY(finish(store, working.completion));
    }
    QVERIFY(finish(store, first.completion));
    QVERIFY(finish(store, second.completion));
    QCOMPARE(m_gpu->read(first.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}}));
    QCOMPARE(m_gpu->read(second.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}, {QRect(63, 3, 3, 5), 0x80445566}}));
}

void KisGpuTileStoreTest::fullOverwriteAvoidsCopy()
{
    KisGpuTileStore store(m_gpu->owner, 3 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, -64, 64, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, bounds, 0x00112233);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(m_gpu->read(edit.version, bounds), expected(bounds, {{bounds, 0x00112233}}));
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
    QCOMPARE(store.statistics().residentBytes, 2 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::budgetRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->owner, 3 * KisGpuTileStore::TileBytes);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto rejected = store.fill(base.version, QRect(-64, 0, 192, 1), 0xFFABCDEF);
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(rejected.version.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(m_gpu->read(base.version, QRect(-1, -1, 3, 3)), expected(QRect(-1, -1, 3, 3), {{QRect(0, 0, 1, 1), 0xFF123456}}));
    const auto huge = store.fill(base.version, QRect(-1000000000, -1000000000, 2000000000, 2000000000), 0);
    QCOMPARE(huge.error, KisGpuTileStore::Error::BudgetExceeded);
}

void KisGpuTileStoreTest::pendingResourcesRemainBudgeted()
{
    KisGpuTileStore store(m_gpu->owner, KisGpuTileStore::TileBytes + 256);
    KisGpuTileStore::Completion completion;
    {
        const auto edit = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
        completion = edit.completion;
    }
    const auto rejected = store.fill(store.emptyVersion(), QRect(64, 0, 1, 1), 0xFFABCDEF);
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QVERIFY(finish(store, completion));
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    const auto retried = store.fill(store.emptyVersion(), QRect(64, 0, 1, 1), 0xFFABCDEF);
    QCOMPARE(retried.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, retried.completion));
}

void KisGpuTileStoreTest::rejectsForeignVersionsAndAcceptsEmptyEdits()
{
    KisGpuTileStore store(m_gpu->owner, 0);
    KisGpuTileStore other(m_gpu->owner, 0);
    QCOMPARE(store.fill(other.emptyVersion(), QRect(0, 0, 1, 1), 0).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.fill({}, QRect(), 0).error, KisGpuTileStore::Error::InvalidVersion);
    const auto empty = store.fill(store.emptyVersion(), QRect(), 0);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(empty.completion.sequence(), quint64(0));
    QCOMPARE(store.statistics().submissions, quint64(0));
}

void KisGpuTileStoreTest::versionOutlivesStore()
{
    KisGpuTileStore::Version version;
    KisGpuTileStore::Completion completion;
    {
        KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes);
        const auto edit = store.fill(store.emptyVersion(), QRect(5, 7, 2, 3), 0x00112233);
        version = edit.version;
        completion = edit.completion;
    }
    QCOMPARE(completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(m_gpu->read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(5, 7, 2, 3), 0x00112233}}));
}

void KisGpuTileStoreTest::emptyEditInheritsSourceCompletion()
{
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes);
    const auto source = store.fill(store.emptyVersion(), QRect(1, 1, 2, 2), 0xFF123456);
    const auto empty = store.fill(source.version, QRect(), 0);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), source.completion.sequence());
    QVERIFY(finish(store, empty.completion));
    QCOMPARE(source.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(store.statistics().submissions, quint64(1));
    QCOMPARE(m_gpu->read(empty.version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 0xFF123456}}));
}

void KisGpuTileStoreTest::repeatedEditsReleaseOldVersions()
{
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes + 256);
    auto version = store.emptyVersion();
    for (quint32 i = 1; i <= 64; ++i) {
        const auto edit = store.fill(version, QRect(1, 1, 2, 2), i);
        QCOMPARE(edit.error, KisGpuTileStore::Error::None);
        QVERIFY(finish(store, edit.completion));
        version = edit.version;
        QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    }
    QCOMPARE(m_gpu->read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 64}}));
    QCOMPARE(store.statistics().submissions, quint64(64));
}

void KisGpuTileStoreTest::batchedPaintPreservesOrderAndCopiesTilesOnce()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, -64, 128, 128);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    using Op = KisGpuTileStore::CompositeOp;
    const QVector<KisGpuTileStore::PaintCommand> commands {
        {QRect(-5, -5, 11, 11), 0x800000FF, Op::Over},
        {QRect(-2, -2, 5, 5), 0xC0FF0000, Op::Over, 192, 128},
        {QRect(0, -1, 3, 5), 0x80000000, Op::Erase, 128, 128},
        {QRect(-63, -63, 1, 1), 0xFFABCDEF, Op::Over},
        {QRect(58, 58, 5, 5), 0xFF00FF00, Op::Over}
    };
    auto sequential = base.version;
    for (const auto &command : commands) {
        const auto edit = store.paint(sequential, command.rectangle, command.rgba, command.operation,
                                      command.opacity, command.coverage);
        QVERIFY(finish(store, edit.completion));
        sequential = edit.version;
    }
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, commands);
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(m_gpu->read(batch.version, bounds), m_gpu->read(sequential, bounds));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(1));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, 4 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::batchFitsOneCopyBudget()
{
    // A stroke has room for the original tile, one changed tile and compact commands.
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes + 8192);
    const QRect bounds(0, 0, 64, 64), area(2, 3, 4, 5);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    QVector<KisGpuTileStore::PaintCommand> commands;
    for (quint32 i = 0; i < 128; ++i) commands.push_back({area, 0xFF000000 | i});
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, commands);
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(store.statistics().residentBytes <= 2 * KisGpuTileStore::TileBytes + 8192);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(m_gpu->read(batch.version, bounds), expected(bounds, {{bounds, 0xFF102030}, {area, 0xFF00007F}}));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(store.statistics().residentBytes, 2 * KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, KisGpuTileStore::TileBytes);
    QVERIFY(store.statistics().commandUploadBytes - before.commandUploadBytes <= 8192);
}

void KisGpuTileStoreTest::batchRejectionIsAtomic()
{
    KisGpuTileStore store(m_gpu->owner, 3 * KisGpuTileStore::TileBytes + 256);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const QVector<QVector<KisGpuTileStore::PaintCommand>> rejectedCommands {
        {{QRect(0, 0, 1, 1), 0xFFABCDEF}, {QRect(64, 0, 128, 1), 0xFF112233}},
        {{QRect(0, 0, 1, 1), 0xFFABCDEF}, {QRect(-1000000000, -1000000000, 2000000000, 2000000000), 0xFF000000}},
        QVector<KisGpuTileStore::PaintCommand>(1024, {QRect(0, 0, 1, 1), 0xFFABCDEF})
    };
    for (const auto &commands : rejectedCommands) {
        const auto rejected = store.paint(base.version, commands);
        QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
        QCOMPARE(rejected.version.tileCount(), qsizetype(0));
        QCOMPARE(store.statistics().residentBytes, before.residentBytes);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
        QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes);
    }
    QCOMPARE(m_gpu->read(base.version, QRect(0, 0, 64, 64)),
             expected(QRect(0, 0, 64, 64), {{QRect(0, 0, 1, 1), 0xFF123456}}));
    QCOMPARE(store.paint({}, rejectedCommands[0]).error, KisGpuTileStore::Error::InvalidVersion);
    KisGpuTileStore other(m_gpu->owner, 0);
    QCOMPARE(store.paint(other.emptyVersion(), rejectedCommands[0]).error, KisGpuTileStore::Error::InvalidVersion);
}

void KisGpuTileStoreTest::emptyBatchInheritsSourceCompletion()
{
    KisGpuTileStore store(m_gpu->owner, KisGpuTileStore::TileBytes + 256);
    const auto source = store.fill(store.emptyVersion(), QRect(1, 1, 2, 2), 0xFF123456);
    const QRect huge(-1000000000, -1000000000, 2000000000, 2000000000);
    using Op = KisGpuTileStore::CompositeOp;
    const QVector<QVector<KisGpuTileStore::PaintCommand>> batches {
        {}, {{QRect(), 0xFF000000}, {huge, 0xFF000000, Op::Over, 0},
             {huge, 0xFF000000, Op::Erase, 255, 0}, {huge, 0x000000FF}}
    };
    for (const auto &commands : batches) {
        const auto empty = store.paint(source.version, commands);
        QCOMPARE(empty.error, KisGpuTileStore::Error::None);
        QCOMPARE(empty.completion.sequence(), source.completion.sequence());
        QVERIFY(finish(store, empty.completion));
        QCOMPARE(m_gpu->read(empty.version, QRect(0, 0, 64, 64)),
                 expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 0xFF123456}}));
    }
    QCOMPARE(store.statistics().submissions, quint64(1));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::groupedTilesRespectDeviceLimits()
{
    KisGpuTestDevice gpu(2 * KisGpuTileStore::TileBytes);
    KisGpuTileStore store(gpu.owner, 16 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, 0, 320, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto batch = store.paint(base.version, {{bounds, 0xFFABCDEF}});
    QCOMPARE(batch.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, batch.completion));
    QCOMPARE(gpu.read(batch.version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
    QCOMPARE(gpu.read(base.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(3));
    QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, 5 * KisGpuTileStore::TileBytes);
    const auto composed = store.composite(base.version, batch.version, bounds);
    QCOMPARE(composed.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, composed.completion));
    QCOMPARE(gpu.read(composed.version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
    QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(6));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::sharedAllocationsStayBudgeted()
{
    KisGpuTileStore store(m_gpu->owner, 8 * KisGpuTileStore::TileBytes);
    const QRect bounds(0, 0, 192, 64);
    auto version = store.emptyVersion();
    {
        const auto base = store.fill(version, bounds, 0xFF123456);
        QVERIFY(finish(store, base.completion));
        version = base.version;
    }
    for (int x = 0; x < 3; ++x) {
        const auto edit = store.fill(version, QRect(x * 64, 0, 64, 64), 0xFFABCDEF);
        QVERIFY(finish(store, edit.completion));
        version = edit.version;
        // A shared GPU buffer remains fully budgeted while any live tile uses it.
        std::set<WGPUBuffer> buffers;
        for (int i = 0; i < 3; ++i) buffers.insert(version.tile(QPoint(i, 0)).buffer);
        quint64 allocated = 0;
        for (const auto buffer : buffers) allocated += wgpuBufferGetSize(buffer);
        QCOMPARE(store.statistics().residentBytes, allocated);
    }
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
    QCOMPARE(m_gpu->read(version, bounds), expected(bounds, {{bounds, 0xFFABCDEF}}));
}

void KisGpuTileStoreTest::compositeVersionsPreservesLayerOrder()
{
    KisGpuTileStore store(m_gpu->owner, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds(-65, -65, 194, 194), redArea(-1, -1, 66, 66), patch(63, 63, 2, 2), greenArea(0, 0, 4, 4);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFFFF0000);
    const auto red = store.fill(store.emptyVersion(), redArea, 0xFF0000FF);
    const auto patched = store.fill(red.version, patch, 0xFF00FF00);
    const auto green = store.fill(store.emptyVersion(), greenArea, 0xFF00FF00);
    const auto first = store.composite(base.version, patched.version, bounds);
    const auto front = store.composite(first.version, green.version, bounds);
    const auto reverseFirst = store.composite(base.version, green.version, bounds);
    const auto reverse = store.composite(reverseFirst.version, patched.version, bounds);
    QVERIFY(finish(store, front.completion));
    QVERIFY(finish(store, reverse.completion));
    QCOMPARE(m_gpu->read(first.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {redArea, 0xFF0000FF}, {patch, 0xFF00FF00}}));
    QCOMPARE(m_gpu->read(front.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {redArea, 0xFF0000FF}, {patch, 0xFF00FF00}, {greenArea, 0xFF00FF00}}));
    QCOMPARE(m_gpu->read(reverse.version, bounds), m_gpu->read(first.version, bounds));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}}));
    QCOMPARE(m_gpu->read(red.version, bounds), expected(bounds, {{redArea, 0xFF0000FF}}));
    const QRect clip(0, 0, 2, 3);
    const auto clipped = store.composite(base.version, patched.version, clip);
    QVERIFY(finish(store, clipped.completion));
    QCOMPARE(m_gpu->read(clipped.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {clip, 0xFF0000FF}}));

    // Same half-transparent layer stack and fixed pixels as the CPU raster contract.
    const QRect area(-2, -2, 67, 67);
    const auto halfRed = store.fill(store.emptyVersion(), area, 0x800000FF);
    const auto halfGreen = store.fill(store.emptyVersion(), area, 0x8000FF00);
    const auto redFirst = store.composite(base.version, halfRed.version, area);
    const auto greenFront = store.composite(redFirst.version, halfGreen.version, area);
    const auto greenFirst = store.composite(base.version, halfGreen.version, area);
    const auto redFront = store.composite(greenFirst.version, halfRed.version, area);
    QVERIFY(finish(store, greenFront.completion));
    QVERIFY(finish(store, redFront.completion));
    QCOMPARE(m_gpu->read(greenFront.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F8040}}));
    QCOMPARE(m_gpu->read(redFront.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F4080}}));
}

void KisGpuTileStoreTest::imageCompositeRejectionAndEmptySource()
{
    KisGpuTileStore store(m_gpu->owner, 4 * KisGpuTileStore::TileBytes + 256);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    const auto source = store.fill(store.emptyVersion(), QRect(0, 0, 128, 1), 0xFFABCDEF);
    QVERIFY(finish(store, source.completion));
    const auto before = store.statistics();
    const auto rejected = store.composite(base.version, source.version, QRect(0, 0, 128, 1));
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes);
    const QRect huge(-1000000000, -1000000000, 2000000000, 2000000000);
    const auto empty = store.composite(base.version, store.emptyVersion(), huge);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), base.completion.sequence());
    QCOMPARE(store.statistics().submissions, before.submissions);
    const auto invisible = store.composite(base.version, source.version, huge, KisGpuTileStore::CompositeOp::Over, 0);
    QCOMPARE(invisible.error, KisGpuTileStore::Error::None);
    QCOMPARE(invisible.completion.sequence(), base.completion.sequence());
    QCOMPARE(store.composite(base.version, {}, QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    KisGpuTileStore other(m_gpu->owner, 0);
    QCOMPARE(store.composite(base.version, other.emptyVersion(), QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.composite(other.emptyVersion(), source.version, QRect()).error, KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(m_gpu->read(base.version, QRect(0, 0, 64, 64)),
             expected(QRect(0, 0, 64, 64), {{QRect(0, 0, 1, 1), 0xFF123456}}));
}

void KisGpuTileStoreTest::compositeRetainsPendingSource()
{
    KisGpuTileStore store(m_gpu->owner, 5 * KisGpuTileStore::TileBytes);
    KisGpuTileStore::Version version;
    KisGpuTileStore::Completion completion;
    const QRect area(-1, 1, 1, 1);
    {
        const auto source = store.fill(store.emptyVersion(), area, 0xFF123456);
        const auto edit = store.composite(store.emptyVersion(), source.version, area);
        QCOMPARE(edit.error, KisGpuTileStore::Error::None);
        version = edit.version;
        completion = edit.completion;
    }
    QVERIFY(finish(store, completion));
    QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    QCOMPARE(m_gpu->read(version, QRect(-64, 0, 64, 64)), expected(QRect(-64, 0, 64, 64), {{area, 0xFF123456}}));
}


void KisGpuTileStoreTest::maskedCompositeUsesCoveragePixels()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes);
    const QRect bounds(-2, -1, 132, 3), selected(-1, 0, 66, 1), opaque(63, 0, 2, 1);
    auto base = store.fill(store.emptyVersion(), bounds, 0xFFFF0000);
    auto source = store.fill(store.emptyVersion(), bounds, 0x800000FF);
    auto mask = store.fill(store.emptyVersion(), selected, 0x807F3F1F);
    mask = store.fill(mask.version, opaque, 0xFF112233);
    const auto before = store.statistics();
    auto result = store.compositeMasked(base.version, source.version, mask.version, bounds);
    QCOMPARE(result.error, KisGpuTileStore::Error::None);
    source = {};
    mask = {};
    QVERIFY(finish(store, result.completion));
    QCOMPARE(m_gpu->read(result.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {selected, 0xFFBF0040}, {opaque, 0xFF7F0080}}));
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}}));
    const auto after = store.statistics();
    QCOMPARE(after.pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(after.pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(after.submissions, before.submissions + 1);
    result = {};
    base = {};
    store.poll();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
}

void KisGpuTileStoreTest::maskedCompositeRejectsForeignMasks()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes, 1);
    KisGpuTileStore other(m_gpu->owner, 0);
    const QRect bounds(0, 0, 1, 1);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    const auto before = store.statistics();
    QCOMPARE(store.compositeMasked(base.version, base.version, other.emptyVersion(), bounds).error,
             KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.compositeMasked(base.version, base.version, {}, bounds).error,
             KisGpuTileStore::Error::InvalidVersion);
    QCOMPARE(store.compositeMasked(base.version, base.version, base.version, bounds).error,
             KisGpuTileStore::Error::QueueFull);
    const auto empty = store.compositeMasked(base.version, base.version, store.emptyVersion(), bounds);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QVERIFY(empty.version == base.version);
    QCOMPARE(empty.completion.sequence(), base.completion.sequence());
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
}


void KisGpuTileStoreTest::timedBatchesRetainAndRecycleQueries()
{
    KisGpuTestDevice gpu(0, true);
    KisGpuTileStore store(gpu.owner, 512 * KisGpuTileStore::TileBytes);
    auto version = store.emptyVersion();
    for (int round = 0; round < 2; ++round) {
        QVector<KisGpuTileStore::Completion> completions;
        for (int i = 0; i < 128; ++i) {
            auto edit = store.fill(version, QRect(0, 0, 64, 64), 0xFF000000 | quint32(i + round * 128));
            QCOMPARE(edit.error, KisGpuTileStore::Error::None);
            version = edit.version;
            completions.push_back(edit.completion);
        }
        QVERIFY(finish(store, completions.back()));
        for (const auto &completion : completions) QVERIFY(completion.gpuComputeNanoseconds().has_value());
        QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    }
    QCOMPARE(gpu.read(version, QRect(0, 0, 1, 1)), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::timestampsMeasureExecutionWithoutReadingPixels()
{
    KisGpuTestDevice gpu(0, true);
    KisGpuTileStore store(gpu.owner, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds(0, 0, 128, 128);
    auto pixels = store.fill(store.emptyVersion(), bounds, 0xFFABCDEF);
    QVERIFY(!pixels.completion.gpuComputeNanoseconds().has_value());
    QVERIFY(finish(store, pixels.completion));
    QVERIFY(pixels.completion.gpuComputeNanoseconds().has_value());
    QVERIFY(*pixels.completion.gpuComputeNanoseconds() > 0);
    auto paint = store.paint(pixels.version, bounds, 0x80123456);
    auto mask = store.fill(store.emptyVersion(), bounds, 0x80111111);
    auto composite = store.compositeMasked(pixels.version, paint.version, mask.version, bounds);
    auto projection = store.project(store.emptyVersion(), {{pixels.version}, {composite.version}}, bounds);
    for (const auto &edit : {paint, mask, composite, projection}) {
        QVERIFY(finish(store, edit.completion));
        QVERIFY(edit.completion.gpuComputeNanoseconds().has_value());
    }
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    auto read = store.readback(projection.version, bounds);
    QVERIFY(finish(store, read.completion));
    QVERIFY(!read.completion.gpuComputeNanoseconds().has_value());
    auto uploaded = store.upload(store.emptyVersion(), bounds, read.bytes());
    QVERIFY(finish(store, uploaded.completion));
    QVERIFY(!uploaded.completion.gpuComputeNanoseconds().has_value());
    QCOMPARE(gpu.read(uploaded.version, bounds), read.bytes());
    QVERIFY(store.statistics().timingReadbackBytes > 0);
    KisGpuTileStore limited(gpu.owner, KisGpuTileStore::TileBytes + 64);
    QCOMPARE(limited.fill(limited.emptyVersion(), QRect(0, 0, 64, 64), 0xFFFFFFFF).error,
             KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(limited.statistics().residentBytes, quint64(0));
    QCOMPARE(limited.statistics().submissions, quint64(0));
    KisGpuTileStore ordinary(m_gpu->owner, KisGpuTileStore::TileBytes + 64);
    const auto plain = ordinary.fill(ordinary.emptyVersion(), QRect(0, 0, 64, 64), 0xFFFFFFFF);
    QVERIFY(finish(ordinary, plain.completion));
    QVERIFY(!plain.completion.gpuComputeNanoseconds().has_value());
    const auto beforeLimit = store.statistics();
    QVector<KisGpuTileStore::Layer> tooMany(94, {pixels.version});
    QCOMPARE(store.project(store.emptyVersion(), tooMany, bounds).error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(store.statistics().residentBytes, beforeLimit.residentBytes);
    QCOMPARE(store.statistics().submissions, beforeLimit.submissions);
    tooMany.removeLast();
    QVector<KisGpuTileStore::Completion> occupied;
    for (int i = 0; i < 64; ++i) {
        const auto projection = store.project(store.emptyVersion(), tooMany, QRect(0, 0, 1, 1));
        QCOMPARE(projection.error, KisGpuTileStore::Error::None);
        occupied.push_back(projection.completion);
    }
    KisGpuTileStore other(gpu.owner, 4 * KisGpuTileStore::TileBytes);
    QCOMPARE(other.fill(other.emptyVersion(), QRect(0, 0, 1, 1), 0xFFFFFFFF).error,
             KisGpuTileStore::Error::QueueFull);
    QCOMPARE(other.statistics().residentBytes, quint64(0));
    QVERIFY(finish(store, occupied.back()));
    const auto retried = other.fill(other.emptyVersion(), QRect(0, 0, 1, 1), 0xFFFFFFFF);
    QVERIFY(finish(other, retried.completion));
    QVERIFY(retried.completion.gpuComputeNanoseconds().has_value());
    const auto pending = store.paint(uploaded.version, bounds, 0xFFFFFFFF);
    gpu.owner.destroy();
    QCOMPARE(pending.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(!pending.completion.gpuComputeNanoseconds().has_value());
    store.poll();
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::projectionUpdatesOnlyDamageInOneSubmission()
{
    KisGpuTileStore store(m_gpu->owner, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds(-65, -2, 196, 5), area(-1, 0, 67, 1), damage(0, 0, 64, 1);
    auto blue = store.fill(store.emptyVersion(), bounds, 0xFFFF0000);
    auto red = store.fill(store.emptyVersion(), area, 0x800000FF);
    auto green = store.fill(store.emptyVersion(), area, 0x8000FF00);
    const auto before = store.statistics();
    auto frontGreen = store.project(store.emptyVersion(), {{blue.version}, {red.version}, {green.version}}, bounds);
    QCOMPARE(frontGreen.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, frontGreen.completion));
    QCOMPARE(store.statistics().submissions, before.submissions + 1);
    QCOMPARE(m_gpu->read(frontGreen.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F8040}}));
    const auto beforeDamage = store.statistics();
    auto frontRed = store.project(frontGreen.version, {{blue.version}, {green.version}, {red.version}}, damage);
    QCOMPARE(frontRed.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, frontRed.completion));
    QCOMPARE(store.statistics().submissions, beforeDamage.submissions + 1);
    QCOMPARE(store.statistics().tileCopyBytes, beforeDamage.tileCopyBytes + KisGpuTileStore::TileBytes);
    const auto untouched = QPoint(-2, -1);
    QCOMPARE(frontRed.version.tile(untouched).buffer, frontGreen.version.tile(untouched).buffer);
    QCOMPARE(frontRed.version.tile(untouched).offset, frontGreen.version.tile(untouched).offset);
    QCOMPARE(m_gpu->read(frontRed.version, bounds),
             expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F8040}, {damage, 0xFF3F4080}}));
    auto removed = store.project(frontRed.version, {{blue.version}}, area);
    QCOMPARE(removed.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, removed.completion));
    QCOMPARE(m_gpu->read(removed.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}}));
    const auto cleared = store.project(removed.version, {}, bounds);
    QVERIFY(finish(store, cleared.completion));
    QCOMPARE(m_gpu->read(cleared.version, bounds), expected(bounds, {}));
    QCOMPARE(m_gpu->read(frontGreen.version, bounds), expected(bounds, {{bounds, 0xFFFF0000}, {area, 0xFF3F8040}}));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

void KisGpuTileStoreTest::projectionMasksAndRejections()
{
    KisGpuTileStore store(m_gpu->owner, 32 * KisGpuTileStore::TileBytes, 1);
    KisGpuTileStore other(m_gpu->owner, 0);
    const QRect area(-1, 0, 67, 1);
    auto blue = store.fill(store.emptyVersion(), area, 0xFFFF0000);
    QVERIFY(finish(store, blue.completion));
    auto red = store.fill(store.emptyVersion(), area, 0xFF0000FF);
    QVERIFY(finish(store, red.completion));
    auto mask = store.fill(store.emptyVersion(), area, 0x80112233);
    QVERIFY(finish(store, mask.completion));
    const auto before = store.statistics();
    auto result = store.project(store.emptyVersion(), {{blue.version}, {red.version, mask.version, 128}}, area);
    QCOMPARE(result.error, KisGpuTileStore::Error::None);
    QCOMPARE(store.project(result.version, {{blue.version}}, area).error, KisGpuTileStore::Error::QueueFull);
    QCOMPARE(store.project(result.version, {{red.version, other.emptyVersion()}}, area).error,
             KisGpuTileStore::Error::InvalidVersion);
    const auto noop = store.project(result.version, {}, QRect());
    QVERIFY(noop.version == result.version);
    QCOMPARE(noop.completion.sequence(), result.completion.sequence());
    red = {};
    mask = {};
    QVERIFY(finish(store, result.completion));
    QCOMPARE(store.statistics().submissions, before.submissions + 1);
    QCOMPARE(m_gpu->read(result.version, area), expected(area, {{area, 0xFFBF0040}}));
    const auto maskedOut = store.project(result.version, {{blue.version, store.emptyVersion()}}, area);
    QVERIFY(finish(store, maskedOut.completion));
    QCOMPARE(m_gpu->read(maskedOut.version, area), expected(area, {}));
    KisGpuTileStore limited(m_gpu->owner, 2 * KisGpuTileStore::TileBytes + 512);
    auto source = limited.fill(limited.emptyVersion(), QRect(0, 0, 1, 1), 0xFFFFFFFF);
    auto old = limited.fill(limited.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(limited, old.completion));
    const auto resident = limited.statistics().residentBytes;
    QCOMPARE(limited.project(old.version, {{source.version}}, QRect(0, 0, 1, 1)).error,
             KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(limited.statistics().residentBytes, resident);
    source = {};
    old = {};
    limited.poll();
    QCOMPARE(limited.statistics().residentBytes, quint64(0));
}


void KisGpuTileStoreTest::projectionKeepsSparseGapsAcrossAllocationGroups()
{
    KisGpuTileStore store(m_gpu->owner, 1024 * KisGpuTileStore::TileBytes);
    QVector<KisGpuTileStore::PaintCommand> commands;
    for (int i = 0; i < 128; ++i) commands.push_back({QRect(i % 16 * 128, i / 16 * 128, 64, 64), 0xFF123456});
    auto source = store.paint(store.emptyVersion(), commands);
    auto projected = store.project(store.emptyVersion(), {{source.version}}, QRect(-1000000000, -1000000000, 2000000000, 2000000000));
    QCOMPARE(projected.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, projected.completion));
    QCOMPARE(projected.version.tileCount(), qsizetype(128));
    const QRect first(0, 0, 192, 64), last(1920, 896, 64, 64);
    QCOMPARE(m_gpu->read(projected.version, first),
             expected(first, {{QRect(0, 0, 64, 64), 0xFF123456}, {QRect(128, 0, 64, 64), 0xFF123456}}));
    QCOMPARE(m_gpu->read(projected.version, last), expected(last, {{last, 0xFF123456}}));
    source = store.fill(source.version, last, 0xFFABCDEF);
    const auto before = store.statistics();
    const auto changed = store.project(projected.version, {{source.version}}, last);
    QVERIFY(finish(store, changed.completion));
    QCOMPARE(store.statistics().submissions, before.submissions + 1);
    QCOMPARE(store.statistics().tileCopyBytes, before.tileCopyBytes);
    QCOMPARE(m_gpu->read(changed.version, last), expected(last, {{last, 0xFFABCDEF}}));
    QCOMPARE(m_gpu->read(projected.version, last), expected(last, {{last, 0xFF123456}}));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}


void KisGpuTileStoreTest::projectionPreservesEveryLayerRounding()
{
    KisGpuTestDevice gpu(2 * KisGpuTileStore::TileBytes);
    KisGpuTileStore store(gpu.owner, 512 * KisGpuTileStore::TileBytes);
    const QRect bounds(-65, -1, 260, 67), damage(-3, 0, 133, 65);
    auto old = store.fill(store.emptyVersion(), bounds, 0x00332211);
    QVector<KisGpuTileStore::Layer> layers;
    for (int i = 0; i < 11; ++i) {
        const QRect area = i % 4 == 0 ? QRect(-65, 0, 64, 64)
                                   : QRect(-2 + i, -1, 192 - i * 2, 66);
        const auto pixels = store.fill(store.emptyVersion(), area,
            (quint32(17 + i * 19) << 24) | quint32(0x123456 + i * 0x090b0d));
        KisGpuTileStore::Version mask;
        if (i % 3 == 1) {
            mask = store.fill(store.emptyVersion(), QRect(62, 0, 68, 64),
                              (quint32(43 + i * 13) << 24) | 0xABCDEF).version;
        }
        layers.push_back({pixels.version, mask, quint8(i == 5 ? 0 : 37 + i * 19),
            i % 4 == 3 ? KisGpuTileStore::CompositeOp::Erase : KisGpuTileStore::CompositeOp::Over});
    }
    auto reference = store.fill(old.version, damage, 0);
    for (const auto &layer : layers) {
        reference = layer.mask.tileCount()
            ? store.compositeMasked(reference.version, layer.pixels, layer.mask, damage, layer.operation, layer.opacity)
            : store.composite(reference.version, layer.pixels, damage, layer.operation, layer.opacity);
        QCOMPARE(reference.error, KisGpuTileStore::Error::None);
    }
    const auto before = store.statistics();
    auto projected = store.project(old.version, layers, damage);
    QCOMPARE(projected.error, KisGpuTileStore::Error::None);
    QCOMPARE(store.statistics().submissions, before.submissions + 1);
    layers.clear();
    old = {};
    QVERIFY(finish(store, projected.completion));
    QCOMPARE(gpu.read(projected.version, bounds), gpu.read(reference.version, bounds));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuTileStoreTest::projectionFitsBoundedCommandMemory()
{
    KisGpuTileStore store(m_gpu->owner, 2 * KisGpuTileStore::TileBytes + 2048);
    const QRect bounds(0, 0, 64, 64);
    const auto source = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, source.completion));
    const auto before = store.statistics();
    auto projected = store.project(store.emptyVersion(), QVector<KisGpuTileStore::Layer>(12, {source.version}), bounds);
    QCOMPARE(projected.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, projected.completion));
    QCOMPARE(m_gpu->read(projected.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(store.statistics().submissions, before.submissions + 1);
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}


void KisGpuTileStoreTest::deviceOwnerInvalidatesBeforeDestroyingPendingReads()
{
    auto device = std::make_unique<KisGpuDevice>();
    KisGpuTileStore store(*device, 16 * KisGpuTileStore::TileBytes);
    const auto edit = store.fill(store.emptyVersion(), QRect(0, 0, 64, 64), 0xFFABCDEF);
    const auto read = store.readback(edit.version, QRect(0, 0, 64, 64));
    QCOMPARE(read.error, KisGpuTileStore::Error::None);
    device->destroy();
    bool refused = false;
    try { KisGpuTileStore invalid(*device, 16 * KisGpuTileStore::TileBytes); }
    catch (const std::runtime_error &) { refused = true; }
    QVERIFY(refused);
    QVERIFY(!store.deviceAvailable());
    QCOMPARE(edit.completion.status(), KisGpuTileStore::Status::Failed);
    QCOMPARE(read.completion.status(), KisGpuTileStore::Status::Failed);
    QVERIFY(read.bytes().isEmpty());
    QCOMPARE(store.fill(edit.version, QRect(0, 0, 1, 1), 0).error, KisGpuTileStore::Error::DeviceLost);
    device.reset();
    store.poll();
}

void KisGpuTileStoreTest::nativeDeviceErrorStopsAllDocuments()
{
    KisGpuDevice device;
    KisGpuTileStore first(device, 4 * KisGpuTileStore::TileBytes), second(device, 4 * KisGpuTileStore::TileBytes);
    QVERIFY(first.deviceAvailable());
    QVERIFY(second.deviceAvailable());
    // Queue writes report native loss after explicit native destruction.
    WGPUBufferDescriptor descriptor{};
    descriptor.size = 4;
    descriptor.usage = WGPUBufferUsage_CopyDst;
    const auto probe = wgpuDeviceCreateBuffer(device.device(), &descriptor);
    const auto queue = wgpuDeviceGetQueue(device.device());
    wgpuDeviceDestroy(device.device());
    const quint32 value = 0;
    wgpuQueueWriteBuffer(queue, probe, 0, &value, sizeof(value));
    wgpuBufferRelease(probe);
    wgpuQueueRelease(queue);
    QVERIFY2(!device.available(), qPrintable(device.lastError()));
    QVERIFY(!first.deviceAvailable());
    QVERIFY(!second.deviceAvailable());
    QCOMPARE(first.fill(first.emptyVersion(), QRect(0, 0, 1, 1), 0).error, KisGpuTileStore::Error::DeviceLost);
}

void KisGpuTileStoreTest::compositing_data()
{
    QFile file(QStringLiteral(RASTER_EDIT_FIXTURE));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    QJsonParseError error;
    const auto fixture = QJsonDocument::fromJson(file.readAll(), &error).object();
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(fixture["schema"].toInt(), 1);
    QCOMPARE(fixture["profile"].toString(), QStringLiteral("sRGB-elle-V2-srgbtrc.icc"));
    QTest::addColumn<QJsonObject>("input");
    QTest::addColumn<QRect>("rectangle");
    QTest::addColumn<int>("sourceKind");
    const QList<QRect> rectangles {QRect(-1, -1, 1, 1), QRect(-2, -2, 67, 67), QRect(62, 62, 131, 3)};
    const auto cases = fixture["cases"].toArray();
    QVERIFY(!cases.isEmpty());
    for (const auto &entry : cases) {
        const auto input = entry.toObject();
        for (int i = 0; i < rectangles.size(); ++i) {
            const QByteArray name = input["id"].toString().toUtf8() + '-' + QByteArray::number(i);
            const QByteArray solidName = name + "-solid", imageName = name + "-image";
            QTest::newRow(solidName.constData()) << input << rectangles[i] << 0;
            QTest::newRow(imageName.constData()) << input << rectangles[i] << 1;
            const QByteArray maskedName = name + "-masked";
            QTest::newRow(maskedName.constData()) << input << rectangles[i] << 2;
        }
    }
}

void KisGpuTileStoreTest::compositing()
{
    QFETCH(QJsonObject, input);
    QFETCH(QRect, rectangle);
    QFETCH(int, sourceKind);
    KisGpuTileStore store(m_gpu->owner, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds = rectangle.adjusted(-2, -2, 2, 2);
    const quint32 destination = rgba(input["dst"].toArray());
    const auto base = store.fill(store.emptyVersion(), bounds, destination);
    QRect paintedRect = rectangle;
    const int mask = input["mask"].toInt();
    if (mask >= 0 && rectangle.width() > 2 && rectangle.height() > 2) paintedRect.adjust(1, 1, -1, -1);
    const auto operation = input["op"].toString() == "erase"
        ? KisGpuTileStore::CompositeOp::Erase : KisGpuTileStore::CompositeOp::Over;
    KisGpuTileStore::Edit edit;
    if (sourceKind) {
        const auto source = store.fill(store.emptyVersion(), paintedRect, rgba(input["src"].toArray()));
        if (sourceKind == 2) {
            const auto coverage = store.fill(store.emptyVersion(), paintedRect, quint32(mask < 0 ? 255 : mask) << 24);
            edit = store.compositeMasked(base.version, source.version, coverage.version, rectangle,
                                         operation, quint8(input["opacity"].toInt()));
        } else {
            edit = store.composite(base.version, source.version, paintedRect,
                                   operation, quint8(input["opacity"].toInt()), quint8(mask < 0 ? 255 : mask));
        }
    } else {
        edit = store.paint(base.version, paintedRect, rgba(input["src"].toArray()),
                           operation, quint8(input["opacity"].toInt()), quint8(mask < 0 ? 255 : mask));
    }
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    const QByteArray actual = m_gpu->read(edit.version, bounds);
    const QByteArray wanted = expected(bounds, {{bounds, destination}, {paintedRect, rgba(input["expected"].toArray())}});
    QCOMPARE(actual.size(), wanted.size());
    for (int offset = 0; offset < actual.size(); offset += 4) {
        QVERIFY2(actual.mid(offset, 4) == wanted.mid(offset, 4),
                 qPrintable(QStringLiteral("(%1,%2): expected RGBA %3, actual %4")
                     .arg(bounds.x() + offset / 4 % bounds.width()).arg(bounds.y() + offset / 4 / bounds.width())
                     .arg(QString::fromLatin1(wanted.mid(offset, 4).toHex()), QString::fromLatin1(actual.mid(offset, 4).toHex()))));
    }
    QCOMPARE(m_gpu->read(base.version, bounds), expected(bounds, {{bounds, destination}}));
}

QTEST_GUILESS_MAIN(KisGpuTileStoreTest)
#include "KisGpuTileStoreTest.moc"
