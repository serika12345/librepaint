/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "KisGpuTestDevice.h"
#include "KisGpuEditSession.h"
#include <cstring>

namespace {
using Store = KisGpuTileStore;
quint32 color(const QJsonValue &value) {
    const auto bytes = QByteArray::fromHex(value.toString().toLatin1());
    return quint8(bytes[0]) | quint32(quint8(bytes[1])) << 8 | quint32(quint8(bytes[2])) << 16 | quint32(quint8(bytes[3])) << 24;
}
bool finish(Store &store, const Store::Completion &completion) {
    QElapsedTimer timer; timer.start();
    while (completion.status() == Store::Status::Pending && timer.elapsed() < 5000) { store.poll(); QTest::qWait(1); }
    return completion.status() == Store::Status::Succeeded;
}
Store::DabCommand dab(QPoint point, QPoint pickup, quint8 rate = 255) {
    Store::DabCommand command;
    command.center = point;
    command.diameter = {0.5, 0.5};
    command.pickup = Store::DabCommand::Pickup{pickup, rate};
    return command;
}
}
/** Ordered GPU pickup observes preceding dabs and retains CPU dulling/Copy rounding. */
class KisGpuDabMixingTest : public QObject {
    Q_OBJECT
    std::unique_ptr<KisGpuTestDevice> gpu;
private Q_SLOTS:
    void initTestCase() { gpu = std::make_unique<KisGpuTestDevice>(); }
    void cleanup() { wgpuDevicePoll(gpu->device, true, nullptr); QCOMPARE(gpu->owner.errorCount(), 0); }
    void matchesProductionCpu() {
        QFile file(QStringLiteral(MIXING_FIXTURE));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(fixture["schema"].toInt(), 1);
        Store store(gpu->owner, 8 * 1024 * 1024);
        int index = 0;
        for (const auto &value : fixture["cases"].toArray()) {
            const auto row = value.toArray();
            const auto destination = store.fill(store.emptyVersion(), {0, 0, 1, 1}, color(row[1]));
            const auto source = store.fill(destination.version, {64, 0, 1, 1}, color(row[0]));
            auto command = dab({0, 0}, {64, 0}, quint8(row[2].toInt()));
            command.opacity = quint8(row[3].toInt());
            command.coverage = quint8(row[4].toInt());
            const auto edit = store.paintDabs(source.version, {command}, {0, 0, 1, 1});
            QCOMPARE(edit.error, Store::Error::None);
            QVERIFY(finish(store, edit.completion));
            const auto actual = gpu->read(edit.version, {0, 0, 1, 1});
            const auto expected = QByteArray::fromHex(row[5].toString().toLatin1());
            QVERIFY2(actual == expected, qPrintable(QString("case %1: actual %2 expected %3").arg(index).arg(QString::fromLatin1(actual.toHex()), QString::fromLatin1(expected.toHex()))));
            ++index;
        }
        QCOMPARE(index, 480);
        QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    }
    void selectionAndOrdinaryDabsKeepTheirOrder() {
        Store store(gpu->owner, 32 * Store::TileBytes);
        const QRect bounds(-1, -1, 1, 1);
        const auto base = store.fill(store.emptyVersion(), bounds, 0xFF0000FF);
        const auto source = store.fill(base.version, {64, 64, 1, 1}, 0xFFFF0000);
        const auto selection = store.fill(store.emptyVersion(), bounds, 0x80000000);
        auto mixed = dab({-1, -1}, {64, 64});
        auto solid = mixed;
        solid.pickup.reset(); solid.rgba = 0xFF00FF00;
        const auto first = store.paintDabs(source.version, {solid}, bounds, selection.version);
        const auto second = store.paintDabs(first.version, {mixed}, bounds, selection.version);
        const auto combined = store.paintDabs(source.version, {solid, mixed}, bounds, selection.version);
        QVERIFY(finish(store, combined.completion));
        QCOMPARE(gpu->read(combined.version, bounds), gpu->read(second.version, bounds));
        auto absent = store.paintDabs(base.version, {dab({-1, -1}, {12345, -23456})}, bounds);
        QVERIFY(finish(store, absent.completion));
        QCOMPARE(gpu->read(absent.version, bounds), QByteArray::fromHex("ff0000ff"));
        const auto emptyMask = store.paintDabs(base.version, {mixed}, bounds, store.emptyVersion());
        QCOMPARE(emptyMask.version, base.version);
    }
    void rejectionIsAtomicAndTemporaryStorageIsReleased() {
        Store store(gpu->owner, 8 * Store::TileBytes, 1);
        const auto base = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF0000FF);
        const auto before = store.statistics();
        QCOMPARE(store.paintDabs(base.version, {dab({0, 0}, {0, 0})}, {0, 0, 1, 1}).error, Store::Error::QueueFull);
        QCOMPARE(store.statistics().submissions, before.submissions);
        QVERIFY(finish(store, base.completion));
        {
            const auto texture = store.uploadBrushTexture({1, 1}, QByteArray(1, char(255)));
            QVERIFY(finish(store, texture.completion));
            const auto beforeTextureMix = store.statistics();
            QCOMPARE(store.paintDabs(base.version, {dab({0, 0}, {0, 0})}, {0, 0, 1, 1}, texture.texture, {}).error,
                     Store::Error::InvalidCommand);
            QCOMPARE(store.statistics().submissions, beforeTextureMix.submissions);
            QCOMPARE(store.statistics().residentBytes, beforeTextureMix.residentBytes);
        }
        QVector<Store::DabCommand> many(17, dab({0, 0}, {0, 0}));
        const auto settled = store.statistics();
        QCOMPARE(store.paintDabs(base.version, many, {0, 0, 1, 1}).error, Store::Error::InvalidCommand);
        auto erase = dab({0, 0}, {0, 0}); erase.operation = Store::CompositeOp::Erase;
        QCOMPARE(store.paintDabs(base.version, {erase}, {0, 0, 1, 1}).error, Store::Error::InvalidCommand);
        QCOMPARE(store.statistics().residentBytes, settled.residentBytes);
        QCOMPARE(store.statistics().submissions, settled.submissions);
        Store tiny(gpu->owner, Store::TileBytes + 99);
        QCOMPARE(tiny.paintDabs(tiny.emptyVersion(), {dab({0, 0}, {0, 0})}, {0, 0, 1, 1}).error, Store::Error::BudgetExceeded);
        QCOMPARE(tiny.statistics().residentBytes, quint64(0));
        QCOMPARE(tiny.statistics().submissions, quint64(0));
        {
            const auto edit = store.paintDabs(base.version, {dab({0, 0}, {0, 0})}, {0, 0, 1, 1});
            QVERIFY(finish(store, edit.completion));
            QCOMPARE(store.statistics().residentBytes, 2 * Store::TileBytes);
        }
        QCOMPARE(store.statistics().residentBytes, Store::TileBytes);
        KisGpuTestDevice lost;
        Store loss(lost.owner, 8 * Store::TileBytes);
        auto pending = loss.paintDabs(loss.emptyVersion(), {dab({0, 0}, {0, 0})}, {0, 0, 1, 1});
        loss.invalidateDevice(); loss.poll();
        QCOMPARE(pending.completion.status(), Store::Status::Failed);
        QCOMPARE(loss.paintDabs(loss.emptyVersion(), {dab({0, 0}, {0, 0})}, {0, 0, 1, 1}).error, Store::Error::DeviceLost);
    }
    void replacementAndHistoryRecoverOnNewDevice() {
        using Session = KisGpuEditSession;
        const QRect bounds(0, 0, 3, 1);
        std::optional<Session::Recovery> recovery;
        QByteArray accepted;
        {
            KisGpuTestDevice original;
            Store store(original.owner, 32 * Store::TileBytes);
            Session session(store, 2);
            auto token = session.begin();
            auto solid = dab({0, 0}, {}); solid.pickup.reset(); solid.rgba = 0xFF0000FF;
            QCOMPARE(session.append(token, {solid}, bounds), Session::Result::Accepted);
            QCOMPARE(session.commit(token), Session::Result::Accepted);
            QTRY_VERIFY_WITH_TIMEOUT((session.poll(), session.state() == Session::State::Idle), 5000);
            token = session.begin();
            QCOMPARE(session.append(token, {dab({1, 0}, {0, 0})}, bounds), Session::Result::Accepted);
            QCOMPARE(session.replace(token, {dab({2, 0}, {0, 0})}, bounds), Session::Result::Accepted);
            QCOMPARE(session.commit(token), Session::Result::Accepted);
            QTRY_VERIFY_WITH_TIMEOUT((session.poll(), session.state() == Session::State::Idle), 5000);
            accepted = original.read(session.head(), bounds);
            QCOMPARE(accepted, QByteArray::fromHex("ff0000ff00000000ff0000cc"));
            recovery = session.recovery();
            original.owner.destroy();
        }
        Store store(gpu->owner, 32 * Store::TileBytes);
        Session session(store, 2, *recovery);
        QTRY_VERIFY_WITH_TIMEOUT((session.poll(), session.state() == Session::State::Idle), 5000);
        QCOMPARE(gpu->read(session.head(), bounds), accepted);
        QVERIFY(session.undo());
        QCOMPARE(gpu->read(session.head(), bounds), QByteArray::fromHex("ff0000ff0000000000000000"));
        QVERIFY(session.redo());
        QCOMPARE(gpu->read(session.head(), bounds), accepted);
        QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    }
    void timingAccountsForEveryOrderedPass() {
        KisGpuTestDevice timed(0, true);
        Store store(timed.owner, 32 * Store::TileBytes);
        const auto base = store.fill(store.emptyVersion(), {0, 0, 1, 1}, 0xFF0000FF);
        QVERIFY(finish(store, base.completion));
        QVector<Store::DabCommand> commands(16, dab({0, 0}, {0, 0}));
        const auto before = store.statistics();
        const auto edit = store.paintDabs(base.version, commands, {0, 0, 1, 1});
        QCOMPARE(edit.error, Store::Error::None);
        QVERIFY(finish(store, edit.completion));
        QVERIFY(edit.completion.gpuComputeNanoseconds().has_value());
        QCOMPARE(store.statistics().timingReadbackBytes - before.timingReadbackBytes, quint64(16 * 16));
        QCOMPARE(store.statistics().pickupCopyBytes - before.pickupCopyBytes, quint64(16 * 4));
        QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(16));
        QCOMPARE(store.statistics().residentBytes, 2 * Store::TileBytes);
        QCOMPARE(timed.read(edit.version, {0, 0, 1, 1}), QByteArray::fromHex("ff0000ff"));
        QCOMPARE(timed.owner.errorCount(), 0);
    }
    void eachPickupSeesPriorDabAcrossTiles() {
        Store store(gpu->owner, 16 * Store::TileBytes);
        auto red = store.fill(store.emptyVersion(), {63, -1, 1, 1}, 0xFF0000FF);
        auto blue = store.fill(red.version, {64, -1, 1, 1}, 0xFFFF0000);
        const auto before = store.statistics();
        const auto edit = store.paintDabs(blue.version,
            {dab({64, -1}, {63, -1}), dab({65, -1}, {64, -1})}, {64, -1, 2, 1});
        QCOMPARE(edit.error, Store::Error::None);
        QVERIFY(finish(store, edit.completion));
        QCOMPARE(gpu->read(edit.version, {63, -1, 3, 1}), QByteArray::fromHex("ff0000ffcc0033ffcc0033cc"));
        QCOMPARE(gpu->read(blue.version, {63, -1, 3, 1}), QByteArray::fromHex("ff0000ff0000ffff00000000"));
        QCOMPARE(store.statistics().submissions - before.submissions, quint64(1));
        QCOMPARE(store.statistics().tileCopyBytes - before.tileCopyBytes, Store::TileBytes);
        QCOMPARE(store.statistics().computeDispatches - before.computeDispatches, quint64(2));
        QCOMPARE(store.statistics().pickupCopyBytes - before.pickupCopyBytes, quint64(8));
    }
};
QTEST_GUILESS_MAIN(KisGpuDabMixingTest)
#include "KisGpuDabMixingTest.moc"
