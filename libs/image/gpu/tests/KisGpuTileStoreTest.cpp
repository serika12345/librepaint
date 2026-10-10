/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <atomic>
#include <future>
#include <map>
#include "KisGpuTileStore.h"

namespace {
int tileCoordinate(int pixel) { return pixel >= 0 ? pixel / 64 : (pixel + 1) / 64 - 1; }

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
    void compositing_data();
    void compositing();
private:
    QByteArray read(const KisGpuTileStore::Version &version, QRect bounds);
    WGPUInstance m_instance = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUQueue m_queue = nullptr;
    std::atomic<int> m_errors{0};
};

void KisGpuTileStoreTest::initTestCase()
{
    WGPUInstanceExtras extras{};
    extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
#ifdef __APPLE__
    extras.backends = WGPUInstanceBackend_Metal;
#else
    extras.backends = WGPUInstanceBackend_Vulkan;
#endif
    extras.flags = WGPUInstanceFlag_Validation;
    WGPUInstanceDescriptor instanceDescriptor{};
    instanceDescriptor.nextInChain = &extras.chain;
    m_instance = wgpuCreateInstance(&instanceDescriptor);
    QVERIFY(m_instance);

    std::promise<WGPUAdapter> adapterPromise;
    auto adapterFuture = adapterPromise.get_future();
    WGPURequestAdapterCallbackInfo adapterCallback{};
    adapterCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    adapterCallback.userdata1 = &adapterPromise;
    adapterCallback.callback = [](WGPURequestAdapterStatus, WGPUAdapter adapter, WGPUStringView, void *data, void *) {
        static_cast<std::promise<WGPUAdapter> *>(data)->set_value(adapter);
    };
    WGPURequestAdapterOptions options{};
    wgpuInstanceRequestAdapter(m_instance, &options, adapterCallback);
    m_adapter = adapterFuture.get();
    QVERIFY2(m_adapter, "A hardware Metal/Vulkan adapter is required for the GPU contract");
    WGPUAdapterInfo info{};
    wgpuAdapterGetInfo(m_adapter, &info);
    const auto adapterType = info.adapterType;
    qInfo() << "GPU:" << QByteArray(info.device.data, qsizetype(info.device.length));
    wgpuAdapterInfoFreeMembers(info);
    QVERIFY(adapterType != WGPUAdapterType_CPU);

    std::promise<WGPUDevice> devicePromise;
    auto deviceFuture = devicePromise.get_future();
    WGPURequestDeviceCallbackInfo deviceCallback{};
    deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallback.userdata1 = &devicePromise;
    deviceCallback.callback = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void *data, void *) {
        static_cast<std::promise<WGPUDevice> *>(data)->set_value(device);
    };
    WGPUDeviceDescriptor descriptor{};
    descriptor.uncapturedErrorCallbackInfo.userdata1 = &m_errors;
    descriptor.uncapturedErrorCallbackInfo.callback = [](const WGPUDevice *, WGPUErrorType, WGPUStringView message, void *data, void *) {
        static_cast<std::atomic<int> *>(data)->fetch_add(1);
        qWarning() << "GPU validation:" << QByteArray(message.data, qsizetype(message.length));
    };
    wgpuAdapterRequestDevice(m_adapter, &descriptor, deviceCallback);
    m_device = deviceFuture.get();
    QVERIFY(m_device);
    m_queue = wgpuDeviceGetQueue(m_device);
}

void KisGpuTileStoreTest::cleanup()
{
    if (m_device) wgpuDevicePoll(m_device, true, nullptr);
    QCOMPARE(m_errors.load(), 0);
}

void KisGpuTileStoreTest::cleanupTestCase()
{
    if (m_queue) wgpuQueueRelease(m_queue);
    if (m_device) wgpuDeviceRelease(m_device);
    if (m_adapter) wgpuAdapterRelease(m_adapter);
    if (m_instance) wgpuInstanceRelease(m_instance);
}

QByteArray KisGpuTileStoreTest::read(const KisGpuTileStore::Version &version, QRect bounds)
{
    std::map<std::pair<int, int>, QByteArray> tiles;
    QByteArray result(bounds.width() * bounds.height() * 4, '\0');
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const auto coordinate = std::make_pair(tileCoordinate(x), tileCoordinate(y));
            const WGPUBuffer source = version.tile(QPoint(coordinate.first, coordinate.second));
            if (!source) continue;
            auto found = tiles.find(coordinate);
            if (found == tiles.end()) {
                WGPUBufferDescriptor descriptor{};
                descriptor.size = KisGpuTileStore::TileBytes;
                descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
                const auto staging = wgpuDeviceCreateBuffer(m_device, &descriptor);
                const auto encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);
                wgpuCommandEncoderCopyBufferToBuffer(encoder, source, 0, staging, 0, descriptor.size);
                const auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
                wgpuQueueSubmit(m_queue, 1, &commands);
                wgpuCommandBufferRelease(commands);
                wgpuCommandEncoderRelease(encoder);
                std::promise<WGPUMapAsyncStatus> promise;
                auto future = promise.get_future();
                WGPUBufferMapCallbackInfo callback{};
                callback.mode = WGPUCallbackMode_AllowSpontaneous;
                callback.userdata1 = &promise;
                callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
                    static_cast<std::promise<WGPUMapAsyncStatus> *>(data)->set_value(status);
                };
                wgpuBufferMapAsync(staging, WGPUMapMode_Read, 0, descriptor.size, callback);
                wgpuDevicePoll(m_device, true, nullptr);
                const auto status = future.get();
                QByteArray bytes;
                if (status == WGPUMapAsyncStatus_Success) {
                    bytes = QByteArray(static_cast<const char *>(wgpuBufferGetConstMappedRange(staging, 0, descriptor.size)), descriptor.size);
                    wgpuBufferUnmap(staging);
                }
                wgpuBufferRelease(staging);
                if (status != WGPUMapAsyncStatus_Success) return {};
                found = tiles.emplace(coordinate, bytes).first;
            }
            const int sourceOffset = ((y - coordinate.second * 64) * 64 + x - coordinate.first * 64) * 4;
            const int targetOffset = ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4;
            memcpy(result.data() + targetOffset, found->second.constData() + sourceOffset, 4);
        }
    }
    return result;
}

void KisGpuTileStoreTest::sparseSignedCoordinates()
{
    KisGpuTileStore store(m_device, 8 * KisGpuTileStore::TileBytes);
    const auto empty = store.emptyVersion();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    const auto edit = store.fill(empty, QRect(-1, -1, 2, 2), 0x80402010);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QCOMPARE(edit.version.tileCount(), qsizetype(4));
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(read(edit.version, QRect(-65, -65, 130, 130)),
             expected(QRect(-65, -65, 130, 130), {{QRect(-1, -1, 2, 2), 0x80402010}}));
    QCOMPARE(empty.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, 4 * KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
}

void KisGpuTileStoreTest::copiesOnlyChangedTiles()
{
    KisGpuTileStore store(m_device, 3 * KisGpuTileStore::TileBytes + 1024);
    const QRect bounds(0, 0, 128, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF102030);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, QRect(2, 3, 4, 5), 0x00406080);
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(read(base.version, bounds), expected(bounds, {{bounds, 0xFF102030}}));
    QCOMPARE(read(edit.version, bounds), expected(bounds, {{bounds, 0xFF102030}, {QRect(2, 3, 4, 5), 0x00406080}}));
    QCOMPARE(store.statistics().tileCopyBytes, KisGpuTileStore::TileBytes);
    QCOMPARE(store.statistics().residentBytes, 3 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::orderedVersionsAndCancellation()
{
    KisGpuTileStore store(m_device, 16 * KisGpuTileStore::TileBytes);
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
    QCOMPARE(read(first.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}}));
    QCOMPARE(read(second.version, bounds), expected(bounds, {{QRect(-1, 2, 66, 4), 0xFF112233}, {QRect(63, 3, 3, 5), 0x80445566}}));
}

void KisGpuTileStoreTest::fullOverwriteAvoidsCopy()
{
    KisGpuTileStore store(m_device, 3 * KisGpuTileStore::TileBytes);
    const QRect bounds(-64, -64, 64, 64);
    const auto base = store.fill(store.emptyVersion(), bounds, 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto edit = store.fill(base.version, bounds, 0x00112233);
    QVERIFY(finish(store, edit.completion));
    QCOMPARE(read(base.version, bounds), expected(bounds, {{bounds, 0xFF123456}}));
    QCOMPARE(read(edit.version, bounds), expected(bounds, {{bounds, 0x00112233}}));
    QCOMPARE(store.statistics().tileCopyBytes, quint64(0));
    QCOMPARE(store.statistics().residentBytes, 2 * KisGpuTileStore::TileBytes);
}

void KisGpuTileStoreTest::budgetRejectionIsAtomic()
{
    KisGpuTileStore store(m_device, 3 * KisGpuTileStore::TileBytes);
    const auto base = store.fill(store.emptyVersion(), QRect(0, 0, 1, 1), 0xFF123456);
    QVERIFY(finish(store, base.completion));
    const auto before = store.statistics();
    const auto rejected = store.fill(base.version, QRect(-64, 0, 192, 1), 0xFFABCDEF);
    QCOMPARE(rejected.error, KisGpuTileStore::Error::BudgetExceeded);
    QCOMPARE(rejected.version.tileCount(), qsizetype(0));
    QCOMPARE(store.statistics().residentBytes, before.residentBytes);
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().commandUploadBytes, before.commandUploadBytes);
    QCOMPARE(read(base.version, QRect(-1, -1, 3, 3)), expected(QRect(-1, -1, 3, 3), {{QRect(0, 0, 1, 1), 0xFF123456}}));
    const auto huge = store.fill(base.version, QRect(-1000000000, -1000000000, 2000000000, 2000000000), 0);
    QCOMPARE(huge.error, KisGpuTileStore::Error::BudgetExceeded);
}

void KisGpuTileStoreTest::pendingResourcesRemainBudgeted()
{
    KisGpuTileStore store(m_device, KisGpuTileStore::TileBytes + 256);
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
    KisGpuTileStore store(m_device, 0);
    KisGpuTileStore other(m_device, 0);
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
        KisGpuTileStore store(m_device, 2 * KisGpuTileStore::TileBytes);
        const auto edit = store.fill(store.emptyVersion(), QRect(5, 7, 2, 3), 0x00112233);
        version = edit.version;
        completion = edit.completion;
    }
    QCOMPARE(completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(5, 7, 2, 3), 0x00112233}}));
}

void KisGpuTileStoreTest::emptyEditInheritsSourceCompletion()
{
    KisGpuTileStore store(m_device, 2 * KisGpuTileStore::TileBytes);
    const auto source = store.fill(store.emptyVersion(), QRect(1, 1, 2, 2), 0xFF123456);
    const auto empty = store.fill(source.version, QRect(), 0);
    QCOMPARE(empty.error, KisGpuTileStore::Error::None);
    QCOMPARE(empty.completion.sequence(), source.completion.sequence());
    QVERIFY(finish(store, empty.completion));
    QCOMPARE(source.completion.status(), KisGpuTileStore::Status::Succeeded);
    QCOMPARE(store.statistics().submissions, quint64(1));
    QCOMPARE(read(empty.version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 0xFF123456}}));
}

void KisGpuTileStoreTest::repeatedEditsReleaseOldVersions()
{
    KisGpuTileStore store(m_device, 2 * KisGpuTileStore::TileBytes + 256);
    auto version = store.emptyVersion();
    for (quint32 i = 1; i <= 64; ++i) {
        const auto edit = store.fill(version, QRect(1, 1, 2, 2), i);
        QCOMPARE(edit.error, KisGpuTileStore::Error::None);
        QVERIFY(finish(store, edit.completion));
        version = edit.version;
        QCOMPARE(store.statistics().residentBytes, KisGpuTileStore::TileBytes);
    }
    QCOMPARE(read(version, QRect(0, 0, 64, 64)), expected(QRect(0, 0, 64, 64), {{QRect(1, 1, 2, 2), 64}}));
    QCOMPARE(store.statistics().submissions, quint64(64));
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
    const QList<QRect> rectangles {QRect(-1, -1, 1, 1), QRect(-2, -2, 67, 67), QRect(62, 62, 131, 3)};
    const auto cases = fixture["cases"].toArray();
    QVERIFY(!cases.isEmpty());
    for (const auto &entry : cases) {
        const auto input = entry.toObject();
        for (int i = 0; i < rectangles.size(); ++i) {
            const QByteArray name = input["id"].toString().toUtf8() + '-' + QByteArray::number(i);
            QTest::newRow(name.constData()) << input << rectangles[i];
        }
    }
}

void KisGpuTileStoreTest::compositing()
{
    QFETCH(QJsonObject, input);
    QFETCH(QRect, rectangle);
    KisGpuTileStore store(m_device, 128 * KisGpuTileStore::TileBytes);
    const QRect bounds = rectangle.adjusted(-2, -2, 2, 2);
    const quint32 destination = rgba(input["dst"].toArray());
    const auto base = store.fill(store.emptyVersion(), bounds, destination);
    QRect paintedRect = rectangle;
    const int mask = input["mask"].toInt();
    if (mask >= 0 && rectangle.width() > 2 && rectangle.height() > 2) paintedRect.adjust(1, 1, -1, -1);
    const auto operation = input["op"].toString() == "erase"
        ? KisGpuTileStore::CompositeOp::Erase : KisGpuTileStore::CompositeOp::Over;
    const auto edit = store.paint(base.version, paintedRect, rgba(input["src"].toArray()),
                                  operation, quint8(input["opacity"].toInt()), quint8(mask < 0 ? 255 : mask));
    QCOMPARE(edit.error, KisGpuTileStore::Error::None);
    QVERIFY(finish(store, edit.completion));
    const QByteArray actual = read(edit.version, bounds);
    const QByteArray wanted = expected(bounds, {{bounds, destination}, {paintedRect, rgba(input["expected"].toArray())}});
    QCOMPARE(actual.size(), wanted.size());
    for (int offset = 0; offset < actual.size(); offset += 4) {
        QVERIFY2(actual.mid(offset, 4) == wanted.mid(offset, 4),
                 qPrintable(QStringLiteral("(%1,%2): expected RGBA %3, actual %4")
                     .arg(bounds.x() + offset / 4 % bounds.width()).arg(bounds.y() + offset / 4 / bounds.width())
                     .arg(QString::fromLatin1(wanted.mid(offset, 4).toHex()), QString::fromLatin1(actual.mid(offset, 4).toHex()))));
    }
    QCOMPARE(read(base.version, bounds), expected(bounds, {{bounds, destination}}));
}

QTEST_GUILESS_MAIN(KisGpuTileStoreTest)
#include "KisGpuTileStoreTest.moc"
