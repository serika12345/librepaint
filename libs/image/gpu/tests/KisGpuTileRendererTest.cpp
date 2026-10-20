/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QThread>
#include <gpu/KisGpuTileRenderer.h>
#include <gpu/KisGpuDevice.h>
#include <gpu/KisGpuReadbackBatch_p.h>
#include <webgpu/wgpu.h>
#include "KisGpuLegacyRecoveryFixture_p.h"

#include "KisGpuMultiplyProjectionFixture_p.h"
#include "KisGpuScreenProjectionFixture_p.h"
#include "KisGpuMinMaxProjectionFixture_p.h"

using namespace LibrePaint::Image::Gpu;
using LibrePaint::Image::Gpu::Test::legacyRecovery;

class KisGpuTileRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void isolatedGroupsKeepProjectionAndRetainedSubtreePixels_data();
    void isolatedGroupsKeepProjectionAndRetainedSubtreePixels();
    void transparentLayerPixelsFollowRootProjectionSemantics_data();
    void transparentLayerPixelsFollowRootProjectionSemantics();
    void layerCountChangesInvalidateTransparentProjectionReuse();
    void layerOpacityChangesProjectionWithoutChangingSourcePixelsOrRetainedVersions();
    void projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels_data();
    void projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels();
    void partialLayerOpacityComposesOverTransparency_data();
    void partialLayerOpacityComposesOverTransparency();
    void normalProjectionPreservesBinary32Rounding_data();
    void normalProjectionPreservesBinary32Rounding();
    void explicitPassThroughReadAfterParentKeepsSubtreePixels();
    void passThroughHierarchyMatchesNativeRootPixels_data();
    void passThroughHierarchyMatchesNativeRootPixels();
    void separableLayerProjectionMatchesNativePixels_data();
    void separableLayerProjectionMatchesNativePixels();
    void threeLayerProjectionFitsOneOutputTile_data();
    void threeLayerProjectionFitsOneOutputTile();
    void previewProjectionTracksBackgroundPropertiesAndRetainedPixels();
    void switchingDocumentsReleasesPreviewDerivedPixelsAtCapacity();
    void roundBrushProducesRgbaPixelsOnTheGpu();
    void solidFiguresPreserveFillOutlineHistoryAndRecovery_data();
    void solidFiguresPreserveFillOutlineHistoryAndRecovery();
    void filledInteriorRespectsRegionAndTileBudget();
    void collinearFillPreservesPixelsWithinTileBudget_data();
    void collinearFillPreservesPixelsWithinTileBudget();
    void removingFigureCoverageRestoresItsPreviewBackground();
    void pathCommandsPreserveCoverageHistoryAndRecovery_data();
    void pathCommandsPreserveCoverageHistoryAndRecovery();
    void symmetryPreservesPixelsOverlapHistoryAndRecovery_data();
    void symmetryPreservesPixelsOverlapHistoryAndRecovery();
    void symmetryKeepsSparseTilesAndIncrementalInputWithinBudget();
    void channelProtectionPreservesPixelsHistoryAndRecovery_data();
    void channelProtectionPreservesPixelsHistoryAndRecovery();
    void alphaLockPreservesCoverageHistoryAndRecovery_data();
    void alphaLockPreservesCoverageHistoryAndRecovery();
    void inputPartitionsAndPreviewProduceTheSamePixels();
    void basicSmoothingPreservesCurvesIdleCompletionAndRecovery_data();
    void basicSmoothingPreservesCurvesIdleCompletionAndRecovery();
    void weightedSmoothingPreservesPixelsAndRecovery_data();
    void weightedSmoothingPreservesPixelsAndRecovery();
    void pixelCorrectionPreservesCornersSensorsAndIncrementalPixels_data();
    void pixelCorrectionPreservesCornersSensorsAndIncrementalPixels();
    void unchangedTilesAreSharedAndBudgetsRejectAtomically();
    void erasingAndLayerVisibilityPreservePixelMeaning();
    void longDiagonalUsesOnlyIntersectingTiles();
    void shapesUndoAndRecoveryRestoreTheSamePixels();
    void rectangleOutlinesPreserveRotationHistoryAndRecovery_data();
    void rectangleOutlinesPreserveRotationHistoryAndRecovery();
    void polylinesPreserveVerticesClosureHistoryAndRecovery_data();
    void polylinesPreserveVerticesClosureHistoryAndRecovery();
    void straightLinesPreserveIntermediatePressure_data();
    void straightLinesPreserveIntermediatePressure();
    void versionsOutliveTheRendererAndRejectAnotherDeviceOwner();
    void visibleLayersComposeOnTheGpuAndReuseUnchangedTiles();
    void compositionPreservesOrderAlphaAndExistingVersionsAtCapacity();
    void paintingAndCompositionShareOneSubmission();
    void longDocumentRebuildUsesBoundedTileStorage();
    void retainedPixelsOwnTheNativeDeviceUntilTheirLastReader();
    void nativeDeviceCreationRequiresTheApplicationThread();
    void rasterCheckpointsPreservePixelsThroughPaintingUndoAndRecovery();
    void overwrittenTileHistoryRebuildsWithinTwoBuffers();
    void tileWritesPreservePaintingOnOtherCoordinates();
    void implicitLayerPixelsStaySparseThroughEditsAndProjection_data();
    void implicitLayerPixelsStaySparseThroughEditsAndProjection();
    void implicitLayersRespectCompositionAndCapacity();
    void partialRenderingMatchesFullTilesAndRejectsIncompleteComposition();
    void changingPreviewReusesItsBackgroundWithinThreeTiles();
    void appendedInputPreservesPixelsOutsideItsChangedTiles();
    void appendedInputPreservesCoverageBackgroundAndOldVersions_data();
    void appendedInputPreservesCoverageBackgroundAndOldVersions();
    void pendingStrokeLeavesCapacityForCompositionAndCancellation();
    void layerCropSharesInteriorPreservesBoundaryAndRetainedPixels_data();
    void layerCropSharesInteriorPreservesBoundaryAndRetainedPixels();
    void layerCropRejectsBoundaryAllocationWithoutChangingRetainedPixels();

private:
    RenderedSnapshot render(TileRenderer &renderer, const Snapshot &snapshot,
                            const RenderedSnapshot &base = {});
    QByteArray readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate);
    QByteArray readBuffer(WGPUBuffer pixels);
    ProjectedSnapshot project(TileRenderer &renderer, const RenderedSnapshot &version,
                              QRect visible, const ProjectedSnapshot &base = {});
    DevicePtr m_device;
};

void KisGpuTileRendererTest::isolatedGroupsKeepProjectionAndRetainedSubtreePixels_data()
{
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("nested");
    QTest::addColumn<bool>("backdrop");
    QTest::addColumn<QByteArray>("rootPixel");
    QTest::addColumn<QByteArray>("groupPixel");
    QTest::newRow("isolated") << false << true << false << true << QByteArray::fromHex("80007fff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("uniform") << true << true << false << true << QByteArray::fromHex("80007fff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("hidden") << false << false << false << true << QByteArray::fromHex("0000ffff") << QByteArray::fromHex("ff0000ff");
    QTest::newRow("nested") << false << true << true << true << QByteArray::fromHex("4000bfff") << QByteArray::fromHex("ff000080");
    QTest::newRow("hidden-rgb") << false << true << false << false << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300");
    QTest::newRow("implicit-hidden-rgb") << true << true << false << false << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300");
}

void KisGpuTileRendererTest::isolatedGroupsKeepProjectionAndRetainedSubtreePixels()
{
    QFETCH(bool, implicit);
    QFETCH(bool, visible);
    QFETCH(bool, nested);
    QFETCH(bool, backdrop);
    QFETCH(QByteArray, rootPixel);
    QFETCH(QByteArray, groupPixel);
    RasterImage source;
    source.size = {64, 48};
    const QUuid inner(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid outer(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    const auto populate = [&](RasterLayer &layer, const QByteArray &pixel) {
        if (implicit) {
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixel[channel]);
        } else {
            const auto bytes = pixel.repeated(RgbaTile::ByteCount / 4);
            layer.tiles = {{{0, 0}, bytes}, {{1, 0}, bytes}};
        }
    };
    if (backdrop) {
        source.layers.append({QStringLiteral("Blue backdrop"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}});
        populate(source.layers.last(), QByteArray::fromHex("0000ffff"));
    }
    source.activeLayer = source.layers.size();
    source.layers.append({QStringLiteral("Nested pixels"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}});
    populate(source.layers.last(), backdrop ? QByteArray::fromHex("ff0000ff") : groupPixel);
    source.layers.last().parentUuid = inner;
    source.layers.append({QStringLiteral("Inner group"), visible, inner, {}});
    source.layers.last().kind = LayerKind::Group;
    source.layers.last().opacity = backdrop ? 128 : 255;
    if (nested) {
        source.layers.last().parentUuid = outer;
        source.layers.append({QStringLiteral("Outer group"), visible, outer, {}});
        source.layers.last().kind = LayerKind::Group;
        source.layers.last().opacity = 128;
    }
    const LayerId group = source.layers.size();
    OperationDocument document(source);
    TileRenderer renderer(m_device, 10 * TileRenderer::TileBytes);
    const auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    const QRect region(0, 0, 64, 48);
    const auto before = project(renderer, raw, region);
    QVERIFY(before);
    const auto projectedPixel = [&](const ProjectedSnapshot &version) {
        for (const auto &tile : version->tiles()) if (tile.coordinate == QPoint()) return readBuffer(tile.pixels).first(4);
        return QByteArray(4, '\0');
    };
    QCOMPARE(projectedPixel(before), rootPixel);
    if (!implicit) {
        QCOMPARE(readTile(raw, group, {0, 0}), groupPixel.repeated(RgbaTile::ByteCount / 4));
        QCOMPARE(readTile(raw, group, {1, 0}), groupPixel.repeated(RgbaTile::ByteCount / 4));
    }
    if (backdrop && visible && !nested) {
        QCOMPARE(document.setLayerOpacity(group, 64), Result::Success);
        const auto changed = render(renderer, document.snapshot(), raw);
        QVERIFY(changed);
        const auto after = project(renderer, changed, region, before);
        QVERIFY(after);
        QCOMPARE(projectedPixel(after), QByteArray::fromHex("4000bfff"));
        QCOMPARE(projectedPixel(before), rootPixel);
        if (!implicit) QCOMPARE(changed->tiles(group).first().pixels, raw->tiles(group).first().pixels);
    }
}

void KisGpuTileRendererTest::layerCropSharesInteriorPreservesBoundaryAndRetainedPixels_data()
{
    QTest::addColumn<QByteArray>("implicit");
    QTest::newRow("transparent") << QByteArray::fromHex("00000000");
    QTest::newRow("hidden-rgb") << QByteArray::fromHex("19334d00");
    QTest::newRow("partial-alpha") << QByteArray::fromHex("19334d49");
}

void KisGpuTileRendererTest::layerCropSharesInteriorPreservesBoundaryAndRetainedPixels()
{
    QFETCH(QByteArray, implicit);
    const auto raw = QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {768, 256};
    input.layers = {{QStringLiteral("Crop target"), true, {}, {}}};
    for (int i = 0; i < 4; ++i) input.layers[0].defaultPixel[i] = quint8(implicit[i]);
    for (int x = -2; x <= 2; ++x) input.layers[0].tiles.append({{x, 0}, raw});
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device);
    const auto before = render(renderer, document.snapshot());
    QVERIFY(before);
    const auto allocated = renderer.residentBytes();
    QCOMPARE(document.cropLayer(layer, {-256, 0, 776, 256}), Result::Success);
    const auto after = render(renderer, document.snapshot(), before);
    QVERIFY(after);
    QCOMPARE(after->tiles(layer).size(), 4);
    QCOMPARE(renderer.residentBytes() - allocated, qsizetype(TileRenderer::TileBytes));
    for (const auto &tile : after->tiles(layer)) {
        if (tile.coordinate.x() == 2) continue;
        const auto old = before->tiles(layer);
        const auto found = std::find_if(old.cbegin(), old.cend(), [&](const auto &value) { return value.coordinate == tile.coordinate; });
        QVERIFY(found != old.cend());
        QCOMPARE(tile.pixels, found->pixels);
    }
    auto expected = implicit.repeated(RgbaTile::ByteCount / 4);
    for (int y = 0; y < 256; ++y) std::memcpy(expected.data() + y * 256 * 4, raw.constData(), 8 * 4);
    QCOMPARE(readTile(after, layer, {2, 0}), expected);
    QCOMPARE(readTile(before, layer, {2, 0}), raw);
    QCOMPARE(readTile(before, layer, {-2, 0}), raw);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), after);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {2, 0}), raw);
    QCOMPARE(document.redo(), Result::Success);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    TileRenderer rebuilt(m_device);
    const auto restored = render(rebuilt, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {2, 0}), expected);
}

void KisGpuTileRendererTest::layerCropRejectsBoundaryAllocationWithoutChangingRetainedPixels()
{
    const auto raw = QByteArray::fromHex("112233ff").repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Crop target"), true, {}, {{{0, 0}, raw}}}};
    OperationDocument document(input);
    TileRenderer renderer(m_device, TileRenderer::TileBytes);
    const auto before = render(renderer, document.snapshot());
    QVERIFY(before);
    QCOMPARE(document.cropLayer(document.activeLayer(), {3, 4, 50, 60}), Result::Success);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto rejected = renderer.record(commands, document.snapshot(), before);
    QCOMPARE(rejected.code, RenderOutcome::CapacityExceeded);
    QVERIFY(!rejected.version);
    QVERIFY(rejected.diagnostic.contains(QStringLiteral("crop")));
    QVERIFY(m_device->submitCommands(true));
    QCOMPARE(readTile(before, document.activeLayer(), {0, 0}), raw);
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

void KisGpuTileRendererTest::symmetryKeepsSparseTilesAndIncrementalInputWithinBudget()
{
    OperationDocument document({1024, 1024});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.diameter = 8;
    brush.mirrorHorizontal = brush.mirrorVertical = true;
    brush.mirrorCenter = {512, 512};
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    const auto fixed = document.preview();
    TileRenderer limited(m_device, 3 * TileRenderer::TileBytes);
    QVERIFY(!render(limited, fixed));
    QCOMPARE(limited.residentBytes(), qsizetype(0));
    QCOMPARE(document.preview(), fixed);
    TileRenderer renderer(m_device);
    const auto first = render(renderer, fixed);
    QCOMPARE(first->tiles(layer).size(), 4);
    for (const QPoint point : {QPoint(0, 0), QPoint(3, 0), QPoint(0, 3), QPoint(3, 3)})
        QVERIFY(!readTile(first, layer, point).isEmpty());
    QCOMPARE(document.appendSamples(stroke, {{{300, 300}, 0.5}}), Result::Success);
    const auto incremental = render(renderer, document.preview(), first);
    const auto rebuilt = render(renderer, document.preview());
    QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
    for (const auto &tile : rebuilt->tiles(layer))
        QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(layer).isEmpty());
    QCOMPARE(first->document(), fixed);
}

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistoryAndRecovery_data()
{
    QTest::addColumn<quint8>("mirrorMode");
    QTest::addColumn<QPoint>("center");
    QTest::addColumn<QPointF>("position");
    QTest::addColumn<bool>("line");
    QTest::newRow("horizontal") << quint8(1) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("vertical") << quint8(2) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("both") << quint8(3) << QPoint(32, 32) << QPointF(16, 16) << false;
    QTest::newRow("moved-axes") << quint8(3) << QPoint(24, 30) << QPointF(16, 16) << false;
    QTest::newRow("outside-canvas") << quint8(1) << QPoint(0, 32) << QPointF(16, 16) << false;
    QTest::newRow("overlapping-axis") << quint8(1) << QPoint(32, 32) << QPointF(32, 16) << false;
    QTest::newRow("retained-line") << quint8(3) << QPoint(32, 32) << QPointF(16, 16) << true;
}

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistoryAndRecovery()
{
    QFETCH(quint8, mirrorMode);
    QFETCH(QPoint, center);
    QFETCH(QPointF, position);
    QFETCH(bool, line);
    Brush brush;
    brush.rgba = {200, 70, 30, 128};
    brush.diameter = 8;
    const auto draw = [&](OperationDocument &document, const Brush &settings) {
        const auto stroke = document.beginStroke(document.activeLayer(), settings, 7);
        if (document.appendSamples(stroke, {{position, 1}}) != Result::Success) return OperationId{};
        if (line) {
            if (document.appendSamples(stroke, {{position + QPointF(4, 4), 0.5}}) != Result::Success ||
                document.updateShape(stroke, {ShapeKind::Line, position, position + QPointF(4, 4), 0}) != Result::Success)
                return OperationId{};
        }
        return stroke;
    };
    OperationDocument plain({64, 64});
    QVERIFY(!draw(plain, brush).isNull());
    TileRenderer renderer(m_device);
    const auto original = readTile(render(renderer, plain.preview()), plain.activeLayer(), {0, 0});
    QByteArray expected(RgbaTile::ByteCount, '\0');
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        const auto pixel = original.mid((y * RgbaTile::Size + x) * 4, 4);
        if (!quint8(pixel[3])) continue;
        for (int reflection = 0; reflection < 4; ++reflection) {
            if ((reflection & mirrorMode) != reflection) continue;
            const int dx = reflection & 1 ? 2 * center.x() - x - 1 : x;
            const int dy = reflection & 2 ? 2 * center.y() - y - 1 : y;
            if (dx >= 0 && dx < 64 && dy >= 0 && dy < 64)
                expected.replace((dy * RgbaTile::Size + dx) * 4, 4, pixel);
        }
    }
    brush.mirrorHorizontal = mirrorMode & 1;
    brush.mirrorVertical = mirrorMode & 2;
    brush.mirrorCenter = center;
    OperationDocument document({64, 64});
    const auto stroke = draw(document, brush);
    QVERIFY(!stroke.isNull());
    const auto pixels = readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0});
    QCOMPARE(pixels, expected);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last())->brush, brush);
    QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("symmetry-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
}

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistoryAndRecovery_data()
{
    QTest::addColumn<quint8>("writeMask");
    QTest::addColumn<quint8>("baseAlpha");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("alpha-transparent") << quint8(7) << quint8(0) << false << QByteArray::fromHex("4b328100");
    QTest::newRow("alpha-translucent") << quint8(7) << quint8(128) << false << QByteArray::fromHex("4b328180");
    QTest::newRow("protected-erase") << quint8(7) << quint8(128) << true << QByteArray::fromHex("0a28b480");
    QTest::newRow("red-only") << quint8(1) << quint8(128) << false << QByteArray::fromHex("4b28b480");
    QTest::newRow("alpha-only") << quint8(8) << quint8(128) << false << QByteArray::fromHex("0a28b4ab");
    QTest::newRow("green-and-alpha") << quint8(10) << quint8(128) << false << QByteArray::fromHex("0a37b4ab");
    QTest::newRow("all-protected") << quint8(0) << quint8(255) << false << QByteArray::fromHex("0a28b4ff");
}

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistoryAndRecovery()
{
    QFETCH(quint8, writeMask);
    QFETCH(quint8, baseAlpha);
    QFETCH(bool, erase);
    QFETCH(QByteArray, expected);
    QByteArray pixel = QByteArray::fromHex("0a28b400");
    pixel[3] = char(baseAlpha);
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Protected channels"), true, {},
                     {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    QCOMPARE(document.writeLayerChannelMask(layer, writeMask), Result::Success);
    Brush brush;
    brush.rgba = {200, 70, 30, 128};
    brush.diameter = 8;
    brush.opacity = 173;
    brush.erase = erase;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto pixels = readTile(render(renderer, document.preview()), layer, {0, 0});
    QCOMPARE(pixels.mid((32 * RgbaTile::Size + 32) * 4, 4), expected);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.preview()->layers.first().channelWriteMask, writeMask);
    QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("channel-protection-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}).mid((32 * RgbaTile::Size + 32) * 4, 4), pixel);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistoryAndRecovery_data()
{
    QTest::addColumn<quint8>("baseAlpha");
    QTest::addColumn<bool>("erase");
    QTest::newRow("transparent") << quint8(0) << false;
    QTest::newRow("almost-transparent") << quint8(1) << false;
    QTest::newRow("translucent") << quint8(128) << false;
    QTest::newRow("opaque") << quint8(255) << false;
    QTest::newRow("erase-translucent") << quint8(128) << true;
    QTest::newRow("erase-transparent") << quint8(0) << true;
}

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistoryAndRecovery()
{
    QFETCH(quint8, baseAlpha);
    QFETCH(bool, erase);
    QByteArray pixel = baseAlpha ? QByteArray::fromHex("0a28b400") : QByteArray(4, '\0');
    pixel[3] = char(baseAlpha);
    const auto background = pixel.repeated(RgbaTile::ByteCount / 4);
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Protected alpha"), true, {}, {{{0, 0}, background}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {200, 70, 30, 128};
    brush.diameter = 8;
    brush.opacity = 173;
    brush.erase = erase;
    brush.alphaLocked = true;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    auto expected = pixel;
    const quint32 alpha = (128u * 173u + 127u) / 255u;
    if (erase) expected[3] = char((baseAlpha * (255u - alpha) + 127u) / 255u);
    else {
        for (int channel = 0; channel < 3; ++channel)
            expected[channel] = char((brush.rgba[channel] * alpha + quint8(pixel[channel]) * (255u - alpha) + 127u) / 255u);
    }
    QCOMPARE(pixels.mid((32 * RgbaTile::Size + 32) * 4, 4), expected);
    QCOMPARE(pixels.left(4), pixel);
    if (!erase) {
        for (qsizetype offset = 3; offset < pixels.size(); offset += 4)
            QCOMPARE(quint8(pixels[offset]), baseAlpha);
    }
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto retained = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    QCOMPARE(retained->brush, brush);
    QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    const auto previous = legacyRecovery(document, 11);
    const auto before = restored.preview();
    QCOMPARE(restored.restoreRecovery(previous), Result::InvalidRecovery);
    QCOMPARE(restored.preview(), before);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("alpha-lock-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), background);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    if (baseAlpha == 0 && !erase) {
        QVERIFY(document.addLayer(QStringLiteral("Empty upper layer")));
        const auto withUpper = render(renderer, document.snapshot());
        const auto projection = project(renderer, withUpper, QRect(0, 0, 64, 64));
        QVERIFY(projection && !projection->tiles().isEmpty());
        QCOMPARE(readBuffer(projection->tiles().first().pixels).mid((32 * RgbaTile::Size + 32) * 4, 4), QByteArray(4, '\0'));
        QCOMPARE(readTile(withUpper, layer, {0, 0}), pixels);
    }
}

void KisGpuTileRendererTest::straightLinesPreserveIntermediatePressure_data()
{
    QTest::addColumn<bool>("offAxis");
    QTest::newRow("collinear") << false;
    QTest::newRow("projected") << true;
}

void KisGpuTileRendererTest::straightLinesPreserveIntermediatePressure()
{
    QFETCH(bool, offAxis);
    Brush brush;
    brush.diameter = 20;
    brush.rgba = {17, 129, 241, 255};
    OperationDocument document({256, 256});
    const auto layer = document.addLayer(QStringLiteral("Pressure line"));
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 64}, 0.15}, {{128, offAxis ? 100.0 : 64.0}, 1}, {{224, 64}, 0.15}}), Result::Success);
    TileRenderer renderer(m_device);
    for (const int y : {64, 128}) {
        OperationDocument expected({256, 256});
        const auto expectedLayer = expected.addLayer(QStringLiteral("Pressure samples"));
        const auto expectedStroke = expected.beginStroke(expectedLayer, brush, 0);
        QCOMPARE(expected.appendSamples(expectedStroke, {{{32, qreal(y)}, 0.15}, {{128, qreal(y)}, 1}, {{224, qreal(y)}, 0.15}}), Result::Success);
        const auto reference = render(renderer, expected.preview());
        QVERIFY(reference);
        QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {32, qreal(y)}, {224, qreal(y)}}), Result::Success);
        const auto line = render(renderer, document.preview());
        QVERIFY(line);
        QCOMPARE(readTile(line, layer, {0, 0}), readTile(reference, expectedLayer, {0, 0}));
    }
}

void KisGpuTileRendererTest::transparentLayerPixelsFollowRootProjectionSemantics_data()
{
    QTest::addColumn<bool>("composite");
    QTest::addColumn<bool>("topPixels");
    QTest::addColumn<bool>("whiteBackground");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("direct-layer") << false << false << false << QByteArray::fromHex("11223300");
    QTest::newRow("empty-upper-layer") << true << false << false << QByteArray::fromHex("00000000");
    QTest::newRow("two-transparent-layers") << true << true << false << QByteArray::fromHex("00000000");
    QTest::newRow("transparent-white-background") << true << true << true << QByteArray::fromHex("ffffff00");
}

void KisGpuTileRendererTest::transparentLayerPixelsFollowRootProjectionSemantics()
{
    QFETCH(bool, composite);
    QFETCH(bool, topPixels);
    QFETCH(bool, whiteBackground);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    if (whiteBackground) input.projectionBackground = {255, 255, 255, 0};
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4)}}}};
    if (composite) input.layers.append({QStringLiteral("Upper"), true, {}, {}});
    if (topPixels) input.layers.last().tiles = {{{0, 0}, QByteArray::fromHex("44556600").repeated(RgbaTile::ByteCount / 4)}};
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    const auto pixels = render(renderer, document.snapshot());
    QVERIFY(pixels);
    const auto projection = project(renderer, pixels, QRect(0, 0, 256, 256));
    QVERIFY(projection);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).left(4), expected);
    QCOMPARE(readTile(pixels, 1, {0, 0}).left(4), QByteArray::fromHex("11223300"));
}

void KisGpuTileRendererTest::layerCountChangesInvalidateTransparentProjectionReuse()
{
    RasterImage input;
    input.size = {256, 256};
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11223300").repeated(RgbaTile::ByteCount / 4)}}}};
    OperationDocument document(input);
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const QRect region(0, 0, 256, 256);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto single = project(renderer, original, region);
    QVERIFY(single);
    QCOMPARE(readBuffer(single->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QVERIFY(document.addLayer(QStringLiteral("Upper")));
    const auto expanded = render(renderer, document.snapshot(), original);
    QVERIFY(expanded);
    const auto composite = project(renderer, expanded, region, single);
    QVERIFY(composite);
    QCOMPARE(readBuffer(composite->tiles().first().pixels).left(4), QByteArray::fromHex("00000000"));
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = render(renderer, document.snapshot(), expanded);
    QVERIFY(restored);
    const auto direct = project(renderer, restored, region, composite);
    QVERIFY(direct);
    QCOMPARE(readBuffer(direct->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QCOMPARE(readBuffer(single->tiles().first().pixels).left(4), QByteArray::fromHex("11223300"));
    QCOMPARE(readBuffer(composite->tiles().first().pixels).left(4), QByteArray::fromHex("00000000"));
    QVERIFY(renderer.residentBytes() <= 2 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::normalProjectionPreservesBinary32Rounding_data()
{
    QTest::addColumn<int>("opacity");
    QTest::addColumn<QByteArray>("expectedHash");
    // Reference: production RGBA8 source-over at 5cd3e87, all byte-alpha
    // pairs, BGR source (18,52,86), destination (51,102,153), transparent
    // destination RGB normalized to zero. Scalar and batched ARM64/NEON64
    // and x86_64 results agree. Hashes cover the complete straight RGBA tile.
    struct Case { int opacity; const char *hash; };
    const Case cases[] = {
        {1, "397ded9ba63a06e74ed56775eb1dfd7b823621913fbe6034fbbf31e9ea8e5c82"},
        {128, "c6633dd35c638b9d271d776fe77e400f84c7ee2863d939e16489ac806974fad6"},
        {254, "91359ff33552e7bc841cbcc1b8fdcd9c0a9a9e34eae84643488544529a992b2a"},
        {255, "181798e20485cea5823ec6c209ccf4e9963cff73ec1d92cf1566beee4ba0dab4"},
    };
    for (const auto &c : cases) {
        const QByteArray name = QByteArray::number(c.opacity);
        QTest::newRow(name.constData()) << c.opacity << QByteArray(c.hash);
    }
}

void KisGpuTileRendererTest::normalProjectionPreservesBinary32Rounding()
{
    QFETCH(int, opacity);
    QFETCH(QByteArray, expectedHash);
    QByteArray lower(RgbaTile::ByteCount, '\0'), upper(RgbaTile::ByteCount, '\0');
    for (int destinationAlpha = 0; destinationAlpha < 256; ++destinationAlpha) {
        for (int sourceAlpha = 0; sourceAlpha < 256; ++sourceAlpha) {
            const auto offset = (destinationAlpha * 256 + sourceAlpha) * 4;
            lower[offset] = '\x99'; lower[offset + 1] = '\x66'; lower[offset + 2] = '\x33';
            lower[offset + 3] = char(destinationAlpha);
            upper[offset] = '\x56'; upper[offset + 1] = '\x34'; upper[offset + 2] = '\x12';
            upper[offset + 3] = char(sourceAlpha);
        }
    }
    RasterImage source;
    source.size = {256, 256};
    source.layers = {{QStringLiteral("Destination"), true, {}, {{{0, 0}, lower}}},
                     {QStringLiteral("Source"), true, {}, {{{0, 0}, upper}}}};
    source.layers.last().opacity = quint8(opacity);
    OperationDocument document(source);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto projected = project(renderer, rendered, QRect(0, 0, 256, 256));
    QVERIFY(projected);
    QByteArray actual;
    for (const auto &tile : projected->tiles()) if (tile.coordinate == QPoint()) actual = readBuffer(tile.pixels);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile artifact(QDir(directory).filePath(QStringLiteral("normalized-opacity-%1.rgba").arg(opacity)));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(actual), actual.size());
    }
    QCOMPARE(actual.size(), RgbaTile::ByteCount);
    QCOMPARE(QCryptographicHash::hash(actual, QCryptographicHash::Sha256).toHex(), expectedHash);
}

void KisGpuTileRendererTest::explicitPassThroughReadAfterParentKeepsSubtreePixels()
{
    RasterImage input;
    input.size = {4, 4};
    const QUuid inner(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid outer(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    input.layers = {{QStringLiteral("First"), true, {},
                     {{{0, 0}, QByteArray::fromHex("c9295d89").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Second"), true, {},
                     {{{0, 0}, QByteArray::fromHex("11d34dbd").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Pass-through"), true, inner, {}},
                    {QStringLiteral("Isolated parent"), true, outer, {}}};
    input.layers[0].parentUuid = input.layers[1].parentUuid = inner;
    input.layers[0].opacity = 153;
    input.layers[1].opacity = 211;
    input.layers[2].kind = input.layers[3].kind = LayerKind::Group;
    input.layers[2].parentUuid = outer;
    input.layers[2].passThrough = true;
    input.layers[2].opacity = 137;
    input.layers[2].projectionChannelMask = 9;
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    auto commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto parent = renderer.recordRegion(commands, document.snapshot(), {0, 0, 4, 4}, {4});
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY2(parent.code == RenderOutcome::Recorded, qPrintable(parent.diagnostic));
    const auto retained = readTile(parent.version, 4, {});
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto both = renderer.recordRegion(commands, document.snapshot(), {0, 0, 4, 4}, {4, 3}, parent.version);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY2(both.code == RenderOutcome::Recorded, qPrintable(both.diagnostic));
    QCOMPARE(readTile(both.version, 3, {}).first(4), QByteArray::fromHex("30b650bc"));
    QCOMPARE(readTile(parent.version, 4, {}), retained);
}

void KisGpuTileRendererTest::passThroughHierarchyMatchesNativeRootPixels_data()
{
    QTest::addColumn<bool>("passThrough");
    QTest::addColumn<int>("nesting");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<int>("mask");
    QTest::addColumn<bool>("uniform");
    QTest::addColumn<QByteArray>("expected");
    // Native RGBA8/sRGB root references at 6e856978f0. Four-pixel square,
    // fixed lower/first/second RGBA:255997ad/c9295d89/11d34dbd;
    // child opacities153/211, inner opacity171 and component mask6.
    struct Case { bool passThrough; int nesting, opacity, mask; const char *rgba; };
    const Case cases[] = {
        {false, 0, 255, 15, "2ea45ee9"},
        {false, 0, 255, 9, "2e5997e9"},
        {false, 0, 137, 15, "2a8774cd"},
        {false, 0, 137, 9, "2a5997cd"},
        {false, 0, 0, 15, "255997ad"},
        {false, 0, 0, 9, "255997ad"},
        {false, 1, 255, 15, "68627cc7"},
        {false, 1, 255, 9, "685997c7"},
        {false, 1, 137, 15, "4c5e88bb"},
        {false, 1, 137, 9, "4c5997bb"},
        {false, 1, 0, 15, "255997ad"},
        {false, 1, 0, 9, "255997ad"},
        {false, 2, 255, 15, "68627cc7"},
        {false, 2, 255, 9, "685997c7"},
        {false, 2, 137, 15, "4c5e88bb"},
        {false, 2, 137, 9, "4c5997bb"},
        {false, 2, 0, 15, "255997ad"},
        {false, 2, 0, 9, "255997ad"},
        {true, 0, 255, 15, "2ea45de9"},
        {true, 0, 255, 9, "2e5997e9"},
        {true, 0, 137, 15, "348371d1"},
        {true, 0, 137, 9, "345997d1"},
        {true, 0, 0, 15, "255997ad"},
        {true, 0, 0, 9, "255997ad"},
        {true, 1, 255, 15, "697f6ac7"},
        {true, 1, 255, 9, "695997c7"},
        {true, 1, 137, 15, "4c6b7cbb"},
        {true, 1, 137, 9, "4c5997bb"},
        {true, 1, 0, 15, "255997ad"},
        {true, 1, 0, 9, "255997ad"},
        {true, 2, 255, 15, "697f6bc7"},
        {true, 2, 255, 9, "698b79c7"},
        {true, 2, 137, 15, "4c6b7cbb"},
        {true, 2, 137, 9, "4c7487bb"},
        {true, 2, 0, 15, "255997ad"},
        {true, 2, 0, 9, "255997ad"},
    };
    for (const auto &c : cases) for (bool uniform : {false, true}) {
        const QByteArray name = QByteArray::number(c.passThrough) + '-' + QByteArray::number(c.nesting) + '-' +
            QByteArray::number(c.opacity) + '-' + QByteArray::number(c.mask) + (uniform ? "-default" : "-tile");
        QTest::newRow(name.constData()) << c.passThrough << c.nesting << c.opacity << c.mask << uniform
                                      << QByteArray::fromHex(c.rgba);
    }
}

void KisGpuTileRendererTest::passThroughHierarchyMatchesNativeRootPixels()
{
    QFETCH(bool, passThrough);
    QFETCH(int, nesting);
    QFETCH(int, opacity);
    QFETCH(int, mask);
    QFETCH(bool, uniform);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {4, 4};
    const QUuid outer(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid inner(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    input.layers = {{QStringLiteral("Bottom"), true, {}, {}},
                    {QStringLiteral("First"), true, {}, {}},
                    {QStringLiteral("Second"), true, {}, {}},
                    {QStringLiteral("Outer"), true, outer, {}}};
    input.layers[1].opacity = 153;
    input.layers[1].parentUuid = outer;
    input.layers[2].opacity = 211;
    input.layers[2].parentUuid = nesting ? inner : outer;
    input.layers[3].kind = LayerKind::Group;
    input.layers[3].passThrough = passThrough;
    input.layers[3].opacity = opacity;
    input.layers[3].projectionChannelMask = mask;
    if (nesting) {
        RasterLayer group{QStringLiteral("Inner"), true, inner, {}};
        group.kind = LayerKind::Group;
        group.parentUuid = outer;
        group.passThrough = nesting == 2;
        group.opacity = 171;
        group.projectionChannelMask = 6;
        input.layers.append(group);
    }
    const QByteArray colors[] = {QByteArray::fromHex("255997ad"), QByteArray::fromHex("c9295d89"),
                                 QByteArray::fromHex("11d34dbd")};
    for (int index = 0; index < 3; ++index) {
        if (uniform) std::copy_n(reinterpret_cast<const quint8 *>(colors[index].constData()), 4, input.layers[index].defaultPixel.begin());
        else input.layers[index].tiles = {{{0, 0}, colors[index].repeated(RgbaTile::ByteCount / 4)}};
    }
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    const auto pixels = render(renderer, document.snapshot());
    QVERIFY(pixels);
    const auto projection = project(renderer, pixels, {0, 0, 4, 4});
    QVERIFY(projection);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).first(4), expected);
    QCOMPARE(document.setLayerVisible(4, false), Result::Success);
    const auto hidden = project(renderer, render(renderer, document.snapshot(), pixels), {0, 0, 4, 4}, projection);
    QVERIFY(hidden);
    QCOMPARE(readBuffer(hidden->tiles().first().pixels).first(4), colors[0]);
    QCOMPARE(readBuffer(projection->tiles().first().pixels).first(4), expected);
}

void KisGpuTileRendererTest::separableLayerProjectionMatchesNativePixels_data()
{
    QTest::addColumn<int>("destinationAlpha");
    QTest::addColumn<int>("sourceAlpha");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<int>("mask");
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<int>("blendMode");
    for (int mode : {1, 2, 3, 4}) {
        const auto references = mode == 1 ? LibrePaint::Image::Gpu::Test::multiplyProjectionReferences() :
            mode == 2 ? LibrePaint::Image::Gpu::Test::screenProjectionReferences() :
            mode == 3 ? LibrePaint::Image::Gpu::Test::darkenProjectionReferences() :
            LibrePaint::Image::Gpu::Test::lightenProjectionReferences();
        qsizetype row = 0;
        for (int destinationAlpha : {0, 173, 255}) for (int sourceAlpha : {0, 137, 255})
        for (int opacity : {137, 255}) for (int mask : {15, 9, 6, 0}) {
            for (bool implicit : {false, true}) {
                const QByteArray name = QByteArray::number(mode) + '-' + QByteArray::number(destinationAlpha) + '-' + QByteArray::number(sourceAlpha) + '-' +
                    QByteArray::number(opacity) + '-' + QByteArray::number(mask) + (implicit ? "-default" : "-stored");
                QTest::newRow(name.constData()) << destinationAlpha << sourceAlpha << opacity << mask << implicit << references.mid(row * 4, 4) << mode;
            }
            ++row;
        }
    }
}

void KisGpuTileRendererTest::separableLayerProjectionMatchesNativePixels()
{
    QFETCH(int, blendMode);
    QFETCH(int, destinationAlpha);
    QFETCH(int, sourceAlpha);
    QFETCH(int, opacity);
    QFETCH(int, mask);
    QFETCH(bool, implicit);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    const std::array<quint8, 4> colors[] = {{37, 89, 151, quint8(destinationAlpha)}, {201, 41, 93, quint8(sourceAlpha)}};
    for (int index = 0; index < 2; ++index) {
        RasterLayer layer;
        layer.opacity = index == 0 ? 255 : opacity;
        layer.blendMode = LayerBlendMode::Normal;
        layer.projectionChannelMask = index == 0 ? 15 : mask;
        if (implicit) layer.defaultPixel = colors[index];
        else layer.tiles = {{{0, 0}, QByteArray(reinterpret_cast<const char *>(colors[index].data()), 4).repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    OperationDocument document(input);
    QCOMPARE(document.writeLayerBlendMode(2, LayerBlendMode(blendMode)), Result::Success);
    TileRenderer renderer(m_device);
    const auto raw = render(renderer, document.snapshot());
    QVERIFY(raw);
    const auto projected = project(renderer, raw, {0, 0, 256, 256});
    QVERIFY(projected);
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(document.setLayerBlendMode(2, LayerBlendMode::Normal), Result::Success);
    const auto normal = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, projected);
    QVERIFY(normal);
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = project(renderer, render(renderer, document.snapshot(), raw), {0, 0, 256, 256}, normal);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(readBuffer(projected->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
}

void KisGpuTileRendererTest::threeLayerProjectionFitsOneOutputTile_data()
{
    QTest::addColumn<int>("bottomAlpha");
    QTest::addColumn<int>("middleMask");
    QTest::addColumn<int>("topMask");
    QTest::addColumn<QByteArray>("expected");
    // RGBA8/sRGB CPU normal-composition references from 6e856978f0.
    // Three 256x256 uniform layers, opacity 255/137/211, bottom-to-top.
    QTest::newRow("transparent-normal") << 0 << 15 << 15 << QByteArray::fromHex("2db94fb9");
    QTest::newRow("transparent-middle-channels") << 0 << 9 << 15 << QByteArray::fromHex("2db241b9");
    QTest::newRow("transparent-inherit-alpha") << 0 << 15 << 6 << QByteArray::fromHex("c991534a");
    QTest::newRow("transparent-masked-bottom") << 0 << 0 << 9 << QByteArray::fromHex("1100009c");
    QTest::newRow("normal") << 173 << 15 << 15 << QByteArray::fromHex("2ca55ee9");
    QTest::newRow("middle-channels") << 173 << 9 << 15 << QByteArray::fromHex("2cab65e9");
    QTest::newRow("inherit-alpha") << 173 << 15 << 6 << QByteArray::fromHex("629d61c5");
    QTest::newRow("masked-bottom") << 173 << 0 << 9 << QByteArray::fromHex("175997df");
}

void KisGpuTileRendererTest::threeLayerProjectionFitsOneOutputTile()
{
    QFETCH(int, bottomAlpha);
    QFETCH(int, middleMask);
    QFETCH(int, topMask);
    QFETCH(QByteArray, expected);
    RasterImage input;
    input.size = {256, 256};
    QByteArray colors[] = {QByteArray::fromHex("25599700"), QByteArray::fromHex("c9295d89"), QByteArray::fromHex("11d34dbd")};
    colors[0][3] = char(bottomAlpha);
    for (int index = 0; index < 3; ++index) {
        RasterLayer layer;
        layer.opacity = index == 0 ? 255 : index == 1 ? 137 : 211;
        layer.projectionChannelMask = index == 0 ? 15 : index == 1 ? middleMask : topMask;
        layer.tiles = {{{0, 0}, colors[index].repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    OperationDocument document(input);
    TileRenderer reference(m_device);
    const auto full = project(reference, render(reference, document.snapshot()), {0, 0, 256, 256});
    QVERIFY(full);
    QCOMPARE(readBuffer(full->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    TileRenderer limited(m_device, 4 * TileRenderer::TileBytes);
    const auto layers = render(limited, document.snapshot());
    QVERIFY(layers);
    const auto composed = project(limited, layers, {0, 0, 256, 256});
    QVERIFY2(composed, "Three source tiles must leave one output tile for projection");
    QCOMPARE(readBuffer(composed->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
    for (int index = 0; index < 3; ++index)
        QCOMPARE(readTile(layers, document.snapshot()->layers[index].id, {}), input.layers[index].tiles.first().rgba);
    QVERIFY(limited.residentBytes() <= 4 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::previewProjectionTracksBackgroundPropertiesAndRetainedPixels()
{
    RasterImage input;
    input.size = {64, 64};
    for (const auto &rgba : {"255997ad", "c9295d89", "11d34dbd"}) {
        RasterLayer layer;
        layer.tiles = {{{0, 0}, QByteArray::fromHex(rgba).repeated(RgbaTile::ByteCount / 4)}};
        input.layers.append(layer);
    }
    input.activeLayer = 2;
    OperationDocument document(input);
    Brush brush;
    brush.rgba = {73, 149, 231, 128};
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 23);
    TileRenderer renderer(m_device);
    RenderedSnapshot rendered;
    ProjectedSnapshot projected, retained;
    QByteArray retainedPixels;
    for (int step = 0; step < 5; ++step) {
        QCOMPARE(document.appendSamples(stroke, {{{16.0 + step * 4, 24}, 1}}), Result::Success);
        if (step == 2) QCOMPARE(document.writeLayerOpacity(document.snapshot()->layers.first().id, 137), Result::Success);
        if (step == 3) QCOMPARE(document.writeLayerProjectionMask(document.snapshot()->layers[1].id, 9), Result::Success);
        rendered = render(renderer, document.preview(), rendered);
        QVERIFY(rendered);
        projected = project(renderer, rendered, {0, 0, 64, 64}, projected);
        QVERIFY(projected);
        const auto actual = readBuffer(projected->tiles().first().pixels);
        TileRenderer independent(m_device);
        const auto fresh = project(independent, render(independent, document.preview()), {0, 0, 64, 64});
        QVERIFY(fresh);
        QCOMPARE(actual, readBuffer(fresh->tiles().first().pixels));
        if (!retained) {
            retained = projected;
            retainedPixels = actual;
        }
        QCOMPARE(readBuffer(retained->tiles().first().pixels), retainedPixels);
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto committed = project(renderer, render(renderer, document.snapshot(), rendered), {0, 0, 64, 64}, projected);
    QVERIFY(committed);
    QCOMPARE(readBuffer(committed->tiles().first().pixels), readBuffer(projected->tiles().first().pixels));
}

void KisGpuTileRendererTest::switchingDocumentsReleasesPreviewDerivedPixelsAtCapacity()
{
    const auto raster = [](int count) {
        RasterImage image;
        image.size = {64, 64};
        image.activeLayer = count - 1;
        for (int index = 0; index < count; ++index) {
            RasterLayer layer;
            layer.tiles = {{{0, 0}, QByteArray::fromHex("255997ad").repeated(RgbaTile::ByteCount / 4)}};
            image.layers.append(layer);
        }
        return image;
    };
    OperationDocument first(raster(3));
    Brush brush;
    const auto stroke = first.beginStroke(first.activeLayer(), brush, 29);
    QCOMPARE(first.appendSamples(stroke, {{{512, 512}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 10 * TileRenderer::TileBytes);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    QList<LayerId> layers;
    for (const auto &layer : first.preview()->layers) layers.append(layer.id);
    const auto preview = renderer.recordRegion(commands, first.preview(), {0, 0, 64, 64}, layers);
    QCOMPARE(preview.code, RenderOutcome::Recorded);
    const auto composed = renderer.compose(commands, preview.version, {0, 0, 64, 64});
    QCOMPARE(composed.code, RenderOutcome::Recorded);
    QVERIFY(m_device->submitCommands(true));
    const auto retainedPixels = readBuffer(composed.version->tiles().first().pixels);
    OperationDocument second(raster(4));
    const auto rendered = render(renderer, second.snapshot());
    QVERIFY(rendered);
    const auto switched = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY2(switched, "Preview-derived pixels must yield capacity to another document");
    QCOMPARE(readBuffer(composed.version->tiles().first().pixels), retainedPixels);
    QVERIFY(renderer.residentBytes() <= 10 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::partialLayerOpacityComposesOverTransparency_data()
{
    QTest::addColumn<QByteArray>("pixel");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("half-opacity") << QByteArray::fromHex("0000ffff") << 128 << false << QByteArray::fromHex("0000ff80");
    QTest::newRow("half-alpha-half-opacity") << QByteArray::fromHex("0000ff80") << 128 << false << QByteArray::fromHex("0000ff40");
    QTest::newRow("hidden-rgb-partial-opacity") << QByteArray::fromHex("0000ff00") << 128 << false << QByteArray::fromHex("00000000");
    QTest::newRow("hidden-rgb-full-opacity") << QByteArray::fromHex("0000ff00") << 255 << false << QByteArray::fromHex("0000ff00");
    QTest::newRow("zero-opacity") << QByteArray::fromHex("0000ffff") << 0 << false << QByteArray::fromHex("00000000");
    QTest::newRow("implicit-background") << QByteArray::fromHex("0000ffff") << 128 << true << QByteArray::fromHex("0000ff80");
}

void KisGpuTileRendererTest::partialLayerOpacityComposesOverTransparency()
{
    QFETCH(QByteArray, pixel);
    QFETCH(int, opacity);
    QFETCH(bool, implicit);
    QFETCH(QByteArray, expected);
    RasterImage source;
    source.size = {256, 256};
    RasterLayer layer;
    layer.name = QStringLiteral("Background");
    layer.opacity = quint8(opacity);
    if (implicit) for (int i = 0; i < 4; ++i) layer.defaultPixel[i] = quint8(pixel[i]);
    else layer.tiles = {{{0, 0}, pixel.repeated(RgbaTile::ByteCount / 4)}};
    source.layers = {layer};
    OperationDocument document(source);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto merged = project(renderer, rendered, QRect(0, 0, 256, 256));
    QVERIFY(merged);
    if (!opacity) QVERIFY(merged->tiles().isEmpty());
    else QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), expected);
}

void KisGpuTileRendererTest::layerOpacityChangesProjectionWithoutChangingSourcePixelsOrRetainedVersions()
{
    RasterImage source;
    source.size = {256, 256};
    source.layers = {
        {QStringLiteral("Red"), true, {}, {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Blue"), true, {}, {{{0, 0}, QByteArray::fromHex("0000ffff").repeated(RgbaTile::ByteCount / 4)}}}
    };
    source.layers[1].opacity = 128;
    OperationDocument document(source);
    const auto blue = document.snapshot()->layers[1].id;
    TileRenderer renderer(m_device);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto merged = project(renderer, original, QRect(0, 0, 256, 256));
    QVERIFY(merged);
    QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(readTile(original, blue, {0, 0}).left(4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.setLayerOpacity(blue, 64), Result::Success);
    const auto changed = render(renderer, document.snapshot(), original);
    QVERIFY(changed);
    const auto updated = project(renderer, changed, QRect(0, 0, 256, 256), merged);
    QVERIFY(updated);
    QCOMPARE(readBuffer(updated->tiles().first().pixels).left(4), QByteArray::fromHex("bf0040ff"));
    QCOMPARE(readBuffer(merged->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(readTile(changed, blue, {0, 0}).left(4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), changed);
    const auto restored = project(renderer, undone, QRect(0, 0, 256, 256), updated);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).left(4), QByteArray::fromHex("7f0080ff"));
}

void KisGpuTileRendererTest::projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels_data()
{
    QTest::addColumn<bool>("uniform");
    QTest::newRow("stored-tiles") << false;
    QTest::newRow("uniform-defaults") << true;
}

void KisGpuTileRendererTest::projectionChannelsInvalidateGroupReuseAndKeepRetainedPixels()
{
    QFETCH(bool, uniform);
    RasterImage input;
    input.size = {256, 256};
    const QUuid parent(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    input.layers = {{QStringLiteral("Lower"), true, QUuid::createUuid(), {}},
                    {QStringLiteral("Upper"), true, QUuid::createUuid(), {}},
                    {QStringLiteral("Group"), true, parent, {}}};
    input.layers[2].kind = LayerKind::Group;
    input.layers[1].opacity = 128;
    const QByteArray pixels[] = {QByteArray::fromHex("996633e2"), QByteArray::fromHex("56341286")};
    for (int index = 0; index < 2; ++index) {
        auto &layer = input.layers[index];
        layer.parentUuid = parent;
        if (uniform) {
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixels[index][channel]);
        } else layer.tiles = {{{0, 0}, pixels[index].repeated(RgbaTile::ByteCount / 4)}};
    }
    OperationDocument document(input);
    TileRenderer renderer(m_device);
    auto current = render(renderer, document.snapshot());
    QVERIFY(current);
    auto projection = project(renderer, current, {0, 0, 256, 256});
    QVERIFY(projection);
    const auto fixed = projection;
    QCOMPARE(readBuffer(fixed->tiles().first().pixels), QByteArray::fromHex("865829ea").repeated(RgbaTile::ByteCount / 4));
    for (const auto mask : {quint8(7), quint8(9)}) {
        QCOMPARE(document.setLayerProjectionMask(2, mask), Result::Success);
        auto changed = render(renderer, document.snapshot(), current);
        QVERIFY(changed);
        auto updated = project(renderer, changed, {0, 0, 256, 256}, projection);
        QVERIFY(updated);
        const auto expected = QByteArray::fromHex(mask == 7 ? "87592ae2" : "866633ea");
        QCOMPARE(readBuffer(updated->tiles().first().pixels), expected.repeated(RgbaTile::ByteCount / 4));
        QCOMPARE(readBuffer(fixed->tiles().first().pixels), QByteArray::fromHex("865829ea").repeated(RgbaTile::ByteCount / 4));
        current = changed;
        projection = updated;
    }
}

void KisGpuTileRendererTest::appendedInputPreservesPixelsOutsideItsChangedTiles()
{
    OperationDocument document({512, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 12;
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 0.5}, {{280, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes), oracle(m_device);
    auto current = render(renderer, document.preview());
    QVERIFY(current);
    QCOMPARE(current->tiles(layer).size(), 2);
    const auto unchanged = readTile(current, layer, {0, 0});
    const auto previous = readTile(current, layer, {1, 0});
    QCOMPARE(document.appendSamples(stroke, {{{300, 32}, 0.75}}), Result::Success);
    auto updated = render(renderer, document.preview(), current);
    QVERIFY2(updated, "Extending one tile must retain the two-tile preview within a three-tile budget");
    QCOMPARE(readTile(current, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(current, layer, {1, 0}), previous);
    QCOMPARE(readTile(updated, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(updated, layer, {1, 0}), readTile(render(oracle, document.preview()), layer, {1, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.appendSamples(stroke, {{{350, 32}, 0.25}}), Result::Success);
    updated = render(renderer, document.preview(), current);
    QVERIFY(updated);
    QCOMPARE(readTile(updated, layer, {0, 0}), unchanged);
    QCOMPARE(readTile(updated, layer, {1, 0}), readTile(render(oracle, document.preview()), layer, {1, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {32, 180}, {100, 180}, 0}), Result::Success);
    updated = render(renderer, document.preview(), current);
    QVERIFY(updated);
    QCOMPARE(updated->tiles(layer).size(), 1);
    const auto shaped = readTile(updated, layer, {0, 0});
    QCOMPARE(shaped, readTile(render(oracle, document.preview()), layer, {0, 0}));
    current = updated;
    updated.clear();
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), current);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), shaped);
}

void KisGpuTileRendererTest::appendedInputPreservesCoverageBackgroundAndOldVersions_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("normal") << 0;
    QTest::newRow("erase") << 1;
    QTest::newRow("alpha-locked") << 2;
    QTest::newRow("protected-channels") << 3;
    QTest::newRow("mirrored-overlap") << 4;
    QTest::newRow("hidden-background-rgb") << 5;
    QTest::newRow("large-brush") << 6;
}

void KisGpuTileRendererTest::appendedInputPreservesCoverageBackgroundAndOldVersions()
{
    QFETCH(int, mode);
    const QByteArray pixel = QByteArray::fromHex(mode == 5 ? "4d2b8700" : "4d2b875f");
    const auto background = pixel.repeated(RgbaTile::ByteCount / 4);
    RasterImage input;
    input.size = {512, 256};
    input.layers = {{QStringLiteral("Background"), true, {},
                     {{{0, 0}, background}, {{1, 0}, background}}}};
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {31, 142, 207, 135};
    brush.diameter = mode == 6 ? 512 : 16.75;
    brush.spacing = 0.27;
    brush.opacity = 149;
    brush.erase = mode == 1;
    brush.alphaLocked = mode == 2;
    brush.channelWriteMask = mode == 3 ? 5 : 15;
    brush.mirrorHorizontal = brush.mirrorVertical = mode == 4;
    brush.mirrorCenter = {64, 32};
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{32.25, 32.25}, 0.35}}), Result::Success);
    // The 512px brush spans up to 4x3 layer tiles, including off-canvas pixels.
    constexpr int maximumPaintedTiles = 4 * 3;
    TileRenderer renderer(m_device, (3 * maximumPaintedTiles + 2) * TileRenderer::TileBytes);
    TileRenderer oracle(m_device, (maximumPaintedTiles + 2) * TileRenderer::TileBytes);
    const auto pixels = [&](const RenderedSnapshot &version) {
        QMap<QPair<int, int>, QByteArray> result;
        for (const auto &tile : version->tiles(layer))
            result.insert({tile.coordinate.x(), tile.coordinate.y()}, readTile(version, layer, tile.coordinate));
        return result;
    };
    auto current = render(renderer, document.preview());
    QVERIFY(current);
    const auto first = current;
    const auto firstPixels = pixels(first);
    const QList<InputSample> additions{{{66.75, 34.5}, 0.87}, {{42.25, 31.75}, 0.6},
                                       {{300.5, 48.75}, 0.95}, {{32.5, 31.25}, 0.55}};
    for (const auto &sample : additions) {
        QCOMPARE(document.appendSamples(stroke, {sample}), Result::Success);
        const auto next = render(renderer, document.preview(), current);
        QVERIFY(next);
        const auto rebuilt = render(oracle, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(pixels(next), pixels(rebuilt));
        QCOMPARE(pixels(first), firstPixels);
        current = next;
    }
    const auto finalPixels = pixels(current);
    if (mode == 4) QVERIFY(finalPixels.contains({-1, 0}));
    if (mode == 6) QVERIFY(finalPixels.contains({-1, -1}));
    const auto recovery = document.recoveryData();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto cancelled = render(renderer, document.snapshot(), current);
    QVERIFY(cancelled);
    QCOMPARE(cancelled->tiles(layer).size(), 2);
    for (int x = 0; x < 2; ++x) QCOMPARE(readTile(cancelled, layer, {x, 0}), background);
    QCOMPARE(pixels(current), finalPixels);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    const auto recovered = render(oracle, restored.preview());
    QVERIFY(recovered);
    QCOMPARE(pixels(recovered), finalPixels);
}

void KisGpuTileRendererTest::pendingStrokeLeavesCapacityForCompositionAndCancellation()
{
    RasterImage input;
    input.size = {64, 64};
    input.activeLayer = 1;
    input.layers = {{QStringLiteral("Lower"), true, {},
                     {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
                    {QStringLiteral("Upper"), true, {}, {}}};
    OperationDocument document(input);
    Brush brush;
    brush.rgba = {0, 0, 255, 128};
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes);
    const auto painted = render(renderer, document.preview());
    QVERIFY(painted);
    const auto projection = project(renderer, painted, {0, 0, 64, 64});
    QVERIFY2(projection, "Pending-stroke derived data must leave room for the required projection");
    const auto offset = (32 * TileRenderer::TileSize + 32) * 4;
    QCOMPARE(readBuffer(projection->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("7f0080ff"));
    QVERIFY(renderer.residentBytes() <= 3 * TileRenderer::TileBytes);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), painted);
    QVERIFY(cancelled);
    const auto restored = project(renderer, cancelled, {0, 0, 64, 64}, projection);
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readBuffer(projection->tiles().first().pixels).mid(offset, 4), QByteArray::fromHex("7f0080ff"));
    QVERIFY(renderer.residentBytes() <= 3 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::changingPreviewReusesItsBackgroundWithinThreeTiles()
{
    OperationDocument document({256, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {20, 80, 160, 128};
    brush.diameter = 12;
    for (int i = 0; i < OperationDocument::HistoryLimit; ++i) {
        const auto id = document.beginStroke(layer, brush, i);
        QCOMPARE(document.appendSamples(id, {{{qreal(20 + i % 180), 40}, 0.75}}), Result::Success);
        QCOMPARE(document.commitStroke(id), Result::Success);
    }
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes), oracle(m_device);
    const auto background = render(renderer, document.snapshot());
    QVERIFY(background);
    const auto backgroundPixels = readTile(background, layer, {0, 0});
    const auto stroke = document.beginStroke(layer, brush, 1000);
    QCOMPARE(document.appendSamples(stroke, {{{32, 80}, 0.5}}), Result::Success);
    auto preview = render(renderer, document.preview(), background);
    QVERIFY(preview);
    const auto firstPixels = readTile(preview, layer, {0, 0});
    QCOMPARE(document.appendSamples(stroke, {{{180, 120}, 1}}), Result::Success);
    auto changed = render(renderer, document.preview(), preview);
    QVERIFY2(changed, "A preview update must preserve the background and previous view within three tiles");
    QCOMPARE(readTile(changed, layer, {0, 0}), readTile(render(oracle, document.preview()), layer, {0, 0}));
    QCOMPARE(readTile(preview, layer, {0, 0}), firstPixels);
    QCOMPARE(readTile(background, layer, {0, 0}), backgroundPixels);
    preview = changed;
    changed.clear();
    for (const auto &shape : {Shape{ShapeKind::Line, {140, 180}, {200, 200}, 0},
                             Shape{ShapeKind::Ellipse, {60, 80}, {100, 110}, 0.2}}) {
        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
        changed = render(renderer, document.preview(), preview);
        QVERIFY(changed);
        QCOMPARE(readTile(changed, layer, {0, 0}), readTile(render(oracle, document.preview()), layer, {0, 0}));
        preview = changed;
        changed.clear();
    }
    const auto recovery = document.recoveryData();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    auto cancelled = render(renderer, document.snapshot(), preview);
    QVERIFY(cancelled);
    QCOMPARE(readTile(cancelled, layer, {0, 0}), backgroundPixels);
    preview.clear();
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    TileRenderer recoveredRenderer(m_device, 3 * TileRenderer::TileBytes);
    const auto recoveredBackground = render(recoveredRenderer, restored.snapshot());
    QVERIFY(recoveredBackground);
    auto recovered = render(recoveredRenderer, restored.preview(), recoveredBackground);
    QVERIFY(recovered);
    QCOMPARE(restored.updateShape(restored.activeOperation(), {ShapeKind::Line, {20, 140}, {200, 160}, 0}), Result::Success);
    const auto recoveredChange = render(recoveredRenderer, restored.preview(), recovered);
    QVERIFY(recoveredChange);
    QCOMPARE(readTile(recoveredChange, restored.activeLayer(), {0, 0}),
             readTile(render(oracle, restored.preview()), restored.activeLayer(), {0, 0}));
}

void KisGpuTileRendererTest::initTestCase()
{
    QString diagnostic;
    m_device = Device::createNative(diagnostic);
    QVERIFY2(m_device, qPrintable(diagnostic));
    QVERIFY(m_device->nativeDevice());
    QVERIFY(m_device->queue());
    WGPUAdapterInfo info{};
    QVERIFY(wgpuAdapterGetInfo(m_device->adapter(), &info) == WGPUStatus_Success);
    qInfo() << "GPU projection adapter:" << QString::fromUtf8(info.device.data, qsizetype(info.device.length))
            << "backend" << int(info.backendType) << "type" << int(info.adapterType);
    wgpuAdapterInfoFreeMembers(info);
}

void KisGpuTileRendererTest::partialRenderingMatchesFullTilesAndRejectsIncompleteComposition()
{
    OperationDocument document(QSize(1024, 512));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    const auto stroke = document.beginStroke(layer, brush, 123);
    QCOMPARE(document.appendSamples(stroke, {{{20, 70}, 0.5}, {{800, 300}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto hidden = document.addLayer(QStringLiteral("Hidden"));
    QCOMPARE(document.setLayerVisible(hidden, false), Result::Success);
    const RgbaTile offCanvas{{-1, -1}, QByteArray::fromHex("12345678").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(hidden, {offCanvas}), Result::Success);
    TileRenderer fullRenderer(m_device);
    const auto full = render(fullRenderer, document.snapshot());
    QVERIFY(full);
    TileRenderer partialRenderer(m_device, 3 * TileRenderer::TileBytes);
    const QRect region(263, 130, 1, 1);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto partial = partialRenderer.recordRegion(commands, document.snapshot(), region, {layer});
    QCOMPARE(partial.code, RenderOutcome::Recorded);
    const auto composed = partialRenderer.compose(commands, partial.version, region);
    QCOMPARE(composed.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, partial.version, {0, 0, 1024, 512}).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(partial.version->tiles(layer).size(), 1);
    QVERIFY(partial.version->tiles(hidden).isEmpty());
    QCOMPARE(partial.version->tiles(layer)[0].coordinate, QPoint(1, 0));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto reused = partialRenderer.recordRegion(commands, document.snapshot(), region, {layer}, partial.version);
    QCOMPARE(reused.version, partial.version);
    const auto shifted = partialRenderer.recordRegion(commands, document.snapshot(), {280, 140, 1, 1}, {layer}, partial.version);
    QCOMPARE(shifted.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    auto expanded = partialRenderer.recordRegion(commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(expanded.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(expanded.version->tiles(layer).size(), 2);
    QCOMPARE(partialRenderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));
    QCOMPARE(readTile(expanded.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    QCOMPARE(readTile(expanded.version, layer, {2, 0}), readTile(full, layer, {2, 0}));
    expanded.version.clear();
    const auto edit = document.beginStroke(layer, brush, 456);
    QCOMPARE(document.appendSamples(edit, {{{300, 140}, 0.75}}), Result::Success);
    QCOMPARE(document.commitStroke(edit), Result::Success);
    const auto editedFull = render(fullRenderer, document.snapshot(), full);
    QVERIFY(editedFull);
    commands = m_device->beginCommands();
    QVERIFY(commands);
    auto edited = partialRenderer.recordRegion(commands, document.snapshot(), {256, 0, 512, 256}, {layer}, partial.version);
    QCOMPARE(edited.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readTile(edited.version, layer, {1, 0}), readTile(editedFull, layer, {1, 0}));
    QCOMPARE(readTile(edited.version, layer, {2, 0}), readTile(editedFull, layer, {2, 0}));
    QCOMPARE(readTile(partial.version, layer, {1, 0}), readTile(full, layer, {1, 0}));
    edited.version.clear();
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto raw = partialRenderer.recordRegion(commands, document.snapshot(), {-1, -1, 1, 1}, {hidden}, partial.version);
    QCOMPARE(raw.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, raw.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    const auto incomplete = partialRenderer.recordRegion(commands, document.snapshot(), {0, 0, 1, 1}, {hidden});
    QCOMPARE(incomplete.code, RenderOutcome::Recorded);
    QCOMPARE(partialRenderer.compose(commands, incomplete.version, {0, 0, 1, 1}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(commands, document.snapshot(), region, {layer, layer}).code, RenderOutcome::InvalidVersion);
    QCOMPARE(partialRenderer.recordRegion(commands, document.snapshot(), region, {999}).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(raw.version->tiles(hidden).size(), 1);
    QCOMPARE(readTile(raw.version, hidden, {-1, -1}), offCanvas.rgba);
}

RenderedSnapshot KisGpuTileRendererTest::render(TileRenderer &renderer, const Snapshot &snapshot,
                                               const RenderedSnapshot &base)
{
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    if (!commands) return {};
    const auto result = renderer.record(commands, snapshot, base);
    const auto submitted = m_device->submitCommands(true);
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    if (!submitted) return {};
    return result.version;
}

QByteArray KisGpuTileRendererTest::readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate)
{
    for (const auto &tile : version->tiles(layer)) {
        if (tile.coordinate != coordinate) continue;
        return readBuffer(tile.pixels);
    }
    return {};
}

QByteArray KisGpuTileRendererTest::readBuffer(WGPUBuffer pixels)
{
    const auto commands = m_device->beginCommands();
    if (!commands) return {};
    const auto reads = Detail::ReadbackBatch::create(*m_device, 1);
    if (!reads || !reads->recordBuffer(*m_device, commands, 0, pixels, TileRenderer::TileBytes)) {
        m_device->submitCommands(true);
        return {};
    }
    if (!m_device->submitCommands(true)) return {};
    return reads->results.front()->data;
}

ProjectedSnapshot KisGpuTileRendererTest::project(TileRenderer &renderer, const RenderedSnapshot &version,
                                                 QRect visible, const ProjectedSnapshot &base)
{
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    if (!commands) return {};
    const auto result = renderer.compose(commands, version, visible, base);
    const auto submitted = m_device->submitCommands(true);
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    return submitted ? result.version : ProjectedSnapshot{};
}

void KisGpuTileRendererTest::roundBrushProducesRgbaPixelsOnTheGpu()
{
    OperationDocument document({512, 512});
    Brush brush;
    brush.rgba = {200, 30, 70, 255};
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto bytes = readTile(preview, document.activeLayer(), {0, 0});
    QCOMPARE(bytes.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 32), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(0, 0), QByteArray(4, '\0'));
    QCOMPARE(preview->tiles(document.activeLayer()).size(), 1);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), document.activeLayer(), {0, 0}), bytes);
}

void KisGpuTileRendererTest::inputPartitionsAndPreviewProduceTheSamePixels()
{
    QList<InputSample> samples;
    for (int i = 0; i < 37; ++i) samples.append({{20.125 + i * 7.25, 70.5 + (i % 5) * 2}, qreal(i % 8 + 1) / 8});
    OperationDocument single({512, 512}), chunks({512, 512});
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    brush.opacity = 177;
    const auto a = single.beginStroke(single.activeLayer(), brush, 123);
    const auto b = chunks.beginStroke(chunks.activeLayer(), brush, 123);
    QCOMPARE(single.appendSamples(a, samples), Result::Success);
    for (const auto &sample : samples) QCOMPARE(chunks.appendSamples(b, {sample}), Result::Success);
    TileRenderer renderer(m_device);
    const auto left = render(renderer, single.preview());
    const auto right = render(renderer, chunks.preview());
    QVERIFY(left && right);
    for (const auto &tile : left->tiles(single.activeLayer())) {
        QCOMPARE(readTile(left, single.activeLayer(), tile.coordinate), readTile(right, chunks.activeLayer(), tile.coordinate));
    }
    // Algorithm 1: the fixed input above was verified on Metal and Vulkan.
    QCOMPARE(QCryptographicHash::hash(readTile(left, single.activeLayer(), {0, 0}), QCryptographicHash::Sha256).toHex(),
             QByteArray("bc2af72a97bb315f96c978a6dd39a838470400a441c26363bdc229b8747078cf"));
}

void KisGpuTileRendererTest::unchangedTilesAreSharedAndBudgetsRejectAtomically()
{
    OperationDocument document({1024, 1024});
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto first = render(renderer, document.snapshot());
    QVERIFY(first);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(next, {{{800, 800}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
    const auto second = render(renderer, document.snapshot(), first);
    QVERIFY(second);
    QCOMPARE(second->tiles(document.activeLayer()).first().pixels, first->tiles(document.activeLayer()).first().pixels);
    const auto before = readTile(first, document.activeLayer(), {0, 0});
    const auto third = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(third, {{{32, 32}, 1}}), Result::Success);
    QVERIFY(!render(renderer, document.preview(), second));
    QCOMPARE(readTile(first, document.activeLayer(), {0, 0}), before);
    QCOMPARE(renderer.residentBytes(), 2 * TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::erasingAndLayerVisibilityPreservePixelMeaning()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    brush.erase = true;
    brush.opacity = 128;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto erased = render(renderer, document.snapshot(), painted);
    QVERIFY(erased);
    QCOMPARE(readTile(erased, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff00007f"));
    brush.opacity = 255;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto fullyErased = render(renderer, document.snapshot(), erased);
    QVERIFY(fullyErased);
    const auto pixel = [&](const RenderedSnapshot &version) {
        return readTile(version, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4);
    };
    QCOMPARE(pixel(fullyErased), QByteArray::fromHex("ff000000"));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot());
    QVERIFY(undone);
    QCOMPARE(pixel(undone), QByteArray::fromHex("ff00007f"));
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(pixel(redone), QByteArray::fromHex("ff000000"));
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto restored = render(renderer, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(pixel(restored), QByteArray::fromHex("ff000000"));
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), fullyErased);
    QVERIFY(hidden);
    QCOMPARE(hidden->tiles(document.activeLayer()).first().pixels, fullyErased->tiles(document.activeLayer()).first().pixels);
    QCOMPARE(readTile(painted, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::longDiagonalUsesOnlyIntersectingTiles()
{
    OperationDocument document({4096, 4096});
    Brush brush;
    brush.diameter = 4;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{4, 4}, 1}, {{4092, 4092}, 1}}), Result::Success);
    TileRenderer renderer(m_device, 64 * TileRenderer::TileBytes);
    const auto version = render(renderer, document.preview());
    QVERIFY2(version, "A sparse diagonal must fit without allocating its bounding rectangle");
    QVERIFY(version->tiles(document.activeLayer()).size() <= 64);
    QCOMPARE(readTile(version, document.activeLayer(), {0, 0}).mid((4 * TileRenderer::TileSize + 4) * 4, 4), QByteArray::fromHex("000000ff"));
}

void KisGpuTileRendererTest::shapesUndoAndRecoveryRestoreTheSamePixels()
{
    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {17, 129, 241, 201};
    brush.diameter = 13.75;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{10, 20}, 0.75}, {{180, 140}, 0.5}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto freehand = render(renderer, document.preview());
    QVERIFY(freehand);
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Line, {60, 80}, {200, 140}, 0}), Result::Success);
    const auto line = render(renderer, document.preview(), freehand);
    QVERIFY(line);
    const auto linePixels = readTile(line, document.activeLayer(), {0, 0});
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Ellipse, {30, 40}, {210, 220}, 0.3}), Result::Success);
    const auto ellipse = render(renderer, document.preview(), line);
    QVERIFY(ellipse);
    const auto pixels = readTile(ellipse, document.activeLayer(), {0, 0});
    QVERIFY(pixels != linePixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), ellipse);
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), {0, 0}), pixels);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(readTile(render(renderer, restored.snapshot()), restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(),
             QByteArray("9b38f73d4cc8a980990534b895dd2ef68b6d59f4156731881162ffc1574fd2ec"));
    const auto cancelled = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(cancelled, {{{128, 128}, 1}}), Result::Success);
    QVERIFY(render(renderer, document.preview(), ellipse));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistoryAndRecovery_data()
{
    QTest::addColumn<qreal>("rotation");
    QTest::addColumn<QPoint>("edge");
    QTest::addColumn<bool>("rounded");
    QTest::addColumn<QSizeF>("radii");
    QTest::newRow("axis-aligned") << qreal(0) << QPoint(16, 32) << false << QSizeF(0, 0);
    QTest::newRow("rotated") << qreal(0.7853981633974483) << QPoint(9, 32) << false << QSizeF(0, 0);
    QTest::newRow("rounded") << qreal(0) << QPoint(16, 32) << true << QSizeF(14, 14);
    QTest::newRow("rounded-rotated") << qreal(0.7853981633974483) << QPoint(21, 21) << true << QSizeF(14, 14);
    QTest::newRow("rounded-clamped") << qreal(0) << QPoint(16, 32) << true << QSizeF(32, 32);
    QTest::newRow("rounded-asymmetric") << qreal(0) << QPoint(16, 32) << true << QSizeF(14, 4);
    QTest::newRow("rounded-zero-axis") << qreal(0) << QPoint(16, 32) << true << QSizeF(0, 14);
}

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistoryAndRecovery()
{
    QFETCH(qreal, rotation);
    QFETCH(QPoint, edge);
    QFETCH(bool, rounded);
    QFETCH(QSizeF, radii);
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    const Shape shape{rounded ? ShapeKind::RoundedRectangle : ShapeKind::Rectangle, {16, 16}, {48, 48}, rotation,
                      radii};
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    const auto pixel = [&](const QByteArray &bytes, QPoint point) {
        return bytes.mid((point.y() * TileRenderer::TileSize + point.x()) * 4, 4);
    };
    for (const auto &point : {edge, QPoint(64 - edge.y(), edge.x()),
                              QPoint(64 - edge.x(), 64 - edge.y()), QPoint(edge.y(), 64 - edge.x())}) {
        QCOMPARE(pixel(pixels, point), QByteArray::fromHex("1181f1ff"));
    }
    QCOMPARE(pixel(pixels, {32, 32}), QByteArray(4, '\0'));
    QCOMPARE(pixel(pixels, {8, 8}), QByteArray(4, '\0'));
    if (rounded && rotation == 0 && radii.width() >= 14 && radii.height() >= 14)
        QCOMPARE(pixel(pixels, {16, 16}), QByteArray(4, '\0'));
    if (rounded && radii.width() == 0) QCOMPARE(pixel(pixels, {16, 16}), QByteArray::fromHex("1181f1ff"));
    if (rounded && (radii.width() > 16 || radii.height() > 16)) {
        auto bounded = shape;
        bounded.cornerRadii = {qMin(qreal(16), radii.width()), qMin(qreal(16), radii.height())};
        QCOMPARE(document.updateShape(stroke, bounded), Result::Success);
        QCOMPARE(readTile(render(renderer, document.preview()), layer, {0, 0}), pixels);
        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    }
    const auto beforeRejectedShape = document.preview();
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Rectangle, {16, 16}, {16, 48}, rotation}), Result::InvalidInput);
    QCOMPARE(document.preview(), beforeRejectedShape);
    if (rounded) {
        auto invalid = shape;
        invalid.cornerRadii.setWidth(-1);
        QCOMPARE(document.updateShape(stroke, invalid), Result::InvalidInput);
        QCOMPARE(document.preview(), beforeRejectedShape);
    }
    QCOMPARE(document.undoCount(), 0);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(restored.hasActiveStroke());
    const auto recovered = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    QCOMPARE(recovered->shape.cornerRadii, shape.cornerRadii);
    if (rounded) {
        for (const quint8 previousFormat : {quint8(9), quint8(10)}) {
            const auto previous = legacyRecovery(document, previousFormat);
            const auto before = restored.preview();
            QCOMPARE(restored.restoreRecovery(previous), Result::InvalidRecovery);
            QCOMPARE(restored.preview(), before);
        }
    } else {
        const auto previous = legacyRecovery(document, 10);
        QCOMPARE(restored.restoreRecovery(previous), Result::Success);
        QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    }
    QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.historyAction(1), HistoryAction::DrawRectangle);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), preview);
    QVERIFY(undone && undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), layer, {0, 0}), pixels);
    const auto cancelled = document.beginStroke(layer, brush, 8);
    QCOMPARE(document.appendSamples(cancelled, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.updateShape(cancelled, {ShapeKind::Rectangle, {20, 20}, {40, 40}, rotation}), Result::Success);
    QVERIFY(render(renderer, document.preview(), preview));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    QCOMPARE(readTile(preview, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistoryAndRecovery_data()
{
    QTest::addColumn<bool>("closed");
    QTest::newRow("open") << false;
    QTest::newRow("closed") << true;
}

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistoryAndRecovery()
{
    QFETCH(bool, closed);
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    const QList<InputSample> vertices{{{16, 16}, 1}, {{48, 16}, 1}, {{48, 48}, 1}};
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {vertices.first()}), Result::Success);
    const Shape shape{closed ? ShapeKind::Polygon : ShapeKind::Polyline, {16, 16}, {48, 48}, 0};
    const auto single = document.preview();
    QCOMPARE(document.updateShape(stroke, shape), Result::InvalidInput);
    QCOMPARE(document.preview(), single);
    QCOMPARE(document.appendSamples(stroke, vertices.mid(1)), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.preview());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    const auto pixel = [&](QPoint p) { return pixels.mid((p.y() * TileRenderer::TileSize + p.x()) * 4, 4); };
    QCOMPARE(pixel({32, 16}), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel({48, 32}), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel({32, 32}), closed ? QByteArray::fromHex("1181f1ff") : QByteArray(4, '\0'));
    QCOMPARE(pixel({16, 48}), QByteArray(4, '\0'));
    OperationDocument oracle({64, 64});
    auto path = vertices;
    if (closed) path.append(path.first());
    const auto expected = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(expected, path), Result::Success);
    QCOMPARE(oracle.commitStroke(expected), Result::Success);
    QCOMPARE(readTile(render(renderer, oracle.snapshot()), oracle.activeLayer(), {0, 0}), pixels);
    const auto operation = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), vertices);
    QCOMPARE(document.undoCount(), 0);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(restored.hasActiveStroke());
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last())->inputSamples(), vertices);
    QCOMPARE(readTile(render(renderer, restored.preview()), restored.activeLayer(), {0, 0}), pixels);
    for (const quint8 previousFormat : {quint8(9), quint8(10)}) {
        const auto previous = legacyRecovery(document, previousFormat);
        const auto before = restored.preview();
        QCOMPARE(restored.restoreRecovery(previous), Result::InvalidRecovery);
        QCOMPARE(restored.preview(), before);
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.historyAction(1), closed ? HistoryAction::DrawPolygon : HistoryAction::DrawPolyline);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), preview);
    QVERIFY(undone && undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), layer, {0, 0}), pixels);
    const auto cancelled = document.beginStroke(layer, brush, 8);
    QCOMPARE(document.appendSamples(cancelled, vertices), Result::Success);
    QCOMPARE(document.updateShape(cancelled, shape), Result::Success);
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::versionsOutliveTheRendererAndRejectAnotherDeviceOwner()
{
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    RenderedSnapshot retained;
    {
        TileRenderer owner(m_device);
        retained = render(owner, document.preview());
        QVERIFY(retained);
    }
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}).size(), TileRenderer::TileBytes);
    TileRenderer replacement(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    QCOMPARE(replacement.record(commands, document.preview(), retained).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(render(replacement, document.preview()));
}

void KisGpuTileRendererTest::visibleLayersComposeOnTheGpuAndReuseUnchangedTiles()
{
    OperationDocument document({1024, 1024});
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    brush.diameter = 16;
    auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto top = document.addLayer(QStringLiteral("Blue"));
    brush.rgba = {0, 0, 255, 128};
    stroke = document.beginStroke(top, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    const auto composed = project(renderer, layers, {0, 0, 256, 256});
    QVERIFY(composed);
    QCOMPARE(composed->tiles().size(), 1);
    const auto pixels = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("7f0080ff"));
    const auto outside = project(renderer, layers, {512, 512, 256, 256});
    QVERIFY(outside);
    QVERIFY(outside->tiles().isEmpty());
    QVERIFY(document.addLayer(QStringLiteral("Empty")));
    const auto expanded = render(renderer, document.snapshot(), layers);
    QVERIFY(expanded);
    const auto reused = project(renderer, expanded, {0, 0, 256, 256}, composed);
    QVERIFY(reused);
    QCOMPARE(reused->tiles().first().pixels, composed->tiles().first().pixels);
    QCOMPARE(document.setLayerVisible(top, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), expanded);
    const auto red = project(renderer, hidden, {0, 0, 256, 256}, reused);
    QVERIFY(red);
    QCOMPARE(readBuffer(red->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), pixels);
}

void KisGpuTileRendererTest::compositionPreservesOrderAlphaAndExistingVersionsAtCapacity()
{
    OperationDocument document({64, 64});
    const QList<std::array<quint8, 4>> colors{{255, 0, 0, 255}, {0, 0, 255, 128}, {0, 255, 0, 64}, {255, 255, 255, 128}};
    LayerId green = 0;
    for (qsizetype i = 0; i < colors.size(); ++i) {
        const auto layer = i == 0 ? document.activeLayer() : document.addLayer(QString::number(i));
        if (i == 2) green = layer;
        Brush brush;
        brush.rgba = colors[i];
        brush.diameter = 16;
        const auto stroke = document.beginStroke(layer, brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto layers = render(renderer, document.snapshot());
    QVERIFY(layers);
    auto composed = project(renderer, layers, {0, 0, 64, 64});
    QVERIFY(composed);
    const auto original = readBuffer(composed->tiles().first().pixels);
    QCOMPARE(original.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("afa0b0ff"));
    QCOMPARE(renderer.residentBytes(), 5 * TileRenderer::TileBytes);
    QCOMPARE(document.setLayerVisible(green, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), layers);
    QVERIFY(hidden);
    auto reduced = project(renderer, hidden, {0, 0, 64, 64}, composed);
    QVERIFY(reduced);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    QVERIFY(renderer.residentBytes() <= 6 * TileRenderer::TileBytes);
    const auto lower = document.snapshot()->layers.first().id;
    QCOMPARE(document.setLayerOpacity(lower, 137), Result::Success);
    const auto changed = render(renderer, document.snapshot(), hidden);
    QVERIFY(changed);
    QVERIFY(!project(renderer, changed, {0, 0, 64, 64}, reduced));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
    composed.clear();
    reduced.clear();
    QCOMPARE(document.setLayerOpacity(lower, 255), Result::Success);
    const auto restored = project(renderer, render(renderer, document.snapshot(), changed), {0, 0, 64, 64});
    QVERIFY(restored);
    QCOMPARE(readBuffer(restored->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
}

void KisGpuTileRendererTest::paintingAndCompositionShareOneSubmission()
{
    OperationDocument document({64, 64});
    for (const auto &color : QList<std::array<quint8, 4>>{{255, 0, 0, 128}, {0, 0, 255, 128}}) {
        if (!document.snapshot()->layers.first().operations.isEmpty()) QVERIFY(document.addLayer(QStringLiteral("Top")));
        Brush brush;
        brush.rgba = color;
        brush.diameter = 16;
        const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    TileRenderer renderer(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto layers = renderer.record(commands, document.snapshot());
    QCOMPARE(layers.code, RenderOutcome::Recorded);
    const auto projection = renderer.compose(commands, layers.version, {0, 0, 64, 64});
    QCOMPARE(projection.code, RenderOutcome::Recorded);
    const auto readback = Detail::ReadbackBatch::create(*m_device, 1);
    QVERIFY(readback);
    QVERIFY(readback->recordBuffer(*m_device, commands, 0, projection.version->tiles().first().pixels, TileRenderer::TileBytes));
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(readback->results.front()->data.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("5500aac0"));
}

void KisGpuTileRendererTest::longDocumentRebuildUsesBoundedTileStorage()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.diameter = 8;
    brush.rgba = {71, 139, 211, 128};
    Snapshot initial;
    for (int i = 0; i < OperationDocument::HistoryLimit; ++i) {
        const auto stroke = document.beginStroke(document.activeLayer(), brush, i);
        QCOMPARE(document.appendSamples(stroke, {{{qreal(8 + i % 48), 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
        if (i == 0) initial = document.snapshot();
    }
    QCOMPARE(document.undoCount(), OperationDocument::HistoryLimit);
    TileRenderer reference(m_device);
    const auto first = render(reference, initial);
    QVERIFY(first);
    const auto firstPixels = readTile(first, document.activeLayer(), {0, 0});
    const auto expected = render(reference, document.snapshot(), first);
    QVERIFY(expected);
    QCOMPARE(readTile(first, document.activeLayer(), {0, 0}), firstPixels);
    const auto pixels = readTile(expected, document.activeLayer(), {0, 0});
    // Algorithm 1: retained image contract for the fixed 250-stroke sequence.
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(),
        QByteArray("9ddc5af102159e58474239292756c24b2ec8a1813a88d33181b19b2dab66bdf7"));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    TileRenderer bounded(m_device, 2 * TileRenderer::TileBytes);
    const auto rebuilt = render(bounded, restored.snapshot());
    QVERIFY2(rebuilt, "A retained 250-operation document must rebuild with two writable tiles per coordinate");
    QCOMPARE(readTile(rebuilt, restored.activeLayer(), {0, 0}), pixels);
    QCOMPARE(bounded.residentBytes(), TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::retainedPixelsOwnTheNativeDeviceUntilTheirLastReader()
{
    QString diagnostic;
    auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    std::weak_ptr<Device> lifetime = device;
    RenderedSnapshot retained;
    ProjectedSnapshot projected;
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    {
        TileRenderer renderer(device);
        WGPUCommandEncoder commands = nullptr;
        commands = device->beginCommands();
        QVERIFY(commands);
        const auto result = renderer.record(commands, document.preview());
        QCOMPARE(result.code, RenderOutcome::Recorded);
        retained = result.version;
        const auto merged = renderer.compose(commands, retained, {0, 0, 64, 64});
        QCOMPARE(merged.code, RenderOutcome::Recorded);
        projected = merged.version;
        QVERIFY(device->submitCommands(true));
    }
    device.reset();
    QVERIFY(!lifetime.expired());
    retained.clear();
    QVERIFY(!lifetime.expired());
    {
        const auto reader = lifetime.lock();
        WGPUCommandEncoder commands = nullptr;
        commands = reader->beginCommands();
        QVERIFY(commands);
        const auto pixels = Detail::ReadbackBatch::create(*reader, 1);
        QVERIFY(pixels);
        QVERIFY(pixels->recordBuffer(*reader, commands, 0, projected->tiles().first().pixels, TileRenderer::TileBytes));
        QVERIFY(reader->submitCommands(true));
        QCOMPARE(pixels->results.front()->data.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("000000ff"));
    }
    projected.clear();
    QVERIFY(lifetime.expired());
}

void KisGpuTileRendererTest::nativeDeviceCreationRequiresTheApplicationThread()
{
    DevicePtr device;
    QString diagnostic;
    std::unique_ptr<QThread> worker(QThread::create([&] { device = Device::createNative(diagnostic); }));
    worker->start();
    QVERIFY(worker->wait(2000));
    QVERIFY(!device);
    QVERIFY(!diagnostic.isEmpty());
}

void KisGpuTileRendererTest::rasterCheckpointsPreservePixelsThroughPaintingUndoAndRecovery()
{
    RasterImage image;
    image.size = {510, 255};
    const QByteArray invisible = QByteArray::fromHex("120a2700").repeated(RgbaTile::ByteCount / 4);
    const QByteArray blue = QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4);
    const QByteArray outside = QByteArray::fromHex("00ff0040").repeated(RgbaTile::ByteCount / 4);
    image.layers = {{QStringLiteral("Imported"), true, {}, {{{0, 0}, invisible}, {{1, 0}, blue}, {{-1, 0}, outside}}}};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto imported = render(renderer, document.snapshot());
    QVERIFY(imported);
    QCOMPARE(imported->tiles(layer).size(), 3);
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    QCOMPARE(readTile(imported, layer, {1, 0}), blue);
    QCOMPARE(readTile(imported, layer, {-1, 0}), outside);
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), imported);
    QVERIFY(painted);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("ff0000ff"));
    QCOMPARE(pixels.first(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(pixels.last(4), QByteArray::fromHex("120a2700"));
    QCOMPARE(readTile(imported, layer, {0, 0}), invisible);
    for (const auto &tile : painted->tiles(layer)) {
        if (tile.coordinate == QPoint(0, 0)) continue;
        for (const auto &old : imported->tiles(layer)) {
            if (old.coordinate == tile.coordinate) QCOMPARE(tile.pixels, old.pixels);
        }
    }
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {0, 0}), invisible);
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    TileRenderer rebuilt(m_device, 4 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    QCOMPARE(readTile(restored, layer, {-1, 0}), outside);
}

void KisGpuTileRendererTest::overwrittenTileHistoryRebuildsWithinTwoBuffers()
{
    OperationDocument document({256, 256});
    const auto layer = document.activeLayer();
    Brush blue;
    blue.rgba = {0, 0, 255, 255};
    for (int i = 0; i < 250; ++i) {
        const auto color = i % 2 ? QByteArray::fromHex("00ff00ff") : QByteArray::fromHex("ff0000ff");
        QCOMPARE(document.replaceTiles(layer, {{{0, 0}, color.repeated(RgbaTile::ByteCount / 4)}}), Result::Success);
        const auto stroke = document.beginStroke(layer, blue, 0);
        QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
        QCOMPARE(document.commitStroke(stroke), Result::Success);
    }
    QCOMPARE(document.undoCount(), 250);
    TileRenderer renderer(m_device, 2 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    const auto pixels = readTile(original, layer, {0, 0});
    QCOMPARE(pixels.first(4), QByteArray::fromHex("00ff00ff"));
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto invisible = QByteArray::fromHex("0a141e00").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, invisible}}), Result::Success);
    const auto replaced = render(renderer, document.snapshot(), original);
    QVERIFY(replaced);
    QCOMPARE(readTile(replaced, layer, {0, 0}), invisible);
    QCOMPARE(readTile(original, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::tileWritesPreservePaintingOnOtherCoordinates()
{
    OperationDocument document({512, 256});
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {255, 0, 0, 255};
    auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{400, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto green = QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, green}}), Result::Success);
    brush.rgba = {0, 0, 255, 255};
    stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{264, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device, 3 * TileRenderer::TileBytes);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    QCOMPARE(readTile(rendered, layer, {0, 0}), green);
    const auto right = readTile(rendered, layer, {1, 0});
    QCOMPARE(right.mid((32 * 256 + 8) * 4, 4), QByteArray::fromHex("0000ffff"));
    QCOMPARE(right.mid((32 * 256 + 44) * 4, 4), QByteArray::fromHex("ff0000ff"));
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection_data()
{
    QTest::addColumn<QByteArray>("defaultRgba");
    QTest::newRow("opaque-white") << QByteArray::fromHex("ffffffff");
    QTest::newRow("transparent-rgb") << QByteArray::fromHex("11171f00");
}

void KisGpuTileRendererTest::implicitLayerPixelsStaySparseThroughEditsAndProjection()
{
    QFETCH(QByteArray, defaultRgba);
    RasterImage image;
    image.size = {4096, 4096};
    image.layers = {{QStringLiteral("White"), true, {}, {}}};
    for (int i = 0; i < 4; ++i) image.layers[0].defaultPixel[i] = quint8(defaultRgba[i]);
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    TileRenderer renderer(m_device, 6 * TileRenderer::TileBytes);
    const auto original = render(renderer, document.snapshot());
    QVERIFY(original);
    QVERIFY(original->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));
    const auto corner = project(renderer, original, {4000, 4000, 96, 96});
    QVERIFY(corner);
    QCOMPARE(corner->tiles().size(), 1);
    QCOMPARE(corner->tiles().first().coordinate, QPoint(15, 15));
    QCOMPARE(readBuffer(corner->tiles().first().pixels), defaultRgba.repeated(RgbaTile::ByteCount / 4));
    Brush brush;
    brush.rgba = {0, 0, 255, 255};
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto painted = render(renderer, document.snapshot(), original);
    QVERIFY(painted);
    QCOMPARE(painted->tiles(layer).size(), 1);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.first(4), defaultRgba);
    QCOMPARE(pixels.mid((32 * 256 + 32) * 4, 4), QByteArray::fromHex("0000ffff"));
    const auto visible = project(renderer, painted, {0, 0, 512, 256});
    QVERIFY(visible);
    QCOMPARE(visible->tiles().size(), 2);
    QCOMPARE(visible->tiles()[1].pixels, corner->tiles().first().pixels);
    QCOMPARE(readBuffer(corner->tiles().first().pixels).first(4), defaultRgba);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), painted);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(2 * TileRenderer::TileBytes));
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.redo(), Result::Success);
    TileRenderer rebuilt(m_device, 2 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::implicitLayersRespectCompositionAndCapacity()
{
    RasterImage image;
    image.size = {512, 256};
    image.layers = {{QStringLiteral("Red"), true, {}, {}}, {QStringLiteral("Blue"), true, {}, {}}};
    image.layers[0].defaultPixel = {255, 0, 0, 255};
    image.layers[1].defaultPixel = {0, 0, 255, 128};
    OperationDocument document(image);
    TileRenderer renderer(m_device, 4 * TileRenderer::TileBytes);
    const auto both = render(renderer, document.snapshot());
    QVERIFY(both);
    const auto merged = project(renderer, both, {0, 0, 512, 256});
    QVERIFY(merged);
    QCOMPARE(merged->tiles().size(), 2);
    for (const auto &tile : merged->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("7f0080ff"));
    QCOMPARE(document.setLayerVisible(2, false), Result::Success);
    const auto hidden = render(renderer, document.snapshot(), both);
    QVERIFY(hidden);
    const auto red = project(renderer, hidden, {0, 0, 512, 256}, merged);
    QVERIFY(red);
    QCOMPARE(red->tiles().size(), 2);
    for (const auto &tile : red->tiles()) QCOMPARE(readBuffer(tile.pixels).first(4), QByteArray::fromHex("ff0000ff"));
    TileRenderer small(m_device, TileRenderer::TileBytes);
    QVERIFY(!render(small, document.snapshot()));
    QCOMPARE(small.residentBytes(), qsizetype(0));
    image.size = {4096, 4096};
    image.layers.removeLast();
    OperationDocument large(image);
    const auto sparse = render(small, large.snapshot());
    QVERIFY(sparse);
    QVERIFY(!project(small, sparse, {0, 0, 4096, 4096}));
    QCOMPARE(small.residentBytes(), qsizetype(TileRenderer::TileBytes));
}

void KisGpuTileRendererTest::removingFigureCoverageRestoresItsPreviewBackground()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto id = document.beginStroke(layer, brush, 42);
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}}), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.preview());
    QVERIFY(painted);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("1181f1ff"));
    FigureStyle style;
    style.outline = false;
    QCOMPARE(document.updateFigure(id, {ShapeKind::Rectangle, {8, 8}, {56, 56}, 0}, style), Result::Success);
    const auto empty = render(renderer, document.preview(), painted);
    QVERIFY(empty);
    QVERIFY(empty->tiles(layer).isEmpty());
    QCOMPARE(readTile(painted, layer, {0, 0}), pixels);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 1);
}

void KisGpuTileRendererTest::collinearFillPreservesPixelsWithinTileBudget_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("vertices");
    for (const auto kind : {ShapeKind::Polyline, ShapeKind::Polygon}) {
        for (int vertices : {2, 3}) {
            const auto name = QString::number(int(kind)) + '-' + QString::number(vertices);
            QTest::newRow(qPrintable(name)) << int(kind) << vertices;
        }
    }
}

void KisGpuTileRendererTest::collinearFillPreservesPixelsWithinTileBudget()
{
    QFETCH(int, kind);
    QFETCH(int, vertices);
    OperationDocument document(QSize(1024, 1024));
    const auto layer = document.activeLayer();
    QList<InputSample> samples{{{0, 0}, 1}};
    if (vertices == 3) samples.append({{512, 512}, 1});
    samples.append({{1024, 1024}, 1});
    FigureStyle style;
    style.outline = false;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(layer, Brush{}, samples,
                                    {static_cast<ShapeKind>(kind), samples.first().position,
                                     samples.last().position, 0}, style, 42), Result::Success);
    TileRenderer renderer(m_device, TileRenderer::TileBytes);
    auto *commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto result = renderer.record(commands, document.snapshot());
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(result.code, RenderOutcome::Recorded);
    QVERIFY(result.version->tiles(layer).isEmpty());
    QCOMPARE(renderer.residentBytes(), qsizetype(0));
    QCOMPARE(document.undoCount(), 1);
}

void KisGpuTileRendererTest::filledInteriorRespectsRegionAndTileBudget()
{
    OperationDocument document(QSize(1024, 1024));
    const auto layer = document.activeLayer();
    FigureStyle style;
    style.outline = false;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(layer, Brush{}, {{{0, 0}, 1}, {{1024, 1024}, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, {1024, 1024}, 0}, style, 42), Result::Success);
    TileRenderer bounded(m_device, 9 * TileRenderer::TileBytes);
    auto *commands = m_device->beginCommands();
    QVERIFY(commands);
    const auto whole = bounded.record(commands, document.snapshot());
    QCOMPARE(whole.code, RenderOutcome::CapacityExceeded);
    QCOMPARE(bounded.residentBytes(), qsizetype(0));
    TileRenderer viewport(m_device, TileRenderer::TileBytes);
    const auto cropped = viewport.recordRegion(commands, document.snapshot(), {0, 0, 64, 64}, {layer});
    QCOMPARE(cropped.code, RenderOutcome::Recorded);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QCOMPARE(viewport.residentBytes(), qsizetype(TileRenderer::TileBytes));
    QCOMPARE(readTile(cropped.version, layer, {0, 0}), QByteArray::fromHex("dd112cff").repeated(TileRenderer::TileBytes / 4));
}

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistoryAndRecovery_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<bool>("outline");
    for (const auto kind : {ShapeKind::Rectangle, ShapeKind::Ellipse, ShapeKind::RoundedRectangle,
                            ShapeKind::Polyline, ShapeKind::Polygon}) {
        for (bool outline : {false, true}) {
            const auto name = QString::number(int(kind)) + (outline ? QStringLiteral("-fill-outline") : QStringLiteral("-fill"));
            QTest::newRow(qPrintable(name)) << int(kind) << outline;
        }
    }
}

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistoryAndRecovery()
{
    QFETCH(int, kind);
    QFETCH(bool, outline);
    const auto shapeKind = static_cast<ShapeKind>(kind);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 4;
    const auto id = document.beginStroke(layer, brush, 42);
    QVERIFY(!id.isNull());
    const QList<InputSample> samples = shapeKind == ShapeKind::Polyline || shapeKind == ShapeKind::Polygon ?
        QList<InputSample>{{{8, 8}, 1}, {{56, 8}, 1}, {{32, 56}, 1}} :
        QList<InputSample>{{{8, 8}, 1}, {{56, 56}, 1}};
    QCOMPARE(document.appendSamples(id, samples), Result::Success);
    Shape shape{shapeKind, samples.first().position, samples.last().position, 0};
    if (shapeKind == ShapeKind::RoundedRectangle) shape.cornerRadii = {8, 8};
    FigureStyle style;
    style.outline = outline;
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(document.updateFigure(id, shape, style), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device);
    const auto version = render(renderer, document.snapshot());
    QVERIFY(version);
    const auto pixels = readTile(version, layer, {0, 0});
    QCOMPARE(pixels.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 24), QByteArray::fromHex("dd112cff"));
    QCOMPARE(pixel(32, 8), QByteArray::fromHex("dd112cff"));
    QCOMPARE(pixel(32, 7), outline ? QByteArray::fromHex("1181f1ff") : QByteArray(4, char(0)));
    QCOMPARE(pixel(0, 0), QByteArray(4, char(0)));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), version);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto restored = render(renderer, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistoryAndRecovery_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("subpaths"),
                              QStringLiteral("even-odd"), QStringLiteral("winding"), QStringLiteral("empty")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistoryAndRecovery()
{
    QFETCH(QString, kind);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    Shape shape;
    shape.kind = ShapeKind::Path;
    QList<InputSample> samples;
    FigureStyle style;
    const bool fill = kind == QLatin1String("even-odd") || kind == QLatin1String("winding");
    if (fill) {
        shape.windingFill = kind == QLatin1String("winding");
        for (const auto &rect : {QRectF(8, 8, 48, 48), QRectF(24, 24, 16, 16)}) {
            shape.pathCommands.append(PathCommand::MoveTo);
            samples.append({rect.topLeft(), 1});
            for (const auto &point : {rect.topRight(), rect.bottomRight(), rect.bottomLeft(), rect.topLeft()}) {
                shape.pathCommands.append(PathCommand::LineTo);
                samples.append({point, 1});
            }
        }
        style.outline = false;
        style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    } else if (kind == QLatin1String("curve")) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        samples = {{{12, 32}, 1}, {{20, 12}, 1}, {{40, 52}, 1}, {{52, 32}, 1}};
    } else if (kind == QLatin1String("subpaths")) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::LineTo};
        samples = {{{12, 12}, 1}, {{52, 12}, 1}, {{12, 52}, 1}, {{52, 52}, 1}};
    }
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    QCOMPARE(document.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    const auto version = render(renderer, document.snapshot());
    QVERIFY(version);
    if (kind == QLatin1String("empty")) {
        QVERIFY(version->tiles(layer).isEmpty());
        QCOMPARE(document.undoCount(), 1);
        return;
    }
    const auto pixels = readTile(version, layer, {0, 0});
    QCOMPARE(pixels.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    if (fill) {
        QCOMPARE(pixel(16, 16), QByteArray::fromHex("dd112cff"));
        QCOMPARE(pixel(32, 32), shape.windingFill ? QByteArray::fromHex("dd112cff") : QByteArray(4, char(0)));
    } else {
        QCOMPARE(pixel(32, kind == QLatin1String("curve") ? 32 : 12), QByteArray::fromHex("1181f1ff"));
        if (kind == QLatin1String("subpaths")) QCOMPARE(pixel(32, 32), QByteArray(4, char(0)));
    }
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), version);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    const auto restored = render(renderer, recovered.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("path-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::pixelCorrectionPreservesCornersSensorsAndIncrementalPixels_data()
{
    QTest::addColumn<QList<InputSample>>("input");
    QTest::addColumn<QList<InputSample>>("corrected");
    QTest::newRow("tap") << QList<InputSample>{{{12.2, 12.7}, 0.8}}
                         << QList<InputSample>{{{12.2, 12.7}, 0.8}};
    QTest::newRow("held-corner") << QList<InputSample>{{{4, 4}, 1}, {{12.2, 12.7}, 0.8},
                                                     {{13.2, 12.3}, 0.4}, {{13.8, 13.8}, 0.6}}
                                 << QList<InputSample>{{{12.5, 12.5}, 0.8}};
    QTest::newRow("flushed-corner") << QList<InputSample>{{{4, 4}, 1}, {{12.2, 12.7}, 0.8},
                                                        {{13.2, 12.3}, 0.4}, {{20.8, 20.8}, 0.6}}
                                    << QList<InputSample>{{{12.5, 12.5}, 0.8}, {{13.5, 12.5}, 0.4}, {{20.5, 20.5}, 0.6}};
    QTest::newRow("initial-tile-removed") << QList<InputSample>{{{300, 300}, 1}, {{12.2, 12.7}, 0.8}}
                                         << QList<InputSample>{{{12.5, 12.5}, 0.8}};
    QTest::newRow("negative-floor") << QList<InputSample>{{{4, 4}, 1}, {{-0.2, 12.7}, 0.8}, {{20.8, 20.8}, 0.6}}
                                    << QList<InputSample>{{{-0.5, 12.5}, 0.8}, {{20.5, 20.5}, 0.6}};
}

void KisGpuTileRendererTest::pixelCorrectionPreservesCornersSensorsAndIncrementalPixels()
{
    QFETCH(QList<InputSample>, input);
    QFETCH(QList<InputSample>, corrected);
    OperationDocument document(QSize(1024, 1024));
    Brush brush;
    brush.diameter = 3;
    brush.rgba = {17, 129, 241, 255};
    brush.pixelPerfect = true;
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (const auto &sample : input) {
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
        for (const auto &tile : rebuilt->tiles(layer))
            QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), input);
    OperationDocument oracle(QSize(1024, 1024));
    brush.pixelPerfect = false;
    const auto expectedId = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(expectedId, corrected), Result::Success);
    QCOMPARE(oracle.commitStroke(expectedId), Result::Success);
    const auto expected = render(renderer, oracle.snapshot());
    QVERIFY(expected);
    QCOMPARE(incremental->tiles(layer).size(), expected->tiles(oracle.activeLayer()).size());
    for (const auto &tile : expected->tiles(oracle.activeLayer()))
        QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(expected, oracle.activeLayer(), tile.coordinate));
    const auto pixels = readTile(incremental, layer, {0, 0});
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), incremental)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(readTile(render(renderer, recovered.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("pixel-perfect-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletionAndRecovery_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("idle-then-continue"),
                            QStringLiteral("idle-two"), QStringLiteral("tap"),
                            QStringLiteral("tap-after-two"), QStringLiteral("stationary")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletionAndRecovery()
{
    QFETCH(QString, kind);
    QList<InputSample> input{{{8, 32}, 1, 0, 0, 0, 0}, {{32, 8}, 1, 0, 0, 0, 8}, {{56, 32}, 1, 0, 0, 0, 16}};
    if (kind == QLatin1String("tap")) input = input.first(1);
    if (kind == QLatin1String("idle-two") || kind == QLatin1String("tap-after-two")) input = input.first(2);
    if (kind == QLatin1String("stationary")) for (auto &sample : input) sample.position = {8, 32};
    const bool idle = kind.startsWith(QLatin1String("idle"));
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.diameter = 4;
    brush.rgba = {17, 129, 241, 255};
    brush.basicSmoothing = true;
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (qsizetype index = 0; index < input.size(); ++index) {
        QCOMPARE(document.appendSamples(id, {input[index]}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        if (index == 0) QVERIFY(incremental->tiles(layer).isEmpty());
        if (index == 1 && idle) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental);
            QVERIFY(!incremental->tiles(layer).isEmpty());
        }
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(incremental->tiles(layer).size(), rebuilt->tiles(layer).size());
        for (const auto &tile : rebuilt->tiles(layer))
            QCOMPARE(readTile(incremental, layer, tile.coordinate), readTile(rebuilt, layer, tile.coordinate));
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto final = render(renderer, document.snapshot(), incremental);
    QVERIFY(final);
    Shape shape;
    QList<InputSample> expected;
    if (kind == QLatin1String("tap") || kind == QLatin1String("tap-after-two") || kind == QLatin1String("stationary")) {
        expected = {input.last()};
    } else {
        shape.kind = ShapeKind::Path;
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        expected = idle ? QList<InputSample>{{{8, 32}, 1}, {{15.68, 24.32}, 1}, {{24.32, 15.68}, 1}, {{32, 8}, 1}} :
                          QList<InputSample>{{{8, 32}, 1}, {{17.6, 22.4}, 1}, {{22.4, 8}, 1}, {{32, 8}, 1}};
        if (input.size() == 3) {
            shape.pathCommands.append(PathCommand::CubicTo);
            expected.append(idle ? QList<InputSample>{{{39.68, 15.68}, 1}, {{48.32, 24.32}, 1}, {{56, 32}, 1}} :
                                   QList<InputSample>{{{41.6, 8}, 1}, {{46.4, 22.4}, 1}, {{56, 32}, 1}});
        }
    }
    OperationDocument oracle(QSize(64, 64));
    brush.basicSmoothing = false;
    QCOMPARE(oracle.paintFigure(oracle.activeLayer(), brush, expected, shape, FigureStyle{}, 7), Result::Success);
    const auto renderedExpected = render(renderer, oracle.snapshot());
    QVERIFY(renderedExpected);
    const auto pixels = readTile(final, layer, {0, 0});
    QCOMPARE(pixels, readTile(renderedExpected, oracle.activeLayer(), {0, 0}));
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), input);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), final)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(readTile(render(renderer, restored.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("basic-smoothing-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::weightedSmoothingPreservesPixelsAndRecovery_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<QPointF>("fourth");
    QTest::addColumn<QPointF>("fifth");
    QTest::addColumn<qreal>("fourthPressure");
    QTest::addColumn<qreal>("fifthPressure");
    // Fixed results of the existing weighted-input rule in kis_tool_freehand_helper.cpp.
    QTest::newRow("weighted") << QStringLiteral("weighted") << QPointF(21.671799047212705, 17.296375248240203)
        << QPointF(26.190447445238277, 19.37998501314707) << qreal(0.31641004763936476) << qreal(0.16306585611055563);
    QTest::newRow("no-pressure") << QStringLiteral("no-pressure") << QPointF(21.671799047212705, 17.296375248240203)
        << QPointF(26.04262003540347, 19.310955108328365) << qreal(0.2) << qreal(0.1);
    QTest::newRow("no-tail") << QStringLiteral("no-tail") << QPointF(21.087831442146605, 17.427899484954125)
        << QPointF(25.512175466564386, 19.274354334843917) << qreal(0.3456084278926697) << qreal(0.19015482878006573);
    QTest::newRow("slow") << QStringLiteral("slow") << QPointF(20.070453973415706, 17.61942028617956)
        << QPointF(22.863223234608107, 19.016418949226647) << qreal(0.39647730132921466) << qreal(0.30008962757304986);
    QTest::newRow("fast") << QStringLiteral("fast") << QPointF(22.522529933316513, 17.04453976461341)
        << QPointF(26.243823024060887, 19.26420364596108) << qreal(0.2738735033341744) << qreal(0.15800647037967294);
    QTest::newRow("zero-distance") << QStringLiteral("zero-distance") << QPointF(24, 16) << QPointF(28, 20) << qreal(0.2) << qreal(0.1);
}

void KisGpuTileRendererTest::weightedSmoothingPreservesPixelsAndRecovery()
{
    QFETCH(QString, kind);
    QFETCH(QPointF, fourth);
    QFETCH(QPointF, fifth);
    QFETCH(qreal, fourthPressure);
    QFETCH(qreal, fifthPressure);
    QList<InputSample> input{{{8, 16}, 1, 0, 0, 0, 0, 0}, {{12, 20}, 0.8, 0, 0, 0, 8, 0.25},
                            {{16, 16}, 0.6, 0, 0, 0, 16, 0.5}, {{20, 20}, 0.4, 0, 0, 0, 24, 0.75},
                            {{24, 16}, 0.2, 0, 0, 0, 32, 1}, {{28, 20}, 0.1, 0, 0, 0, 40, 0}};
    if (kind == QLatin1String("slow") || kind == QLatin1String("fast"))
        for (auto &sample : input) sample.speed = kind == QLatin1String("fast") ? 1 : 0;
    Brush brush;
    brush.diameter = 4;
    brush.rgba = {17, 129, 241, 255};
    brush.basicSmoothing = true;
    brush.weightedSmoothing = WeightedSmoothing{24, 48, kind == QLatin1String("no-tail") ? 0.0 : 0.01,
                                               kind != QLatin1String("no-pressure")};
    if (kind == QLatin1String("zero-distance")) brush.weightedSmoothing->distanceMin = brush.weightedSmoothing->distanceMax = 0;
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 7);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    for (const auto &sample : input) {
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(incremental, layer, {0, 0}), readTile(rebuilt, layer, {0, 0}));
    }
    const auto pinned = incremental;
    const auto pinnedPixels = readTile(pinned, layer, {0, 0});
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto painted = render(renderer, document.snapshot(), incremental);
    QVERIFY(painted);
    auto expected = input;
    expected[4].position = fourth;
    expected[4].pressure = fourthPressure;
    expected[5].position = fifth;
    expected[5].pressure = fifthPressure;
    OperationDocument oracle(QSize(64, 64));
    brush.weightedSmoothing.reset();
    const auto reference = oracle.beginStroke(oracle.activeLayer(), brush, 7);
    QCOMPARE(oracle.appendSamples(reference, expected), Result::Success);
    QCOMPARE(oracle.flushSmoothing(reference), Result::Success);
    QCOMPARE(oracle.commitStroke(reference), Result::Success);
    const auto pixels = readTile(painted, layer, {0, 0});
    QCOMPARE(pixels, readTile(render(renderer, oracle.snapshot()), oracle.activeLayer(), {0, 0}));
    QCOMPARE(readTile(pinned, layer, {0, 0}), pinnedPixels);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), painted)->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(readTile(render(renderer, restored.snapshot()), layer, {0, 0}), pixels);
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last())->inputSamples(), input);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("weighted-smoothing-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTileRendererTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTileRendererTest.moc"
