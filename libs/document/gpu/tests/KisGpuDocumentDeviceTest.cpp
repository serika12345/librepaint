/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentDevice.h"
#include <QtTest>

namespace {
using Host = KisGpuDocumentDevice;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Store = KisGpuTileStore;
const QRect Bounds(0, 0, 64, 64);
Store::DabCommand dab(quint32 rgba) {
    Store::DabCommand result;
    result.center = {20, 20}; result.diameter = {10, 10}; result.rgba = rgba;
    return result;
}
bool settle(Host &host, const std::shared_ptr<Document> &document) {
    QElapsedTimer timeout; timeout.start();
    while ((document->state() == Document::State::Committing || document->state() == Document::State::Restoring)
           && timeout.elapsed() < 5000) { host.poll(); QTest::qWait(1); }
    return document->state() == Document::State::Idle || document->state() == Document::State::Editing;
}
bool paint(Host &host, const std::shared_ptr<Document> &document, quint32 rgba) {
    auto edit = document->begin(1);
    return edit.result == Document::Result::Accepted
        && edit.session->append(edit.token, {dab(rgba)}, Bounds) == Session::Result::Accepted
        && document->commit(edit.token) == Document::Result::Accepted && settle(host, document);
}
QByteArray read(Host &host, Host::DocumentId id, const Store::Version &version) {
    auto store = host.store(id);
    auto result = store->readback(version, Bounds);
    QElapsedTimer timeout; timeout.start();
    while (result.completion.status() == Store::Status::Pending && timeout.elapsed() < 5000) {
        host.poll(); QTest::qWait(1);
    }
    return result.bytes();
}
}

/** Shared device lifetime preserves independent document identities and accepted editing input. */
class KisGpuDocumentDeviceTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void registeredAndIndependentDevicesShareInitializationAndKeepIndependentLifetimes() {
        Host host;
        const auto id = host.createDocument({});
        QVERIFY(id);
        KisGpuDevice independent;
        QCOMPARE(host.device()->instance(), independent.instance());
        QVERIFY(host.device()->device() != independent.device());
        const auto instance = independent.instance();
        QVERIFY(host.suspend());
        QVERIFY(independent.available());
        QVERIFY(host.resume());
        QCOMPARE(host.device()->instance(), instance);
        independent.destroy();
        QVERIFY(host.device()->available());
        QCOMPARE(host.device()->errorCount(), 0);
    }
    void emptyLifecycleAndInvalidValuesKeepDeviceCreationLazy() {
        Host host;
        QVERIFY(!host.device());
        QCOMPARE(host.restoreDocument({}, Document::Recovery{}), Host::DocumentId(0));
        QVERIFY(!host.device());
        QVERIFY(host.suspend());
        QVERIFY(host.resume());
        QVERIFY(!host.device());
        QVERIFY(!host.isSuspended());
    }
    void sharedPressureReleasesHistoryAndRetriesTheSameInput() {
        Host host(32 * Store::TileBytes);
        Host::Limits limits; limits.documentBytes = 32 * Store::TileBytes;
        const auto first = host.createDocument(limits), second = host.createDocument(limits);
        auto a = host.document(first), b = host.document(second);
        QVERIFY(a && b);
        a->addLayer(QStringLiteral("First")); b->addLayer(QStringLiteral("Second"));
        for (quint32 color : {0xFF0000FFu, 0xFFFF0000u, 0xFF00FF00u}) {
            QVERIFY(paint(host, a, color)); QVERIFY(paint(host, b, color));
        }
        auto editing = a->begin(1);
        auto occupied = host.device()->reserveMemory(host.device()->availableMemory());
        QVERIFY(occupied);
        QCOMPARE(editing.session->append(editing.token, {dab(0xFF123456)}, Bounds), Session::Result::GpuRejected);
        const auto before = host.store(first)->statistics();
        host.requestHistoryEviction();
        QVERIFY(host.historyEvictionPending());
        host.poll();
        QVERIFY(!host.historyEvictionPending());
        QCOMPARE(host.store(first)->statistics().submissions, before.submissions);
        QCOMPARE(host.store(first)->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        QCOMPARE(a->state(), Document::State::Editing);
        QCOMPARE(editing.session->append(editing.token, {dab(0xFF123456)}, Bounds), Session::Result::Accepted);
        QCOMPARE(a->commit(editing.token), Document::Result::Accepted);
        QVERIFY(settle(host, a));
        occupied = {};
        QVERIFY(b->undo()); QVERIFY(settle(host, b));
        QCOMPARE(host.store(first)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(host.store(second)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(host.device()->errorCount(), 0);
    }
    void sharedDeviceSuspendsAndRestoresEveryDocument() {
        Host host;
        const auto first = host.createDocument({});
        const auto second = host.createDocument({});
        QVERIFY(first && second && first != second);
        auto a = host.document(first), b = host.document(second);
        a->addLayer(QStringLiteral("First")); b->addLayer(QStringLiteral("Second"));
        QVERIFY(paint(host, a, 0xFF0000FF));
        QVERIFY(paint(host, b, 0xFFFF0000));
        const auto aPixels = read(host, first, a->layers()[0].paint.pixels);
        const auto bPixels = read(host, second, b->layers()[0].paint.pixels);
        auto editing = a->begin(1);
        QCOMPARE(editing.session->append(editing.token, {dab(0x8000FF00)}, Bounds), Session::Result::Accepted);
        const auto acceptedInput = a->recovery().working->data;
        auto oldStore = host.store(first);
        const auto before = oldStore->statistics();
        int notices = 0;
        const auto connection = connect(&host, &Host::aboutToReleaseGpu, &host, [&] {
            ++notices;
            QVERIFY(host.device() != nullptr);
            QCOMPARE(host.createDocument({}), Host::DocumentId(0));
        });
        QVERIFY(host.suspend());
        QCOMPARE(notices, 1);
        QObject::disconnect(connection);
        QVERIFY(host.isSuspended());
        QVERIFY(!host.device());
        QCOMPARE(a->state(), Document::State::Suspended);
        QCOMPARE(b->state(), Document::State::Suspended);
        QCOMPARE(a->recovery().working->data.working->size(), acceptedInput.working->size());
        QCOMPARE(oldStore->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
        QCOMPARE(oldStore->statistics().submissions, before.submissions);
        QVERIFY(!oldStore->deviceAvailable());
        QVERIFY(host.suspend());
        QCOMPARE(notices, 1);
        QVERIFY(host.resume());
        QVERIFY(settle(host, a)); QVERIFY(settle(host, b));
        QCOMPARE(host.document(first), a); QCOMPARE(host.document(second), b);
        QCOMPARE(read(host, first, a->layers()[0].paint.pixels), aPixels);
        QCOMPARE(read(host, second, b->layers()[0].paint.pixels), bPixels);
        QCOMPARE(a->commit(editing.token), Document::Result::Stale);
        const auto resumed = a->currentEditing();
        QCOMPARE(a->commit(resumed.token), Document::Result::Accepted);
        QVERIFY(settle(host, a));
        QVERIFY(a->undo()); QVERIFY(settle(host, a));
        QCOMPARE(read(host, first, a->layers()[0].paint.pixels), aPixels);
        QCOMPARE(host.device()->errorCount(), 0);
    }
    void closeAndOwnerDestructionInvalidateOnlyOwnedEditing() {
        std::shared_ptr<Document> survivor;
        Session::Token token;
        {
            Host host;
            const auto first = host.createDocument({}), second = host.createDocument({});
            auto closed = host.document(first); survivor = host.document(second);
            closed->addLayer(QStringLiteral("Closed")); survivor->addLayer(QStringLiteral("Survives"));
            auto editing = closed->begin(1);
            QCOMPARE(editing.session->append(editing.token, {dab(0xFF0000FF)}, Bounds), Session::Result::Accepted);
            QVERIFY(host.closeDocument(first));
            QVERIFY(!host.document(first));
            QCOMPARE(closed->state(), Document::State::Suspended);
            QCOMPARE(closed->commit(editing.token), Document::Result::Stale);
            QVERIFY(paint(host, survivor, 0xFFFF0000));
            editing = survivor->begin(1); token = editing.token;
            QCOMPARE(editing.session->append(token, {dab(0xFF00FF00)}, Bounds), Session::Result::Accepted);
        }
        QCOMPARE(survivor->state(), Document::State::Suspended);
        QVERIFY(survivor->recovery().working);
        QCOMPARE(survivor->commit(token), Document::Result::Stale);
    }
    void lossAndInvalidRestorePreserveCpuDocuments() {
        Host host;
        const auto id = host.createDocument({});
        auto document = host.document(id);
        document->addLayer(QStringLiteral("Retained"));
        QVERIFY(paint(host, document, 0xFF123456));
        const auto saved = document->recovery();
        QCOMPARE(host.restoreDocument({}, Document::Recovery{}), Host::DocumentId(0));
        QVERIFY(host.device()->available());
        host.device()->destroy();
        host.poll();
        QVERIFY(host.isSuspended());
        QCOMPARE(document->recovery().cursor, saved.cursor);
        QCOMPARE(document->recovery().nextId, saved.nextId);
        QVERIFY(host.resume());
        QVERIFY(settle(host, document));
        QCOMPARE(document->state(), Document::State::Idle);
        const auto restored = host.restoreDocument({}, saved);
        QVERIFY(restored && restored != id);
        QVERIFY(settle(host, host.document(restored)));
        QCOMPARE(read(host, id, document->layers()[0].paint.pixels),
                 read(host, restored, host.document(restored)->layers()[0].paint.pixels));
    }
};
QTEST_GUILESS_MAIN(KisGpuDocumentDeviceTest)
#include "KisGpuDocumentDeviceTest.moc"
