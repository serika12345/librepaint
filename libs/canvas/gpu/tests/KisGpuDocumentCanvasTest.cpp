/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "DocumentCanvas.h"
#include "KisGpuLayerDocument.h"
#include "KisGpuTestDevice.h"
#include "KisGpuWindowTestEventLoop.h"
#include <QWindow>
#include <QtTest>

namespace {
using Canvas = Krita::Canvas::DocumentCanvas;
using Document = KisGpuLayerDocument;
using Store = KisGpuTileStore;
using Session = KisGpuEditSession;
using Surface = Krita::Canvas::SurfaceRenderer;
const QRect Bounds(0, 0, 64, 64);
Store::DabCommand dab(QPointF position, quint32 rgba)
{
    Store::DabCommand command;
    command.center = position; command.diameter = {16, 16}; command.rgba = rgba;
    return command;
}
}

/** The real canvas coalesces document updates and presents only successful images. */
class KisGpuDocumentCanvasTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void continuousInputPresentsWithOnePendingStoreSubmission() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes, 1);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        QWindow window; window.setFlag(Qt::WindowStaysOnTopHint);
        Surface::prepareWindow(window); window.resize(64, 64); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        Canvas canvas(gpu.owner, store, document, window, Bounds);
        auto edit = document->begin(1);
        for (int i = 0; i < 24; ++i) {
            bool accepted = false;
            QTRY_VERIFY((canvas.poll(), accepted || (accepted = edit.session->append(edit.token,
                {dab({qreal(i * 2 + 4), 32}, 0xFF123456)}, Bounds) == Session::Result::Accepted)));
            canvas.poll(); QTest::qWait(2);
        }
        QVERIFY(canvas.statistics().presentationRequests > 1);
        wgpuDevicePoll(gpu.device, true, nullptr); document->poll();
        const auto preview = edit.session->preview();
        Canvas::Result result = Canvas::Result::Idle;
        QVERIFY2(waitForGpuWindow([&] {
            result = canvas.poll();
            return canvas.displayedSnapshot().layers.size() == 1
                && canvas.displayedSnapshot().layers.first().pixels == preview
                && canvas.lastFrame().rendering.status() == Store::Status::Succeeded;
        }),
            qPrintable(QStringLiteral("result=%1 gpu=%2 surface=%3 frames=%4 presentations=%5 accepted=%6 visible=%7 exposed=%8")
                .arg(int(result)).arg(int(canvas.lastGpuError())).arg(int(canvas.lastSurfaceError()))
                .arg(canvas.statistics().rendering.pendingFrames).arg(canvas.statistics().pendingPresentations)
                .arg(canvas.statistics().presentationRequests).arg(window.isVisible()).arg(window.isExposed())));
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void previewReplacementCommitAndIdleKeepOneDisplayPath() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Base")); document->addLayer(QStringLiteral("Paint"));
        auto base = document->begin(1);
        QCOMPARE(base.session->append(base.token, {dab({32, 32}, 0xFF332211)}, Bounds), Session::Result::Accepted);
        document->commit(base.token);
        QTRY_VERIFY((document->poll(), document->state() == Document::State::Idle));
        QWindow window;
        Surface::prepareWindow(window); window.resize(128, 128); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        Canvas canvas(gpu.owner, store, document, window, Bounds);
        Krita::Canvas::GpuRenderer::View view; view.canvasToTarget.scale(2, 2); canvas.setView(view);
        QTRY_VERIFY((canvas.poll(), canvas.lastFrame().accepted()
            && canvas.lastFrame().rendering.status() == Store::Status::Succeeded));
        const auto idle = store->statistics();
        const auto presentations = canvas.statistics().presentationRequests;
        for (int i = 0; i < 20; ++i) { canvas.poll(); QTest::qWait(1); }
        QCOMPARE(store->statistics().submissions, idle.submissions);
        QCOMPARE(canvas.statistics().presentationRequests, presentations);
        auto edit = document->begin(2);
        QCOMPARE(edit.session->append(edit.token, {dab({8, 8}, 0xFF0000FF)}, Bounds), Session::Result::Accepted);
        QCOMPARE(edit.session->replace(edit.token, {dab({16, 48}, 0xFF00FF00)}, Bounds), Session::Result::Accepted);
        QCOMPARE(edit.session->replace(edit.token, {dab({48, 16}, 0xFFFF0000)}, Bounds), Session::Result::Accepted);
        wgpuDevicePoll(gpu.device, true, nullptr); document->poll();
        const auto preview = edit.session->preview();
        QTRY_VERIFY((canvas.poll(), canvas.displayedSnapshot().layers.size() == 2
            && canvas.displayedSnapshot().layers.last().pixels == preview
            && canvas.lastFrame().rendering.status() == Store::Status::Succeeded));
        auto expected = store->project(store->emptyVersion(), document->projectionLayers(), Bounds);
        QCOMPARE(expected.error, Store::Error::None);
        QTRY_VERIFY((store->poll(), expected.completion.status() == Store::Status::Succeeded));
        QCOMPARE(gpu.read(canvas.displayedSnapshot().pixels, Bounds), gpu.read(expected.version, Bounds));
        const auto beforeCommit = store->statistics();
        const auto displayBeforeCommit = canvas.statistics().presentationRequests;
        QCOMPARE(document->commit(edit.token), Document::Result::Accepted);
        QTRY_VERIFY((canvas.poll(), document->state() == Document::State::Idle));
        for (int i = 0; i < 5; ++i) canvas.poll();
        QCOMPARE(store->statistics().submissions, beforeCommit.submissions);
        QCOMPARE(canvas.statistics().presentationRequests, displayBeforeCommit);
        const auto beforeView = store->statistics();
        view.canvasToTarget.translate(2, 3); canvas.setView(view);
        QTRY_VERIFY((canvas.poll(), canvas.statistics().presentationRequests > displayBeforeCommit));
        QCOMPARE(store->statistics().submissions, beforeView.submissions);
        QCOMPARE(store->statistics().imageTextureAllocationBytes, beforeView.imageTextureAllocationBytes);
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void refusedProjectionRetriesAndWindowRecreationReusesImage() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        QWindow window; Surface::prepareWindow(window); window.resize(64, 64); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        Canvas canvas(gpu.owner, store, document, window, Bounds);
        QTRY_VERIFY((canvas.poll(), canvas.lastFrame().accepted()
            && canvas.lastFrame().rendering.status() == Store::Status::Succeeded));
        const auto previous = canvas.displayedSnapshot().pixels;
        auto edit = document->begin(1);
        QCOMPARE(edit.session->append(edit.token, {dab({16, 16}, 0xFF123456)}, Bounds), Session::Result::Accepted);
        wgpuDevicePoll(gpu.device, true, nullptr); document->poll();
        auto held = gpu.owner.reserveMemory(gpu.owner.availableMemory());
        QVERIFY(held);
        QCOMPARE(canvas.poll(), Canvas::Result::GpuRejected);
        QCOMPARE(canvas.lastGpuError(), Store::Error::BudgetExceeded);
        QVERIFY(canvas.displayedSnapshot().pixels == previous);
        held = {};
        const auto preview = edit.session->preview();
        QTRY_VERIFY((canvas.poll(), canvas.displayedSnapshot().layers.size() == 1
            && canvas.displayedSnapshot().layers.first().pixels == preview
            && canvas.lastFrame().rendering.status() == Store::Status::Succeeded));
        const auto before = store->statistics();
        const auto sequence = canvas.lastFrame().rendering.sequence();
        window.destroy(); window.resize(96, 64); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        canvas.requestRepaint();
        QTRY_VERIFY((canvas.poll(), canvas.lastFrame().rendering.sequence() > sequence
            && canvas.lastFrame().rendering.status() == Store::Status::Succeeded));
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(store->statistics().imageTextureAllocationBytes, before.imageTextureAllocationBytes);
        gpu.owner.destroy();
        QCOMPARE(canvas.poll(), Canvas::Result::DeviceLost);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
};
QTEST_MAIN(KisGpuDocumentCanvasTest)
#include "KisGpuDocumentCanvasTest.moc"
