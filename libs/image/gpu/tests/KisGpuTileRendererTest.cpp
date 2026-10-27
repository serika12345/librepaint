/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QTest>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QScopeGuard>
#include <QThread>
#include <cstring>
#include <gpu/KisGpuTileRenderer.h>
#include <gpu/KisGpuDevice.h>
#include <gpu/KisGpuReadbackBatch_p.h>
#include <webgpu/wgpu.h>
#include "KisGpuGaussianBrushFixture_p.h"
#include "KisGpuGaussianBasicSmoothingFixture_p.h"
#include "KisGpuGaussianWeightedSmoothingFixture_p.h"
#include "KisGpuGaussianPixelPerfectFixture_p.h"
#include "gpu/KisGpuBasicCurveControls.h"

#include "KisGpuMultiplyProjectionFixture_p.h"
#include "KisGpuScreenProjectionFixture_p.h"
#include "KisGpuMinMaxProjectionFixture_p.h"
#include "KisGpuAdditionProjectionFixture_p.h"

using namespace LibrePaint::Image::Gpu;

class KisGpuTileRendererTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void basicCurveControlsPreserveClosedSegmentDecisions_data();
    void basicCurveControlsPreserveClosedSegmentDecisions();
    void initTestCase();
    void markerMatchesNormalEngine_data();
    void markerMatchesNormalEngine();
    void gaussianBasicKeepsCpuPixelsFlushes_data();
    void gaussianBasicKeepsCpuPixelsFlushes();
    void gaussianWeightedKeepsCpuPixels_data();
    void gaussianWeightedKeepsCpuPixels();
    void gaussianWeightedOnImportedPixelKeepsCpuMergeByte();
    void gaussianPixelCorrectionKeepsCpuExtent_data();
    void gaussianPixelCorrectionKeepsCpuExtent();
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
    void normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries_data();
    void normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries();
    void explicitPassThroughReadAfterParentKeepsSubtreePixels();
    void passThroughHierarchyMatchesNativeRootPixels_data();
    void passThroughHierarchyMatchesNativeRootPixels();
    void separableLayerProjectionMatchesNativePixels_data();
    void separableLayerProjectionMatchesNativePixels();
    void threeLayerProjectionFitsOneOutputTile_data();
    void threeLayerProjectionFitsOneOutputTile();
    void multiPassGroupsKeepDistinctLayerParameters_data();
    void multiPassGroupsKeepDistinctLayerParameters();
    void previewProjectionTracksBackgroundPropertiesAndRetainedPixels();
    void switchingDocumentsReleasesPreviewDerivedPixelsAtCapacity();
    void roundBrushProducesRgbaPixelsOnTheGpu();
    void selectionLimitsPaintFillAndEraseAcrossHistory();
    void featheredSelectionClipsPaintFillEraseHistory();
    void invertColorsComplementsRgbPreservesAlphaAndClipsSelection();
    void invertColorsInterpolatesFeatheredAndInvertedCoverage();
    void grayscaleUsesRec709CoefficientsWithIntegerRounding();
    void grayscaleClipsEllipseInvertedBinaryAndAlphaCoverage();
    void hsvAdjustmentUsesProductValuesAndNeutralPixels();
    void hsvAdjustmentClipsCoverageAndSupportsLegacyConversion();
    void yuvResetPreservesStoredPixels();
    void yuvUsesProductPrecision_data();
    void yuvUsesProductPrecision();
    void colorizeUsesProductLumaRounding_data();
    void colorizeUsesProductLumaRounding();
    void colorizePreservesDocumentLuma_data();
    void colorizePreservesDocumentLuma();
    void hsvAdjustmentUsesLumaDependentProductRulesAndFrozenCoefficients();
    void hsvAdjustmentLumaModesClipAlphaAndSelectionCoverage();
    void patternFillTilesAcrossSelectionHistory();
    void patternFillClipsAlphaCompositesErasesAndRejectsInvalidInput();
    void fillBlendModesMatchLayerComposition_data();
    void fillBlendModesMatchLayerComposition();
    void strokeBlendModesMatchFillBlendComposition_data();
    void strokeBlendModesMatchFillBlendComposition();
    void strokeBlendModePreservesOpacitySelectionAlphaLockAndChannelMask();
    void gaussianStrokeBlendModeMatchesSeparableComposition();
    void patternFillBlendModeMatchesLayerComposition();
    void fillBlendModePreservesOpacityAlphaLockAndChannelMask();
    void layerTransparencyMaskMultipliesCompositionAlpha();
    void gaussianWashProducesCoverageAndRetainsPixels();
    void gaussianEraseKeepsColorsHistory_data();
    void gaussianEraseKeepsColorsHistory();
    void gaussianFreehandKeepsInitialDistanceAndFinalTap_data();
    void gaussianFreehandKeepsInitialDistanceAndFinalTap();
    void gaussianWashSkipsDabsBelowTheBrushMinimum();
    void gaussianLineShapeEditsPreserveInputHistory_data();
    void gaussianLineShapeEditsPreserveInputHistory();
    void gaussianPathCallsKeepDistanceHistory_data();
    void gaussianPathCallsKeepDistanceHistory();
    void gaussianCubicPathsKeepPixelsInputHistory_data();
    void gaussianCubicPathsKeepPixelsInputHistory();
    void solidFiguresPreserveFillOutlineHistory_data();
    void solidFiguresPreserveFillOutlineHistory();
    void figureFillMatchesPainterCoverage_data();
    void figureFillMatchesPainterCoverage();
    void longFillEdgesKeepPixelsAcrossRegions();
    void emptyFillKeepsLineOutline();
    void filledInteriorRespectsRegionAndTileBudget();
    void collinearFillPreservesPixelsWithinTileBudget_data();
    void collinearFillPreservesPixelsWithinTileBudget();
    void removingFigureCoverageRestoresItsPreviewBackground();
    void pathCommandsPreserveCoverageHistory_data();
    void pathCommandsPreserveCoverageHistory();
    void symmetryPreservesPixelsOverlapHistory_data();
    void symmetryPreservesPixelsOverlapHistory();
    void symmetryKeepsSparseTilesAndIncrementalInputWithinBudget();
    void channelProtectionPreservesPixelsHistory_data();
    void channelProtectionPreservesPixelsHistory();
    void alphaLockPreservesCoverageHistory_data();
    void alphaLockPreservesCoverageHistory();
    void inputPartitionsAndPreviewProduceTheSamePixels();
    void basicSmoothingPreservesCurvesIdleCompletion_data();
    void basicSmoothingPreservesCurvesIdleCompletion();
    void weightedSmoothingPreservesPixels_data();
    void weightedSmoothingPreservesPixels();
    void pixelCorrectionPreservesCornersSensorsAndIncrementalPixels_data();
    void pixelCorrectionPreservesCornersSensorsAndIncrementalPixels();
    void unchangedTilesAreSharedAndBudgetsRejectAtomically();
    void erasingAndLayerVisibilityPreservePixelMeaning();
    void longDiagonalUsesOnlyIntersectingTiles();
    void shapesUndoRestoreTheSamePixels();
    void rectangleOutlinesPreserveRotationHistory_data();
    void rectangleOutlinesPreserveRotationHistory();
    void polylinesPreserveVerticesClosureHistory_data();
    void polylinesPreserveVerticesClosureHistory();
    void straightLinesPreserveIntermediatePressure_data();
    void straightLinesPreserveIntermediatePressure();
    void versionsOutliveTheRendererAndRejectAnotherDeviceOwner();
    void visibleLayersComposeOnTheGpuAndReuseUnchangedTiles();
    void compositionPreservesOrderAlphaAndExistingVersionsAtCapacity();
    void paintingAndCompositionShareOneSubmission();
    void longDocumentRebuildUsesBoundedTileStorage();
    void longStrokeRebuildKeepsPixelsAndHistory();
    void retainedPixelsOwnTheNativeDeviceUntilTheirLastReader();
    void nativeDeviceCreationRequiresTheApplicationThread();
    void rasterCheckpointsPreservePixelsThroughPaintingUndo();
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
    void regionalInputKeepsPixelsAcrossViewportAndCapacityChanges_data();
    void regionalInputKeepsPixelsAcrossViewportAndCapacityChanges();
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

void KisGpuTileRendererTest::markerMatchesNormalEngine_data()
{
    QTest::addColumn<bool>("sixteenBits");
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<bool>("moving");
    for (const bool sixteenBits : {false, true}) for (const qreal opacity : {qreal(1), qreal(.5)})
        for (const bool erase : {false, true}) for (const bool moving : {false, true}) {
            const auto name = QStringLiteral("%1%2-%3-%4")
                .arg(sixteenBits ? QStringLiteral("u16-") : QString())
                .arg(opacity == 1 ? QStringLiteral("opaque") : QStringLiteral("half"))
                .arg(erase ? QStringLiteral("erase") : QStringLiteral("normal"))
                .arg(moving ? QStringLiteral("moving") : QStringLiteral("tap"));
            QTest::newRow(qPrintable(name)) << sixteenBits << opacity << erase << moving;
        }
}

void KisGpuTileRendererTest::markerMatchesNormalEngine()
{
    QFETCH(bool, sixteenBits);
    QFETCH(qreal, opacity);
    const auto format = sixteenBits ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const int pixelSize = pixelByteCount(format);
    const int tileBytes = RgbaTile::byteCount(format);
    QFETCH(bool, erase);
    QFETCH(bool, moving);
    const QDir fixtures(QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/roundmarker"));
    const auto load = [&](const QString &name) {
        QFile file(fixtures.filePath(name));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    constexpr int side = 128;
    const auto initial = load(sixteenBits ? QStringLiteral("u16-initial.rgba") : QStringLiteral("initial.rgba"));
    const auto expected = load(QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba"));
    QCOMPARE(initial.size(), side * side * pixelSize);
    QCOMPARE(expected.size(), initial.size());
    QByteArray tile(tileBytes, '\0');
    for (int y = 0; y < side; ++y)
        std::memcpy(tile.data() + y * RgbaTile::Size * pixelSize, initial.constData() + y * side * pixelSize, side * pixelSize);
    RasterImage raster;
    raster.size = {side, side};
    raster.format = format;
    raster.layers.append({QStringLiteral("Mixed alpha"), true,
        QUuid(QStringLiteral("{1e75cc2a-b43b-4146-9216-ce12bc9ba3ae}")), {{{0, 0}, tile, format}}});
    OperationDocument document(raster);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    TileRenderer renderer(m_device);
    Brush brush;
    brush.rgba = sixteenBits ? RgbaPixel{65534, 257, 1, 65535} : RgbaPixel{17, 34, 51, 255};
    brush.diameter = 10;
    brush.spacing = .02;
    brush.opacity = opacity;
    brush.erase = erase;
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    const QList<QPointF> positions{{12.25, 13.75}, {60.125, 31.5}, {105.5, 91.25}};
    RenderedSnapshot pending;
    for (int i = 0; i < (moving ? positions.size() : 1); ++i) {
        InputSample sample;
        sample.position = positions[i];
        sample.pressure = 1;
        sample.timestamp = i * 16;
        QCOMPARE(document.appendSamples(id, {sample}), Result::Success);
        pending = render(renderer, document.preview(), pending);
        QVERIFY(pending);
    }
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto final = render(renderer, document.snapshot(), pending);
    QVERIFY(final);
    const auto actualTile = readTile(final, layer, {0, 0});
    QCOMPARE(actualTile.size(), tileBytes);
    QByteArray actual;
    for (int y = 0; y < side; ++y) actual.append(actualTile.mid(y * RgbaTile::Size * pixelSize, side * pixelSize));
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), actualTile);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), tile);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), actualTile);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {std::pair{QStringLiteral("actual"), actual},
                                  std::pair{QStringLiteral("expected"), expected}}) {
            QFile file(QDir(directory).filePath(QStringLiteral("marker-%1-%2.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag()), entry.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
    QCOMPARE(actual, expected);
}

void KisGpuTileRendererTest::basicCurveControlsPreserveClosedSegmentDecisions_data()
{
    QTest::addColumn<QPointF>("first");
    QTest::addColumn<QPointF>("last");
    QTest::addColumn<QPointF>("tangent1");
    QTest::addColumn<QPointF>("tangent2");
    QTest::addColumn<bool>("present");
    QTest::addColumn<QPointF>("expected1");
    QTest::addColumn<QPointF>("expected2");
    // Unquantized controls of the fixed Gaussian closed input keep its curve choice.
    QTest::newRow("closed-AB") << QPointF(0x1.88p+3, 0x1.b8p+3) << QPointF(0x1.e1p+5, 0x1.f8p+4) << QPointF(0x1.feaaaaaaaaaabp+0, 0x1.7aaaaaaaaaaabp-1) << QPointF(0x1.f155555555555p+0, 0x1.9d55555555555p+0)
        << true << QPointF(0x1.bb292a54b24b1p+4, 0x1.37a2faf94d778p+4) << QPointF(0x1.68a57010e8adfp+5, 0x1.2ff2bba60ec58p+4);
    QTest::newRow("closed-BC") << QPointF(0x1.e1p+5, 0x1.f8p+4) << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.f155555555555p+0, 0x1.9d55555555555p+0) << QPointF(-0x1.feaaaaaaaaaabp-1, -0x1.7aaaaaaaaaaabp-2)
        << true << QPointF(0x1.0d73333333346p+7, 0x1.760000000001fp+6) << QPointF(0x1.f2999999999acp+6, 0x1.8966666666676p+6);
    QTest::newRow("closed-CA") << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.88p+3, 0x1.b8p+3) << QPointF(-0x1.feaaaaaaaaaabp-1, -0x1.7aaaaaaaaaaabp-2) << QPointF(-0x1.f155555555555p+1, -0x1.9d55555555555p+1)
        << true << QPointF(0x1.a6p+6, 0x1.6dp+6) << QPointF(0x1.5b66666666666p+6, 0x1.2efffffffffffp+6);
    QTest::newRow("closed-endpoint") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF(5, 0) << QPointF(3, -4) << true << QPointF(3.2, 0) << QPointF(8.08, 2.56);
    QTest::newRow("closed-endpoint-reversed") << QPointF(10, 0) << QPointF(0, 0)
        << QPointF(-3, 4) << QPointF(-5, 0) << true << QPointF(8.08, 2.56) << QPointF(3.2, 0);
    QTest::newRow("parallel-midpoint") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF(3, 4) << QPointF(-3, -4) << true << QPointF(3.2, 0) << QPointF(6.8, 0);
    QTest::newRow("zero-tangent") << QPointF(0, 0) << QPointF(10, 0)
        << QPointF() << QPointF(3, 4) << false << QPointF() << QPointF();
}

void KisGpuTileRendererTest::basicCurveControlsPreserveClosedSegmentDecisions()
{
    QFETCH(QPointF, first);
    QFETCH(QPointF, last);
    QFETCH(QPointF, tangent1);
    QFETCH(QPointF, tangent2);
    QFETCH(bool, present);
    QFETCH(QPointF, expected1);
    QFETCH(QPointF, expected2);
    const auto controls = basicCurveControls(first, last, tangent1, tangent2);
    QCOMPARE(controls.has_value(), present);
    if (controls) {
        QCOMPARE((*controls)[0], expected1);
        QCOMPARE((*controls)[1], expected2);
    }
}

void KisGpuTileRendererTest::gaussianPixelCorrectionKeepsCpuExtent_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianPixelPerfectCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianPixelCorrectionKeepsCpuExtent()
{
    QFETCH(int, index);
    qInfo() << "Gaussian pixel correction platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianPixelPerfectCases()[index];
    QCOMPARE(test.rgba.size(), test.extent.width() * test.extent.height() * 4);
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.gaussianWash);
    brush.gaussianWash->flow = .57;
    brush.pixelPerfect = true;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00011-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    TileRenderer renderer(m_device);
    using Tiles = QMap<QPair<int, int>, QByteArray>;
    const auto tileIndex = [](int value) {
        return value >= 0 ? value / RgbaTile::Size : (value - RgbaTile::Size + 1) / RgbaTile::Size;
    };
    const auto tiles = [&](const RenderedSnapshot &version, LayerId target) {
        Tiles result;
        if (version) for (const auto &tile : version->tiles(target))
            result.insert({tile.coordinate.x(), tile.coordinate.y()}, readTile(version, target, tile.coordinate));
        return result;
    };
    const auto region = [&](const Tiles &source, QRect bounds) {
        QByteArray result(bounds.width() * bounds.height() * 4, '\0');
        for (int y = bounds.top(); y <= bounds.bottom(); ++y) for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const int tx = tileIndex(x), ty = tileIndex(y);
            const auto found = source.constFind({tx, ty});
            if (found == source.cend() || found->size() != RgbaTile::ByteCount) continue;
            const qsizetype offset = ((y - ty * RgbaTile::Size) * RgbaTile::Size + x - tx * RgbaTile::Size) * 4;
            std::memcpy(result.data() + ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4,
                        found->constData() + offset, 4);
        }
        return result;
    };
    Tiles expected;
    for (int y = 0; y < test.extent.height(); ++y) for (int x = 0; x < test.extent.width(); ++x) {
        const auto pixel = test.rgba.mid((y * test.extent.width() + x) * 4, 4);
        if (pixel == QByteArray(4, '\0')) continue;
        const int gx = test.extent.x() + x, gy = test.extent.y() + y;
        const int tx = tileIndex(gx), ty = tileIndex(gy);
        auto found = expected.find({tx, ty});
        if (found == expected.end()) found = expected.insert({tx, ty}, QByteArray(RgbaTile::ByteCount, '\0'));
        const qsizetype offset = ((gy - ty * RgbaTile::Size) * RgbaTile::Size + gx - tx * RgbaTile::Size) * 4;
        std::memcpy(found->data() + offset, pixel.constData(), 4);
    }
    QVERIFY(!expected.isEmpty());
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.full-extent.rgba"), test.rgba}};
    QJsonArray inputs, tileCoordinates, pendingTileCoordinates;
    for (const auto &sample : test.input)
        inputs.append(QJsonObject{{"x", sample.position.x()}, {"y", sample.position.y()},
            {"pressure", sample.pressure}, {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt},
            {"rotation", sample.rotation}, {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    QString phase = QStringLiteral("begin");
    qsizetype completedInputs = 0;
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase},
            {"completedInputs", int(completedInputs)}, {"input", inputs},
            {"extent", QJsonArray{test.extent.x(), test.extent.y(), test.extent.width(), test.extent.height()}},
            {"tileCoordinates", tileCoordinates}, {"pendingTileCoordinates", pendingTileCoordinates},
            {"tileSize", RgbaTile::Size}, {"flow", brush.gaussianWash->flow},
            {"opacity", brush.opacity}, {"failed", QTest::currentTestFailed()}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-pixel-perfect-") +
                QString::fromLatin1(test.name) + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    const auto packedTiles = [](const Tiles &source, QJsonArray &coordinates) {
        QByteArray result;
        for (auto it = source.cbegin(); it != source.cend(); ++it) {
            coordinates.append(QJsonArray{it.key().first, it.key().second});
            result.append(it.value());
        }
        return result;
    };
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<Tiles> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        completedInputs = i + 1;

        phase = QStringLiteral("input-%1-render").arg(i);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY2(incremental, "Gaussian pixel-corrected pending rendering must be supported");
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        const auto pending = tiles(incremental, layer);
        QVERIFY(pending == tiles(rebuilt, layer));
        // A raw tap, and every still-single corrected prefix, retain Gaussian's
        // empty stationary preview. A held corner is not flushed on release.
        const auto name = QByteArray(test.name);
        if (i < 2 || name.startsWith("static") || name == "held-corner" ||
            name == "initial-tile-removed" || (name == "flushed-corner" && i < 3))
            QVERIFY(pending.isEmpty());
        retained.append(incremental);
        retainedIntents.append(document.preview());
        retainedPixels.append(pending);
    }
    const auto pending = tiles(incremental, layer);
    artifacts[QStringLiteral(".pending.tiles.rgba")] = packedTiles(pending, pendingTileCoordinates);

    phase = QStringLiteral("pending-rebuild");
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QVERIFY(tiles(restoredPending, document.activeLayer()) == pending);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto retainedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto actual = tiles(final, document.activeLayer());
    artifacts[QStringLiteral(".tiles.rgba")] = packedTiles(actual, tileCoordinates);
    artifacts[QStringLiteral(".full-extent.rgba")] = region(actual, test.extent);
    artifacts[QStringLiteral(".rgba")] = region(actual, {0, 0, 128, 128});
    // Whole native tiles include zeros outside the CPU extent and expose any
    // abandoned first tile or extra off-canvas print, beyond the logical canvas.
    QVERIFY2(actual == expected, "Gaussian pixel correction must preserve the complete CPU pixels and sparse tiles");
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), retainedVersion);
    const auto replayed = render(renderer, document.snapshot());
    QVERIFY(replayed);
    QVERIFY(tiles(replayed, document.activeLayer()) == actual);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), final);
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QVERIFY(tiles(redone, document.activeLayer()) == actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto cancelled = render(renderer, document.snapshot(), incremental);
    QVERIFY(cancelled);
    QVERIFY(cancelled->tiles(layer).isEmpty());
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QVERIFY(tiles(retained[i], layer) == retainedPixels[i]);
        const auto replay = render(renderer, retainedIntents[i]);
        QVERIFY(replay);
        QVERIFY(tiles(replay, layer) == retainedPixels[i]);
    }
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianWeightedKeepsCpuPixels_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianWeightedCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianWeightedKeepsCpuPixels()
{
    QFETCH(int, index);
    qInfo() << "Gaussian Weighted platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianWeightedCases()[index];
    QCOMPARE(test.rgba.size(), 128 * 128 * 4);
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.gaussianWash);
    // Same captured Basic-5 resource; the observed CPU recipe changes FlowValue.
    brush.gaussianWash->flow = .57;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = test.smoothing;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00010-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    QVERIFY(!id.isNull());
    TileRenderer renderer(m_device);
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.rgba"), test.rgba}};
    QJsonArray input;
    for (const auto &sample : test.input)
        input.append(QJsonObject{{"x", sample.position.x()}, {"y", sample.position.y()},
            {"pressure", sample.pressure}, {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt},
            {"rotation", sample.rotation}, {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    QJsonArray flushes;
    for (const auto count : test.flushes) flushes.append(int(count));
    QString phase = QStringLiteral("begin");
    qsizetype completedInputs = 0;
    bool lastRenderValid = false;
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase},
            {"completedInputs", int(completedInputs)}, {"input", input}, {"flushes", flushes},
            {"distanceMin", test.smoothing.distanceMin}, {"distanceMax", test.smoothing.distanceMax},
            {"tail", test.smoothing.tailAggressiveness}, {"smoothPressure", test.smoothing.smoothPressure},
            {"flow", brush.gaussianWash->flow}, {"opacity", brush.opacity},
            {"lastRenderValid", lastRenderValid}, {"failed", QTest::currentTestFailed()},
            {"actualFinalPixelsSaved", artifacts.contains(QStringLiteral(".rgba"))}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-weighted-") +
                QString::fromLatin1(test.name) + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    const auto pixels = [&](const RenderedSnapshot &version, LayerId target) {
        if (!version) return QByteArray();
        const auto tile = readTile(version, target, {0, 0});
        if (tile.isEmpty()) return QByteArray(128 * 128 * 4, '\0');
        if (tile.size() != RgbaTile::ByteCount) return QByteArray();
        QByteArray bytes;
        for (int y = 0; y < 128; ++y) bytes.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
        return bytes;
    };
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        completedInputs = i + 1;

        phase = QStringLiteral("input-%1-render").arg(i);
        incremental = render(renderer, document.preview(), incremental);
        lastRenderValid = bool(incremental);
        QVERIFY2(incremental, "Gaussian Weighted pending rendering must be supported");
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        const auto pendingPixels = pixels(incremental, layer);
        QCOMPARE(pendingPixels.size(), 128 * 128 * 4);
        QCOMPARE(pendingPixels, pixels(rebuilt, layer));
        retainedIntents.append(document.preview());
        retained.append(incremental);
        retainedPixels.append(pendingPixels);
        if (test.flushes.contains(i + 1)) {
            phase = QStringLiteral("input-%1-flush").arg(i);
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            const auto once = document.preview();
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            QCOMPARE(document.preview(), once);

            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental);
            QCOMPARE(pixels(incremental, layer), pixels(render(renderer, document.preview()), layer));
            retainedIntents.append(document.preview());
            retained.append(incremental);
            retainedPixels.append(pixels(incremental, layer));
        }
    }
    const auto pendingPixels = pixels(incremental, layer);
    artifacts[QStringLiteral(".pending.rgba")] = pendingPixels;
    const auto retainedVersion = document.preview();
    phase = QStringLiteral("pending-rebuild");

    QCOMPARE(document.preview(), retainedVersion);
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(pixels(restoredPending, document.activeLayer()), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto capturedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    lastRenderValid = bool(final);
    QVERIFY(final);
    const auto actual = pixels(final, document.activeLayer());
    artifacts[QStringLiteral(".rgba")] = actual;
    QCOMPARE(actual, test.rgba);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QVERIFY(operation->smoothingFinished);
    QCOMPARE(operation->smoothingFlushes, test.flushes);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), capturedVersion);
    QCOMPARE(pixels(render(renderer, document.snapshot()), document.activeLayer()), actual);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixels(render(renderer, document.snapshot(), final), document.activeLayer()), QByteArray(actual.size(), '\0'));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(pixels(render(renderer, document.snapshot()), document.activeLayer()), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(pixels(render(renderer, document.snapshot(), incremental), layer), QByteArray(actual.size(), '\0'));
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QCOMPARE(pixels(retained[i], layer), retainedPixels[i]);
        QCOMPARE(pixels(render(renderer, retainedIntents[i]), layer), retainedPixels[i]);
    }
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianWeightedOnImportedPixelKeepsCpuMergeByte()
{
    qInfo() << "Gaussian Weighted imported pixel platform" << QGuiApplication::platformName();
    const auto test = Test::gaussianWeightedImportedPixelCase();
    const QPoint coordinate(-1, 0);
    const qsizetype offset = (5 * RgbaTile::Size + 247) * 4;
    const auto beforePixel = QByteArray::fromHex("9d1e656d");
    auto brush = Test::defaultGaussianBrush();
    QVERIFY(brush.gaussianWash);
    brush.rgba = {83, 177, 41, 255};
    brush.gaussianWash->flow = .57;
    brush.opacity = .57;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = test.smoothing;
    QMap<QString, QByteArray> artifacts{{QStringLiteral(".expected.rgba"), test.rgba},
                                      {QStringLiteral(".before.rgba"), beforePixel}};
    QString phase = QStringLiteral("begin");
    QJsonArray input;
    for (const auto &sample : test.input) input.append(QJsonObject{
        {"x", sample.position.x()}, {"y", sample.position.y()}, {"pressure", sample.pressure},
        {"xTilt", sample.xTilt}, {"yTilt", sample.yTilt}, {"rotation", sample.rotation},
        {"elapsed", qint64(sample.timestamp)}, {"speed", sample.speed}});
    const auto retain = qScopeGuard([&] {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return;
        QVERIFY(QDir().mkpath(directory));
        artifacts[QStringLiteral(".input.json")] = QJsonDocument(QJsonObject{
            {"platform", QGuiApplication::platformName()}, {"phase", phase}, {"input", input},
            {"flushes", QJsonArray{5, 6}}, {"x", -9}, {"y", 5},
            {"brushRgba", QJsonArray{83, 177, 41, 255}}, {"diameter", brush.diameter},
            {"spacing", brush.spacing},
            {"distanceMin", test.smoothing.distanceMin}, {"distanceMax", test.smoothing.distanceMax},
            {"tail", test.smoothing.tailAggressiveness}, {"smoothPressure", test.smoothing.smoothPressure},
            {"flow", brush.gaussianWash->flow}, {"opacity", brush.opacity},
            {"creamy", brush.gaussianWash->creamy}, {"failed", QTest::currentTestFailed()}}).toJson();
        for (auto it = artifacts.cbegin(); it != artifacts.cend(); ++it) {
            QFile file(QDir(directory).filePath(QStringLiteral("gaussian-weighted-imported-pixel") + it.key()));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(it.value()), qint64(it.value().size()));
        }
    });
    RgbaTile background{coordinate, QByteArray(RgbaTile::ByteCount, '\0')};
    background.rgba.replace(offset, 4, beforePixel);
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer layer; layer.name = QStringLiteral("Paint Layer 1");
    layer.uuid = QUuid(QStringLiteral("{33b00011-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {background}; canvas.layers = {layer};
    OperationDocument document(canvas);
    TileRenderer renderer(m_device);
    const auto imported = render(renderer, document.snapshot());
    QVERIFY(imported);
    QCOMPARE(readTile(imported, document.activeLayer(), coordinate), background.rgba);
    const auto id = document.beginStroke(document.activeLayer(), brush, 0);
    QVERIFY(!id.isNull());
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        if (test.flushes.contains(i + 1)) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            const auto once = document.preview();
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            QCOMPARE(document.preview(), once);
        }
    }
    const auto retainedVersion = document.preview();

    phase = QStringLiteral("pending-render");
    const auto pending = render(renderer, document.preview(), imported);
    QVERIFY(pending);
    const auto pendingPixels = readTile(pending, document.activeLayer(), coordinate);
    artifacts[QStringLiteral(".pending.tile.rgba")] = pendingPixels;
    phase = QStringLiteral("pending-rebuild");

    QCOMPARE(document.preview(), retainedVersion);
    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(readTile(restoredPending, document.activeLayer(), coordinate), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto capturedVersion = document.preview();

    phase = QStringLiteral("committed-render");
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto pixels = readTile(final, document.activeLayer(), coordinate);
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    artifacts[QStringLiteral(".tile.rgba")] = pixels;
    const auto actual = pixels.mid(offset, 4);
    artifacts[QStringLiteral(".rgba")] = actual;
    QCOMPARE(actual, test.rgba);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QCOMPARE(operation->smoothingFlushes, test.flushes);
    QVERIFY(operation->smoothingFinished);
    phase = QStringLiteral("committed-history");

    QCOMPARE(document.preview(), capturedVersion);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), coordinate), pixels);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), final), document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), coordinate), pixels);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), pending), document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(readTile(imported, document.activeLayer(), coordinate), background.rgba);
    QCOMPARE(readTile(pending, document.activeLayer(), coordinate), pendingPixels);
    phase = QStringLiteral("complete");
}

void KisGpuTileRendererTest::gaussianBasicKeepsCpuPixelsFlushes_data()
{
    QTest::addColumn<int>("index");
    const auto cases = Test::gaussianBasicCases();
    for (qsizetype i = 0; i < cases.size(); ++i) QTest::newRow(cases[i].name) << int(i);
}

void KisGpuTileRendererTest::gaussianBasicKeepsCpuPixelsFlushes()
{
    QFETCH(int, index);
    const auto test = Test::gaussianBasicCases()[index];
    auto brush = Test::defaultGaussianBrush();
    brush.basicSmoothing = true;
    RasterImage canvas; canvas.size = {128, 128};
    RasterLayer paintLayer; paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{33b00009-1122-3344-5566-778899aabbcc}"));
    canvas.layers = {paintLayer};
    OperationDocument document(canvas);
    const auto layer = document.activeLayer();
    const auto id = document.beginStroke(layer, brush, 0);
    TileRenderer renderer(m_device);
    RenderedSnapshot incremental;
    QList<RenderedSnapshot> retained;
    QList<Snapshot> retainedIntents;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(document.appendSamples(id, {test.input[i]}), Result::Success);
        incremental = render(renderer, document.preview(), incremental);
        QVERIFY(incremental);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(incremental, layer, {0, 0}), readTile(rebuilt, layer, {0, 0}));
        if (i < 2) QVERIFY(incremental->tiles(layer).isEmpty());
        retainedIntents.append(document.preview());
        retained.append(incremental);
        retainedPixels.append(readTile(incremental, layer, {0, 0}));
        if (i + 1 == test.flushAt) {
            QCOMPARE(document.flushSmoothing(id), Result::Success);
            incremental = render(renderer, document.preview(), incremental);
            QVERIFY(incremental && !incremental->tiles(layer).isEmpty());
            retainedIntents.append(document.preview());
            retained.append(incremental);
            retainedPixels.append(readTile(incremental, layer, {0, 0}));
        }
    }
    const auto pendingPixels = readTile(incremental, layer, {0, 0});

    const auto restoredPending = render(renderer, document.preview());
    QVERIFY(restoredPending);
    QCOMPARE(readTile(restoredPending, document.activeLayer(), {0, 0}), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto final = render(renderer, document.snapshot(), restoredPending);
    QVERIFY(final);
    const auto tile = readTile(final, document.activeLayer(), {0, 0});
    QCOMPARE(tile.size(), RgbaTile::ByteCount);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    const auto save = [&](const QString &suffix, const QByteArray &bytes) {
        QFile output(QDir(directory).filePath(QStringLiteral("gaussian-basic-") + QString::fromLatin1(test.name) + suffix));
        return output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size();
    };
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(save(QStringLiteral(".rgba"), pixels));
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), test.rgbaSha256);
    if (QByteArray(test.name).startsWith("stationary")) {
        OperationDocument tap(QSize(128, 128));
        const auto tapId = tap.beginStroke(tap.activeLayer(), brush, 0);
        QCOMPARE(tap.appendSamples(tapId, {test.input.last()}), Result::Success);
        QCOMPARE(tap.commitStroke(tapId), Result::Success);
        QCOMPARE(readTile(render(renderer, tap.snapshot()), tap.activeLayer(), {0, 0}), tile);
    }
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->inputSamples(), test.input);
    QCOMPARE(operation->brush, brush);
    QCOMPARE(operation->smoothingFlushes, test.flushAt ? QList<qsizetype>{test.flushAt} : QList<qsizetype>{});
    QVERIFY(operation->smoothingFinished);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), final)->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QVERIFY(render(renderer, document.snapshot(), incremental)->tiles(layer).isEmpty());
    QCOMPARE(document.undoCount(), 0);
    for (qsizetype i = 0; i < retained.size(); ++i) {
        QCOMPARE(readTile(retained[i], layer, {0, 0}), retainedPixels[i]);
        const auto rebuilt = render(renderer, retainedIntents[i]);
        QVERIFY(rebuilt);
        QCOMPARE(readTile(rebuilt, layer, {0, 0}), retainedPixels[i]);
    }
    // Eraser mode consumes the same spatial mask and preserves hidden RGB.
    auto maskBrush = brush;
    maskBrush.opacity = 1;
    OperationDocument mask(canvas);
    const auto maskId = mask.beginStroke(mask.activeLayer(), maskBrush, 0);
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(mask.appendSamples(maskId, {test.input[i]}), Result::Success);
        if (i + 1 == test.flushAt) QCOMPARE(mask.flushSmoothing(maskId), Result::Success);
    }
    QCOMPARE(mask.commitStroke(maskId), Result::Success);
    const auto maskRendered = render(renderer, mask.snapshot()); QVERIFY(maskRendered);
    const auto maskTile = readTile(maskRendered, mask.activeLayer(), {0, 0});
    QCOMPARE(maskTile.size(), RgbaTile::ByteCount);
    // CPU RGBA8 erase rounds .57 * 255 to 145, then multiplies mask and
    // destination alpha separately with round-to-nearest 8-bit products.
    const quint32 opacityByte = quint32(qRound(float(brush.opacity) * 255));
    QCOMPARE(opacityByte, 145u);
    const auto multiplyAlpha = [](quint32 a, quint32 b) { return (a * b + 127) / 255; };
    auto eraseBrush = brush;
    eraseBrush.erase = true;
    RgbaTile background{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *p = background.rgba.data() + 4 * (y * RgbaTile::Size + x);
        p[0] = char(x * 7 + y * 13); p[1] = char(x * 5 + y * 11); p[2] = char(x * 17 + y * 3);
        p[3] = char((x + y) % 256);
    }
    RasterImage raster; raster.size = {128, 128};
    RasterLayer rasterLayer; rasterLayer.name = QStringLiteral("Paint");
    rasterLayer.uuid = QUuid(QStringLiteral("{33b00008-1122-3344-5566-778899aabbcc}"));
    rasterLayer.tiles = {background}; raster.layers = {rasterLayer};
    OperationDocument erase(raster);
    const auto eraseId = erase.beginStroke(erase.activeLayer(), eraseBrush, 0);
    for (qsizetype i = 0; i < test.input.size(); ++i) {
        QCOMPARE(erase.appendSamples(eraseId, {test.input[i]}), Result::Success);
        if (i + 1 == test.flushAt) QCOMPARE(erase.flushSmoothing(eraseId), Result::Success);
    }

    QCOMPARE(erase.commitStroke(erase.activeOperation()), Result::Success);
    const auto erased = render(renderer, erase.snapshot()); QVERIFY(erased);
    const auto erasedTile = readTile(erased, erase.activeLayer(), {0, 0});
    QCOMPARE(erasedTile.size(), background.rgba.size());
    bool changedAlpha = false;
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        const auto offset = 4 * (y * RgbaTile::Size + x);
        QCOMPARE(erasedTile.mid(offset, 3), background.rgba.mid(offset, 3));
        const auto expectedAlpha = multiplyAlpha(quint8(background.rgba[offset + 3]),
            255 - multiplyAlpha(quint8(maskTile[offset + 3]), opacityByte));
        QCOMPARE(quint32(quint8(erasedTile[offset + 3])), expectedAlpha);
        changedAlpha = changedAlpha || erasedTile[offset + 3] != background.rgba[offset + 3];
    }
    QVERIFY(changedAlpha);
    const auto erasePaint = std::get<PaintSnapshot>(erase.snapshot()->layers.first().operations.last());
    QCOMPARE(erasePaint->inputSamples(), test.input); QVERIFY(erasePaint->brush.erase);
    QCOMPARE(erase.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, erase.snapshot(), erased), erase.activeLayer(), {0, 0}), background.rgba);
    QCOMPARE(erase.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, erase.snapshot()), erase.activeLayer(), {0, 0}), erasedTile);
    if (!directory.isEmpty()) {
        QByteArray pendingRgba, eraseRgba;
        for (int y = 0; y < 128; ++y) {
            pendingRgba.append(pendingPixels.isEmpty() ? QByteArray(128 * 4, '\0') :
                               pendingPixels.mid(y * RgbaTile::Size * 4, 128 * 4));
            eraseRgba.append(erasedTile.mid(y * RgbaTile::Size * 4, 128 * 4));
        }
        QVERIFY(save(QStringLiteral(".pending.rgba"), pendingRgba));
        QVERIFY(save(QStringLiteral(".erase.rgba"), eraseRgba));

    }
}

void KisGpuTileRendererTest::gaussianFreehandKeepsInitialDistanceAndFinalTap_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Actual NO_SMOOTHING mouse tool at 62cfadd521, shipped Basic-5,
    // full pressure, flow .5, canvas opacity .57 and the fixed alpha gradient.
    QTest::newRow("tap") << 0 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("two") << 1 << QByteArrayLiteral("4141da91dfe824fa1fe8c468c369e318bb62aac2da13a825aa936f020601513f");
    QTest::newRow("curve") << 2 << QByteArrayLiteral("9768513405d0c7bb276ff79b7a6475bbd15f76122c16cd02d90a857c45e450f2");
    QTest::newRow("closed") << 3 << QByteArrayLiteral("16a05f0a9afe0944736109eb6359ef301c5126040dafb51d4e178f9576032413");
    // Repeated stationary input uses the maintained single-tap baseline.
    QTest::newRow("stationary-two") << -2 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("stationary") << -3 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("stationary-eight") << -8 << QByteArrayLiteral("5e82950920eb254d739d256f1593368d009cab3fc5fbf5248537b5acf1f950ce");
    QTest::newRow("duplicate-first") << 5 << QByteArrayLiteral("6149e497bf814729e64b66d409c9ee605ef32900d4970afc0103080413c99725");
    QTest::newRow("return") << 6 << QByteArrayLiteral("add58eb08c2539e1fa3db420d884a0a21b835dc8b5b6d98a141ee36974879294");
    QTest::newRow("subspacing") << 7 << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
}

void KisGpuTileRendererTest::gaussianFreehandKeepsInitialDistanceAndFinalTap()
{
    QFETCH(int, kind);
    QFETCH(QByteArray, expectedSha256);
    const auto brush = Test::defaultGaussianBrush();
    QList<InputSample> input{{{12.25, 13.75}, 1}, {{60.125, 31.5}, 1}, {{105.5, 91.25}, 1}};
    if (kind == 0) input = input.first(1);
    else if (kind == 1) input = input.first(2);
    else if (kind == 3) input.append(input.first());
    else if (kind < 0) input = QList<InputSample>(-kind, input.first());
    else if (kind == 5) input = {input.first(), input.first(), input.last()};
    else if (kind == 6) input = {input.first(), input[1], input.first()};
    else if (kind == 7) input = {input.first(), {{input.first().position.x() + 1e-12, input.first().position.y()}, 1}};
    for (qsizetype i = 0; i < input.size(); ++i) {
        input[i].timestamp = i * 16;
        if (kind < 0) {
            input[i].pressure = qreal(i + 1) / input.size();
            input[i].xTilt = i * 7 - 28;
            input[i].yTilt = 14 - i * 3;
            input[i].rotation = i * 11;
            input[i].speed = qreal(i) / input.size();
        }
    }
    RgbaTile tile{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    const std::array<int, 8> alphas{{0, 1, 2, 63, 127, 128, 254, 255}};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *pixel = tile.rgba.data() + 4 * (y * RgbaTile::Size + x);
        pixel[0] = char((x * 7 + y * 13) % 256);
        pixel[1] = char((x * 5 + y * 11) % 256);
        pixel[2] = char((x * 17 + y * 3) % 256);
        pixel[3] = char(alphas[(x / 16 + y / 16) % alphas.size()]);
    }
    RasterImage image;
    image.size = {128, 128};
    RasterLayer layer;
    layer.name = QStringLiteral("Paint");
    layer.uuid = QUuid(QStringLiteral("{33b00004-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {tile};
    image.layers = {layer};
    OperationDocument document(image);
    const auto target = document.activeLayer();
    const auto stroke = document.beginStroke(target, brush, 0);
    TileRenderer renderer(m_device);
    RenderedSnapshot preview;
    QList<Snapshot> retainedIntents;
    QList<RenderedSnapshot> retainedPreviews;
    QList<QByteArray> retainedPixels;
    for (qsizetype i = 0; i < input.size(); ++i) {
        QCOMPARE(document.appendSamples(stroke, {input[i]}), Result::Success);
        preview = render(renderer, document.preview(), preview);
        QVERIFY(preview);
        if (!i || kind < 0) QCOMPARE(readTile(preview, target, {0, 0}), tile.rgba);
        const auto rebuilt = render(renderer, document.preview());
        QVERIFY(rebuilt);
        QCOMPARE(readTile(preview, target, {0, 0}), readTile(rebuilt, target, {0, 0}));
        retainedIntents.append(document.preview());
        retainedPreviews.append(preview);
        retainedPixels.append(readTile(preview, target, {0, 0}));
    }
    const auto pendingPixels = readTile(preview, target, {0, 0});

    const auto pendingPixelsVersion = render(renderer, document.preview());
    QVERIFY(pendingPixelsVersion);
    QCOMPARE(readTile(pendingPixelsVersion, document.activeLayer(), {0, 0}), pendingPixels);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto final = render(renderer, document.snapshot(), pendingPixelsVersion);
    QVERIFY(final);
    const auto actual = readTile(final, document.activeLayer(), {0, 0});
    for (qsizetype i = 0; i < retainedIntents.size(); ++i) {
        QCOMPARE(readTile(retainedPreviews[i], target, {0, 0}), retainedPixels[i]);
        QCOMPARE(readTile(render(renderer, retainedIntents[i]), target, {0, 0}), retainedPixels[i]);
    }
    QByteArray tapPixels;
    if (kind <= 0) {
        OperationDocument tap(image);
        const auto tapStroke = tap.beginStroke(tap.activeLayer(), brush, 0);
        QCOMPARE(tap.appendSamples(tapStroke, {input.last()}), Result::Success);
        QCOMPARE(tap.commitStroke(tapStroke), Result::Success);
        tapPixels = readTile(render(renderer, tap.snapshot()), tap.activeLayer(), {0, 0});
    }

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(actual.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-freehand-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    if (kind <= 0) QCOMPARE(actual, tapPixels);
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), target, {0, 0}), tile.rgba);
}

void KisGpuTileRendererTest::gaussianEraseKeepsColorsHistory_data()
{
    QTest::addColumn<int>("calls");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<qreal>("opacity");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Normal CPU figure erase at 62cfadd521, shipped Basic-5 preset,
    // pressure profile (.12,.91,.18), four worker threads and the fixed
    // alpha-gradient background below. Only BGRA/RGBA channel order changes.
    QTest::newRow("line") << 0 << qreal(0.5) << qreal(0.57)
        << QByteArrayLiteral("3c71e36b0b28727bf75c49a3f996f2fd94edd29e4675c70d5cba9b91cae8d0cd");
    QTest::newRow("cubic-open") << 5 << qreal(0.5) << qreal(0.57)
        << QByteArrayLiteral("d825b60fbe1ae9a35115c6cd573260a5b97d438be668c9a78075acdfeac6c5fe");
    QTest::newRow("cubic-closed") << 6 << qreal(0.5) << qreal(1)
        << QByteArrayLiteral("593e84384a8121987d9856c1148fe4e92c70d452d43cc04cd50a95d651f4f1e1");
    QTest::newRow("mixed") << 9 << qreal(1) << qreal(1)
        << QByteArrayLiteral("232813697f63e549ab6af1ede44dbdf42baa4594289e04fd5224145a6e35d951");
    QTest::newRow("zero-flow") << 0 << qreal(0) << qreal(1)
        << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
    QTest::newRow("zero-opacity") << 0 << qreal(1) << qreal(0)
        << QByteArrayLiteral("dc1393988897084bb62a1142bc4d7fc6c4d3b03c10fafe253824863cb311901f");
    QTest::newRow("half-opacity-byte") << 0 << qreal(1) << qreal(0.49607843137254903)
        << QByteArrayLiteral("59238903eb5152baf26cc05eaa97934fefa0c2243ea924692cef0e7a035069d3");
    QTest::newRow("low-opacity-byte") << 0 << qreal(1) << qreal(0.0058823529411764705)
        << QByteArrayLiteral("5498ca9ca9d477ca17eafd7fbcd49cbe9af263dbda817f959a1714acd73f0dbf");
}

void KisGpuTileRendererTest::gaussianEraseKeepsColorsHistory()
{
    QFETCH(int, calls);
    QFETCH(qreal, flow);
    QFETCH(qreal, opacity);
    QFETCH(QByteArray, expectedSha256);
    auto brush = Test::defaultGaussianBrush();
    brush.erase = true;
    brush.opacity = opacity;
    brush.gaussianWash->flow = flow;
    Shape shape;
    shape.kind = ShapeKind::Path;
    QList<InputSample> input;
    if (calls == 0) {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::LineTo};
        input = {{{12.25, 13.75}, .12}, {{60.125, 31.5}, .91}, {{60.125, 31.5}, .91}, {{105.5, 91.25}, .18}};
    } else {
        shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
        shape.pathCalls = {{0, 4}};
        input = {{{12.25, 13.75}, 1}, {{18.5, 48.75}, 1}, {{86.125, 5.25}, 1}, {{105.5, 91.25}, 1}};
        if (calls == 6) {
            input[1].position = {114.125, -15.75};
            input[2].position = {-26.375, 115.25};
            input[3].position = input[0].position;
        } else if (calls == 9) {
            input = {{{12.25, 13.75}, .2}, {{105.5, 25.75}, .8},
                     {{18.125, 91.5}, 1}, {{21.75, 43.125}, 1}, {{108.25, 120.375}, 1}, {{112.5, 78.25}, 1},
                     {{24.25, 48.125}, .9}, {{101.5, 53.75}, .4}};
            shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo,
                                  PathCommand::CubicTo, PathCommand::MoveTo, PathCommand::LineTo};
            shape.pathCalls = {{2, 6}};
        }
    }
    RgbaTile tile{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')};
    const std::array<int, 8> alphas{{0, 1, 2, 63, 127, 128, 254, 255}};
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
        auto *pixel = tile.rgba.data() + 4 * (y * RgbaTile::Size + x);
        pixel[0] = char((x * 7 + y * 13) % 256);
        pixel[1] = char((x * 5 + y * 11) % 256);
        pixel[2] = char((x * 17 + y * 3) % 256);
        pixel[3] = char(alphas[(x / 16 + y / 16) % alphas.size()]);
    }
    RasterImage image;
    image.size = {128, 128};
    RasterLayer layer;
    layer.name = QStringLiteral("Paint");
    layer.uuid = QUuid(QStringLiteral("{33b00003-1122-3344-5566-778899aabbcc}"));
    layer.tiles = {tile};
    image.layers = {layer};
    OperationDocument document(image);
    const auto target = document.activeLayer();
    TileRenderer renderer(m_device);
    const auto background = render(renderer, document.snapshot());
    QVERIFY(background);
    const auto stroke = document.beginStroke(target, brush, 1789);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto preview = render(renderer, document.preview(), background);
    QVERIFY(preview);
    const auto actual = readTile(preview, target, {0, 0});
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(actual.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-erase-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);
    for (qsizetype i = 0; i < actual.size(); i += 4) {
        QCOMPARE(actual.mid(i, 3), tile.rgba.mid(i, 3));
        QVERIFY(quint8(actual[i + 3]) <= quint8(tile.rgba[i + 3]));
    }
    QCOMPARE(readTile(background, target, {0, 0}), tile.rgba);
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), input);
    QCOMPARE(retained->brush, brush);

    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->brush, brush);
    if (!directory.isEmpty()) {
        const auto prefix = QStringLiteral("gaussian-erase-") + QString::fromLatin1(QTest::currentDataTag());
    }
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), tile.rgba);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(readTile(render(renderer, document.snapshot(), preview), target, {0, 0}), tile.rgba);
}

void KisGpuTileRendererTest::gaussianWashProducesCoverageAndRetainsPixels()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.rgba = {17, 34, 51, 255};
    brush.diameter = 40;
    auto &settings = brush.gaussianWash.emplace();
    settings.flow = .5;
    brush.opacity = .57;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device, 8 * TileRenderer::TileBytes);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto pixels = readTile(painted, document.activeLayer(), {0, 0});
    // One half-flow stamp has alpha 128 at its center; canvas opacity is
    // applied once at merge. The edge's Gaussian mask alpha is 126.
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("11223349"));
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 51) * 4, 4), QByteArray::fromHex("11223324"));
    QCOMPARE(pixels.mid((12 * TileRenderer::TileSize + 12) * 4, 4), QByteArray(4, '\0'));

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot());
    QVERIFY(undone);
    QVERIFY(undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(readTile(painted, document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::gaussianWashSkipsDabsBelowTheBrushMinimum()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.diameter = 40;
    auto &settings = brush.gaussianWash.emplace();
    brush.sizeResponse = {PressureResponse::Kind::Direct, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{32, 32}, .0002}}), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    QVERIFY(painted->tiles(document.activeLayer()).isEmpty());
}

void KisGpuTileRendererTest::gaussianLineShapeEditsPreserveInputHistory_data()
{
    QTest::addColumn<int>("inputKind");
    QTest::newRow("endpoints") << 0;
    QTest::newRow("projected-pressure") << 1;
    QTest::newRow("single-sample") << 2;
    QTest::newRow("coincident-input") << 3;
}

void KisGpuTileRendererTest::gaussianLineShapeEditsPreserveInputHistory()
{
    QFETCH(int, inputKind);
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 20;
    auto &settings = brush.gaussianWash.emplace();
    settings.flow = .5;
    brush.opacity = .57;
    brush.sizeResponse = {PressureResponse::Kind::Direct, 1, {}};
    settings.opacityResponse = {PressureResponse::Kind::Direct, 1, {}};
    QList<InputSample> samples{{{32, 40}, .2, .1, -.2, .3, 16}, {{224, 40}, .8, -.1, .2, -.3, 48}};
    if (inputKind == 1 || inputKind == 3) samples.insert(1, {{128, 76}, 1, .2, .1, .4, 32});
    if (inputKind == 2) samples = {samples.first()};
    if (inputKind == 3) for (auto &sample : samples) sample.position = {32, 40};

    OperationDocument document({512, 512});
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 1789);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto initialPixels = readTile(initial, layer, {0, 0});
    RenderedSnapshot previous = initial;
    QByteArray pixels;
    QPoint coordinate;
    for (const auto &shape : {Shape{ShapeKind::Line, {32, 368}, {224, 368}},
                              Shape{ShapeKind::Line, {304, 464}, {464, 384}}}) {
        // A newly entered straight path is the same drawing with explicit
        // positions. Original sensor values remain attached to their samples.
        auto placed = samples;
        if (placed.size() == 1) placed.append(placed.first());
        placed.first().position = shape.first;
        placed.last().position = shape.second;
        if (placed.size() == 3) placed[1].position = (shape.first + shape.second) / 2;
        OperationDocument reference({512, 512});
        const auto referenceStroke = reference.beginStroke(reference.activeLayer(), brush, 1789);
        QCOMPARE(reference.appendSamples(referenceStroke, placed), Result::Success);
        Shape path;
        path.kind = ShapeKind::Path;
        path.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo};
        if (placed.size() == 3) path.pathCommands.append(PathCommand::LineTo);
        QCOMPARE(reference.updateShape(referenceStroke, path), Result::Success);
        const auto expected = render(renderer, reference.preview());
        QVERIFY(expected);

        QCOMPARE(document.updateShape(stroke, shape), Result::Success);
        const auto edited = render(renderer, document.preview(), previous);
        QVERIFY(edited);
        coordinate = {int(shape.first.x()) / RgbaTile::Size, int(shape.first.y()) / RgbaTile::Size};
        QCOMPARE(edited->tiles(layer).size(), 1);
        pixels = readTile(edited, layer, coordinate);
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        QCOMPARE(pixels, readTile(expected, reference.activeLayer(), coordinate));
        const auto operation = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
        QCOMPARE(operation->inputSamples(), samples);
        previous = edited;
    }

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), coordinate), pixels);
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), samples);
    QCOMPARE(retained->brush, brush);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), previous);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
    QCOMPARE(document.undoCount(), 0);
    const auto accepted = document.beginStroke(layer, retained->brush, retained->randomSeed);
    QCOMPARE(document.appendSamples(accepted, retained->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(accepted, retained->shape, retained->figureStyle), Result::Success);
    QCOMPARE(document.commitStroke(accepted), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), coordinate), pixels);
    QCOMPARE(readTile(initial, layer, {0, 0}), initialPixels);
    QCOMPARE(readTile(previous, layer, coordinate), pixels);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-line-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
}

void KisGpuTileRendererTest::gaussianPathCallsKeepDistanceHistory_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Normal CPU figure calls at 1c93d9c2d4, with the preset revision captured
    // in the format-38 fixture. Only BGRA/RGBA channel order was converted.
    QTest::newRow("compound") << 0 << QByteArrayLiteral("3a7642cbafa29707d014a9b6285b5e9d631cb42aba252be4ee53c120700d7ee2");
    QTest::newRow("separate-paths") << 1 << QByteArrayLiteral("387f9e7f1c67aa72ff0f2bcc09c793569016bae32500b1eb7140973949c40e21");
    QTest::newRow("line-path-line") << 2 << QByteArrayLiteral("b63d3be4177fa29a8312bf026642143e3c10ef84c80d3d41a389465daa2d8e7f");
    QTest::newRow("path-line-path") << 3 << QByteArrayLiteral("c8a4dfcf172693154e832d4f83129bfbbfac0a1f1279907b7f016b62ed9c06e5");
}

void KisGpuTileRendererTest::gaussianPathCallsKeepDistanceHistory()
{
    QFETCH(int, kind);
    QFETCH(QByteArray, expectedSha256);
    const auto operation = Test::gaussianCompoundPathInput();
    auto shape = operation.shape;
    auto input = operation.inputSamples();
    if (kind >= 2) {
        shape.pathCommands.append({PathCommand::MoveTo, PathCommand::LineTo});
        input.append({{24.25, 48.125}, 1});
        input.append({{101.5, 53.75}, 1});
        if (kind == 2) {
            input[0].pressure = .2; input[1].pressure = .8;
            input[4].pressure = .9; input[5].pressure = .4;
        } else { input[2].pressure = .2; input[3].pressure = .8; }
    }
    OperationDocument document(QSize(128, 128));
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, operation.brush, 0);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto pinnedPixels = readTile(initial, layer, {0, 0});
    if (kind == 0) shape.pathCalls = {{0, 4}};
    else if (kind == 1) shape.pathCalls = {{0, 2}, {2, 4}};
    else if (kind == 2) shape.pathCalls = {{2, 4}};
    else shape.pathCalls = {{0, 2}, {4, 6}};
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto changed = render(renderer, document.preview(), initial);
    QVERIFY(changed);
    const auto tile = readTile(changed, layer, {0, 0});
    QCOMPARE(tile.size(), TileRenderer::TileBytes);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-path-calls-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);
    QCOMPARE(readTile(initial, layer, {0, 0}), pinnedPixels);

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->inputSamples(), input);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), undone), document.activeLayer(), {0, 0}), tile);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), changed);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
}

void KisGpuTileRendererTest::gaussianCubicPathsKeepPixelsInputHistory_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QByteArray>("expectedSha256");
    // Unchanged normal CPU figure calls at 2d57883297 with the captured brush.
    // Only BGRA/RGBA byte-channel order was converted for the complete images.
    QTest::newRow("open") << 0 << QByteArrayLiteral("fd9cfb1e45be12615d7764cc8250e9fbfe963a12086c79067c1b6be49f194841");
    QTest::newRow("closed") << 1 << QByteArrayLiteral("6e4827361a867bc92a15fe38c2d8f9f777f8ef15886a682f5d4941d869ada5ae");
    QTest::newRow("compound") << 2 << QByteArrayLiteral("6826252ac05a1a397cefc746c98b4061fe3da7ebffbc66cc04dfb900c3e25a10");
    QTest::newRow("separate") << 3 << QByteArrayLiteral("4500dcdb3d60097b00cbc222370dee64c2458992db7f65cf260701da1860b61e");
    QTest::newRow("line-cubic-line") << 4 << QByteArrayLiteral("efb5ac9576f8202c28e17aeb27296e183ca8f401b4ad820f1d6c05710f81e700");
}

void KisGpuTileRendererTest::gaussianCubicPathsKeepPixelsInputHistory()
{
    QFETCH(int, kind);
    QFETCH(QByteArray, expectedSha256);
    const auto brush = Test::defaultGaussianBrush();
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
    QList<InputSample> input{{{12.25, 13.75}, 1}, {{18.5, 48.75}, 1}, {{86.125, 5.25}, 1}, {{105.5, 91.25}, 1}};
    shape.pathCalls = {{0, 4}};
    if (kind == 1) {
        input[1].position = {114.125, -15.75};
        input[2].position = {-26.375, 115.25};
        input[3].position = input[0].position;
    } else if (kind >= 2) {
        const QList<InputSample> second{{{18.125, 91.5}, 1}, {{21.75, 43.125}, 1},
                                        {{108.25, 120.375}, 1}, {{112.5, 78.25}, 1}};
        if (kind == 4) {
            input = {{{12.25, 13.75}, .2}, {{105.5, 25.75}, .8}};
            input.append(second);
            input.append({{24.25, 48.125}, .9});
            input.append({{101.5, 53.75}, .4});
            shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo,
                                  PathCommand::CubicTo, PathCommand::MoveTo, PathCommand::LineTo};
            shape.pathCalls = {{2, 6}};
        } else {
            input.append(second);
            shape.pathCommands.append({PathCommand::MoveTo, PathCommand::CubicTo});
            shape.pathCalls = kind == 2 ? QList<QPair<qsizetype, qsizetype>>{{0, 8}} :
                                         QList<QPair<qsizetype, qsizetype>>{{0, 4}, {4, 8}};
        }
    }
    auto outline = shape;
    outline.pathCommands.clear();
    for (const auto command : shape.pathCommands) {
        if (command == PathCommand::CubicTo)
            outline.pathCommands.append({PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo});
        else outline.pathCommands.append(command);
    }
    OperationDocument document(QSize(128, 128));
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 0);
    QCOMPARE(document.appendSamples(stroke, input), Result::Success);
    QCOMPARE(document.updateShape(stroke, outline), Result::Success);
    TileRenderer renderer(m_device);
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    const auto initialPixels = readTile(initial, layer, {0, 0});
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto preview = document.preview();
    const auto changed = render(renderer, preview, initial);
    QVERIFY(changed);
    const auto tile = readTile(changed, layer, {0, 0});
    QCOMPARE(tile.size(), TileRenderer::TileBytes);
    QVERIFY(tile != initialPixels);
    QCOMPARE(readTile(initial, layer, {0, 0}), initialPixels);
    const auto retained = std::get<PaintSnapshot>(preview->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), input);
    QCOMPARE(retained->shape.pathCommands, shape.pathCommands);
    QCOMPARE(retained->shape.pathCalls, shape.pathCalls);
    QByteArray pixels;
    for (int y = 0; y < 128; ++y) pixels.append(tile.mid(y * RgbaTile::Size * 4, 128 * 4));
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("gaussian-cubic-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), pixels.size());
    }
    QCOMPARE(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex(), expectedSha256);

    const auto rebuilt = render(renderer, document.preview());
    QVERIFY(rebuilt);
    QCOMPARE(readTile(rebuilt, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), rebuilt);
    QVERIFY(undone && undone->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, document.activeLayer(), {0, 0}), tile);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
    const auto cancellationInput = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(document.undo(), Result::Success);
    const auto cancellation = document.beginStroke(document.activeLayer(), cancellationInput->brush, cancellationInput->randomSeed);
    QCOMPARE(document.appendSamples(cancellation, cancellationInput->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(cancellation, cancellationInput->shape, cancellationInput->figureStyle), Result::Success);
    QVERIFY(render(renderer, document.preview()));
    QCOMPARE(document.cancelStroke(cancellation), Result::Success);
    const auto cancelled = render(renderer, document.snapshot(), changed);
    QVERIFY(cancelled && cancelled->tiles(layer).isEmpty());
}

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
    QTest::addColumn<bool>("sixteenBits");
    QTest::newRow("transparent") << QByteArray::fromHex("00000000") << false;
    QTest::newRow("hidden-rgb") << QByteArray::fromHex("19334d00") << false;
    QTest::newRow("partial-alpha") << QByteArray::fromHex("19334d49") << false;
    QTest::newRow("native-transparent") << rgbaBytes({}, PixelFormat::Rgba16) << true;
    QTest::newRow("native-hidden-rgb") << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16) << true;
    QTest::newRow("native-partial-alpha") << rgbaBytes({1, 257, 65534, 32767}, PixelFormat::Rgba16) << true;
}

void KisGpuTileRendererTest::layerCropSharesInteriorPreservesBoundaryAndRetainedPixels()
{
    QFETCH(QByteArray, implicit);
    QFETCH(bool, sixteenBits);
    const auto format = sixteenBits ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    const int pixelSize = pixelByteCount(format);
    const int tileBytes = RgbaTile::byteCount(format);
    const QByteArray pixel = sixteenBits ? rgbaBytes({3, 256, 32768, 0}, format) : QByteArray::fromHex("11223300");
    const auto raw = pixel.repeated(tileBytes / pixelSize);
    RasterImage input;
    input.size = {768, 256};
    input.format = format;
    input.layers = {{QStringLiteral("Crop target"), true, {}, {}}};
    const auto defaultPixel = rgbaPixel(implicit, format);
    QVERIFY(defaultPixel);
    input.layers[0].defaultPixel = *defaultPixel;
    for (int x = -2; x <= 2; ++x) input.layers[0].tiles.append({{x, 0}, raw, format});
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
    QCOMPARE(renderer.residentBytes() - allocated, qsizetype(tileBytes));
    for (const auto &tile : after->tiles(layer)) {
        if (tile.coordinate.x() == 2) continue;
        const auto old = before->tiles(layer);
        const auto found = std::find_if(old.cbegin(), old.cend(), [&](const auto &value) { return value.coordinate == tile.coordinate; });
        QVERIFY(found != old.cend());
        QCOMPARE(tile.pixels, found->pixels);
    }
    auto expected = implicit.repeated(tileBytes / pixelSize);
    for (int y = 0; y < 256; ++y) std::memcpy(expected.data() + y * 256 * pixelSize, raw.constData(), 8 * pixelSize);
    QCOMPARE(readTile(after, layer, {2, 0}), expected);
    QCOMPARE(readTile(before, layer, {2, 0}), raw);
    QCOMPARE(readTile(before, layer, {-2, 0}), raw);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(renderer, document.snapshot(), after);
    QVERIFY(undone);
    QCOMPARE(readTile(undone, layer, {2, 0}), raw);
    QCOMPARE(document.redo(), Result::Success);

    TileRenderer rebuilt(m_device);
    const auto restored = render(rebuilt, document.snapshot());
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
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
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

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistory_data()
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

void KisGpuTileRendererTest::symmetryPreservesPixelsOverlapHistory()
{
    QFETCH(quint8, mirrorMode);
    QFETCH(QPoint, center);
    QFETCH(QPointF, position);
    QFETCH(bool, line);
    Brush brush;
    brush.rgba = {200, 70, 30, 255};
    brush.opacity = 128.0 / 255;
    brush.diameter = 8;
    const auto draw = [&](OperationDocument &document, const Brush &settings) {
        const auto stroke = document.beginStroke(document.activeLayer(), settings, 7);
        if (document.appendSamples(stroke, {{position, 1}}) != Result::Success) return OperationId{};
        if (line) {
            if (document.appendSamples(stroke, {{position + QPointF(4, 4), 0.5}}) != Result::Success ||
                document.updateShape(stroke, {ShapeKind::Line, position, position + QPointF(4, 4), 0}) != Result::Success)
                return OperationId{};
        }
        if (document.commitStroke(stroke) != Result::Success) return OperationId{};
        return stroke;
    };
    OperationDocument plain({64, 64});
    QVERIFY(!draw(plain, brush).isNull());
    TileRenderer renderer(m_device);
    const auto original = readTile(render(renderer, plain.snapshot()), plain.activeLayer(), {0, 0});
    QCOMPARE(original.size(), RgbaTile::ByteCount);
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
    const auto pixels = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    QCOMPARE(pixels, expected);

    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->brush, brush);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
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

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistory_data()
{
    QTest::addColumn<quint8>("writeMask");
    QTest::addColumn<quint8>("baseAlpha");
    QTest::addColumn<bool>("erase");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("alpha-transparent") << quint8(7) << quint8(0) << false << QByteArray::fromHex("4b328100");
    QTest::newRow("alpha-translucent") << quint8(7) << quint8(128) << false << QByteArray::fromHex("4b328180");
    QTest::newRow("protected-erase") << quint8(7) << quint8(128) << true << QByteArray::fromHex("0a28b454");
    QTest::newRow("red-only") << quint8(1) << quint8(128) << false << QByteArray::fromHex("4b28b480");
    QTest::newRow("alpha-only") << quint8(8) << quint8(128) << false << QByteArray::fromHex("0a28b4ab");
    QTest::newRow("green-and-alpha") << quint8(10) << quint8(128) << false << QByteArray::fromHex("0a37b4ab");
    QTest::newRow("all-protected") << quint8(0) << quint8(255) << false << QByteArray::fromHex("0a28b4ff");
}

void KisGpuTileRendererTest::channelProtectionPreservesPixelsHistory()
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
    brush.rgba = {200, 70, 30, 255};
    brush.diameter = 8;
    brush.opacity = 87.0 / 255;
    brush.erase = erase;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto pixels = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(pixels.mid((32 * RgbaTile::Size + 32) * 4, 4), expected);

    QCOMPARE(document.snapshot()->layers.first().channelWriteMask, writeMask);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("channel-protection-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}).mid((32 * RgbaTile::Size + 32) * 4, 4), pixel);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistory_data()
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

void KisGpuTileRendererTest::alphaLockPreservesCoverageHistory()
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
    brush.rgba = {200, 70, 30, 255};
    brush.diameter = 8;
    brush.opacity = 87.0 / 255;
    brush.erase = erase;
    brush.alphaLocked = true;
    const auto stroke = document.beginStroke(layer, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto preview = render(renderer, document.snapshot());
    QVERIFY(preview);
    const auto pixels = readTile(preview, layer, {0, 0});
    QCOMPARE(pixels.size(), RgbaTile::ByteCount);
    auto expected = pixel;
    const quint32 alpha = 87;
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

    const auto retained = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->brush, brush);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("alpha-lock-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(pixels), pixels.size());
    }
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

void KisGpuTileRendererTest::normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries_data()
{
    QTest::addColumn<bool>("implicit");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("destinationPixel");
    QTest::addColumn<int>("opacity");
    QTest::addColumn<QByteArray>("expectedPixel");
    QTest::addColumn<bool>("nativeFormat");
    for (const bool implicit : {false, true}) {
        QTest::newRow(implicit ? "default-pixels" : "tiles") << implicit
            << QByteArray::fromHex("7e7e7eff") << QByteArray::fromHex("818181ff")
            << 85 << QByteArray::fromHex("808080ff") << false;
        QTest::newRow(implicit ? "negative-half-default-pixels" : "negative-half-tiles") << implicit
            << QByteArray::fromHex("00000026") << QByteArray::fromHex("fdfdfd19")
            << 1 << QByteArray::fromHex("fbfbfb19") << false;
        // Full-opacity references retain binary32 fused color rounding followed
        // by nearest-even byte conversion, for implicit and stored layer pixels.
        QTest::newRow(implicit ? "full-opacity-even-up-default" : "full-opacity-even-up-tile") << implicit
            << QByteArray::fromHex("ffffff02") << QByteArray::fromHex("01010102")
            << 255 << QByteArray::fromHex("80808004") << false;
        QTest::newRow(implicit ? "full-opacity-even-down-default" : "full-opacity-even-down-tile") << implicit
            << QByteArray::fromHex("00000002") << QByteArray::fromHex("fefefe02")
            << 255 << QByteArray::fromHex("7e7e7e04") << false;
        QTest::newRow(implicit ? "full-opacity-quarter-default" : "full-opacity-quarter-tile") << implicit
            << QByteArray::fromHex("a9a9a902") << QByteArray::fromHex("00000006")
            << 255 << QByteArray::fromHex("2a2a2a08") << false;
        QTest::newRow(implicit ? "full-opacity-mixed-boundary-default" : "full-opacity-mixed-boundary-tile") << implicit
            << QByteArray::fromHex("ff005002") << QByteArray::fromHex("01fe1102")
            << 255 << QByteArray::fromHex("807e3104") << false;
        QTest::newRow(implicit ? "full-opacity-mixed-colors-default" : "full-opacity-mixed-colors-tile") << implicit
            << QByteArray::fromHex("1464c880") << QByteArray::fromHex("c8641480")
            << 255 << QByteArray::fromHex("50648cc0") << false;
        QTest::newRow(implicit ? "native-fma-default" : "native-fma-tile") << implicit
            << rgbaBytes({7435, 7030, 44014, 53742}, PixelFormat::Rgba16)
            << rgbaBytes({6428, 56750, 11086, 49993}, PixelFormat::Rgba16) << 255
            << rgbaBytes({7291, 14159, 39292, 62738}, PixelFormat::Rgba16) << true;
        QTest::newRow(implicit ? "native-zero-source-default" : "native-zero-source-tile") << implicit
            << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16)
            << rgbaBytes({65534, 257, 1, 65535}, PixelFormat::Rgba16) << 128
            << rgbaBytes({65534, 257, 1, 65535}, PixelFormat::Rgba16) << true;
        QTest::newRow(implicit ? "native-copy-default" : "native-copy-tile") << implicit
            << rgbaBytes({65534, 1, 257, 32768}, PixelFormat::Rgba16)
            << rgbaBytes({1, 257, 65534, 0}, PixelFormat::Rgba16) << 255
            << rgbaBytes({65534, 1, 257, 32768}, PixelFormat::Rgba16) << true;
    }
}

void KisGpuTileRendererTest::normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries()
{
    QFETCH(bool, implicit);
    QFETCH(bool, nativeFormat);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, destinationPixel);
    QFETCH(int, opacity);
    QFETCH(QByteArray, expectedPixel);
    const QUuid group(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const auto format = nativeFormat ? PixelFormat::Rgba16 : PixelFormat::Rgba8;
    // The extra transparent layer exercises the paired native projection pass.
    for (int extraLayer = 0; extraLayer <= int(nativeFormat); ++extraLayer) {
        RasterImage input;
        input.format = format;
        input.size = {64, 48};
        input.layers = {{QStringLiteral("Destination"), true, {}, {}, {129, 129, 129, 255}},
                        {QStringLiteral("Source"), true, {}, {}, {126, 126, 126, 255}},
                        {QStringLiteral("Group"), true, group, {}}};
        input.layers[0].parentUuid = input.layers[1].parentUuid = group;
        input.layers[1].opacity = quint8(opacity);
        input.layers[2].kind = LayerKind::Group;
        input.layers[0].defaultPixel = rgbaPixel(destinationPixel, format).value();
        input.layers[1].defaultPixel = rgbaPixel(sourcePixel, format).value();
        const auto expected = rgbaPixel(expectedPixel, format).value();
        const int pixelBytes = pixelByteCount(format);
        const int tileBytes = RgbaTile::byteCount(format);
        if (!implicit) {
            input.layers[0].tiles = {{{0, 0}, destinationPixel.repeated(tileBytes / pixelBytes), format}};
            input.layers[1].tiles = {{{0, 0}, sourcePixel.repeated(tileBytes / pixelBytes), format}};
        }
        if (extraLayer) {
            RasterLayer transparent{QStringLiteral("Transparent"), true, {}, {}, {1, 257, 65534, 0}};
            transparent.parentUuid = group;
            if (!implicit) transparent.tiles = {{{0, 0}, rgbaBytes(transparent.defaultPixel, format).repeated(tileBytes / pixelBytes), format}};
            input.layers.insert(2, transparent);
        }
        OperationDocument document(input);
        // Uniform defaults and materialized pixels share both the binary32
        // multiply-add rounding and the subsequent nearest-even byte conversion.
        QCOMPARE(document.snapshot()->projectionDefaultPixels().value(input.layers.size()), expected);
        TileRenderer renderer(m_device);
        const auto raw = render(renderer, document.snapshot());
        QVERIFY(raw);
        const auto projected = project(renderer, raw, {0, 0, 64, 48});
        QVERIFY(projected && projected->tiles().size() == 1);
        const auto actual = readBuffer(projected->tiles().first().pixels);
        QCOMPARE(actual, expectedPixel.repeated(tileBytes / pixelBytes));
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            QFile artifact(QDir(directory).filePath(QStringLiteral("normal-rounding-%1.rgba")
                .arg(QString::fromLatin1(QTest::currentDataTag()))));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(actual), actual.size());
        }
    }
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
    for (int mode : {1, 2, 3, 4, 5, 6}) {
        const auto references = mode == 1 ? LibrePaint::Image::Gpu::Test::multiplyProjectionReferences() :
            mode == 2 ? LibrePaint::Image::Gpu::Test::screenProjectionReferences() :
            mode == 3 ? LibrePaint::Image::Gpu::Test::darkenProjectionReferences() :
            mode == 4 ? LibrePaint::Image::Gpu::Test::lightenProjectionReferences() :
            LibrePaint::Image::Gpu::Test::additionProjectionReferences();
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
    const RgbaPixel colors[] = {{37, 89, 151, quint8(destinationAlpha)},
        blendMode >= int(LayerBlendMode::Addition) ? RgbaPixel{208, 224, 240, quint8(sourceAlpha)} :
        RgbaPixel{201, 41, 93, quint8(sourceAlpha)}};
    for (int index = 0; index < 2; ++index) {
        RasterLayer layer;
        layer.opacity = index == 0 ? 255 : opacity;
        layer.blendMode = LayerBlendMode::Normal;
        layer.projectionChannelMask = index == 0 ? 15 : mask;
        if (implicit) layer.defaultPixel = colors[index];
        else layer.tiles = {{{0, 0}, rgbaBytes(colors[index], PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4)}};
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

void KisGpuTileRendererTest::multiPassGroupsKeepDistinctLayerParameters_data()
{
    QTest::addColumn<bool>("hiddenRgb");
    QTest::newRow("transparent-intermediate") << true;
    QTest::newRow("different-paired-attributes") << false;
}

void KisGpuTileRendererTest::multiPassGroupsKeepDistinctLayerParameters()
{
    QFETCH(bool, hiddenRgb);
    RasterImage image;
    image.size = {768, 256};
    const QUuid groupUuid(QStringLiteral("{6a58d5cc-ef89-49a1-9c67-f36721c173f6}"));
    const int count = hiddenRgb ? 5 : 8;
    const quint8 opacities[] = {255, 137, 211, 255, 61, 255, 193, 59};
    for (int index = 0; index < count; ++index) {
        RasterLayer layer;
        layer.parentUuid = groupUuid;
        layer.opacity = hiddenRgb ? (index == 1 || index == 3 ? 1 : 255) : opacities[index];
        if (!hiddenRgb && index == 4) {
            layer.projectionChannelMask = 9;
            layer.blendMode = LayerBlendMode::Multiply;
        }
        if (!hiddenRgb && index == 6) {
            layer.projectionChannelMask = 6;
            layer.blendMode = LayerBlendMode::Screen;
        }
        for (int x = 0; x < 3; ++x) {
            QByteArray color;
            for (const int value : {17 + index * 23 + x * 5, 239 - index * 19 - x * 7,
                                    41 + index * 11 + x * 13, hiddenRgb ? (index == 1 ? 1 : 0) : 47 + index * 23})
                color.append(char(value));
            layer.tiles.append({{x, 0}, color.repeated(RgbaTile::ByteCount / 4)});
        }
        image.layers.append(layer);
    }
    image.layers.append({QStringLiteral("Multi-pass group"), true, groupUuid, {}});
    image.layers.last().kind = LayerKind::Group;
    const LayerId group = count + 1;
    // Uniform group defaults and materialized tiles have the same RGBA8 contract.
    const auto expected = [&](int x, bool changed) {
        auto uniform = image;
        for (int index = 0; index < count; ++index) {
            auto &layer = uniform.layers[index];
            const auto pixel = layer.tiles[x].rgba.first(4);
            for (int channel = 0; channel < 4; ++channel) layer.defaultPixel[channel] = quint8(pixel[channel]);
            layer.tiles.clear();
        }
        if (changed) uniform.layers[1].opacity = 255;
        const OperationDocument reference(uniform);
        const auto pixel = reference.snapshot()->projectionDefaultPixels().value(group);
        return rgbaBytes(pixel, PixelFormat::Rgba8).repeated(RgbaTile::ByteCount / 4);
    };
    OperationDocument document(image);
    TileRenderer renderer(m_device);
    const auto first = render(renderer, document.snapshot());
    QVERIFY(first);
    for (int x = 0; x < 3; ++x) QCOMPARE(readTile(first, group, {x, 0}), expected(x, false));
    QCOMPARE(document.setLayerOpacity(2, 255), Result::Success);
    const auto second = render(renderer, document.snapshot(), first);
    QVERIFY(second);
    for (int x = 0; x < 3; ++x) {
        QCOMPARE(readTile(second, group, {x, 0}), expected(x, true));
        QCOMPARE(readTile(first, group, {x, 0}), expected(x, false));
    }
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
    brush.opacity = 149.0 / 255;
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
    const auto retainedInput = document.preview();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto cancelled = render(renderer, document.snapshot(), current);
    QVERIFY(cancelled);
    QCOMPARE(cancelled->tiles(layer).size(), 2);
    for (int x = 0; x < 2; ++x) QCOMPARE(readTile(cancelled, layer, {x, 0}), background);
    QCOMPARE(pixels(current), finalPixels);

    const auto rebuilt = render(oracle, retainedInput);
    QVERIFY(rebuilt);
    QCOMPARE(pixels(rebuilt), finalPixels);
}

void KisGpuTileRendererTest::regionalInputKeepsPixelsAcrossViewportAndCapacityChanges_data()
{
    QTest::addColumn<int>("maximumTiles");
    QTest::newRow("budget-16") << 16;
    QTest::newRow("budget-96") << 96;
}

void KisGpuTileRendererTest::regionalInputKeepsPixelsAcrossViewportAndCapacityChanges()
{
    QFETCH(int, maximumTiles);
    RasterImage image;
    image.size = {1024, 512};
    RasterLayer background;
    background.defaultPixel = {27, 73, 149, 180};
    image.layers = {background};
    OperationDocument document(image);
    const auto layer = document.activeLayer();
    Brush brush;
    brush.diameter = 512;
    brush.rgba = {211, 37, 83, 137};
    brush.opacity = 173.0 / 255;
    const auto stroke = document.beginStroke(layer, brush, 73);
    QVERIFY(!stroke.isNull());
    const QList<InputSample> samples = {{{360, 256}, 1}, {{260, 256}, 0.6}, {{150, 200}, 0.3},
                                       {{180, 180}, 0.4}, {{170, 220}, 0.7}};
    const QList<QRect> regions = {{0, 0, 512, 512}, {0, 0, 512, 512}, {0, 0, 512, 512},
                                 {256, 0, 512, 512}, {256, 0, 256, 512}};
    TileRenderer renderer(m_device, maximumTiles * TileRenderer::TileBytes);
    TileRenderer reference(m_device);
    const auto fallback = QByteArray::fromHex("1b4995b4").repeated(TileRenderer::TileBytes / 4);
    const auto pixels = [&](const RenderedSnapshot &version, QRect region) {
        QByteArray result;
        for (int y = region.top(); y <= region.bottom(); y += TileRenderer::TileSize) {
            for (int x = region.left(); x <= region.right(); x += TileRenderer::TileSize) {
                const auto tile = readTile(version, layer, {x / TileRenderer::TileSize, y / TileRenderer::TileSize});
                result += tile.isEmpty() ? fallback : tile;
            }
        }
        return result;
    };
    RenderedSnapshot previous, pinned;
    QByteArray pinnedPixels, observations;
    for (int step = 0; step < samples.size(); ++step) {
        QCOMPARE(document.appendSamples(stroke, {samples[step]}), Result::Success);
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        const auto generated = renderer.recordRegion(commands, document.preview(), regions[step], {layer}, previous);
        const bool submitted = m_device->submitCommands(true);
        QCOMPARE(generated.code, RenderOutcome::Recorded);
        QVERIFY2(submitted, qPrintable(m_device->diagnostic()));
        const auto whole = render(reference, document.preview());
        QVERIFY(whole);
        const auto actual = pixels(generated.version, regions[step]);
        QCOMPARE(actual, pixels(whole, regions[step]));
        observations += actual;
        if (!pinned) {
            pinned = generated.version;
            pinnedPixels = actual;
        }
        QCOMPARE(pixels(pinned, regions.first()), pinnedPixels);
        QVERIFY(renderer.residentBytes() <= maximumTiles * TileRenderer::TileBytes);
        previous = generated.version;
    }
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    for (bool undo : {true, false}) {
        QCOMPARE(undo ? document.undo() : document.redo(), Result::Success);
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        const auto generated = renderer.recordRegion(commands, document.snapshot(), regions.last(), {layer}, previous);
        const bool submitted = m_device->submitCommands(true);
        QCOMPARE(generated.code, RenderOutcome::Recorded);
        QVERIFY2(submitted, qPrintable(m_device->diagnostic()));
        const auto whole = render(reference, document.snapshot());
        QVERIFY(whole);
        const auto actual = pixels(generated.version, regions.last());
        QCOMPARE(actual, pixels(whole, regions.last()));
        observations += actual;
        QCOMPARE(pixels(pinned, regions.first()), pinnedPixels);
        QVERIFY(renderer.residentBytes() <= maximumTiles * TileRenderer::TileBytes);
        previous = generated.version;
    }
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("regional-stroke-append-") +
                       QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        // Fixed RGBA8 inputs, seed 73, five viewports, undo and redo; row-major tiles.
        QCOMPARE(artifact.write(observations), observations.size());
    }
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
    brush.rgba = {0, 0, 255, 255};
    brush.opacity = 128.0 / 255;
    brush.diameter = 16;
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
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
    const auto retainedInput = document.preview();
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    auto cancelled = render(renderer, document.snapshot(), preview);
    QVERIFY(cancelled);
    QCOMPARE(readTile(cancelled, layer, {0, 0}), backgroundPixels);
    preview.clear();
    QCOMPARE(renderer.residentBytes(), qsizetype(TileRenderer::TileBytes));

    TileRenderer rebuiltRenderer(m_device, 3 * TileRenderer::TileBytes);
    const auto rebuiltBackground = render(rebuiltRenderer, document.snapshot());
    QVERIFY(rebuiltBackground);
    const auto intent = std::get<PaintSnapshot>(retainedInput->layers.first().operations.last());
    const auto next = document.beginStroke(layer, intent->brush, intent->randomSeed);
    QCOMPARE(document.appendSamples(next, intent->inputSamples()), Result::Success);
    QCOMPARE(document.updateFigure(next, intent->shape, intent->figureStyle), Result::Success);
    auto rebuilt = render(rebuiltRenderer, document.preview(), rebuiltBackground);
    QVERIFY(rebuilt);
    QCOMPARE(document.updateShape(document.activeOperation(), {ShapeKind::Line, {20, 140}, {200, 160}, 0}), Result::Success);
    const auto rebuiltChange = render(rebuiltRenderer, document.preview(), rebuilt);
    QVERIFY(rebuiltChange);
    QCOMPARE(readTile(rebuiltChange, document.activeLayer(), {0, 0}),
             readTile(render(oracle, document.preview()), document.activeLayer(), {0, 0}));
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
    if (!commands) {
        qWarning() << "GPU command recording unavailable" << m_device->diagnostic();
        return {};
    }
    const auto result = renderer.record(commands, snapshot, base);
    const auto submitted = m_device->submitCommands(true);
    if (result.code != RenderOutcome::Recorded) qWarning() << result.diagnostic;
    if (!submitted) {
        qWarning() << "GPU command submission failed" << m_device->diagnostic();
        return {};
    }
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
    if (!reads || !reads->recordBuffer(*m_device, commands, 0, pixels, wgpuBufferGetSize(pixels))) {
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
    const auto initial = render(renderer, document.preview());
    QVERIFY(initial);
    QVERIFY(initial->tiles(document.activeLayer()).isEmpty());
    QCOMPARE(document.appendSamples(stroke, {{{48, 32}, 1}}), Result::Success);
    const auto preview = render(renderer, document.preview(), initial);
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

void KisGpuTileRendererTest::selectionLimitsPaintFillAndEraseAcrossHistory()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {210, 40, 90, 255};
    brush.diameter = 32;
    QCOMPARE(document.setSelectionRect(QRect(64, 64, 64, 64)), Result::Success);
    const auto first = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(first, {{{32, 96}, 1}, {{224, 96}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->selectionRect, QRect(64, 64, 64, 64));
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 1, 1)), Result::EditInProgress);
    QCOMPARE(document.commitStroke(first), Result::Success);

    QCOMPARE(document.setSelectionRect(QRect(160, 64, 64, 64)), Result::Success);
    const auto second = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(second, {{{32, 96}, 1}, {{224, 96}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(second), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto bytes = readTile(painted, document.activeLayer(), {0, 0});
    QCOMPARE(pixel(bytes, 96, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(bytes, 192, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(bytes, 32, 96), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes, 224, 96), QByteArray(4, '\0'));

    // Both operations retain their own selection through replay.

    QCOMPARE(document.selectionRect(), QRect(160, 64, 64, 64));
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    QCOMPARE(document.undo(), Result::Success);
    const auto undone = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    QCOMPARE(pixel(undone, 96, 96), QByteArray::fromHex("d2285aff"));
    QCOMPARE(pixel(undone, 192, 96), QByteArray(4, '\0'));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    // Fill and erase use the same frozen selection.
    OperationDocument fillDocument({256, 256});
    QCOMPARE(fillDocument.setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    QCOMPARE(fillDocument.fillLayer(fillDocument.activeLayer(), {17, 34, 51}), Result::Success);
    const auto filled = readTile(render(renderer, fillDocument.snapshot()), fillDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(filled, 64, 64), QByteArray::fromHex("112233ff"));
    QCOMPARE(pixel(filled, 16, 64), QByteArray(4, '\0'));
    QCOMPARE(pixel(filled, 128, 64), QByteArray(4, '\0'));

    OperationDocument eraseDocument({256, 256});
    QCOMPARE(eraseDocument.fillLayer(eraseDocument.activeLayer(), {20, 80, 160}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionRect(QRect(32, 32, 64, 64)), Result::Success);
    Brush eraser = brush;
    eraser.rgba[3] = 255;
    eraser.erase = true;
    const auto erase = eraseDocument.beginStroke(eraseDocument.activeLayer(), eraser, 0);
    QCOMPARE(eraseDocument.appendSamples(erase, {{{64, 64}, 1}}), Result::Success);
    QCOMPARE(eraseDocument.commitStroke(erase), Result::Success);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseDocument.activeLayer(), {0, 0});
    QVERIFY(pixel(erased, 16, 16)[3] == char(255));
    QCOMPARE(pixel(erased, 64, 64), QByteArray::fromHex("1450a000"));
    QCOMPARE(eraseDocument.clearSelection(), Result::Success);
    QVERIFY(eraseDocument.selectionRect().isEmpty());
}

void KisGpuTileRendererTest::patternFillTilesAcrossSelectionHistory()
{
    const auto patternPixel = [](int x, int y) {
        const int px = ((x % 4) + 4) % 4;
        const int py = ((y % 4) + 4) % 4;
        QByteArray value(4, '\0');
        value[0] = char(px * 60 + 3);
        value[1] = char(py * 60 + 5);
        value[2] = char((px + py) * 20 + 7);
        value[3] = char(255);
        return value;
    };
    PatternFill pattern;
    pattern.size = QSize(4, 4);
    pattern.rgba = QByteArray(4 * 4 * 4, '\0');
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const auto value = patternPixel(x, y);
            std::memcpy(pattern.rgba.data() + (y * 4 + x) * 4, value.constData(), 4);
        }
    }
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };

    // The selection crosses all four 256-pixel tiles; the pattern repeats from
    // the document origin, so tile boundaries do not shift its phase.
    OperationDocument document({512, 512});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 120, 120)), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::FillLayer);
    TileRenderer renderer(m_device);
    auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto bytes00 = readTile(rendered, layer, {0, 0});
    const auto bytes10 = readTile(rendered, layer, {1, 0});
    const auto bytes01 = readTile(rendered, layer, {0, 1});
    const auto bytes11 = readTile(rendered, layer, {1, 1});
    QCOMPARE(bytes00.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes10.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes01.size(), TileRenderer::TileBytes);
    QCOMPARE(bytes11.size(), TileRenderer::TileBytes);
    QCOMPARE(pixel(bytes00, 200, 200), patternPixel(200, 200));
    QCOMPARE(pixel(bytes00, 201, 200), patternPixel(201, 200));
    QCOMPARE(pixel(bytes00, 255, 255), patternPixel(255, 255));
    QCOMPARE(pixel(bytes10, 0, 200), patternPixel(256, 200));
    QCOMPARE(pixel(bytes01, 200, 0), patternPixel(200, 256));
    QCOMPARE(pixel(bytes11, 0, 0), patternPixel(256, 256));
    QCOMPARE(pixel(bytes11, 63, 63), patternPixel(319, 319));
    QCOMPARE(pixel(bytes00, 199, 200), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes10, 64, 200), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes01, 200, 64), QByteArray(4, '\0'));
    QCOMPARE(pixel(bytes11, 64, 64), QByteArray(4, '\0'));

    // The fill is one history entry; undo removes the filled tiles and redo
    // restores the same pixels.
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(render(renderer, document.snapshot())->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    QCOMPARE(readTile(rendered, layer, {0, 0}), bytes00);
    QCOMPARE(readTile(rendered, layer, {1, 0}), bytes10);
    QCOMPARE(readTile(rendered, layer, {0, 1}), bytes01);
    QCOMPARE(readTile(rendered, layer, {1, 1}), bytes11);
    QCOMPARE(document.undoCount(), 2);
}

void KisGpuTileRendererTest::patternFillClipsAlphaCompositesErasesAndRejectsInvalidInput()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const QRect clip(QPoint(), QSize(64, 64));
    const QByteArray coverage = QByteArray::fromHex(
        "ff804001" "20c80af0" "50a01464" "fa05823c");
    const auto mask = SelectionMask::fromAlphaCoverage(QRect(16, 16, 4, 4), coverage, clip);
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(16, 16, 4, 4));
    QCOMPARE(mask->coverage(16, 16), quint8(255));
    QCOMPARE(mask->coverage(17, 16), quint8(128));

    // Pattern alpha times selection alpha times fill opacity, with straight
    // RGBA over a transparent destination.
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray(2 * 2 * 4, '\0');
    for (qsizetype i = 0; i < pattern.rgba.size(); i += 4) {
        pattern.rgba[i + 0] = char(10);
        pattern.rgba[i + 1] = char(20);
        pattern.rgba[i + 2] = char(30);
        pattern.rgba[i + 3] = char(200);
    }
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionMask(mask), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern, 128.0 / 255), Result::Success);
    TileRenderer renderer(m_device);
    const auto clipped = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(pixel(clipped, 16, 16), QByteArray::fromHex("0a141e64"));
    QCOMPARE(pixel(clipped, 17, 16), QByteArray::fromHex("0a141e32"));
    QCOMPARE(pixel(clipped, 18, 16), QByteArray::fromHex("0a141e19"));
    // Positive native float alpha rounds to zero while retaining straight RGB.
    QCOMPARE(pixel(clipped, 19, 16), QByteArray::fromHex("0a141e00"));
    QCOMPARE(pixel(clipped, 15, 16), QByteArray(4, '\0'));
    QCOMPARE(pixel(clipped, 20, 16), QByteArray(4, '\0'));

    // Erase mode removes pattern alpha times selection alpha from the retained
    // destination while preserving its RGB values.
    OperationDocument eraseDocument({64, 64});
    const auto eraseLayer = eraseDocument.activeLayer();
    QCOMPARE(eraseDocument.fillLayer(eraseLayer, {200, 30, 70}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionMask(mask), Result::Success);
    const auto undoBefore = eraseDocument.undoCount();
    QCOMPARE(eraseDocument.fillLayerPattern(eraseLayer, pattern, 255.0 / 255, true), Result::Success);
    QCOMPARE(eraseDocument.undoCount(), undoBefore + 1);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0});
    QCOMPARE(pixel(erased, 16, 16), QByteArray::fromHex("c81e4637"));
    QCOMPARE(pixel(erased, 17, 16), QByteArray::fromHex("c81e469b"));
    QCOMPARE(pixel(erased, 18, 16), QByteArray::fromHex("c81e46cd"));
    QCOMPARE(pixel(erased, 19, 16), QByteArray::fromHex("c81e46fe"));
    QCOMPARE(pixel(erased, 15, 16), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(eraseDocument.undo(), Result::Success);
    const auto restored = readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0});
    QCOMPARE(pixel(restored, 16, 16), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(eraseDocument.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, eraseDocument.snapshot()), eraseLayer, {0, 0}), erased);

    // Malformed frozen pixels are rejected before touching history or pixels.
    const auto before = document.snapshot();
    PatternFill malformed{QSize(2, 2), QByteArray(3, '\0')};
    QCOMPARE(document.fillLayerPattern(layer, malformed), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
}

void KisGpuTileRendererTest::fillBlendModesMatchLayerComposition_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply) << QByteArray::fromHex("0f0209ff");
    QTest::newRow("screen") << int(LayerBlendMode::Screen) << QByteArray::fromHex("df3156ff");
    QTest::newRow("darken") << int(LayerBlendMode::Darken) << QByteArray::fromHex("11112cff");
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("addition") << int(LayerBlendMode::Addition) << QByteArray::fromHex("ee335fff");
    QTest::newRow("linear-dodge") << int(LayerBlendMode::LinearDodge) << QByteArray::fromHex("ee335fff");
}

void KisGpuTileRendererTest::fillBlendModesMatchLayerComposition()
{
    QFETCH(int, mode);
    QFETCH(QByteArray, expected);
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 32, 32)), Result::Success);
    const auto historyBefore = document.undoCount();
    QCOMPARE(document.fillLayer(layer, {221, 17, 44}, 255.0 / 255, false, false, LayerBlendMode(mode)),
             Result::Success);
    QCOMPARE(document.undoCount(), historyBefore + 1);
    QCOMPARE(document.historyAction(historyBefore + 1), HistoryAction::FillLayer);
    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto filled = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(filled, 24, 24), expected);
    QCOMPARE(pixel(filled, 16, 16), expected);
    QCOMPARE(pixel(filled, 47, 47), expected);
    QCOMPARE(pixel(filled, 15, 16), base);
    QCOMPARE(pixel(filled, 48, 48), base);
    QCOMPARE(pixel(filled, 8, 8), base);
    QCOMPARE(quint8(pixel(filled, 24, 24).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    const auto undoneVersion = render(renderer, document.snapshot());
    QVERIFY(undoneVersion);
    const auto undone = readTile(undoneVersion, layer, {0, 0});
    QCOMPARE(pixel(undone, 24, 24), base);
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(renderer, document.snapshot());
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), filled);
}

void KisGpuTileRendererTest::strokeBlendModesMatchFillBlendComposition_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply) << QByteArray::fromHex("0f0209ff");
    QTest::newRow("screen") << int(LayerBlendMode::Screen) << QByteArray::fromHex("df3156ff");
    QTest::newRow("darken") << int(LayerBlendMode::Darken) << QByteArray::fromHex("11112cff");
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten) << QByteArray::fromHex("dd2233ff");
    QTest::newRow("addition") << int(LayerBlendMode::Addition) << QByteArray::fromHex("ee335fff");
    QTest::newRow("linear-dodge") << int(LayerBlendMode::LinearDodge) << QByteArray::fromHex("ee335fff");
}

void KisGpuTileRendererTest::strokeBlendModesMatchFillBlendComposition()
{
    QFETCH(int, mode);
    QFETCH(QByteArray, expected);
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 32;
    brush.blendMode = LayerBlendMode(mode);
    const auto stroke = document.beginStroke(layer, brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto painted = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(painted, 32, 32), expected);
    QCOMPARE(pixel(painted, 8, 8), base);
    QCOMPARE(quint8(pixel(painted, 32, 32).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), layer, {0, 0}), 32, 32), base);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), painted);
}

void KisGpuTileRendererTest::strokeBlendModePreservesOpacitySelectionAlphaLockAndChannelMask()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    const auto strokeAt = [](OperationDocument &document, const Brush &brush) {
        const auto id = document.beginStroke(document.activeLayer(), brush, 7);
        if (id.isNull()) return Result::InvalidInput;
        const auto result = document.appendSamples(id, {{{32, 32}, 1}});
        return result == Result::Success ? document.commitStroke(id) : result;
    };
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 32;
    brush.blendMode = LayerBlendMode::Multiply;
    TileRenderer renderer(m_device);

    // Brush opacity scales the source alpha before the separable blend.
    OperationDocument opacityDocument(QSize(64, 64));
    const auto opacityLayer = opacityDocument.activeLayer();
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {17, 34, 51}), Result::Success);
    brush.opacity = 128.0 / 255;
    QCOMPARE(strokeAt(opacityDocument, brush), Result::Success);
    const auto opacityPixels = readTile(render(renderer, opacityDocument.snapshot()), opacityLayer, {0, 0});
    QCOMPARE(pixel(opacityPixels, 32, 32), QByteArray::fromHex("10121eff"));

    // Selection coverage limits the blended source without changing the mode.
    OperationDocument selectionDocument(QSize(64, 64));
    const auto selectionLayer = selectionDocument.activeLayer();
    QCOMPARE(selectionDocument.fillLayer(selectionLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(selectionDocument.setSelectionRect(QRect(16, 16, 32, 32)), Result::Success);
    brush.opacity = 255.0 / 255;
    QCOMPARE(strokeAt(selectionDocument, brush), Result::Success);
    const auto selectionPixels = readTile(render(renderer, selectionDocument.snapshot()), selectionLayer, {0, 0});
    QCOMPARE(pixel(selectionPixels, 32, 32), QByteArray::fromHex("0f0209ff"));
    QCOMPARE(pixel(selectionPixels, 8, 8), base);

    // Alpha lock keeps the destination alpha and hidden RGB contract.
    OperationDocument alphaDocument(QSize(64, 64));
    const auto alphaLayer = alphaDocument.activeLayer();
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {17, 34, 51}, 128.0 / 255), Result::Success);
    brush.alphaLocked = true;
    QCOMPARE(strokeAt(alphaDocument, brush), Result::Success);
    const auto alphaPixels = readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0});
    QCOMPARE(pixel(alphaPixels, 32, 32), QByteArray::fromHex("76091a80"));
    brush.alphaLocked = false;

    // Protected channels keep their destination bytes after blending.
    OperationDocument channelDocument(QSize(64, 64));
    const auto channelLayer = channelDocument.activeLayer();
    QCOMPARE(channelDocument.fillLayer(channelLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(channelDocument.setLayerChannelMask(channelLayer, 14), Result::Success);
    QCOMPARE(strokeAt(channelDocument, brush), Result::Success);
    const auto channelPixels = readTile(render(renderer, channelDocument.snapshot()), channelLayer, {0, 0});
    QCOMPARE(pixel(channelPixels, 32, 32), QByteArray::fromHex("110209ff"));
}

void KisGpuTileRendererTest::gaussianStrokeBlendModeMatchesSeparableComposition()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 64;
    brush.blendMode = LayerBlendMode::Multiply;
    GaussianWash wash;
    brush.opacity = 1;
    wash.flow = 1;
    brush.sizeResponse = {PressureResponse::Kind::Constant, 1, {}};
    wash.opacityResponse = {PressureResponse::Kind::Constant, 1, {}};
    brush.gaussianWash = wash;
    const auto stroke = document.beginStroke(layer, brush, 11);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);

    TileRenderer renderer(m_device);
    const auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    const auto painted = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(painted, 32, 32), QByteArray::fromHex("0f0209ff"));
    QCOMPARE(pixel(painted, 8, 8), QByteArray::fromHex("112233ff"));
}

void KisGpuTileRendererTest::patternFillBlendModeMatchesLayerComposition()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto base = QByteArray::fromHex("112233ff");
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray(2 * 2 * 4, '\0');
    for (qsizetype index = 0; index < pattern.rgba.size(); index += 4) {
        pattern.rgba[index + 0] = char(34);
        pattern.rgba[index + 1] = char(68);
        pattern.rgba[index + 2] = char(102);
        pattern.rgba[index + 3] = char(200);
    }

    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {17, 34, 51}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 4, 4)), Result::Success);
    QCOMPARE(document.fillLayerPattern(layer, pattern, 255.0 / 255, false, false, LayerBlendMode::Multiply),
             Result::Success);
    TileRenderer renderer(m_device);
    const auto filled = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    // Pattern alpha 200 times selection alpha 255 and Multiply over the opaque base.
    const auto expected = QByteArray::fromHex("050e1bff");
    QCOMPARE(pixel(filled, 16, 16), expected);
    QCOMPARE(pixel(filled, 19, 19), expected);
    QCOMPARE(pixel(filled, 20, 16), base);
    QCOMPARE(pixel(filled, 8, 8), base);
    QCOMPARE(quint8(pixel(filled, 16, 16).at(3)), 255);

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), layer, {0, 0}), 16, 16), base);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filled);

    QCOMPARE(pixel(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), 16, 16),
             expected);
}

void KisGpuTileRendererTest::fillBlendModePreservesOpacityAlphaLockAndChannelMask()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    TileRenderer renderer(m_device);

    // Fill opacity multiplies the source alpha before the blend.
    OperationDocument opacityDocument(QSize(64, 64));
    const auto opacityLayer = opacityDocument.activeLayer();
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(opacityDocument.fillLayer(opacityLayer, {221, 17, 44}, 128.0 / 255, false, false,
                                       LayerBlendMode::Multiply), Result::Success);
    const auto opacityPixels = readTile(render(renderer, opacityDocument.snapshot()), opacityLayer, {0, 0});
    QCOMPARE(pixel(opacityPixels, 24, 24), QByteArray::fromHex("10121eff"));

    // Alpha lock keeps the destination alpha, including hidden RGB.
    OperationDocument alphaDocument(QSize(64, 64));
    const auto alphaLayer = alphaDocument.activeLayer();
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {17, 34, 51}, 128.0 / 255), Result::Success);
    QCOMPARE(alphaDocument.fillLayer(alphaLayer, {221, 17, 44}, 255.0 / 255, false, true,
                                     LayerBlendMode::Multiply), Result::Success);
    const auto alphaPixels = readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0});
    QCOMPARE(pixel(alphaPixels, 24, 24), QByteArray::fromHex("76091a80"));
    QCOMPARE(alphaDocument.undo(), Result::Success);
    QCOMPARE(pixel(readTile(render(renderer, alphaDocument.snapshot()), alphaLayer, {0, 0}), 24, 24),
             QByteArray::fromHex("11223380"));

    // Channel protection keeps the protected component while the writable
    // components use the same blend as composition.
    OperationDocument channelDocument(QSize(64, 64));
    const auto channelLayer = channelDocument.activeLayer();
    QCOMPARE(channelDocument.fillLayer(channelLayer, {17, 34, 51}), Result::Success);
    QCOMPARE(channelDocument.setLayerChannelMask(channelLayer, 14), Result::Success);
    QCOMPARE(channelDocument.fillLayer(channelLayer, {221, 17, 44}, 255.0 / 255, false, false,
                                       LayerBlendMode::Multiply), Result::Success);
    const auto channelPixels = readTile(render(renderer, channelDocument.snapshot()), channelLayer, {0, 0});
    QCOMPARE(pixel(channelPixels, 24, 24), QByteArray::fromHex("110209ff"));
}

void KisGpuTileRendererTest::invertColorsComplementsRgbPreservesAlphaAndClipsSelection()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("00000000"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Invert");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 64, 64)), Result::Success);
    QCOMPARE(document.invertColors(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::InvertColors);
    QCOMPARE(operation->selectionRect, QRect(0, 0, 64, 64));
    QVERIFY(!operation->selectionInverted);

    // Selection changes after acceptance do not rewrite the frozen coverage.
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(0, 0, 64, 64));

    TileRenderer renderer(m_device);
    const auto inverted = render(renderer, document.snapshot());
    QVERIFY(inverted);
    const auto invertedBytes = readTile(inverted, layer, {0, 0});
    QCOMPARE(pixel(invertedBytes, 10, 10), QByteArray::fromHex("f5ebe128"));
    // Transparent pixels keep alpha zero; their hidden RGB follows the same
    // complement as the channel-mask arithmetic used for locked painting.
    QCOMPARE(pixel(invertedBytes, 20, 20), QByteArray::fromHex("ffffff00"));
    QCOMPARE(pixel(invertedBytes, 30, 30), QByteArray::fromHex("007fffff"));
    QCOMPARE(pixel(invertedBytes, 200, 10), QByteArray::fromHex("11223344"));
    QCOMPARE(pixel(invertedBytes, 64, 64), QByteArray(4, '\0'));

    // One history entry: undo restores the exact source pixels, redo the filter.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto original = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    QCOMPARE(original, bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), invertedBytes);

    // Retained input preserves the frozen operation over the current document selection.

    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->selectionRect, QRect(0, 0, 64, 64));
}

void KisGpuTileRendererTest::invertColorsInterpolatesFeatheredAndInvertedCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto mixed = [](int base, int selected) {
        const int complement = 255 - base;
        const int keep = 255 - selected;
        return quint8((base * keep + complement * selected + 127) / 255);
    };
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto soft = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(soft);
    QVERIFY(soft->isAlpha());

    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(10, 10, QByteArray::fromHex("10203040"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Feather");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    QCOMPARE(document.invertColors(layer), Result::Success);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    TileRenderer renderer(m_device);
    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QByteArray fractional(4, '\0');
    fractional[0] = char(mixed(10, 128));
    fractional[1] = char(mixed(20, 128));
    fractional[2] = char(mixed(30, 128));
    fractional[3] = char(40);
    QCOMPARE(pixel(filteredBytes, 20, 20), fractional);
    QCOMPARE(pixel(filteredBytes, 30, 30), QByteArray::fromHex("007fffff"));
    // Unselected pixels remain byte-identical.
    QCOMPARE(pixel(filteredBytes, 10, 10), QByteArray::fromHex("10203040"));

    // An inverted rectangular selection complements only the outside.
    OperationDocument invertedDocument(input);
    const auto invertedLayer = invertedDocument.activeLayer();
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.invertColors(invertedLayer), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()), invertedLayer, {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("10203040"));
    QCOMPARE(pixel(inverted, 200, 10), QByteArray::fromHex("ffffff00"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    const auto ellipseLayer = ellipseDocument.activeLayer();
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.invertColors(ellipseLayer), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()), ellipseLayer, {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("10203040"));
    QCOMPARE(pixel(ellipse, 128, 128), QByteArray::fromHex("ffffff00"));
}

void KisGpuTileRendererTest::grayscaleUsesRec709CoefficientsWithIntegerRounding()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    // The documented rule: Rec.709 weights applied to stored sRGB-encoded
    // bytes, integer denominator 10000, round half up.
    const auto luma = [](int r, int g, int b) {
        return quint8((2126 * r + 7152 * g + 722 * b + 5000) / 10000);
    };
    const auto gray = [&](int r, int g, int b, int a) {
        const auto value = luma(r, g, b);
        QByteArray result(4, '\0');
        result[0] = result[1] = result[2] = char(value);
        result[3] = char(a);
        return result;
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));  // 10,20,30,40
    set(20, 20, QByteArray::fromHex("000100ff"));  // 0.7152 rounds up to 1
    set(30, 30, QByteArray::fromHex("000001ff"));  // 0.0722 rounds down to 0
    set(40, 40, QByteArray::fromHex("6496c880"));  // 100,150,200,128 rounds up
    set(50, 50, QByteArray::fromHex("ff0000ff"));
    set(60, 60, QByteArray::fromHex("00ff00ff"));
    set(70, 70, QByteArray::fromHex("0000ffff"));
    set(80, 80, QByteArray::fromHex("808080ff"));  // neutral gray is preserved
    set(90, 90, QByteArray::fromHex("ffffff00"));  // hidden RGB, alpha stays 0
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Grayscale");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(document.grayscale(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);

    // The filter kind and coverage are frozen in the accepted operation.
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::Grayscale);
    QCOMPARE(operation->selectionRect, QRect(0, 0, 100, 100));
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(0, 0, 100, 100));

    TileRenderer renderer(m_device);
    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QCOMPARE(pixel(filteredBytes, 10, 10), gray(10, 20, 30, 40));
    QCOMPARE(pixel(filteredBytes, 20, 20), gray(0, 1, 0, 255));
    QCOMPARE(pixel(filteredBytes, 30, 30), gray(0, 0, 1, 255));
    QCOMPARE(pixel(filteredBytes, 40, 40), gray(100, 150, 200, 128));
    QCOMPARE(pixel(filteredBytes, 50, 50), gray(255, 0, 0, 255));
    QCOMPARE(pixel(filteredBytes, 60, 60), gray(0, 255, 0, 255));
    QCOMPARE(pixel(filteredBytes, 70, 70), gray(0, 0, 255, 255));
    QCOMPARE(pixel(filteredBytes, 80, 80), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filteredBytes, 90, 90), gray(255, 255, 255, 0));
    QCOMPARE(pixel(filteredBytes, 200, 10), QByteArray::fromHex("11223344"));
    QCOMPARE(luma(255, 0, 0), quint8(54));
    QCOMPARE(luma(0, 255, 0), quint8(182));
    QCOMPARE(luma(0, 0, 255), quint8(18));
    QCOMPARE(luma(0, 1, 0), quint8(1));
    QCOMPARE(luma(0, 0, 1), quint8(0));

    // One filter history entry: undo restores the source bytes, redo the result.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // Retained input preserves the kind, the frozen coverage and the same pixels.

    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->kind, FilterKind::Grayscale);
    QCOMPARE(retained->selectionRect, QRect(0, 0, 100, 100));
}

void KisGpuTileRendererTest::grayscaleClipsEllipseInvertedBinaryAndAlphaCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto luma = [](int r, int g, int b) {
        return quint8((2126 * r + 7152 * g + 722 * b + 5000) / 10000);
    };
    const auto gray = [&](int r, int g, int b, int a) {
        const auto value = luma(r, g, b);
        QByteArray result(4, '\0');
        result[0] = result[1] = result[2] = char(value);
        result[3] = char(a);
        return result;
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("10203040"));
    set(30, 30, QByteArray::fromHex("ff8000ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    set(120, 120, QByteArray::fromHex("c86432ff"));
    set(128, 128, QByteArray::fromHex("141e28ff"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // A binary freehand mask clips exactly to its selected tiles and pixels.
    OperationDocument binaryDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray binaryCoverage(256 * 256, '\0');
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) binaryCoverage[y * 256 + x] = char(255);
    const auto binary = SelectionMask::fromCoverage(clip, binaryCoverage, clip);
    QVERIFY(binary);
    QCOMPARE(binaryDocument.setSelectionMask(binary), Result::Success);
    QCOMPARE(binaryDocument.grayscale(binaryDocument.activeLayer()), Result::Success);
    const auto binaryBytes = readTile(render(renderer, binaryDocument.snapshot()), binaryDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(binaryBytes, 10, 10), QByteArray::fromHex("13131328"));
    QCOMPARE(pixel(binaryBytes, 20, 20), gray(16, 32, 48, 64));
    QCOMPARE(pixel(binaryBytes, 30, 30), QByteArray::fromHex("929292ff"));
    QCOMPARE(pixel(binaryBytes, 100, 100), QByteArray::fromHex("11223344"));

    // Alpha coverage interpolates between the source and the gray value.
    OperationDocument alphaDocument(input);
    QByteArray alphaCoverage(256 * 256, '\0');
    alphaCoverage[20 * 256 + 20] = char(128);
    alphaCoverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, alphaCoverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.grayscale(alphaDocument.activeLayer()), Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()), alphaDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("171f2740"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("929292ff"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));

    // An inverted rectangle filters only the outside.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.grayscale(invertedDocument.activeLayer()), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()), invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("20202044"));
    QCOMPARE(pixel(inverted, 120, 120), QByteArray::fromHex("767676ff"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.grayscale(ellipseDocument.activeLayer()), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()), ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(ellipse, 128, 128), gray(20, 30, 40, 255));
}

void KisGpuTileRendererTest::hsvAdjustmentUsesProductValuesAndNeutralPixels()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("c81e46ff"));  // 200, 30, 70, 255
    set(20, 20, QByteArray::fromHex("808080ff"));  // neutral gray
    set(30, 30, QByteArray::fromHex("000000ff"));  // black
    set(40, 40, QByteArray::fromHex("646464ff"));  // neutral gray 100
    set(50, 50, QByteArray::fromHex("0a141e28"));  // 10, 20, 30, alpha 40
    set(200, 10, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("HSV");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // Saturation moves chroma but not value: the product's modern HSV rule.
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const HsvAdjustment saturation{0, 50, 0, HsvAdjustmentType::Hsv, false};
    QCOMPARE(document.hsvAdjustment(layer, saturation), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->kind, FilterKind::HsvAdjustment);
    QCOMPARE(operation->hue, 0);
    QCOMPARE(operation->saturation, 50);
    QCOMPARE(operation->value, 0);
    QCOMPARE(operation->hsvType, HsvAdjustmentType::Hsv);
    QVERIFY(!operation->compatibilityMode);

    const auto filtered = render(renderer, document.snapshot());
    QVERIFY(filtered);
    const auto filteredBytes = readTile(filtered, layer, {0, 0});
    QCOMPARE(pixel(filteredBytes, 10, 10), QByteArray::fromHex("c8002fff"));
    // Neutral pixels ignore hue and saturation; alpha is carried through.
    QCOMPARE(pixel(filteredBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(filteredBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(filteredBytes, 40, 40), QByteArray::fromHex("646464ff"));
    QCOMPARE(pixel(filteredBytes, 50, 50), QByteArray::fromHex("000f1e28"));
    QCOMPARE(pixel(filteredBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Frozen settings survive a later selection edit.
    QCOMPARE(document.setSelectionRect(QRect(200, 200, 8, 8)), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // One filter history entry: undo restores the source, redo the result.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), filteredBytes);

    // Retained input preserves the frozen settings and the same pixels.

    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto retained = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retained->saturation, 50);
    QCOMPARE(retained->value, 0);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), filteredBytes);

    // Value moves value and chroma toward the endpoint; saturation is unchanged
    // in the product rule, so a neutral pixel simply gets brighter.
    OperationDocument valueDocument(input);
    const auto valueLayer = valueDocument.activeLayer();
    QCOMPARE(valueDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(valueDocument.hsvAdjustment(valueLayer, {0, 0, 25, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto valueBytes = readTile(render(renderer, valueDocument.snapshot()), valueLayer, {0, 0});
    QCOMPARE(pixel(valueBytes, 10, 10), QByteArray::fromHex("d65674ff"));
    QCOMPARE(pixel(valueBytes, 20, 20), QByteArray::fromHex("a0a0a0ff"));
    QCOMPARE(pixel(valueBytes, 30, 30), QByteArray::fromHex("404040ff"));
    QCOMPARE(pixel(valueBytes, 50, 50), QByteArray::fromHex("474f5628"));

    // The byte conversion rounds half up, matching the CPU rule.
    OperationDocument rounding(input);
    const auto roundingLayer = rounding.activeLayer();
    QCOMPARE(rounding.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(rounding.hsvAdjustment(roundingLayer, {0, 0, 10, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto roundingBytes = readTile(render(renderer, rounding.snapshot()), roundingLayer, {0, 0});
    // 100 + 0.1 * (255 - 100) = 115.5 in byte units, rounded to 116.
    QCOMPARE(pixel(roundingBytes, 40, 40), QByteArray::fromHex("747474ff"));
    QCOMPARE(pixel(roundingBytes, 20, 20), QByteArray::fromHex("8d8d8dff"));

    // Hue rotation leaves neutral pixels unchanged.
    OperationDocument hueDocument(input);
    const auto hueLayer = hueDocument.activeLayer();
    QCOMPARE(hueDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hueDocument.hsvAdjustment(hueLayer, {60, 0, 0, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto hueBytes = readTile(render(renderer, hueDocument.snapshot()), hueLayer, {0, 0});
    QCOMPARE(pixel(hueBytes, 10, 10), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(hueBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // The product `type` key selects the HSL policy; the same slider values
    // produce a different result from the HSV policy.
    OperationDocument hslDocument(input);
    const auto hslLayer = hslDocument.activeLayer();
    QCOMPARE(hslDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hslDocument.hsvAdjustment(hslLayer, {0, 50, 0, HsvAdjustmentType::Hsl, false}),
             Result::Success);
    const auto hslOperation = std::get<FilterSnapshot>(hslDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(hslOperation->hsvType, HsvAdjustmentType::Hsl);
    const auto hslBytes = readTile(render(renderer, hslDocument.snapshot()), hslLayer, {0, 0});
    QCOMPARE(pixel(hslBytes, 10, 10), QByteArray::fromHex("e60036ff"));
    QCOMPARE(pixel(hslBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // HSI (type 2) uses the product's HCI policy.
    OperationDocument hsiDocument(input);
    const auto hsiLayer = hsiDocument.activeLayer();
    QCOMPARE(hsiDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(hsiDocument.hsvAdjustment(hsiLayer, {0, 50, 0, HsvAdjustmentType::Hsi, false}),
             Result::Success);
    const auto hsiBytes = readTile(render(renderer, hsiDocument.snapshot()), hsiLayer, {0, 0});
    QCOMPARE(pixel(hsiBytes, 10, 10), QByteArray::fromHex("ed043bff"));
    QCOMPARE(pixel(hsiBytes, 20, 20), QByteArray::fromHex("808080ff"));

    // The legacy conversions apply to HSL and HSI as well.
    OperationDocument legacyHslDocument(input);
    const auto legacyHslLayer = legacyHslDocument.activeLayer();
    QCOMPARE(legacyHslDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyHslDocument.hsvAdjustment(legacyHslLayer,
                                             {0, 50, 0, HsvAdjustmentType::Hsl, true}), Result::Success);
    const auto legacyHslBytes = readTile(render(renderer, legacyHslDocument.snapshot()),
                                         legacyHslLayer, {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("hsv-legacy-hsl.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(legacyHslBytes), qint64(legacyHslBytes.size()));
    }
    // The actual product hsv_adjustment transformation gives (243, 0, 48)
    // on both ARM64 and x86_64 for these settings and stored input bytes.
    QCOMPARE(pixel(legacyHslBytes, 10, 10), QByteArray::fromHex("f30030ff"));

    OperationDocument legacyHsiDocument(input);
    const auto legacyHsiLayer = legacyHsiDocument.activeLayer();
    QCOMPARE(legacyHsiDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyHsiDocument.hsvAdjustment(legacyHsiLayer,
                                             {0, 50, 0, HsvAdjustmentType::Hsi, true}), Result::Success);
    const auto legacyHsiBytes = readTile(render(renderer, legacyHsiDocument.snapshot()),
                                         legacyHsiLayer, {0, 0});
    QCOMPARE(pixel(legacyHsiBytes, 10, 10), QByteArray::fromHex("ff0035ff"));
}

void KisGpuTileRendererTest::hsvAdjustmentClipsCoverageAndSupportsLegacyConversion()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("0a141e28"));
    set(40, 40, QByteArray::fromHex("646464ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    // Alpha coverage interpolates between the source and the adjusted pixel.
    OperationDocument alphaDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.hsvAdjustment(alphaDocument.activeLayer(), {0, 0, 50, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()),
                                     alphaDocument.activeLayer(), {0, 0});
    // 10,20,30 adjusted to 133,138,143 at 128/255 coverage.
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("484f5728"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("858a8f28"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));

    // An inverted rectangle filters only the outside.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.hsvAdjustment(invertedDocument.activeLayer(),
                                            {0, 0, 50, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()),
                                   invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("88919944"));

    // Elliptical coverage intersects its bounding box before the pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.hsvAdjustment(ellipseDocument.activeLayer(),
                                           {0, 0, 50, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()),
                                  ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    // The unset pixel is transparent; its hidden RGB still follows the filter.
    QCOMPARE(pixel(ellipse, 128, 128), QByteArray::fromHex("80808000"));

    // The legacy conversion can color a neutral pixel when saturation rises;
    // the modern HSV rule cannot. This difference is frozen with the settings.
    OperationDocument modernDocument(input);
    QCOMPARE(modernDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(modernDocument.hsvAdjustment(modernDocument.activeLayer(),
                                          {0, 50, 0, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto modern = readTile(render(renderer, modernDocument.snapshot()),
                                 modernDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(modern, 40, 40), QByteArray::fromHex("646464ff"));

    OperationDocument legacyDocument(input);
    QCOMPARE(legacyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyDocument.hsvAdjustment(legacyDocument.activeLayer(),
                                          {0, 50, 0, HsvAdjustmentType::Hsv, true}), Result::Success);
    const auto legacyOperation = std::get<FilterSnapshot>(legacyDocument.snapshot()->layers.first().operations.last());
    QVERIFY(legacyOperation->compatibilityMode);
    const auto legacy = readTile(render(renderer, legacyDocument.snapshot()),
                                 legacyDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(legacy, 40, 40), QByteArray::fromHex("643233ff"));

    // The legacy value adjustment differs from the modern rule by more than
    // one byte on the same neutral input.
    OperationDocument legacyValue(input);
    QCOMPARE(legacyValue.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(legacyValue.hsvAdjustment(legacyValue.activeLayer(),
                                       {0, 0, 10, HsvAdjustmentType::Hsv, true}), Result::Success);
    const auto legacyValueBytes = readTile(render(renderer, legacyValue.snapshot()),
                                           legacyValue.activeLayer(), {0, 0});
    QCOMPARE(pixel(legacyValueBytes, 40, 40), QByteArray::fromHex("7e7e7eff"));
    OperationDocument modernValue(input);
    QCOMPARE(modernValue.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(modernValue.hsvAdjustment(modernValue.activeLayer(),
                                       {0, 0, 10, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto modernValueBytes = readTile(render(renderer, modernValue.snapshot()),
                                           modernValue.activeLayer(), {0, 0});
    QCOMPARE(pixel(modernValueBytes, 40, 40), QByteArray::fromHex("747474ff"));

    // Retained input preserves the legacy flag and the same pixels.

    QCOMPARE(legacyDocument.historyAction(legacyDocument.undoCount()), HistoryAction::HsvAdjustment);
    QVERIFY(std::get<FilterSnapshot>(legacyDocument.snapshot()->layers.first().operations.last())->compatibilityMode);
    QCOMPARE(readTile(render(renderer, legacyDocument.snapshot()), legacyDocument.activeLayer(), {0, 0}), legacy);
}

void KisGpuTileRendererTest::yuvResetPreservesStoredPixels()
{
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < TileRenderer::TileSize; ++y) {
        for (int x = 0; x < TileRenderer::TileSize; ++x) {
            const int offset = (y * TileRenderer::TileSize + x) * 4;
            bytes[offset] = char(x);
            bytes[offset + 1] = char(y);
            bytes[offset + 2] = char((73 * x + 151 * y) % 256);
            bytes[offset + 3] = char((x + y) % 256);
        }
    }
    RasterImage input;
    input.size = {TileRenderer::TileSize, TileRenderer::TileSize};
    RasterLayer source;
    source.name = QStringLiteral("YUV reset");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000054}"));
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    HsvAdjustment reset{0, 0, 0, HsvAdjustmentType::Yuv, false};
    reset.lumaFrozen = true;
    QCOMPARE(document.hsvAdjustment(layer, reset), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), layer, {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("yuv-reset.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    // RGBToYUV stores both neutral chroma components at 0.5. With zero
    // slider values, YUVToRGB restores every stored color and alpha byte.
    QCOMPARE(actual, bytes);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::HsvAdjustment);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), bytes);
}

void KisGpuTileRendererTest::yuvUsesProductPrecision_data()
{
    QTest::addColumn<int>("hue");
    QTest::addColumn<int>("saturation");
    QTest::addColumn<int>("value");
    QTest::addColumn<qreal>("lumaRed");
    QTest::addColumn<qreal>("lumaGreen");
    QTest::addColumn<qreal>("lumaBlue");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("productSha256");
    QFile fixture(QFINDTESTDATA("data/yuv-product-rgba8.json"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    const auto rows = QJsonDocument::fromJson(fixture.readAll()).object().value("rows").toArray();
    QVERIFY(!rows.isEmpty());
    for (const auto &value : rows) {
        const auto row = value.toObject();
        const auto luma = row.value("luma").toArray();
        const auto source = row.value("sourceRgba").toArray();
        const auto controls = row.value("controls").toArray();
        QCOMPARE(controls.size(), 3);
        QCOMPARE(luma.size(), 3);
        QCOMPARE(source.size(), 4);
        QByteArray pixel;
        for (const auto &channel : source) pixel.append(char(channel.toInt()));
        QTest::newRow(qPrintable(row.value("name").toString()))
            << controls[0].toInt() << controls[1].toInt() << controls[2].toInt()
            << qreal(luma[0].toDouble()) << qreal(luma[1].toDouble()) << qreal(luma[2].toDouble())
            << pixel << row.value("rgbaSha256").toString().toLatin1();
    }
}

void KisGpuTileRendererTest::yuvUsesProductPrecision()
{
    QFETCH(int, hue);
    QFETCH(int, saturation);
    QFETCH(int, value);
    QFETCH(qreal, lumaRed);
    QFETCH(qreal, lumaGreen);
    QFETCH(qreal, lumaBlue);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, productSha256);
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < TileRenderer::TileSize; ++y) {
        for (int x = 0; x < TileRenderer::TileSize; ++x) {
            const int offset = (y * TileRenderer::TileSize + x) * 4;
            bytes[offset] = char(x);
            bytes[offset + 1] = char(y);
            bytes[offset + 2] = char((73 * x + 151 * y) % 256);
            bytes[offset + 3] = char((x + y) % 256);
        }
    }
    bytes.replace(0, 4, sourcePixel);
    auto hiddenPixel = sourcePixel;
    hiddenPixel[3] = '\0';
    bytes.replace(4, 4, hiddenPixel);
    RasterImage input;
    input.size = {TileRenderer::TileSize, TileRenderer::TileSize};
    RasterLayer source;
    source.name = QStringLiteral("YUV precision");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000057}"));
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    HsvAdjustment settings{hue, saturation, value, HsvAdjustmentType::Yuv, false};
    QVERIFY(freezeLumaCoefficients({lumaRed, lumaGreen, lumaBlue}, &settings.luma));
    settings.lumaFrozen = true;
    QCOMPARE(document.hsvAdjustment(document.activeLayer(), settings), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        const auto name = QStringLiteral("yuv-product-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
        QFile file(QDir(directory).filePath(name + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    // The fixed hash comes from the complete product CPU transform, with
    // separately rounded binary64 operations, rather than a second algorithm
    // in this test. Include every RGB and alpha byte in the comparison.
    QCOMPARE(QCryptographicHash::hash(actual, QCryptographicHash::Sha256).toHex(), productSha256);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
}

void KisGpuTileRendererTest::colorizeUsesProductLumaRounding_data()
{
    QTest::addColumn<qreal>("lumaRed");
    QTest::addColumn<qreal>("lumaGreen");
    QTest::addColumn<qreal>("lumaBlue");
    QTest::newRow("rec709") << qreal(0.2126f) << qreal(0.7152f) << qreal(0.0722f);
    QTest::newRow("quarters") << qreal(0.25f) << qreal(0.5f) << qreal(0.25f);
    QTest::newRow("small-coefficients") << qreal(1e-30f) << qreal(0.9f) << qreal(1e-38f);
    QTest::newRow("original-rec709") << qreal(0.2126) << qreal(0.7152) << qreal(0.0722);
    QTest::newRow("rgba-profile") << qreal(0.2224884) << qreal(0.71690369) << qreal(0.06060791);
}

void KisGpuTileRendererTest::colorizeUsesProductLumaRounding()
{
    QFETCH(qreal, lumaRed);
    QFETCH(qreal, lumaGreen);
    QFETCH(qreal, lumaBlue);
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < TileRenderer::TileSize; ++y) {
        for (int x = 0; x < TileRenderer::TileSize; ++x) {
            const int offset = (y * TileRenderer::TileSize + x) * 4;
            bytes[offset] = char(x);
            bytes[offset + 1] = char(y);
            bytes[offset + 2] = char((73 * x + 151 * y) % 256);
            bytes[offset + 3] = char((x + y) % 256);
        }
    }
    const int opaqueOffset = (43 * TileRenderer::TileSize + 2) * 4;
    const int hiddenOffset = opaqueOffset + 4;
    bytes.replace(opaqueOffset, 4, QByteArray::fromHex("022b2eff"));
    bytes.replace(hiddenOffset, 4, QByteArray::fromHex("022b2e00"));
    RasterImage input;
    input.size = {TileRenderer::TileSize, TileRenderer::TileSize};
    RasterLayer source;
    source.name = QStringLiteral("Colorize rounding");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000055}"));
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    OperationDocument document(input);
    HsvAdjustment colorize{0, 0, 0, HsvAdjustmentType::Hsv, false};
    colorize.colorize = true;
    colorize.luma = {lumaRed, lumaGreen, lumaBlue};
    colorize.lumaFrozen = true;
    QByteArray expected = bytes;
    for (int offset = 0; offset < bytes.size(); offset += 4) {
        const float red = quint8(bytes.at(offset)) / 255.0f;
        const float green = quint8(bytes.at(offset + 1)) / 255.0f;
        const float blue = quint8(bytes.at(offset + 2)) / 255.0f;
        const float lightness = red * qreal(lumaRed) + green * qreal(lumaGreen) + blue * qreal(lumaBlue);
        const char value = char(int(qBound(0.0f, lightness * 255.0f, 255.0f) + 0.5f));
        expected[offset] = value;
        expected[offset + 1] = value;
        expected[offset + 2] = value;
    }
    QCOMPARE(document.hsvAdjustment(document.activeLayer(), colorize), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("colorize-rounding-%1.rgba")
                                               .arg(QString::fromLatin1(QTest::currentDataTag()))));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    // This scalar reference matches the product's hsv_adjustment transform
    // across the full tile. Rec.709 maps the boundary pixel to 35, including
    // when the coefficients are first frozen to binary32.
    QCOMPARE(actual.mid(opaqueOffset, 4), expected.mid(opaqueOffset, 4));
    QCOMPARE(actual.mid(hiddenOffset, 4), expected.mid(hiddenOffset, 4));
    QCOMPARE(actual, expected);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
}

void KisGpuTileRendererTest::colorizePreservesDocumentLuma_data()
{
    QTest::addColumn<qreal>("lumaRed");
    QTest::addColumn<qreal>("lumaGreen");
    QTest::addColumn<qreal>("lumaBlue");
    QTest::addColumn<QByteArray>("sourcePixel");
    QTest::addColumn<QByteArray>("expectedPixel");
    // KoDummyColorProfile's colorants, normalised by KoColorSpace, are the
    // unmanaged RGBA8 document coefficients. The product hsv_adjustment
    // transform gives 173; first rounding these coefficients to float gives 172.
    QTest::newRow("rgba-profile") << qreal(0.2224884) << qreal(0.71690369) << qreal(0.06060791)
        << QByteArray::fromHex("80b9bcff") << QByteArray::fromHex("adadadff");
    // The product substitutes its qreal Rec.709 constants for a non-positive
    // coefficient. Its result is 33, while float-rounded constants give 32.
    QTest::newRow("rec709-fallback") << qreal(-1.0) << qreal(0.7152) << qreal(0.0722)
        << QByteArray::fromHex("630477ff") << QByteArray::fromHex("212121ff");
}

void KisGpuTileRendererTest::colorizePreservesDocumentLuma()
{
    QFETCH(qreal, lumaRed);
    QFETCH(qreal, lumaGreen);
    QFETCH(qreal, lumaBlue);
    QFETCH(QByteArray, sourcePixel);
    QFETCH(QByteArray, expectedPixel);
    RasterImage input;
    input.size = {8, 8};
    RasterLayer source;
    source.name = QStringLiteral("Document luma");
    source.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-000000000056}"));
    for (int i = 0; i < 4; ++i) source.defaultPixel[i] = quint8(sourcePixel[i]);
    input.layers.append(source);
    OperationDocument document(input);
    HsvAdjustment settings{0, 0, 0, HsvAdjustmentType::Hsv, false};
    settings.colorize = true;
    QVERIFY(freezeLumaCoefficients({lumaRed, lumaGreen, lumaBlue}, &settings.luma));
    settings.lumaFrozen = true;
    QCOMPARE(document.hsvAdjustment(document.activeLayer(), settings), Result::Success);
    TileRenderer renderer(m_device);
    const auto actual = readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0});

    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        const auto name = QStringLiteral("colorize-original-%1").arg(QString::fromLatin1(QTest::currentDataTag()));
        QFile file(QDir(directory).filePath(name + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(actual), qint64(actual.size()));
    }
    QCOMPARE(actual.left(4), expectedPixel);
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = project(renderer, render(renderer, document.snapshot()), QRect(0, 0, 8, 8));
    QVERIFY(undone);
    QVERIFY(!undone->tiles().isEmpty());
    QCOMPARE(readBuffer(undone->tiles().first().pixels).left(4), sourcePixel);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), actual);
}

void KisGpuTileRendererTest::hsvAdjustmentUsesLumaDependentProductRulesAndFrozenCoefficients()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("c81e46ff"));  // 200,30,70,255
    set(20, 20, QByteArray::fromHex("808080ff"));  // neutral gray
    set(30, 30, QByteArray::fromHex("000000ff"));  // black
    set(40, 40, QByteArray::fromHex("646464ff"));  // neutral gray 100
    set(50, 50, QByteArray::fromHex("0a141e28"));  // 10,20,30, alpha 40
    set(200, 10, QByteArray::fromHex("11223344")); // outside the selection
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Luma");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);

    const std::array<double, 3> srgb{0.2126f, 0.7152f, 0.0722f};
    const auto lumaSettings = [](qint32 hue, qint32 saturation, qint32 value,
                                 HsvAdjustmentType type, bool legacy, bool colorize,
                                 const std::array<double, 3> &luma) {
        HsvAdjustment settings{hue, saturation, value, type, legacy};
        settings.colorize = colorize;
        settings.luma = luma;
        settings.lumaFrozen = true;
        return settings;
    };

    // HSY (type 3) uses the product HCY policy: the luma coefficients select
    // the value channel and the requested hue and chroma are shifted back to
    // the requested luma. A neutral pixel ignores hue and saturation.
    OperationDocument hsyDocument(input);
    const auto hsyLayer = hsyDocument.activeLayer();
    QCOMPARE(hsyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto hsy = lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, false, false, srgb);
    QCOMPARE(hsyDocument.hsvAdjustment(hsyLayer, hsy), Result::Success);
    const auto hsyOperation =
        std::get<FilterSnapshot>(hsyDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(hsyOperation->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(!hsyOperation->colorize);
    QVERIFY(hsyOperation->luma == srgb);
    const auto hsyBytes = readTile(render(renderer, hsyDocument.snapshot()), hsyLayer, {0, 0});
    QCOMPARE(pixel(hsyBytes, 10, 10), QByteArray::fromHex("ff0a46ff"));
    QCOMPARE(pixel(hsyBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(hsyBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(hsyBytes, 40, 40), QByteArray::fromHex("646464ff"));
    QCOMPARE(pixel(hsyBytes, 50, 50), QByteArray::fromHex("01152928"));
    QCOMPARE(pixel(hsyBytes, 200, 10), QByteArray::fromHex("11223344"));

    // The legacy HSY conversion is the product's qreal HCY conversion and
    // produces a different fractional byte on the same input.
    OperationDocument legacyHsyDocument(input);
    const auto legacyHsyLayer = legacyHsyDocument.activeLayer();
    QCOMPARE(legacyHsyDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto legacyHsy =
        lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, true, false, srgb);
    QCOMPARE(legacyHsyDocument.hsvAdjustment(legacyHsyLayer, legacyHsy), Result::Success);
    const auto legacyHsyBytes =
        readTile(render(renderer, legacyHsyDocument.snapshot()), legacyHsyLayer, {0, 0});
    QCOMPARE(pixel(legacyHsyBytes, 10, 10), QByteArray::fromHex("ff0a46ff"));
    QCOMPARE(pixel(legacyHsyBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(legacyHsyBytes, 50, 50), QByteArray::fromHex("06152428"));
    QCOMPARE(pixel(legacyHsyBytes, 200, 10), QByteArray::fromHex("11223344"));

    // YUV (type 4) scales the blue chroma by the hue slider, the red chroma by
    // the saturation slider and offsets luma. It has no legacy variant.
    // References use RGBToYUV/YUVToRGB with the product's slider scaling.
    OperationDocument yuvDocument(input);
    const auto yuvLayer = yuvDocument.activeLayer();
    QCOMPARE(yuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto yuv = lumaSettings(30, 20, -10, HsvAdjustmentType::Yuv, false, false, srgb);
    QCOMPARE(yuvDocument.hsvAdjustment(yuvLayer, yuv), Result::Success);
    const auto yuvOperation =
        std::get<FilterSnapshot>(yuvDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(yuvOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(!yuvOperation->colorize);
    QVERIFY(yuvOperation->luma == srgb);
    const auto yuvBytes = readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0});
    QCOMPARE(pixel(yuvBytes, 10, 10), QByteArray::fromHex("f10074ff"));
    QCOMPARE(pixel(yuvBytes, 20, 20), QByteArray::fromHex("8f53adff"));
    QCOMPARE(pixel(yuvBytes, 30, 30), QByteArray::fromHex("0f002dff"));
    QCOMPARE(pixel(yuvBytes, 40, 40), QByteArray::fromHex("733791ff"));
    QCOMPARE(pixel(yuvBytes, 50, 50), QByteArray::fromHex("17004f28"));
    QCOMPARE(pixel(yuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Colorize uses the adjusted document luma as HSL lightness. The mode key
    // remains observable only through its slider range.
    OperationDocument colorizeDocument(input);
    const auto colorizeLayer = colorizeDocument.activeLayer();
    QCOMPARE(colorizeDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto colorize =
        lumaSettings(120, 80, 0, HsvAdjustmentType::Hsv, false, true, srgb);
    QCOMPARE(colorizeDocument.hsvAdjustment(colorizeLayer, colorize), Result::Success);
    const auto colorizeOperation =
        std::get<FilterSnapshot>(colorizeDocument.snapshot()->layers.first().operations.last());
    QVERIFY(colorizeOperation->colorize);
    QVERIFY(colorizeOperation->luma == srgb);
    const auto colorizeBytes =
        readTile(render(renderer, colorizeDocument.snapshot()), colorizeLayer, {0, 0});
    QCOMPARE(pixel(colorizeBytes, 10, 10), QByteArray::fromHex("0e7c0eff"));
    QCOMPARE(pixel(colorizeBytes, 20, 20), QByteArray::fromHex("1ae61aff"));
    QCOMPARE(pixel(colorizeBytes, 30, 30), QByteArray::fromHex("000000ff"));
    QCOMPARE(pixel(colorizeBytes, 40, 40), QByteArray::fromHex("14b414ff"));
    QCOMPARE(pixel(colorizeBytes, 50, 50), QByteArray::fromHex("04210428"));
    QCOMPARE(pixel(colorizeBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Colorize with the YUV mode keeps the product's 0..100 Yellow-Blue
    // scaling. Its result differs from the type 0 Colorize rule.
    OperationDocument colorizeYuvDocument(input);
    const auto colorizeYuvLayer = colorizeYuvDocument.activeLayer();
    QCOMPARE(colorizeYuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto colorizeYuv =
        lumaSettings(50, 80, 20, HsvAdjustmentType::Yuv, true, true, srgb);
    QCOMPARE(colorizeYuvDocument.hsvAdjustment(colorizeYuvLayer, colorizeYuv), Result::Success);
    const auto colorizeYuvOperation =
        std::get<FilterSnapshot>(colorizeYuvDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(colorizeYuvOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(colorizeYuvOperation->colorize);
    const auto colorizeYuvBytes =
        readTile(render(renderer, colorizeYuvDocument.snapshot()), colorizeYuvLayer, {0, 0});
    QCOMPARE(pixel(colorizeYuvBytes, 10, 10), QByteArray::fromHex("15bfbfff"));
    QCOMPARE(pixel(colorizeYuvBytes, 20, 20), QByteArray::fromHex("48ebebff"));
    QCOMPARE(pixel(colorizeYuvBytes, 50, 50), QByteArray::fromHex("0d777728"));
    QCOMPARE(pixel(colorizeYuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    // A different frozen coefficient set changes the result even though the
    // operation no longer reads a color-space object. The original values
    // remain fixed at admission.
    const std::array<double, 3> custom{0.25f, 0.5f, 0.25f};
    const std::array<qreal, 3> supplied{0.25, 0.5, 0.25};
    std::array<double, 3> rounded{};
    QVERIFY(freezeLumaCoefficients(supplied, &rounded));
    QVERIFY(rounded == custom);
    OperationDocument customDocument(input);
    const auto customLayer = customDocument.activeLayer();
    QCOMPARE(customDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    const auto customHsy =
        lumaSettings(0, 50, 0, HsvAdjustmentType::Hsy, false, false, rounded);
    QCOMPARE(customDocument.hsvAdjustment(customLayer, customHsy), Result::Success);
    const auto customBytes =
        readTile(render(renderer, customDocument.snapshot()), customLayer, {0, 0});
    QCOMPARE(pixel(customBytes, 10, 10), QByteArray::fromHex("ff0440ff"));
    QCOMPARE(pixel(customBytes, 20, 20), QByteArray::fromHex("808080ff"));
    QCOMPARE(pixel(customBytes, 50, 50), QByteArray::fromHex("00142828"));
    QCOMPARE(pixel(customBytes, 200, 10), QByteArray::fromHex("11223344"));

    // YUV and Colorize consume the same frozen coefficient set, so changing it
    // changes their pixels as well.
    OperationDocument customYuvDocument(input);
    const auto customYuvLayer = customYuvDocument.activeLayer();
    QCOMPARE(customYuvDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(customYuvDocument.hsvAdjustment(
                 customYuvLayer,
                 lumaSettings(30, 20, -10, HsvAdjustmentType::Yuv, false, false, rounded)),
             Result::Success);
    const auto customYuvBytes =
        readTile(render(renderer, customYuvDocument.snapshot()), customYuvLayer, {0, 0});
    QCOMPARE(pixel(customYuvBytes, 10, 10), QByteArray::fromHex("ec0062ff"));
    QCOMPARE(pixel(customYuvBytes, 20, 20), QByteArray::fromHex("8d37a0ff"));
    QCOMPARE(pixel(customYuvBytes, 30, 30), QByteArray::fromHex("0d0020ff"));
    QCOMPARE(pixel(customYuvBytes, 50, 50), QByteArray::fromHex("15004128"));
    QCOMPARE(pixel(customYuvBytes, 200, 10), QByteArray::fromHex("11223344"));

    OperationDocument customColorizeDocument(input);
    const auto customColorizeLayer = customColorizeDocument.activeLayer();
    QCOMPARE(customColorizeDocument.setSelectionRect(QRect(0, 0, 100, 100)), Result::Success);
    QCOMPARE(customColorizeDocument.hsvAdjustment(
                 customColorizeLayer,
                 lumaSettings(120, 80, 0, HsvAdjustmentType::Hsv, false, true, rounded)),
             Result::Success);
    const auto customColorizeBytes =
        readTile(render(renderer, customColorizeDocument.snapshot()), customColorizeLayer, {0, 0});
    QCOMPARE(pixel(customColorizeBytes, 10, 10), QByteArray::fromHex("119511ff"));
    QCOMPARE(pixel(customColorizeBytes, 20, 20), QByteArray::fromHex("1ae61aff"));
    QCOMPARE(pixel(customColorizeBytes, 50, 50), QByteArray::fromHex("04240428"));
    QCOMPARE(pixel(customColorizeBytes, 200, 10), QByteArray::fromHex("11223344"));

    // Undo restores the source bytes and redo restores the filtered bytes.
    QCOMPARE(yuvDocument.undo(), Result::Success);
    QCOMPARE(yuvDocument.undo(), Result::Success);
    QCOMPARE(readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0}), bytes);
    QCOMPARE(yuvDocument.redo(), Result::Success);
    QCOMPARE(yuvDocument.redo(), Result::Success);
    QCOMPARE(readTile(render(renderer, yuvDocument.snapshot()), yuvLayer, {0, 0}), yuvBytes);

    // Retained input preserves Colorize, the custom coefficients and the same pixels.

    const auto retainedOperation =
        std::get<FilterSnapshot>(customDocument.snapshot()->layers.first().operations.last());
    QCOMPARE(retainedOperation->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(retainedOperation->luma == custom);

    const auto retainedColorizeOperation =
        std::get<FilterSnapshot>(colorizeYuvDocument.snapshot()->layers.first().operations.last());
    QVERIFY(retainedColorizeOperation->colorize);
    QCOMPARE(retainedColorizeOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(retainedColorizeOperation->luma == srgb);
}

void KisGpuTileRendererTest::hsvAdjustmentLumaModesClipAlphaAndSelectionCoverage()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    QByteArray bytes(TileRenderer::TileBytes, '\0');
    const auto set = [&](int x, int y, const QByteArray &rgba) {
        std::memcpy(bytes.data() + (y * TileRenderer::TileSize + x) * 4, rgba.constData(), 4);
    };
    set(10, 10, QByteArray::fromHex("0a141e28"));
    set(20, 20, QByteArray::fromHex("0a141e28"));
    set(30, 30, QByteArray::fromHex("0a141e28"));
    set(40, 40, QByteArray::fromHex("646464ff"));
    set(100, 100, QByteArray::fromHex("11223344"));
    RasterImage input;
    input.size = {256, 256};
    RasterLayer source;
    source.name = QStringLiteral("Luma coverage");
    source.uuid = QUuid::createUuid();
    source.tiles.append({{0, 0}, bytes});
    input.layers.append(source);
    TileRenderer renderer(m_device);
    const std::array<double, 3> luma{0.2126f, 0.7152f, 0.0722f};
    const auto settings = [&](qint32 hue, qint32 saturation, qint32 value,
                              HsvAdjustmentType type, bool colorize) {
        HsvAdjustment result{hue, saturation, value, type, false};
        result.colorize = colorize;
        result.luma = luma;
        result.lumaFrozen = true;
        return result;
    };

    // A feathered alpha selection interpolates the YUV result; alpha itself is
    // carried through unchanged.
    OperationDocument alphaDocument(input);
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    coverage[20 * 256 + 20] = char(128);
    coverage[30 * 256 + 30] = char(255);
    const auto alpha = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(alpha);
    QCOMPARE(alphaDocument.setSelectionMask(alpha), Result::Success);
    QCOMPARE(alphaDocument.hsvAdjustment(alphaDocument.activeLayer(),
                                        settings(0, 0, 50, HsvAdjustmentType::Yuv, false)),
             Result::Success);
    const auto alphaBytes = readTile(render(renderer, alphaDocument.snapshot()),
                                     alphaDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(alphaBytes, 20, 20), QByteArray::fromHex("4a545e28"));
    QCOMPARE(pixel(alphaBytes, 30, 30), QByteArray::fromHex("8a949e28"));
    QCOMPARE(pixel(alphaBytes, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(alphaBytes, 40, 40), QByteArray::fromHex("646464ff"));

    // Colorize also filters only the complement of an inverted rectangle.
    OperationDocument invertedDocument(input);
    QCOMPARE(invertedDocument.setSelectionRect(QRect(0, 0, 64, 64), true), Result::Success);
    QCOMPARE(invertedDocument.hsvAdjustment(invertedDocument.activeLayer(),
                                           settings(120, 80, 0, HsvAdjustmentType::Hsv, true)),
             Result::Success);
    const auto inverted = readTile(render(renderer, invertedDocument.snapshot()),
                                   invertedDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(inverted, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(inverted, 100, 100), QByteArray::fromHex("06390644"));

    // YUV follows the elliptical coverage pixel test.
    OperationDocument ellipseDocument(input);
    QCOMPARE(ellipseDocument.setSelectionEllipse(QRect(64, 64, 128, 128)), Result::Success);
    QCOMPARE(ellipseDocument.hsvAdjustment(ellipseDocument.activeLayer(),
                                          settings(0, 0, 50, HsvAdjustmentType::Yuv, false)),
             Result::Success);
    const auto ellipse = readTile(render(renderer, ellipseDocument.snapshot()),
                                  ellipseDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(ellipse, 10, 10), QByteArray::fromHex("0a141e28"));
    QCOMPARE(pixel(ellipse, 100, 100), QByteArray::fromHex("91a2b344"));

    // The operation owns the coefficients used by the renderer.
    const auto retainedOperation =
        std::get<FilterSnapshot>(ellipseDocument.snapshot()->layers.first().operations.last());
    QVERIFY(retainedOperation->luma == luma);
    QCOMPARE(retainedOperation->hsvType, HsvAdjustmentType::Yuv);
}

void KisGpuTileRendererTest::featheredSelectionClipsPaintFillEraseHistory()
{
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const QRect clip(QPoint(), QSize(256, 256));
    QByteArray coverage(256 * 256, '\0');
    for (int y = 64; y < 192; ++y)
        for (int x = 64; x < 192; ++x) coverage[y * 256 + x] = char(255);
    const auto hard = SelectionMask::fromCoverage(QRect(QPoint(), QSize(256, 256)), coverage, clip);
    QVERIFY(hard);
    const auto soft = hard->feathered(8, clip);
    QVERIFY(soft);
    QVERIFY(soft->isAlpha());
    const quint8 edge = soft->coverage(64, 128);
    QVERIFY(edge > 0 && edge < 255);
    QCOMPARE(soft->coverage(128, 128), 255);

    OperationDocument document({256, 256});
    Brush brush;
    brush.rgba = {210, 40, 90, 255};
    brush.diameter = 32;
    QCOMPARE(document.setSelectionMask(hard), Result::Success);
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{48, 128}, 1}, {{208, 128}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QVERIFY(pending->selectionMask == soft);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    TileRenderer renderer(m_device);
    const auto painted = render(renderer, document.snapshot());
    QVERIFY(painted);
    const auto bytes = readTile(painted, document.activeLayer(), {0, 0});
    QCOMPARE(pixel(bytes, 128, 128), QByteArray::fromHex("d2285aff"));
    QCOMPARE(quint8(pixel(bytes, 64, 128).at(3)), edge);
    QCOMPARE(pixel(bytes, 40, 128), QByteArray(4, '\0'));

    // Fill applies the fractional selection as its source alpha.
    TileRenderer fillRenderer(m_device);
    OperationDocument fillDocument({256, 256});
    QCOMPARE(fillDocument.setSelectionMask(soft), Result::Success);
    QCOMPARE(fillDocument.fillLayer(fillDocument.activeLayer(), {17, 34, 51}), Result::Success);
    const auto filled = readTile(render(fillRenderer, fillDocument.snapshot()), fillDocument.activeLayer(), {0, 0});
    QCOMPARE(pixel(filled, 128, 128), QByteArray::fromHex("112233ff"));
    QCOMPARE(quint8(pixel(filled, 64, 128).at(3)), edge);
    QCOMPARE(pixel(filled, 40, 128), QByteArray(4, '\0'));

    // Erase removes exactly the fractional share of the existing alpha.
    OperationDocument eraseDocument({256, 256});
    QCOMPARE(eraseDocument.fillLayer(eraseDocument.activeLayer(), {20, 80, 160}), Result::Success);
    QCOMPARE(eraseDocument.setSelectionMask(soft), Result::Success);
    Brush eraser = brush;
    eraser.erase = true;
    const auto erase = eraseDocument.beginStroke(eraseDocument.activeLayer(), eraser, 0);
    QCOMPARE(eraseDocument.appendSamples(erase, {{{48, 128}, 1}, {{208, 128}, 1}}), Result::Success);
    QCOMPARE(eraseDocument.commitStroke(erase), Result::Success);
    const auto erased = readTile(render(renderer, eraseDocument.snapshot()), eraseDocument.activeLayer(), {0, 0});
    QCOMPARE(quint8(pixel(erased, 128, 128).at(3)), 0);
    QCOMPARE(quint8(pixel(erased, 64, 128).at(3)), quint8(255 - edge));
    QCOMPARE(quint8(pixel(erased, 40, 128).at(3)), 255);

    // The feather is one history entry; undo restores the hard coverage and redo
    // restores the same fractional pixels.
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == soft);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == hard);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionMask()->isAlpha());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);

    QVERIFY(document.selectionMask()->isAlpha());
    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), bytes);
}

void KisGpuTileRendererTest::layerTransparencyMaskMultipliesCompositionAlpha()
{
    const QRect clip(QPoint(), QSize(64, 64));
    const auto pixel = [](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * TileRenderer::TileSize + x) * 4, 4);
    };
    const auto projectedBytes = [&](const ProjectedSnapshot &projected) {
        for (const auto &tile : projected->tiles())
            if (tile.coordinate == QPoint(0, 0)) return readBuffer(tile.pixels);
        return QByteArray();
    };
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayer(layer, {200, 30, 70}), Result::Success);
    QCOMPARE(document.setSelectionRect(QRect(16, 16, 16, 16)), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    TileRenderer renderer(m_device);
    auto rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    // The paint layer retains its own pixels; composition applies the mask.
    const auto layerBytes = readTile(rendered, layer, {0, 0});
    QCOMPARE(pixel(layerBytes, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(layerBytes, 24, 24), QByteArray::fromHex("c81e46ff"));
    auto projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    auto composed = projectedBytes(projected);
    QCOMPARE(composed.size(), TileRenderer::TileBytes);
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 40, 40), QByteArray(4, '\0'));

    // Hiding the mask disables its composition effect without deleting it.
    QCOMPARE(document.setTransparencyMaskVisible(layer, false), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.setTransparencyMaskVisible(layer, true), Result::Success);

    // Removal restores full coverage; undo reinstates the retained mask.
    QCOMPARE(document.removeTransparencyMask(layer), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.undo(), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 24, 24), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(document.undo(), Result::Success); // hide
    QCOMPARE(document.undo(), Result::Success); // show
    QCOMPARE(document.undo(), Result::Success); // remove mask addition
    QVERIFY(!document.layerTransparencyMask(layer));

    // Fractional selection coverage becomes fractional composition alpha and
    // masks pixels outside its bounds completely.
    QByteArray coverage(64 * 64, '\0');
    for (int y = 32; y < 48; ++y)
        for (int x = 32; x < 48; ++x) coverage[y * 64 + x] = char(128);
    const auto soft = SelectionMask::fromAlphaCoverage(QRect(QPoint(), QSize(64, 64)), coverage, clip);
    QVERIFY(soft && soft->isAlpha());
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    rendered = render(renderer, document.snapshot());
    QVERIFY(rendered);
    projected = project(renderer, rendered, {0, 0, 64, 64});
    QVERIFY(projected);
    composed = projectedBytes(projected);
    QCOMPARE(pixel(composed, 40, 40), QByteArray::fromHex("c81e4680"));
    QCOMPARE(pixel(composed, 8, 8), QByteArray(4, '\0'));
    QCOMPARE(pixel(composed, 56, 56), QByteArray(4, '\0'));
}

void KisGpuTileRendererTest::inputPartitionsAndPreviewProduceTheSamePixels()
{
    QList<InputSample> samples;
    for (int i = 0; i < 37; ++i) samples.append({{20.125 + i * 7.25, 70.5 + (i % 5) * 2}, qreal(i % 8 + 1) / 8});
    OperationDocument single({512, 512}), chunks({512, 512});
    Brush brush;
    brush.rgba = {42, 88, 203, 139};
    brush.diameter = 29.25;
    brush.opacity = 177.0 / 255;
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
    QCOMPARE(document.appendSamples(third, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
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
    brush.opacity = 128.0 / 255;
    stroke = document.beginStroke(document.activeLayer(), brush, 0);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto erased = render(renderer, document.snapshot(), painted);
    QVERIFY(erased);
    QCOMPARE(readTile(erased, document.activeLayer(), {0, 0}).mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("ff00007f"));
    brush.opacity = 255.0 / 255;
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

    const auto restored = render(renderer, document.snapshot());
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

void KisGpuTileRendererTest::shapesUndoRestoreTheSamePixels()
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

    QCOMPARE(readTile(render(renderer, document.snapshot()), document.activeLayer(), {0, 0}), pixels);
    const auto cancelled = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(cancelled, {{{128, 128}, 1}}), Result::Success);
    QVERIFY(render(renderer, document.preview(), ellipse));
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(readTile(render(renderer, document.snapshot(), ellipse), document.activeLayer(), {0, 0}), pixels);
}

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistory_data()
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

void KisGpuTileRendererTest::rectangleOutlinesPreserveRotationHistory()
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

    QVERIFY(document.hasActiveStroke());
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->shape.cornerRadii, shape.cornerRadii);
    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), pixels);
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

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistory_data()
{
    QTest::addColumn<bool>("closed");
    QTest::newRow("open") << false;
    QTest::newRow("closed") << true;
}

void KisGpuTileRendererTest::polylinesPreserveVerticesClosureHistory()
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

    QVERIFY(document.hasActiveStroke());
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->inputSamples(), vertices);
    QCOMPARE(readTile(render(renderer, document.preview()), document.activeLayer(), {0, 0}), pixels);
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
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    RenderedSnapshot retained;
    {
        TileRenderer owner(m_device);
        retained = render(owner, document.snapshot());
        QVERIFY(retained);
    }
    QCOMPARE(readTile(retained, document.activeLayer(), {0, 0}).size(), TileRenderer::TileBytes);
    TileRenderer replacement(m_device);
    WGPUCommandEncoder commands = nullptr;
    commands = m_device->beginCommands();
    QVERIFY(commands);
    QCOMPARE(replacement.record(commands, document.snapshot(), retained).code, RenderOutcome::InvalidVersion);
    QVERIFY2(m_device->submitCommands(true), qPrintable(m_device->diagnostic()));
    QVERIFY(render(replacement, document.snapshot()));
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
    brush.rgba = {0, 0, 255, 255};
    brush.opacity = 128.0 / 255;
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
    const QList<RgbaPixel> colors{{255, 0, 0, 255}, {0, 0, 255, 128}, {0, 255, 0, 64}, {255, 255, 255, 128}};
    LayerId green = 0;
    for (qsizetype i = 0; i < colors.size(); ++i) {
        const auto layer = i == 0 ? document.activeLayer() : document.addLayer(QString::number(i));
        if (i == 2) green = layer;
        Brush brush;
        brush.rgba = colors[i];
        brush.opacity = qreal(colors[i][3]) / 255;
        brush.rgba[3] = 255;
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
    for (const auto &color : QList<RgbaPixel>{{255, 0, 0, 128}, {0, 0, 255, 128}}) {
        if (!document.snapshot()->layers.first().operations.isEmpty()) QVERIFY(document.addLayer(QStringLiteral("Top")));
        Brush brush;
        brush.rgba = color;
        brush.opacity = qreal(color[3]) / 255;
        brush.rgba[3] = 255;
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
    brush.rgba = {71, 139, 211, 255};
    brush.opacity = 128.0 / 255;
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

    TileRenderer bounded(m_device, 2 * TileRenderer::TileBytes);
    const auto rebuilt = render(bounded, document.snapshot());
    QVERIFY2(rebuilt, "A retained 250-operation document must rebuild with two writable tiles per coordinate");
    QCOMPARE(readTile(rebuilt, document.activeLayer(), {0, 0}), pixels);
    QCOMPARE(bounded.residentBytes(), TileRenderer::TileBytes);
}

void KisGpuTileRendererTest::longStrokeRebuildKeepsPixelsAndHistory()
{
    Brush brush;
    brush.diameter = 8;
    brush.spacing = 0.25;
    brush.rgba = {71, 139, 211, 255};
    QList<InputSample> samples{{{32, 32}, 1}};
    for (int i = 0; i < 2304; ++i)
        samples.append({{qreal(6 + i % 56), qreal(6 + (i * 7) % 56)}, 1});

    OperationDocument partitioned({64, 64});
    const auto referenceStroke = partitioned.beginStroke(partitioned.activeLayer(), brush, 97);
    TileRenderer reference(m_device);
    RenderedSnapshot incremental;
    for (qsizetype first = 0; first < samples.size(); first += 128) {
        QCOMPARE(partitioned.appendSamples(referenceStroke, samples.mid(first, 128)), Result::Success);
        incremental = render(reference, partitioned.preview(), incremental);
        QVERIFY(incremental);
    }
    QCOMPARE(partitioned.commitStroke(referenceStroke), Result::Success);
    incremental = render(reference, partitioned.snapshot(), incremental);
    QVERIFY(incremental);
    const auto expected = readTile(incremental, partitioned.activeLayer(), {0, 0});

    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    const auto stroke = document.beginStroke(layer, brush, 97);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.snapshot()->sampleCount, samples.size());
    TileRenderer bounded(m_device, 2 * TileRenderer::TileBytes);
    const auto original = render(bounded, document.snapshot());
    QVERIFY(original);
    const auto pixels = readTile(original, layer, {0, 0});
    QCOMPARE(pixels, expected);
    QCOMPARE(pixels.mid((32 * TileRenderer::TileSize + 32) * 4, 4), QByteArray::fromHex("478bd3ff"));
    QCOMPARE(pixels.right(4), QByteArray(4, char(0)));
    QCOMPARE(document.undo(), Result::Success);
    const auto undone = render(bounded, document.snapshot(), original);
    QVERIFY(undone);
    QVERIFY(undone->tiles(layer).isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    const auto redone = render(bounded, document.snapshot(), undone);
    QVERIFY(redone);
    QCOMPARE(readTile(redone, layer, {0, 0}), pixels);
    QCOMPARE(readTile(original, layer, {0, 0}), pixels);
    QVERIFY(bounded.residentBytes() <= 2 * TileRenderer::TileBytes);

    TileRenderer rebuilt(m_device, TileRenderer::TileBytes);
    const auto restored = render(rebuilt, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, document.activeLayer(), {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("long-stroke-rebuild.rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
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
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    {
        TileRenderer renderer(device);
        WGPUCommandEncoder commands = nullptr;
        commands = device->beginCommands();
        QVERIFY(commands);
        const auto result = renderer.record(commands, document.snapshot());
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

void KisGpuTileRendererTest::rasterCheckpointsPreservePixelsThroughPaintingUndo()
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

    TileRenderer rebuilt(m_device, 4 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, document.snapshot());
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

    QCOMPARE(document.redo(), Result::Success);
    TileRenderer rebuilt(m_device, 2 * TileRenderer::TileBytes);
    const auto restored = render(rebuilt, document.snapshot());
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
    QCOMPARE(document.appendSamples(id, {{{32, 32}, 1}, {{40, 32}, 1}}), Result::Success);
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
    style.fill = RgbaPixel{221, 17, 44, 255};
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
    style.fill = RgbaPixel{221, 17, 44, 255};
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

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistory_data()
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

void KisGpuTileRendererTest::solidFiguresPreserveFillOutlineHistory()
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
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.updateFigure(id, shape, style), Result::Success);
    QCOMPARE(document.commitStroke(id), Result::Success);
    TileRenderer renderer(m_device);
    const auto version = render(renderer, document.snapshot());
    QVERIFY(version);
    const auto pixels = readTile(version, layer, {0, 0});
    QCOMPARE(pixels.size(), TileRenderer::TileBytes);
    const auto pixel = [&](int x, int y) { return pixels.mid((y * TileRenderer::TileSize + x) * 4, 4); };
    QCOMPARE(pixel(32, 24), QByteArray::fromHex("dd112cff"));
    const auto topPixel = shapeKind == ShapeKind::Ellipse ?
        (outline ? QByteArray::fromHex("1181f1ff") : QByteArray::fromHex("dd112cfa")) : QByteArray::fromHex("dd112cff");
    QCOMPARE(pixel(32, 8), topPixel);
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

    const auto restored = render(renderer, document.snapshot());
    QVERIFY(restored);
    QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
}

void KisGpuTileRendererTest::figureFillMatchesPainterCoverage_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto *kind : {"ellipse", "ellipse-48", "rounded-rectangle", "triangle", "reverse-triangle",
                             "small-curve", "clipped-curve", "odd-even", "winding", "mirrored", "mirrored-half-grid", "mirrored-collapsed",
                             "large-coordinates", "large-vertical", "distant-mirror"})
        QTest::newRow(kind) << QString::fromLatin1(kind);
}

void KisGpuTileRendererTest::figureFillMatchesPainterCoverage()
{
    QFETCH(QString, kind);
    const bool distantMirror = kind == QLatin1String("distant-mirror");
    const QPoint origin(distantMirror ? int(OperationDocument::CoordinateLimit) - 256 : 0, 0);
    QPainterPath path;
    const bool mirrored = kind.startsWith(QLatin1String("mirrored"));
    if (kind == QLatin1String("mirrored-collapsed")) path.addRect(QRectF(16 + 1. / 128, 16, 1. / 128, 16));
    else if (kind == QLatin1String("mirrored-half-grid")) path.addRect(QRectF(16 + 1. / 128, 16 + 1. / 128, 32, 32));
    else if (kind == QLatin1String("ellipse") || mirrored) path.addEllipse(QRectF(16, 16, 32, 32));
    else if (kind == QLatin1String("ellipse-48")) path.addEllipse(QRectF(8, 8, 48, 48));
    else if (kind == QLatin1String("rounded-rectangle")) path.addRoundedRect(QRectF(16, 16, 32, 32), 14, 14);
    else if (kind == QLatin1String("small-curve")) {
        path.moveTo(16, 16);
        path.cubicTo(16.25, 16.0625, 16.5, 16.0625, 16.75, 16);
        path.lineTo(16.75, 17);
        path.lineTo(16, 17);
    } else if (kind == QLatin1String("clipped-curve")) {
        path.addEllipse(QRectF(-9.25, -7.125, 32.5, 40.75));
    } else if (kind == QLatin1String("odd-even") || kind == QLatin1String("winding")) {
        path.addEllipse(QRectF(8.125, 8.25, 48, 48));
        path.addEllipse(QRectF(16.375, 16.5, 32, 32));
        if (kind == QLatin1String("winding")) path.setFillRule(Qt::WindingFill);
    } else if (kind == QLatin1String("large-coordinates")) {
        const auto limit = OperationDocument::CoordinateLimit;
        path.moveTo(-limit, -limit);
        path.lineTo(limit, -limit);
        path.lineTo(limit, limit);
    } else if (kind == QLatin1String("large-vertical")) {
        const auto limit = OperationDocument::CoordinateLimit;
        path.addRect(QRectF(-limit / 2, -limit / 2, limit, limit));
    } else if (distantMirror) {
        path.addRect(QRectF(OperationDocument::CoordinateLimit - .5, 0, .5, 1));
    } else {
        path.moveTo(16, 16);
        path.lineTo(48, 16);
        path.lineTo(48, 48);
        if (kind == QLatin1String("reverse-triangle")) path = path.toReversed();
    }
    path.closeSubpath();
    Brush brush;
    if (mirrored) {
        brush.mirrorHorizontal = brush.mirrorVertical = true;
        brush.mirrorCenter = {32, 32};
    } else if (distantMirror) {
        brush.mirrorHorizontal = true;
        brush.mirrorCenter = {-int(OperationDocument::CoordinateLimit), 0};
    }
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.windingFill = path.fillRule() == Qt::WindingFill;
    QList<InputSample> samples;
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto point = path.elementAt(i);
        shape.pathCommands.append(point.isMoveTo() ? PathCommand::MoveTo :
                                  point.isLineTo() ? PathCommand::LineTo : PathCommand::CubicTo);
        samples.append({point, 1});
        if (point.isCurveTo()) {
            samples.append({path.elementAt(++i), 1});
            samples.append({path.elementAt(++i), 1});
        }
    }
    QImage mask(256, 256, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    {
        QPainter painter(&mask);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(-origin);
        painter.fillPath(path, Qt::white);
        if (mirrored) {
            for (const auto &scale : {QPointF(-1, 1), QPointF(1, -1), QPointF(-1, -1)}) {
                QTransform mirror;
                mirror.translate(32, 32).scale(scale.x(), scale.y()).translate(-32, -32);
                painter.fillPath(mirror.map(path), Qt::white);
            }
        }
    }
    QByteArray expected(TileRenderer::TileBytes, '\0');
    for (int y = 0; y < mask.height(); ++y) {
        for (int x = 0; x < mask.width(); ++x) {
            const int alpha = qAlpha(mask.pixel(x, y));
            if (!alpha) continue;
            const int offset = (y * 256 + x) * 4;
            expected[offset] = 17;
            expected[offset + 1] = char(129);
            expected[offset + 2] = char(241);
            expected[offset + 3] = char(alpha);
        }
    }
    OperationDocument document(QSize(origin.x() + 256, 256));
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{17, 129, 241, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), brush, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    const auto commands = m_device->beginCommands();
    QVERIFY(commands);
    auto finishFrame = qScopeGuard([&] { m_device->submitCommands(true); });
    const auto rendered = renderer.recordRegion(commands, document.snapshot(), QRect(origin, QSize(256, 256)), {document.activeLayer()});
    QCOMPARE(rendered.code, RenderOutcome::Recorded);
    QVERIFY(m_device->submitCommands(true));
    finishFrame.dismiss();
    const auto actual = readTile(rendered.version, document.activeLayer(), {origin.x() / 256, 0});
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {qMakePair(QStringLiteral("actual"), actual), qMakePair(QStringLiteral("expected"), expected)}) {
            QFile file(QDir(directory).filePath(QStringLiteral("figure-fill-%1-%2.rgba").arg(kind, entry.first)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
    QCOMPARE(actual, expected);
}

void KisGpuTileRendererTest::emptyFillKeepsLineOutline()
{
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo};
    const QList<InputSample> samples{{{16, 16}, 1}, {{32, 16}, 1}};
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    brush.diameter = 8;
    brush.opacity = 128.0 / 255;
    QByteArray reference;
    for (bool filled : {false, true}) {
        OperationDocument document(QSize(64, 64));
        const auto layer = document.activeLayer();
        FigureStyle style;
        if (filled) style.fill = RgbaPixel{221, 17, 44, 255};
        QCOMPARE(document.paintFigure(layer, brush, samples, shape, style, 42), Result::Success);
        TileRenderer renderer(m_device);
        const auto version = render(renderer, document.snapshot());
        QVERIFY(version);
        const auto pixels = readTile(version, layer, {0, 0});
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        QCOMPARE(pixels.mid((16 * TileRenderer::TileSize + 16) * 4, 4), QByteArray::fromHex("1181f180"));
        if (!filled) reference = pixels;
        else QCOMPARE(pixels, reference);
        QCOMPARE(document.undo(), Result::Success);
        const auto undone = render(renderer, document.snapshot(), version);
        QVERIFY(undone);
        QVERIFY(undone->tiles(layer).isEmpty());
        QCOMPARE(document.redo(), Result::Success);
        const auto redone = render(renderer, document.snapshot(), undone);
        QVERIFY(redone);
        QCOMPARE(readTile(redone, layer, {0, 0}), pixels);

        const auto restored = render(renderer, document.snapshot());
        QVERIFY(restored);
        QCOMPARE(readTile(restored, layer, {0, 0}), pixels);
    }
}

void KisGpuTileRendererTest::longFillEdgesKeepPixelsAcrossRegions()
{
    const auto limit = OperationDocument::CoordinateLimit;
    OperationDocument document(QSize(1024, 256));
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::LineTo, PathCommand::LineTo};
    const QList<InputSample> samples{{{-limit, 16.25}, 1}, {{limit, 16.265625}, 1},
                                     {{limit, 48}, 1}, {{-limit, 48}, 1}};
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{17, 129, 241, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples, shape, style, 42), Result::Success);
    TileRenderer renderer(m_device);
    QByteArray reference;
    for (const auto &region : {QRect(0, 0, 256, 256), QRect(-256, 0, 768, 256), QRect(0, 0, 768, 256)}) {
        const auto commands = m_device->beginCommands();
        QVERIFY(commands);
        auto finishFrame = qScopeGuard([&] { m_device->submitCommands(true); });
        const auto version = renderer.recordRegion(commands, document.snapshot(), region, {document.activeLayer()});
        QCOMPARE(version.code, RenderOutcome::Recorded);
        QVERIFY(m_device->submitCommands(true));
        finishFrame.dismiss();
        const auto pixels = readTile(version.version, document.activeLayer(), {0, 0});
        QCOMPARE(pixels.size(), TileRenderer::TileBytes);
        // The original 26.6 endpoints span 4/256 pixel vertically. At x=0,
        // integer cell splitting retains y=16*256+66, hence coverage 190.
        // Clipping the line before quantization would discard this precision.
        QCOMPARE(pixels.mid(16 * 256 * 4, 4), QByteArray::fromHex("1181f1be"));
        if (reference.isEmpty()) reference = pixels;
        else QCOMPARE(pixels, reference);
    }
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistory_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("subpaths"),
                              QStringLiteral("even-odd"), QStringLiteral("winding"), QStringLiteral("empty")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::pathCommandsPreserveCoverageHistory()
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
        style.fill = RgbaPixel{221, 17, 44, 255};
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

    const auto restored = render(renderer, document.snapshot());
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
    incremental = render(renderer, document.snapshot(), incremental);
    QVERIFY(incremental);
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

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("pixel-perfect-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletion_data()
{
    QTest::addColumn<QString>("kind");
    for (const auto &kind : {QStringLiteral("curve"), QStringLiteral("idle-then-continue"),
                            QStringLiteral("idle-two"), QStringLiteral("tap"),
                            QStringLiteral("tap-after-two"), QStringLiteral("stationary")})
        QTest::newRow(qPrintable(kind)) << kind;
}

void KisGpuTileRendererTest::basicSmoothingPreservesCurvesIdleCompletion()
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

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile file(QDir(results).filePath(QStringLiteral("basic-smoothing-") + kind + QStringLiteral(".rgba")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(pixels), qint64(pixels.size()));
    }
}

void KisGpuTileRendererTest::weightedSmoothingPreservesPixels_data()
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

void KisGpuTileRendererTest::weightedSmoothingPreservesPixels()
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

    QCOMPARE(readTile(render(renderer, document.snapshot()), layer, {0, 0}), pixels);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
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
