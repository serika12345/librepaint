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

using namespace LibrePaint::Image::Gpu;
using LibrePaint::Image::Gpu::Test::legacyRecovery;

class KisGpuTileRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void transparentLayerPixelsFollowRootProjectionSemantics_data();
    void transparentLayerPixelsFollowRootProjectionSemantics();
    void layerCountChangesInvalidateTransparentProjectionReuse();
    void layerOpacityChangesProjectionWithoutChangingSourcePixelsOrRetainedVersions();
    void partialLayerOpacityComposesOverTransparency_data();
    void partialLayerOpacityComposesOverTransparency();
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

private:
    RenderedSnapshot render(TileRenderer &renderer, const Snapshot &snapshot,
                            const RenderedSnapshot &base = {});
    QByteArray readTile(const RenderedSnapshot &version, LayerId layer, QPoint coordinate);
    QByteArray readBuffer(WGPUBuffer pixels);
    ProjectedSnapshot project(TileRenderer &renderer, const RenderedSnapshot &version,
                              QRect visible, const ProjectedSnapshot &base = {});
    DevicePtr m_device;
};

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
    QVERIFY(!project(renderer, hidden, {0, 0, 64, 64}, composed));
    QCOMPARE(readBuffer(composed->tiles().first().pixels), original);
    composed.clear();
    const auto reduced = project(renderer, hidden, {0, 0, 64, 64});
    QVERIFY(reduced);
    QCOMPARE(readBuffer(reduced->tiles().first().pixels).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("bf80c0ff"));
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

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuTileRendererTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuTileRendererTest.moc"
