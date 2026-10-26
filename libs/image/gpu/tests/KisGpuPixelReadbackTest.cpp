/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QColorSpace>
#include <QDir>
#include <QFile>
#include <gpu/KisGpuExecution.h>
#include <chrono>
#include <future>

using namespace LibrePaint::Image::Gpu;

class KisGpuPixelReadbackTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void colorSamplesKeepCanonicalDiskAlphaAndTransferLimits_data();
    void colorSamplesKeepCanonicalDiskAlphaAndTransferLimits();
    void colorSamplesKeepPaintedVersionsAndPublication();
    void colorSamplesOutliveCloseAndDeviceReplacement();
    void rejectedColorSamplesLeaveTheNextReadAvailable();
    void passThroughReadsKeepSubtreeBoundaryAndRootCapacity_data();
    void passThroughReadsKeepSubtreeBoundaryAndRootCapacity();
    void groupReadsKeepSubtreePixelsDefaultsAndPinnedVersions_data();
    void groupReadsKeepSubtreePixelsDefaultsAndPinnedVersions();
    void projectionSkipsHiddenSubtreesWithinVisibleCapacity();
    void pinnedPixelsKeepNewerRequestsAndPublishedState();
    void hiddenLayersAndVisibleProjectionRemainDistinct();
    void boundedReadsRejectAtomicallyAndRecover();
    void closedDocumentsAndDeviceGenerationsGuardReads();
    void cpuPixelsOutliveExecutionAndDevices();
    void savingReadsOffCanvasTilesAndRetainsImplicitPixels();
    void selectedReadsFitWithoutGeneratingUnrequestedPixels();
    void sparseLayerReadsIncludeSolidFigureInteriors();
    void completeImagesStreamThinCanvasesAndOutliveTheirDevice();
    void rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping_data();
    void rectangularPixelsStreamDefaultsHiddenLayersAndCanvasClipping();
    void asynchronousPixelsKeepThePinnedVersionAndNewerRequests();
    void synchronousPreviewsCompleteEarlierAsynchronousReads_data();
    void synchronousPreviewsCompleteEarlierAsynchronousReads();
    void asynchronousResultsOutliveClosedDocumentsAndTheDevice();
    void discardedAsynchronousReadsReleaseTheirNativeStorage();
    void projectionTransparencyUsesPinnedPixelsAndCanvasClipping();
    void transparencyResultsOutliveClosedDocumentsAndTheirDevice();
    void boundsKeepFixedVersionsAndRejectWithoutChangingRequests();
    void boundsStreamMoreTilesThanTheGpuCanRetain();
    void boundsReusePixelsRetainedByTheDisplayedFrame();
    void integerTranslationPreservesRgbaAcrossTileBoundariesAndFixedVersions();
    void contentBoundsRetainTransparentRgbForMovementAdmission();
    void projectionReadsTranslatedCheckpointsWithinCanvasBudget();
    void thumbnailsStreamFixedHiddenPixelsAndPreserveRequests_data();
    void thumbnailsStreamFixedHiddenPixelsAndPreserveRequests();
    void coarseThumbnailsRetainProjectionBackgroundOutsideStorageEdges();
    void thumbnailWorkingStorageWaitsForPinnedGpuPixels();
    void resizedProjectionPreservesFixedPixelsRequestsAndReturnedImage();
    void resizedProjectionRejectsPinnedWorkingStorageWithoutChangingPublication();
    void constantProjectionPreviewUsesOnlyOnePixelTransfer();
private:
    DevicePtr m_device;
};

void KisGpuPixelReadbackTest::colorSamplesKeepCanonicalDiskAlphaAndTransferLimits_data()
{
    QTest::addColumn<int>("radius");
    QTest::addColumn<QPoint>("point");
    QTest::addColumn<bool>("projection");
    QTest::addColumn<int>("defaultAlpha");
    QTest::addColumn<QByteArray>("center");
    QTest::addColumn<bool>("halfTie");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("one-pixel") << 1 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("0000ffff");
    QTest::newRow("radius-two-center") << 2 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("0000ffff");
    QTest::newRow("nine-pixel-disk") << 3 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("e3001cff");
    QTest::newRow("twenty-five-pixel-disk") << 4 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("f5000aff");
    QTest::newRow("circle-not-square") << 5 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("f90006ff");
    QTest::newRow("projection-disk") << 3 << QPoint(12,9) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("e3001cff");
    QTest::newRow("layer-corner-defaults") << 3 << QPoint(0,0) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("e3001cff");
    QTest::newRow("projection-corner-padding") << 3 << QPoint(0,0) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("bf004071");
    QTest::newRow("projection-last-corner-padding") << 3 << QPoint(63,47) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("bf004071");
    QTest::newRow("negative-layer-tile") << 3 << QPoint(-1,-1) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("e3001cff");
    QTest::newRow("projection-outside-center") << 3 << QPoint(-1,-1) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("ff00001c");
    QTest::newRow("projection-positive-outside-center") << 3 << QPoint(64,48) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("ff00001c");
    QTest::newRow("fully-outside-projection") << 3 << QPoint(-500,-500) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray(4,'\0');
    QTest::newRow("alpha-weighted-color") << 3 << QPoint(12,9) << false << 255 << QByteArray::fromHex("0000ff80") << false << QByteArray::fromHex("f0000ff1");
    QTest::newRow("transparent-neighbors") << 3 << QPoint(12,9) << false << 0 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("0000ff1c");
    QTest::newRow("zero-alpha") << 3 << QPoint(12,9) << false << 0 << QByteArray::fromHex("12345600") << false << QByteArray(4,'\0');
    QTest::newRow("nearest-half-up") << 3 << QPoint(12,9) << false << 0 << QByteArray::fromHex("01000005") << true << QByteArray::fromHex("01000001");
    QTest::newRow("large-layer-sum") << 900 << QPoint(1024,1024) << false << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("ff0000ff");
    QTest::newRow("large-projection-sum") << 900 << QPoint(1024,1024) << true << 255 << QByteArray::fromHex("0000ffff") << false << QByteArray::fromHex("ff0000ff");
}

void KisGpuPixelReadbackTest::colorSamplesKeepCanonicalDiskAlphaAndTransferLimits()
{
    QFETCH(int,radius);
    QFETCH(QPoint,point);
    QFETCH(bool,projection);
    QFETCH(int,defaultAlpha);
    QFETCH(QByteArray,center);
    QFETCH(bool,halfTie);
    QFETCH(QByteArray,expected);
    RasterImage input;
    input.size = radius == 900 ? QSize(2048,2048) : QSize(64,48);
    input.layers = {{QStringLiteral("Color sample"),true,{}, {}, {255,0,0,quint8(defaultAlpha)}}};
    constexpr int side = TileRenderer::TileSize;
    const auto floorTile = [](int value) { return value / side - (value % side < 0); };
    const QPoint coordinate(floorTile(point.x()),floorTile(point.y()));
    const QPoint local = point - coordinate * side;
    QByteArray pixel = QByteArray::fromHex("ff0000ff");
    pixel[3] = char(defaultAlpha);
    auto bytes = pixel.repeated(RgbaTile::ByteCount / 4);
    const int offset = (local.y() * side + local.x()) * 4;
    bytes.replace(offset,4,center);
    if (halfTie) bytes.replace(offset+4,4,QByteArray::fromHex("00000005"));
    input.layers[0].tiles = {{coordinate,bytes}};
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device,128 * TileRenderer::TileBytes,1,32);
    const auto handle = execution.open(fixed).handle;
    auto future = execution.readColorSampleAsync(handle,fixed,projection ? 0 : fixed->layers[0].id,point,radius);
    QTRY_VERIFY((execution.pollReadbacks(), future.wait_for(std::chrono::seconds(0)) == std::future_status::ready));
    const auto result = future.get();
    QVERIFY2(result.code == ExecutionCode::Success,qPrintable(result.diagnostic));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(result.rgba.data()),4),expected);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("color-sample-%1.rgba").arg(QString::fromLatin1(QTest::currentDataTag()))));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(reinterpret_cast<const char *>(result.rgba.data()), 4), qint64(4));
    }
    QCOMPARE(result.document,fixed);
    QCOMPARE(result.generation,execution.generation());
    QCOMPARE(execution.requested(handle),fixed);
    QVERIFY(!execution.published(handle));
    QCOMPARE(document.snapshot(),fixed);
}

void KisGpuPixelReadbackTest::passThroughReadsKeepSubtreeBoundaryAndRootCapacity_data()
{
    QTest::addColumn<bool>("hidden");
    QTest::newRow("visible") << false;
    QTest::newRow("hidden") << true;
}

void KisGpuPixelReadbackTest::passThroughReadsKeepSubtreeBoundaryAndRootCapacity()
{
    QFETCH(bool, hidden);
    RasterImage input;
    input.size = {4, 4};
    const QUuid group(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    input.layers = {{QStringLiteral("Bottom"), true, {}, {}},
                    {QStringLiteral("First"), true, {}, {}},
                    {QStringLiteral("Second"), true, {}, {}},
                    {QStringLiteral("Group"), !hidden, group, {}}};
    input.layers[1].parentUuid = input.layers[2].parentUuid = group;
    input.layers[1].opacity = 153;
    input.layers[2].opacity = 211;
    input.layers[3].kind = LayerKind::Group;
    input.layers[3].passThrough = true;
    input.layers[3].opacity = 137;
    input.layers[3].projectionChannelMask = 9;
    const QByteArray colors[] = {QByteArray::fromHex("255997ad"), QByteArray::fromHex("c9295d89"),
                                 QByteArray::fromHex("11d34dbd")};
    for (int i = 0; i < 3; ++i)
        input.layers[i].tiles = {{{0, 0}, colors[i].repeated(RgbaTile::ByteCount / 4)}};
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, 4 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    const auto handle = execution.open(fixed).handle;
    QCOMPARE(document.setLayerPassThrough(4, false), Result::Success);
    QCOMPARE(document.writeLayerOpacity(4, 0), Result::Success);
    const auto current = document.snapshot();
    QCOMPARE(execution.request(handle, current).code, ExecutionCode::Success);
    const auto root = execution.readRgbaPixels(handle, fixed, {0, 0, 1, 1}, 0, 4);
    QVERIFY2(root.code == ExecutionCode::Success, qPrintable(root.diagnostic));
    QCOMPARE(root.bytes, hidden ? colors[0] : QByteArray::fromHex("345997d1"));
    const auto own = execution.readRgbaPixels(handle, fixed, {0, 0, 1, 1}, 4, 4);
    QVERIFY2(own.code == ExecutionCode::Success, qPrintable(own.diagnostic));
    // Independent isolated subtree: target attributes and outside state excluded.
    QCOMPARE(own.bytes, QByteArray::fromHex("30b650bc"));
    const auto preview = execution.readThumbnail(handle, fixed, 4, {0, 0, 4, 4}, {2, 2}, 16);
    QVERIFY2(preview.code == ExecutionCode::Success, qPrintable(preview.diagnostic));
    for (int y = 0; y < 2; ++y)
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(preview.image.constScanLine(y)), 8), own.bytes.repeated(2));
    const auto bounds = execution.readBounds(handle, fixed, 4);
    QVERIFY2(bounds.code == ExecutionCode::Success, qPrintable(bounds.diagnostic));
    QCOMPARE(*bounds.bounds, QRect(0, 0, 256, 256));
    QCOMPARE(execution.requested(handle), current);
    QVERIFY(!execution.published(handle));
    QCOMPARE(document.snapshot(), current);
    QVERIFY(fixed->layers[3].passThrough);
}

void KisGpuPixelReadbackTest::groupReadsKeepSubtreePixelsDefaultsAndPinnedVersions_data()
{
    QTest::addColumn<bool>("uniform");
    QTest::addColumn<bool>("hidden");
    QTest::addColumn<QByteArray>("rgba");
    QTest::newRow("explicit") << false << false << QByteArray::fromHex("ff0000ff");
    QTest::newRow("uniform") << true << false << QByteArray::fromHex("ff0000ff");
    QTest::newRow("hidden") << false << true << QByteArray::fromHex("ff0000ff");
    QTest::newRow("implicit-hidden-rgb") << true << false << QByteArray::fromHex("11223300");
}

void KisGpuPixelReadbackTest::groupReadsKeepSubtreePixelsDefaultsAndPinnedVersions()
{
    QFETCH(bool, uniform);
    QFETCH(bool, hidden);
    QFETCH(QByteArray, rgba);
    RasterImage input;
    input.size = {64, 48};
    input.layers = {
        {QStringLiteral("Backdrop"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}, {0, 0, 255, 255}},
        {QStringLiteral("Child"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}},
        {QStringLiteral("Group"), !hidden, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    input.layers[1].parentUuid = input.layers[2].uuid;
    input.layers[2].kind = LayerKind::Group;
    input.layers[2].opacity = 128;
    if (uniform) std::copy_n(reinterpret_cast<const quint8 *>(rgba.constData()), 4, input.layers[1].defaultPixel.begin());
    else input.layers[1].tiles = {{{-1, 0}, rgba.repeated(RgbaTile::ByteCount / 4)}, {{0, 0}, rgba.repeated(RgbaTile::ByteCount / 4)}};
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, 12 * TileRenderer::TileBytes, 1, 2 * TileRenderer::TileBytes);
    const auto handle = execution.open(fixed).handle;
    QCOMPARE(document.setLayerOpacity(3, 64), Result::Success);
    const auto newer = document.snapshot();
    QCOMPARE(execution.request(handle, newer).code, ExecutionCode::Success);
    const auto packed = execution.readRgbaPixels(handle, fixed, {-5, 7, 8, 2}, 3, 64);
    QVERIFY2(packed.code == ExecutionCode::Success, qPrintable(packed.diagnostic));
    QCOMPARE(packed.bytes, rgba.repeated(16));
    const auto complete = execution.readLayerPixels(handle, fixed, 3, 2 * TileRenderer::TileBytes);
    QVERIFY2(complete.code == ExecutionCode::Success, qPrintable(complete.diagnostic));
    QCOMPARE(complete.tiles.size(), uniform ? 0 : 2);
    for (const auto &tile : complete.tiles) QCOMPARE(tile.rgba, rgba.repeated(RgbaTile::ByteCount / 4));
    const auto bounds = execution.readBounds(handle, fixed, 3);
    QVERIFY2(bounds.code == ExecutionCode::Success, qPrintable(bounds.diagnostic));
    QCOMPARE(*bounds.bounds, uniform ? QRect() : QRect(-256, 0, 512, 256));
    const auto thumbnail = execution.readThumbnail(handle, fixed, 3, {-5, 7, 8, 2}, {4, 1}, 16);
    QVERIFY2(thumbnail.code == ExecutionCode::Success, qPrintable(thumbnail.diagnostic));
    QCOMPARE(thumbnail.image.size(), QSize(4, 1));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(thumbnail.image.constBits()), 16), rgba.repeated(4));
    const auto projection = execution.readProjectionImage(handle, fixed, 64 * 48 * 4);
    QVERIFY2(projection.code == ExecutionCode::Success, qPrintable(projection.diagnostic));
    const auto expected = hidden || rgba[3] == 0 ? QColor(0, 0, 255, 255) : QColor(128, 0, 127, 255);
    QCOMPARE(projection.image.pixelColor(12, 9), expected);
    for (LayerId sampledLayer : {LayerId(3), LayerId(0)}) {
        auto pending = execution.readColorSampleAsync(handle, fixed, sampledLayer, {0, 9}, 3);
        QTRY_VERIFY((execution.pollReadbacks(), pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready));
        const auto sampled = pending.get();
        QVERIFY2(sampled.code == ExecutionCode::Success, qPrintable(sampled.diagnostic));
        const auto expectedRgba = sampledLayer ? (rgba[3] == 0 ? QByteArray(4, '\0') : rgba)
            : QByteArray::fromHex(hidden || rgba[3] == 0 ? "0000ffaa" : "80007faa");
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(sampled.rgba.data()), 4), expectedRgba);
    }
    QCOMPARE(execution.requested(handle), newer);
    QVERIFY(!execution.published(handle));
    QCOMPARE(document.snapshot(), newer);
}

void KisGpuPixelReadbackTest::projectionSkipsHiddenSubtreesWithinVisibleCapacity()
{
    RasterImage input;
    input.size = {64, 48};
    input.layers = {
        {QStringLiteral("Backdrop"), true, {}, {}, {0, 0, 255, 255}},
        {QStringLiteral("Hidden subtree paint"), true, {}, {{{0, 0}, QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4)}}, {255, 0, 0, 255}},
        {QStringLiteral("Hidden group"), false, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    input.layers[1].parentUuid = input.layers[2].uuid;
    input.layers[2].kind = LayerKind::Group;
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, 2 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    const auto handle = execution.open(fixed).handle;
    const auto projected = execution.readProjectionImage(handle, fixed, 64 * 48 * 4);
    QVERIFY2(projected.code == ExecutionCode::Success, qPrintable(projected.diagnostic));
    QCOMPARE(projected.image.pixelColor(12, 9), QColor(0, 0, 255, 255));
    const auto preview = execution.readThumbnail(handle, fixed, 0, {0, 0, 64, 48}, {8, 6}, 8 * 6 * 4);
    QVERIFY2(preview.code == ExecutionCode::Success, qPrintable(preview.diagnostic));
    QCOMPARE(preview.image.pixelColor(1, 1), QColor(0, 0, 255, 255));
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto display = execution.project(handle, {0, 0, 64, 48});
    QVERIFY2(display.code == RenderOutcome::Recorded, qPrintable(display.diagnostic));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(document.snapshot(), fixed);
}

void KisGpuPixelReadbackTest::constantProjectionPreviewUsesOnlyOnePixelTransfer()
{
    RasterImage input;
    input.size = {9000, 9000};
    input.layers = {{QStringLiteral("Uniform canvas"), true, {}, {}, {17, 34, 51, 255}}};
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 1, 4);
    const auto handle = execution.open(fixed).handle;
    const auto result = execution.readResizedProjectionImage(handle, fixed, {32, 16}, 2048);
    QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.image.size(), QSize(16, 16));
    for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x)
        QCOMPARE(result.image.pixelColor(x, y), QColor(17, 34, 51, 255));
    QCOMPARE(execution.requested(handle), fixed);
    QVERIFY(!execution.published(handle));
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.cropLayer(document.activeLayer(), {3, 4, 50, 60}), Result::Success);
    const auto cropped = document.snapshot();
    QCOMPARE(execution.request(handle, cropped).code, ExecutionCode::Success);
    const auto afterCrop = execution.readResizedProjectionImage(handle, cropped, {32, 16}, 2048);
    QVERIFY2(afterCrop.code == ExecutionCode::Success, qPrintable(afterCrop.diagnostic));
    QCOMPARE(afterCrop.image, result.image);
    QCOMPARE(execution.requested(handle), cropped);
    QVERIFY(!execution.published(handle));
}

void KisGpuPixelReadbackTest::resizedProjectionPreservesFixedPixelsRequestsAndReturnedImage()
{
    QImage retained;
    {
        QString diagnostic;
        const auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        RasterImage input;
        input.size = {32, 16};
        const auto rgba = QByteArray::fromHex("11223349").repeated(RgbaTile::ByteCount / 4);
        input.layers = {{QStringLiteral("Fixed preview"), true, {}, {{{0, 0}, rgba}}}};
        OperationDocument document(input);
        const auto fixed = document.snapshot();
        Execution execution(device, 8 * TileRenderer::TileBytes, 1, 16);
        const auto handle = execution.open(fixed).handle;
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto displayed = execution.project(handle, {0, 0, 32, 16});
        QCOMPARE(displayed.code, RenderOutcome::Recorded);
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        const auto replacement = QByteArray::fromHex("aabbccff").repeated(RgbaTile::ByteCount / 4);
        QCOMPARE(document.replaceTiles(fixed->layers.first().id, {{{0, 0}, replacement}}), Result::Success);
        const auto newer = document.snapshot();
        QCOMPARE(execution.request(handle, newer).code, ExecutionCode::Success);
        const auto result = execution.readResizedProjectionImage(handle, fixed, {16, 8}, 16 * 8 * 4);
        QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
        retained = result.image;
        QCOMPARE(retained.size(), QSize(16, 8));
        for (int y = 0; y < retained.height(); ++y) for (int x = 0; x < retained.width(); ++x)
            QCOMPARE(retained.pixelColor(x, y), QColor(17, 34, 51, 73));
        QCOMPARE(result.document, fixed);
        QCOMPARE(execution.requested(handle), newer);
        QCOMPARE(execution.published(handle), fixed);
        const auto rejected = execution.readResizedProjectionImage(handle, fixed, {16, 8}, 16 * 8 * 4 - 1);
        QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
        QVERIFY(rejected.image.isNull());
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto busy = execution.readResizedProjectionImage(handle, fixed, {16, 8}, 16 * 8 * 4);
        QCOMPARE(busy.code, ExecutionCode::Busy);
        QVERIFY(busy.image.isNull());
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(execution.requested(handle), newer);
        QCOMPARE(execution.published(handle), fixed);
        QCOMPARE(document.snapshot(), newer);
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
        QCOMPARE(execution.readResizedProjectionImage(handle, fixed, {16, 8}, 16 * 8 * 4).code, ExecutionCode::InvalidRequest);
    }
    QCOMPARE(retained.pixelColor(0, 0), QColor(17, 34, 51, 73));
    QCOMPARE(retained.pixelColor(15, 7), QColor(17, 34, 51, 73));
}

void KisGpuPixelReadbackTest::resizedProjectionRejectsPinnedWorkingStorageWithoutChangingPublication()
{
    RasterImage input;
    input.size = {32, 16};
    input.layers = {{QStringLiteral("Pinned previews"), true, {}, {{{0, 0}, QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    Execution execution(m_device, 4 * TileRenderer::TileBytes, 1, 64);
    const auto handle = execution.open(document.snapshot()).handle;
    QList<ProjectedSnapshot> pinned;
    for (int i = 0; i < 4; ++i) {
        if (i) {
            const QByteArray rgba = QByteArray(1, char(17 + i)) + QByteArray::fromHex("2233ff");
            QCOMPARE(document.replaceTiles(document.activeLayer(), {{{0, 0}, rgba.repeated(RgbaTile::ByteCount / 4)}}), Result::Success);
            QCOMPARE(execution.request(handle, document.snapshot()).code, ExecutionCode::Success);
        }
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto displayed = execution.project(handle, {0, 0, 32, 16});
        QCOMPARE(displayed.code, RenderOutcome::Recorded);
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        pinned.append(displayed.version);
    }
    QCOMPARE(execution.residentBytes(), qsizetype(4 * TileRenderer::TileBytes));
    const auto fixed = document.snapshot();
    const auto rejected = execution.readResizedProjectionImage(handle, fixed, {4, 2}, 32);
    QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
    QVERIFY(rejected.image.isNull());
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.published(handle), fixed);
    pinned.clear();
    const auto result = execution.readResizedProjectionImage(handle, fixed, {4, 2}, 32);
    QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.image.pixelColor(0, 0), QColor(20, 34, 51, 255));
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.published(handle), fixed);
}

void KisGpuPixelReadbackTest::thumbnailWorkingStorageWaitsForPinnedGpuPixels()
{
    RasterImage input;
    input.size = {32, 16};
    const auto red = QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4);
    const auto blue = QByteArray::fromHex("335577ff").repeated(RgbaTile::ByteCount / 4);
    input.layers = {{QStringLiteral("Lower"), true, {}, {{{0, 0}, red}}},
                    {QStringLiteral("Upper"), true, {}, {{{0, 0}, blue}}}};
    input.layers.last().opacity = 128;
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, 3 * TileRenderer::TileBytes, 1, 64);
    const auto handle = execution.open(fixed).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    auto displayed = execution.project(handle, {0, 0, 32, 16});
    QCOMPARE(displayed.code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.residentBytes(), qsizetype(3 * TileRenderer::TileBytes));
    const auto rejected = execution.readThumbnail(handle, fixed, fixed->layers.first().id, {0, 0, 32, 16}, {4, 2}, 32);
    QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
    QVERIFY(rejected.image.isNull());
    QCOMPARE(execution.residentBytes(), qsizetype(3 * TileRenderer::TileBytes));
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.published(handle), fixed);
    displayed.version.clear();
    const auto result = execution.readThumbnail(handle, fixed, fixed->layers.first().id, {0, 0, 32, 16}, {4, 2}, 32);
    QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.image.pixelColor(0, 0), QColor(17, 34, 51, 255));
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.published(handle), fixed);
}

void KisGpuPixelReadbackTest::coarseThumbnailsRetainProjectionBackgroundOutsideStorageEdges()
{
    RasterImage input;
    input.size = {33, 17};
    input.projectionBackground = {17, 34, 51, 128};
    input.layers = {{QStringLiteral("Visible"), true, {}, {}, {49, 99, 149, 255}},
                    {QStringLiteral("Hidden"), false, {}, {}}};
    OperationDocument document(input);
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 1, 64);
    const auto fixed = document.snapshot();
    const auto handle = execution.open(fixed).handle;
    const auto result = execution.readThumbnail(handle, fixed, 0, {0, 0, 33, 17}, {16, 16}, 1024, true);
    QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
    for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x)
        QCOMPARE(result.image.pixelColor(x, y), x * 4 < 33 && y * 4 < 17 ? QColor(49, 99, 149, 255) : QColor(17, 34, 51, 128));
    QCOMPARE(execution.requested(handle), fixed);
}

void KisGpuPixelReadbackTest::thumbnailsStreamFixedHiddenPixelsAndPreserveRequests_data()
{
    QTest::addColumn<bool>("projection");
    QTest::newRow("hidden-layer") << false;
    QTest::newRow("visible-projection") << true;
}

void KisGpuPixelReadbackTest::thumbnailsStreamFixedHiddenPixelsAndPreserveRequests()
{
    QFETCH(bool, projection);
    QImage retained;
    QByteArray expected(16 * 8 * 4, '\0');
    const QRect region(-3, -1, 32, 16);
    const auto color = [](int x, int y) {
        QByteArray bytes(4, '\0');
        bytes[0] = char((x + 300) % 256);
        bytes[1] = char((y + 500) % 256);
        bytes[2] = char((x + y + 700) % 256);
        bytes[3] = char((x + y) % 3 ? 128 : 0);
        return bytes;
    };
    {
        QString diagnostic;
        const auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        RasterImage input;
        input.size = {32, 16};
        input.layers = {{QStringLiteral("Visible"), true, {}, {}, {49, 99, 149, 255}},
                        {QStringLiteral("Hidden"), false, {}, {}}};
        for (int ty = -1; ty <= 0; ++ty) for (int tx = -1; tx <= 0; ++tx) {
            QByteArray bytes(RgbaTile::ByteCount, '\0');
            for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x)
                bytes.replace((y * 256 + x) * 4, 4, color(tx * 256 + x, ty * 256 + y));
            input.layers.last().tiles.append({{tx, ty}, bytes});
        }
        OperationDocument document(input);
        const auto fixed = document.snapshot();
        Execution execution(device, 8 * TileRenderer::TileBytes, 1, 64);
        const auto handle = execution.open(fixed).handle;
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto displayed = execution.project(handle, {0, 0, 32, 16});
        QCOMPARE(displayed.code, RenderOutcome::Recorded);
        QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
        const auto resident = execution.residentBytes();
        if (projection) {
            QCOMPARE(document.setLayerOpacity(fixed->layers.first().id, 0), Result::Success);
        } else {
            const auto replacement = QByteArray::fromHex("aabbccff").repeated(RgbaTile::ByteCount / 4);
            QCOMPARE(document.replaceTiles(fixed->layers.last().id, {{{0, 0}, replacement}}), Result::Success);
        }
        const auto newer = document.snapshot();
        QCOMPARE(execution.request(handle, newer).code, ExecutionCode::Success);
        const auto layer = projection ? 0 : fixed->layers.last().id;
        const auto read = execution.readThumbnail(handle, fixed, layer, region, {16, 8}, expected.size());
        QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
        QCOMPARE(read.document, fixed);
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 16; ++x) {
            const QPoint point(-3 + 2 * x, -1 + 2 * y);
            const auto pixel = projection ? (QRect(0, 0, 32, 16).contains(point) ? QByteArray::fromHex("316395ff") : QByteArray(4, '\0')) : color(point.x(), point.y());
            expected.replace((y * 16 + x) * 4, 4, pixel);
        }
        retained = read.image;
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(retained.constBits()), retained.sizeInBytes()), expected);
        QCOMPARE(execution.requested(handle), newer);
        QCOMPARE(execution.published(handle), fixed);
        QCOMPARE(execution.residentBytes(), resident);
        QCOMPARE(execution.readThumbnail(handle, fixed, layer, region, {16, 8}, expected.size() - 1).code,
                 ExecutionCode::CapacityExceeded);
        QCOMPARE(execution.readThumbnail(handle, fixed, 999, region, {16, 8}, expected.size()).code,
                 ExecutionCode::InvalidRequest);
        QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
        const auto busy = execution.readThumbnail(handle, fixed, layer, region, {16, 8}, expected.size());
        QCOMPARE(busy.code, ExecutionCode::Busy);
        QVERIFY(busy.image.isNull());
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(execution.requested(handle), newer);
        QCOMPARE(execution.published(handle), fixed);
        QCOMPARE(document.snapshot(), newer);
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
        QCOMPARE(execution.readThumbnail(handle, fixed, layer, region, {16, 8}, expected.size()).code,
                 ExecutionCode::InvalidRequest);
    }
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(retained.constBits()), retained.sizeInBytes()), expected);
}

void KisGpuPixelReadbackTest::projectionReadsTranslatedCheckpointsWithinCanvasBudget()
{
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Four source tiles"), true, {}, {}}};
    for (int y = -1; y <= 0; ++y) for (int x = -1; x <= 0; ++x) {
        QByteArray pixels(RgbaTile::ByteCount, '\0');
        const int px = x < 0 ? 255 : 0, py = y < 0 ? 255 : 0;
        pixels.replace((py * 256 + px) * 4, 4, QByteArray::fromHex("112233ff"));
        input.layers.first().tiles.append({{x, y}, pixels});
    }
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    const auto operation = document.beginLayerTranslation(document.activeLayer());
    QCOMPARE(document.updateLayerTranslation(operation, {1, 1}), Result::Success);
    QCOMPARE(document.commitLayerTranslation(operation), Result::Success);
    const auto moved = document.snapshot();
    Execution execution(m_device, 12 * TileRenderer::TileBytes, 1);
    const auto handle = execution.open(moved).handle;
    const auto projection = execution.readProjectionImage(handle, moved, 64 * 64 * 4);
    QVERIFY2(projection.code == ExecutionCode::Success, qPrintable(projection.diagnostic));
    QByteArray expected(64 * 64 * 4, '\0');
    for (int y = 0; y < 2; ++y) for (int x = 0; x < 2; ++x)
        expected.replace((y * 64 + x) * 4, 4, QByteArray::fromHex("112233ff"));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(projection.image.constBits()), projection.image.sizeInBytes()), expected);
    const auto original = execution.readProjectionImage(handle, fixed, 64 * 64 * 4);
    QVERIFY2(original.code == ExecutionCode::Success, qPrintable(original.diagnostic));
    expected.fill('\0');
    expected.replace(0, 4, QByteArray::fromHex("112233ff"));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(original.image.constBits()), original.image.sizeInBytes()), expected);
    QCOMPARE(execution.readProjectionImage(handle, moved, 64 * 64 * 4 - 1).code, ExecutionCode::CapacityExceeded);
    QCOMPARE(execution.readLayerPixels(handle, moved, document.activeLayer(), TileRenderer::TileBytes).code, ExecutionCode::CapacityExceeded);
    QCOMPARE(execution.requested(handle), moved);
}

void KisGpuPixelReadbackTest::contentBoundsRetainTransparentRgbForMovementAdmission()
{
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((7 * 256 + 255) * 4, 4, QByteArray::fromHex("11223300"));
    RasterImage input;
    input.size = {32, 16};
    input.layers = {{QStringLiteral("Hidden edge pixel"), true, {}, {{{32767, 0}, pixels}}}};
    OperationDocument document(input);
    Execution execution(m_device, TileRenderer::TileBytes, 1, 16);
    const auto fixed = document.snapshot();
    const auto handle = execution.open(fixed).handle;
    QCOMPARE(execution.readBounds(handle, fixed, 1).bounds, std::optional<QRect>(QRect{}));
    const auto content = execution.readBounds(handle, fixed, 1, true);
    QVERIFY2(content.code == ExecutionCode::Success, qPrintable(content.diagnostic));
    QCOMPARE(content.bounds, std::optional<QRect>(QRect(8388607, 7, 1, 1)));
    QCOMPARE(execution.requested(handle), fixed);
}

void KisGpuPixelReadbackTest::integerTranslationPreservesRgbaAcrossTileBoundariesAndFixedVersions()
{
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((1 * 256 + 255) * 4, 4, QByteArray::fromHex("112233ff"));
    pixels.replace((1 * 256) * 4, 4, QByteArray::fromHex("44556600"));
    RasterImage input;
    input.size = {320, 64};
    input.layers = {{QStringLiteral("Moved pixels"), true, {}, {{{0, 0}, pixels}}}};
    input.layers.first().position = {7, -3};
    OperationDocument document(input);
    const auto before = document.snapshot();
    const auto layer = document.activeLayer();
    Execution execution(m_device, 12 * TileRenderer::TileBytes, 1);
    const auto handle = execution.open(before).handle;
    QCOMPARE(execution.readRgbaPixels(handle, before, {255, 1, 1, 1}, layer, 4).bytes, QByteArray::fromHex("112233ff"));
    const auto move = document.beginLayerTranslation(layer);
    QVERIFY(!move.isNull());
    QCOMPARE(document.updateLayerTranslation(move, {-13, 17}), Result::Success);
    QCOMPARE(document.commitLayerTranslation(move), Result::Success);
    const auto moved = document.snapshot();
    QCOMPARE(execution.request(handle, moved).code, ExecutionCode::Success);
    const auto visible = execution.readRgbaPixels(handle, moved, {242, 18, 1, 1}, layer, 4);
    QVERIFY2(visible.code == ExecutionCode::Success, qPrintable(visible.diagnostic));
    QCOMPARE(visible.bytes, QByteArray::fromHex("112233ff"));
    const auto hidden = execution.readRgbaPixels(handle, moved, {-13, 18, 1, 1}, layer, 4);
    QVERIFY2(hidden.code == ExecutionCode::Success, qPrintable(hidden.diagnostic));
    QCOMPARE(hidden.bytes, QByteArray::fromHex("44556600"));
    QCOMPARE(execution.readRgbaPixels(handle, moved, {255, 1, 1, 1}, layer, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(handle, before, {255, 1, 1, 1}, layer, 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(execution.requested(handle), moved);
    QCOMPARE(moved->layers.first().position, QPoint(-6, 14));
    Brush brush;
    brush.diameter = 3;
    brush.rgba = {117, 53, 201, 255};
    const auto stroke = document.beginStroke(layer, brush, 42);
    const QList<InputSample> samples{{QPointF(100.5, 33.5), 0.75, -0.2, 0.3, 17, 10}};
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = document.snapshot();
    const QRect patch(98, 31, 5, 5);
    const auto original = execution.readRgbaPixels(handle, painted, patch, layer, 100);
    QVERIFY2(original.code == ExecutionCode::Success, qPrintable(original.diagnostic));
    QVERIFY(original.bytes != QByteArray(100, '\0'));
    const auto next = document.beginLayerTranslation(layer);
    QCOMPARE(document.updateLayerTranslation(next, {171, -7}), Result::Success);
    QCOMPARE(document.commitLayerTranslation(next), Result::Success);
    const auto final = document.snapshot();
    const auto shiftedPaint = execution.readRgbaPixels(handle, final, patch.translated(171, -7), layer, 100);
    QVERIFY2(shiftedPaint.code == ExecutionCode::Success, qPrintable(shiftedPaint.diagnostic));
    QCOMPARE(shiftedPaint.bytes, original.bytes);
    QCOMPARE(execution.readRgbaPixels(handle, final, {158, 11, 1, 1}, layer, 4).bytes, QByteArray::fromHex("44556600"));

}

void KisGpuPixelReadbackTest::boundsReusePixelsRetainedByTheDisplayedFrame()
{
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((9 * 256 + 7) * 4, 4, QByteArray::fromHex("112233ff"));
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Displayed layer"), true, {}, {{{0, 0}, pixels}}}};
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    Execution execution(m_device, TileRenderer::TileBytes, 1, 16);
    const auto handle = execution.open(fixed).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto displayed = execution.project(handle, {0, 0, 64, 64});
    QVERIFY2(displayed.code == RenderOutcome::Recorded, qPrintable(displayed.diagnostic));
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    const auto bounds = execution.readBounds(handle, fixed, fixed->layers.first().id);
    QVERIFY2(bounds.code == ExecutionCode::Success, qPrintable(bounds.diagnostic));
    QCOMPARE(bounds.bounds, std::optional<QRect>(QRect(7, 9, 1, 1)));
    QCOMPARE(displayed.version->tiles().size(), 1);
    QCOMPARE(execution.published(handle), fixed);
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

void KisGpuPixelReadbackTest::boundsStreamMoreTilesThanTheGpuCanRetain()
{
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((7 * 256 + 254) * 4, 4, QByteArray::fromHex("112233ff"));
    RasterImage image;
    image.size = {1024, 256};
    image.layers = {{QStringLiteral("Sparse"), true, {},
                    {{{-1, 0}, pixels}, {{0, 0}, pixels}, {{2, 0}, pixels}}}};
    OperationDocument document(image);
    Execution execution(m_device, TileRenderer::TileBytes, 1, 16);
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    const auto read = execution.readBounds(handle, pinned, pinned->layers.first().id);
    QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
    QCOMPARE(read.bounds, std::optional<QRect>(QRect(-2, 7, 769, 1)));
    const auto projection = execution.readBounds(handle, pinned, 0);
    QVERIFY2(projection.code == ExecutionCode::Success, qPrintable(projection.diagnostic));
    QCOMPARE(projection.bounds, std::optional<QRect>(QRect(254, 7, 513, 1)));
    QVERIFY(read.tiles.isEmpty());
    QVERIFY(projection.tiles.isEmpty());
    QCOMPARE(execution.requested(handle), pinned);
    QVERIFY(!execution.published(handle));
}

void KisGpuPixelReadbackTest::boundsKeepFixedVersionsAndRejectWithoutChangingRequests()
{
    RasterImage image;
    image.size = {64, 64};
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((7 * 256 + 254) * 4, 4, QByteArray::fromHex("112233ff"));
    image.layers = {{QStringLiteral("Hidden"), false, {}, {{{-1, 0}, pixels}}}};
    OperationDocument document(image);
    const auto pinned = document.snapshot();
    const auto layer = pinned->layers.first().id;
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 1, 16);
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(document.replaceTiles(layer, {{{-1, 0}, QByteArray(RgbaTile::ByteCount, '\0')}}), Result::Success);
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    const auto read = execution.readBounds(handle, pinned, layer);
    QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
    QCOMPARE(read.bounds, std::optional<QRect>(QRect(-2, 7, 1, 1)));
    QVERIFY(read.tiles.isEmpty());
    QCOMPARE(read.document, pinned);
    QCOMPARE(execution.requested(handle), latest);
    QVERIFY(!execution.published(handle));
    const auto empty = execution.readBounds(handle, latest, layer);
    QCOMPARE(empty.code, ExecutionCode::Success);
    QCOMPARE(empty.bounds, std::optional<QRect>(QRect()));
    QCOMPARE(execution.readBounds(handle, latest, 0).bounds, std::optional<QRect>(QRect()));
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    const auto busy = execution.readBounds(handle, pinned, layer);
    QCOMPARE(busy.code, ExecutionCode::Busy);
    QVERIFY(!busy.bounds);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    OperationDocument foreign({64, 64});
    const auto wrong = execution.readBounds(handle, foreign.snapshot(), layer);
    QCOMPARE(wrong.code, ExecutionCode::InvalidRequest);
    QVERIFY(!wrong.bounds);
    Execution limited(m_device, 8 * TileRenderer::TileBytes, 1, 15);
    const auto small = limited.open(pinned).handle;
    const auto rejected = limited.readBounds(small, pinned, layer);
    QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded);
    QVERIFY(!rejected.bounds);
    QCOMPARE(limited.requested(small), pinned);
    QVERIFY(!limited.published(small));
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.readBounds(handle, pinned, layer).code, ExecutionCode::InvalidRequest);
    QCOMPARE(read.bounds, std::optional<QRect>(QRect(-2, 7, 1, 1)));
}

void KisGpuPixelReadbackTest::projectionTransparencyUsesPinnedPixelsAndCanvasClipping()
{
    QByteArray rgba = QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4);
    rgba[7] = '\0';
    rgba[TileRenderer::TileSize * 4 + 3] = '\0';
    RasterImage image;
    image.size = {1, 2};
    image.layers = {{QStringLiteral("Visible"), true, {}, {{{0, 0}, rgba}}},
                    {QStringLiteral("Hidden"), false, {}, {}, {20, 40, 60, 255}}};
    OperationDocument document(image);
    const auto pinned = document.snapshot();
    Execution execution(m_device, 4 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    const auto handle = execution.open(pinned).handle;
    auto pending = execution.readTransparencyAsync(handle, pinned, {0, 0, 1, 1});
    QCOMPARE(document.setLayerOpacity(pinned->layers.first().id, 128), Result::Success);
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    QTRY_VERIFY_WITH_TIMEOUT((execution.pollReadbacks(), pending.wait_for(std::chrono::seconds(0))) == std::future_status::ready, 5000);
    const auto opaque = pending.get();
    QVERIFY2(opaque.code == ExecutionCode::Success, qPrintable(opaque.diagnostic));
    QVERIFY(opaque.containsTransparency.has_value());
    QVERIFY(!*opaque.containsTransparency);
    QVERIFY(opaque.tiles.isEmpty());
    QCOMPARE(opaque.document, pinned);
    QCOMPARE(execution.requested(handle), latest);
    QVERIFY(!execution.published(handle));
    pending = execution.readTransparencyAsync(handle, latest, {-1, -1, 3, 4});
    QTRY_VERIFY_WITH_TIMEOUT((execution.pollReadbacks(), pending.wait_for(std::chrono::seconds(0))) == std::future_status::ready, 5000);
    const auto transparent = pending.get();
    QVERIFY2(transparent.code == ExecutionCode::Success, qPrintable(transparent.diagnostic));
    QCOMPARE(transparent.region, QRect(0, 0, 1, 2));
    QVERIFY(transparent.containsTransparency.value_or(false));
    QVERIFY(transparent.tiles.isEmpty());
    QCOMPARE(document.writeProjectionBackground({0, 0, 0, 255}), Result::Success);
    pending = execution.readTransparencyAsync(handle, document.snapshot(), {0, 0, 1, 2});
    QTRY_VERIFY_WITH_TIMEOUT((execution.pollReadbacks(), pending.wait_for(std::chrono::seconds(0))) == std::future_status::ready, 5000);
    const auto background = pending.get();
    QVERIFY2(background.code == ExecutionCode::Success, qPrintable(background.diagnostic));
    QVERIFY(background.containsTransparency.has_value());
    QVERIFY(!*background.containsTransparency);
    QCOMPARE(execution.requested(handle), latest);
    QTRY_COMPARE(execution.pollReadbacks().code, ExecutionCode::Success);
    OperationDocument foreign({1, 2});
    const auto rejected = execution.readTransparencyAsync(handle, foreign.snapshot(), {0, 0, 1, 2}).get();
    QCOMPARE(rejected.code, ExecutionCode::InvalidRequest);
    QVERIFY(!rejected.containsTransparency);
    QVERIFY(rejected.tiles.isEmpty());
}

void KisGpuPixelReadbackTest::transparencyResultsOutliveClosedDocumentsAndTheirDevice()
{
    std::future<PixelReadbackOutcome> pending;
    std::weak_ptr<Device> lifetime;
    {
        QString diagnostic;
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        lifetime = device;
        OperationDocument document({513, 1});
        Execution execution(device, 4 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
        const auto handle = execution.open(document.snapshot()).handle;
        pending = execution.readTransparencyAsync(handle, document.snapshot(), {256, 0, 256, 1});
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    }
    QVERIFY(lifetime.expired());
    QCOMPARE(pending.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    const auto transparent = pending.get();
    QVERIFY2(transparent.code == ExecutionCode::Success, qPrintable(transparent.diagnostic));
    QVERIFY(transparent.containsTransparency.value_or(false));
    QVERIFY(transparent.tiles.isEmpty());
}

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
    brush.opacity = qreal(rgba[3]) / 255;
    brush.rgba[3] = 255;
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
}

void KisGpuPixelReadbackTest::colorSamplesKeepPaintedVersionsAndPublication()
{
    OperationDocument document({64, 64});
    dot(document, {12, 12}, {17, 129, 241, 255});
    const auto pinned = document.snapshot();
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 1, 32);
    const auto handle = execution.open(pinned).handle;
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.project(handle, {0, 0, 64, 64}).code, RenderOutcome::Recorded);
    QCOMPARE(execution.submitFrame().code, ExecutionCode::Success);
    auto pending = execution.readColorSampleAsync(handle, pinned, document.activeLayer(), {12, 12}, 3);
    dot(document, {12, 12}, {219, 33, 81, 255});
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    QTRY_VERIFY((execution.pollReadbacks(), pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready));
    const auto sampled = pending.get();
    QVERIFY2(sampled.code == ExecutionCode::Success, qPrintable(sampled.diagnostic));
    QCOMPARE(sampled.rgba, (std::array<quint8, 4>{17, 129, 241, 255}));
    QCOMPARE(sampled.document, pinned);
    QCOMPARE(execution.requested(handle), latest);
    QCOMPARE(execution.published(handle), pinned);
    auto newer = execution.readColorSampleAsync(handle, latest, 0, {12, 12}, 3);
    QTRY_VERIFY((execution.pollReadbacks(), newer.wait_for(std::chrono::seconds(0)) == std::future_status::ready));
    const auto next = newer.get();
    QVERIFY2(next.code == ExecutionCode::Success, qPrintable(next.diagnostic));
    QCOMPARE(next.rgba, (std::array<quint8, 4>{219, 33, 81, 255}));
    QCOMPARE(execution.published(handle), pinned);
}

void KisGpuPixelReadbackTest::colorSamplesOutliveCloseAndDeviceReplacement()
{
    std::future<ColorSampleOutcome> retained;
    std::weak_ptr<Device> lifetime;
    {
        QString diagnostic;
        const auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        lifetime = device;
        OperationDocument document({64, 64});
        dot(document, {12, 12}, {17, 129, 241, 255});
        const auto pinned = document.snapshot();
        Execution execution(device, 8 * TileRenderer::TileBytes, 1, 32);
        const auto handle = execution.open(pinned).handle;
        auto old = execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 3);
        const auto replacement = Device::createNative(diagnostic);
        QVERIFY2(replacement, qPrintable(diagnostic));
        QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
        const auto previous = old.get();
        QCOMPARE(previous.code, ExecutionCode::Success);
        QCOMPARE(previous.generation, quint64(1));
        QCOMPARE(previous.rgba, (std::array<quint8, 4>{17, 129, 241, 255}));
        retained = execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 3);
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    }
    QVERIFY(lifetime.expired());
    QCOMPARE(retained.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    const auto sampled = retained.get();
    QVERIFY2(sampled.code == ExecutionCode::Success, qPrintable(sampled.diagnostic));
    QCOMPARE(sampled.generation, quint64(2));
    QCOMPARE(sampled.rgba, (std::array<quint8, 4>{17, 129, 241, 255}));
}

void KisGpuPixelReadbackTest::rejectedColorSamplesLeaveTheNextReadAvailable()
{
    OperationDocument document({64, 64});
    dot(document, {12, 12}, {17, 129, 241, 255});
    const auto pinned = document.snapshot();
    Execution tooSmall(m_device, 8 * TileRenderer::TileBytes, 1, 31);
    const auto smallHandle = tooSmall.open(pinned).handle;
    QCOMPARE(tooSmall.readColorSampleAsync(smallHandle, pinned, 0, {12, 12}, 3).get().code, ExecutionCode::CapacityExceeded);
    QCOMPARE(tooSmall.residentBytes(), qsizetype(0));
    Execution execution(m_device, TileRenderer::TileBytes, 1, 32);
    const auto handle = execution.open(pinned).handle;
    OperationDocument other({64, 64});
    QCOMPARE(execution.readColorSampleAsync(handle, other.snapshot(), 0, {12, 12}, 3).get().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 0).get().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 901).get().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 999, {12, 12}, 3).get().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {int(OperationDocument::CoordinateLimit), 0}, 3).get().code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 3).get().code, ExecutionCode::Busy);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 3).get().code, ExecutionCode::CapacityExceeded);
    auto recovered = execution.readColorSampleAsync(handle, pinned, 0, {-500, -500}, 3);
    QTRY_VERIFY((execution.pollReadbacks(), recovered.wait_for(std::chrono::seconds(0)) == std::future_status::ready));
    const auto result = recovered.get();
    QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.rgba, (std::array<quint8, 4>{}));
    QCOMPARE(execution.requested(handle), pinned);
    QVERIFY(!execution.published(handle));
    QCOMPARE(document.snapshot(), pinned);
    QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    QCOMPARE(execution.readColorSampleAsync(handle, pinned, 0, {12, 12}, 3).get().code, ExecutionCode::InvalidRequest);
}

void KisGpuPixelReadbackTest::asynchronousPixelsKeepThePinnedVersionAndNewerRequests()
{
    OperationDocument document({64, 64});
    dot(document, {12, 12}, {17, 129, 241, 255});
    const auto pinned = document.snapshot();
    Execution execution(m_device);
    const auto handle = execution.open(pinned).handle;
    QVERIFY(handle);
    auto pending = execution.readPixelsAsync(handle, pinned, {0, 0, 64, 64});
    dot(document, {12, 12}, {219, 33, 81, 255});
    const auto latest = document.snapshot();
    QCOMPARE(execution.request(handle, latest).code, ExecutionCode::Success);
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        execution.pollReadbacks();
        return pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    })(), 5000);
    const auto pixels = pending.get();
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.document, pinned);
    QCOMPARE(pixel(pixels, 0, {12, 12}), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(execution.requested(handle), latest);
    QVERIFY(!execution.published(handle));
    OperationDocument unrelated({64, 64});
    auto rejected = execution.readPixelsAsync(handle, unrelated.snapshot(), {0, 0, 64, 64});
    QCOMPARE(rejected.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    const auto failure = rejected.get();
    QCOMPARE(failure.code, ExecutionCode::InvalidRequest);
    QVERIFY(failure.tiles.isEmpty());
}

void KisGpuPixelReadbackTest::synchronousPreviewsCompleteEarlierAsynchronousReads_data()
{
    QTest::addColumn<bool>("summary");
    QTest::addColumn<int>("preview");
    QTest::newRow("pixels-thumbnail") << false << 0;
    QTest::newRow("pixels-resized-projection") << false << 1;
    QTest::newRow("pixels-bounds") << false << 2;
    QTest::newRow("transparency-thumbnail") << true << 0;
    QTest::newRow("transparency-resized-projection") << true << 1;
    QTest::newRow("transparency-bounds") << true << 2;
}

void KisGpuPixelReadbackTest::synchronousPreviewsCompleteEarlierAsynchronousReads()
{
    QFETCH(bool, summary);
    QFETCH(int, preview);
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Preview"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11223349").repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    input.layers[0].tiles[0].rgba = QByteArray::fromHex("536d89bf").repeated(RgbaTile::ByteCount / 4);
    OperationDocument exported(input);
    const auto fixed = document.snapshot();
    const auto exportedFixed = exported.snapshot();
    Execution execution(m_device, 8 * TileRenderer::TileBytes, 2, TileRenderer::TileBytes);
    const auto handle = execution.open(fixed).handle;
    const auto exportedHandle = execution.open(exportedFixed).handle;
    QVERIFY(handle && exportedHandle);
    const auto readPreview = [&] {
        return preview == 0 ? execution.readThumbnail(handle, fixed, 0, {0, 0, 64, 64}, {8, 8}, 256)
                            : execution.readResizedProjectionImage(handle, fixed, {8, 8}, 256);
    };
    QImage expected;
    if (preview != 2) {
        const auto initial = readPreview();
        QVERIFY2(initial.code == ExecutionCode::Success, qPrintable(initial.diagnostic));
        expected = initial.image;
        QVERIFY(!expected.isNull());
    }
    auto pending = summary ? execution.readTransparencyAsync(exportedHandle, exportedFixed, {0, 0, 64, 64})
                           : execution.readPixelsAsync(exportedHandle, exportedFixed, {0, 0, 64, 64});
    const bool unfinished = pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready;
    qInfo() << "Earlier native read still pending:" << unfinished;
    if (unfinished) {
        auto deferred = execution.readPixelsAsync(handle, fixed, {0, 0, 64, 64});
        QCOMPARE(deferred.wait_for(std::chrono::seconds(0)), std::future_status::ready);
        const auto busy = deferred.get();
        QCOMPARE(busy.code, ExecutionCode::Busy);
        QVERIFY(busy.tiles.isEmpty());
    }
    QCOMPARE(exported.setLayerOpacity(exportedFixed->layers.first().id, 0), Result::Success);
    const auto latest = exported.snapshot();
    QCOMPARE(execution.request(exportedHandle, latest).code, ExecutionCode::Success);
    if (preview == 2) {
        const auto bounds = execution.readBounds(handle, fixed, 0);
        QVERIFY2(bounds.code == ExecutionCode::Success, qPrintable(bounds.diagnostic));
        QCOMPARE(bounds.bounds, std::optional<QRect>(QRect(0, 0, 64, 64)));
    } else {
        const auto result = readPreview();
        QVERIFY2(result.code == ExecutionCode::Success, qPrintable(result.diagnostic));
        QCOMPARE(result.image, expected);
    }
    QCOMPARE(pending.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    const auto prior = pending.get();
    QVERIFY2(prior.code == ExecutionCode::Success, qPrintable(prior.diagnostic));
    QCOMPARE(prior.document, exportedFixed);
    if (summary) {
        QVERIFY(prior.containsTransparency.value_or(false));
        QVERIFY(prior.tiles.isEmpty());
    } else {
        QCOMPARE(pixel(prior, 0, {12, 12}), QByteArray::fromHex("536d89bf"));
    }
    QCOMPARE(execution.requested(handle), fixed);
    QCOMPARE(execution.requested(exportedHandle), latest);
    QVERIFY(!execution.published(handle));
    QVERIFY(!execution.published(exportedHandle));
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(exported.snapshot(), latest);
    QVERIFY(!m_device->isRecordingFrame());
    QCOMPARE(execution.pollReadbacks().code, ExecutionCode::Success);
}

void KisGpuPixelReadbackTest::asynchronousResultsOutliveClosedDocumentsAndTheDevice()
{
    std::future<PixelReadbackOutcome> pending;
    std::weak_ptr<Device> lifetime;
    {
        QString diagnostic;
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        lifetime = device;
        OperationDocument document({64, 64});
        dot(document, {12, 12}, {17, 129, 241, 255});
        Execution execution(device);
        const auto handle = execution.open(document.snapshot()).handle;
        pending = execution.readPixelsAsync(handle, document.snapshot(), {0, 0, 64, 64});
        QCOMPARE(execution.close(handle).code, ExecutionCode::Success);
    }
    QVERIFY(lifetime.expired());
    QCOMPARE(pending.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    const auto pixels = pending.get();
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixel(pixels, 0, {12, 12}), QByteArray::fromHex("1181f1ff"));
}

void KisGpuPixelReadbackTest::discardedAsynchronousReadsReleaseTheirNativeStorage()
{
    OperationDocument document({64, 64});
    dot(document, {12, 12}, {17, 129, 241, 255});
    Execution execution(m_device);
    const auto pinned = document.snapshot();
    const auto handle = execution.open(pinned).handle;
    {
        auto discarded = execution.readPixelsAsync(handle, pinned, {0, 0, 64, 64});
        QVERIFY(discarded.valid());
    }
    QTRY_VERIFY_WITH_TIMEOUT(execution.pollReadbacks().code == ExecutionCode::Success, 5000);
    const auto pixels = execution.readPixels(handle, pinned, {0, 0, 64, 64});
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixel(pixels, 0, {12, 12}), QByteArray::fromHex("1181f1ff"));
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

void KisGpuPixelReadbackTest::sparseLayerReadsIncludeSolidFigureInteriors()
{
    OperationDocument document(QSize(1024, 1024));
    const auto layer = document.activeLayer();
    FigureStyle style;
    style.outline = false;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{1024, 1024}, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, {1024, 1024}, 0}, style, 42), Result::Success);
    Execution execution(m_device, 32 * TileRenderer::TileBytes, 1, 16 * TileRenderer::TileBytes);
    const auto handle = execution.open(document.snapshot()).handle;
    const auto read = execution.readLayerPixels(handle, document.snapshot(), layer, 16 * TileRenderer::TileBytes);
    QVERIFY2(read.code == ExecutionCode::Success, qPrintable(read.diagnostic));
    QCOMPARE(read.tiles.size(), 16);
    QCOMPARE(pixel(read, layer, {512, 512}), QByteArray::fromHex("dd112cff"));
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("solid-fill-layer-interior.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        for (const auto &tile : read.tiles) QCOMPARE(file.write(tile.rgba), qint64(RgbaTile::ByteCount));
    }
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
    QVERIFY(!m_device->isRecordingFrame());
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
