/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <QScreen>
#include <gpu/KisGpuCanvasWindow.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuTilePresentation.h>
#include <limits>
#include <stdexcept>
#include "../KisGpuWindowSurface_p.h"
#include <webgpu/wgpu.h>

using namespace LibrePaint::Image::Gpu;
using namespace LibrePaint::Canvas::Gpu;

namespace {
KoColorConversionStages identityDisplay(const Snapshot &source)
{
    KoColorConversionStages value;
    value.input.depth = source->format == PixelFormat::Rgba16
        ? KoColorConversionStages::Encoding::U16 : KoColorConversionStages::Encoding::U8;
    value.input.iccProfile = source->colorProfile;
    value.output = value.input;
    return value;
}
}

class KisGpuCanvasWindowTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void presentationFramesSubmitThroughTheOwningWindow();
    void unconfiguredSurfacesAreRejectedBeforeNativeSubmission();
    void failedDevicesRejectSurfaceAcquisitionAndReleaseWindowResources();

    void editsAndViewChangesUseSharedExecution();
    void displayChannelsChangeOnlyPresentation();
    void queuedUpdatesIgnoreClosedDocuments();
    void nativeSurfaceRecreationPreservesDocumentAndContinuesPresentation();
    void capacityFailurePreservesPublicationAndCanRetry();
    void busyFramesRetryTheLatestRequestedImage();
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
    QVERIFY(window.setDisplayConversion(admission.handle, identityDisplay(execution.requested(admission.handle))));
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
    for (int i = 0; i < 2; ++i) window.refresh();
    QTRY_COMPARE_WITH_TIMEOUT(execution.published(admission.handle), document.preview(), 5000);
    QCOMPARE(frames.last()[0].toULongLong(), admission.handle);
    QCOMPARE(frames.last()[1].toULongLong(), document.preview()->revision());
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

void KisGpuCanvasWindowTest::displayChannelsChangeOnlyPresentation()
{
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Display channels"), true, QUuid::createUuid(),
        {{{0, 0}, QByteArray::fromHex("4995e780").repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    const auto before = document.snapshot();
    const auto retainedVersion = document.preview();
    Execution execution(m_device);
    const auto handle = execution.open(before).handle;
    QVERIFY(handle);
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QVERIFY(window.setDisplayConversion(handle, identityDisplay(execution.requested(handle))));
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.resize(64, 64);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
#ifdef Q_OS_LINUX
    // X11's native window capture observes the presented surface, independently
    // of the offscreen render-target contract. The suite uses a private display.
    const auto displayed = [&] {
        const auto image = window.screen()->grabWindow(window.winId()).toImage();
        return image.rect().contains(8, 8) ? image.pixelColor(8, 8) : QColor{};
    };
    QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(164, 202, 243), 5000);
#endif
    QVERIFY(!window.setDisplayChannels({16, false}));
    const std::pair<DisplayChannels, QColor> cases[] = {
        {{14, false}, QColor(127, 202, 243)},
        {{9, false}, QColor(164, 164, 164)},
        {{0, false}, Qt::white},
        {{9, true}, QColor(164, 127, 127)}
    };
    for (const auto &entry : cases) {
        const auto count = frames.size();
        QVERIFY(window.setDisplayChannels(entry.first));
        QTRY_VERIFY_WITH_TIMEOUT(frames.size() > count, 5000);
#ifdef Q_OS_LINUX
        QTRY_COMPARE_WITH_TIMEOUT(displayed(), entry.second, 5000);
#endif
        QCOMPARE(execution.requested(handle), before);
        QCOMPARE(execution.published(handle), before);
        QCOMPARE(document.preview(), retainedVersion);
    }
    const auto count = frames.size();
    QVERIFY(window.resetSurface());
    QTRY_VERIFY_WITH_TIMEOUT(frames.size() > count, 5000);
#ifdef Q_OS_LINUX
    QTRY_COMPARE_WITH_TIMEOUT(displayed(), QColor(164, 127, 127), 5000);
#endif
    QCOMPARE(errors.size(), 0);
}

void KisGpuCanvasWindowTest::queuedUpdatesIgnoreClosedDocuments()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QVERIFY(window.setDisplayConversion(handle, identityDisplay(execution.requested(handle))));
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

void KisGpuCanvasWindowTest::busyFramesRetryTheLatestRequestedImage()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QVERIFY(window.setDisplayConversion(handle, identityDisplay(execution.requested(handle))));
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
    QTest::qWait(30);
    const auto before = frames.size();
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    QEvent update(QEvent::UpdateRequest);
    QCoreApplication::sendEvent(&window, &update);
    QCOMPARE(frames.size(), before);
    QCOMPARE(errors.size(), 0);
    QTest::qWait(100);
    QCOMPARE(frames.size(), before);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QTRY_COMPARE_WITH_TIMEOUT(execution.published(handle), latest, 5000);
    QVERIFY(frames.size() > before);
    QCOMPARE(errors.size(), 0);
}

void KisGpuCanvasWindowTest::nativeSurfaceRecreationPreservesDocumentAndContinuesPresentation()
{
    Execution execution(m_device);
    OperationDocument document({64, 64});
    const auto handle = execution.open(document.snapshot()).handle;
    CanvasWindow window(execution);
    QVERIFY(window.setDocument(handle));
    QVERIFY(window.setDisplayConversion(handle, identityDisplay(execution.requested(handle))));
    window.resize(128, 128);
    QSignalSpy frames(&window, &CanvasWindow::frameSubmitted);
    QSignalSpy errors(&window, &CanvasWindow::renderingFailed);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
    window.hide();
    window.destroy();
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
    const auto previousFrames = frames.size();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(frames.size() > previousFrames, 5000);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(errors.size(), 0);
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
    QVERIFY(window.setDisplayConversion(handle, identityDisplay(execution.requested(handle))));
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
    QCOMPARE(qvariant_cast<ExecutionCode>(errors.last()[1]), ExecutionCode::CapacityExceeded);
    QVERIFY(!errors.last()[2].toString().isEmpty());
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
    TilePresentation presentation(m_device);
    const auto drawn = presentation.record(frame.commands, projection.version,
        {m_device, frame.target, surface->size(), surface->format()}, QTransform(), Qt::white, identityDisplay(projection.version->document()));
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), document.snapshot());
    QCOMPARE(execution.pendingDocuments(), 0);
}

void KisGpuCanvasWindowTest::unconfiguredSurfacesAreRejectedBeforeNativeSubmission()
{
    Execution execution(m_device);
    QWindow window;
    LibrePaint::Canvas::Gpu::Detail::setNativeSurfaceType(window);
    window.resize(128, 128);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    if (QGuiApplication::platformName() == QLatin1String("offscreen")) {
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, LibrePaint::Canvas::Gpu::Detail::createWindowSurface(m_device, window));
        QVERIFY(!m_device->isRecordingFrame());
        QVERIFY(m_device->diagnostic().isEmpty());
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        return;
    }
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

void KisGpuCanvasWindowTest::failedDevicesRejectSurfaceAcquisitionAndReleaseWindowResources()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const std::weak_ptr<Device> lifetime = device;
    {
        QWindow window;
        LibrePaint::Canvas::Gpu::Detail::setNativeSurfaceType(window);
        window.resize(128, 128);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto surface = LibrePaint::Canvas::Gpu::Detail::createWindowSurface(device, window);
        QVERIFY(surface->configure(QSizeF(window.size() * window.devicePixelRatio()).toSize()));
        // A validation error fails the device without the wgpu-native paths that abort.
        WGPUBufferDescriptor invalid{};
        invalid.size = 256;
        invalid.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_Storage;
        wgpuBufferRelease(wgpuDeviceCreateBuffer(device->nativeDevice(), &invalid));
        QVERIFY(!device->diagnostic().isEmpty());
        const auto acquired = surface->acquire();
        QCOMPARE(acquired.code, ExecutionCode::DeviceFailure);
        QCOMPARE(acquired.diagnostic, device->diagnostic());
        QVERIFY(!surface->target());
        QVERIFY(!device->isRecordingFrame());
    }
    device.reset();
    QVERIFY(lifetime.expired());
}





int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuCanvasWindowTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuCanvasWindowTest.moc"
