/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasSession.h"
#include "DocumentCanvas.h"
#include "KisGpuStrokeInput.h"
#include "KisGpuPointerInput.h"
#include <QEventLoop>
#include <QMouseEvent>
#include <QWindow>
#include <QtTest>

namespace {
using Document = KisGpuLayerDocument;
using Input = KisGpuStrokeInput;
using Session = KisGpuCanvasSession;
using Store = KisGpuTileStore;
const QRect Bounds(0, 0, 64, 64);
Input::Configuration configuration() {
    Input::Configuration config; config.clip = Bounds;
    config.brush.diameter = {12, 12}; config.brush.spacing = 0.25; config.brush.rgba = 0xFF123456;
    return config;
}
void expose(QWindow &window) {
    Krita::Canvas::SurfaceRenderer::prepareWindow(window); window.resize(64, 64); window.show();
}
void mouse(QWindow &window, QEvent::Type type, QPointF point, Qt::MouseButton button, Qt::MouseButtons buttons) {
    QMouseEvent event(type, point, point, button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &event);
}
}

/** Application consumers retain input across shared GPU replacement and release closed documents. */
class KisGpuCanvasSessionTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void pointerInputContinuesWhileHiddenAndDisplaysAfterResume() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        auto model = documents.document(id); model->addLayer(QStringLiteral("Paint"));
        QWindow window;
        expose(window);
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        window.hide();
        application.applicationStateChanged(Qt::ApplicationActive);
        Session canvas(documents, application, id, window, Bounds);
        QVERIFY(canvas.setPointerTool(1, configuration()));
        Krita::Canvas::GpuRenderer::View view;
        view.canvasToTarget.scale(window.devicePixelRatio() * 2, window.devicePixelRatio() * 2);
        QVERIFY(canvas.setView(view));
        auto invalidView = view; invalidView.canvasToTarget.scale(0, 1);
        QVERIFY(!canvas.setView(invalidView));
        application.memoryPressure(255);
        QVector<KisGpuBrushStroke::Sample> samples{{{4, 4}, 1}};
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QEventLoop loop;
        QTimer stream;
        int count = 0;
        bool progressedDuringInput = false;
        QObject::connect(&stream, &QTimer::timeout, &loop, [&] {
            const QPointF position(8 + count % 32, 8 + count % 16);
            samples.append({position * .5, 1});
            mouse(window, QEvent::MouseMove, position, Qt::NoButton, Qt::LeftButton);
            if (++count == 60) {
                const auto layers = model->projectionLayers();
                progressedDuringInput = !layers.empty() && layers.first().pixels.tileCount() > 0;
                samples.append({position * .5, 1});
                mouse(window, QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::NoButton);
                stream.stop(); loop.quit();
            }
        });
        stream.start(2);
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();
        QCOMPARE(count, 60);
        QVERIFY(progressedDuringInput);
        QTRY_COMPARE(canvas.inputState(), Input::State::Finished);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        QTRY_VERIFY(canvas.lastFrame().accepted() && canvas.lastFrame().rendering.status() == Store::Status::Succeeded);
        auto store = documents.store(id);
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        auto reference = std::make_shared<Document>(store, 16);
        reference->addLayer(QStringLiteral("Reference"));
        const auto expected = reference->begin(1);
        KisGpuBrushStroke brush(*expected.session, expected.token, configuration().brush, Bounds);
        QCOMPARE(brush.append(samples).result, KisGpuBrushStroke::Result::Accepted);
        QCOMPARE(reference->commit(expected.token), Document::Result::Accepted);
        QTRY_COMPARE((reference->poll(), reference->state()), Document::State::Idle);
        const auto actual = store->readback(model->layers().first().paint.pixels, Bounds);
        const auto desired = store->readback(reference->layers().first().paint.pixels, Bounds);
        QTRY_VERIFY((store->poll(), actual.completion.status() == Store::Status::Succeeded
            && desired.completion.status() == Store::Status::Succeeded));
        QCOMPARE(actual.bytes(), desired.bytes());
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void displayAllocationRefusalDoesNotBlockAcceptedCommit() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        KisGpuDocumentDevice::Limits limits;
        limits.documentBytes = 2 * Store::TileBytes + 1024;
        const auto id = documents.createDocument(limits);
        QVERIFY(id);
        auto model = documents.document(id);
        model->addLayer(QStringLiteral("Paint"));
        auto seed = model->begin(1);
        Store::DabCommand dab; dab.center = {16, 16}; dab.diameter = {8, 8}; dab.rgba = 0xFF123456;
        QCOMPARE(seed.session->append(seed.token, {dab}, Bounds), KisGpuEditSession::Result::Accepted);
        QCOMPARE(model->commit(seed.token), Document::Result::Accepted);
        QTRY_COMPARE((model->poll(), model->state()), Document::State::Idle);
        const auto base = model->layers().first().paint.pixels;
        QWindow window;
        Krita::Canvas::SurfaceRenderer::prepareWindow(window); window.resize(64, 64);
        application.applicationStateChanged(Qt::ApplicationActive);
        Session canvas(documents, application, id, window, Bounds);
        const auto started = canvas.begin(1, configuration());
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(canvas.append(started.token, {{{32, 32}, 1}}), Input::Result::Accepted);
        canvas.poll();
        QTRY_VERIFY((model->poll(), !(model->currentEditing().session->preview() == base)));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        QTRY_VERIFY(!canvas.lastError().isEmpty());
        QVERIFY(!canvas.lastFrame().accepted());
        auto held = documents.device()->reserveMemory(documents.device()->availableMemory());
        QVERIFY(held);
        QCOMPARE(canvas.commit(started.token), Input::Result::Accepted);
        QTRY_COMPARE(canvas.inputState(), Input::State::Finished);
        QCOMPARE(model->state(), Document::State::Idle);
        QVERIFY(!(model->layers().first().paint.pixels == base));
        QCOMPARE(documents.store(id)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(documents.store(id)->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void twoViewsAndQueuedInputSurvivePressureAndFinishIdentically() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        const auto referenceId = documents.createDocument({});
        QVERIFY(id && referenceId);
        auto model = documents.document(id), reference = documents.document(referenceId);
        model->addLayer(QStringLiteral("Paint")); reference->addLayer(QStringLiteral("Reference"));
        QWindow first, second, referenceWindow;
        expose(first); expose(second); expose(referenceWindow);
        QVERIFY(QTest::qWaitForWindowExposed(&first, 3000));
        QVERIFY(QTest::qWaitForWindowExposed(&second, 3000));
        QVERIFY(QTest::qWaitForWindowExposed(&referenceWindow, 3000));
        application.applicationStateChanged(Qt::ApplicationActive);
        Session drawing(documents, application, id, first, Bounds);
        Session mirror(documents, application, id, second, Bounds);
        Session control(documents, application, referenceId, referenceWindow, Bounds);
        const auto expected = control.begin(1, configuration());
        QCOMPARE(expected.result, Input::Result::Accepted);
        const QVector<KisGpuBrushStroke::Sample> samples{{{4, 8}, 0.5}, {{58, 48}, 0.9}};
        QCOMPARE(control.append(expected.token, samples), Input::Result::Accepted);
        QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
        QTRY_COMPARE(control.inputState(), Input::State::Finished);
        QByteArray expectedPixels;
        {
            auto expectedStore = documents.store(referenceId);
            const auto read = expectedStore->readback(reference->layers().first().paint.pixels, Bounds);
            QCOMPARE(read.error, Store::Error::None);
            QTRY_VERIFY((expectedStore->poll(), read.completion.status() == Store::Status::Succeeded));
            expectedPixels = read.bytes();
        }
        const auto started = drawing.begin(1, configuration());
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(mirror.begin(1, configuration()).result, Input::Result::Busy);
        QCOMPARE(drawing.append(started.token, {samples.first()}), Input::Result::Accepted);
        drawing.poll();
        QCOMPARE(drawing.append(started.token, {samples.last()}), Input::Result::Accepted);
        application.memoryPressure(255);
        QVERIFY(documents.isSuspended());
        QVERIFY(!drawing.lastFrame().accepted());
        QVERIFY(!mirror.lastFrame().accepted());
        QCOMPARE(drawing.commit(started.token), Input::Result::Accepted);
        application.memoryPressure(0);
        QTRY_COMPARE(drawing.inputState(), Input::State::Finished);
        QTRY_COMPARE(control.inputState(), Input::State::Finished);
        QTRY_VERIFY(drawing.lastFrame().accepted() && mirror.lastFrame().accepted() && control.lastFrame().accepted());
        QTRY_VERIFY(drawing.lastFrame().rendering.status() == Store::Status::Succeeded
            && mirror.lastFrame().rendering.status() == Store::Status::Succeeded);
        auto store = documents.store(id);
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        const auto actual = store->readback(model->layers().first().paint.pixels, Bounds);
        QTRY_VERIFY((store->poll(), actual.completion.status() == Store::Status::Succeeded));
        QCOMPARE(actual.bytes(), expectedPixels);
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void closeAndLossReleaseOnlyOwnedInputAndWindowRecreationRepaints() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        const auto otherId = documents.createDocument({});
        auto model = documents.document(id), other = documents.document(otherId);
        model->addLayer(QStringLiteral("First")); other->addLayer(QStringLiteral("Other"));
        QWindow window, otherWindow; expose(window); expose(otherWindow);
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        QVERIFY(QTest::qWaitForWindowExposed(&otherWindow, 3000));
        application.applicationStateChanged(Qt::ApplicationActive);
        Session canvas(documents, application, id, window, Bounds);
        Session retained(documents, application, otherId, otherWindow, Bounds);
        QTRY_VERIFY(canvas.lastFrame().accepted() && retained.lastFrame().accepted());
        auto store = documents.store(id);
        const auto before = store->statistics();
        const auto presentations = canvas.statistics().presentationRequests;
        window.destroy(); window.resize(96, 64); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        QTRY_VERIFY(canvas.statistics().presentationRequests > presentations);
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(store->statistics().imageTextureAllocationBytes, before.imageTextureAllocationBytes);
        application.applicationStateChanged(Qt::ApplicationActive);
        auto started = canvas.begin(1, configuration());
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(canvas.append(started.token, {{{8, 8}, 1}, {{48, 48}, 0.5}}), Input::Result::Accepted);
        documents.device()->destroy();
        QTRY_VERIFY(documents.device() && documents.device()->available());
        QTRY_VERIFY(canvas.lastFrame().accepted() && retained.lastFrame().accepted());
        QCOMPARE(canvas.cancel(started.token), Input::Result::Accepted);
        QTRY_COMPARE(model->state(), Document::State::Idle);
        QVERIFY(documents.closeDocument(id));
        QCOMPARE(canvas.poll(), Session::Result::Closed);
        QCOMPARE(canvas.append(started.token, {{{16, 16}, 1}}), Input::Result::Stale);
        QVERIFY(!canvas.setPointerTool(1, configuration()));
        QVERIFY(!canvas.lastFrame().accepted());
        QVERIFY(documents.device() && documents.document(otherId) == other);
        QVERIFY(retained.lastFrame().accepted());
        QCOMPARE(documents.device()->errorCount(), 0);
    }
    void hiddenWindowFinishesInputAndSessionDestructionCancelsItsOwnEdit() {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState application(documents);
        const auto id = documents.createDocument({});
        auto model = documents.document(id); model->addLayer(QStringLiteral("Paint"));
        QWindow window; expose(window);
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        application.applicationStateChanged(Qt::ApplicationActive);
        {
            Session canvas(documents, application, id, window, Bounds);
            auto started = canvas.begin(1, configuration());
            QCOMPARE(started.result, Input::Result::Accepted);
            QCOMPARE(canvas.append(started.token, {{{8, 8}, 1}, {{48, 48}, 0.5}}), Input::Result::Accepted);
            window.hide();
            QCOMPARE(canvas.commit(started.token), Input::Result::Accepted);
            QTRY_COMPARE(canvas.inputState(), Input::State::Finished);
            QCOMPARE(model->state(), Document::State::Idle);
        }
        const auto pixels = model->layers().first().paint.pixels;
        {
            Session cancelled(documents, application, id, window, Bounds);
            application.applicationStateChanged(Qt::ApplicationActive);
            auto started = cancelled.begin(1, configuration());
            QCOMPARE(started.result, Input::Result::Accepted);
            QCOMPARE(cancelled.replace(started.token, {{{32, 16}, 1}}), Input::Result::Accepted);
            cancelled.poll();
        }
        QCOMPARE(model->state(), Document::State::Idle);
        QVERIFY(model->layers().first().paint.pixels == pixels);
        QVERIFY(model->undo());
        QCOMPARE(documents.device()->errorCount(), 0);
    }
};
QTEST_MAIN(KisGpuCanvasSessionTest)
#include "KisGpuCanvasSessionTest.moc"
