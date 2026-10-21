/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTest>
#include <QTransform>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QScopeGuard>
#include "../KisGpuPresentationResources_p.h"
#include <gpu/KisGpuTilePresentation.h>
#include <gpu/KisGpuTileRenderer.h>
#include <limits>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cmath>

using namespace LibrePaint::Image::Gpu;
using namespace LibrePaint::Canvas::Gpu;

class KisGpuTilePresentationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void invalidCropPreviewsPreserveTheTarget_data();
    void invalidCropPreviewsPreserveTheTarget();
    void cropPreviewRetainsAntialiasedGeometryAndDocument_data();
    void cropPreviewRetainsAntialiasedGeometryAndDocument();
    void panZoomAndTransparencyUseThePaintingQueue();
    void tileOverBackgroundMatchesEightBitSourceOver_data();
    void tileOverBackgroundMatchesEightBitSourceOver();
    void checkerBackgroundPreservesDocumentPixels_data();
    void checkerBackgroundPreservesDocumentPixels();
    void invalidCheckerPatternsPreserveTheTarget_data();
    void invalidCheckerPatternsPreserveTheTarget();
    void clippedTilesKeepTheirIndependentTransforms();
    void displayChannelsPreserveDocumentPixels_data();
    void displayChannelsPreserveDocumentPixels();
    void rejectedPresentationPreservesTheTarget();
    void projectionCannotCrossDevices();
    void displayResourcesRetainTheirNativeDevice();
    void toolOutlinesMoveAndClearWithoutChangingDocumentPixels_data();
    void toolOutlinesMoveAndClearWithoutChangingDocumentPixels();
    void invalidToolOutlinesPreserveTheTarget_data();
    void invalidToolOutlinesPreserveTheTarget();

private:
    DevicePtr m_device;
};

namespace {
struct Surface
{
    LibrePaint::Canvas::Gpu::Detail::Texture texture;
    LibrePaint::Canvas::Gpu::Detail::TextureView view;
    PresentationTarget target;
    bool ready = false;
    Surface(DevicePtr device, QSize size, quint32 format = WGPUTextureFormat_RGBA8Unorm)
    {
        WGPUTextureDescriptor descriptor{};
        descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        descriptor.dimension = WGPUTextureDimension_2D;
        descriptor.size = {quint32(size.width()), quint32(size.height()), 1};
        descriptor.format = WGPUTextureFormat(format);
        descriptor.mipLevelCount = 1;
        descriptor.sampleCount = 1;
        texture.reset(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
        if (!texture) return;
        view.reset(wgpuTextureCreateView(texture.get(), nullptr));
        target = {std::move(device), view.get(), size, format};
        ready = bool(view);
    }
};
void clear(WGPUCommandEncoder commands, const PresentationTarget &target, QColor color)
{
    WGPURenderPassColorAttachment attachment{};
    attachment.view = target.view;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    attachment.clearValue = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
    WGPURenderPassDescriptor descriptor{};
    descriptor.colorAttachmentCount = 1;
    descriptor.colorAttachments = &attachment;
    const auto pass = wgpuCommandEncoderBeginRenderPass(commands, &descriptor);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
}
struct Readback
{
    struct Result {
        LibrePaint::Canvas::Gpu::Detail::Buffer buffer;
        QByteArray data;
        quint64 size = 0;
        std::atomic<bool> ready{false};
    };
    std::vector<std::shared_ptr<Result>> results;
    void copy(WGPUCommandEncoder commands, WGPUBuffer source, quint64 size, Device &device)
    {
        auto result = allocate(device, size);
        wgpuCommandEncoderCopyBufferToBuffer(commands, source, 0, result->buffer.get(), 0, size);
    }
    std::shared_ptr<Result> allocate(Device &device, quint64 size)
    {
        auto result = std::make_shared<Result>();
        WGPUBufferDescriptor descriptor{};
        descriptor.size = result->size = size;
        descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
        result->buffer.reset(wgpuDeviceCreateBuffer(device.nativeDevice(), &descriptor));
        results.push_back(result);
        return result;
    }
    bool finish(Device &device)
    {
        for (const auto &result : results) {
            WGPUBufferMapCallbackInfo callback{};
            callback.mode = WGPUCallbackMode_AllowSpontaneous;
            callback.userdata1 = new std::shared_ptr<Result>(result);
            callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *context, void *) noexcept {
                std::unique_ptr<std::shared_ptr<Result>> holder(static_cast<std::shared_ptr<Result> *>(context));
                auto &result = **holder;
                if (status != WGPUMapAsyncStatus_Success) return;
                try {
                    if (const auto bytes = wgpuBufferGetConstMappedRange(result.buffer.get(), 0, result.size)) {
                        result.data = QByteArray(static_cast<const char *>(bytes), qsizetype(result.size));
                    }
                } catch (...) {}
                wgpuBufferUnmap(result.buffer.get());
                result.ready.store(!result.data.isEmpty(), std::memory_order_release);
            };
            wgpuBufferMapAsync(result->buffer.get(), WGPUMapMode_Read, 0, result->size, callback);
        }
        return device.finish() && std::all_of(results.begin(), results.end(), [](const auto &result) { return result->ready.load(std::memory_order_acquire); });
    }
};
std::shared_ptr<Readback> readTexture(Device &device, WGPUCommandEncoder commands, const Surface &surface, WGPUBuffer sourcePixels = nullptr)
{
    const auto size = surface.target.size;
    auto reads = std::make_shared<Readback>();
    auto texture = reads->allocate(device, quint64(size.width()) * size.height() * 4);
    WGPUTexelCopyTextureInfo source{};
    source.texture = surface.texture.get();
    source.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo destination{};
    destination.buffer = texture->buffer.get();
    destination.layout.bytesPerRow = size.width() * 4;
    destination.layout.rowsPerImage = size.height();
    WGPUExtent3D extent{quint32(size.width()), quint32(size.height()), 1};
    wgpuCommandEncoderCopyTextureToBuffer(commands, &source, &destination, &extent);
    if (sourcePixels) reads->copy(commands, sourcePixels, TileRenderer::TileBytes, device);
    return reads;
}

void dot(OperationDocument &document, QPointF point, std::array<quint8, 4> color)
{
    Brush brush;
    brush.rgba = color;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{point, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
}
}

void KisGpuTilePresentationTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));

}

void KisGpuTilePresentationTest::panZoomAndTransparencyUseThePaintingQueue()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 4;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{8, 12}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    brush.rgba = {0, 0, 255, 128};
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{20, 16}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    Surface surface(m_device, {128, 128});
    QVERIFY(surface.ready);
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target,
        QTransform::fromTranslate(8, 16).scale(2, 2), QColor(0, 0, 0, 0));
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    auto readback = readTexture(*m_device, commands, surface);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    const auto pixel = [&](int x, int y) { return readback->results.front()->data.mid((y * 128 + x) * 4, 4); };
    QCOMPARE(pixel(24, 40), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(48, 48), QByteArray::fromHex("00008080"));
    QCOMPARE(pixel(8, 12), QByteArray(4, '\0'));
    QCOMPARE(drawn.tileCount, 1);

    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto background = presentation.record(commands, projection.version, surface.target,
        QTransform::fromTranslate(8, 16).scale(2, 2), QColor(0, 255, 0, 128));
    QVERIFY2(background.recorded, qPrintable(background.diagnostic));
    readback = readTexture(*m_device, commands, surface);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QCOMPARE(pixel(48, 48), QByteArray::fromHex("004080c0"));
    QCOMPARE(pixel(8, 12), QByteArray::fromHex("00800080"));
}


void KisGpuTilePresentationTest::invalidCropPreviewsPreserveTheTarget_data()
{
    QTest::addColumn<int>("invalid");
    for (int i = 0; i < 6; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
}
void KisGpuTilePresentationTest::invalidCropPreviewsPreserveTheTarget()
{
    QFETCH(int, invalid);
    CropPreview crop;
    crop.image = QRectF(0, 0, 16, 16);
    crop.crop = QRectF(4, 4, 8, 8);
    switch (invalid) {
    case 0: crop.crop.moveLeft(std::numeric_limits<qreal>::quiet_NaN()); break;
    case 1: crop.shade = QColor(); break;
    case 2: crop.thickness = 0; break;
    case 3: crop.toTarget.scale(0, 1); break;
    case 4: for (int i = 0; i < 9; ++i) crop.guides.append(QLineF(0, i, 8, i)); break;
    case 5: crop.handles.cubicTo(1, 2, 3, 4, 5, 6); break;
    }
    OperationDocument document({16, 16});
    Surface surface(m_device, {64, 16});
    QVERIFY(surface.ready);
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 16, 16});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    clear(commands, surface.target, QColor(7, 13, 29));
    const auto outcome = presentation.record(commands, projection.version, surface.target, {}, Qt::white, {}, {}, {}, crop);
    QVERIFY(!outcome.recorded);
    QVERIFY(!outcome.diagnostic.isEmpty());
    const auto readback = readTexture(*m_device, commands, surface);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QCOMPARE(readback->results.front()->data, QByteArray::fromHex("070d1dff").repeated(64 * 16));
}

void KisGpuTilePresentationTest::cropPreviewRetainsAntialiasedGeometryAndDocument_data()
{
    QTest::addColumn<int>("scenario");
    QTest::addColumn<quint32>("format");
    for (auto format : {WGPUTextureFormat_RGBA8Unorm, WGPUTextureFormat_BGRA8Unorm})
        for (int scenario = 0; scenario < 18; ++scenario) {
            const auto name = QStringLiteral("%1-case-%2").arg(format == WGPUTextureFormat_RGBA8Unorm ? "rgba8" : "bgra8").arg(scenario);
            QTest::newRow(qPrintable(name)) << scenario << quint32(format);
        }
}

void KisGpuTilePresentationTest::cropPreviewRetainsAntialiasedGeometryAndDocument()
{
    QFETCH(int, scenario);
    QFETCH(quint32, format);
    const int ratio = scenario == 1 ? 2 : 1;
    const QSize size = scenario == 12 ? QSize(1600, 960) : QSize(256 * ratio, 256 * ratio);
    OperationDocument document({128, 128});
    const auto before = document.snapshot();
    const auto recovery = document.recoveryData();
    CropPreview preview;
    preview.image = QRectF(0, 0, 128, 128);
    preview.crop = (scenario == 4 || scenario == 8) ? QRectF(60, 60, 3, 5) : QRectF(32, 32, 64, 64);
    preview.toTarget = QTransform::fromTranslate(32.25, 32.5);
    if (scenario == 1) preview.toTarget *= QTransform::fromScale(ratio, ratio);
    if (scenario == 2 || scenario == 7) preview.toTarget = QTransform::fromTranslate(128, 40).rotate(23);
    if (scenario == 3) preview.toTarget = QTransform::fromTranslate(200, 32).scale(-1, 1);
    if (scenario == 5) {
        preview.image = QRectF(0, 0, RgbaTile::CoordinateLimit, 128);
        preview.crop = QRectF(RgbaTile::CoordinateLimit - 64, 32, 64, 64);
        preview.toTarget = QTransform::fromTranslate(-RgbaTile::CoordinateLimit + 96, 32);
    }
    if (scenario >= 6 && scenario <= 8) preview.thickness = 3;
    if (scenario == 10) preview.crop = QRectF(60, 32, 0, 64);
    if (scenario == 11) preview.crop = QRectF(60, 32, -5, 64);
    if (scenario == 12) {
        // Half zoom, DPR2 and a 23-degree canvas rotation from an ordinary
        // crop drag: adjacent cosmetic handle edges meet inside one pixel.
        preview.image = QRectF(0, 0, 32, 32);
        preview.crop = QRectF(9.5, 9, 18, 18);
        preview.thickness = 2;
        preview.toTarget = QTransform(1.8410097069048807, 0.7814622569785475,
                                     -0.7814622569785475, 1.8410097069048807,
                                     760.6500511845652, 429.9078549457024);
    }
    if (scenario == 13) {
        // A small crop during a rotated DPR2 drag has overlapping wide handles.
        preview.image = QRectF(0, 0, 32, 32);
        preview.crop = QRectF(15, 15, 3, 5);
        preview.thickness = 2;
        preview.toTarget = QTransform::fromTranslate(128, 128).rotate(23).translate(-16, -16);
    }
    if (scenario == 14) {
        // A zero-width drag on a rotated DPR3 canvas retains the handle outline.
        preview.image = QRectF(0, 0, 32, 32);
        preview.crop = QRectF(16, 12, 0, 10);
        preview.thickness = 3;
        preview.toTarget = QTransform::fromTranslate(128, 128).rotate(-23).scale(3, 3).translate(-16, -16);
    }
    if (scenario >= 15 && scenario <= 17) {
        // A reversed drag can leave wide guide endpoints outside the viewport.
        preview.image = QRectF(0, 0, 32, 32);
        preview.crop = QRectF(16, 12, -5, 10);
        preview.thickness = scenario == 17 ? 3 : 2;
        preview.toTarget = QTransform::fromTranslate(4, 5).rotate(scenario == 16 ? -23 : 23)
            .scale(scenario == 16 ? -3 : 3, 3).translate(-16, -16);
    }
    const auto rect = preview.crop;
    for (const auto center : {rect.topLeft(), rect.topRight(), rect.bottomLeft(), rect.bottomRight(),
                              QPointF(rect.center().x(), rect.top()), QPointF(rect.center().x(), rect.bottom()),
                              QPointF(rect.left(), rect.center().y()), QPointF(rect.right(), rect.center().y())})
        preview.handles.addRect(QRectF(center - QPointF(6.5, 6.5), QSizeF(13, 13)));
    for (const qreal fraction : {0.2, 0.4, 0.6, 0.8}) {
        preview.guides.append(QLineF(rect.left(), rect.top() + fraction * rect.height(), rect.right(), rect.top() + fraction * rect.height()));
        preview.guides.append(QLineF(rect.left() + fraction * rect.width(), rect.top(), rect.left() + fraction * rect.width(), rect.bottom()));
    }
    if (scenario == 12) {
        preview.guides = {QLineF(rect.left(), rect.center().y(), rect.right(), rect.center().y()),
                          QLineF(rect.center().x(), rect.top(), rect.center().x(), rect.bottom())};
    }
    const QColor background = scenario == 12 ? QColor(Qt::white) : QColor(80, 100, 120, scenario == 9 ? 128 : 255);
    QImage reference(size, QImage::Format_ARGB32_Premultiplied);
    reference.fill(background);
    QPainter painter(&reference);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(preview.toTarget);
    QPainterPath shade;
    shade.addRect(preview.image);
    shade.addRect(preview.crop);
    painter.fillPath(shade, preview.shade);
    QPen pen(preview.stroke, preview.thickness, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(preview.handleFill);
    painter.drawPath(preview.handles);
    painter.setClipRect(preview.crop, Qt::IntersectClip);
    for (const auto &line : preview.guides) painter.drawLine(line);
    painter.end();
    Surface surface(m_device, size, format);
    QVERIFY(surface.ready);
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto finishCommands = qScopeGuard([&] {
        if (m_device->ownsCommands(commands)) m_device->submitCommands(true);
    });
    const auto layers = renderer.record(commands, before);
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 128, 128});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target, {}, background, {}, {}, {}, preview);
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    const auto readback = readTexture(*m_device, commands, surface);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    auto actual = readback->results.front()->data;
    if (format == WGPUTextureFormat_BGRA8Unorm)
        for (qsizetype i = 0; i < actual.size(); i += 4) std::swap(actual[i], actual[i + 2]);
    const auto expectedImage = reference.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const QByteArray expected(reinterpret_cast<const char*>(expectedImage.constBits()), expectedImage.sizeInBytes());
    const QString directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {qMakePair(QStringLiteral("-actual.rgba"), actual), qMakePair(QStringLiteral("-expected.rgba"), expected)}) {
            QFile file(QDir(directory).filePath(QStringLiteral("crop-") + QString::fromLatin1(QTest::currentDataTag()) + entry.first));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), entry.second.size());
        }
    }
    QCOMPARE(actual.size(), expected.size());
    qsizetype differences = 0;
    int maximumDifference = 0;
    for (qsizetype i = 0; i < actual.size(); ++i) {
        const int difference = qAbs(int(quint8(actual[i])) - int(quint8(expected[i])));
        differences += difference != 0;
        maximumDifference = qMax(maximumDifference, difference);
    }
    qInfo() << "Crop display byte differences" << differences << "maximum" << maximumDifference;
    // Coverage rounding is confined to this display-only target (#91).
    QVERIFY2(maximumDifference <= 2, qPrintable(QStringLiteral("Crop AA maximum byte difference: %1").arg(maximumDifference)));
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
}

void KisGpuTilePresentationTest::tileOverBackgroundMatchesEightBitSourceOver_data()
{
    QTest::addColumn<QColor>("backgroundColor");
    QTest::addColumn<quint32>("format");
    const QList<QPair<QString, QColor>> backgrounds{
        {QStringLiteral("opaque"), QColor(64, 96, 128)},
        {QStringLiteral("translucent"), QColor(64, 96, 128, 128)},
        {QStringLiteral("transparent"), QColor(64, 96, 128, 0)},
        {QStringLiteral("fractional"), QColor::fromRgba64(13000, 25000, 48000, 32800)}};
    for (auto format : {WGPUTextureFormat_RGBA8Unorm, WGPUTextureFormat_BGRA8Unorm})
        for (const auto &background : backgrounds) {
            const auto name = (format == WGPUTextureFormat_RGBA8Unorm ? QStringLiteral("rgba8-") : QStringLiteral("bgra8-")) + background.first;
            QTest::newRow(qPrintable(name)) << background.second << quint32(format);
        }
}

void KisGpuTilePresentationTest::tileOverBackgroundMatchesEightBitSourceOver()
{
    QFETCH(QColor, backgroundColor);
    QFETCH(quint32, format);
    QByteArray raw(RgbaTile::ByteCount, char(0));
    for (int alpha = 1; alpha < 256; ++alpha) for (int color = 0; color < 256; ++color) {
        const int i = (alpha * 256 + color) * 4;
        raw[i] = char(color);
        raw[i + 1] = char((color * 149 + 37) & 255);
        raw[i + 2] = char((color * 73 + 113) & 255);
        raw[i + 3] = char(alpha);
    }
    raw.replace(0, 4, QByteArray::fromHex("c0114080"));
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Matte source"), true,
                     QUuid(QStringLiteral("d4a7d91c-9a48-4c74-a269-7df471371520")), {{{0, 0}, raw}}}};
    OperationDocument document(input);
    const auto before = document.snapshot();
    const auto recovery = document.recoveryData();
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {512, 512}, format);
    QVERIFY(surface.ready);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, before);
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 256, 256});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target, {}, backgroundColor);
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    auto readback = readTexture(*m_device, commands, surface, layers.version->tiles(document.activeLayer()).first().pixels);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    const auto actual = readback->results.front()->data;
    const auto sourceBytes = raw;
    const QImage straight(reinterpret_cast<const uchar *>(sourceBytes.constData()), 256, 256, 256 * 4, QImage::Format_RGBA8888);
    const auto source = straight.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    QImage reference(512, 512, QImage::Format_RGBA8888_Premultiplied);
    reference.fill(backgroundColor);
    QPainter painter(&reference);
    painter.drawImage(0, 0, source);
    painter.end();
    QByteArray expected(reinterpret_cast<const char *>(reference.constBits()), reference.sizeInBytes());
    if (format == WGPUTextureFormat_BGRA8Unorm)
        for (qsizetype i = 0; i < expected.size(); i += 4) std::swap(expected[i], expected[i + 2]);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QVERIFY(QDir().mkpath(directory));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            qInfo() << "Matte actual/expected artifacts at" << directory;
        }
    });
    const auto stem = QDir(directory).filePath(QStringLiteral("matte-palette-") + QString::fromLatin1(QTest::currentDataTag()));
    for (const auto &entry : {qMakePair(QStringLiteral("-actual.rgba"), actual), qMakePair(QStringLiteral("-expected.rgba"), expected)}) {
        QFile file(stem + entry.first);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(entry.second), entry.second.size());
    }
    QCOMPARE(actual, expected);
    QCOMPARE(readback->results.back()->data, raw);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
}

void KisGpuTilePresentationTest::checkerBackgroundPreservesDocumentPixels_data()
{
    QTest::addColumn<quint32>("format");
    QTest::addColumn<int>("scenario");
    const QStringList names{QStringLiteral("empty-fixed"), QStringLiteral("empty-scrolling-clipped"),
                            QStringLiteral("painted-scaled"), QStringLiteral("painted-quarter-rotated")};
    for (auto format : {WGPUTextureFormat_RGBA8Unorm, WGPUTextureFormat_BGRA8Unorm})
        for (int scenario = 0; scenario < names.size(); ++scenario) {
            const auto name = (format == WGPUTextureFormat_RGBA8Unorm ? QStringLiteral("rgba8-") : QStringLiteral("bgra8-")) + names[scenario];
            QTest::newRow(qPrintable(name)) << quint32(format) << scenario;
        }
}

void KisGpuTilePresentationTest::checkerBackgroundPreservesDocumentPixels()
{
    QFETCH(quint32, format);
    QFETCH(int, scenario);
    const bool painted = scenario >= 2;
    const auto raw = QByteArray::fromHex("c0114080").repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Checker source"), true,
                     QUuid(QStringLiteral("8dedfa5c-5e78-48f2-894b-6256a7f62ec9")), {}}};
    if (painted) input.layers.first().tiles = {{{0, 0}, raw}};
    OperationDocument document(input);
    const auto before = document.snapshot();
    const auto recovery = document.recoveryData();
    const QColor border(9, 33, 57, scenario >= 2 ? 128 : 255);
    CheckerPattern pattern;
    pattern.first = QColor(24, 68, 116, 128);
    pattern.second = QColor(173, 209, 67, scenario == 0 ? 255 : 128);
    pattern.cellSize = 8;
    QTransform transform = QTransform::fromTranslate(16, 24);
    if (scenario == 1) {
        transform = QTransform::fromTranslate(-16, 8);
        pattern.targetToPattern = QTransform::fromTranslate(-11, -7);
    } else if (scenario == 2) {
        transform = QTransform::fromTranslate(16, 16).scale(1.5, 1.5);
        pattern.targetToPattern = QTransform::fromScale(0.5, 0.5);
    } else if (scenario == 3) {
        transform = QTransform::fromTranslate(104, 8).rotate(90).scale(1.5, 1.5);
        pattern.targetToPattern = transform.inverted();
    }
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {128, 128}, format);
    QVERIFY(surface.ready);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, before);
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target, transform, border, {}, {}, pattern);
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    const auto sourcePixels = painted ? layers.version->tiles(document.activeLayer()).first().pixels : nullptr;
    const auto reads = readTexture(*m_device, commands, surface, sourcePixels);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(reads->finish(*m_device));
    const auto actual = reads->results.front()->data;
    QImage swatches(2, 1, QImage::Format_ARGB32_Premultiplied);
    swatches.fill(border);
    QPainter painter(&swatches);
    painter.fillRect(swatches.rect(), pattern.second);
    painter.fillRect(0, 0, 1, 1, pattern.first);
    if (painted) painter.fillRect(swatches.rect(), QColor(192, 17, 64, 128));
    painter.end();
    QImage reference(128, 128, QImage::Format_ARGB32_Premultiplied);
    reference.fill(border);
    const QRgb colors[] = {swatches.pixel(0, 0), swatches.pixel(1, 0)};
    const auto toDocument = transform.inverted();
    for (int y = 0; y < reference.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(reference.scanLine(y));
        for (int x = 0; x < reference.width(); ++x) {
            const QPointF center(x + 0.5, y + 0.5);
            const auto point = toDocument.map(center);
            if (point.x() >= 0 && point.y() >= 0 && point.x() < 64 && point.y() < 64) {
                const auto patternPoint = pattern.targetToPattern.map(center);
                const auto column = qint64(std::floor(patternPoint.x() / pattern.cellSize));
                const auto patternRow = qint64(std::floor(patternPoint.y() / pattern.cellSize));
                row[x] = colors[(column ^ patternRow) & 1];
            }
        }
    }
    reference = reference.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    QByteArray expected(reinterpret_cast<const char *>(reference.constBits()), reference.sizeInBytes());
    if (format == WGPUTextureFormat_BGRA8Unorm)
        for (qsizetype i = 0; i < expected.size(); i += 4) std::swap(expected[i], expected[i + 2]);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QVERIFY(QDir().mkpath(directory));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            qInfo() << "Checker actual/expected artifacts at" << directory;
        }
    });
    const auto stem = QDir(directory).filePath(QStringLiteral("checker-background-") + QString::fromLatin1(QTest::currentDataTag()));
    for (const auto &entry : {qMakePair(QStringLiteral("-actual.rgba"), actual), qMakePair(QStringLiteral("-expected.rgba"), expected)}) {
        QFile file(stem + entry.first);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(entry.second), entry.second.size());
    }
    QCOMPARE(actual, expected);
    if (painted) QCOMPARE(reads->results.back()->data, raw);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
}

void KisGpuTilePresentationTest::invalidCheckerPatternsPreserveTheTarget_data()
{
    QTest::addColumn<int>("invalid");
    QTest::newRow("invalid-first-color") << 0;
    QTest::newRow("invalid-second-color") << 1;
    QTest::newRow("zero-cell-size") << 2;
    QTest::newRow("singular-transform") << 3;
    QTest::newRow("nonfinite-transform") << 4;
    QTest::newRow("coordinate-limit") << 5;
}

void KisGpuTilePresentationTest::invalidCheckerPatternsPreserveTheTarget()
{
    QFETCH(int, invalid);
    OperationDocument document({64, 64});
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {128, 128});
    QVERIFY(surface.ready);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    clear(commands, surface.target, QColor(100, 20, 30));
    CheckerPattern pattern;
    if (invalid == 0) pattern.first = QColor();
    if (invalid == 1) pattern.second = QColor();
    if (invalid == 2) pattern.cellSize = 0;
    if (invalid == 3) pattern.targetToPattern.scale(0, 1);
    if (invalid == 4) pattern.targetToPattern.translate(std::numeric_limits<qreal>::quiet_NaN(), 0);
    if (invalid == 5) pattern.targetToPattern.translate(qreal(8388609) * pattern.cellSize, 0);
    const auto outcome = presentation.record(commands, projection.version, surface.target, {}, Qt::white, {}, {}, pattern);
    const auto reads = readTexture(*m_device, commands, surface);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(reads->finish(*m_device));
    QVERIFY(!outcome.recorded);
    QVERIFY(!outcome.diagnostic.isEmpty());
    QCOMPARE(reads->results.front()->data, QByteArray::fromHex("64141eff").repeated(128 * 128));
}

void KisGpuTilePresentationTest::toolOutlinesMoveAndClearWithoutChangingDocumentPixels_data()
{
    QTest::addColumn<QColor>("backgroundColor");
    QTest::addColumn<QTransform>("imageTransform");
    QTest::newRow("opaque") << QColor(64, 96, 128) << QTransform();
    QTest::newRow("translucent") << QColor(64, 96, 128, 128) << QTransform();
    QTest::newRow("transparent-transformed") << QColor(64, 96, 128, 0)
        << QTransform::fromTranslate(4, 8).rotate(30).scale(0.75, 0.5);
}

void KisGpuTilePresentationTest::toolOutlinesMoveAndClearWithoutChangingDocumentPixels()
{
    QFETCH(QColor, backgroundColor);
    QFETCH(QTransform, imageTransform);
    OperationDocument document({64, 64});
    dot(document, {16.5, 24.5}, {192, 17, 64, 255});
    const auto version = document.snapshot();
    const auto history = document.undoCount();
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {128, 128});
    QVERIFY(surface.ready);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, version);
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QVERIFY(!projection.version->tiles().isEmpty());
    const auto sourcePixels = projection.version->tiles().first().pixels;
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QVERIFY(QDir().mkpath(directory));
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            qInfo() << "Tool outline actual/expected artifacts at" << directory;
        }
    });
    QByteArray background, initialPixels;
    const QColor cursor(255, 192, 96);
    for (int stage = 0; stage < 4; ++stage) {
        if (stage) commands = m_device->beginCommands();
        QVERIFY(commands);
        ToolOutline outline;
        QRect outside, inside;
        if (stage == 1 || stage == 2) {
            outline.contours = {QPolygonF{{16.5, 16.5}, {47.5, 16.5}, {47.5, 47.5}, {16.5, 47.5}, {16.5, 16.5}}};
            outline.color = cursor;
            outline.thickness = stage == 1 ? 1 : 3;
            if (stage == 1) {
                outside = {16, 16, 32, 32};
                inside = {17, 17, 30, 30};
            } else {
                for (auto &point : outline.contours.first()) point += QPointF(16, 0);
                outside = {31, 15, 34, 34};
                inside = {34, 18, 28, 28};
            }
        }
        const auto drawn = presentation.record(commands, projection.version, surface.target,
            imageTransform, backgroundColor, {}, outline);
        QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
        auto reads = readTexture(*m_device, commands, surface, sourcePixels);
        QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
        QVERIFY(reads->finish(*m_device));
        const auto actual = reads->results.front()->data;
        if (stage == 0) {
            background = actual;
            initialPixels = reads->results.back()->data;
        }
        auto expected = background;
        // The existing primary canvas uses cursorRGB * (1 - displayedRGB)
        // on opaque pixels. The display keeps the destination alpha.
        const int components[] = {cursor.red(), cursor.green(), cursor.blue()};
        for (int y = outside.top(); !outside.isEmpty() && y <= outside.bottom(); ++y) {
            for (int x = outside.left(); x <= outside.right(); ++x) {
                if (inside.contains(x, y)) continue;
                const auto offset = (y * 128 + x) * 4;
                const int alpha = quint8(background[offset + 3]);
                for (int channel = 0; channel < 3; ++channel)
                    expected[offset + channel] = char((components[channel] * (alpha - quint8(background[offset + channel])) + 127) / 255);
            }
        }
        for (const auto &artifact : {qMakePair(QStringLiteral("actual"), actual), qMakePair(QStringLiteral("expected"), expected)}) {
            QFile file(QDir(directory).filePath(QStringLiteral("tool-outline-%1-stage-%2-%3.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag())).arg(stage).arg(artifact.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(artifact.second), artifact.second.size());
        }
        QCOMPARE(actual, expected);
        QCOMPARE(reads->results.back()->data, initialPixels);
        QCOMPARE(document.snapshot(), version);
        QCOMPARE(document.undoCount(), history);
    }
}

void KisGpuTilePresentationTest::invalidToolOutlinesPreserveTheTarget_data()
{
    QTest::addColumn<int>("invalid");
    QTest::newRow("nonfinite-point") << 0;
    QTest::newRow("negative-width") << 1;
    QTest::newRow("invalid-color") << 2;
    QTest::newRow("vertex-budget") << 3;
}

void KisGpuTilePresentationTest::invalidToolOutlinesPreserveTheTarget()
{
    QFETCH(int, invalid);
    OperationDocument document({64, 64});
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {128, 128});
    QVERIFY(surface.ready);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    clear(commands, surface.target, QColor(100, 20, 30));
    ToolOutline outline;
    outline.contours = {QPolygonF{{16, 16}, {32, 32}}};
    if (invalid == 0) outline.contours.first()[0].setX(std::numeric_limits<qreal>::quiet_NaN());
    if (invalid == 1) outline.thickness = -1;
    if (invalid == 2) outline.color = QColor();
    if (invalid == 3) {
        auto &contour = outline.contours.first();
        for (int i = 0; i < 12000; ++i) contour.append(QPointF(i, i % 2));
    }
    const auto outcome = presentation.record(commands, projection.version, surface.target, QTransform(), Qt::white, {}, outline);
    auto reads = readTexture(*m_device, commands, surface);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(reads->finish(*m_device));
    QVERIFY(!outcome.recorded);
    QVERIFY(!outcome.diagnostic.isEmpty());
    QCOMPARE(reads->results.front()->data, QByteArray::fromHex("64141eff").repeated(128 * 128));
}

void KisGpuTilePresentationTest::clippedTilesKeepTheirIndependentTransforms()
{
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    dot(document, {8, 30}, {0, 0, 255, 255});
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {512, 64});
    QVERIFY(surface.ready);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {4, 4, 272, 16});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target, {}, Qt::transparent);
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    QCOMPARE(drawn.tileCount, 2);
    auto readback = readTexture(*m_device, commands, surface, projection.version->tiles().first().pixels);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QCOMPARE(readback->results[1]->data.mid((12 * 256 + 8) * 4, 4), QByteArray::fromHex("ff0000ff"));
    auto pixel = [&](int x, int y) { return readback->results.front()->data.mid((y * 512 + x) * 4, 4); };
    QCOMPARE(pixel(8, 12), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(264, 12), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixel(8, 30), QByteArray(4, '\0'));

    Surface rotated(m_device, {64, 64});
    QVERIFY(rotated.ready);
    TilePresentation oneVisibleTile(m_device, 1);
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto rotatedDraw = oneVisibleTile.record(commands, projection.version, rotated.target,
        QTransform(0, 1, -1, 0, 64, 0), Qt::transparent);
    QVERIFY2(rotatedDraw.recorded, qPrintable(rotatedDraw.diagnostic));
    QCOMPARE(rotatedDraw.tileCount, 1);
    readback = readTexture(*m_device, commands, rotated);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QCOMPARE(readback->results.front()->data.mid((8 * 64 + 51) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readback->results.front()->data.mid((8 * 64 + 33) * 4, 4), QByteArray(4, '\0'));
}

void KisGpuTilePresentationTest::displayChannelsPreserveDocumentPixels_data()
{
    QTest::addColumn<int>("mask");
    QTest::addColumn<bool>("asColor");
    QTest::addColumn<QByteArray>("visualBgra");
    // Produced by rgb8()->convertChannelToVisualRepresentation, following
    // KisImagePyramid::retrieveImageData at 066f2dc2b1. RGBA8/sRGB input is
    // (73,149,231,128); the two rows fix showSingleChannelAsColor=false/true.
    const char *expected[2][16] = {
        {"00000000", "49494980", "95959580", "00954900", "e7e7e780", "e7004900", "e7950000", "e7954900",
         "00000080", "49494980", "95959580", "00954980", "e7e7e780", "e7004980", "e7950080", "e7954980"},
        {"00000000", "00004900", "00950000", "00954900", "e7000000", "e7004900", "e7950000", "e7954900",
         "00000080", "00004980", "00950080", "00954980", "e7000080", "e7004980", "e7950080", "e7954980"}
    };
    for (int asColor = 0; asColor < 2; ++asColor) for (int mask = 0; mask < 16; ++mask) {
        const QByteArray tag = QByteArray(asColor ? "color-" : "gray-") + QByteArray::number(mask, 16);
        QTest::newRow(tag.constData()) << mask << bool(asColor) << QByteArray::fromHex(expected[asColor][mask]);
    }
}

void KisGpuTilePresentationTest::displayChannelsPreserveDocumentPixels()
{
    QFETCH(int, mask);
    QFETCH(bool, asColor);
    QFETCH(QByteArray, visualBgra);
    const auto raw = QByteArray::fromHex("4995e780").repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Display channels"), true, QUuid::createUuid(), {{{0, 0}, raw}}}};
    OperationDocument document(input);
    const auto before = document.snapshot();
    const auto recovery = document.recoveryData();
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device);
    Surface surface(m_device, {64, 64});
    QVERIFY(surface.ready);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, before);
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto drawn = presentation.record(commands, projection.version, surface.target, {}, Qt::transparent,
                                           DisplayChannels{quint8(mask), asColor});
    QVERIFY2(drawn.recorded, qPrintable(drawn.diagnostic));
    auto readback = readTexture(*m_device, commands, surface, layers.version->tiles(document.activeLayer()).first().pixels);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QByteArray rgba = visualBgra;
    std::swap(rgba[0], rgba[2]);
    const QImage straight(reinterpret_cast<const uchar *>(rgba.constData()), 1, 1, QImage::Format_RGBA8888);
    const auto premultiplied = straight.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const QByteArray expected(reinterpret_cast<const char *>(premultiplied.constBits()), 4);
    QCOMPARE(readback->results.front()->data, expected.repeated(64 * 64));
    QCOMPARE(readback->results[1]->data, raw);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
}

void KisGpuTilePresentationTest::rejectedPresentationPreservesTheTarget()
{
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    TileRenderer renderer(m_device);
    TilePresentation presentation(m_device, 1);
    Surface surface(m_device, {512, 64});
    QVERIFY(surface.ready);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 512, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    clear(commands, surface.target, QColor(7, 13, 29));
    QVERIFY(!presentation.record(commands, projection.version, surface.target, {}, Qt::transparent).recorded);
    QVERIFY(!presentation.record(commands, projection.version, surface.target,
        QTransform::fromScale(0, 1), Qt::transparent).recorded);
    QVERIFY(!presentation.record(commands, projection.version, surface.target,
        QTransform::fromTranslate(std::numeric_limits<double>::quiet_NaN(), 0), Qt::transparent).recorded);
    TilePresentation unrestricted(m_device);
    QVERIFY(!unrestricted.record(commands, projection.version, surface.target, {}, Qt::transparent, {16, false}).recorded);
    const auto readback = readTexture(*m_device, commands, surface);
    QVERIFY(readback);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(readback->finish(*m_device));
    QCOMPARE(readback->results.front()->data, QByteArray::fromHex("070d1dff").repeated(512 * 64));
}

void KisGpuTilePresentationTest::projectionCannotCrossDevices()
{
    OperationDocument document({64, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    TileRenderer renderer(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QString diagnostic;
    const auto other = Device::createNative(diagnostic);
    QVERIFY2(other, qPrintable(diagnostic));
    TilePresentation presentation(other);
    Surface surface(other, {64, 64});
    QVERIFY(surface.ready);
    commands = other->beginCommands();
    QVERIFY(commands);
    clear(commands, surface.target, Qt::white);
    const auto drawn = presentation.record(commands, projection.version, surface.target, {}, Qt::transparent);
    QVERIFY(!drawn.recorded);
    QVERIFY(!drawn.diagnostic.isEmpty());
    const auto readback = readTexture(*other, commands, surface);
    QVERIFY(readback);
    QVERIFY2(other->submitCommands(true), qPrintable(other->diagnostic()));
    QVERIFY(readback->finish(*other));
    QCOMPARE(readback->results.front()->data, QByteArray::fromHex("ffffffff").repeated(64 * 64));
}

void KisGpuTilePresentationTest::displayResourcesRetainTheirNativeDevice()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> lifetime = device;
    auto presentation = std::make_unique<TilePresentation>(device);
    device.reset();
    QVERIFY(!lifetime.expired());
    presentation.reset();
    QVERIFY(lifetime.expired());
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTilePresentationTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTilePresentationTest.moc"
