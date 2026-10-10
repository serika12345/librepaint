/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuRenderer.h"
#include "KisGpuDevice.h"
#include "KisGpuTileStore.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
using Renderer = Krita::Canvas::GpuRenderer;
using Store = KisGpuTileStore;
struct Target {
    WGPUTexture texture;
    Target(KisGpuTestDevice &gpu, QSize size, WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm)
    {
        WGPUTextureDescriptor descriptor{};
        descriptor.dimension = WGPUTextureDimension_2D;
        descriptor.size = {quint32(size.width()), quint32(size.height()), 1};
        descriptor.format = format;
        descriptor.mipLevelCount = 1;
        descriptor.sampleCount = 1;
        descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        texture = wgpuDeviceCreateTexture(gpu.device, &descriptor);
    }
    ~Target() { wgpuTextureRelease(texture); }
};

template<typename Operation, typename Result>
bool finish(Operation &owner, const Result &result)
{
    QElapsedTimer timer;
    timer.start();
    while (result.status() == Store::Status::Pending && timer.elapsed() < 5000) {
        owner.poll();
        QTest::qWait(1);
    }
    owner.poll();
    return result.status() == Store::Status::Succeeded;
}

Store::TextureSnapshot image(Store &store, QRect bounds, const QByteArray &pixels)
{
    auto uploaded = store.upload(store.emptyVersion(), bounds, pixels);
    auto snapshot = store.textureSnapshot(uploaded.version, bounds);
    if (!finish(store, snapshot.completion)) return {};
    return snapshot;
}
const QByteArray Pattern = QByteArray::fromHex("ff0000ff00ff00ff0000ffffffff00ff");
}

/** Canvas consumers observe transformed pixels, bounded work and GPU resource retention. */
class KisGpuCanvasRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void transformedPixels_data();
    void transformedPixels();
    void interpolationKeepsTransparentColorsOutOfVisiblePixels();
    void backgroundAndBgraTarget();
    void boundedFramesPinResourcesUntilCollection();
    void frameOutlivesRenderer();
    void rejectedFramesLeaveQueueUnchanged();
    void foreignTargetFailsWithoutLosingDevice();
    void destroyedTargetFailsWithoutLosingDevice();
    void rejectsUnavailableConfiguration();
    void deviceLossStopsRendering();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuCanvasRendererTest::initTestCase()
{
    m_gpu = std::make_unique<KisGpuTestDevice>();
    qInfo() << "GPU:" << m_gpu->name;
}

void KisGpuCanvasRendererTest::cleanup()
{
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    QCOMPARE(m_gpu->owner.errorCount(), 0);
}

void KisGpuCanvasRendererTest::transformedPixels_data()
{
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QSize>("size");
    QTest::newRow("origin") << QTransform::fromTranslate(-5, 3) << QSize(2, 2);
    QTransform zoom;
    zoom.scale(2, 2);
    zoom.translate(-5, 3);
    QTest::newRow("zoom") << zoom << QSize(4, 4);
    QTransform rotate;
    rotate.translate(2, 0);
    rotate.rotate(90);
    rotate.translate(-5, 3);
    QTest::newRow("quarter-turn") << rotate << QSize(2, 2);
    QTest::newRow("mirror") << QTransform(-1, 0, 0, 1, 7, 3) << QSize(2, 2);
    QTransform arbitrary;
    arbitrary.translate(4, 2);
    arbitrary.rotate(31);
    arbitrary.scale(1.3, 1.7);
    arbitrary.translate(-5, 3);
    QTest::newRow("arbitrary-rotation") << arbitrary << QSize(8, 8);
}

void KisGpuCanvasRendererTest::transformedPixels()
{
    QFETCH(QTransform, transform);
    QFETCH(QSize, size);
    Store store(m_gpu->owner, 8 * Store::TileBytes);
    const auto source = image(store, QRect(5, -3, 2, 2), Pattern);
    QVERIFY(source.texture());
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    Target target(*m_gpu, size);
    Renderer::View view;
    view.canvasToTarget = transform;
    view.backgroundRgba = 0xFF302010;
    view.sampling = Renderer::Sampling::Nearest;
    const auto before = store.statistics();
    const auto frame = renderer.render(source, target.texture, view);
    QCOMPARE(frame.error, Renderer::Error::None);
    QVERIFY(finish(renderer, frame));
    QByteArray expected;
    const auto inverse = transform.inverted();
    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            const QPointF canvas = inverse.map(QPointF(x + 0.5, y + 0.5)) - QPointF(5, -3);
            const int sx = std::floor(canvas.x()), sy = std::floor(canvas.y());
            expected += sx >= 0 && sx < 2 && sy >= 0 && sy < 2
                ? Pattern.mid((sy * 2 + sx) * 4, 4) : QByteArray::fromHex("102030ff");
        }
    }
    QCOMPARE(m_gpu->read(target.texture, size), expected);
    QCOMPARE(store.statistics().pixelUploadBytes, before.pixelUploadBytes);
    QCOMPARE(store.statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(renderer.statistics().submissions, quint64(1));
    QVERIFY(renderer.statistics().parameterUploadBytes <= 128);
}

void KisGpuCanvasRendererTest::interpolationKeepsTransparentColorsOutOfVisiblePixels()
{
    Store store(m_gpu->owner, 4 * Store::TileBytes);
    const auto source = image(store, QRect(0, 0, 2, 1), QByteArray::fromHex("ff0000ff00ff0000"));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    Target target(*m_gpu, QSize(3, 1));
    Renderer::View view;
    view.canvasToTarget.translate(0.5, 0);
    view.backgroundRgba = 0xFF000000;
    auto frame = renderer.render(source, target.texture, view);
    QCOMPARE(frame.error, Renderer::Error::None);
    QVERIFY(finish(renderer, frame));
    QCOMPARE(m_gpu->read(target.texture, QSize(3, 1)), QByteArray::fromHex("800000ff800000ff000000ff"));
}

void KisGpuCanvasRendererTest::backgroundAndBgraTarget()
{
    Store store(m_gpu->owner, 4 * Store::TileBytes);
    const auto source = image(store, QRect(0, 0, 1, 1), QByteArray::fromHex("ff000080"));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_BGRA8Unorm);
    Target target(*m_gpu, QSize(2, 1), WGPUTextureFormat_BGRA8Unorm);
    Renderer::View view;
    view.backgroundRgba = 0xFFFF0000;
    view.sampling = Renderer::Sampling::Nearest;
    auto frame = renderer.render(source, target.texture, view);
    QCOMPARE(frame.error, Renderer::Error::None);
    QVERIFY(finish(renderer, frame));
    QCOMPARE(m_gpu->read(target.texture, QSize(2, 1)), QByteArray::fromHex("7f0080ffff0000ff"));
}

void KisGpuCanvasRendererTest::boundedFramesPinResourcesUntilCollection()
{
    Store store(m_gpu->owner, Store::TileBytes);
    auto source = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 2, 2));
    QVERIFY(finish(store, source.completion));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm, 1);
    Target target(*m_gpu, QSize(2, 2));
    auto frame = renderer.render(source, target.texture, {});
    QCOMPARE(frame.error, Renderer::Error::None);
    QCOMPARE(renderer.render(source, target.texture, {}).error, Renderer::Error::QueueFull);
    QCOMPARE(renderer.statistics().submissions, quint64(1));
    QCOMPARE(renderer.statistics().pendingFrames, quint32(1));
    QVERIFY(renderer.statistics().residentParameterBytes > 0);
    source = {};
    frame = {};
    QCOMPARE(store.statistics().residentBytes, quint64(16));
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    QCOMPARE(store.statistics().residentBytes, quint64(16));
    renderer.poll();
    QCOMPARE(store.statistics().residentBytes, quint64(0));
    QCOMPARE(renderer.statistics().residentParameterBytes, quint64(0));
    QCOMPARE(renderer.statistics().pendingFrames, quint32(0));
    auto retry = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 2, 2));
    QVERIFY(finish(store, retry.completion));
    QCOMPARE(renderer.render(retry, target.texture, {}).error, Renderer::Error::None);
}

void KisGpuCanvasRendererTest::frameOutlivesRenderer()
{
    Store store(m_gpu->owner, 4 * Store::TileBytes);
    auto source = image(store, QRect(0, 0, 2, 2), Pattern);
    Target target(*m_gpu, QSize(2, 2));
    Renderer::Frame frame;
    {
        Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
        frame = renderer.render(source, target.texture, {});
        QCOMPARE(frame.error, Renderer::Error::None);
    }
    QCOMPARE(frame.status(), Store::Status::Succeeded);
    QCOMPARE(frame.sequence(), quint64(1));
    QCOMPARE(m_gpu->read(target.texture, QSize(2, 2)), Pattern);
    wgpuTextureDestroy(target.texture);
    {
        Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
        frame = renderer.render(source, target.texture, {});
        QCOMPARE(frame.error, Renderer::Error::None);
    }
    QCOMPARE(frame.status(), Store::Status::Failed);
}

void KisGpuCanvasRendererTest::rejectedFramesLeaveQueueUnchanged()
{
    Store store(m_gpu->owner, 4 * Store::TileBytes);
    auto source = image(store, QRect(0, 0, 2, 2), Pattern);
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    Target target(*m_gpu, QSize(2, 2)), wrongFormat(*m_gpu, QSize(2, 2), WGPUTextureFormat_BGRA8Unorm);
    QCOMPARE(renderer.render({}, target.texture, {}).error, Renderer::Error::InvalidImage);
    auto pending = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 2, 2));
    QCOMPARE(renderer.render(pending, target.texture, {}).error, Renderer::Error::ImagePending);
    QCOMPARE(renderer.render(source, nullptr, {}).error, Renderer::Error::InvalidTarget);
    QCOMPARE(renderer.render(source, wrongFormat.texture, {}).error, Renderer::Error::InvalidTarget);
    Renderer::View view;
    view.canvasToTarget.scale(0, 1);
    QCOMPARE(renderer.render(source, target.texture, view).error, Renderer::Error::InvalidTransform);
    view.canvasToTarget = QTransform(1, 0, 0.1, 0, 1, 0, 0, 0, 1);
    QCOMPARE(renderer.render(source, target.texture, view).error, Renderer::Error::InvalidTransform);
    view.canvasToTarget = QTransform::fromTranslate(std::numeric_limits<qreal>::infinity(), 0);
    QCOMPARE(renderer.render(source, target.texture, view).error, Renderer::Error::InvalidTransform);
    KisGpuTestDevice otherGpu;
    Store otherStore(otherGpu.owner, Store::TileBytes);
    auto foreign = otherStore.textureSnapshot(otherStore.emptyVersion(), QRect(0, 0, 1, 1));
    QVERIFY(finish(otherStore, foreign.completion));
    QCOMPARE(renderer.render(foreign, target.texture, {}).error, Renderer::Error::InvalidImage);
    QCOMPARE(renderer.statistics().submissions, quint64(0));
    QCOMPARE(renderer.statistics().residentParameterBytes, quint64(0));
}

void KisGpuCanvasRendererTest::foreignTargetFailsWithoutLosingDevice()
{
    Store store(m_gpu->owner, Store::TileBytes);
    auto source = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QVERIFY(finish(store, source.completion));
    KisGpuTestDevice otherGpu;
    Target foreign(otherGpu, QSize(1, 1));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    auto frame = renderer.render(source, foreign.texture, {});
    QCOMPARE(frame.error, Renderer::Error::InvalidTarget);
    QCOMPARE(renderer.statistics().submissions, quint64(0));
    QVERIFY(m_gpu->owner.available());
    Target valid(*m_gpu, QSize(1, 1));
    auto retry = renderer.render(source, valid.texture, {});
    QCOMPARE(retry.error, Renderer::Error::None);
    QVERIFY(finish(renderer, retry));
}

void KisGpuCanvasRendererTest::deviceLossStopsRendering()
{
    KisGpuTestDevice gpu;
    Store store(gpu.owner, Store::TileBytes);
    auto source = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QVERIFY(finish(store, source.completion));
    Target target(gpu, QSize(1, 1));
    Renderer renderer(gpu.owner, WGPUTextureFormat_RGBA8Unorm);
    auto frame = renderer.render(source, target.texture, {});
    QCOMPARE(frame.error, Renderer::Error::None);
    gpu.owner.destroy();
    renderer.poll();
    QCOMPARE(frame.status(), Store::Status::Failed);
    QCOMPARE(renderer.render(source, target.texture, {}).error, Renderer::Error::DeviceLost);
    QCOMPARE(renderer.statistics().residentParameterBytes, quint64(0));
}

void KisGpuCanvasRendererTest::destroyedTargetFailsWithoutLosingDevice()
{
    Store store(m_gpu->owner, Store::TileBytes);
    auto source = store.textureSnapshot(store.emptyVersion(), QRect(0, 0, 1, 1));
    QVERIFY(finish(store, source.completion));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    Target target(*m_gpu, QSize(1, 1));
    wgpuTextureDestroy(target.texture);
    auto frame = renderer.render(source, target.texture, {});
    QCOMPARE(frame.error, Renderer::Error::None);
    QVERIFY(!finish(renderer, frame));
    QCOMPARE(frame.status(), Store::Status::Failed);
    QCOMPARE(renderer.statistics().pendingFrames, quint32(0));
    QVERIFY(m_gpu->owner.available());
}

void KisGpuCanvasRendererTest::rejectsUnavailableConfiguration()
{
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, Renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm, 0));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, Renderer(m_gpu->owner, WGPUTextureFormat_RGBA8UnormSrgb));
    KisGpuTestDevice lost;
    lost.owner.destroy();
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, Renderer(lost.owner, WGPUTextureFormat_RGBA8Unorm));
}

QTEST_GUILESS_MAIN(KisGpuCanvasRendererTest)
#include "KisGpuCanvasRendererTest.moc"
