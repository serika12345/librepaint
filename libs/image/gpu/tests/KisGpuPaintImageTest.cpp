/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QTest>
#include <limits>
#include <cmath>
#include <cstring>
#include <stdexcept>
#ifdef Q_OS_LINUX
#include <QFileInfo>
#include <sys/stat.h>
#endif

using namespace LibrePaint::Image::Gpu;

class KisGpuPaintImageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void sequentialConnectionsReleaseDevicesAndKeepGpuDescriptorsBounded();
    void rasterCopiesRetainFixedPixelsAndExecution_data();
    void rasterCopiesRetainFixedPixelsAndExecution();
    void native16PixelWritesKeepLowBitsHiddenRgbAndPinnedHistory();
    void native16ProtectedStrokeKeepsZeroCoverageHistoryBlocks();
    void native16RasterCopiesKeepPrecisionPixelsAndWorkingCapacity();
    void positionedPixelWritesPreservePixelsAndVersions_data();
    void positionedPixelWritesPreservePixelsAndVersions();
    void directPixelWritesFollowCanvasOriginsAcrossHistory();
    void canvasResizePublishesPinnedSizesAndRejectsIncompleteMoves();
    void layerCropPublishesFixedPixelsAndPreservesRejectedRequests();
    void directWritesKeepTheirCropHistoryBlocks_data();
    void directWritesKeepTheirCropHistoryBlocks();
    void translationPreviewsPublishPixelsAndCancelWithoutReplacingRedo();
    void affinePreviewPreservesCanonicalPixelsOnCancelIdentityAndRejection();
    void selectedMoveUsesNativeEmptyDetectionAndKeepsLowAlphaPixels();
    void nativeBoundsIgnoreSubByteImplicitAlpha();
    void affineResamplingReadsMovedSourceTilesAcrossCanonicalBoundaries();
    void erasedMasksRemainTransparentAndAcceptFurtherPainting_data();
    void erasedMasksRemainTransparentAndAcceptFurtherPainting();
    void defaultMasksKeepGroupProjectionAndCoordinateEdits();
    void paintedMasksKeepSparseGpuCoverageAfterClosure();
    void filterNodeProjectionReadersFollowCoverageEdits_data();
    void filterNodeProjectionReadersFollowCoverageEdits();
    void maskPixelWritesKeepCommandBlocks_data();
    void maskPixelWritesKeepCommandBlocks();
    void adjustmentMergesFreezeTheOriginalLowerStack_data();
    void adjustmentMergesFreezeTheOriginalLowerStack();
    void sparsePaintedMasksMoveCombineCropAndRecover();
    void layerPickingKeepsNativeAlphaMasksAndPinnedRequests_data();
    void layerPickingKeepsNativeAlphaMasksAndPinnedRequests();
    void layerForestTranslationKeepsPixelsMasksAndOneHistory_data();
    void layerForestTranslationKeepsPixelsMasksAndOneHistory();
    void rejectedTranslationKeepsHiddenPixelsHistoryPreviewAndRequests();
    void colorFillKeepsVersionsHistoryAndWholeRequestFailures();
    void contiguousFillKeepsFrozenPixelsHistoryAndClones();
    void paperReferencesRejectRetainedResourceOverflow();
    void mergedSparseDefaultsKeepAllocatedBytes_data();
    void mergedSparseDefaultsKeepAllocatedBytes();
    void mergedPixelsKeepRootReadersAndIndependentClearHistory_data();
    void mergedPixelsKeepRootReadersAndIndependentClearHistory();
    void nativeRootReadsKeepOffCanvasPixelsAndPinnedVersions_data();
    void nativeRootReadsKeepOffCanvasPixelsAndPinnedVersions();
    void clearedLayersKeepFixedVersionsAndRejectWholeRequests();
    void clearHistoryKeepsItsMeaningAfterDirectWrites();
    void retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory();
    void rejectedRetainedLayerWritesPreserveVersionsAndHistory();
    void retainedImagesAndClonesKeepTheirExecutionAlive();
    void clonesKeepIndependentCurrentStateAndOperationIdentities();
    void nestedCopiesRemapParentsAndDetachTheSelectedSubtree();
    void retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime();
    void insertedLayerPixelsPublishAtomicallyAndRemainIndependent();
    void filledFiguresPublishOneVersionAndRejectWholeRequests();
    void protectedCopyOutlineKeepsNormalFillDestinationBounds();
    void sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment();
    void directPixelsReachGpuWithoutEditingNotificationsOrHistory();
    void rectangularWritesPreserveOtherRegionsAcrossHistory_data();
    void rectangularWritesPreserveOtherRegionsAcrossHistory();
    void rectangularWritesValidateBeforeMutationAndCrossTileBoundaries();
    void rectangularHistoryEditsPreserveIndependentUntrackedPixels();
    void autoBrushKeepsIndependentPixelWritesAcrossUndo();
    void interiorPixelWritesRemainWithTheirFilledHistory();
    void idleSmoothingPublishesAcceptedVersionsWithoutDuplicateNotifications();
    void derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites_data();
    void derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites();
    void historyNavigationPublishesOneVersionAndPreservesSavedState();
};


// A colour selection retains the painted source. Reject its complete input
// before accepting a history entry when the retained paper exceeds capacity.
void KisGpuPaintImageTest::paperReferencesRejectRetainedResourceOverflow()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 8 * RgbaTile::ByteCount, 4, RgbaTile::ByteCount);
    RasterImage input; input.size = {64, 64}; input.layers = {{QStringLiteral("Paper")}};
    Limits limits; limits.rasterBytes = 128 * 128;
    PaintImage image(execution, input, limits);
    Brush brush; brush.rgba = {17, 34, 51, 255}; brush.diameter = 10;
    brush.autoBrush.emplace().mask = AutoBrush::Mask::Circle;
    auto &paper = brush.texture.emplace(); paper.size = {128, 128};
    paper.alpha = QByteArray(128 * 128, char(0)); paper.mode = BrushTexture::Mode::Subtract;
    paper.pressure = PressureResponse{PressureResponse::Kind::Curve, 1, QList<qreal>(256, 1)};
    const auto id = image.beginStroke(image.operations().activeLayer(), brush, 17); QVERIFY(!id.isNull());
    QCOMPARE(image.appendSamples(id, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(id), Result::Success);
    const auto before = image.operations().snapshot();
    const auto pixels = image.readRgbaPixels(image.operations().activeLayer(), QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(pixels.code, ExecutionCode::Success);
    RegionFillSettings settings; settings.seed = {32, 32}; settings.threshold = 1;
    QCOMPARE(image.selectRegion(image.operations().activeLayer(), settings, SelectionCombination::Replace), Result::CapacityExceeded);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.readRgbaPixels(image.operations().activeLayer(), QRect(0, 0, 64, 64), pixels.bytes.size()).bytes, pixels.bytes);
}

void KisGpuPaintImageTest::native16PixelWritesKeepLowBitsHiddenRgbAndPinnedHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    constexpr auto format = PixelFormat::Rgba16;
    constexpr int tileBytes = RgbaTile::byteCount(format);
    Execution execution(device, 8 * tileBytes, 4, tileBytes);
    RasterImage input;
    input.size = {4, 3};
    input.format = format;
    input.layers = {{QStringLiteral("Native writes"), true, {}, {}}};
    PaintImageSP image;
    try {
        image = PaintImageSP::create(execution, input);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    const auto layer = image->operations().activeLayer();
    const QRect region(-1, 0, 2, 1);
    const QByteArray first = rgbaBytes({1, 257, 65534, 0}, format) + rgbaBytes({3, 256, 32768, 1}, format);
    const auto initial = image->operations().snapshot();
    QCOMPARE(image->writeRgbaPixels(layer, region, first.first(first.size() - 1)), Result::InvalidInput);
    QCOMPARE(image->operations().snapshot(), initial);
    QCOMPARE(image->writeRgbaPixels(layer, region, first), Result::Success);
    const auto pinned = image->operations().snapshot();
    QCOMPARE(image->operations().undoCount(), 0);
    auto pixels = image->readRgbaPixels(layer, region, first.size());
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, first);
    const QByteArray second = rgbaBytes({65534, 1, 257, 256}, format) + rgbaBytes({257, 65534, 1, 65535}, format);
    QCOMPARE(image->replaceRgbaPixels(layer, region, second), Result::Success);
    QCOMPARE(image->operations().undoCount(), 1);
    const auto current = image->operations().snapshot();
    QCOMPARE(image->readRgbaPixels(layer, region, second.size()).bytes, second);
    QCOMPARE(execution.readRgbaPixels(image->handle(), pinned, region, layer, first.size()).bytes, first);
    QCOMPARE(image->operations().snapshot(), current);
    QCOMPARE(image->undo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, first.size()).bytes, first);
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, region, second.size()).bytes, second);
}

void KisGpuPaintImageTest::native16ProtectedStrokeKeepsZeroCoverageHistoryBlocks()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    constexpr auto format = PixelFormat::Rgba16;
    constexpr int tileBytes = RgbaTile::byteCount(format);
    Execution execution(device, 32 * tileBytes, 4, tileBytes);
    RasterImage input;
    input.size = {130, 129}; input.format = format;
    RasterLayer layer; layer.defaultPixel = {1, 257, 65534, 0};
    input.layers = {layer};
    const auto image = PaintImageSP::create(execution, input);
    const auto id = image->operations().activeLayer();
    const QRect corner(-16, -16, 1, 1), outside(192, 192, 1, 1);
    const auto original = rgbaBytes(layer.defaultPixel, format);
    const auto before = image->operations().snapshot();
    QCOMPARE(image->readRgbaPixels(id, corner, 8).bytes, original);
    Brush brush;
    brush.rgba = {0x1357, 0x4321, 0x9876, 65535};
    brush.diameter = 140; brush.channelWriteMask = 9; brush.blendMode = LayerBlendMode::Multiply;
    const auto stroke = image->beginStroke(id, brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(image->appendSamples(stroke, {{{50, 50}, 1}}), Result::Success);
    QCOMPARE(image->commitStroke(stroke), Result::Success);
    // This corner block is inside the native source allocation and outside
    // the circle. Partial channels still normalize its transparent pixels.
    QCOMPARE(image->readRgbaPixels(id, corner, 8).bytes, QByteArray(8, '\0'));
    const auto painted = image->operations().snapshot();
    const auto written = rgbaBytes({17, 256, 60001, 0}, format);
    QCOMPARE(image->writeRgbaPixels(id, corner, written), Result::Success);
    QCOMPARE(image->writeRgbaPixels(id, outside, written), Result::Success);
    QCOMPARE(image->operations().undoCount(), 1);
    QCOMPARE(image->readRgbaPixels(id, corner, 8).bytes, written);
    QCOMPARE(execution.readRgbaPixels(image->handle(), painted, corner, id, 8).bytes, QByteArray(8, '\0'));
    QCOMPARE(image->undo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(id, corner, 8).bytes, original);
    QCOMPARE(image->readRgbaPixels(id, outside, 8).bytes, written);
    QCOMPARE(execution.readRgbaPixels(image->handle(), before, outside, id, 8).bytes, original);
    QCOMPARE(image->redo(), Result::Success);
    QCOMPARE(image->readRgbaPixels(id, corner, 8).bytes, written);
    QCOMPARE(image->readRgbaPixels(id, outside, 8).bytes, written);
}

void KisGpuPaintImageTest::native16RasterCopiesKeepPrecisionPixelsAndWorkingCapacity()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    constexpr auto format = PixelFormat::Rgba16;
    constexpr int tileBytes = RgbaTile::byteCount(format);
    Execution execution(device, 12 * tileBytes, 4, tileBytes);
    RasterImage input;
    input.size = {4, 3};
    input.format = format;
    input.layers = {{QStringLiteral("Native copy"), true, {}, {}}};
    PaintImageSP image;
    try {
        image = PaintImageSP::create(execution, input);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    const auto layer = image->operations().activeLayer();
    const QRect source(-1, 0, 2, 1);
    const QByteArray bytes = rgbaBytes({65534, 1, 257, 0}, format) + rgbaBytes({257, 65534, 1, 65535}, format);
    QCOMPARE(image->writeRgbaPixels(layer, source, bytes), Result::Success);
    const auto pinned = image->operations().snapshot();
    const QUuid id(QStringLiteral("{81828384-8586-8788-898a-8b8c8d8e8f90}"));
    const qsizetype workingBytes = bytes.size() + 2 * tileBytes;
    QVERIFY(!image->cloneRasterLayer(pinned, layer, source, id, workingBytes - 1));
    QCOMPARE(image->operations().snapshot(), pinned);
    const auto clone = image->cloneRasterLayer(pinned, layer, source, id, workingBytes);
    QVERIFY(clone);
    QCOMPARE(clone->operations().snapshot()->format, format);
    QCOMPARE(clone->operations().snapshot()->size, source.size());
    QCOMPARE(clone->readRgbaPixels(clone->operations().activeLayer(), {0, 0, 2, 1}, bytes.size()).bytes, bytes);
    QCOMPARE(clone->clearLayers({clone->operations().activeLayer()}), Result::Success);
    QCOMPARE(image->readRgbaPixels(layer, source, bytes.size()).bytes, bytes);
    QCOMPARE(image->operations().snapshot(), pinned);
}

void KisGpuPaintImageTest::sequentialConnectionsReleaseDevicesAndKeepGpuDescriptorsBounded()
{
#ifdef Q_OS_LINUX
    QVERIFY2(QDir(QStringLiteral("/proc/self/fd")).isReadable(),
             "Linux GPU descriptor accounting requires readable /proc/self/fd");
    const auto gpuDescriptors = [] {
        const QDir descriptors(QStringLiteral("/proc/self/fd"));
        QStringList result;
        const auto entries = descriptors.entryList(QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot);
        for (const auto &entry : entries) {
            const auto path = descriptors.filePath(entry);
            struct stat status{};
            if (::stat(QFile::encodeName(path).constData(), &status) != 0 || !S_ISCHR(status.st_mode)) continue;
            const auto target = QFileInfo(path).symLinkTarget();
            if (target.startsWith(QStringLiteral("/dev/nvidia")) || target.startsWith(QStringLiteral("/dev/dri/")))
                result.append(entry + QStringLiteral(": ") + target);
        }
        return result;
    };
#endif
    QString diagnostic;
    auto warm = Device::createNative(diagnostic);
    QVERIFY2(warm, qPrintable(diagnostic));
    const std::weak_ptr<Device> warmLifetime = warm;
    warm.reset();
    QVERIFY(warmLifetime.expired());
#ifdef Q_OS_LINUX
    const auto baseline = gpuDescriptors();
    qInfo() << "GPU descriptors after warmup:" << baseline;
#endif
    for (int cycle = 1; cycle <= 32; ++cycle) {
        auto device = Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        const std::weak_ptr<Device> lifetime = device;
        device.reset();
        QVERIFY2(lifetime.expired(), "The last connection reference must release its logical Device");
#ifdef Q_OS_LINUX
        const auto remaining = gpuDescriptors();
        qInfo() << "GPU descriptors after connection" << cycle << ':' << remaining;
        QVERIFY2(remaining.size() <= baseline.size(), qPrintable(QStringLiteral(
            "GPU character descriptors grew after connection %1: warmup=%2 remaining=%3")
            .arg(cycle).arg(baseline.size()).arg(remaining.size())));
#endif
    }
}

void KisGpuPaintImageTest::rasterCopiesRetainFixedPixelsAndExecution_data()
{
    QTest::addColumn<bool>("projection");
    QTest::newRow("layer") << false;
    QTest::newRow("canonical-projection") << true;
}

void KisGpuPaintImageTest::rasterCopiesRetainFixedPixelsAndExecution()
{
    QFETCH(bool, projection);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    RasterImage input;
    input.size = {8, 6};
    input.layers = {{QStringLiteral("Source"), true,
        QUuid(QStringLiteral("{c1000001-1122-3344-5566-778899aabbcc}")), {}}};
    QByteArray negative(RgbaTile::ByteCount, '\0'), positive(RgbaTile::ByteCount, '\0');
    negative.replace((255 * 256 + 255) * 4, 4, QByteArray::fromHex("ff221100"));
    positive.replace((1 * 256 + 1) * 4, 4, QByteArray::fromHex("00ff00ff"));
    input.layers.first().tiles = {{{-1, -1}, negative}, {{0, 0}, positive}};
    auto source = PaintImageSP::create(execution, input);
    const auto fixed = source->operations().snapshot();
    QCOMPARE(source->writeRgbaPixels(1, {1, 1, 1, 1}, QByteArray::fromHex("ff0000ff")), Result::Success);
    const auto current = source->operations().snapshot();
    const auto retainedVersion = source->operations().preview();
    const LayerId layer = projection ? 0 : 1;
    const QUuid capturedId(QStringLiteral("{c1000002-1122-3344-5566-778899aabbcc}"));
    QVERIFY(!source->cloneRasterLayer(fixed, layer, {-2, -2, 4, 4}, capturedId, 1));
    auto captured = source->cloneRasterLayer(fixed, layer, {-2, -2, 4, 4}, capturedId, 1024 * 1024);
    QVERIFY(captured);
    QCOMPARE(captured->operations().snapshot()->size, QSize(4, 4));
    QCOMPARE(captured->operations().snapshot()->layers.size(), 1);
    QCOMPARE(captured->operations().snapshot()->layers.first().kind, LayerKind::Paint);
    QCOMPARE(captured->operations().snapshot()->layers.first().uuid, capturedId);
    QCOMPARE(captured->operations().undoCount(), 0);
    QVERIFY(!captured->operations().isModified());
    QCOMPARE(source->operations().snapshot(), current);
    QCOMPARE(source->operations().preview(), retainedVersion);
    QByteArray expected(4 * 4 * 4, '\0');
    expected.replace((1 * 4 + 1) * 4, 4, QByteArray::fromHex("ff221100"));
    expected.replace((3 * 4 + 3) * 4, 4, QByteArray::fromHex("00ff00ff"));
    source.clear();
    execution.reset();
    const auto result = captured->readRgbaPixels(1, {0, 0, 4, 4}, expected.size());
    QCOMPARE(result.code, ExecutionCode::Success);
    QCOMPARE(result.bytes, expected);
    QCOMPARE(captured->replaceRgbaPixels(1, {3, 3, 1, 1}, QByteArray::fromHex("0000ffff")), Result::Success);
    QCOMPARE(captured->undo(), Result::Success);
    QCOMPARE(captured->readRgbaPixels(1, {0, 0, 4, 4}, expected.size()).bytes, expected);
}

void KisGpuPaintImageTest::positionedPixelWritesPreservePixelsAndVersions_data()
{
    QTest::addColumn<QPoint>("position");
    QTest::addColumn<QRect>("region");
    QTest::addColumn<bool>("recordHistory");
    const QList<QPoint> positions{{0, 0}, {7, -9}, {-73, 19}, {257, -513}};
    const QList<QRect> regions{{-8, -4, 80, 64}, {0, 0, 256, 256}};
    for (int p = 0; p < positions.size(); ++p) for (int r = 0; r < regions.size(); ++r) {
        for (const bool recorded : {false, true}) {
            const auto name = QStringLiteral("position-%1-region-%2-%3").arg(p).arg(r).arg(recorded ? "recorded" : "direct").toLatin1();
            QTest::newRow(name.constData()) << positions[p] << regions[r] << recorded;
        }
    }
}

void KisGpuPaintImageTest::positionedPixelWritesPreservePixelsAndVersions()
{
    QFETCH(QPoint, position);
    QFETCH(QRect, region);
    QFETCH(bool, recordHistory);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage source;
    source.size = {64, 48};
    source.layers = {{QStringLiteral("Positioned write"), true,
                      QUuid(QStringLiteral("{a1a2a3a4-a5a6-a7a8-a9aa-abacadaeafb0}")), {}}};
    PaintImage image(execution, source);
    QCOMPARE(image.writeLayerPosition(1, position), Result::Success);
    const auto stroke = image.beginStroke(1, Brush{}, 42);
    QCOMPARE(image.appendSamples(stroke, {{{1000, 1000}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto before = image.operations().snapshot();
    QCOMPARE(image.markSaved(before), Result::Success);
    QByteArray pixels(region.width() * region.height() * 4, '\0');
    for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
        const int offset = (y * region.width() + x) * 4;
        pixels[offset] = char(x + 17);
        pixels[offset + 1] = char(y + 31);
        pixels[offset + 2] = char(x ^ y);
        pixels[offset + 3] = (x + y) % 2 ? char(255) : char(0);
    }
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto written = recordHistory ? image.replaceRgbaPixels(1, region, pixels) : image.writeRgbaPixels(1, region, pixels);
    QCOMPARE(written, Result::Success);
    QCOMPARE(image.operations().undoCount(), 1 + int(recordHistory));
    QCOMPARE(image.operations().isModified(), recordHistory);
    QCOMPARE(changed.size(), int(recordHistory));
    QCOMPARE(image.readRgbaPixels(1, region, pixels.size()).bytes, pixels);
    const QByteArray empty(pixels.size(), '\0');
    QCOMPARE(execution.readRgbaPixels(image.handle(), before, region, 1, pixels.size()).bytes, empty);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, region, pixels.size()).bytes, recordHistory ? empty : pixels);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, region, pixels.size()).bytes, pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const auto stem = QDir(results).filePath(QStringLiteral("positioned-write-") + QString::fromLatin1(QTest::currentDataTag()));
        QFile output(stem + QStringLiteral(".rgba"));
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(output.write(image.readRgbaPixels(1, region, pixels.size()).bytes), pixels.size());
    }
}

void KisGpuPaintImageTest::directPixelWritesFollowCanvasOriginsAcrossHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 48));
    QCOMPARE(image.resizeCanvas({8, 6, 32, 24}, false), Result::Success);
    const auto before = image.operations().snapshot();
    QCOMPARE(image.markSaved(before), Result::Success);
    const auto pixels = QByteArray::fromHex("11223300556677ff").repeated(256 * 256 / 2);
    QCOMPARE(image.writeRgbaPixels(1, {0, 0, 256, 256}, pixels), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.readRgbaPixels(image.handle(), before, {0, 0, 256, 256}, 1, pixels.size()).bytes,
             QByteArray(pixels.size(), '\0'));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->size, QSize(64, 48));
    QCOMPARE(image.operations().snapshot()->layers.first().position, QPoint());
    QCOMPARE(image.readRgbaPixels(1, {8, 6, 256, 256}, pixels.size()).bytes, pixels);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, {0, 0, 256, 256}, pixels.size()).bytes, pixels);
}

void KisGpuPaintImageTest::canvasResizePublishesPinnedSizesAndRejectsIncompleteMoves()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 48));
    const auto pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(1, {8, 8, 1, 1}, pixel), Result::Success);
    const auto fixed = image.operations().snapshot();
    const auto reference = image.retainLayerPixels(1);
    QSignalSpy changed(&image, &PaintImage::changed);
    bool requestedBeforeNotification = false;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedBeforeNotification = execution.requested(image.handle()) == image.operations().snapshot() &&
                                      reference->version() == image.operations().snapshot();
    });
    QCOMPARE(image.resizeCanvas({0, 0, 64, 48}, false), Result::Success);
    QCOMPARE(image.operations().snapshot(), fixed);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.resizeCanvas({8, 6, 32, 24}, true), Result::Success);
    QVERIFY(requestedBeforeNotification);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.readRgbaPixels(1, {0, 2, 1, 1}, 4).bytes, pixel);
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, {8, 8, 1, 1}, 1, 4).bytes, pixel);
    QCOMPARE(fixed->size, QSize(64, 48));
    const auto resized = image.operations().snapshot();
    QCOMPARE(resized->size, QSize(32, 24));
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    changed.clear();
    QCOMPARE(image.resizeCanvas({-1, -1, 64, 48}, false), Result::Busy);
    QCOMPARE(image.operations().snapshot(), resized);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.operations().snapshot()->size, QSize(32, 24));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->size, QSize(64, 48));
    QCOMPARE(image.readRgbaPixels(1, {8, 8, 1, 1}, 4).bytes, pixel);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, {0, 2, 1, 1}, 4).bytes, pixel);

    RasterImage source;
    source.size = {64, 48};
    QByteArray edge(RgbaTile::ByteCount, '\0');
    edge.replace(255 * 4, 4, pixel);
    source.layers = {{QStringLiteral("Edge pixels"), true, {}, {{{32767, 0}, edge}}}};
    PaintImage bounded(execution, source);
    const auto original = bounded.operations().snapshot();
    QCOMPARE(bounded.resizeCanvas({-1, 0, 64, 48}, false), Result::InvalidInput);
    QCOMPARE(bounded.operations().snapshot(), original);
    QCOMPARE(bounded.operations().undoCount(), 0);
    QCOMPARE(bounded.resizeCanvas({-1, 0, 64, 48}, true), Result::Success);
    QCOMPARE(bounded.readRgbaPixels(1, {RgbaTile::CoordinateLimit - 1, 0, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    Execution limited(device, 8 * TileRenderer::TileBytes, 1, 15);
    PaintImage capacity(limited, source);
    const auto unchanged = capacity.operations().snapshot();
    QCOMPARE(capacity.resizeCanvas({-1, 0, 64, 48}, false), Result::CapacityExceeded);
    QCOMPARE(capacity.operations().snapshot(), unchanged);
    QCOMPARE(limited.requested(capacity.handle()), unchanged);
}

void KisGpuPaintImageTest::layerCropPublishesFixedPixelsAndPreservesRejectedRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    const auto pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, {7, 9, 1, 1}, pixel), Result::Success);
    const auto fixed = image.operations().snapshot();
    QSignalSpy changes(&image, &PaintImage::changed);
    bool published = false;
    connect(&image, &PaintImage::changed, this, [&] {
        published = execution.requested(image.handle()) == image.operations().snapshot();
    });
    QCOMPARE(image.cropLayer(layer, {10, 10, 20, 20}), Result::Success);
    QVERIFY(published);
    QCOMPARE(changes.size(), 1);
    const auto cropped = image.operations().snapshot();
    QCOMPARE(image.readRgbaPixels(layer, {7, 9, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, {7, 9, 1, 1}, layer, 4).bytes, pixel);
    QCOMPARE(execution.requested(image.handle()), cropped);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(image.readRgbaPixels(layer, {7, 9, 1, 1}, 4).code, ExecutionCode::Busy);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.readRgbaPixels(layer, {7, 9, 1, 1}, 3).code, ExecutionCode::CapacityExceeded);
    QCOMPARE(execution.requested(image.handle()), cropped);
    QCOMPARE(image.operations().snapshot(), cropped);
    QCOMPARE(image.undo(), Result::Success);
    const auto pending = image.beginStroke(layer, Brush{}, 42);
    QCOMPARE(image.appendSamples(pending, {{{4, 5}, 1}}), Result::Success);
    const auto retainedVersion = image.operations().preview();
    const auto requested = execution.requested(image.handle());
    changes.clear();
    QCOMPARE(image.cropLayer(layer, {1, 2, 3, 4}), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), retainedVersion);
    QCOMPARE(execution.requested(image.handle()), requested);
    QCOMPARE(changes.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
    const auto unchanged = image.operations().preview();
    changes.clear();
    QCOMPARE(image.cropLayer(layer, {-RgbaTile::CoordinateLimit - 1, 0, 1, 1}), Result::InvalidInput);
    QCOMPARE(image.cropLayer(layer + 1, {1, 2, 3, 4}), Result::LayerUnavailable);
    QCOMPARE(image.operations().preview(), unchanged);
    QCOMPARE(changes.size(), 0);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.operations().historyAction(1), HistoryAction::CropLayer);
    QCOMPARE(image.writeRgbaPixels(layer, {1000, 7, 1, 1}, pixel), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {1000, 7, 1, 1}, 4).bytes, pixel);
    image.close();
    QCOMPARE(image.cropLayer(layer, {1, 2, 3, 4}), Result::Closed);
}

void KisGpuPaintImageTest::directWritesKeepTheirCropHistoryBlocks_data()
{
    QTest::addColumn<QPoint>("writePoint");
    QTest::addColumn<QList<QRect>>("crops");
    QTest::addColumn<int>("owningCrop");
    QTest::addColumn<int>("sourceKind");
    QTest::addColumn<QPoint>("translation");
    const QList<QRect> removed{{64, 0, 64, 64}};
    QTest::newRow("unused-far-block") << QPoint(1100, 44) << removed << 0 << 0 << QPoint();
    QTest::newRow("unused-block-in-same-gpu-tile") << QPoint(160, 8) << removed << 0 << 0 << QPoint();
    QTest::newRow("fully-kept-block") << QPoint(16, 8) << QList<QRect>{{0, 0, 64, 64}} << 0 << 0 << QPoint();
    QTest::newRow("removed-block") << QPoint(8, 8) << removed << 1 << 0 << QPoint();
    QTest::newRow("kept-side-of-boundary-block") << QPoint(40, 8) << QList<QRect>{{32, 0, 32, 64}} << 1 << 0 << QPoint();
    QTest::newRow("explicit-default-block") << QPoint(8, 8) << removed << 1 << 2 << QPoint();
    QTest::newRow("painted-block") << QPoint(8, 8) << removed << 1 << 1 << QPoint();
    QTest::newRow("previously-removed-block") << QPoint(8, 8) << QList<QRect>{{64, 0, 64, 64}, {128, 0, 64, 64}} << 1 << 0 << QPoint();
    QTest::newRow("retained-default-boundary-block") << QPoint(40, 8) << QList<QRect>{{32, 0, 32, 64}, {64, 0, 64, 64}} << 2 << 0 << QPoint();
    QTest::newRow("translated-removed-block") << QPoint(-65, 27) << removed << 1 << 0 << QPoint(-73, 19);
    QTest::newRow("translated-unused-block") << QPoint(-400, 27) << removed << 0 << 0 << QPoint(-73, 19);
    QTest::newRow("paint-after-non-aligned-position") << QPoint(40, 8) << removed << 0 << 3 << QPoint();
    QTest::newRow("fill-after-non-aligned-position") << QPoint(40, 8) << removed << 0 << 4 << QPoint();
    QTest::newRow("mirrored-paint-after-non-aligned-position") << QPoint(40, 8) << removed << 0 << 5 << QPoint();
}

void KisGpuPaintImageTest::directWritesKeepTheirCropHistoryBlocks()
{
    QFETCH(QPoint, writePoint);
    QFETCH(QList<QRect>, crops);
    QFETCH(int, owningCrop);
    QFETCH(int, sourceKind);
    QFETCH(QPoint, translation);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    if (sourceKind >= 3) QCOMPARE(image.writeLayerPosition(layer, {17, 0}), Result::Success);
    if (sourceKind == 4) {
        FigureStyle style;
        style.outline = false;
        style.fill = RgbaPixel{17, 34, 51, 255};
        QCOMPARE(image.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{16, 16}, 1}},
                                   {ShapeKind::Rectangle, {0, 0}, {16, 16}, 0}, style, 42), Result::Success);
    } else if (sourceKind == 1 || sourceKind >= 3) {
        Brush brush;
        brush.diameter = 4;
        brush.mirrorHorizontal = sourceKind == 5;
        brush.mirrorCenter = {64, 0};
        const auto stroke = image.beginStroke(layer, brush, 42);
        QCOMPARE(image.appendSamples(stroke, {{{8, 8}, 1}}), Result::Success);
        QCOMPARE(image.commitStroke(stroke), Result::Success);
    } else {
        const auto pixel = sourceKind == 2 ? QByteArray(4, '\0') : QByteArray::fromHex("112233ff");
        QCOMPARE(image.writeRgbaPixels(layer, {8, 8, 1, 1}, pixel), Result::Success);
    }
    if (!translation.isNull()) {
        const auto move = image.beginLayerTranslation(layer);
        QCOMPARE(image.updateLayerTranslation(move, translation), Result::Success);
        QCOMPARE(image.commitLayerTranslation(move), Result::Success);
    }
    const auto baseHistory = image.operations().undoCount();
    const QRect region(writePoint, QSize(1, 1));
    QList<QByteArray> before{image.readRgbaPixels(layer, region, 4).bytes};
    for (const auto &crop : crops) {
        QCOMPARE(image.cropLayer(layer, crop), Result::Success);
        before.append(image.readRgbaPixels(layer, region, 4).bytes);
    }
    const auto fixed = image.operations().snapshot();
    const auto written = QByteArray::fromHex("77889900");
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeRgbaPixels(layer, region, written), Result::Success);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.operations().undoCount(), baseHistory + crops.size());
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, region, layer, 4).bytes, before.last());
    for (qsizetype index = crops.size(); index >= 0; --index) {
        QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes,
                 owningCrop == 0 || index >= owningCrop ? written : before[index]);
        QCOMPARE(image.operations().undoCount(), baseHistory + index);
        if (index) QCOMPARE(image.undo(), Result::Success);
    }
    for (qsizetype index = 1; index <= crops.size(); ++index) {
        QCOMPARE(image.redo(), Result::Success);
        QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes,
                 owningCrop == 0 || index >= owningCrop ? written : before[index]);
    }
}

void KisGpuPaintImageTest::rejectedTranslationKeepsHiddenPixelsHistoryPreviewAndRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QByteArray pixels(RgbaTile::ByteCount, '\0');
    pixels.replace((7 * 256 + 255) * 4, 4, QByteArray::fromHex("11223300"));
    RasterImage input;
    input.size = {32, 16};
    input.layers = {{QStringLiteral("Hidden edge pixel"), true, {}, {{{32767, 0}, pixels}}}};
    Execution execution(device);
    PaintImage image(execution, input);
    const auto fixed = image.operations().snapshot();
    const auto reference = image.retainLayerPixels(1);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeLayerPosition(1, {1, 0}), Result::InvalidInput);
    QCOMPARE(image.operations().snapshot(), fixed);
    QCOMPARE(execution.requested(image.handle()), fixed);
    const auto token = image.beginLayerTranslation(1);
    QVERIFY(!token.isNull());
    QCOMPARE(image.updateLayerTranslation(token, {1, 0}), Result::InvalidInput);
    QCOMPARE(image.operations().preview(), fixed);
    QCOMPARE(reference->version(), fixed);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.updateLayerTranslation(token, {-1, 0}), Result::Success);
    const auto preview = image.operations().preview();
    QCOMPARE(image.updateLayerTranslation(token, {1, 0}), Result::InvalidInput);
    QCOMPARE(image.operations().preview(), preview);
    QCOMPARE(execution.requested(image.handle()), preview);
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.cancelLayerTranslation(token), Result::Success);
    QCOMPARE(reference->version(), fixed);
    QCOMPARE(image.readRgbaPixels(1, {8388607, 7, 1, 1}, 4).bytes, QByteArray::fromHex("11223300"));
    Execution bounded(device, 8 * TileRenderer::TileBytes, 1, 15);
    PaintImage constrained(bounded, input);
    const auto unchanged = constrained.operations().snapshot();
    const auto move = constrained.beginLayerTranslation(1);
    QVERIFY(!move.isNull());
    QCOMPARE(constrained.updateLayerTranslation(move, {-1, 0}), Result::CapacityExceeded);
    QCOMPARE(constrained.operations().preview(), unchanged);
    QCOMPARE(bounded.requested(constrained.handle()), unchanged);
    QCOMPARE(constrained.cancelLayerTranslation(move), Result::Success);
    const auto busyMove = image.beginLayerTranslation(1);
    QVERIFY(!busyMove.isNull());
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(image.updateLayerTranslation(busyMove, {-1, 0}), Result::Busy);
    QCOMPARE(image.operations().preview(), fixed);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.cancelLayerTranslation(busyMove), Result::Success);
}

void KisGpuPaintImageTest::affinePreviewPreservesCanonicalPixelsOnCancelIdentityAndRejection()
{
    QString diagnostic; const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 32 * RgbaTile::ByteCount);
    PaintImage image(execution, QSize(32, 32));
    const auto layer = image.operations().activeLayer();
    const QByteArray pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, {1, 2, 1, 1}, pixel), Result::Success);
    const auto before = image.operations().snapshot();
    const auto token = image.beginAffineTransform({{layer, true, false}}); QVERIFY(!token.isNull());
    auto plan = QSharedPointer<AffinePlan>::create(); plan->filterId = QStringLiteral("NearestNeighbor");
    AffinePass pass; pass.kind = AffinePass::Remap; pass.mapWholeLayer = true; pass.map = {1, 0, 0, 1, 7, -5}; plan->passes.append(pass); plan->positionDelta = {7, -5};
    QCOMPARE(image.updateAffineTransform(token, {{layer, plan, {}}}), Result::Success);
    const auto preview = image.operations().preview();
    QCOMPARE(execution.readRgbaPixels(image.handle(), preview, {8, -3, 1, 1}, layer, 4).bytes, pixel);
    auto invalid = QSharedPointer<AffinePlan>::create(*plan); invalid->passes[0].map[0] = 2;
    QCOMPARE(image.updateAffineTransform(token, {{layer, invalid, {}}}), Result::InvalidInput);
    QCOMPARE(image.operations().preview(), preview); QCOMPARE(execution.requested(image.handle()), preview);
    QCOMPARE(image.cancelAffineTransform(token), Result::Success);
    QCOMPARE(image.operations().snapshot(), before); QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.readRgbaPixels(layer, {1, 2, 1, 1}, 4).bytes, pixel);
    const auto again = image.beginAffineTransform({{layer, true, false}}); QVERIFY(!again.isNull());
    QCOMPARE(image.updateAffineTransform(again, {{layer, plan, {}}}), Result::Success);
    auto identity = QSharedPointer<AffinePlan>::create(); identity->filterId = plan->filterId;
    QCOMPARE(image.updateAffineTransform(again, {{layer, identity, {}}}), Result::Success);
    QCOMPARE(image.operations().preview(), before);
    QCOMPARE(image.commitAffineTransform(again), Result::Success);
    QCOMPARE(image.operations().snapshot(), before); QCOMPARE(image.operations().undoCount(), 0);
}

void KisGpuPaintImageTest::affineResamplingReadsMovedSourceTilesAcrossCanonicalBoundaries()
{
    QString diagnostic; const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 64 * RgbaTile::ByteCount, 4, RgbaTile::ByteCount);
    const auto pixel = QByteArray::fromHex("112233ff");
    RasterImage input; input.size = {512, 512};
    input.layers = {{QStringLiteral("Moved transform"), true, {}, {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}}}};
    PaintImage image(execution, input); const auto layer = image.operations().activeLayer();
    const auto move = image.beginLayerTranslation(layer); QVERIFY(!move.isNull());
    QCOMPARE(image.updateLayerTranslation(move, {1, 1}), Result::Success); QCOMPARE(image.commitLayerTranslation(move), Result::Success);
    auto kernel = QSharedPointer<AffineKernel>::create(); kernel->positionScale = 128; kernel->support = .5;
    for (int i = 0; i < 256; ++i) { kernel->phases[i] = {i, 1, 0}; kernel->weights.append(255); }
    auto plan = QSharedPointer<AffinePlan>::create(); plan->filterId = QStringLiteral("NearestNeighbor");
    const auto horizontal = affineResamplePass(AffinePass::Horizontal, {1, 1, 256, 256}, .5, 0, 0, kernel); QVERIFY(horizontal);
    const auto vertical = affineResamplePass(AffinePass::Vertical, horizontal->destination, .5, 0, 0, kernel); QVERIFY(vertical);
    plan->passes = {*horizontal, *vertical};
    const auto token = image.beginAffineTransform({{layer, true, false}}); QVERIFY(!token.isNull());
    QCOMPARE(image.updateAffineTransform(token, {{layer, plan, {}}}), Result::Success);
    const auto actual = execution.readRgbaPixels(image.handle(), image.operations().preview(), {10, 10, 1, 1}, layer, 4);
    QVERIFY2(actual.code == ExecutionCode::Success, qPrintable(actual.diagnostic)); QCOMPARE(actual.bytes, pixel);
    QCOMPARE(image.commitAffineTransform(token), Result::Success); QCOMPARE(image.operations().undoCount(), 2);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {256, 256, 1, 1}, 4).bytes, pixel);
}

void KisGpuPaintImageTest::nativeBoundsIgnoreSubByteImplicitAlpha()
{
    QString diagnostic; const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input; input.size = {32, 32}; input.format = PixelFormat::Rgba16;
    input.layers = {{QStringLiteral("Native alpha bounds"), true, {}, {}}}; input.layers.first().defaultPixel = {1, 257, 65534, 1};
    PaintImage image(execution, input); const auto layer = image.operations().activeLayer();
    const QRect area(4, 5, 1, 1);
    QCOMPARE(image.writeRgbaPixels(layer, area, rgbaBytes({3, 256, 32768, 127}, input.format)), Result::Success);
    const auto native = execution.readBounds(image.handle(), image.operations().snapshot(), layer, false, true);
    QCOMPARE(native.code, ExecutionCode::Success); QCOMPARE(*native.bounds, QRect{});
    const auto raw = execution.readBounds(image.handle(), image.operations().snapshot(), layer, true);
    QCOMPARE(raw.code, ExecutionCode::Success); QCOMPARE(*raw.bounds, area);
    QCOMPARE(image.writeRgbaPixels(layer, area, rgbaBytes({3, 256, 32768, 128}, input.format)), Result::Success);
    const auto visible = execution.readBounds(image.handle(), image.operations().snapshot(), layer, false, true);
    QCOMPARE(visible.code, ExecutionCode::Success); QCOMPARE(*visible.bounds, area);
}

void KisGpuPaintImageTest::selectedMoveUsesNativeEmptyDetectionAndKeepsLowAlphaPixels()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster; raster.size = {32, 32}; raster.format = PixelFormat::Rgba16;
    raster.layers = {{QStringLiteral("Selected"), true, QUuid::createUuid(), {}}};
    PaintImage image(execution, raster);
    const auto layer = image.operations().activeLayer();
    const auto low = rgbaBytes({4353, 33282, 61955, 1}, raster.format);
    QCOMPARE(image.writeRgbaPixels(layer, {12, 9, 1, 1}, low), Result::Success);
    QCOMPARE(image.setSelectionRect({0, 0, 32, 32}), Result::Success);
    const auto initial = image.operations().snapshot();
    QVERIFY(image.beginSelectionTranslation(layer, {0, 0, 32, 32}, {0, 0, 32, 32}).isNull());
    QCOMPARE(image.operations().snapshot(), initial); QVERIFY(!image.operations().hasActiveStroke());
    const auto visible = rgbaBytes({65432, 271, 17, 128}, raster.format);
    QCOMPARE(image.writeRgbaPixels(layer, {5, 7, 1, 1}, visible), Result::Success);
    const auto token = image.beginSelectionTranslation(layer, {0, 0, 32, 32}, {0, 0, 32, 32}); QVERIFY(!token.isNull());
    QCOMPARE(image.selectionTranslationBounds(token), QRect(5, 7, 1, 1));
    QCOMPARE(image.updateLayerTranslation(token, {1, 1}), Result::Success);
    const auto preview = execution.readRgbaPixels(image.handle(), image.operations().preview(), {13, 10, 1, 1}, layer, 8);
    QCOMPARE(preview.code, ExecutionCode::Success); QCOMPARE(preview.bytes, low);
    QCOMPARE(image.cancelLayerTranslation(token), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {12, 9, 1, 1}, 8).bytes, low);
}

void KisGpuPaintImageTest::erasedMasksRemainTransparentAndAcceptFurtherPainting_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("groupOwner");
    QTest::newRow("u8") << false << false;
    QTest::newRow("u16") << true << false;
    QTest::newRow("group-u8") << false << true;
    QTest::newRow("group-u16") << true << true;
}

void KisGpuPaintImageTest::erasedMasksRemainTransparentAndAcceptFurtherPainting()
{
    QFETCH(bool, words); QFETCH(bool, groupOwner);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster; raster.size = {64, 64}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    RasterLayer layer{QStringLiteral("Masked"), true, QUuid::createUuid(), {}};
    layer.defaultPixel = {17, 129, 241, quint16(maximumComponent(raster.format))};
    layer.transparencyMask = SelectionMask::fromRectangle({0, 0, 64, 64}, {0, 0, 64, 64});
    if (groupOwner) {
        RasterLayer child{QStringLiteral("Child paint"), true, QUuid::createUuid(), {}};
        child.parentUuid = layer.uuid; child.defaultPixel = layer.defaultPixel;
        layer.kind = LayerKind::Group; layer.defaultPixel = {};
        raster.layers = {layer, child};
    } else raster.layers = {layer};
    PaintImage image(execution, raster);
    const auto layerId = image.operations().activeLayer();
    const auto initial = image.operations().snapshot();
    Brush erase; erase.rgba = {0, 0, 0, 255}; erase.diameter = 256; erase.blendMode = LayerBlendMode::Erase;
    const auto stroke = image.beginTransparencyMaskStroke(layerId, erase, 7); QVERIFY(!stroke.isNull());
    QCOMPARE(image.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto erased = image.operations().snapshot();
    QVERIFY(erased->layers.first().transparencyMask); QVERIFY(erased->layers.first().transparencyMask->isEmpty());
    QCOMPARE(erased->layers.first().defaultPixel, layer.defaultPixel);
    const auto projection = execution.readRgbaPixels(image.handle(), erased, {0, 0, 64, 64}, 0, 64 * 64 * pixelByteCount(raster.format));
    QCOMPARE(projection.code, ExecutionCode::Success); QCOMPARE(projection.bytes, QByteArray(projection.bytes.size(), '\0'));
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.layerTransparencyMask(layerId), initial->layers.first().transparencyMask);
    QCOMPARE(image.redo(), Result::Success); QVERIFY(image.layerTransparencyMask(layerId)->isEmpty());
    Brush paint; paint.rgba = {255, 255, 255, 255}; paint.diameter = 8;
    const auto next = image.beginTransparencyMaskStroke(layerId, paint, 11); QVERIFY(!next.isNull());
    QCOMPARE(image.appendSamples(next, {{{24, 24}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(next), Result::Success);
    QVERIFY(image.layerTransparencyMask(layerId)->isGpuGenerated());
    const auto coverage = execution.readSelectionPixels(image.layerTransparencyMask(layerId), QRect(0, 0, 64, 64), 64 * 64);
    QCOMPARE(coverage.code, ExecutionCode::Success);
    QVERIFY(quint8(coverage.bytes[24 * 64 + 24]) > 0);
    QCOMPARE(quint8(coverage.bytes[0]), quint8(0));
    QCOMPARE(image.undo(), Result::Success); QVERIFY(image.layerTransparencyMask(layerId)->isEmpty());
    const auto pinned = execution.readRgbaPixels(image.handle(), erased, {0, 0, 1, 1}, 0, pixelByteCount(raster.format));
    QCOMPARE(pinned.code, ExecutionCode::Success); QCOMPARE(pinned.bytes, QByteArray(pixelByteCount(raster.format), '\0'));
    const auto primitives = image.beginTransparencyMaskStroke(layerId, paint, 13);
    QVERIFY(!primitives.isNull());
    InputSample tap{{48, 48}, 1}; tap.timestamp = 20;
    InputSample first{{16, 16}, 1}, last{{24, 24}, 1}; last.timestamp = 10;
    QCOMPARE(image.appendPoint(primitives, tap), Result::Success);
    QCOMPARE(image.appendLine(primitives, first, last), Result::Success);
    QCOMPARE(image.commitStroke(primitives), Result::Success);
    const auto painted = image.operations().snapshot();
    QCOMPARE(painted->layers.first().operations, erased->layers.first().operations);
    QCOMPARE(painted->layers.first().defaultPixel, erased->layers.first().defaultPixel);
    const auto primitiveCoverage = execution.readSelectionPixels(image.layerTransparencyMask(layerId), {0, 0, 64, 64}, 64 * 64);
    QCOMPARE(primitiveCoverage.code, ExecutionCode::Success);
    QVERIFY(quint8(primitiveCoverage.bytes[48 * 64 + 48]) > 0);
    QVERIFY(quint8(primitiveCoverage.bytes[20 * 64 + 20]) > 0);
    QCOMPARE(quint8(primitiveCoverage.bytes[40 * 64 + 40]), quint8(0));
    QCOMPARE(image.undo(), Result::Success); QVERIFY(image.layerTransparencyMask(layerId)->isEmpty());
    QCOMPARE(image.redo(), Result::Success); QCOMPARE(image.operations().snapshot(), painted);
    QCOMPARE(image.undo(), Result::Success);
    const auto batch = image.beginTransparencyMaskStroke(layerId, paint, 13);
    QVERIFY(!batch.isNull());
    const auto beforeBatch = image.operations().preview();
    QSignalSpy batchChanges(&image, &PaintImage::changed);
    QCOMPARE(image.appendPainterPrimitives(batch, {tap, first, last},
        {PathCommand::Point, PathCommand::MoveTo, PathCommand::LineTo}), Result::Success);
    QCOMPARE(batchChanges.size(), 1);
    QCOMPARE(image.operations().snapshot(), erased);
    QVERIFY(image.operations().preview() != beforeBatch);
    const auto batchPreview = image.operations().preview();
    const auto batchCoverage = execution.readSelectionPixels(batchPreview->layers.first().transparencyMask, {0, 0, 64, 64}, 64 * 64);
    QCOMPARE(batchCoverage.code, ExecutionCode::Success);
    QCOMPARE(batchCoverage.bytes, primitiveCoverage.bytes);
    QCOMPARE(image.appendPainterPrimitives(batch, {last, first},
        {PathCommand::MoveTo, PathCommand::LineTo}), Result::InvalidInput);
    QCOMPARE(image.operations().preview(), batchPreview);
    QCOMPARE(execution.requested(image.handle()), batchPreview);
    QCOMPARE(image.commitStroke(batch), Result::Success);
    const auto batched = image.operations().snapshot();
    QCOMPARE(batched->layers.first().operations, erased->layers.first().operations);
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), erased);
    QCOMPARE(image.redo(), Result::Success); QCOMPARE(image.operations().snapshot(), batched);
}

void KisGpuPaintImageTest::defaultMasksKeepGroupProjectionAndCoordinateEdits()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        const quint16 scale = format == PixelFormat::Rgba16 ? 257 : 1;
        const RgbaPixel color{quint16(19 * scale), quint16(211 * scale), quint16(97 * scale), quint16(255 * scale)};
        for (quint8 coverage : {quint8(0), quint8(17), quint8(255)}) {
            RasterImage input; input.size = {64, 64}; input.format = format;
            RasterLayer group{QStringLiteral("Group"), true, QUuid::createUuid(), {}}; group.kind = LayerKind::Group;
            RasterLayer child{QStringLiteral("Masked"), true, QUuid::createUuid(), {}};
            child.parentUuid = group.uuid; child.defaultPixel = color;
            child.transparencyMask = SelectionMask::uniform(coverage);
            input.layers = {group, child};
            PaintImage image(execution, input);
            auto expected = color;
            expected[3] = quint16(coverage * scale);
            if (!coverage) expected = {};
            QCOMPARE(image.operations().snapshot()->projectionDefaultPixels().value(1), expected);
            const auto pixels = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {7, 9, 1, 1}, 0, pixelByteCount(format));
            QCOMPARE(pixels.code, ExecutionCode::Success); QCOMPARE(pixels.bytes, rgbaBytes(expected, format));
        }
        for (quint8 coverage : {quint8(17), quint8(255)}) {
            SelectionTileMap tiles;
            QByteArray values(SelectionMask::AlphaTileBytes, char(coverage));
            values[12 * 256 + 14] = 0;
            values[20 * 256 + 22] = char(128);
            tiles.insert({0, 0}, values);
            const auto mask = SelectionMask::fromAlphaTiles(tiles, coverage); QVERIFY(mask);
            RasterImage input; input.size = {64, 64}; input.format = format;
            input.layers = {{QStringLiteral("Coordinate edits")}};
            input.layers.first().defaultPixel = color; input.layers.first().transparencyMask = mask;
            PaintImage image(execution, input);
            const auto read = [&](const SelectionMaskSP &value, QPoint point) {
                return execution.readSelectionPixels(value, QRect(point, QSize(1, 1)), 1).bytes;
            };
            const auto original = image.operations().snapshot();
            Brush unchanged; unchanged.diameter = 2; unchanged.opacity = 0;
            const auto stroke = image.beginTransparencyMaskStroke(1, unchanged, 37);
            QVERIFY2(!stroke.isNull(), qPrintable(image.lastRequestOutcome().diagnostic));
            QCOMPARE(image.appendSamples(stroke, {{{50, 50}, 1}}), Result::Success);
            QCOMPARE(read(image.operations().preview()->layers.first().transparencyMask, {14, 12}), QByteArray(1, '\0'));
            QCOMPARE(image.cancelStroke(stroke), Result::Success);
            QCOMPARE(image.operations().snapshot(), original);
            const auto move = image.beginLayerTranslation(QList<LayerTranslationTarget>{{1, false, true}});
            QVERIFY(!move.isNull());
            QCOMPARE(image.updateLayerTranslation(move, {3, -2}), Result::Success);
            QCOMPARE(read(image.operations().preview()->layers.first().transparencyMask, {17, 10}), QByteArray(1, '\0'));
            QCOMPARE(image.cancelLayerTranslation(move), Result::Success);
            QCOMPARE(image.operations().snapshot(), original);
            const auto committed = image.beginLayerTranslation(QList<LayerTranslationTarget>{{1, false, true}});
            QCOMPARE(image.updateLayerTranslation(committed, {3, -2}), Result::Success);
            QCOMPARE(image.commitLayerTranslation(committed), Result::Success);
            QCOMPARE(image.layerTransparencyMask(1)->defaultCoverage(), coverage);
            QCOMPARE(read(image.layerTransparencyMask(1), {17, 10}), QByteArray(1, '\0'));
            QCOMPARE(read(image.layerTransparencyMask(1), {25, 18}), QByteArray(1, char(128)));
            QCOMPARE(read(image.layerTransparencyMask(1), {100000, -100000}), QByteArray(1, char(coverage)));
            QCOMPARE(image.operations().snapshot()->layers.first().defaultPixel, color);
            QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), original);
            QCOMPARE(image.redo(), Result::Success);
            AffinePlan plan;
            AffinePass pass; pass.kind = AffinePass::Remap;
            pass.source = image.layerTransparencyMask(1)->bounds();
            pass.destination = pass.source.translated(-5, 4);
            pass.map = {1, 0, 0, 1, -5, 4}; pass.mapWholeLayer = true;
            plan.positionDelta = {-5, 4};
            plan.passes = {pass};
            const auto affine = image.transformedSelectionMask(image.layerTransparencyMask(1), plan);
            QVERIFY2(affine, qPrintable(image.lastRequestOutcome().diagnostic));
            QCOMPARE(affine->defaultCoverage(), coverage);
            QCOMPARE(read(affine, {12, 14}), QByteArray(1, '\0'));
            QCOMPARE(read(affine, {20, 22}), QByteArray(1, char(128)));
            QCOMPARE(read(affine, {100000, -100000}), QByteArray(1, char(coverage)));
            const auto beforeCapture = image.operations().snapshot();
            const auto transformedPart = image.transformedTransparencyMask(image.layerTransparencyMask(1),
                SelectionMask::fromRectangle({0, 0, 32, 32}, {0, 0, 64, 64}), false, {0, 0, 32, 32}, plan);
            QVERIFY2(transformedPart, qPrintable(image.lastRequestOutcome().diagnostic));
            QCOMPARE(transformedPart->defaultCoverage(), coverage);
            QCOMPARE(image.operations().snapshot(), beforeCapture);
            QCOMPARE(image.cropLayer(1, {0, 0, 20, 20}), Result::Success);
            QCOMPARE(read(image.layerTransparencyMask(1), {17, 10}), QByteArray(1, '\0'));
            QCOMPARE(read(image.layerTransparencyMask(1), {25, 18}), QByteArray(1, char(coverage)));
            QCOMPARE(image.layerTransparencyMask(1)->defaultCoverage(), coverage);
            QCOMPARE(image.resizeCanvas({2, 1, 80, 80}, false), Result::Success);
            QCOMPARE(read(image.layerTransparencyMask(1), {15, 9}), QByteArray(1, '\0'));
            QCOMPARE(read(image.layerTransparencyMask(1), {100000, -100000}), QByteArray(1, char(coverage)));
        }
        for (bool inverted : {false, true}) {
            RasterImage input; input.size = {64, 64}; input.format = format;
            input.layers = {{QStringLiteral("Selection to mask")}};
            PaintImage image(execution, input);
            QCOMPARE(image.setSelectionRect({-7, 9, 20, 22}, inverted), Result::Success);
            const auto before = image.operations().snapshot();
            QCOMPARE(image.addTransparencyMask(1), Result::Success);
            const auto mask = image.layerTransparencyMask(1); QVERIFY(mask);
            QCOMPARE(mask->defaultCoverage(), quint8(inverted ? 255 : 0));
            QCOMPARE(execution.readSelectionPixels(mask, {-5, 12, 1, 1}, 1).bytes, QByteArray(1, char(inverted ? 0 : 255)));
            QCOMPARE(execution.readSelectionPixels(mask, {100000, -100000, 1, 1}, 1).bytes, QByteArray(1, char(inverted ? 255 : 0)));
            QVERIFY(image.operations().snapshot()->selection.isEmpty());
            const auto added = image.operations().snapshot();
            QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), before);
            QCOMPARE(image.redo(), Result::Success); QCOMPARE(image.operations().snapshot(), added);
        }
    }
}

void KisGpuPaintImageTest::paintedMasksKeepSparseGpuCoverageAfterClosure()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    SelectionTileMap tiles;
    QByteArray alpha(SelectionMask::AlphaTileBytes, '\0');
    for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x)
        if (QRect(3, 5, 29, 31).contains(x, y)) alpha[y * 256 + x] = char(1 + (x + y * 3) % 255);
    tiles.insert({0, 0}, alpha); tiles.insert({4000, 5000}, alpha);
    const auto original = SelectionMask::fromAlphaTiles(tiles);
    QVERIFY(original);
    const QRect far(QPoint(4000, 5000) * 256, QSize(256, 256));
    SelectionMaskSP painted;
    QByteArray nearPixels;
    {
        RasterImage raster; raster.size = {64, 64};
        raster.layers = {{QStringLiteral("Sparse masked layer"), true, {}, {}}};
        raster.layers.first().defaultPixel = {17, 129, 241, 255};
        raster.layers.first().transparencyMask = original;
        PaintImage image(execution, raster);
        Brush erase; erase.diameter = 8; erase.erase = true;
        const auto before = image.operations().snapshot();
        const auto cancelled = image.beginTransparencyMaskStroke(1, erase, 51); QVERIFY(!cancelled.isNull());
        QCOMPARE(image.appendSamples(cancelled, {{{17, 19}, 1}, {{18, 19}, 1}}), Result::Success);
        QCOMPARE(image.operations().snapshot(), before); QCOMPARE(image.operations().undoCount(), 0);
        const auto preview = image.operations().preview(); QVERIFY(preview != before);
        QCOMPARE(execution.requested(image.handle()), preview);
        const auto previewPixels = execution.readSelectionPixels(preview->layers.first().transparencyMask, {17, 19, 1, 1}, 1);
        QCOMPARE(previewPixels.code, ExecutionCode::Success); QCOMPARE(previewPixels.bytes, QByteArray(1, '\0'));
        QCOMPARE(image.cancelStroke(cancelled), Result::Success);
        QCOMPARE(image.operations().preview(), before); QCOMPARE(execution.requested(image.handle()), before);
        QVERIFY(!image.operations().hasActiveStroke());
        const auto stroke = image.beginTransparencyMaskStroke(1, erase, 53);
        QVERIFY(!stroke.isNull());
        QCOMPARE(image.appendSamples(stroke, {{{17, 19}, 1}}), Result::Success);
        const auto active = execution.beginReadbackFrame(); QCOMPARE(active.code, ExecutionCode::Success);
        QCOMPARE(image.commitStroke(stroke), Result::EditInProgress);
        QVERIFY(image.operations().hasActiveStroke()); QCOMPARE(image.operations().snapshot(), before);
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(image.commitStroke(stroke), Result::Success);
        painted = image.layerTransparencyMask(1);
        QVERIFY(painted && painted->isGpuGenerated());
        QCOMPARE(painted->tileCoordinates().size(), 2);
        QVERIFY(painted->tiles().isEmpty());
        QCOMPARE(execution.readSelectionPixels(painted, far, alpha.size()).bytes, alpha);
        const auto near = execution.readSelectionPixels(painted, QRect(0, 0, 256, 256), alpha.size());
        QCOMPARE(near.code, ExecutionCode::Success);
        nearPixels = near.bytes;
        QVERIFY(nearPixels != alpha);
        QCOMPARE(quint8(nearPixels[19 * 256 + 17]), quint8(0));
        QCOMPARE(image.undo(), Result::Success);
        QCOMPARE(image.layerTransparencyMask(1), original);
        QCOMPARE(image.redo(), Result::Success);
        QCOMPARE(image.layerTransparencyMask(1), painted);
        image.close();
    }
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
    QCOMPARE(execution.readSelectionPixels(painted, far, alpha.size()).bytes, alpha);
    const auto replacement = Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QCOMPARE(execution.readSelectionPixels(painted, far, alpha.size()).bytes, alpha);
    QCOMPARE(execution.readSelectionPixels(painted, QRect(0, 0, 256, 256), alpha.size()).bytes, nearPixels);
}

void KisGpuPaintImageTest::sparsePaintedMasksMoveCombineCropAndRecover()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    SelectionTileMap tiles;
    QByteArray alpha(SelectionMask::AlphaTileBytes, '\0');
    for (int y = 240; y < 256; ++y) for (int x = 240; x < 256; ++x)
        alpha[y * 256 + x] = char(1 + (x + y * 3) % 255);
    tiles.insert({-2, -3}, alpha); tiles.insert({4000, 5000}, alpha);
    RasterImage raster; raster.size = {64, 64};
    raster.layers = {{QStringLiteral("Sparse moving mask"), true, {}, {}}};
    raster.layers.first().transparencyMask = SelectionMask::fromAlphaTiles(tiles);
    QVERIFY(raster.layers.first().transparencyMask);
    PaintImage image(execution, raster);
    Brush erase; erase.diameter = 4; erase.erase = true;
    const auto stroke = image.beginTransparencyMaskStroke(1, erase, 17); QVERIFY(!stroke.isNull());
    QCOMPARE(image.appendSamples(stroke, {{{-266, -522}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto painted = image.layerTransparencyMask(1); QVERIFY(painted->isGpuGenerated());
    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        const auto read = execution.readSelectionPixels(painted, QRect(QPoint(tile.key().first, tile.key().second) * 256,
            QSize(256, 256)), alpha.size());
        QCOMPARE(read.code, ExecutionCode::Success); tile.value() = read.bytes;
    }
    const auto literal = SelectionMask::fromAlphaTiles(tiles); QVERIFY(literal);
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    const auto matches = [&](const SelectionMaskSP &actual, const SelectionMaskSP &expected) {
        if (!actual || !expected || actual->bounds() != expected->bounds()) return false;
        auto points = actual->tileCoordinates(); points.append(expected->tileCoordinates());
        for (const auto point : points) {
            const auto read = execution.readSelectionPixels(actual, QRect(point * 256, QSize(256, 256)), alpha.size());
            if (read.code != ExecutionCode::Success) return false;
            for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x)
                if (quint8(read.bytes[y * 256 + x]) != expected->coverage(point.x() * 256 + x, point.y() * 256 + y)) return false;
        }
        return true;
    };
    SelectionMaskSP replayed, replayedLiteral;
    for (const QPoint delta : {QPoint(13, -7), QPoint(-271, 263)}) {
        const auto moved = image.translatedSelectionMask(painted, delta, domain);
        const auto expected = literal->translated(delta, domain);
        QVERIFY2(matches(moved, expected), qPrintable(image.lastRequestOutcome().diagnostic));
        QVERIFY(moved->tiles().isEmpty());
        QCOMPARE(image.setSelectionMask(painted), Result::Success);
        for (const auto action : {SelectionCombination::Add, SelectionCombination::Subtract,
                                 SelectionCombination::Intersect, SelectionCombination::SymmetricDifference}) {
            QCOMPARE(image.setSelectionMask(painted), Result::Success);
            const auto before = image.operations().snapshot();
            const auto combinedResult = image.combineSelectionMask(moved, action);
            QVERIFY2(combinedResult == Result::Success, qPrintable(QStringLiteral("action %1, result %2: %3")
                .arg(int(action)).arg(int(combinedResult)).arg(image.lastRequestOutcome().diagnostic)));
            const auto combined = image.selectionMask() ? image.selectionMask() : SelectionMask::empty();
            const auto expectedCombined = literal->combined(*expected, action, domain);
            QVERIFY(matches(combined, expectedCombined));
            QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), before);
            QCOMPARE(image.redo(), Result::Success);
            replayed = combined; replayedLiteral = expectedCombined;
        }
    }
    // Transform-tool mask capture/clear/merge retain their inputs across
    // document closure and device replacement; native window tests fix math.
    const auto captured = painted;
    AffinePlan plan;
    AffinePass pass; pass.kind = AffinePass::Remap;
    pass.source = captured->bounds(); pass.destination = pass.source.translated(13, -7);
    pass.map = {1, 0, 0, 1, 13, -7}; pass.mapWholeLayer = true;
    plan.passes = {pass};
    const auto shifted = image.transformedSelectionMask(captured, plan); QVERIFY(shifted);
    const auto movedCapture = image.translatedSelectionMask(captured, {13, -7}, domain); QVERIFY(movedCapture);
    QCOMPARE(shifted->bounds(), movedCapture->bounds());
    for (const auto point : shifted->tileCoordinates()) {
        const QRect tile(point * 256, QSize(256, 256));
        QCOMPARE(execution.readSelectionPixels(shifted, tile, alpha.size()).bytes,
                 execution.readSelectionPixels(movedCapture, tile, alpha.size()).bytes);
    }
    replayed = image.transformedTransparencyMask(painted, literal, true, {-280, -540, 24, 28}, plan);
    QVERIFY2(replayed, qPrintable(image.lastRequestOutcome().diagnostic));
    SelectionTileMap retained;
    for (const auto point : replayed->tileCoordinates()) {
        const auto read = execution.readSelectionPixels(replayed, QRect(point * 256, QSize(256, 256)), alpha.size());
        QCOMPARE(read.code, ExecutionCode::Success); retained.insert({point.x(), point.y()}, read.bytes);
    }
    replayedLiteral = SelectionMask::fromAlphaTiles(retained); QVERIFY(replayedLiteral);
    QCOMPARE(image.clearSelection(), Result::Success);
    const auto before = image.operations().snapshot();
    QVERIFY(!image.complementSelectionMask(painted, domain));
    QCOMPARE(image.lastRequestOutcome().code, ExecutionCode::CapacityExceeded);
    QCOMPARE(image.operations().snapshot(), before);
    const QPoint delta(13, -7);
    const auto expectedMove = literal->translated(delta, domain);
    for (const bool pixels : {false, true}) {
        const auto token = image.beginLayerTranslation(QList<LayerTranslationTarget>{{1, pixels, true}});
        QVERIFY(!token.isNull());
        QCOMPARE(image.updateLayerTranslation(token, delta), Result::Success);
        const auto preview = image.operations().preview();
        QVERIFY(matches(preview->layers.first().transparencyMask, expectedMove));
        QCOMPARE(image.operations().snapshot(), before);
        QCOMPARE(image.updateLayerTranslation(token, {limit, 0}), Result::InvalidInput);
        QCOMPARE(image.operations().preview(), preview);
        QCOMPARE(execution.requested(image.handle()), preview);
        const auto busy = execution.beginReadbackFrame(); QCOMPARE(busy.code, ExecutionCode::Success);
        QCOMPARE(image.updateLayerTranslation(token, {17, 9}), Result::Busy);
        QCOMPARE(image.operations().preview(), preview);
        QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
        QCOMPARE(image.cancelLayerTranslation(token), Result::Success);
        QCOMPARE(image.operations().snapshot(), before);
        const auto committed = image.beginLayerTranslation(QList<LayerTranslationTarget>{{1, pixels, true}});
        QCOMPARE(image.updateLayerTranslation(committed, delta), Result::Success);
        QCOMPARE(image.commitLayerTranslation(committed), Result::Success);
        QVERIFY(matches(image.layerTransparencyMask(1), expectedMove));
        QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), before);
        QCOMPARE(image.redo(), Result::Success); QVERIFY(matches(image.layerTransparencyMask(1), expectedMove));
        QCOMPARE(image.undo(), Result::Success);
    }
    const QRect crop(-280, -540, 24, 28);
    const auto expectedCrop = literal->translated({}, crop);
    QCOMPARE(image.cropLayer(1, crop), Result::Success);
    QVERIFY(matches(image.layerTransparencyMask(1), expectedCrop));
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(image.resizeCanvas(crop, true), Result::Success);
    QVERIFY(matches(image.layerTransparencyMask(1), literal->translated(-crop.topLeft(), QRect(QPoint(), crop.size()))));
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), before);
    image.close();
    const auto replacement = Device::createNative(diagnostic); QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution.replaceDevice(replacement).code, ExecutionCode::Success);
    QVERIFY(matches(replayed, replayedLiteral));
    QVERIFY(matches(painted, literal));
}

void KisGpuPaintImageTest::layerPickingKeepsNativeAlphaMasksAndPinnedRequests_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuPaintImageTest::layerPickingKeepsNativeAlphaMasksAndPinnedRequests()
{
    QFETCH(bool, words);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster;
    raster.size = {64, 64}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    QByteArray tile(RgbaTile::byteCount(raster.format), '\0');
    const quint16 alpha[]{0, 1, 127, 128, quint16(maximumComponent(raster.format)), quint16(maximumComponent(raster.format))};
    for (int x = 0; x < 6; ++x) {
        const auto pixel = rgbaBytes({17, 129, 241, alpha[x]}, raster.format);
        std::memcpy(tile.data() + (256 + x) * pixelByteCount(raster.format), pixel.constData(), pixel.size());
    }
    RasterLayer group{QStringLiteral("Group"), true, QUuid::createUuid(), {}};
    group.kind = LayerKind::Group;
    RasterLayer layer{QStringLiteral("Masked layer"), true, QUuid::createUuid(), {{{0, 0}, tile, raster.format}}};
    layer.parentUuid = group.uuid;
    layer.transparencyMask = SelectionMask::fromAlphaCoverage({0, 1, 6, 1}, QByteArray::fromHex("ffffffff0080"), {0, 0, 64, 64});
    RasterLayer implicit{QStringLiteral("Implicit"), true, QUuid::createUuid(), {}};
    implicit.defaultPixel = {17, 129, 241, quint16(maximumComponent(raster.format))};
    raster.layers = {group, layer, implicit};
    PaintImage image(execution, raster);
    const auto version = image.operations().snapshot();
    const auto current = execution.requested(image.handle());
    for (int index : {0, 1}) for (int x = 0; x < 6; ++x) {
        const auto read = execution.readLayerHitTest(image.handle(), version, version->layers[index].id, {x, 1});
        QCOMPARE(read.code, ExecutionCode::Success); QVERIFY(read.bounds); QVERIFY(read.tiles.isEmpty());
        const bool hit = x != 0 && x != 4 && (!words || x >= 3);
        QCOMPARE(!read.bounds->isEmpty(), hit);
        QCOMPARE(execution.requested(image.handle()), current);
        QCOMPARE(image.operations().snapshot(), version);
    }
    const auto layerId = version->layers[1].id;
    QCOMPARE(image.setTransparencyMaskVisible(layerId, false), Result::Success);
    const auto unmasked = image.operations().snapshot();
    for (const auto &pinned : {version, unmasked}) {
        const auto read = execution.readLayerHitTest(image.handle(), pinned, layerId, {4, 1});
        QCOMPARE(read.code, ExecutionCode::Success); QVERIFY(read.bounds);
        QCOMPARE(!read.bounds->isEmpty(), pinned == unmasked);
    }
    const auto uniform = execution.readLayerHitTest(image.handle(), unmasked, version->layers[2].id, {-17, 77});
    QCOMPARE(uniform.code, ExecutionCode::Success); QCOMPARE(*uniform.bounds, QRect(-17, 77, 1, 1));
    const auto empty = execution.readLayerHitTest(image.handle(), unmasked, layerId, {17, 17});
    QCOMPARE(empty.code, ExecutionCode::Success); QVERIFY(empty.bounds->isEmpty());
    QCOMPARE(execution.readLayerHitTest(image.handle(), unmasked, 0, {}).code, ExecutionCode::InvalidRequest);
    QCOMPARE(execution.readLayerHitTest(image.handle(), unmasked, layerId, {RgbaTile::CoordinateLimit, 0}).code,
             ExecutionCode::InvalidRequest);
}

void KisGpuPaintImageTest::layerForestTranslationKeepsPixelsMasksAndOneHistory_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuPaintImageTest::layerForestTranslationKeepsPixelsMasksAndOneHistory()
{
    QFETCH(bool, words);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {64, 64};
    input.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const RgbaPixel color = words ? RgbaPixel{4353, 33282, 61955, 65535} : RgbaPixel{17, 129, 241, 255};
    auto hidden = color;
    hidden[3] = 0;
    const QByteArray pixels = rgbaBytes(color, input.format) + rgbaBytes(hidden, input.format);
    const QRect sourceRect(12, 9, 2, 1);
    QByteArray tile(RgbaTile::byteCount(input.format), '\0');
    std::memcpy(tile.data() + (9 * 256 + 12) * pixelByteCount(input.format), pixels.constData(), pixels.size());
    const auto mask = SelectionMask::fromAlphaCoverage({12, 9, 2, 1}, QByteArray::fromHex("ff80"), {0, 0, 64, 64});
    QVERIFY(mask);
    RasterLayer group{QStringLiteral("Group"), true, QUuid::createUuid(), {}};
    group.kind = LayerKind::Group;
    group.position = {3, 5};
    group.transparencyMask = mask;
    RasterLayer child{QStringLiteral("Child"), true, QUuid::createUuid(), {{{0, 0}, tile, input.format}}};
    child.parentUuid = group.uuid;
    child.position = {7, -3};
    child.transparencyMask = mask;
    RasterLayer top{QStringLiteral("Top"), true, QUuid::createUuid(), {{{0, 0}, tile, input.format}}};
    top.position = {-5, 2};
    input.layers = {group, child, top};
    PaintImage image(execution, input);
    const auto before = image.operations().snapshot();
    const auto groupId = before->layers[0].id, childId = before->layers[1].id, topId = before->layers[2].id;
    const auto childPixels = image.retainLayerPixels(childId), topPixels = image.retainLayerPixels(topId);
    QCOMPARE(image.markSaved(before), Result::Success);
    const QList<LayerTranslationTarget> targets{{groupId, true, true}, {childId, true, true}, {topId, true, false}};
    const QPoint delta(13, -7);
    auto token = image.beginLayerTranslation(targets);
    QVERIFY(!token.isNull());
    QCOMPARE(image.updateLayerTranslation(token, {1, 2}), Result::Success);
    QCOMPARE(image.updateLayerTranslation(token, delta), Result::Success);
    const auto preview = image.operations().preview();
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(childPixels->version(), preview);
    QCOMPARE(topPixels->version(), preview);
    for (int i = 0; i < 3; ++i) QCOMPARE(preview->layers[i].position, before->layers[i].position + delta);
    for (const auto layer : {childId, topId}) {
        const auto read = execution.readRgbaPixels(image.handle(), preview, sourceRect.translated(delta), layer, pixels.size());
        QCOMPARE(read.code, ExecutionCode::Success);
        QCOMPARE(read.bytes, pixels);
        QCOMPARE(image.readRgbaPixels(layer, sourceRect, pixels.size()).bytes, pixels);
    }
    for (int i : {0, 1}) {
        QCOMPARE(execution.readSelectionPixels(preview->layers[i].transparencyMask, {26, 2, 1, 1}, 1).bytes, QByteArray(1, char(128)));
        QCOMPARE(execution.readSelectionPixels(preview->layers[i].transparencyMask, {12, 9, 1, 1}, 1).bytes, QByteArray(1, char(0)));
    }
    QCOMPARE(image.cancelLayerTranslation(token), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(childPixels->version(), before);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.operations().undoCount(), 0);
    token = image.beginLayerTranslation(targets);
    QCOMPARE(image.updateLayerTranslation(token, delta), Result::Success);
    const auto retainedPreview = image.operations().preview();
    QCOMPARE(image.updateLayerTranslation(token, {RgbaTile::CoordinateLimit, 0}), Result::InvalidInput);
    QCOMPARE(image.operations().preview(), retainedPreview);
    QCOMPARE(execution.requested(image.handle()), retainedPreview);
    QCOMPARE(image.commitLayerTranslation(token), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.operations().historyAction(1), HistoryAction::TranslateLayer);
    QCOMPARE(image.writeLayerPosition(topId, {31, 21}), Result::Success);
    QCOMPARE(image.operations().snapshot()->layers[2].position, QPoint(31, 21));
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.undo(), Result::Success);
    for (int i = 0; i < 3; ++i) QCOMPARE(image.operations().snapshot()->layers[i].position, before->layers[i].position);
    for (int i : {0, 1}) QCOMPARE(image.operations().snapshot()->layers[i].transparencyMask, mask);
    QCOMPARE(image.readRgbaPixels(topId, sourceRect, pixels.size()).bytes, pixels);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->layers[1].position, child.position + delta);
    QCOMPARE(image.operations().snapshot()->layers[2].position, top.position + delta);
    QCOMPARE(execution.readSelectionPixels(image.operations().snapshot()->layers[1].transparencyMask, {26, 2, 1, 1}, 1).bytes, QByteArray(1, char(128)));
    QCOMPARE(before->layers[1].position, child.position);
    for (int i : {0, 1}) QCOMPARE(before->layers[i].transparencyMask, mask);
    const auto groupBefore = image.operations().snapshot();
    token = image.beginLayerTranslation(QList<LayerTranslationTarget>{{groupId, false, true}});
    QVERIFY(!token.isNull());
    QCOMPARE(image.updateLayerTranslation(token, {-2, 3}), Result::Success);
    QCOMPARE(image.commitLayerTranslation(token), Result::Success);
    const auto movedGroup = image.operations().snapshot();
    QCOMPARE(movedGroup->layers[0].transparencyMask->bounds(), groupBefore->layers[0].transparencyMask->bounds().translated(-2, 3));
    for (int i = 0; i < 3; ++i) QCOMPARE(movedGroup->layers[i].position, groupBefore->layers[i].position);
    QCOMPARE(movedGroup->layers[1].transparencyMask, groupBefore->layers[1].transparencyMask);
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.operations().snapshot(), groupBefore);
    QCOMPARE(image.redo(), Result::Success); QCOMPARE(image.operations().snapshot(), movedGroup);
    const auto maskBefore = image.operations().snapshot()->layers[1].transparencyMask;
    token = image.beginLayerTranslation(QList<LayerTranslationTarget>{{childId, false, true}});
    QVERIFY(!token.isNull());
    QCOMPARE(image.updateLayerTranslation(token, {2, 3}), Result::Success);
    QCOMPARE(image.commitLayerTranslation(token), Result::Success);
    const QPoint absolute(31, 21);
    QCOMPARE(image.writeLayerPosition(childId, absolute), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->layers[1].position, absolute);
    QCOMPARE(image.operations().snapshot()->layers[1].transparencyMask, maskBefore);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.operations().snapshot()->layers[1].position, absolute);
    QCOMPARE(image.operations().snapshot()->layers[1].transparencyMask->bounds(), maskBefore->bounds().translated(2, 3));
}

void KisGpuPaintImageTest::translationPreviewsPublishPixelsAndCancelWithoutReplacingRedo()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    const auto reference = image.retainLayerPixels(layer);
    const auto pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, {7, 9, 1, 1}, pixel), Result::Success);
    QCOMPARE(image.replaceRgbaPixels(layer, {7, 9, 1, 1}, QByteArray::fromHex("778899ff")), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    const auto fixed = image.operations().snapshot();
    QSignalSpy changes(&image, &PaintImage::changed);
    bool requestedBeforeNotification = false;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedBeforeNotification = execution.requested(image.handle()) == image.operations().preview();
    });
    const auto token = image.beginLayerTranslation(layer);
    QVERIFY(!token.isNull());
    QCOMPARE(image.updateLayerTranslation(token, {-13, 17}), Result::Success);
    QCOMPARE(changes.size(), 1);
    QVERIFY(requestedBeforeNotification);
    QCOMPARE(reference->version(), image.operations().preview());
    QCOMPARE(execution.readRgbaPixels(image.handle(), reference->version(), {-6, 26, 1, 1}, layer, 4).bytes, pixel);
    QCOMPARE(image.readRgbaPixels(layer, {7, 9, 1, 1}, 4).bytes, pixel);
    QCOMPARE(image.operations().snapshot(), fixed);
    QCOMPARE(image.cancelLayerTranslation(token), Result::Success);
    QCOMPARE(reference->version(), fixed);
    QCOMPARE(image.operations().redoCount(), 1);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {7, 9, 1, 1}, 4).bytes, QByteArray::fromHex("778899ff"));
    QCOMPARE(image.updateLayerTranslation(token, {1, 1}), Result::StaleOperation);
    const auto closing = image.beginLayerTranslation(layer);
    QVERIFY(!closing.isNull());
    QCOMPARE(image.updateLayerTranslation(closing, {5, 7}), Result::Success);
    image.close();
    QCOMPARE(reference->version(), image.operations().snapshot());
    QCOMPARE(image.updateLayerTranslation(closing, {9, 11}), Result::Closed);
}

void KisGpuPaintImageTest::colorFillKeepsVersionsHistoryAndWholeRequestFailures()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(32, 16));
    const auto layer = image.operations().activeLayer();
    const auto other = image.addLayer(QStringLiteral("Other"));
    QCOMPARE(image.selectLayers({other}, other), Result::Success);
    const QRect outside(-4, 5, 1, 1);
    const auto seed = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, outside, seed), Result::Success);
    const auto fixed = image.operations().snapshot();
    QCOMPARE(image.markSaved(fixed), Result::Success);
    const auto count = image.operations().undoCount();
    QSignalSpy changed(&image, &PaintImage::changed);
    bool published = false;
    connect(&image, &PaintImage::changed, this, [&] {
        published = execution.requested(image.handle()) == image.operations().snapshot();
    });
    std::array<quint8, 3> rgb{17, 129, 241};
    QCOMPARE(image.fillLayer(layer, rgb, 128.0 / 255), Result::Success);
    rgb = {0, 255, 0};
    QCOMPARE(changed.size(), 1);
    QVERIFY(published);
    QCOMPARE(image.operations().undoCount(), count + 1);
    QCOMPARE(image.operations().historyAction(count + 1), HistoryAction::FillLayer);
    QCOMPARE(image.operations().snapshot()->selectedLayers, fixed->selectedLayers);
    QCOMPARE(image.operations().activeLayer(), other);
    QCOMPARE(image.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.readRgbaPixels(layer, outside, 4).bytes, seed);
    QCOMPARE(image.readRgbaPixels(other, {8, 8, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, {8, 8, 1, 1}, layer, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(image.undo(), Result::Success);
    QVERIFY(!image.operations().isModified());
    const auto pending = image.beginStroke(layer, Brush{}, 42);
    QCOMPARE(image.appendSamples(pending, {{{3, 4}, 1}}), Result::Success);
    const auto preview = image.operations().preview();
    const auto pendingPixelsVersion = image.operations().preview();
    changed.clear();
    QCOMPARE(image.fillLayer(layer, rgb), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), preview);
    QCOMPARE(image.operations().preview(), pendingPixelsVersion);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
    const auto redo = image.operations().preview();
    QCOMPARE(image.fillLayer(9999, rgb), Result::LayerUnavailable);
    QCOMPARE(image.operations().preview(), redo);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.writeRgbaPixels(layer, {3, 4, 1, 1}, seed), Result::Success);
    QCOMPARE(image.operations().historyAction(count + 1), HistoryAction::FillLayer);
    QCOMPARE(image.readRgbaPixels(layer, {3, 4, 1, 1}, 4).bytes, seed);
    QCOMPARE(image.readRgbaPixels(layer, {8, 8, 1, 1}, 4).bytes, QByteArray::fromHex("1181f180"));
    QCOMPARE(image.writeLayerLocked(layer, true), Result::Success);
    const auto locked = image.operations().preview();
    changed.clear();
    QCOMPARE(image.fillLayer(layer, rgb), Result::LayerUnavailable);
    QCOMPARE(image.operations().preview(), locked);
    QCOMPARE(changed.size(), 0);
    Limits limits;
    limits.samplesPerStroke = 1;
    PaintImage bounded(execution, QSize(32, 16), limits);
    const auto before = bounded.operations().preview();
    QCOMPARE(bounded.fillLayer(bounded.operations().activeLayer(), rgb), Result::CapacityExceeded);
    QCOMPARE(bounded.operations().preview(), before);
    auto closing = PaintImageSP::create(execution, QSize(32, 16));
    const auto handle = closing->handle();
    connect(closing.data(), &PaintImage::changed, this, [&] { closing.clear(); });
    QCOMPARE(closing->fillLayer(closing->operations().activeLayer(), rgb), Result::Success);
    QVERIFY(!closing);
    QVERIFY(!execution.requested(handle));
}

void KisGpuPaintImageTest::clearHistoryKeepsItsMeaningAfterDirectWrites()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(16, 16));
    const auto layer = image.operations().activeLayer();
    QCOMPARE(image.replaceRgbaPixels(layer, {1, 1, 1, 1}, QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(image.clearLayers({layer}), Result::Success);
    const auto count = image.operations().undoCount();
    QCOMPARE(image.writeRgbaPixels(layer, {8, 8, 1, 1}, QByteArray::fromHex("445566ff")), Result::Success);
    QCOMPARE(image.operations().undoCount(), count);
    // The document's public undo display reads this action after direct writes.
    QCOMPARE(image.operations().historyAction(count), HistoryAction::ClearPixels);
    QCOMPARE(image.operations().historyAction(count), HistoryAction::ClearPixels);
}

void KisGpuPaintImageTest::mergedSparseDefaultsKeepAllocatedBytes_data()
{ mergedPixelsKeepRootReadersAndIndependentClearHistory_data(); }

void KisGpuPaintImageTest::mergedSparseDefaultsKeepAllocatedBytes()
{
    // KRA and native paint devices distinguish untouched storage from allocated
    // transparent bytes: changing the default changes only the former.
    QFETCH(bool, words);
    QString diagnostic; const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic)); Execution execution(device);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const RgbaPixel color = words ? RgbaPixel{11111, 22222, 33333, 65535} : RgbaPixel{17, 91, 203, 255};
    const RgbaPixel implicit = words ? RgbaPixel{55555, 44444, 33333, 22222} : RgbaPixel{219, 179, 139, 99};
    const auto pixel = rgbaBytes(color, format), defaultPixel = rgbaBytes(implicit, format);
    QByteArray source(256 * 256 * pixel.size(), '\0');
    for (const int x : {5, 197}) source.replace((3 * 256 + x) * pixel.size(), pixel.size(), pixel);
    RasterImage input; input.size = {256, 64}; input.format = format;
    input.layers = {{QStringLiteral("Sparse source"), true, {}, {{{0, 0}, source, format}}}, {QStringLiteral("Empty")}};
    input.layers[0].writtenAreas = {QRect(5, 3, 1, 1), QRect(197, 3, 1, 1)};
    PaintImage image(execution, input);
    LayerMergeRequest request; request.before = image.operations().snapshot(); request.mode = LayerMergeMode::Down;
    request.layers = {1, 2}; request.above = 2; request.name = QStringLiteral("Merged");
    request.homogeneousComposition = true; request.destinationUuid = QUuid::createUuid();
    request.sourceBounds.insert(1, QRect(0, 0, 256, 64)); request.sourceBounds.insert(2, QRect{});
    request.allocatedBlocks = {QRect(0, 0, 64, 64), QRect(192, 0, 64, 64)};
    QCOMPARE(image.mergeLayers(request), Result::Success);
    const auto merged = image.operations().snapshot();
    const auto pixels = image.retainLayerPixels(merged->activeLayer); QVERIFY(pixels);
    QCOMPARE(image.markSaved(merged), Result::Success);
    QCOMPARE(image.writeLayerDefaultPixel(pixels, implicit), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1); QVERIFY(!image.operations().isModified());
    const auto area = QRect(0, 0, 256, 64);
    auto expected = defaultPixel.repeated(area.width() * area.height());
    const auto zeros = QByteArray(64 * pixel.size(), '\0');
    for (int y = 0; y < 64; ++y) for (const int x : {0, 192}) expected.replace((y * 256 + x) * pixel.size(), zeros.size(), zeros);
    for (const int x : {5, 197}) expected.replace((3 * 256 + x) * pixel.size(), pixel.size(), pixel);
    const auto actual = image.readRgbaPixels(merged->activeLayer, area, expected.size());
    QCOMPARE(actual.code, ExecutionCode::Success); QCOMPARE(actual.bytes, expected);
    const auto retained = execution.readRgbaPixels(image.handle(), merged, area, merged->activeLayer, expected.size());
    QCOMPARE(retained.code, ExecutionCode::Success); QCOMPARE(retained.bytes, source.first(area.width() * area.height() * pixel.size()));
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(merged->activeLayer, area, expected.size()).bytes, expected);
}

void KisGpuPaintImageTest::mergedPixelsKeepRootReadersAndIndependentClearHistory_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuPaintImageTest::mergedPixelsKeepRootReadersAndIndependentClearHistory()
{
    // Public root devices outlive Flatten Image. A direct write after clearing
    // its output must stay with that clear step when the merge is undone/redone.
    QFETCH(bool, words);
    QString diagnostic; const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const RgbaPixel color = words ? RgbaPixel{11111, 22222, 33333, 65535} : RgbaPixel{17, 91, 203, 255};
    const auto pixel = rgbaBytes(color, format);
    RasterImage input; input.size = {32, 16}; input.format = format;
    input.layers = {{QStringLiteral("Source"), true, {}, {{{0, 0}, pixel.repeated(256 * 256), format}}},
                    {QStringLiteral("Empty")}};
    PaintImage image(execution, input);
    const auto before = image.operations().snapshot();
    const auto oldRoot = image.retainLayerPixels(0); QVERIFY(oldRoot);
    LayerMergeRequest request; request.before = before; request.mode = LayerMergeMode::FlattenImage;
    request.layers = {1, 2}; request.above = 2; request.name = QStringLiteral("Merged");
    request.destinationUuid = QUuid::createUuid();
    request.sourceBounds.insert(1, QRect(0, 0, 256, 256)); request.sourceBounds.insert(2, QRect(0, 0, 256, 256));
    request.allocatedBlocks = {QRect(0, 0, 256, 256)};
    QCOMPARE(image.mergeLayers(request), Result::Success);
    const auto merged = image.operations().snapshot(); const auto layer = merged->activeLayer;
    const auto newRoot = image.retainLayerPixels(0); QVERIFY(newRoot); QVERIFY(newRoot != oldRoot);
    QCOMPARE(oldRoot->version(), before); QCOMPARE(newRoot->version(), merged);
    const QRect area(0, 0, 32, 16);
    const auto expected = pixel.repeated(area.width() * area.height());
    const auto original = execution.readRgbaPixels(image.handle(), oldRoot->version(), area, 0, expected.size());
    QCOMPARE(original.code, ExecutionCode::Success); QCOMPARE(original.bytes, expected);
    QCOMPARE(image.clearLayers({layer}), Result::Success);
    const auto edit = rgbaBytes(words ? RgbaPixel{55555, 44444, 33333, 65535} : RgbaPixel{219, 179, 139, 255}, format);
    QCOMPARE(image.writeRgbaPixels(layer, QRect(3, 5, 1, 1), edit), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(oldRoot->version(), before); QCOMPARE(newRoot->version(), image.operations().snapshot());
    const auto restored = image.readRgbaPixels(layer, area, expected.size());
    QCOMPARE(restored.code, ExecutionCode::Success); QCOMPARE(restored.bytes, expected);
    QCOMPARE(image.redo(), Result::Success);
    auto cleared = QByteArray(expected.size(), '\0');
    cleared.replace((5 * area.width() + 3) * pixel.size(), pixel.size(), edit);
    const auto redone = image.readRgbaPixels(layer, area, expected.size());
    QCOMPARE(redone.code, ExecutionCode::Success); QCOMPARE(redone.bytes, cleared);
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(oldRoot->version(), image.operations().snapshot());
    QCOMPARE(newRoot->version()->rootUuid, merged->rootUuid);
    QCOMPARE(image.renameLayer(1, QStringLiteral("Independent source")), Result::Success);
    QCOMPARE(image.operations().redoCount(), 0);
    const auto retained = execution.readRgbaPixels(image.handle(), newRoot->version(), area, 0, expected.size());
    QCOMPARE(retained.code, ExecutionCode::Success); QCOMPARE(retained.bytes, expected);
}

void KisGpuPaintImageTest::nativeRootReadsKeepOffCanvasPixelsAndPinnedVersions_data()
{
    QTest::addColumn<bool>("words"); QTest::addColumn<int>("alpha");
    QTest::newRow("u8") << false << 64;
    QTest::newRow("u16") << true << 64 * 257;
    QTest::newRow("u16-sub-byte-default") << true << 1;
    QTest::newRow("u8-transparent-default") << false << 0;
    QTest::newRow("u16-transparent-default") << true << 0;
}

void KisGpuPaintImageTest::nativeRootReadsKeepOffCanvasPixelsAndPinnedVersions()
{
    // Whole-image merging and retained public root devices need the native
    // projection beyond the canvas. Display/export reads keep their canvas.
    QFETCH(bool, words); QFETCH(int, alpha);
    const auto format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const int scale = words ? 257 : 1;
    const RgbaPixel background = alpha ? RgbaPixel{quint16(12 * scale), quint16(34 * scale), quint16(56 * scale), quint16(alpha)} : RgbaPixel{};
    const RgbaPixel color{quint16(17 * scale), quint16(91 * scale), quint16(203 * scale), maximumComponent(format)};
    const auto pixel = rgbaBytes(color, format), implicit = rgbaBytes(background, format);
    const QRect canvas(0, 0, 32, 16), first(-5, 2, 3, 4), second(28, 17, 3, 3), region(-8, -4, 160, 32);
    QString diagnostic; const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input; input.size = canvas.size(); input.format = format; input.projectionBackground = background;
    input.layers = {{QStringLiteral("Lower")}, {QStringLiteral("Upper")}};
    PaintImage image(execution, input);
    QCOMPARE(image.writeRgbaPixels(1, first, pixel.repeated(first.width() * first.height())), Result::Success);
    QCOMPARE(image.writeRgbaPixels(2, second, pixel.repeated(second.width() * second.height())), Result::Success);
    auto hidden = color; hidden[3] = 0;
    QCOMPARE(image.writeRgbaPixels(1, QRect(100, 5, 1, 1), rgbaBytes(hidden, format)), Result::Success);
    const auto before = image.operations().snapshot();
    const auto oldRoot = image.retainLayerPixels(0); QVERIFY(oldRoot);
    // Default-255 selection bounds query the root device's allocated extent,
    // independently of its tighter nontransparent bounds and pinned pixels.
    const auto allocation = execution.layerTileCoordinates(image.handle(), before, 0, 16);
    QCOMPARE(allocation.code, ExecutionCode::Success);
    QCOMPARE(allocation.writtenAreas, QList<QRect>{QRect(-64, 0, 192, 64)});
    QCOMPARE(execution.layerTileCoordinates(image.handle(), before, 0, 0).code, ExecutionCode::CapacityExceeded);
    const bool opaqueDefault = words ? (alpha - (alpha >> 8) + 128) >> 8 : alpha;
    const auto expectedBounds = opaqueDefault ? (first | second | canvas) : (first | second);
    const auto bounds = execution.readNativeProjectionBounds(image.handle(), before);
    QCOMPARE(bounds.code, ExecutionCode::Success); QCOMPARE(bounds.bounds.value(), expectedBounds);
    auto expected = implicit.repeated(region.width() * region.height());
    const auto replace = [&](QByteArray &bytes, QPoint point, const QByteArray &value) {
        bytes.replace(((point.y() - region.y()) * region.width() + point.x() - region.x()) * pixel.size(), pixel.size(), value);
    };
    for (const auto rect : {first, second})
        for (int y = rect.top(); y <= rect.bottom(); ++y) for (int x = rect.left(); x <= rect.right(); ++x)
            replace(expected, {x, y}, pixel);
    auto native = execution.readNativeProjectionPixels(image.handle(), before, region, expected.size());
    QCOMPARE(native.code, ExecutionCode::Success); QCOMPARE(native.bytes, expected);
    const auto canonical = execution.readRgbaPixels(image.handle(), before, first, 0, first.width() * first.height() * pixel.size());
    QCOMPARE(canonical.code, ExecutionCode::Success); QCOMPARE(canonical.bytes, QByteArray(canonical.bytes.size(), '\0'));
    const auto rejected = execution.readNativeProjectionPixels(image.handle(), before, region, expected.size() - 1);
    QCOMPARE(rejected.code, ExecutionCode::CapacityExceeded); QVERIFY(rejected.bytes.isEmpty());
    QCOMPARE(image.operations().snapshot(), before);
    LayerMergeRequest request; request.before = before; request.mode = LayerMergeMode::FlattenImage;
    request.layers = {1, 2}; request.above = 2; request.name = QStringLiteral("Merged"); request.destinationUuid = QUuid::createUuid();
    for (const auto id : request.layers) request.sourceBounds.insert(id, expectedBounds | canvas);
    request.allocatedBlocks = {QRect(-64, 0, 128, 64)};
    QCOMPARE(image.mergeLayers(request), Result::Success);
    QCOMPARE(oldRoot->version(), before);
    native = execution.readNativeProjectionPixels(image.handle(), oldRoot->version(), region, expected.size());
    QCOMPARE(native.code, ExecutionCode::Success); QCOMPARE(native.bytes, expected);
    const auto originalPixels = expected;
    const auto copied = expectedBounds | canvas;
    for (int y = region.top(); y <= region.bottom(); ++y) for (int x = region.left(); x <= region.right(); ++x)
        if (!copied.contains(x, y)) replace(expected, {x, y}, QByteArray(pixel.size(), '\0'));
    const auto mergedPixels = image.readRgbaPixels(image.operations().activeLayer(), region, expected.size());
    QCOMPARE(mergedPixels.code, ExecutionCode::Success); QCOMPARE(mergedPixels.bytes, expected);
    // Native root replacement keeps its background. A nontransparent root
    // composites the copied layer over that background; a root whose native
    // opacityU8 is zero borrows its single child's projection instead.
    if (opaqueDefault) {
        expected = originalPixels;
        auto composedBackground = background;
        const quint64 maximum = maximumComponent(format);
        composedBackground[3] = quint16((2 * quint64(alpha) * maximum - quint64(alpha) * alpha + maximum / 2) / maximum);
        const auto composed = rgbaBytes(composedBackground, format);
        for (int y = region.top(); y <= region.bottom(); ++y) for (int x = region.left(); x <= region.right(); ++x)
            if (copied.contains(x, y) && !first.contains(x, y) && !second.contains(x, y)) replace(expected, {x, y}, composed);
    }
    const auto current = execution.readNativeProjectionPixels(image.handle(), image.operations().snapshot(), region, expected.size());
    QCOMPARE(current.code, ExecutionCode::Success); QCOMPARE(current.bytes, expected);
    QCOMPARE(image.undo(), Result::Success); QCOMPARE(oldRoot->version(), image.operations().snapshot());
}

void KisGpuPaintImageTest::clearedLayersKeepFixedVersionsAndRejectWholeRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster{QSize(64, 64), {{QStringLiteral("Default pixels"), true, QUuid::createUuid(), {}, {5, 6, 7, 255}},
                                      {QStringLiteral("Paint"), true, QUuid::createUuid(), {}}}, 1};
    Limits limits;
    limits.rasterBytes = RgbaTile::ByteCount;
    PaintImage image(execution, raster, limits);
    const auto first = image.operations().snapshot()->layers[0].id;
    const auto second = image.operations().snapshot()->layers[1].id;
    const QRect region(-4, 8, 1, 1);
    const auto pixel = QByteArray::fromHex("112233ff");
    QCOMPARE(image.replaceRgbaPixels(first, region, pixel), Result::Success);
    Brush brush;
    const auto stroke = image.beginStroke(second, brush, 11);
    QCOMPARE(image.appendSamples(stroke, {{{8, 8}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    const auto fixed = image.operations().snapshot();
    const auto reference = image.retainLayerPixels(first);
    QCOMPARE(image.markSaved(fixed), Result::Success);
    const auto count = image.operations().undoCount();
    QSignalSpy changed(&image, &PaintImage::changed);
    bool requestedBeforeNotification = true;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedBeforeNotification &= execution.requested(image.handle()) == image.operations().preview();
    });
    const auto pending = image.beginStroke(second, brush, 12);
    QCOMPARE(image.appendSamples(pending, {{{24, 24}, 1}}), Result::Success);
    const auto preview = image.operations().preview();
    const auto history = image.operations().preview();
    changed.clear();
    QCOMPARE(image.clearLayers({first, second}), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), preview);
    QCOMPARE(image.operations().preview(), history);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
    changed.clear();
    QCOMPARE(image.clearLayers({first, 9999}), Result::LayerUnavailable);
    QCOMPARE(image.clearLayers({first, first}), Result::InvalidInput);
    QCOMPARE(image.operations().snapshot(), fixed);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.clearLayers({first, second}), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.operations().undoCount(), count + 1);
    QCOMPARE(image.operations().historyAction(count + 1), HistoryAction::ClearPixels);
    QCOMPARE(image.operations().snapshot()->sampleCount, 0);
    QCOMPARE(image.operations().snapshot()->rasterBytes, 0);
    QCOMPARE(image.operations().snapshot()->selectedLayers, fixed->selectedLayers);
    QCOMPARE(image.operations().activeLayer(), fixed->activeLayer);
    QCOMPARE(reference->version(), image.operations().snapshot());
    QCOMPARE(image.readRgbaPixels(first, region, 4).bytes, QByteArray::fromHex("050607ff"));
    QCOMPARE(image.readRgbaPixels(second, {8, 8, 1, 1}, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), fixed, region, first, 4).bytes, pixel);
    QVERIFY(image.operations().isModified());
    QCOMPARE(image.undo(), Result::Success);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.readRgbaPixels(first, region, 4).bytes, pixel);
    QCOMPARE(image.redo(), Result::Success);
    const auto cleared = image.operations().snapshot();
    QCOMPARE(image.writeLayerLocked(second, true), Result::Success);
    const auto locked = image.operations().snapshot();
    changed.clear();
    QCOMPARE(image.clearLayers({first, second}), Result::LayerUnavailable);
    QCOMPARE(image.operations().snapshot(), locked);
    QCOMPARE(changed.size(), 0);
    QVERIFY(requestedBeforeNotification);
    QCOMPARE(execution.readRgbaPixels(image.handle(), cleared, region, first, 4).bytes, QByteArray::fromHex("050607ff"));
    auto closing = PaintImageSP::create(execution, QSize(32, 32));
    const auto handle = closing->handle();
    connect(closing.data(), &PaintImage::changed, this, [&] { closing.clear(); });
    QCOMPARE(closing->clearLayers({closing->operations().activeLayer()}), Result::Success);
    QVERIFY(!closing);
    QVERIFY(!execution.requested(handle));
}

void KisGpuPaintImageTest::idleSmoothingPublishesAcceptedVersionsWithoutDuplicateNotifications()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    Brush brush;
    brush.basicSmoothing = true;
    QSignalSpy changed(&image, &PaintImage::changed);
    bool requestedAtNotification = true;
    connect(&image, &PaintImage::changed, this, [&] {
        requestedAtNotification &= execution.requested(image.handle()) == image.operations().preview();
    });
    const auto id = image.beginStroke(image.operations().activeLayer(), brush, 7);
    QCOMPARE(image.appendSamples(id, {{{8, 32}, 1}, {{32, 8}, 1}}), Result::Success);
    QCOMPARE(changed.size(), 1);
    const auto beforeIdle = image.operations().preview();
    QCOMPARE(image.flushSmoothing(id), Result::Success);
    QCOMPARE(changed.size(), 2);
    const auto afterIdle = image.operations().preview();
    QVERIFY(beforeIdle != afterIdle);
    QCOMPARE(image.flushSmoothing(id), Result::Success);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(image.operations().preview(), afterIdle);
    QCOMPARE(image.commitStroke(id), Result::Success);
    QCOMPARE(changed.size(), 3);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(requestedAtNotification);
}

void KisGpuPaintImageTest::derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites_data()
{
    QTest::addColumn<bool>("basicSmoothing");
    QTest::addColumn<bool>("weightedSmoothing");
    QTest::newRow("path") << false << false;
    QTest::newRow("basic-smoothing") << true << false;
    QTest::newRow("weighted-smoothing") << true << true;
}

void KisGpuPaintImageTest::derivedGeometryBudgetsPreserveInputAndRejectWholeReadsAndWrites()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    QFETCH(bool, basicSmoothing);
    QFETCH(bool, weightedSmoothing);
    Shape shape;
    Brush brush;
    brush.basicSmoothing = basicSmoothing;
    QList<InputSample> samples;
    const qreal limit = OperationDocument::CoordinateLimit;
    if (weightedSmoothing) {
        brush.weightedSmoothing = WeightedSmoothing{24, 48, 0.01, true};
        for (int index = 0; index < 6000; ++index)
            samples.append({{8, 16}, 1, 0, 0, 0, quint64(index * 8)});
    } else if (basicSmoothing) {
        const QList<QPointF> corners{{0, 0}, {limit, 0}, {limit, limit}, {0, limit}};
        for (int index = 0; index < 512; ++index)
            samples.append({corners[index % corners.size()], 1, 0, 0, 0, quint64(index * 8)});
    } else {
        shape.kind = ShapeKind::Path;
        for (int curve = 0; curve < 65; ++curve) {
            shape.pathCommands.append({PathCommand::MoveTo, PathCommand::CubicTo});
            samples.append({{{0, 0}, 1}, {{limit, 0}, 1}, {{limit, limit}, 1}, {{0, limit}, 1}});
        }
    }
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, FigureStyle{}, 42), Result::Success);
    const auto pinned = image.operations().snapshot();
    const auto retainedVersion = image.operations().preview();
    TileRenderer renderer(device);
    auto *commands = device->beginCommands();
    QVERIFY(commands);
    const auto recorded = renderer.record(commands, pinned);
    QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
    QCOMPARE(recorded.code, RenderOutcome::CapacityExceeded);
    QCOMPARE(renderer.residentBytes(), qsizetype(0));
    const auto read = execution.readLayerPixels(image.handle(), pinned, layer, RgbaTile::ByteCount);
    QCOMPARE(read.code, ExecutionCode::CapacityExceeded);
    QVERIFY(read.tiles.isEmpty());
    QCOMPARE(image.writeRgbaPixels(layer, {8, 8, 1, 1}, QByteArray::fromHex("112233ff")), Result::CapacityExceeded);
    QCOMPARE(image.operations().snapshot(), pinned);
    QCOMPARE(image.operations().preview(), retainedVersion);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(image.operations().activeOperation().isNull());
}

void KisGpuPaintImageTest::interiorPixelWritesRemainWithTheirFilledHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(1024, 1024));
    const auto layer = image.operations().activeLayer();
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(image.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{1024, 1024}, 1}},
                               {ShapeKind::Rectangle, {0, 0}, {1024, 1024}, 0}, style, 42), Result::Success);
    const QRect region(512, 512, 1, 1);
    const auto written = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, region, written), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, written);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, QByteArray(4, char(0)));
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, region, 4).bytes, written);
}

void KisGpuPaintImageTest::protectedCopyOutlineKeepsNormalFillDestinationBounds()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {128, 128};
    RasterLayer layer;
    layer.defaultPixel = {1, 2, 3, 0};
    input.layers = {layer};
    PaintImage image(execution, input);
    const auto id = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    Brush brush;
    brush.rgba = {17, 63, 129, 255};
    brush.diameter = 8;
    brush.alphaLocked = true;
    brush.blendMode = LayerBlendMode::Copy;
    FigureStyle style;
    style.fill = RgbaPixel{200, 127, 1, 255};
    const Shape shape{ShapeKind::Rectangle, {32, 32}, {96, 96}, 0};
    QCOMPARE(image.paintFigure(id, brush, {{{32, 32}, 1}, {{96, 96}, 1}}, shape, style, 7), Result::Success);
    const auto painted = image.operations().snapshot();
    const auto read = [&](const Snapshot &version, QPoint point) {
        return execution.readRgbaPixels(image.handle(), version, QRect(point, QSize(1, 1)), id, 4).bytes;
    };
    // Copy paints hidden RGB beyond the earlier allocation. The normal fill
    // in the same figure retains its native protected-destination clipping.
    QCOMPARE(read(painted, {64, 30}), QByteArray::fromHex("113f8100"));
    QCOMPARE(read(painted, {64, 64}), QByteArray::fromHex("01020300"));
    QCOMPARE(read(painted, {0, 0}), QByteArray::fromHex("01020300"));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(read(image.operations().snapshot(), {64, 30}), QByteArray::fromHex("01020300"));
    QCOMPARE(read(painted, {64, 30}), QByteArray::fromHex("113f8100"));
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(read(image.operations().snapshot(), {64, 30}), QByteArray::fromHex("113f8100"));
    QCOMPARE(read(before, {64, 30}), QByteArray::fromHex("01020300"));
}

void KisGpuPaintImageTest::filledFiguresPublishOneVersionAndRejectWholeRequests()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.operations().activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 4;
    FigureStyle style;
    style.fill = RgbaPixel{221, 17, 44, 255};
    const Shape shape{ShapeKind::Rectangle, {8, 8}, {56, 56}, 0};
    const QList<InputSample> samples{{{8, 8}, 1}, {{56, 56}, 1}};
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.operations().undoCount(), 1);
    QCOMPARE(image.readRgbaPixels(layer, {32, 24, 1, 1}, 4).bytes, QByteArray::fromHex("dd112cff"));
    QCOMPARE(image.readRgbaPixels(layer, {32, 8, 1, 1}, 4).bytes, QByteArray::fromHex("dd112cff"));
    QCOMPARE(image.readRgbaPixels(layer, {32, 7, 1, 1}, 4).bytes, QByteArray::fromHex("1181f1ff"));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, {32, 24, 1, 1}, 4).bytes, QByteArray(4, char(0)));
    QCOMPARE(image.redo(), Result::Success);
    const auto before = image.operations().preview();
    changed.clear();
    auto invalid = samples;
    invalid.first().pressure = -1;
    QCOMPARE(image.paintFigure(layer, brush, invalid, shape, style, 43), Result::InvalidInput);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.operations().preview(), before);
    const auto pending = image.beginStroke(layer, brush, 44);
    QVERIFY(!pending.isNull());
    QCOMPARE(image.paintFigure(layer, brush, samples, shape, style, 45), Result::EditInProgress);
    QCOMPARE(image.operations().activeOperation(), pending);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(image.cancelStroke(pending), Result::Success);
}

void KisGpuPaintImageTest::insertedLayerPixelsPublishAtomicallyAndRemainIndependent()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage source(execution, QSize(16, 16));
    PaintImage destination(execution, QSize(16, 16));
    const auto sourceLayer = source.operations().activeLayer();
    const auto pixels = source.retainLayerPixels(sourceLayer);
    const QRect rect(5, 7, 1, 1);
    const auto bytes = QByteArray::fromHex("112233ff");
    QCOMPARE(source.writeRgbaPixels(sourceLayer, rect, bytes), Result::Success);
    RasterLayer properties;
    properties.name = QStringLiteral("Inserted pixels");
    properties.uuid = QUuid::createUuid();
    QSignalSpy changed(&destination, &PaintImage::changed);
    const auto inserted = destination.insertLayer(source, pixels, properties, 1);
    QVERIFY(inserted);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(destination.operations().undoCount(), 1);
    QCOMPARE(destination.readRgbaPixels(inserted, rect, 4).bytes, bytes);
    const auto fixed = destination.operations().snapshot();
    QCOMPARE(source.writeRgbaPixels(sourceLayer, rect, QByteArray::fromHex("445566ff")), Result::Success);
    QCOMPARE(destination.readRgbaPixels(inserted, rect, 4).bytes, bytes);
    QCOMPARE(destination.undo(), Result::Success);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.operations().snapshot(), fixed);
    auto rejected = properties;
    rejected.uuid = QUuid::createUuid();
    const auto before = destination.operations().preview();
    QVERIFY(!destination.insertLayer(destination, pixels, rejected, 0));
    QCOMPARE(destination.operations().preview(), before);
    QCOMPARE(source.removeLayer(sourceLayer), Result::Success);
    const auto retained = destination.insertLayer(source, pixels, rejected, 0);
    QVERIFY(retained);
    QCOMPARE(destination.readRgbaPixels(retained, rect, 4).bytes, QByteArray::fromHex("445566ff"));
    OperationDocument empty(QSize(16, 16));
    rejected.uuid = QUuid::createUuid();
    QVERIFY(destination.insertLayer(empty, empty.activeLayer(), rejected, 0));
    QCOMPARE(destination.limits().rasterBytes, Limits{}.rasterBytes);
}

void KisGpuPaintImageTest::retainedLayerPixelsRemainEditableThroughRemovalAndDiscardedHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(16, 16));
    const auto bottom = image.operations().activeLayer();
    const auto layer = image.addLayer(QStringLiteral("Retained"));
    const auto pixels = image.retainLayerPixels(layer);
    QVERIFY(pixels);
    QCOMPARE(image.retainLayerPixels(layer), pixels);
    const auto first = QByteArray::fromHex("112233ff");
    const auto second = QByteArray::fromHex("445566aa");
    const QRect rect(5, 5, 1, 1);
    QCOMPARE(image.writeRgbaPixels(layer, rect, first), Result::Success);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    const auto pinned = pixels->version();
    const auto before = image.operations().undoCount();
    QCOMPARE(image.removeLayer(layer), Result::Success);
    const auto removed = image.operations().snapshot();
    QCOMPARE(image.markSaved(removed), Result::Success);
    QCOMPARE(pixels->version(), pinned);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, second), Result::Success);
    QCOMPARE(image.operations().snapshot(), removed);
    QCOMPARE(image.operations().undoCount(), before + 1);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, second);
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, rect, layer, 4).bytes, first);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(layer, rect, 4).bytes, second);
    QCOMPARE(pixels->version(), image.operations().snapshot());
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.renameLayer(bottom, QStringLiteral("Discarded layer branch")), Result::Success);
    QCOMPARE(image.operations().redoCount(), 0);
    const auto branch = image.operations().snapshot();
    const auto history = image.operations().undoCount();
    const auto lastPixels = pixels->version();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, first), Result::Success);
    QCOMPARE(image.operations().snapshot(), branch);
    QCOMPARE(image.operations().undoCount(), history);
    QCOMPARE(image.readRgbaPixels(bottom, rect, 4).bytes, QByteArray(4, '\0'));
    QCOMPARE(execution.readRgbaPixels(image.handle(), pixels->version(), rect, layer, 4).bytes, first);
    QCOMPARE(execution.readRgbaPixels(image.handle(), lastPixels, rect, layer, 4).bytes, second);
    QVERIFY(!image.retainLayerPixels(layer));
}

void KisGpuPaintImageTest::rejectedRetainedLayerWritesPreserveVersionsAndHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    Limits limits;
    limits.rasterBytes = 2 * RgbaTile::ByteCount;
    PaintImage image(execution, QSize(16, 16), limits);
    PaintImage other(execution, QSize(16, 16));
    const auto layer = image.operations().activeLayer();
    const auto pixels = image.retainLayerPixels(layer);
    const auto foreign = other.retainLayerPixels(other.operations().activeLayer());
    QVERIFY(pixels && foreign);
    const QRect rect(5, 5, 1, 1);
    const auto bytes = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(layer, rect, bytes), Result::Success);
    QCOMPARE(image.removeLayer(layer), Result::Success);
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::Success);
    const auto pinned = pixels->version();
    const auto before = image.operations().preview();
    QCOMPARE(image.writeRgbaPixels(pixels, rect, bytes), Result::CapacityExceeded);
    QCOMPARE(image.writeRgbaPixels(foreign, rect, bytes), Result::InvalidInput);
    QCOMPARE(pixels->version(), pinned);
    QCOMPARE(image.operations().preview(), before);
}

void KisGpuPaintImageTest::retainedImagesAndClonesKeepTheirExecutionAlive()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto image = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = image->operations().activeLayer();
    QCOMPARE(image->writeRgbaPixels(layer, QRect(4, 5, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    auto clone = image->clone(true);
    execution.reset();
    device.reset();
    image.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(clone->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));

    auto destinationExecution = std::make_shared<Execution>(Device::createNative(diagnostic));
    std::weak_ptr<Execution> destinationLifetime = destinationExecution;
    auto destination = PaintImageSP::create(destinationExecution, QSize(32, 32));
    auto replacement = clone->cloneTo(*destination, true);
    destinationExecution.reset();
    destination.reset();
    clone.reset();
    QVERIFY(lifetime.expired());
    QVERIFY(!destinationLifetime.expired());
    QCOMPARE(replacement->readRgbaPixels(layer, QRect(4, 5, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    replacement.reset();
    QVERIFY(destinationLifetime.expired());
    QVERIFY_EXCEPTION_THROWN(PaintImage(std::shared_ptr<Execution>{}, QSize(32, 32)), std::invalid_argument);
}

void KisGpuPaintImageTest::nestedCopiesRemapParentsAndDetachTheSelectedSubtree()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage input;
    input.size = {64, 48};
    input.layers = {
        {QStringLiteral("Paint"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")),
         {{{0, 0}, QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Inner"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}},
        {QStringLiteral("Outer"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    input.layers[1].kind = input.layers[2].kind = LayerKind::Group;
    input.layers[0].parentUuid = input.layers[1].uuid;
    input.layers[1].parentUuid = input.layers[2].uuid;
    auto document = PaintImageSP::create(execution, input);
    const auto fixed = document->operations().snapshot();
    auto copy = document->clone(false);
    QVERIFY(copy);
    const auto copied = copy->operations().snapshot();
    QCOMPARE(copied->layers.size(), 3);
    QCOMPARE(copied->layers[0].parentUuid, copied->layers[1].uuid);
    QCOMPARE(copied->layers[1].parentUuid, copied->layers[2].uuid);
    for (int i = 0; i < 3; ++i) QVERIFY(copied->layers[i].uuid != fixed->layers[i].uuid);
    auto subtree = document->cloneLayerPixels(document->retainLayerPixels(2));
    QVERIFY(subtree);
    const auto nested = subtree->operations().snapshot();
    QCOMPARE(nested->layers.size(), 2);
    QCOMPARE(nested->layers[0].parentUuid, nested->layers[1].uuid);
    QVERIFY(nested->layers[1].parentUuid.isNull());
    QCOMPARE(nested->rasterBytes, fixed->rasterBytes);
    auto leaf = document->cloneLayerPixels(document->retainLayerPixels(1));
    QVERIFY(leaf);
    QCOMPARE(leaf->operations().snapshot()->layers.size(), 1);
    QVERIFY(leaf->operations().snapshot()->layers.first().parentUuid.isNull());
    QCOMPARE(leaf->readRgbaPixels(1, {7, 9, 1, 1}, 4).bytes, QByteArray::fromHex("112233ff"));
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(document->writeRgbaPixels(2, {0, 0, 1, 1}, QByteArray::fromHex("112233ff")), Result::LayerUnavailable);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(document->operations().snapshot(), fixed);
}

void KisGpuPaintImageTest::clonesKeepIndependentCurrentStateAndOperationIdentities()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto source = PaintImageSP::create(execution, QSize(64, 64));
    const auto layer = source->operations().activeLayer();
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    const auto hidden = source->addLayer(QStringLiteral("Hidden"));
    QVERIFY(hidden);
    QCOMPARE(source->writeRgbaPixels(hidden, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    QCOMPARE(source->setLayerVisible(hidden, false), Result::Success);
    const auto before = source->operations().snapshot();
    const auto history = source->operations().undoCount();
    auto exact = source->clone(true);
    QVERIFY(exact);
    QVERIFY(exact->handle() != source->handle());
    QVERIFY(exact->operations().snapshot()->documentId() != before->documentId());
    QCOMPARE(exact->operations().undoCount(), 0);
    QCOMPARE(exact->operations().redoCount(), 0);
    QVERIFY(!exact->operations().isModified());
    QCOMPARE(exact->operations().snapshot()->layers[0].uuid, before->layers[0].uuid);
    QCOMPARE(exact->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes, source->readRgbaPixels(layer, QRect(12, 12, 1, 1), 4).bytes);
    QCOMPARE(exact->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(exact->replaceRgbaPixels(layer, QRect(12, 12, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(source->operations().snapshot(), before);
    QCOMPARE(source->operations().undoCount(), history);
    auto renamed = source->clone(false);
    QVERIFY(renamed);
    for (int index = 0; index < before->layers.size(); ++index)
        QVERIFY(renamed->operations().snapshot()->layers[index].uuid != before->layers[index].uuid);
    Execution destination(device);
    auto transferred = source->cloneTo(destination, true);
    QVERIFY(transferred);
    QCOMPARE(&transferred->execution(), &destination);
    QCOMPARE(destination.requested(transferred->handle()), transferred->operations().snapshot());
    QCOMPARE(transferred->readRgbaPixels(hidden, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    Execution full(device, 128 * 1024 * 1024, 1);
    PaintImage oneSlot(full, QSize(64, 64));
    const auto retained = oneSlot.operations().snapshot();
    QVERIFY_EXCEPTION_THROWN(oneSlot.clone(true), std::length_error);
    QVERIFY_EXCEPTION_THROWN(oneSlot.cloneLayerPixels(oneSlot.retainLayerPixels(oneSlot.operations().activeLayer())), std::length_error);
    QCOMPARE(oneSlot.operations().snapshot(), retained);
    QCOMPARE(full.requested(oneSlot.handle()), retained);
}

void KisGpuPaintImageTest::retainedLayerCopiesKeepIndependentPixelsAndExecutionLifetime()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Execution>(device);
    std::weak_ptr<Execution> lifetime = execution;
    auto source = PaintImageSP::create(execution, QSize(32, 32));
    const auto layer = source->addLayer(QStringLiteral("Retained copy"));
    const auto stroke = source->beginStroke(layer, Brush{}, 17);
    QCOMPARE(source->appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    QCOMPARE(source->commitStroke(stroke), Result::Success);
    QCOMPARE(source->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("aabbcc00")), Result::Success);
    const auto pixels = source->retainLayerPixels(layer);
    const auto retained = pixels->version();
    const auto expected = execution->readRgbaPixels(source->handle(), retained, QRect(0, 0, 32, 32), layer, 4096).bytes;
    QCOMPARE(source->removeLayer(layer), Result::Success);
    const auto before = source->operations().snapshot();
    QVERIFY(!source->cloneLayerPixels({}));
    QVERIFY(!source->cloneLayerPixels(PaintImageSP::create(execution, QSize(1, 1))->retainLayerPixels(1)));
    auto copy = source->cloneLayerPixels(pixels);
    QVERIFY(copy);
    const auto version = copy->operations().snapshot();
    QCOMPARE(version->layers.size(), 1);
    QCOMPARE(version->activeLayer, layer);
    QCOMPARE(version->selectedLayers, QList<LayerId>{layer});
    QCOMPARE(version->sampleCount, retained->sampleCount);
    QCOMPARE(version->rasterBytes, retained->rasterBytes);
    QVERIFY(version->documentId() != retained->documentId());
    QCOMPARE(copy->operations().undoCount(), 0);
    QVERIFY(!copy->operations().isModified());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(0, 0, 32, 32), 4096).bytes, expected);
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(copy->writeRgbaPixels(layer, QRect(-5, 7, 1, 1), QByteArray::fromHex("112233ff")), Result::Success);
    QCOMPARE(execution->readRgbaPixels(source->handle(), retained, QRect(-5, 7, 1, 1), layer, 4).bytes, QByteArray::fromHex("aabbcc00"));
    QCOMPARE(source->operations().snapshot(), before);
    source.reset();
    execution.reset();
    QVERIFY(!lifetime.expired());
    QCOMPARE(copy->readRgbaPixels(layer, QRect(-5, 7, 1, 1), 4).bytes, QByteArray::fromHex("112233ff"));
    copy.reset();
    QVERIFY(lifetime.expired());
}

void KisGpuPaintImageTest::historyNavigationPublishesOneVersionAndPreservesSavedState()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    const auto layer = image.addLayer(QStringLiteral("Second layer"));
    QVERIFY(layer);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Saved layer")), Result::Success);
    QSignalSpy saved(&image, &PaintImage::savedStateChanged);
    QCOMPARE(image.markSaved(image.operations().snapshot()), Result::Success);
    QCOMPARE(saved.size(), 1);
    QCOMPARE(image.renameLayer(layer, QStringLiteral("Later layer")), Result::Success);
    const auto versions = image.operations().historyVersions();
    QCOMPARE(versions.size(), 4);
    QCOMPARE(image.operations().savedHistoryIndex(), 2);
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.setHistoryIndex(0), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(execution.requested(image.handle()), versions[0]);
    QCOMPARE(image.operations().redoCount(), 3);
    QVERIFY(image.operations().isModified());
    QCOMPARE(image.setHistoryIndex(-1), Result::InvalidInput);
    QCOMPARE(image.setHistoryIndex(4), Result::InvalidInput);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.setHistoryIndex(2), Result::Success);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(image.operations().historyVersions(), versions);
    const auto stroke = image.beginStroke(layer, {}, 17);
    QCOMPARE(image.appendSamples(stroke, {{{12, 12}, 1}}), Result::Success);
    const auto pending = image.operations().preview();
    const int notifications = changed.size();
    QCOMPARE(image.setHistoryIndex(0), Result::EditInProgress);
    QCOMPARE(image.operations().preview(), pending);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.cancelStroke(stroke), Result::Success);
    QCOMPARE(image.setHistoryIndex(3), Result::Success);
    QVERIFY(image.operations().isModified());
}

void KisGpuPaintImageTest::sharedEditsPublishBeforeNotificationAndReleaseTheLastAttachment()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    auto owner = PaintImageSP::create(execution, QSize(64, 64));
    auto reader = owner;
    const auto handle = owner->handle();
    QVERIFY(handle);
    QSignalSpy changed(owner.data(), &PaintImage::changed);
    bool currentAtNotification = true;
    connect(owner.data(), &PaintImage::changed, this, [&] {
        currentAtNotification &= execution.requested(handle) == reader->operations().preview();
    });
    owner.clear();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto stroke = reader->beginStroke(reader->operations().activeLayer(), brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(reader->appendSamples(stroke, {{{12, 12}, 1}, {{32, 32}, 0.5}}), Result::Success);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(reader->commitStroke(stroke), Result::Success);
    QCOMPARE(reader->operations().undoCount(), 1);
    QVERIFY(reader->operations().isModified());
    const auto pixels = execution.readPixels(handle, reader->operations().snapshot(), {0, 0, 64, 64});
    QCOMPARE(pixels.code, ExecutionCode::Success);
    QCOMPARE(pixels.tiles.size(), 1);
    QCOMPARE(pixels.tiles.front().rgba.mid((12 * 256 + 12) * 4, 4), QByteArray::fromHex("1181f1ff"));
    const auto count = changed.size();
    QCOMPARE(reader->markSaved(reader->operations().snapshot()), Result::Success);
    QVERIFY(!reader->operations().isModified());
    QCOMPARE(changed.size(), count);
    QCOMPARE(reader->undo(), Result::Success);
    QCOMPARE(reader->redo(), Result::Success);
    QVERIFY(currentAtNotification);
    reader.clear();
    QVERIFY(!execution.requested(handle));
    QCOMPARE(execution.pendingDocuments(), qsizetype(0));
}

void KisGpuPaintImageTest::directPixelsReachGpuWithoutEditingNotificationsOrHistory()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const RgbaTile pixels{{-1, -1}, QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Success);
    QVERIFY(changed.isEmpty());
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.requested(image.handle()), image.operations().snapshot());
    const auto read = execution.readPixels(image.handle(), image.operations().snapshot(), {-1, -1, 1, 1},
                                           {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.tiles.size(), 1);
    QCOMPARE(read.tiles.first().rgba, pixels.rgba);
    const auto old = execution.readPixels(image.handle(), before, {-1, -1, 1, 1},
                                          {layer}, false, PixelReadArea::LayerCoordinates);
    QCOMPARE(old.code, ExecutionCode::Success);
    QVERIFY(old.tiles.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, {-256, -256, 256, 256}, pixels.rgba), Result::Closed);
}

void KisGpuPaintImageTest::rectangularWritesPreserveOtherRegionsAcrossHistory_data()
{
    QTest::addColumn<QPoint>("source");
    QTest::addColumn<QPoint>("point");
    QTest::addColumn<bool>("mirrored");
    QTest::newRow("canvas") << QPoint(8, 12) << QPoint(8, 12) << false;
    QTest::newRow("negative-layer-pixels") << QPoint(-4, -4) << QPoint(-4, -4) << false;
    QTest::newRow("mirrored-outside-canvas") << QPoint(8, 12) << QPoint(-520, 12) << true;
}

void KisGpuPaintImageTest::rectangularWritesPreserveOtherRegionsAcrossHistory()
{
    QFETCH(QPoint, source);
    QFETCH(QPoint, point);
    QFETCH(bool, mirrored);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
                         {QStringLiteral("Imported"), true, {}, {}}};
    const QPoint tile(int(std::floor(qreal(point.x()) / RgbaTile::Size)),
                      int(std::floor(qreal(point.y()) / RgbaTile::Size)));
    if (!tile.isNull()) definition.layers[0].tiles.append({tile, definition.layers[0].tiles.first().rgba});
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto pinned = image.operations().snapshot();
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    brush.diameter = 6;
    brush.mirrorHorizontal = mirrored;
    brush.mirrorCenter = {-256, 12};
    const auto stroke = image.beginStroke(1, brush, 0);
    QCOMPARE(image.appendSamples(stroke, {{QPointF(source), 1}}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    const auto notifications = changed.size();
    const auto raw = QByteArray::fromHex("120a2700");
    const auto green = QByteArray::fromHex("00ff00ff");
    QCOMPARE(image.writeRgbaPixels(2, {-1, -1, 1, 1}, raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, QRect(point, QSize(1, 1)), raw), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(changed.size(), notifications);
    QCOMPARE(image.operations().undoCount(), 0);
    QCOMPARE(image.operations().redoCount(), 1);
    QVERIFY(!image.operations().isModified());
    QCOMPARE(execution.readRgbaPixels(image.handle(), pinned, QRect(point, QSize(1, 1)), 1, 4).bytes, QByteArray::fromHex("ff0000ff"));
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, green), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), QRect(point, QSize(1, 1)), 1, 4).bytes,
             QByteArray::fromHex("0000ffff"));
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, green);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {-1, -1, 1, 1}, 2, 4).bytes, raw);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), QRect(point, QSize(1, 1)), 1, 4).bytes, raw);
    const auto yellow = QByteArray::fromHex("ffff00ff");
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, yellow), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, yellow);
    QCOMPARE(image.undo(), Result::Success);
    QVERIFY(!image.operations().isModified());
}

void KisGpuPaintImageTest::rectangularWritesValidateBeforeMutationAndCrossTileBoundaries()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 3 * TileRenderer::TileBytes, 1, TileRenderer::TileBytes);
    PaintImage image(execution, QSize(64, 64));
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto layer = image.operations().activeLayer();
    const auto before = image.operations().snapshot();
    const QRect region(-1, -1, 2, 2);
    const auto pixels = QByteArray::fromHex("120a2700197fafe00000ffffff000080");
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels.first(15)), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, -1, 2}, pixels), Result::InvalidInput);
    QCOMPARE(image.writeRgbaPixels(99, region, pixels), Result::LayerUnavailable);
    QCOMPARE(image.writeRgbaPixels(layer, {0, 0, 0, 0}, {}), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.requested(image.handle()), before);
    QCOMPARE(execution.beginReadbackFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Busy);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(execution.cancelFrame().code, ExecutionCode::Success);
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels + QByteArrayLiteral("ignored suffix")), Result::Success);
    const auto read = execution.readRgbaPixels(image.handle(), image.operations().snapshot(), region, layer, pixels.size());
    QCOMPARE(read.code, ExecutionCode::Success);
    QCOMPARE(read.bytes, pixels);
    QCOMPARE(image.operations().undoCount(), 0);
    QVERIFY(!image.operations().isModified());
    QVERIFY(changed.isEmpty());
    image.close();
    QCOMPARE(image.writeRgbaPixels(layer, region, pixels), Result::Closed);
}

void KisGpuPaintImageTest::rectangularHistoryEditsPreserveIndependentUntrackedPixels()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage definition;
    definition.size = {128, 128};
    definition.layers = {{QStringLiteral("Edited"), true, {},
                          {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}}};
    PaintImage image(execution, definition);
    QSignalSpy changed(&image, &PaintImage::changed);
    const auto blue = QByteArray::fromHex("0000ffff");
    const auto raw = QByteArray::fromHex("120a2700");
    QCOMPARE(image.replaceRgbaPixels(1, {8, 12, 1, 1}, blue), Result::Success);
    QCOMPARE(image.operations().undoCount(), 1);
    QVERIFY(image.operations().isModified());
    QCOMPARE(changed.size(), 1);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.writeRgbaPixels(1, {96, 96, 1, 1}, raw), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes, blue);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {8, 12, 1, 1}, 1, 4).bytes,
             QByteArray::fromHex("ff0000ff"));
    QCOMPARE(execution.readRgbaPixels(image.handle(), image.operations().snapshot(), {96, 96, 1, 1}, 1, 4).bytes, raw);
    QVERIFY(!image.operations().isModified());
}

void KisGpuPaintImageTest::autoBrushKeepsIndependentPixelWritesAcrossUndo()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    PaintImage image(execution, QSize(128, 128));
    Brush brush;
    brush.diameter = 40;
    auto &settings = brush.autoBrush.emplace();
    brush.sizeResponse = {PressureResponse::Kind::Constant, .1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    const auto id = image.beginStroke(1, brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(image.appendSamples(id, {{{50, 32}, 1}}), Result::Success);
    QCOMPARE(image.commitStroke(id), Result::Success);
    const auto independent = QByteArray::fromHex("112233ff");
    QCOMPARE(image.writeRgbaPixels(1, {70, 32, 1, 1}, independent), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, {70, 32, 1, 1}, 4).bytes, independent);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(1, {70, 32, 1, 1}, 4).bytes, independent);
}

void KisGpuPaintImageTest::contiguousFillKeepsFrozenPixelsHistoryAndClones()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device, 64 * 1024 * 1024);
    for (const auto format : {PixelFormat::Rgba8, PixelFormat::Rgba16}) {
        // Native edge smoothing retains a hard rectangle's right angles.
        // Both settings must keep frozen references through clone and history.
        for (const int threshold : {1, 8}) for (const bool antiAlias : {false, true}) {
            const quint16 maximum = maximumComponent(format);
            const int stride = pixelByteCount(format);
            const QRect area(0, 0, 270, 20);
            const RgbaPixel hidden{quint16(maximum / 3), 17, 39, quint16(format == PixelFormat::Rgba16 ? 127 : 0)};
            const RgbaPixel wall{3, 5, 7, maximum};
            QByteArray initial;
            for (int y = 0; y < area.height(); ++y) for (int x = 0; x < area.width(); ++x)
                initial += rgbaBytes(x == 0 || x == 130 || x == 269 || y == 0 || y == 19 ? wall : hidden, format);
            RasterImage raster; raster.size = area.size(); raster.format = format;
            raster.layers = {{QStringLiteral("Closed regions"), true, {}, {}}};
            PaintImage image(execution, raster);
            QCOMPARE(image.writeRgbaPixels(1, area, initial), Result::Success);
            const auto before = image.operations().snapshot();
            auto earlier = execution.readPixelsAsync(image.handle(), before, {0, 0, 1, 1}, {1}, false);
            RegionFillSettings settings;
            settings.seed = {3, 3}; settings.threshold = threshold; settings.antiAlias = antiAlias;
            const RgbaPixel color{quint16(maximum - 13), 173, 61, maximum};
            for (const int invalid : {-1, 401}) {
                settings.feather = invalid;
                QCOMPARE(image.fillRegion(1, settings, color, {}), Result::InvalidInput);
                QCOMPARE(image.operations().snapshot(), before);
            }
            settings.feather = 0;
            const auto filled = image.fillRegion(1, settings, color, {});
            QVERIFY2(filled == Result::Success, qPrintable(image.lastRequestOutcome().diagnostic + QStringLiteral("; ") + device->diagnostic()));
            QCOMPARE(earlier.get().code, ExecutionCode::Success);
            QCOMPARE(image.operations().undoCount(), 1);
            auto expected = initial;
            const auto bytes = rgbaBytes(color, format);
            for (int y = 1; y < 19; ++y) for (int x = 1; x < 130; ++x)
                std::memcpy(expected.data() + (y * area.width() + x) * stride, bytes.constData(), stride);
            auto pixels = image.readRgbaPixels(1, area, expected.size());
            QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
            QCOMPARE(pixels.bytes, expected);
            QCOMPARE(image.undo(), Result::Success);
            QCOMPARE(image.readRgbaPixels(1, area, initial.size()).bytes, initial);
            QCOMPARE(image.redo(), Result::Success);
            QCOMPARE(image.readRgbaPixels(1, area, initial.size()).bytes, expected);
            QCOMPARE(execution.readRgbaPixels(image.handle(), before, area, 1, initial.size()).bytes, initial);
            // A direct write changes old checkpoints too. The accepted region
            // must still refer to its original closed wall when replayed.
            QCOMPARE(image.writeRgbaPixels(1, {130, 3, 1, 1}, rgbaBytes(hidden, format)), Result::Success);
            std::memcpy(expected.data() + (3 * area.width() + 130) * stride, rgbaBytes(hidden, format).constData(), stride);
            QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, expected);
            Execution other(device, 64 * 1024 * 1024);
            auto clone = image.cloneTo(other);
            QVERIFY(clone);
            QCOMPARE(clone->readRgbaPixels(clone->operations().activeLayer(), area, expected.size()).bytes, expected);
            const auto pinned = image.operations().snapshot();
            const int count = image.operations().undoCount();
            settings.seed = {0, 0}; settings.threshold = 8;
            QCOMPARE(image.fillRegion(1, settings, color, {}), Result::InvalidInput);
            QCOMPARE(image.operations().snapshot(), pinned);
            QCOMPARE(image.operations().undoCount(), count);
            QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, expected);
            Brush brush;
            const auto input = image.beginStroke(1, brush, 17);
            QVERIFY(!input.isNull());
            QCOMPARE(image.appendSamples(input, {{{17, 9}, 1}}), Result::Success);
            const auto preview = image.operations().preview();
            QCOMPARE(image.fillRegion(1, settings, color, {}), Result::EditInProgress);
            QCOMPARE(image.operations().preview(), preview);
            QCOMPARE(image.cancelStroke(input), Result::Success);
            QCOMPARE(image.operations().undoCount(), count);
            QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, expected);
            Execution small(device, 1024 * 1024);
            PaintImage limited(small, raster);
            QCOMPARE(limited.writeRgbaPixels(1, area, initial), Result::Success);
            const auto retained = limited.operations().snapshot();
            settings.seed = {3, 3};
            QCOMPARE(limited.fillRegion(1, settings, color, {}), Result::CapacityExceeded);
            QCOMPARE(limited.operations().snapshot(), retained);
            QCOMPARE(limited.operations().undoCount(), 0);
            QCOMPARE(image.setSelectionRect({1, 1, 2, 2}), Result::Success);
            const auto excluded = image.operations().snapshot();
            const int history = image.operations().undoCount();
            settings.seed = {0, 0}; settings.threshold = 8;
            QCOMPARE(image.fillRegion(1, settings, color, {}), Result::EmptyOperation);
            QCOMPARE(image.operations().snapshot(), excluded);
            QCOMPARE(image.operations().undoCount(), history);
            QCOMPARE(image.readRgbaPixels(1, area, expected.size()).bytes, expected);
            if (format == PixelFormat::Rgba16 && threshold == 8) {
                // Native UINT16_TO_UINT8 maps 127 to zero and 128 to one.
                RasterImage visible; visible.size = {1, 1}; visible.format = format;
                visible.layers = {{QStringLiteral("First visible alpha"), true, {}, {}, {11, 39, 51, 128}}};
                PaintImage firstVisible(execution, visible);
                settings.seed = {0, 0};
                const auto fixed = firstVisible.operations().snapshot();
                QCOMPARE(firstVisible.fillRegion(1, settings, color, {}), Result::InvalidInput);
                QCOMPARE(firstVisible.operations().snapshot(), fixed);
                QCOMPARE(firstVisible.operations().undoCount(), 0);
            }
        }
    }
    // Ordinary frames keep the preceding canvas resident. Old fill recipes
    // whose tiles were reused must not allocate another full-region workspace.
    Execution warm(device, 8 * 1024 * 1024);
    RasterImage raster; raster.size = {16, 16}; raster.format = PixelFormat::Rgba16;
    raster.layers = {{QStringLiteral("Repeated ordinary fills"), true, {}, {}}};
    PaintImage repeated(warm, raster);
    const QRect area(QPoint(), raster.size);
    const auto frame = [&] {
        const auto begun = warm.beginReadbackFrame();
        if (begun.code != ExecutionCode::Success) return false;
        const auto projected = warm.project(repeated.handle(), area);
        if (projected.code != RenderOutcome::Recorded) { warm.cancelFrame(); return false; }
        return warm.submitFrame().code == ExecutionCode::Success;
    };
    QVERIFY(frame());
    RegionFillSettings settings; settings.seed = {5, 5}; settings.threshold = 1;
    RgbaPixel last;
    for (int i = 0; i < 32; ++i) {
        last = {quint16(i & 1 ? 61347 : 119), 37193, 913, 65535};
        QCOMPARE(repeated.fillRegion(1, settings, last, {}), Result::Success);
        QVERIFY2(frame(), qPrintable(device->diagnostic()));
    }
    QCOMPARE(repeated.operations().undoCount(), 32);
    const auto pixels = repeated.readRgbaPixels(1, area, 2048);
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, rgbaBytes(last, raster.format).repeated(256));
    const auto layerPixels = warm.readLayerPixels(repeated.handle(), repeated.operations().snapshot(), 1, RgbaTile::byteCount(raster.format));
    QVERIFY2(layerPixels.code == ExecutionCode::Success, qPrintable(layerPixels.diagnostic));
    const auto projection = warm.readProjectionImage(repeated.handle(), repeated.operations().snapshot(), 2048);
    QVERIFY2(projection.code == ExecutionCode::Success, qPrintable(projection.diagnostic));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(projection.image.constBits()), 2048), pixels.bytes);
}

void KisGpuPaintImageTest::filterNodeProjectionReadersFollowCoverageEdits_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("mask");
    for (bool words : {false, true}) for (bool mask : {false, true}) {
        const QByteArray row = QByteArray(words ? "word-" : "byte-") + (mask ? "mask" : "adjustment");
        QTest::newRow(row.constData()) << words << mask;
    }
}

void KisGpuPaintImageTest::filterNodeProjectionReadersFollowCoverageEdits()
{
    QFETCH(bool, words); QFETCH(bool, mask);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster;
    raster.size = {16, 12}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const quint16 scale = words ? 257 : 1;
    const RgbaPixel original{quint16(17 * scale), quint16(91 * scale), quint16(203 * scale), maximumComponent(raster.format)};
    const RgbaPixel filtered{0, 0, quint16(203 * scale), maximumComponent(raster.format)};
    RasterLayer paint; paint.name = QStringLiteral("Source"); paint.uuid = QUuid::createUuid();
    paint.defaultPixel = original; raster.layers.append(paint);
    PaintImage image(execution, raster);
    RasterLayer node;
    node.kind = mask ? LayerKind::FilterMask : LayerKind::Adjustment;
    node.name = QStringLiteral("Maximum"); node.parentUuid = mask ? paint.uuid : QUuid{};
    node.blendMode = mask ? LayerBlendMode::Normal : LayerBlendMode::Copy;
    FilterDefinition definition; definition.id = QStringLiteral("maximize");
    definition.xml = QByteArrayLiteral("<params version=\"1\"/>");
    definition.settings.kind = FilterKind::MaximizeChannel;
    node.filter = definition;
    const auto target = image.addFilterNode(node, 1); QVERIFY(target);
    const auto physical = mask ? LayerId(1) : target;
    const int stride = pixelByteCount(raster.format);
    const auto read = [&](const Snapshot &version, QPoint point) {
        return execution.readLayerProjectionPixels(image.handle(), version, physical, QRect(point, QSize(1, 1)), stride);
    };
    const auto before = image.operations().snapshot();
    auto pixels = read(before, {6, 6});
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, rgbaBytes(filtered, raster.format));
    const auto nativeProjection = execution.readNativeProjectionPixels(image.handle(), before, {6, 6, 1, 1}, stride);
    QVERIFY2(nativeProjection.code == ExecutionCode::Success, qPrintable(nativeProjection.diagnostic));
    QCOMPARE(nativeProjection.bytes, rgbaBytes(filtered, raster.format));
    QCOMPARE(image.readRgbaPixels(1, {6, 6, 1, 1}, stride).bytes, rgbaBytes(original, raster.format));
    Brush erase; erase.erase = true; erase.diameter = 8;
    const auto cancelled = image.beginTransparencyMaskStroke(target, erase, 17); QVERIFY(!cancelled.isNull());
    QCOMPARE(image.appendPoint(cancelled, {{6, 6}, 1}), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    pixels = read(image.operations().preview(), {6, 6});
    QVERIFY2(pixels.code == ExecutionCode::Success, qPrintable(pixels.diagnostic));
    QCOMPARE(pixels.bytes, rgbaBytes(original, raster.format));
    QCOMPARE(image.cancelStroke(cancelled), Result::Success);
    QCOMPARE(image.operations().snapshot(), before);
    QCOMPARE(read(before, {6, 6}).bytes, rgbaBytes(filtered, raster.format));
    const auto stroke = image.beginTransparencyMaskStroke(target, erase, 17); QVERIFY(!stroke.isNull());
    QCOMPARE(image.appendPoint(stroke, {{6, 6}, 1}), Result::Success);
    QCOMPARE(image.commitStroke(stroke), Result::Success);
    QCOMPARE(image.operations().undoCount(), 2);
    QCOMPARE(read(image.operations().snapshot(), {6, 6}).bytes, rgbaBytes(original, raster.format));
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(read(image.operations().snapshot(), {6, 6}).bytes, rgbaBytes(filtered, raster.format));
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(read(image.operations().snapshot(), {6, 6}).bytes, rgbaBytes(original, raster.format));
    QCOMPARE(image.readRgbaPixels(1, {6, 6, 1, 1}, stride).bytes, rgbaBytes(original, raster.format));
    QCOMPARE(image.setLayerVisible(target, false), Result::Success);
    QCOMPARE(image.readRgbaPixels(0, {0, 0, 1, 1}, stride).bytes, rgbaBytes(original, raster.format));
    QCOMPARE(read(image.operations().snapshot(), {0, 0}).bytes, rgbaBytes(mask ? original : filtered, raster.format));
    if (!mask) {
        QCOMPARE(image.setLayerVisible(target, true), Result::Success);
        QCOMPARE(image.setFilterSelection(target, SelectionMask::uniform(255)), Result::Success);
        QCOMPARE(image.addTransparencyMask(target), Result::Success);
        const auto paired = image.operations().snapshot();
        QUuid internalUuid, transparencyUuid;
        for (const auto &layer : paired->layers) if (layer.id == target) {
            internalUuid = layer.uuid; transparencyUuid = layer.transparencyMaskUuid;
        }
        QVERIFY(!internalUuid.isNull()); QVERIFY(!transparencyUuid.isNull());
        const auto coverage = [&](const Snapshot &version, const QUuid &identity) {
            for (const auto &layer : version->layers)
                if (layer.id == target) return editableMaskCoverage(layer, identity);
            return SelectionMaskSP{};
        };
        const auto alpha = [&](const Snapshot &version, const QUuid &identity) {
            return execution.readSelectionPixels(coverage(version, identity), {0, 0, 16, 12}, 192).bytes;
        };
        const auto internal = image.beginTransparencyMaskStroke(target, erase, 17, internalUuid);
        QVERIFY(!internal.isNull());
        QCOMPARE(image.appendPoint(internal, {{6, 6}, 1}), Result::Success);
        QCOMPARE(coverage(image.operations().preview(), transparencyUuid), coverage(paired, transparencyUuid));
        QCOMPARE(image.commitStroke(internal), Result::Success);
        const auto internalEdited = image.operations().snapshot();
        const auto erasedAlpha = alpha(internalEdited, internalUuid);
        QCOMPARE(erasedAlpha.size(), 192);
        QCOMPARE(quint8(erasedAlpha[6 * 16 + 6]), quint8(0));
        QCOMPARE(coverage(internalEdited, transparencyUuid), coverage(paired, transparencyUuid));
        QCOMPARE(image.undo(), Result::Success);
        QCOMPARE(image.operations().snapshot(), paired);
        const auto child = image.beginTransparencyMaskStroke(target, erase, 17, transparencyUuid);
        QVERIFY(!child.isNull());
        QCOMPARE(image.appendPoint(child, {{6, 6}, 1}), Result::Success);
        QCOMPARE(coverage(image.operations().preview(), internalUuid), coverage(paired, internalUuid));
        QCOMPARE(image.commitStroke(child), Result::Success);
        const auto childEdited = image.operations().snapshot();
        QCOMPARE(alpha(childEdited, transparencyUuid), erasedAlpha);
        QCOMPARE(coverage(childEdited, internalUuid), coverage(paired, internalUuid));
        QCOMPARE(alpha(internalEdited, internalUuid), erasedAlpha);
        QCOMPARE(image.readRgbaPixels(1, {6, 6, 1, 1}, stride).bytes, rgbaBytes(original, raster.format));
        QCOMPARE(image.undo(), Result::Success);
        QCOMPARE(image.operations().snapshot(), paired);
        QCOMPARE(image.redo(), Result::Success);
        QCOMPARE(image.operations().snapshot(), childEdited);
        const auto invalid = QUuid::createUuid();
        QVERIFY(image.beginTransparencyMaskStroke(target, erase, 17, invalid).isNull());
        QCOMPARE(image.clearMaskPixels(target, false, invalid), Result::LayerUnavailable);
        QCOMPARE(image.operations().snapshot(), childEdited);
    }
}

void KisGpuPaintImageTest::maskPixelWritesKeepCommandBlocks_data()
{ filterNodeProjectionReadersFollowCoverageEdits_data(); }

void KisGpuPaintImageTest::maskPixelWritesKeepCommandBlocks()
{
    QFETCH(bool, words); QFETCH(bool, mask);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster;
    raster.size = {192, 96}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    RasterLayer paint; paint.uuid = QUuid::createUuid(); raster.layers.append(paint);
    PaintImage image(execution, raster);
    RasterLayer effect;
    effect.uuid = QUuid::createUuid(); effect.name = QStringLiteral("Mask pixel target");
    effect.kind = mask ? LayerKind::FilterMask : LayerKind::Adjustment;
    effect.parentUuid = mask ? paint.uuid : QUuid{};
    effect.blendMode = LayerBlendMode::Copy;
    FilterDefinition definition; definition.id = "maximize";
    definition.settings.kind = FilterKind::MaximizeChannel;
    definition.xml = QByteArrayLiteral("<params version=\"1\"/>"); effect.filter = definition;
    const auto target = image.addFilterNode(effect, 1); QVERIFY(target);
    const auto coverage = [&](const Snapshot &version) {
        for (const auto &layer : version->layers)
            if (layer.id == target) return editableMaskCoverage(layer, effect.uuid);
        return SelectionMaskSP{};
    };
    const QRect area(0, 0, 128, 64);
    const auto bytes = [&](const Snapshot &version) {
        return execution.readSelectionPixels(coverage(version), area, area.width() * area.height()).bytes;
    };
    QCOMPARE(image.writeMaskPixels(target, effect.uuid, {8, 8, 1, 1}, QByteArray(1, char(160))), Result::Success);
    const auto initial = image.operations().snapshot();
    QByteArray original(area.width() * area.height(), char(255)); original[8 * area.width() + 8] = char(160);
    const auto initialActual = bytes(initial);
    if (initialActual != original) {
        const auto value = coverage(initial);
        const auto source = value->paintSource()->source;
        qInfo() << "direct mask defaults" << value->defaultCoverage() << source->layers.first().defaultPixel
                << "bounds" << value->bounds() << "operations" << source->layers.first().operations.size()
                << "written alpha" << quint8(initialActual[8 * area.width() + 8]);
    }
    QCOMPARE(initialActual, original);
    QCOMPARE(image.setSelectionRect({4, 4, 8, 8}), Result::Success);
    QCOMPARE(image.clearMaskPixels(target, true, effect.uuid), Result::Success);
    QCOMPARE(image.operations().historyAction(image.operations().undoCount()), HistoryAction::ClearPixels);
    const auto cleared = image.operations().snapshot();
    QByteArray input(area.width() * area.height(), Qt::Uninitialized);
    for (qsizetype i = 0; i < input.size(); ++i) input[i] = char((i * 37 + 11) % 256);
    const auto count = image.operations().undoCount();
    QSignalSpy changed(&image, &PaintImage::changed);
    QCOMPARE(image.writeMaskPixels(target, effect.uuid, area, input), Result::Success);
    QCOMPARE(bytes(image.operations().snapshot()), input);
    QCOMPARE(image.operations().undoCount(), count); QCOMPARE(changed.count(), 0);
    QCOMPARE(image.undo(), Result::Success);
    QByteArray undone = original;
    for (int y = 0; y < area.height(); ++y)
        undone.replace(y * area.width() + 64, 64, input.mid(y * area.width() + 64, 64));
    QCOMPARE(bytes(image.operations().snapshot()), undone);
    QCOMPARE(image.writeMaskPixels(target, effect.uuid, {40, 40, 1, 1}, QByteArray(1, char(9))), Result::Success);
    QCOMPARE(image.writeMaskPixels(target, effect.uuid, {90, 20, 1, 1}, QByteArray(1, char(42))), Result::Success);
    QCOMPARE(image.redo(), Result::Success);
    input[20 * area.width() + 90] = char(42);
    QCOMPARE(bytes(image.operations().snapshot()), input);
    QCOMPARE(bytes(initial), original);
    auto retained = coverage(image.operations().snapshot()); QVERIFY(retained);
    image.close();
    QCOMPARE(image.writeMaskPixels(0, QUuid::createUuid(), {100, 30, 1, 1}, QByteArray(1, char(73)), &retained, raster.size), Result::Success);
    input[30 * area.width() + 100] = char(73);
    QCOMPARE(execution.readSelectionPixels(retained, area, input.size()).bytes, input);
    QVERIFY(coverage(cleared));

    Limits limits; limits.rasterBytes = 32 * 1024;
    RasterImage small;
    small.size = {16, 16}; small.layers = {{QStringLiteral("Bounded mask")}};
    small.layers.first().transparencyMask = SelectionMask::uniform(255);
    small.layers.first().transparencyMaskUuid = QUuid::createUuid();
    PaintImage bounded(execution, small, limits);
    const auto beforeFailure = bounded.operations().snapshot();
    QCOMPARE(bounded.writeMaskPixels(1, beforeFailure->layers.first().transparencyMaskUuid,
        {3, 4, 1, 1}, QByteArray(1, char(51))), Result::CapacityExceeded);
    QCOMPARE(bounded.operations().snapshot(), beforeFailure);
}

void KisGpuPaintImageTest::adjustmentMergesFreezeTheOriginalLowerStack_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("down");
    for (bool words : {false, true}) for (bool down : {false, true}) {
        const QByteArray row = QByteArray(words ? "word-" : "byte-") + (down ? "down" : "flatten");
        QTest::newRow(row.constData()) << words << down;
    }
}

void KisGpuPaintImageTest::adjustmentMergesFreezeTheOriginalLowerStack()
{
    QFETCH(bool, words); QFETCH(bool, down);
    QString diagnostic;
    const auto device = Device::createNative(diagnostic); QVERIFY2(device, qPrintable(diagnostic));
    Execution execution(device);
    RasterImage raster;
    raster.size = {16, 12}; raster.format = words ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const quint16 scale = words ? 257 : 1;
    RasterLayer lower; lower.name = QStringLiteral("Outside merge"); lower.uuid = QUuid::createUuid();
    lower.defaultPixel = {quint16(31 * scale), quint16(133 * scale), quint16(59 * scale), maximumComponent(raster.format)};
    RasterLayer upper; upper.name = QStringLiteral("Inside merge"); upper.uuid = QUuid::createUuid();
    upper.defaultPixel = {quint16(199 * scale), quint16(47 * scale), quint16(13 * scale), quint16(128 * scale)};
    raster.layers = {lower, upper};
    PaintImage image(execution, raster);
    RasterLayer node; node.kind = LayerKind::Adjustment; node.name = QStringLiteral("Maximum");
    node.blendMode = LayerBlendMode::Copy;
    FilterDefinition definition; definition.id = QStringLiteral("maximize");
    definition.xml = QByteArrayLiteral("<params version=\"1\"/>"); definition.settings.kind = FilterKind::MaximizeChannel;
    node.filter = definition;
    const auto adjustment = image.addFilterNode(node, 2); QVERIFY(adjustment);
    const QRect area(QPoint(), raster.size);
    const int bytes = area.width() * area.height() * pixelByteCount(raster.format);
    const auto before = image.operations().snapshot();
    const auto baseline = image.readRgbaPixels(0, area, bytes);
    QVERIFY2(baseline.code == ExecutionCode::Success, qPrintable(baseline.diagnostic));
    LayerMergeRequest request;
    request.before = before; request.mode = down ? LayerMergeMode::Down : LayerMergeMode::FlattenLayer;
    request.layers = down ? QList<LayerId>{2, adjustment} : QList<LayerId>{adjustment};
    request.above = adjustment; request.name = QStringLiteral("Baked filter");
    request.homogeneousComposition = !down; request.destinationUuid = QUuid::createUuid();
    request.allocatedBlocks = {QRect(0, 0, 64, 64)};
    for (const auto id : request.layers) request.sourceBounds.insert(id, area);
    QCOMPARE(image.mergeLayers(request), Result::Success);
    const auto mergedId = image.operations().activeLayer();
    const auto merged = image.readRgbaPixels(0, area, bytes);
    QVERIFY2(merged.code == ExecutionCode::Success, qPrintable(merged.diagnostic));
    QCOMPARE(merged.bytes, baseline.bytes);
    QCOMPARE(image.readRgbaPixels(mergedId, area, bytes).bytes, baseline.bytes);
    QCOMPARE(image.fillLayerColor(1, {0, 0, maximumComponent(raster.format), maximumComponent(raster.format)}), Result::Success);
    QCOMPARE(image.readRgbaPixels(0, area, bytes).bytes, baseline.bytes);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.undo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(0, area, bytes).bytes, baseline.bytes);
    QCOMPARE(image.redo(), Result::Success);
    QCOMPARE(image.readRgbaPixels(0, area, bytes).bytes, baseline.bytes);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    KisGpuPaintImageTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuPaintImageTest.moc"
