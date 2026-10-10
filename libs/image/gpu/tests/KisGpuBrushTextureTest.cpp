/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "KisGpuTileStore.h"
#include "KisGpuEditSession.h"
#include "KisGpuTestDevice.h"
#include <limits>

namespace {
using Store = KisGpuTileStore;
using Session = KisGpuEditSession;
bool finish(Store &store, const Store::Completion &completion) {
    QElapsedTimer timer;
    timer.start();
    while (completion.status() == Store::Status::Pending && timer.elapsed() < 5000) { store.poll(); QTest::qWait(1); }
    return completion.status() == Store::Status::Succeeded;
}
Store::DabCommand dab(quint32 rgba) {
    Store::DabCommand result;
    result.center = {0, 0};
    result.diameter = {4096, 4096};
    result.rgba = rgba;
    return result;
}
void settle(Session &session) {
    QElapsedTimer timer;
    timer.start();
    do { session.poll(); QTest::qWait(1); }
    while ((session.state() == Session::State::Committing || session.state() == Session::State::Restoring) && timer.elapsed() < 5000);
}
}
/** Texture assets upload once; edits consume immutable masks and recover their prepared source. */
class KisGpuBrushTextureTest : public QObject
{
    Q_OBJECT
    std::unique_ptr<KisGpuTestDevice> gpu;
private Q_SLOTS:
    void initTestCase() { gpu = std::make_unique<KisGpuTestDevice>(); }
    void cleanup() { wgpuDevicePoll(gpu->device, true, nullptr); QCOMPARE(gpu->owner.errorCount(), 0); }
    void alphaMatchesCpuFixture() {
        QFile file(QStringLiteral(TEXTURE_FIXTURE));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
        const auto alpha = fixture["alpha"].toArray();
        const auto products = fixture["products"].toArray();
        QByteArray mask;
        for (const auto &value : alpha) mask.append(char(value.toInt()));
        Store store(gpu->owner, 4 * 1024 * 1024);
        const auto texture = store.uploadBrushTexture({int(alpha.size()), 1}, mask);
        QCOMPARE(texture.error, Store::Error::None);
        // A pending asset is accepted in queue order, with no CPU wait or reupload.
        for (int row = 0; row < alpha.size(); ++row) {
            const QRect bounds(0, 0, int(alpha.size()), 1);
            const auto edit = store.paintDabs(store.emptyVersion(), {dab((quint32(alpha[row].toInt()) << 24) | 0xC75311)},
                                              bounds, texture.texture, {});
            QCOMPARE(edit.error, Store::Error::None);
            QVERIFY(finish(store, edit.completion));
            const auto pixels = gpu->read(edit.version, bounds);
            const auto expected = QByteArray::fromHex(products[row].toString().toLatin1());
            for (int x = 0; x < expected.size(); ++x) {
                QCOMPARE(quint8(pixels[x * 4 + 3]), quint8(expected[x]));
                QCOMPARE(pixels.mid(x * 4, 3), expected[x] ? QByteArray::fromHex("1153c7") : QByteArray(3, '\0'));
            }
        }
        QCOMPARE(store.statistics().pixelUploadBytes, quint64(mask.size()));
        QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    }
    void phaseSelectionEraseAndRepeatedUse() {
        Store store(gpu->owner, 4 * 1024 * 1024);
        const QRect bounds(-65, -65, 194, 3);
        const auto texture = store.uploadBrushTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"));
        const QPoint origin(2, -3);
        const auto base = store.fill(store.emptyVersion(), bounds, 0xC7604020);
        const auto selection = store.fill(store.emptyVersion(), bounds, 0x75443322);
        for (const auto operation : {Store::CompositeOp::Over, Store::CompositeOp::Erase}) {
            auto red = dab(0xC10000FF), blue = dab(0xC1FF0000);
            red.operation = blue.operation = operation;
            red.opacity = blue.opacity = 155;
            red.coverage = blue.coverage = 201;
            const auto before = store.statistics();
            const auto actual = store.paintDabs(base.version, {red, blue}, bounds, texture.texture, origin, &selection.version);
            QVERIFY(finish(store, actual.completion));
            QCOMPARE(store.statistics().submissions, before.submissions + 1);
            QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
            QVector<Store::DabCommand> expectedCommands;
            for (int y = bounds.y(); y < bounds.y() + bounds.height(); ++y) {
                for (int x = bounds.x(); x < bounds.x() + bounds.width(); ++x) {
                    const int tx = ((x - origin.x()) % 3 + 3) % 3, ty = ((y - origin.y()) % 2 + 2) % 2;
                    const quint32 mask = quint8(QByteArray::fromHex("0080ff0140fe")[ty * 3 + tx]);
                    const quint32 p = 193 * mask * 255 + 0x7F5B, alpha = ((p >> 7) + p) >> 16;
                    red.rgba = (alpha << 24) | 0xFF;
                    blue.rgba = (alpha << 24) | 0xFF0000;
                    red.center = blue.center = QPointF(x, y);
                    red.diameter = blue.diameter = QSizeF(.5, .5);
                    expectedCommands.append(red);
                    expectedCommands.append(blue);
                }
            }
            const auto expected = store.paintDabs(base.version, expectedCommands, bounds, selection.version);
            QVERIFY(finish(store, expected.completion));
            QCOMPARE(gpu->read(actual.version, bounds), gpu->read(expected.version, bounds));
        }
    }
    void rejectionAndResourceLifetime() {
        Store store(gpu->owner, 4 * Store::TileBytes, 1), other(gpu->owner, 4 * Store::TileBytes);
        QCOMPARE(store.uploadBrushTexture({}, {}).error, Store::Error::InvalidCommand);
        QCOMPARE(store.uploadBrushTexture({2, 2}, QByteArray(3, 'x')).error, Store::Error::InvalidCommand);
        QCOMPARE(store.uploadBrushTexture({65536, 65536}, {}).error, Store::Error::InvalidCommand);
        QCOMPARE(store.statistics().residentBytes, quint64(0));
        auto texture = store.uploadBrushTexture({3, 1}, QByteArray::fromHex("4080ff"));
        QCOMPARE(texture.error, Store::Error::None);
        QCOMPARE(store.uploadBrushTexture({1, 1}, QByteArray(1, 'x')).error, Store::Error::QueueFull);
        QVERIFY(finish(store, texture.completion));
        QCOMPARE(other.paintDabs(other.emptyVersion(), {dab(0xFF0000FF)}, {0, 0, 1, 1}, texture.texture, {}).error,
                 Store::Error::InvalidVersion);
        auto edit = store.paintDabs(store.emptyVersion(), {dab(0xFF0000FF)}, {0, 0, 1, 1}, texture.texture, {});
        texture = {};
        QVERIFY(finish(store, edit.completion));
        QCOMPARE(gpu->read(edit.version, {0, 0, 1, 1}), QByteArray::fromHex("ff000040"));
        QCOMPARE(store.statistics().residentBytes, Store::TileBytes);
        Store tiny(gpu->owner, 3);
        QCOMPARE(tiny.uploadBrushTexture({3, 1}, QByteArray(3, 'x')).error, Store::Error::BudgetExceeded);
        QCOMPARE(tiny.statistics().submissions, quint64(0));
        Store::BrushTexture retained;
        {
            Store transient(gpu->owner, 4 * Store::TileBytes);
            retained = transient.uploadBrushTexture({1, 1}, QByteArray(1, 'x')).texture;
        }
        QVERIFY(bool(retained));
        KisGpuTestDevice lost;
        Store lossStore(lost.owner, 4 * Store::TileBytes);
        auto pending = lossStore.uploadBrushTexture({1, 1}, QByteArray(1, 'x'));
        lossStore.invalidateDevice();
        lossStore.poll();
        QCOMPARE(pending.completion.status(), Store::Status::Failed);
        QCOMPARE(lossStore.uploadBrushTexture({1, 1}, QByteArray(1, 'x')).error, Store::Error::DeviceLost);
    }
    void wideCanvasCoordinatesKeepPatternPhase() {
        Store store(gpu->owner, 4 * Store::TileBytes);
        const auto texture = store.uploadBrushTexture({3, 1}, QByteArray::fromHex("4080ff"));
        const int x = std::numeric_limits<int>::max() - 4, origin = std::numeric_limits<int>::min();
        auto command = dab(0xFF0000FF);
        command.center = QPointF(x, 0);
        const auto edit = store.paintDabs(store.emptyVersion(), {command}, {x, 0, 1, 1}, texture.texture, {origin, 0});
        QVERIFY(finish(store, edit.completion));
        const auto alpha = QByteArray::fromHex("4080ff")[int((qint64(x) - origin) % 3)];
        const auto pixels = gpu->read(edit.version, {x, 0, 1, 1});
        QCOMPARE(pixels, QByteArray::fromHex("ff0000") + QByteArray(1, alpha));
    }
    void recoveryCountsSharedTextureOnceAndRejectsBeforeGpu() {
        Store store(gpu->owner, 16 * Store::TileBytes);
        Session session(store, 2);
        const auto alpha = QByteArray(1024, char(128));
        const auto texture = session.createTexture({32, 32}, alpha).texture;
        const auto token = session.begin();
        const QRect bounds(0, 0, 2, 1);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(session.append(token, {dab(0xFF0000FF)}, bounds, texture), Session::Result::Accepted);
        session.commit(token);
        settle(session);
        const quint64 budget = sizeof(Session::ReplayTexture) + alpha.size() + 2 * sizeof(Session::ReplayEdit)
            + 4 * (sizeof(Session::ReplayBatch) + sizeof(Store::DabCommand));
        Store destination(gpu->owner, 16 * Store::TileBytes);
        Session restored(destination, 2, *session.recovery(), budget);
        settle(restored);
        QCOMPARE(restored.state(), Session::State::Idle);
        QCOMPARE(gpu->read(restored.head(), bounds), gpu->read(session.head(), bounds));
        const auto fresh = restored.createTexture({32, 32}, alpha).texture;
        const auto next = restored.begin();
        const auto before = destination.statistics().submissions;
        QCOMPARE(restored.append(next, {dab(0xFFFF0000)}, bounds, fresh), Session::Result::RecoveryBudgetExceeded);
        QCOMPARE(destination.statistics().submissions, before);
        QVERIFY(restored.recovery()->working->isEmpty());
        QCOMPARE(restored.replace(next, {}, bounds, Session::Texture{}), Session::Result::GpuRejected);
        QCOMPARE(destination.statistics().submissions, before);
        Session tiny(destination, 2, Session::Recovery{}, 1);
        settle(tiny);
        QCOMPARE(tiny.createTexture({32, 32}, alpha).result, Session::Result::RecoveryBudgetExceeded);
    }
    void selectedTextureReplacementRecoversHistory() {
        const QRect bounds(-4, -2, 8, 4);
        std::optional<Session::Recovery> recovery;
        QByteArray accepted;
        {
            KisGpuTestDevice old;
            Store store(old.owner, 16 * Store::TileBytes);
            Session session(store, 2);
            auto texture = session.createTexture({3, 1}, QByteArray::fromHex("0080ff"), {1, -1});
            QCOMPARE(texture.result, Session::Result::Accepted);
            const auto selection = session.createSelection({dab(0x80123456)}, bounds);
            const auto token = session.begin();
            QCOMPARE(session.append(token, {dab(0xFF0000FF)}, bounds, texture.texture, selection.selection), Session::Result::Accepted);
            QCOMPARE(session.replace(token, {dab(0xFFFF0000)}, bounds, texture.texture, selection.selection), Session::Result::Accepted);
            texture = {};
            session.commit(token);
            settle(session);
            QCOMPARE(session.state(), Session::State::Idle);
            accepted = old.read(session.head(), bounds);
            recovery = session.recovery();
            QVERIFY(recovery.has_value());
            old.owner.destroy();
        }
        Store store(gpu->owner, 16 * Store::TileBytes);
        Session restored(store, 2, *recovery);
        settle(restored);
        QCOMPARE(restored.state(), Session::State::Idle);
        QCOMPARE(gpu->read(restored.head(), bounds), accepted);
        QVERIFY(restored.undo());
        QCOMPARE(gpu->read(restored.head(), bounds), QByteArray(bounds.width() * bounds.height() * 4, '\0'));
        QVERIFY(restored.redo());
        QCOMPARE(gpu->read(restored.head(), bounds), accepted);
    }
};
QTEST_GUILESS_MAIN(KisGpuBrushTextureTest)
#include "KisGpuBrushTextureTest.moc"
