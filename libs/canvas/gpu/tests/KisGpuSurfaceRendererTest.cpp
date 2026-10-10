/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SurfaceRenderer.h"
#include "GpuWindowSurface_p.h"
#include "KisGpuTestDevice.h"
#include <QGuiApplication>
#include <QWindow>
#include <QProcess>
#include <QtTest>
#include <stdexcept>
using namespace Krita::Canvas;
namespace {
int nativeSurfaceCase(const QString &mode)
{
    KisGpuDevice device;
    QWindow window;
    SurfaceRenderer::prepareWindow(window);
    window.resize(64, 64);
    window.show();
    if (!QTest::qWaitForWindowExposed(&window, 3000)) return 2;
    if (mode == QStringLiteral("unsupported-platform")) {
        try { SurfaceRenderer renderer(device, window); }
        catch (const std::runtime_error &) { return device.available() ? 0 : 1; }
        return 1;
    }
    auto surface = std::make_unique<GpuWindowSurface>(device, window);
    const auto size = quint32(qRound(64 * window.devicePixelRatio()));
    WGPUSurfaceConfiguration configuration{};
    configuration.device = device.device();
    configuration.format = WGPUTextureFormat_BGRA8Unorm;
    configuration.usage = WGPUTextureUsage_RenderAttachment;
    configuration.width = size;
    configuration.height = size;
    configuration.presentMode = WGPUPresentMode_Fifo;
    configuration.alphaMode = WGPUCompositeAlphaMode_Opaque;
    wgpuSurfaceConfigure(surface->surface(), &configuration);
    const auto queue = wgpuDeviceGetQueue(device.device());
    WGPUTexture previous = nullptr;
    bool succeeded = true;
    for (int i = 0; i < (mode == QStringLiteral("retained") ? 3 : 1); ++i) {
        WGPUSurfaceTexture current{};
        wgpuSurfaceGetCurrentTexture(surface->surface(), &current);
        if (!current.texture) { succeeded = false; break; }
        if (previous) wgpuTextureRelease(previous);
        const auto view = wgpuTextureCreateView(current.texture, nullptr);
        const auto encoder = wgpuDeviceCreateCommandEncoder(device.device(), nullptr);
        WGPURenderPassColorAttachment color{};
        color.view = view;
        color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color.loadOp = WGPULoadOp_Clear;
        color.storeOp = WGPUStoreOp_Store;
        color.clearValue = {1, 0, 0, 1};
        WGPURenderPassDescriptor descriptor{};
        descriptor.colorAttachmentCount = 1;
        descriptor.colorAttachments = &color;
        const auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &descriptor);
        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);
        const auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &commands);
        succeeded = wgpuSurfacePresent(surface->surface()) == WGPUStatus_Success && succeeded;
        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(view);
        previous = current.texture;
        wgpuDevicePoll(device.device(), true, nullptr);
        QCoreApplication::processEvents();
    }
    bool expectedLoss = false;
    if (mode == QStringLiteral("detach") || mode == QStringLiteral("discard-after-detach")) {
        WGPUSurfaceTexture extra{};
        if (mode == QStringLiteral("discard-after-detach")) wgpuSurfaceGetCurrentTexture(surface->surface(), &extra);
        surface.reset();
        if (extra.texture) wgpuTextureRelease(extra.texture);
    } else if (mode == QStringLiteral("unconfigured-read")) {
        wgpuSurfaceUnconfigure(surface->surface());
        WGPUSurfaceTexture extra{};
        wgpuSurfaceGetCurrentTexture(surface->surface(), &extra);
        succeeded = succeeded && !extra.texture && extra.status == WGPUSurfaceGetCurrentTextureStatus_Error;
    } else {
        if (mode != QStringLiteral("retained")) {
            device.destroy();
            expectedLoss = true;
            if (mode == QStringLiteral("acquire-after-loss")) {
                WGPUSurfaceTexture extra{};
                wgpuSurfaceGetCurrentTexture(surface->surface(), &extra);
                succeeded = succeeded && !extra.texture && extra.status == WGPUSurfaceGetCurrentTextureStatus_Error;
            } else {
                wgpuSurfaceConfigure(surface->surface(), &configuration);
                succeeded = succeeded && device.errorCount() > 0;
            }
        }
    }
    if (previous) wgpuTextureRelease(previous);
    wgpuQueueRelease(queue);
    return succeeded && device.available() != expectedLoss ? 0 : 1;
}
}
class KisGpuSurfaceRendererTest : public QObject
{
    Q_OBJECT
    void expose(QWindow &window) {
        SurfaceRenderer::prepareWindow(window);
        window.resize(64, 64);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
    }
    static KisGpuTileStore::TextureSnapshot image(KisGpuTileStore &store) {
        const auto painted = store.fill(store.emptyVersion(), {0, 0, 32, 32}, 0xFF0000FF);
        auto result = store.textureSnapshot(painted.version, {0, 0, 32, 32});
        QElapsedTimer timer;
        timer.start();
        while (result.completion.status() == KisGpuTileStore::Status::Pending && timer.elapsed() < 3000) store.poll();
        return result;
    }
private Q_SLOTS:
    void nativeSurfaceLifetimeAndFailure_data() {
        QTest::addColumn<QString>("operation");
        for (const auto *name : {"retained", "detach", "discard-after-detach", "unconfigured-read",
                                 "acquire-after-loss", "configure-after-loss", "unsupported-platform"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void nativeSurfaceLifetimeAndFailure() {
        QFETCH(QString, operation);
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        if (operation == QStringLiteral("unsupported-platform")) {
            auto environment = QProcessEnvironment::systemEnvironment();
            environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
            process.setProcessEnvironment(environment);
        }
        process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--native-surface-case"), operation});
        QVERIFY(process.waitForStarted(5000));
        if (!process.waitForFinished(10000)) {
            process.kill();
            process.waitForFinished();
            QFAIL("Native presentation operation did not return");
        }
        const auto diagnostic = process.readAll();
        QVERIFY2(process.exitStatus() == QProcess::NormalExit, diagnostic.constData());
        QVERIFY2(process.exitCode() == 0, diagnostic.constData());
        QVERIFY2(!diagnostic.contains("VUID-") && !diagnostic.contains("Validation Error"), diagnostic.constData());
    }
    void consecutiveFramesKeepOlderTargetsAlive() {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 4 * 1024 * 1024);
        auto input = image(store);
        QWindow window;
        expose(window);
        SurfaceRenderer renderer(gpu.owner, window, 4 * 1024 * 1024, 2);
        GpuRenderer::View view;
        QList<SurfaceRenderer::Frame> frames;
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < 6; ++i) {
            QVERIFY(timer.elapsed() < 3000);
            auto frame = renderer.present(input, view);
            if (frame.error == SurfaceRenderer::Error::QueueFull) {
                renderer.poll();
                --i;
                continue;
            }
            QVERIFY(frame.accepted());
            frames.append(frame);
        }
        QTRY_VERIFY_WITH_TIMEOUT((renderer.poll(), renderer.statistics().rendering.pendingFrames == 0), 3000);
        for (const auto &frame : frames) QCOMPARE(frame.rendering.status(), GpuRenderer::Status::Succeeded);
        QCOMPARE(renderer.statistics().presentationRequests, quint64(6));
        QCOMPARE(renderer.statistics().rendering.parameterUploadBytes, quint64(6 * 48));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void resizeWaitsForRetirementAndUsesPhysicalPixels() {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 4 * 1024 * 1024);
        auto input = image(store);
        QWindow window;
        expose(window);
        SurfaceRenderer renderer(gpu.owner, window);
        auto before = renderer.present(input, {});
        QVERIFY(before.accepted());
        window.resize(80, 48);
        QCoreApplication::processEvents();
        QCOMPARE(renderer.present(input, {}).error, SurfaceRenderer::Error::ResizePending);
        QTRY_VERIFY_WITH_TIMEOUT((renderer.poll(), before.rendering.status() != GpuRenderer::Status::Pending), 3000);
        SurfaceRenderer::Frame after;
        QElapsedTimer resizeDeadline;
        resizeDeadline.start();
        do {
            after = renderer.present(input, {});
            if (after.error == SurfaceRenderer::Error::ResizePending) QTest::qWait(1);
        } while (after.error == SurfaceRenderer::Error::ResizePending && resizeDeadline.elapsed() < 3000);
        QVERIFY(after.accepted());
        QCOMPARE(renderer.statistics().physicalSize, QSize(qRound(80 * window.devicePixelRatio()), qRound(48 * window.devicePixelRatio())));
    }
    void hiddenWindowAndBudgetLeaveQueueUnchanged() {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 4 * 1024 * 1024);
        auto input = image(store);
        QWindow window;
        expose(window);
        SurfaceRenderer renderer(gpu.owner, window, 1);
        QCOMPARE(renderer.statistics().reservedSurfaceBytes, quint64(0));
        QCOMPARE(renderer.present(input, {}).error, SurfaceRenderer::Error::BudgetExceeded);
        QCOMPARE(renderer.statistics().rendering.submissions, quint64(0));
        window.hide();
        QCoreApplication::processEvents();
        QCOMPARE(renderer.present(input, {}).error, SurfaceRenderer::Error::WindowUnavailable);
    }
    void rejectedImageCanBeFollowedByValidPresentation() {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 4 * 1024 * 1024);
        auto input = image(store);
        QWindow window;
        expose(window);
        SurfaceRenderer renderer(gpu.owner, window);
        QCOMPARE(renderer.present({}, {}).error, SurfaceRenderer::Error::ImageRejected);
        GpuRenderer::View singular;
        singular.canvasToTarget.scale(0, 1);
        QCOMPARE(renderer.present(input, singular).error, SurfaceRenderer::Error::ImageRejected);
        auto frame = renderer.present(input, {});
        QVERIFY(frame.accepted());
        QTRY_VERIFY_WITH_TIMEOUT((renderer.poll(), frame.rendering.status() != GpuRenderer::Status::Pending), 3000);
        QCOMPARE(frame.rendering.status(), GpuRenderer::Status::Succeeded);
        QCOMPARE(renderer.statistics().presentationRequests, quint64(1));
    }
    void framesFinishAfterOwnerDestructionAndDeviceLoss() {
        KisGpuTestDevice gpu;
        KisGpuTileStore store(gpu.owner, 4 * 1024 * 1024);
        auto input = image(store);
        QWindow window;
        expose(window);
        SurfaceRenderer::Frame frame;
        {
            SurfaceRenderer renderer(gpu.owner, window);
            frame = renderer.present(input, {});
            QVERIFY(frame.accepted());
        }
        QCOMPARE(frame.rendering.status(), GpuRenderer::Status::Succeeded);
        SurfaceRenderer renderer(gpu.owner, window);
        frame = renderer.present(input, {});
        QVERIFY(frame.accepted());
        gpu.owner.destroy();
        renderer.poll();
        QCOMPARE(frame.rendering.status(), GpuRenderer::Status::Failed);
        QCOMPARE(renderer.present(input, {}).error, SurfaceRenderer::Error::DeviceLost);
    }
    void invalidConstructionIsReported() {
        KisGpuTestDevice gpu;
        QWindow window;
        QVERIFY_EXCEPTION_THROWN(SurfaceRenderer(gpu.owner, window), std::runtime_error);
        expose(window);
        QVERIFY_EXCEPTION_THROWN(SurfaceRenderer(gpu.owner, window, 0), std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(SurfaceRenderer(gpu.owner, window, 1024, 0), std::runtime_error);
    }
};
int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--native-surface-case")) {
        try { return nativeSurfaceCase(QString::fromLocal8Bit(argv[2])); }
        catch (const std::exception &error) { qCritical() << error.what(); return 2; }
    }
    KisGpuSurfaceRendererTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuSurfaceRendererTest.moc"
