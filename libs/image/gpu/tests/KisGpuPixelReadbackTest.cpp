/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QColorSpace>
#include <rhi/qrhi.h>
#include <gpu/KisGpuExecution.h>

using namespace LibrePaint::Image::Gpu;

class KisGpuPixelReadbackTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void pinnedPixelsKeepNewerRequestsAndPublishedState();
    void hiddenLayersAndVisibleProjectionRemainDistinct();
    void boundedReadsRejectAtomicallyAndRecover();
    void closedDocumentsAndDeviceGenerationsGuardReads();
    void cpuPixelsOutliveExecutionAndDevices();
    void savingReadsOffCanvasTilesAndRetainsImplicitPixels();
    void selectedReadsFitWithoutGeneratingUnrequestedPixels();
    void completeImagesStreamThinCanvasesAndOutliveTheirDevice();
    void rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping_data();
    void rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping();
private:
    DevicePtr m_device;
};

void KisGpuPixelReadbackTest::rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping_data()
{
    QTest::addColumn<bool>("projection");
    QTest::newRow("hidden layer coordinates") << false;
    QTest::newRow("visible clipped projection") << true;
}

void KisGpuPixelReadbackTest::rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping()
{
    QFETCH(bool, projection);
    RasterImage image;
    image.size = {1, 513};
    image.layers = {{QStringLiteral("Visible"), true, {}, {}, {20, 80, 160, 128}},
                    {QStringLiteral("Hidden"), false, {},
                     {{{-1, -1}, QByteArray::fromHex("137fafe0").repeated(RgbaTile::ByteCount / 4)}}, {7, 11, 13, 0}}};
    OperationDocument document(image);
    const auto pinned = document.snapshot();
    Execution execution(m_device, 3 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    const auto handle = execution.open(pinned).handle;
    const LayerId layer = projection ? 0 : pinned->layers.last().id;
    const QRect region(-1, -1, 2, 515);
    const qsizetype bytes = region.width() * region.height() * 4;
    QCOMPARE(document.renameLayer(document.activeLayer(), QStringLiteral("Latest")), Result::Success);
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    const auto rejected = execution.readRgbaPixels(handle, pinned, region, layer, bytes - 1);
    QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
    QVERIFY(rejected.bytes.isEmpty());
    const auto read = execution.readRgbaPixels(handle, pinned, region, layer, bytes);
    QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
    QCOMPARE(read.document, pinned);
    QCOMPARE(read.generation, execution.generation());
    QCOMPARE(read.region, region);
    QByteArray expected = projection ? QByteArray(bytes, '\0') : QByteArray::fromHex("070b0d00").repeated(bytes / 4);
    if (projection) {
        for (int y = 0; y < 513; ++y) expected.replace(((y + 1) * 2 + 1) * 4, 4, QByteArray::fromHex("1450a080"));
    } else {
        expected.replace(0, 4, QByteArray::fromHex("137fafe0"));
    }
    QCOMPARE(read.bytes, expected);
    QCOMPARE(execution.requested(handle), latest);
    QVERIFY(!execution.published(handle));
    const auto empty = execution.readRgbaPixels(handle, pinned, {0, 0, 0, 0}, layer, 0);
    QCOMPARE(empty.code, ExecutionCode::Success);
    QVERIFY(empty.bytes.isEmpty());
    const auto invalid = execution.readRgbaPixels(handle, pinned, {0, 0, -1, 1}, layer, bytes);
    QCOMPARE(invalid.code, ExecutionCode::InvalidRequest);
    QVERIFY(invalid.bytes.isEmpty());
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.readRgbaPixels(handle, pinned, region, layer, bytes).code, ExecutionCode::InvalidRequest);
    QCOMPARE(read.bytes, expected);
}

void KisGpuPixelReadbackTest::completeImagesStreamThinCanvasesAndOutliveTheirDevice()
{
    ImageReadbackOutcome retained;
    std::weak_ptr<Device> deviceLifetime;
    {
        QString diagnostic;
        const auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        deviceLifetime = device;
        RasterImage image;
        image.size = {1, 513};
        image.layers = {{QStringLiteral("Implicit"), true, {}, {}, {20, 80, 160, 128}}};
        OperationDocument document(image);
        const auto pinned = document.snapshot();
        Execution execution(device, 2 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
        const auto handle = execution.open(pinned).handle;
        QVERIFY(handle);
        {
            QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
            const auto displayed = execution.project(handle, {0, 0, 1, 1});
            QCOMPARE(displayed.code, RenderOutcome::Recorded);
            QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        }
        QCOMPARE(document.renameLayer(document.activeLayer(), QStringLiteral("Latest")), Result::Success);
        const auto latest = document.snapshot();
        QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
        const auto rejected = execution.readProjectionImage(handle, pinned, 1 * 513 * 4 - 1);
        QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
        QVERIFY(rejected.image.isNull());
        retained = execution.readProjectionImage(handle, pinned, 1 * 513 * 4);
        QVERIFY2(retained.code == ExecutionCode::Success, qPrintable(retained.diagnostic));
        QCOMPARE(retained.document, pinned);
        QCOMPARE(retained.generation, execution.generation());
        QCOMPARE(retained.image.size(), image.size);
        QCOMPARE(retained.image.colorSpace(), QColorSpace(QColorSpace::SRgb));
        for (int y = 0; y < retained.image.height(); ++y)
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(retained.image.constScanLine(y)), 4), QByteArray::fromHex("1450a080"));
        QCOMPARE(execution.requested(handle), latest);
        QCOMPARE(execution.published(handle), pinned);
        const auto sparse = execution.readLayerPixels(handle, pinned, document.activeLayer(), 0);
        QCOMPARE(sparse.code, ExecutionCode::Success);
        QVERIFY(sparse.tiles.isEmpty());
        QVERIFY(sparse.document->layers.front().defaultPixel == image.layers.front().defaultPixel);
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
        const auto closed = execution.readProjectionImage(handle, pinned, 1 * 513 * 4);
        QCOMPARE(closed.code, ExecutionCode::InvalidRequest);
        QVERIFY(closed.image.isNull());
    }
    QVERIFY(deviceLifetime.expired());
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(retained.image.constScanLine(512)), 4), QByteArray::fromHex("1450a080"));
}

namespace {
void dot(OperationDocument &document, QPointF point, std::array<quint8, 4> rgba)
{
    Brush brush;
    brush.rgba = rgba;
    brush.diameter = 6;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{point, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
}

QByteArray pixel(const PixelReadbackOutcome &read, LayerId layer, QPoint point)
{
    for (const auto &tile : read.tiles) {
        const QPoint origin = tile.coordinate * TileRenderer::TileSize;
        if (tile.layer == layer && QRect(origin, QSize(TileRenderer::TileSize, TileRenderer::TileSize)).contains(point)) {
            const auto local = point - origin;
            return tile.rgba.mid((local.y() * TileRenderer::TileSize + local.x()) * 4, 4);
        }
    }
    return {};
}
}

void KisGpuPixelReadbackTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    qInfo() << "GPU backend:" << m_device->rhi().backendName();
}

void KisGpuPixelReadbackTest::selectedReadsFitWithoutGeneratingUnrequestedPixels()
{
    RasterImage image;
    image.size = {9 * RgbaTile::Size, 64};
    image.layers = {{QStringLiteral("Visible"), true, {}, {}}, {QStringLiteral("Hidden"), false, {}, {}}};
    for (int tile = -1; tile < 9; ++tile) {
        image.layers[0].tiles.append({{tile, 0}, QByteArray::fromHex("001234ff").repeated(RgbaTile::ByteCount / 4)});
        image.layers[1].tiles.append({{tile, 0}, QByteArray::fromHex("ab567880").repeated(RgbaTile::ByteCount / 4)});
    }
    OperationDocument document(image);
    Execution execution(m_device, 3 * TileRenderer::TileBytes, 1, 2 * TileRenderer::TileBytes);
    const auto handle = execution.open(document.snapshot()).handle;
    const auto visible = document.snapshot()->layers[0].id;
    const auto hidden = document.snapshot()->layers[1].id;
    for (const int tile : {0, 8, 0}) {
        const QPoint point(tile * 256 + 8, 12);
        const auto read = execution.readPixels(handle, document.snapshot(), QRect(point, QSize(1, 1)), {hidden});
        QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
        QCOMPARE(read.tiles.size(), 2);
        QCOMPARE(pixel(read, hidden, point), QByteArray::fromHex("ab567880"));
        QCOMPARE(pixel(read, 0, point), QByteArray::fromHex("001234ff"));
        QVERIFY(pixel(read, visible, point).isEmpty());
        QVERIFY(!execution.published(handle));
        QCOMPARE(execution.requested(handle), document.snapshot());
    }
    const auto raw = execution.readPixels(handle, document.snapshot(), {-1, 12, 1, 1}, {hidden}, false,
                                          PixelReadArea::LayerCoordinates);
    QCOMPARE(raw.code, ExecutionCode::Success);
    QCOMPARE(raw.tiles.size(), 1);
    QCOMPARE(raw.tiles[0].coordinate, QPoint(-1, 0));
    QCOMPARE(pixel(raw, hidden, {-1, 12}), QByteArray::fromHex("ab567880"));
}

void KisGpuPixelReadbackTest::pinnedPixelsKeepNewerRequestsAndPublishedState()
{
    Execution execution(m_device);
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.snapshot();
    const auto layer = document.activeLayer();
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 512, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    dot(document, {264, 12}, {0, 0, 255, 128});
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);

    const auto old = execution.readPixels(handle, pinned, {0, 0, 512, 64}, {layer});
    QCOMPARE(old.code, ExecutionCode::Success);
    QCOMPARE(old.document, pinned);
    QCOMPARE(old.generation, execution.generation());
    QCOMPARE(old.region, QRect(0, 0, 512, 64));
    QCOMPARE(old.tiles.size(), 2);
    QCOMPARE(pixel(old, layer, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(old, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.published(handle), pinned);
    QCOMPARE(execution.pendingDocuments(), 1);
    const auto current = execution.readPixels(handle, latest, {256, 0, 256, 64}, {layer});
    QCOMPARE(current.code, ExecutionCode::Success);
    QCOMPARE(current.tiles.size(), 2);
    QCOMPARE(pixel(current, layer, {264, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(pixel(current, 0, {264, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.published(handle), pinned);
    QCOMPARE(document.markSaved(pinned), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
}

void KisGpuPixelReadbackTest::hiddenLayersAndVisibleProjectionRemainDistinct()
{
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 16, 2 * TileRenderer::TileBytes);
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto second = document.addLayer(QStringLiteral("Hidden blue"));
    dot(document, {8, 12}, {0, 0, 255, 128});
    QCOMPARE(document.setLayerVisible(second, false), Result::Success);
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    const auto read = execution.readPixels(handle, pinned, {-10, -10, 100, 100}, {first, second});
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.region, QRect(0, 0, 64, 64));
    QCOMPARE(read.tiles.size(), 3);
    QCOMPARE(pixel(read, first, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(read, second, {8, 12}), QByteArray::fromHex("0000ff80"));
    QCOMPARE(pixel(read, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    const auto rawLayer = execution.readPixels(handle, pinned, {0, 0, 64, 64}, {second}, false);
    QCOMPARE(rawLayer.code, ExecutionCode::Success);
    QCOMPARE(rawLayer.tiles.size(), 1);
    QCOMPARE(pixel(rawLayer, second, {8, 12}), QByteArray::fromHex("0000ff80"));
    QVERIFY(pixel(rawLayer, 0, {8, 12}).isEmpty());
    QVERIFY(!execution.published(handle));
    QCOMPARE(execution.pendingDocuments(), 1);
    QCOMPARE(document.setLayerVisible(second, true), Result::Success);
    const auto visible = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64}, {first, second});
    QCOMPARE(visible.code, ExecutionCode::CapacityExceeded);
    QVERIFY(visible.tiles.isEmpty());
    const auto onlyProjection = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64});
    QCOMPARE(onlyProjection.code, ExecutionCode::Success);
    QCOMPARE(pixel(onlyProjection, 0, {8, 12}), QByteArray::fromHex("7f0080ff"));
}

void KisGpuPixelReadbackTest::boundedReadsRejectAtomicallyAndRecover()
{
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 16, TileRenderer::TileBytes);
    OperationDocument document({512, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    dot(document, {264, 12}, {0, 255, 0, 255});
    const auto pinned = document.snapshot();
    const auto layer = document.activeLayer();
    const auto handle = execution.open(pinned).handle;
    const auto full = execution.readPixels(handle, pinned, {0, 0, 512, 64}, {layer});
    QCOMPARE(full.code, ExecutionCode::CapacityExceeded);
    QVERIFY(full.tiles.isEmpty());
    QVERIFY(!full.diagnostic.isEmpty());
    QVERIFY(!m_device->rhi().isRecordingFrame());
    QCOMPARE(execution.requested(handle), pinned);
    QVERIFY(!execution.published(handle));
    const auto partial = execution.readPixels(handle, pinned, {256, 0, 256, 64}, {layer});
    QCOMPARE(partial.code, ExecutionCode::Success);
    QCOMPARE(partial.tiles.size(), 2);
    QCOMPARE(pixel(partial, layer, {264, 12}), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}, {layer, layer}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}, {999}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, -1, 64}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}).code, ExecutionCode::Busy);
    const auto busy = execution.readRgbaPixels(handle, pinned, {0, 0, 1, 1}, layer, 4);
    QCOMPARE(busy.code, ExecutionCode::Busy);
    QVERIFY(busy.bytes.isEmpty());
    QCOMPARE(execution.project(handle, {0, 0, 512, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.published(handle), pinned);
}

void KisGpuPixelReadbackTest::closedDocumentsAndDeviceGenerationsGuardReads()
{
    Execution execution(m_device);
    OperationDocument document({64, 64}), other({64, 64});
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(execution.readPixels(handle, other.snapshot(), {0, 0, 64, 64}).code, ExecutionCode::InvalidRequest);
    const auto empty = execution.readPixels(handle, pinned, {128, 128, 64, 64});
    QCOMPARE(empty.code, ExecutionCode::Success);
    QVERIFY(empty.region.isEmpty());
    QVERIFY(empty.tiles.isEmpty());
    const auto before = execution.readPixels(handle, pinned, {0, 0, 64, 64});
    QCOMPARE(before.code, ExecutionCode::Success);
    QString diagnostic;
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    const auto rebuilt = execution.readPixels(handle, pinned, {0, 0, 64, 64});
    QCOMPARE(rebuilt.code, ExecutionCode::Success);
    QVERIFY(rebuilt.generation > before.generation);
    QCOMPARE(pixel(rebuilt, 0, {8, 12}), pixel(before, 0, {8, 12}));
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.readPixels(handle, pinned, {0, 0, 64, 64}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(pixel(before, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
}

void KisGpuPixelReadbackTest::cpuPixelsOutliveExecutionAndDevices()
{
    PixelReadbackOutcome cpu;
    PixelBytesOutcome cpuBytes;
    std::weak_ptr<Device> owner;
    {
        QString diagnostic;
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        owner = device;
        Execution execution(device);
        OperationDocument document({64, 64});
        dot(document, {8, 12}, {255, 0, 0, 255});
        const auto handle = execution.open(document.snapshot()).handle;
        cpu = execution.readPixels(handle, document.snapshot(), {0, 0, 64, 64});
        QCOMPARE(cpu.code, ExecutionCode::Success);
        cpuBytes = execution.readRgbaPixels(handle, document.snapshot(), {8, 12, 1, 1}, 0, 4);
        QCOMPARE(cpuBytes.code, ExecutionCode::Success);
        device.reset();
        QVERIFY(!owner.expired());
    }
    QVERIFY(owner.expired());
    QVERIFY(cpu.document);
    QCOMPARE(pixel(cpu, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(cpuBytes.bytes, QByteArray::fromHex("ff0000ff"));
}

void KisGpuPixelReadbackTest::savingReadsOffCanvasTilesAndRetainsImplicitPixels()
{
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Imported"), true, {},
                     {{{-1, 0}, QByteArray::fromHex("11171f00").repeated(RgbaTile::ByteCount / 4)}}}};
    image.layers[0].defaultPixel = {255, 255, 255, 255};
    OperationDocument document(image);
    dot(document, {8, 12}, {255, 0, 0, 255});
    const auto pinned = document.snapshot();
    const auto layer = document.activeLayer();
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 16, 2 * TileRenderer::TileBytes);
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(document.replaceTiles(layer, {{{-1, 0}, QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4)}}), Result::Success);
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    const QRect area(-256, 0, 512, 256);
    const auto clipped = execution.readPixels(handle, pinned, area, {layer});
    QCOMPARE(clipped.code, ExecutionCode::Success);
    QCOMPARE(clipped.region, QRect(0, 0, 64, 64));
    QCOMPARE(clipped.tiles.size(), 2);
    const auto all = execution.readPixels(handle, pinned, area, {layer}, true, PixelReadArea::LayerCoordinates);
    QCOMPARE(all.code, ExecutionCode::Success);
    QCOMPARE(all.region, area);
    QCOMPARE(all.tiles.size(), 3);
    QCOMPARE(pixel(all, layer, {-10, 12}), QByteArray::fromHex("11171f00"));
    QCOMPARE(pixel(all, layer, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixel(all, layer, {1, 1}), QByteArray::fromHex("ffffffff"));
    QCOMPARE(pixel(all, 0, {8, 12}), QByteArray::fromHex("ff0000ff"));
    QVERIFY(pixel(all, 0, {-10, 12}).isEmpty());
    QCOMPARE(all.document->layers.first().defaultPixel, image.layers.first().defaultPixel);
    QCOMPARE(execution.requested(handle), latest);
    QVERIFY(!execution.published(handle));
    const auto outside = execution.readPixels(handle, pinned, {-256, 0, 256, 256}, {layer}, true, PixelReadArea::LayerCoordinates);
    QCOMPARE(outside.code, ExecutionCode::Success);
    QCOMPARE(outside.tiles.size(), 1);
    QCOMPARE(pixel(outside, layer, {-10, 12}), QByteArray::fromHex("11171f00"));
    const auto invalid = execution.readPixels(handle, pinned, area, {layer}, false, PixelReadArea(99));
    QCOMPARE(invalid.code, ExecutionCode::InvalidRequest);
    QVERIFY(invalid.tiles.isEmpty());
    QCOMPARE(document.markSaved(pinned), Result::Success);
    QVERIFY(document.isModified());
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuPixelReadbackTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPixelReadbackTest.moc"
