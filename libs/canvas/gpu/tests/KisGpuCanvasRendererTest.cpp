/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuRenderer.h"
#include "KisGpuDevice.h"
#include "KisGpuTileStore.h"
#include "KisGpuEditSession.h"
#include "KisGpuLayerProjection.h"
#include "KisGpuLayerDocument.h"
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
    void globalBudgetRefusalCanBeRetried();
    void layerProjectionFollowsPreviewHistoryAndDocumentSwitch();
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

void KisGpuCanvasRendererTest::layerProjectionFollowsPreviewHistoryAndDocumentSwitch()
{
    using Session = KisGpuEditSession;
    using Projection = KisGpuLayerProjection;
    using Document = KisGpuLayerDocument;
    const QRect bounds(0, 0, 2, 2);
    auto ownedStore = std::make_shared<Store>(m_gpu->owner, 128 * Store::TileBytes);
    auto &store = *ownedStore;
    Document model(ownedStore, 4), otherModel(ownedStore, 4);
    Projection document(store, bounds), anotherDocument(store, bounds);
    for (auto *imageModel : {&model, &otherModel}) {
        const auto background = imageModel->addLayer(QStringLiteral("Background"));
        const auto filling = imageModel->begin(background.id);
        Store::DabCommand blue;
        blue.center = QPointF(0, 0);
        blue.diameter = QSizeF(100, 100);
        blue.rgba = 0xFFFF0000;
        QCOMPARE(filling.session->append(filling.token, {blue}, bounds), Session::Result::Accepted);
        imageModel->commit(filling.token);
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        imageModel->poll();
        QCOMPARE(imageModel->state(), Document::State::Idle);
    }
    const auto foreground = model.addLayer(QStringLiteral("Foreground"));
    const auto editing = model.begin(foreground.id);
    auto &edit = *editing.session;
    Store::DabCommand dab;
    dab.center = QPointF(0, 0);
    dab.diameter = QSizeF(.5, .5);
    dab.rgba = 0xFF0000FF;
    const auto token = editing.token;
    QCOMPARE(edit.append(token, {dab}, bounds), Session::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    model.poll();
    QCOMPARE(document.setLayers(model.projectionLayers()), Store::Error::None);
    QCOMPARE(anotherDocument.setLayers(otherModel.projectionLayers()), Store::Error::None);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    document.poll();
    anotherDocument.poll();
    const auto oldImage = store.textureSnapshot(document.snapshot().pixels, bounds);
    QVERIFY(finish(store, oldImage.completion));
    Renderer renderer(m_gpu->owner, WGPUTextureFormat_RGBA8Unorm);
    Renderer::View view;
    view.sampling = Renderer::Sampling::Nearest;
    Target oldTarget(*m_gpu, bounds.size()), newTarget(*m_gpu, bounds.size()), otherTarget(*m_gpu, bounds.size());
    const auto oldFrame = renderer.render(oldImage, oldTarget.texture, view);
    QCOMPARE(oldFrame.error, Renderer::Error::None);
    dab.center = QPointF(1, 1);
    dab.rgba = 0xFF00FF00;
    QCOMPARE(edit.replace(token, {dab}, bounds), Session::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    model.poll();
    const auto preview = edit.preview();
    QCOMPARE(document.setLayers(model.projectionLayers()), Store::Error::None);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    document.poll();
    const auto replacementImage = store.textureSnapshot(document.snapshot().pixels, bounds);
    const auto otherImage = store.textureSnapshot(anotherDocument.snapshot().pixels, bounds);
    QVERIFY(finish(store, otherImage.completion));
    const auto newFrame = renderer.render(replacementImage, newTarget.texture, view);
    const auto otherFrame = renderer.render(otherImage, otherTarget.texture, view);
    QVERIFY(finish(renderer, oldFrame));
    QVERIFY(finish(renderer, newFrame));
    QVERIFY(finish(renderer, otherFrame));
    const QByteArray blue = QByteArray::fromHex("0000ffff");
    QCOMPARE(m_gpu->read(oldTarget.texture, bounds.size()), QByteArray::fromHex("ff0000ff") + blue.repeated(3));
    QCOMPARE(m_gpu->read(newTarget.texture, bounds.size()), blue.repeated(3) + QByteArray::fromHex("00ff00ff"));
    QCOMPARE(m_gpu->read(otherTarget.texture, bounds.size()), blue.repeated(4));
    QCOMPARE(model.commit(token), Document::Result::Accepted);
    model.poll();
    QCOMPARE(model.state(), Document::State::Idle);
    const auto beforeCommitProjection = store.statistics();
    QCOMPARE(document.setLayers(model.projectionLayers()), Store::Error::None);
    document.poll();
    QCOMPARE(store.statistics().submissions, beforeCommitProjection.submissions);
    QVERIFY(model.undo());
    QCOMPARE(document.setLayers(model.projectionLayers()), Store::Error::None);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    document.poll();
    const auto undoImage = store.textureSnapshot(document.snapshot().pixels, bounds);
    QVERIFY(finish(store, undoImage.completion));
    QVERIFY(finish(renderer, renderer.render(undoImage, newTarget.texture, view)));
    QCOMPARE(m_gpu->read(newTarget.texture, bounds.size()), blue.repeated(4));
    QVERIFY(model.redo());
    QCOMPARE(document.setLayers(model.projectionLayers()), Store::Error::None);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    document.poll();
    QVERIFY(document.snapshot().layers[1].pixels == preview);
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

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

void KisGpuCanvasRendererTest::globalBudgetRefusalCanBeRetried()
{
    KisGpuTestDevice gpu(0, false, 4 * Store::TileBytes);
    Store store(gpu.owner, 8 * Store::TileBytes);
    auto source = image(store, {0, 0, 1, 1}, QByteArray::fromHex("ff0000ff"));
    Target target(gpu, {1, 1});
    Renderer renderer(gpu.owner, WGPUTextureFormat_RGBA8Unorm);
    const auto baseline = gpu.owner.memoryStatistics().reservedBytes;
    auto occupied = gpu.owner.reserveMemory(gpu.owner.availableMemory());
    QVERIFY(bool(occupied));
    const auto refused = renderer.render(source, target.texture, {});
    QCOMPARE(refused.error, Renderer::Error::BudgetExceeded);
    QCOMPARE(renderer.statistics().submissions, quint64(0));
    QCOMPARE(renderer.statistics().residentParameterBytes, quint64(0));
    occupied = {};
    auto accepted = renderer.render(source, target.texture, {});
    QCOMPARE(accepted.error, Renderer::Error::None);
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, baseline + 48);
    QVERIFY(finish(renderer, accepted));
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, baseline);
    source = {};
    QCOMPARE(gpu.owner.memoryStatistics().reservedBytes, quint64(0));
    QCOMPARE(gpu.owner.errorCount(), 0);
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
