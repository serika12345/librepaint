/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <simpletest.h>
#include <testui.h>
#include <KisGpuPixelTolerance.h>
#include <KisPaintingEngineTestScope.h>
#include <KisGpuPaintDeviceTransfer_p.h>
#include <KisGpuExportRasterRows.h>
#include <Document.h>
#include <Node.h>
#include <application/ui/orchestration/KisPart.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <QSignalSpy>
#include <limits>
#include <memory>
#include <stdexcept>

class TestGpuPublicPixelTransfer : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativePixelsKeepPublicAndFormatViews_data()
    {
        QTest::addColumn<bool>("boundary");
        QTest::newRow("u8") << false;
        QTest::newRow("u16-cross-tile-off-canvas-and-rejection") << true;
    }
    void nativePixelsKeepPublicAndFormatViews();
};

void TestGpuPublicPixelTransfer::nativePixelsKeepPublicAndFormatViews()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, boundary);
    const TestUtil::PaintingEngineScope gpu(true);
    const auto *space = boundary ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = boundary ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int pixelSize = space->pixelSize();
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = {270, 4};
    raster.format = format;
    raster.colorProfile = space->profile()->rawData();
    Gpu::RasterLayer layer;
    layer.name = QStringLiteral("Native API pixels");
    layer.defaultPixel = boundary ? Gpu::RgbaPixel{54321, 12345, 591, 23456} : Gpu::RgbaPixel{211, 73, 19, 173};
    raster.layers = {layer};
    auto owner = Gpu::PaintImageSP::create(execution, raster);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Public pixel transfer"), space);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    document->setCurrentImage(image);
    Document api(document.get(), false);
    std::unique_ptr<Node> node(Node::createNode(image, image->root()->firstChild()));
    const auto id = owner->operations().snapshot()->layers.first().id;
    QCOMPARE(Gpu::pixelAccessExecutionLocation(Gpu::PixelAccess::Format), Gpu::ExecutionLocation::Cpu);
    QCOMPARE(Gpu::pixelAccessExecutionLocation(Gpu::PixelAccess::PublicApi), Gpu::ExecutionLocation::Cpu);

    const KoColor color(QColor(37, 151, 223, 201), space);
    const QByteArray pixel(reinterpret_cast<const char *>(color.data()), pixelSize);
    const QRect area = boundary ? QRect(252, -1, 21, 3) : QRect(5, 1, 9, 2);
    const auto bytes = pixel.repeated(area.width() * area.height());
    const auto before = owner->operations().snapshot();
    QVERIFY(node->setPixelData(bytes, area.x(), area.y(), area.width(), area.height()));
    const auto fixed = owner->operations().snapshot();
    const auto expected = node->pixelData(area.x(), area.y(), area.width(), area.height());
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(expected, bytes, boundary ? 2 : 1);
    QVERIFY(fixed != before);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(node->pixelData(area.x(), area.y(), area.width(), area.height()), bytes, boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuReadNativePixels(owner, fixed, id, area, bytes.size(), KisGpuPixelReadSource::Layer), bytes, boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(node->projectionPixelData(area.x(), area.y(), area.width(), area.height()), bytes, boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(api.pixelData(area.x(), area.y(), area.width(), area.height()), bytes, boundary ? 2 : 1);
    if (boundary) {
        // Canvas exports clip implicit/off-canvas pixels; native API reads retain them.
        const auto clipped = kisGpuReadNativePixels(owner, fixed, 0, area, bytes.size(), KisGpuPixelReadSource::CanvasProjection);
        QCOMPARE_PIXELS16_WITHIN_STEP(clipped.left(area.width() * pixelSize), QByteArray(area.width() * pixelSize, '\0'));
    }

    const auto projection = api.pixelData(0, 0, raster.size.width(), raster.size.height());
    KisGpuExportRasterRows exported(image, 16 * 1024 * 1024);
    const auto rows = exported.rows();
    QCOMPARE(rows->colorSpace, space);
    QCOMPARE(rows->size, raster.size);
    const auto reference = owner->retainLayerPixels(id);
    const KoColor other(Qt::green, space);
    const QByteArray replacement(reinterpret_cast<const char *>(other.data()), pixelSize);
    QCOMPARE(kisGpuWriteNativePixels(owner, reference, {0, 0, 1, 1}, replacement), Gpu::Result::Success);
    QByteArray result;
    for (int y = 0; y < raster.size.height(); ++y) {
        QByteArray row;
        QVERIFY(rows->readRow(y, row).isOk());
        result.append(row);
    }
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(result, projection, boundary ? 2 : 1);
    QVERIFY(api.pixelData(0, 0, 1, 1) != projection.left(pixelSize));
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuReadNativePixels(owner, fixed, id, {0, 0, 1, 1}, pixelSize,
        KisGpuPixelReadSource::Layer), projection.left(pixelSize), boundary ? 2 : 1);
    QCOMPARE_PIXELS_WITHIN_DEPTH_STEP(kisGpuReadNativePixels(owner, fixed, id, area, bytes.size(), KisGpuPixelReadSource::Layer), expected, boundary ? 2 : 1);

    if (boundary) {
        QCOMPARE(owner->setLayerOpacity(id, 197), Gpu::Result::Success);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        const auto unchanged = owner->operations().snapshot();
        const auto requested = execution->requested(owner->handle());
        const auto referenceVersion = reference->version();
        QSignalSpy changed(owner.data(), &Gpu::PaintImage::changed);
        const int maximum = std::numeric_limits<int>::max();
        QVERIFY(node->pixelData(0, 0, maximum, maximum).isEmpty());
        QVERIFY(node->projectionPixelData(0, 0, -1, 1).isEmpty());
        QVERIFY(api.pixelData(maximum, 0, 2, 1).isEmpty());
        QVERIFY(!node->setPixelData(bytes.first(bytes.size() - 1), area.x(), area.y(), area.width(), area.height()));
        QCOMPARE(kisGpuWriteNativePixels(owner, reference, area, bytes.first(bytes.size() - 1)), Gpu::Result::CapacityExceeded);
        QVERIFY_EXCEPTION_THROWN(kisGpuReadNativePixels(owner, fixed, id, area, bytes.size() - 1, KisGpuPixelReadSource::Layer), std::length_error);
        QVERIFY_EXCEPTION_THROWN(kisGpuPixelTransfer(QByteArrayView(bytes).first(pixelSize - 1), format, false), std::invalid_argument);
        QCOMPARE(owner->operations().snapshot(), unchanged);
        QCOMPARE(reference->version(), referenceVersion);
        QCOMPARE(execution->requested(owner->handle()), requested);
        QCOMPARE(owner->operations().redoCount(), 1);
        QVERIFY(!owner->operations().isModified());
        QCOMPARE(changed.size(), 0);
    }
}

KISTEST_MAIN(TestGpuPublicPixelTransfer)
#include "TestGpuPublicPixelTransfer.moc"
