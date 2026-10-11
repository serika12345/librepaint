/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPointerInput.h"
#include "KisGpuTestDevice.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTimer>
#include <QWindow>
#include <QtTest>
#include <cmath>
#include <limits>

namespace {
using Input = KisGpuStrokeInput;
using Pointer = KisGpuPointerInput;
using Document = KisGpuLayerDocument;
using Store = KisGpuTileStore;
const QRect Bounds(0, 0, 64, 64);
Input::Configuration configuration() {
    Input::Configuration result; result.clip = Bounds;
    result.brush.diameter = {12, 12}; result.brush.rgba = 0xFF123456;
    return result;
}
void mouse(QWindow &window, QEvent::Type type, QPointF point, Qt::MouseButton button,
           Qt::MouseButtons buttons, Qt::MouseEventSource source = Qt::MouseEventNotSynthesized) {
    QMouseEvent event(type, point, point, point, button, buttons, Qt::NoModifier, source);
    QCoreApplication::sendEvent(&window, &event);
}
void tablet(QWindow &window, const QPointingDevice &device, QEvent::Type type, QPointF point, qreal pressure) {
    const auto button = type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton;
    QTabletEvent event(type, &device, point, point, pressure, 0, 0, 0, 0, 0, Qt::NoModifier,
        button, type == QEvent::TabletRelease ? Qt::NoButton : Qt::LeftButton);
    QCoreApplication::sendEvent(&window, &event);
}
std::shared_ptr<Document> document(const std::shared_ptr<Store> &store) {
    auto result = std::make_shared<Document>(store, 16); result->addLayer(QStringLiteral("Paint")); return result;
}
}

/** Native mouse/pen input preserves coordinates, pressure, bounded waiting and operation ownership. */
class KisGpuPointerInputTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void windowMovementRestartsHoldAtTheCurrentZoom() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store);
        Input input(model);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        QVERIFY(pointer.setQuickShapeEnabled(true));
        QVERIFY(pointer.setWindowToCanvas(QTransform::fromScale(.1, .1)));
        QTimer progress;
        QObject::connect(&progress, &QTimer::timeout, this, [&] { pointer.poll(); input.poll(); });
        progress.start(1);
        mouse(window, QEvent::MouseButtonPress, {80, 80}, Qt::LeftButton, Qt::LeftButton);
        mouse(window, QEvent::MouseMove, {480, 80}, Qt::NoButton, Qt::LeftButton);
        QTest::qWait(350);
        mouse(window, QEvent::MouseMove, {486, 80}, Qt::NoButton, Qt::LeftButton);
        QTest::qWait(200);
        QVERIFY(!pointer.quickShapeActive());
        QTRY_VERIFY(pointer.quickShapeActive());
        QCOMPARE(pointer.cancel(), Input::Result::Accepted);
        QCOMPARE(model->state(), Document::State::Idle);
        QCOMPARE(model->layers().first().paint.pixels.tileCount(), qsizetype(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void recognizedLineContinuesAfterGpuExchange() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store);
        Input input(model);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        QVERIFY(pointer.setQuickShapeEnabled(true));
        QTimer progress;
        QObject::connect(&progress, &QTimer::timeout, this, [&] { pointer.poll(); input.poll(); });
        progress.start(1);
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        mouse(window, QEvent::MouseMove, {48, 8}, Qt::NoButton, Qt::LeftButton);
        QTRY_VERIFY(pointer.quickShapeActive());
        input.releaseGpu();
        QCOMPARE(model->suspend(), Document::Result::Accepted);
        gpu.owner.destroy();
        KisGpuTestDevice replacement;
        auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
        QCOMPARE(model->resume(nextStore), Document::Result::Accepted);
        QTRY_COMPARE(model->state(), Document::State::Editing);
        QVERIFY(pointer.quickShapeActive());
        mouse(window, QEvent::MouseMove, {52, 44}, Qt::NoButton, Qt::LeftButton);
        mouse(window, QEvent::MouseButtonRelease, {52, 44}, Qt::LeftButton, Qt::NoButton);
        QTRY_VERIFY(!pointer.active() && input.state() == Input::State::Finished);
        auto reference = document(nextStore);
        Input control(reference);
        QObject::connect(&progress, &QTimer::timeout, this, [&] { control.poll(); });
        const auto expected = control.begin(1, configuration());
        QCOMPARE(expected.result, Input::Result::Accepted);
        QCOMPARE(control.append(expected.token, {{{8, 8}, 1}, {{52, 44}, 1}}), Input::Result::Accepted);
        QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
        QTRY_COMPARE(control.state(), Input::State::Finished);
        QCOMPARE(replacement.read(model->layers().first().paint.pixels, Bounds),
                 replacement.read(reference->layers().first().paint.pixels, Bounds));
        QCOMPARE(nextStore->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(nextStore->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(replacement.owner.errorCount(), 0);
        progress.stop();
    }
    void recognitionLimitKeepsTheAcceptedFreehandStroke() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store), reference = document(store);
        Input input(model), control(reference);
        QWindow window;
        Pointer pointer(input, window, 4);
        QVERIFY(pointer.setTool(1, configuration()));
        QVERIFY(pointer.setQuickShapeEnabled(true));
        QTimer progress;
        QObject::connect(&progress, &QTimer::timeout, this, [&] { pointer.poll(); input.poll(); control.poll(); });
        progress.start(1);
        QVector<KisGpuBrushStroke::Sample> samples;
        for (int i = 0; i < 7; ++i) {
            const QPointF point(8 + i * 6, 12 + i % 2);
            samples.append({point, 1});
            mouse(window, i ? QEvent::MouseMove : QEvent::MouseButtonPress, point,
                i ? Qt::NoButton : Qt::LeftButton, Qt::LeftButton);
            QCOMPARE(pointer.poll(), Input::Result::Accepted);
        }
        QTest::qWait(600);
        QVERIFY(pointer.active());
        QVERIFY(!pointer.quickShapeActive());
        const auto last = samples.last(); samples.append(last);
        mouse(window, QEvent::MouseButtonRelease, last.position, Qt::LeftButton, Qt::NoButton);
        QTRY_VERIFY(!pointer.active() && input.state() == Input::State::Finished);
        const auto expected = control.begin(1, configuration());
        QCOMPARE(control.append(expected.token, samples), Input::Result::Accepted);
        QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
        QTRY_COMPARE(control.state(), Input::State::Finished);
        QCOMPARE(gpu.read(model->layers().first().paint.pixels, Bounds), gpu.read(reference->layers().first().paint.pixels, Bounds));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void heldEllipseKeepsPressureAndCirclePreview_data() {
        QTest::addColumn<bool>("texture"); QTest::addColumn<bool>("mixed");
        QTest::newRow("solid") << false << false;
        QTest::newRow("textured-selected") << true << false;
        QTest::newRow("mixed-selected") << false << true;
    }
    void heldEllipseKeepsPressureAndCirclePreview() {
        QFETCH(bool, texture); QFETCH(bool, mixed);
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store), reference = document(store);
        Input input(model), control(reference);
        QWindow window;
        Pointer pointer(input, window);
        auto config = configuration();
        if (texture) config.texture = std::make_shared<const KisGpuEditSession::ReplayTexture>(
            KisGpuEditSession::ReplayTexture{{3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3}});
        if (mixed) config.brush.smudgeRate = 213;
        if (texture || mixed) {
            Store::DabCommand mask; mask.center = {28, 32}; mask.diameter = {60, 48}; mask.rgba = 0xCC000000;
            config.selection = std::make_shared<const KisGpuEditSession::ReplaySelection>(
                KisGpuEditSession::ReplaySelection{{mask}, Bounds});
        }
        QVERIFY(pointer.setTool(1, config));
        QVERIFY(pointer.setQuickShapeEnabled(true));
        QTimer progress;
        QObject::connect(&progress, &QTimer::timeout, this, [&] { pointer.poll(); input.poll(); control.poll(); });
        progress.start(1);
        for (const auto &target : {model, reference}) {
            const auto seed = target->begin(1);
            Store::DabCommand red; red.center = {26, 32}; red.diameter = {32, 64}; red.rgba = 0xFF2020EE;
            auto blue = red; blue.center = {46, 26}; blue.diameter = {20, 20}; blue.rgba = 0xFFEE4020;
            QCOMPARE(seed.session->append(seed.token, {red, blue}, Bounds), KisGpuEditSession::Result::Accepted);
            QCOMPARE(target->commit(seed.token), Document::Result::Accepted);
        }
        QTRY_COMPARE(model->state(), Document::State::Idle);
        QTRY_COMPARE(reference->state(), Document::State::Idle);
        const auto base = model->layers().first().paint.pixels;
        QPointingDevice pen(QStringLiteral("Shape pen"), 3, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen, QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 1);
        QPointF last;
        for (int i = 0; i < 64; ++i) {
            const qreal angle = 2 * M_PI * i / 64;
            last = {32 + 20 * std::cos(angle), 32 + 12 * std::sin(angle)};
            tablet(window, pen, i ? QEvent::TabletMove : QEvent::TabletPress, last, .65);
        }
        QTRY_VERIFY(pointer.quickShapeActive());
        const auto before = model->currentEditing().session->preview();
        QKeyEvent circle(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
        QCoreApplication::sendEvent(&window, &circle);
        QCOMPARE(pointer.poll(), Input::Result::Accepted);
        QTRY_VERIFY(!input.pendingSamples() && !(model->currentEditing().session->preview() == before));
        const auto preview = model->currentEditing().session->preview();
        const auto ready = store->statistics();
        tablet(window, pen, QEvent::TabletRelease, last, 0);
        QTRY_VERIFY(!pointer.active() && input.state() == Input::State::Finished);
        QCOMPARE(model->layers().first().paint.pixels, preview);
        QCOMPARE(store->statistics().submissions, ready.submissions);
        QCOMPARE(store->statistics().pixelUploadBytes, ready.pixelUploadBytes);
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QVector<KisGpuBrushStroke::Sample> circleSamples;
        for (int i = 0; i < 256; ++i) {
            const qreal angle = 2 * M_PI * i / 256;
            circleSamples.append({{32 + 20 * std::cos(angle), 32 + 20 * std::sin(angle)}, .65});
        }
        circleSamples.append(circleSamples.first());
        const auto expected = control.begin(1, config);
        QCOMPARE(expected.result, Input::Result::Accepted);
        QCOMPARE(control.append(expected.token, circleSamples), Input::Result::Accepted);
        QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
        QTRY_COMPARE(control.state(), Input::State::Finished);
        QCOMPARE(gpu.read(preview, Bounds), gpu.read(reference->layers().first().paint.pixels, Bounds));
        QVERIFY(model->undo()); QCOMPARE(model->layers().first().paint.pixels, base);
        QVERIFY(model->redo()); QCOMPARE(model->layers().first().paint.pixels, preview);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void heldLineUsesTheGpuPreviewForCommit() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store), reference = document(store);
        Input input(model), control(reference);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        QVERIFY(pointer.setQuickShapeEnabled(true));
        QTimer progress;
        QObject::connect(&progress, &QTimer::timeout, this, [&] { pointer.poll(); input.poll(); control.poll(); });
        progress.start(1);
        QVERIFY(!pointer.hasPendingWork());
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QVERIFY(pointer.hasPendingWork());
        mouse(window, QEvent::MouseMove, {28, 9}, Qt::NoButton, Qt::LeftButton);
        mouse(window, QEvent::MouseMove, {48, 8}, Qt::NoButton, Qt::LeftButton);
        QCOMPARE(pointer.poll(), Input::Result::Accepted);
        QVERIFY(!pointer.hasPendingWork());
        QTRY_VERIFY(pointer.quickShapeActive());
        const auto before = model->currentEditing().session->preview();
        mouse(window, QEvent::MouseMove, {52, 40}, Qt::NoButton, Qt::LeftButton);
        QVERIFY(pointer.hasPendingWork());
        QCOMPARE(pointer.poll(), Input::Result::Accepted);
        QVERIFY(!pointer.hasPendingWork());
        QTRY_VERIFY(input.pendingSamples() == 0 && !(model->currentEditing().session->preview() == before));
        const auto preview = model->currentEditing().session->preview();
        const auto ready = store->statistics();
        mouse(window, QEvent::MouseButtonRelease, {52, 40}, Qt::LeftButton, Qt::NoButton);
        QVERIFY(pointer.hasPendingWork());
        QTRY_VERIFY(!pointer.active() && input.state() == Input::State::Finished);
        QVERIFY(!pointer.hasPendingWork());
        QCOMPARE(model->layers().first().paint.pixels, preview);
        QCOMPARE(store->statistics().submissions, ready.submissions);
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        const auto expected = control.begin(1, configuration());
        QCOMPARE(expected.result, Input::Result::Accepted);
        QCOMPARE(control.append(expected.token, {{{8, 8}, 1}, {{52, 40}, 1}}), Input::Result::Accepted);
        QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
        QTRY_COMPARE(control.state(), Input::State::Finished);
        QCOMPARE(gpu.read(preview, Bounds), gpu.read(reference->layers().first().paint.pixels, Bounds));
        QVERIFY(model->undo());
        QCOMPARE(model->layers().first().paint.pixels.tileCount(), qsizetype(0));
        QVERIFY(model->redo());
        QCOMPARE(model->layers().first().paint.pixels, preview);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void invalidConfigurationRetainsToolAndInvalidSampleCancelsBeforeGpuWork() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store);
        Input input(model);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        auto invalid = configuration(); invalid.brush.diameter = {0, 12};
        QVERIFY(!pointer.setTool(1, invalid));
        QVERIFY(!pointer.setWindowToCanvas(QTransform::fromScale(0, 1)));
        QSignalSpy rejected(&pointer, &Pointer::inputRejected);
        const auto before = store->statistics();
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::RightButton, Qt::RightButton);
        QVERIFY(!pointer.active());
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QVERIFY(pointer.active());
        QVERIFY(!pointer.setTool(1, invalid));
        QVERIFY(pointer.active());
        mouse(window, QEvent::MouseMove, {std::numeric_limits<qreal>::quiet_NaN(), 8}, Qt::NoButton, Qt::LeftButton);
        QVERIFY(!pointer.active());
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(model->state(), Document::State::Idle);
        QCOMPARE(store->statistics().submissions, before.submissions);
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QEvent deactivate(QEvent::WindowDeactivate);
        QCoreApplication::sendEvent(&window, &deactivate);
        QTRY_VERIFY((pointer.poll(), input.poll(), !pointer.active() && input.state() == Input::State::Finished));
        QVERIFY(model->layers().first().paint.pixels.tileCount() > 0);
        QVERIFY(model->undo());
        QVERIFY(model->layers().first().paint.pixels.tileCount() == 0);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void mouseCoordinatesAndReleaseMatchNormalizedInput() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store), reference = document(store);
        Input input(model), control(reference);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        QTransform transform; transform.scale(.5, .5);
        QVERIFY(pointer.setWindowToCanvas(transform));
        mouse(window, QEvent::MouseButtonPress, {16, 16}, Qt::LeftButton, Qt::LeftButton);
        mouse(window, QEvent::MouseMove, {64, 32}, Qt::NoButton, Qt::LeftButton);
        mouse(window, QEvent::MouseButtonRelease, {96, 96}, Qt::LeftButton, Qt::NoButton);
        QVERIFY(pointer.active());
        QTRY_VERIFY((pointer.poll(), input.poll(), !pointer.active() && input.state() == Input::State::Finished));
        auto started = control.begin(1, configuration());
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(control.append(started.token, {{{8, 8}, 1}, {{32, 16}, 1}, {{48, 48}, 1}}), Input::Result::Accepted);
        QCOMPARE(control.commit(started.token), Input::Result::Accepted);
        QTRY_COMPARE((control.poll(), control.state()), Input::State::Finished);
        QCOMPARE(gpu.read(model->layers().first().paint.pixels, Bounds), gpu.read(reference->layers().first().paint.pixels, Bounds));
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void penPressureAndEraserIgnoreSynthesizedMouseDuplicates() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store), reference = document(store);
        Input input(model), control(reference);
        QWindow window;
        Pointer pointer(input, window);
        QVERIFY(pointer.setTool(1, configuration()));
        const auto capabilities = QInputDevice::Capability::Position | QInputDevice::Capability::Pressure;
        QPointingDevice pen(QStringLiteral("Contract pen"), 1, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen, capabilities, 1, 1);
        QPointingDevice eraser(QStringLiteral("Contract eraser"), 2, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Eraser, capabilities, 1, 1);
        for (const auto *device : {&pen, &eraser}) {
            tablet(window, *device, QEvent::TabletPress, {8, 8}, .5);
            mouse(window, QEvent::MouseButtonPress, {60, 2}, Qt::LeftButton, Qt::LeftButton, Qt::MouseEventSynthesizedByQt);
            tablet(window, *device, QEvent::TabletMove, {32, 16}, .9);
            tablet(window, *device, QEvent::TabletRelease, {48, 48}, .2);
            QTRY_VERIFY((pointer.poll(), input.poll(), !pointer.active() && input.state() == Input::State::Finished));
            auto config = configuration();
            if (device == &eraser) config.brush.operation = Store::CompositeOp::Erase;
            const auto expected = control.begin(1, config);
            QCOMPARE(expected.result, Input::Result::Accepted);
            QCOMPARE(control.append(expected.token, {{{8, 8}, .5}, {{32, 16}, .9}, {{48, 48}, .2}}), Input::Result::Accepted);
            QCOMPARE(control.commit(expected.token), Input::Result::Accepted);
            QTRY_COMPARE((control.poll(), control.state()), Input::State::Finished);
            QCOMPARE(gpu.read(model->layers().first().paint.pixels, Bounds), gpu.read(reference->layers().first().paint.pixels, Bounds));
        }
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void busyBeginRetainsInputAndOverflowCancelsOnlyItsOwnOperation() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto model = document(store);
        Input input(model);
        QWindow window;
        Pointer pointer(input, window, 2);
        QVERIFY(pointer.setTool(1, configuration()));
        QSignalSpy rejected(&pointer, &Pointer::inputRejected);
        auto foreign = model->begin(1);
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QCOMPARE(pointer.poll(), Input::Result::Busy);
        mouse(window, QEvent::MouseMove, {16, 8}, Qt::NoButton, Qt::LeftButton);
        mouse(window, QEvent::MouseMove, {32, 8}, Qt::NoButton, Qt::LeftButton);
        QVERIFY(!pointer.active());
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(model->currentEditing().token, foreign.token);
        QCOMPARE(model->cancel(foreign.token), Document::Result::Accepted);
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        QCOMPARE(pointer.poll(), Input::Result::Accepted);
        input.poll();
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &escape);
        QVERIFY(!pointer.active());
        QCOMPARE(model->state(), Document::State::Idle);
        QVERIFY(model->layers().first().paint.pixels.tileCount() == 0);
        auto busy = model->begin(1);
        mouse(window, QEvent::MouseButtonPress, {8, 8}, Qt::LeftButton, Qt::LeftButton);
        mouse(window, QEvent::MouseButtonRelease, {16, 8}, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(pointer.poll(), Input::Result::Busy);
        QCOMPARE(model->cancel(busy.token), Document::Result::Accepted);
        QTRY_VERIFY((pointer.poll(), input.poll(), !pointer.active() && input.state() == Input::State::Finished));
        QVERIFY(model->layers().first().paint.pixels.tileCount() > 0);
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
};
QTEST_MAIN(KisGpuPointerInputTest)
#include "KisGpuPointerInputTest.moc"
