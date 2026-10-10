/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SurfaceRenderer.h"
#include "KisGpuTestDevice.h"
#include <QGuiApplication>
#include <QWindow>
#include <QtTest>

namespace {
using Store = KisGpuTileStore;
using Renderer = Krita::Canvas::SurfaceRenderer;
Store::TextureSnapshot image(Store &store)
{
    const auto painted = store.fill(store.emptyVersion(), {0, 0, 32, 32}, 0xFF332211);
    auto snapshot = store.textureSnapshot(painted.version, {0, 0, 32, 32});
    QElapsedTimer timer; timer.start();
    while (snapshot.completion.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        store.poll(); QTest::qWait(1);
    }
    return snapshot;
}
}

/** Qt may recreate its native window while the canvas and document remain alive. */
class KisGpuSurfaceLifetimeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativeWindowRecreationRetainsImageAndSequence();
    void closingWindowAfterDeviceLossReleasesCanvasResources();
};

void KisGpuSurfaceLifetimeTest::nativeWindowRecreationRetainsImageAndSequence()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 4 * 1024 * 1024);
    const auto input = image(store);
    QCOMPARE(input.completion.status(), Store::Status::Succeeded);
    const auto before = store.statistics();
    const auto imageBytes = gpu.owner.memoryStatistics().reservedBytes;
    QWindow window;
    Renderer::prepareWindow(window);
    window.resize(64, 64);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
    Renderer renderer(gpu.owner, window, 4 * 1024 * 1024, 2);
    Krita::Canvas::GpuRenderer::View view;
    quint64 sequence = 0;
    for (int i = 0; i < 3; ++i) {
        if (i) {
            window.resize(64 + 16 * i, 64);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
        }
        const auto frame = renderer.present(input, view);
        QVERIFY(frame.accepted());
        const auto submitted = renderer.statistics();
        QVERIFY(submitted.acquireCallNanoseconds > 0);
        QVERIFY(submitted.nativePresentCallNanoseconds > 0);
        QVERIFY(frame.rendering.sequence() > sequence);
        sequence = frame.rendering.sequence();
        window.destroy();
        QCOMPARE(renderer.statistics().reservedSurfaceBytes, quint64(0));
        QCOMPARE(renderer.statistics().rendering.pendingFrames, quint32(0));
        QCOMPARE(renderer.statistics().pendingPresentations, quint32(0));
        QCOMPARE(frame.rendering.status(), Store::Status::Succeeded);
        QVERIFY(frame.presentation.status() != Renderer::Presentation::Status::Pending);
        QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, imageBytes);
        QCOMPARE(renderer.statistics().presentationRequests, quint64(i + 1));
        QCOMPARE(renderer.statistics().rendering.submissions, quint64(i + 1));
        QCOMPARE(renderer.statistics().acquireCallNanoseconds, submitted.acquireCallNanoseconds);
        QCOMPARE(renderer.statistics().nativePresentCallNanoseconds, submitted.nativePresentCallNanoseconds);
        QCOMPARE(renderer.present(input, view).error, Renderer::Error::WindowUnavailable);
    }
    QCOMPARE(store.statistics().submissions, before.submissions);
    QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

void KisGpuSurfaceLifetimeTest::closingWindowAfterDeviceLossReleasesCanvasResources()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, 4 * 1024 * 1024);
    const auto input = image(store);
    QCOMPARE(input.completion.status(), Store::Status::Succeeded);
    const auto imageBytes = gpu.owner.memoryStatistics().reservedBytes;
    QWindow window;
    Renderer::prepareWindow(window);
    window.resize(64, 64);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, 3000));
    Renderer renderer(gpu.owner, window, 4 * 1024 * 1024, 2);
    const auto frame = renderer.present(input, {});
    QVERIFY(frame.accepted());
    gpu.owner.destroy();
    QVERIFY(window.close());
    QCOMPARE(renderer.statistics().reservedSurfaceBytes, quint64(0));
    QCOMPARE(renderer.statistics().rendering.pendingFrames, quint32(0));
    QCOMPARE(renderer.statistics().pendingPresentations, quint32(0));
    QVERIFY(frame.rendering.status() != Store::Status::Pending);
    QVERIFY(frame.presentation.status() != Renderer::Presentation::Status::Pending);
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, imageBytes);
    QCOMPARE(renderer.present(input, {}).error, Renderer::Error::DeviceLost);
    QCOMPARE(gpu.owner.errorCount(), 0);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuSurfaceLifetimeTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuSurfaceLifetimeTest.moc"
