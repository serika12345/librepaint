/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <gpu/KisGpuCanvasWindow.h>
#include <gpu/KisGpuExecution.h>
#include <limits>
#include "../KisGpuWindowSurface_p.h"
#include <webgpu/wgpu.h>

using namespace LibrePaint::Image::Gpu;
using namespace LibrePaint::Canvas::Gpu;

class KisGpuCanvasWindowTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void presentationFramesSubmitThroughTheOwningWindow();
    void unconfiguredSurfacesAreRejectedBeforeNativeSubmission();

    void editsAndViewChangesUseSharedExecution();
    void queuedUpdatesIgnoreClosedDocuments();
    void capacityFailurePreservesPublicationAndCanRetry();
    void deviceReplacementRecreatesVisibleAndHiddenSurfaces();
private:
    DevicePtr m_device;
};

void KisGpuCanvasWindowTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
}

void KisGpuCanvasWindowTest::editsAndViewChangesUseSharedExecution()
{
    Execution execution(m_device);
    OperationDocument document({256, 256});
    const auto admission = execution.open(document.snapshot());
    QCOMPARE(admission.code, ExecutionCode::Success);
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(admission.handle));
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
    QCOMPARE(execution.published(admission.handle), document.snapshot());
    QCOMPARE(window.documentViewport(), QRect(0, 0, 128, 128));

    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(execution.request(admission.handle, document.preview()).code, ExecutionCode::Success);
    for (int i = 0; i < 240; ++i) window.refresh();
    QTRY_COMPARE_WITH_TIMEOUT(execution.published(admission.handle), document.preview(), 5000);
    QCOMPARE(frames.last()[0].toULongLong(), admission.handle);
    QCOMPARE(frames.last()[1].toULongLong(), document.preview()->revision());
    QCOMPARE(frames.last()[2].toULongLong(), execution.generation());
    const auto transform = QTransform::fromTranslate(8, 16).scale(2, 2);
    const auto before = frames.size();
    QVERIFY(window.setDocumentToWindow(transform));
    QCOMPARE(window.documentViewport(), QRect(0, 0, 60, 56));
    QVERIFY(!window.setDocumentToWindow(QTransform::fromScale(0, 1)));
    QVERIFY(!window.setDocumentToWindow(QTransform::fromTranslate(std::numeric_limits<double>::quiet_NaN(), 0)));
    QCOMPARE(window.documentToWindow(), transform);
    QTRY_VERIFY_WITH_TIMEOUT(frames.size() > before, 5000);
    const auto resized = frames.size();
    window.resize(256, 128);
    window.refresh();
    QCOMPARE(window.documentViewport(), QRect(0, 0, 124, 56));
    QTRY_VERIFY_WITH_TIMEOUT(frames.size() > resized, 5000);
    QCOMPARE(errors.size(), 0);
}

void KisGpuCanvasWindowTest::queuedUpdatesIgnoreClosedDocuments()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
    const auto before = frames.size();
    window.refresh();
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QEvent update(QEvent::UpdateRequest);
    QCoreApplication::sendEvent(&window, &update);
    QCOMPARE(frames.size(), before);
    QVERIFY(window.documentViewport().isEmpty());
    QVERIFY(!window.setDocument(handle));
    QVERIFY(window.setDocument(0));
    QCOMPARE(errors.size(), 0);
}

void KisGpuCanvasWindowTest::deviceReplacementRecreatesVisibleAndHiddenSurfaces()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> previous = device;
    Execution execution(device);
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto handle = execution.open(document.snapshot()).handle;
    CanvasWindow visible(execution), hidden(execution);
    QVERIFY(visible.setDocument(handle));
    QVERIFY(hidden.setDocument(handle));
    visible.resize(128, 128);
    hidden.resize(128, 128);
    visible.setPosition(100, 100);
    hidden.setPosition(260, 100);
    QSignalSpy visibleFrames(&visible, &CanvasWindow::frameSubmitted);
    QSignalSpy hiddenFrames(&hidden, &CanvasWindow::frameSubmitted);
    QSignalSpy visibleErrors(&visible, &CanvasWindow::renderingFailed);
    QSignalSpy hiddenErrors(&hidden, &CanvasWindow::renderingFailed);
    visible.show();
    hidden.show();
    QVERIFY(QTest::qWaitForWindowExposed(&visible));
    QVERIFY(QTest::qWaitForWindowExposed(&hidden));
    QTRY_VERIFY_WITH_TIMEOUT(!visibleFrames.isEmpty() && !hiddenFrames.isEmpty(), 5000);
    hidden.hide();
    device.reset();
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QVERIFY(visible.resetSurface());
    QVERIFY(hidden.resetSurface());
    QVERIFY(previous.expired());
    QTRY_COMPARE_WITH_TIMEOUT(visibleFrames.last()[2].toULongLong(), execution.generation(), 5000);
    hidden.show();
    QVERIFY(QTest::qWaitForWindowExposed(&hidden));
    QTRY_COMPARE_WITH_TIMEOUT(hiddenFrames.last()[2].toULongLong(), execution.generation(), 5000);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(visibleErrors.size(), 0);
    QCOMPARE(hiddenErrors.size(), 0);
}

void KisGpuCanvasWindowTest::capacityFailurePreservesPublicationAndCanRetry()
{
    Execution execution(m_device, 2 * TileRenderer::TileBytes);
    OperationDocument document({64, 64});
    auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto original = document.snapshot();
    const auto handle = execution.open(original).handle;
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
    const auto submitted = frames.size();

    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QVERIFY(!window.resetSurface());
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    const auto layer = document.addLayer(QStringLiteral("Second"));
    QVERIFY(layer);
    stroke = document.beginStroke(layer, Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
    window.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(!errors.isEmpty(), 5000);
    QCOMPARE(errors.last()[0].toULongLong(), handle);
    QCOMPARE(errors.last()[1].toULongLong(), execution.generation());
    QCOMPARE(qvariant_cast<ExecutionCode>(errors.last()[2]), ExecutionCode::CapacityExceeded);
    QVERIFY(!errors.last()[3].toString().isEmpty());
    QCOMPARE(execution.published(handle), original);
    QCOMPARE(frames.size(), submitted);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.setLayerVisible(layer, false), Result::Success);
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
    window.refresh();
    QTRY_COMPARE_WITH_TIMEOUT(execution.published(handle), document.snapshot(), 5000);
    QCOMPARE(document.undoCount(), 4);
}

void KisGpuCanvasWindowTest::presentationFramesSubmitThroughTheOwningWindow()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    QWindow window;
    LibrePaint::Canvas::Gpu::Detail::setNativeSurfaceType(window);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto surface = LibrePaint::Canvas::Gpu::Detail::createWindowSurface(m_device, window);
    QVERIFY(surface->configure(QSizeF(window.size() * window.devicePixelRatio()).toSize()));
    const auto frame = execution.beginPresentationFrame(*surface);
    QCOMPARE(frame.code, ExecutionCode::Success);
    QVERIFY(frame.commands);
    QVERIFY(frame.target);
    const auto projection = execution.project(handle, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QVERIFY(!execution.published(handle));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(execution.pendingDocuments(), 0);
    QString diagnostic;
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QCOMPARE(execution.beginPresentationFrame(*surface).code, ExecutionCode::InvalidRequest);
}

void KisGpuCanvasWindowTest::unconfiguredSurfacesAreRejectedBeforeNativeSubmission()
{
    Execution execution(m_device);
    QWindow window;
    LibrePaint::Canvas::Gpu::Detail::setNativeSurfaceType(window);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto surface = LibrePaint::Canvas::Gpu::Detail::createWindowSurface(m_device, window);
    QCOMPARE(execution.beginPresentationFrame(*surface).code, ExecutionCode::SurfaceOutOfDate);
    QVERIFY(!m_device->isRecordingFrame());
    QVERIFY(!surface->configure({0, 128}));
    QVERIFY(!surface->configure({std::numeric_limits<int>::max(), 128}));
    QVERIFY(m_device->diagnostic().isEmpty());
    QVERIFY(surface->configure({128, 128}));
    QCOMPARE(execution.beginPresentationFrame(*surface).code, ExecutionCode::Success);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
}





int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuCanvasWindowTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuCanvasWindowTest.moc"
