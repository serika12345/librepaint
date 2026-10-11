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
#include <QWindow>
#include <QtTest>
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
