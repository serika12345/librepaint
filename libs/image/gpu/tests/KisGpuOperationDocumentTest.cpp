/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <limits>
#include <stdexcept>

#include <gpu/KisGpuOperationDocument.h>
#include "KisGpuMultiplyProjectionFixture_p.h"
#include "KisGpuScreenProjectionFixture_p.h"
#include "KisGpuMinMaxProjectionFixture_p.h"
#include "KisGpuAdditionProjectionFixture_p.h"

using namespace LibrePaint::Image::Gpu;

namespace {
InputSample sample(qreal x, qreal y, qreal pressure = 1, quint64 time = 0)
{
    return {{x, y}, pressure, 0, 0, 0, time};
}

OperationId paint(OperationDocument &document, const QList<InputSample> &samples)
{
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    if (id.isNull() || document.appendSamples(id, samples) != Result::Success ||
        document.commitStroke(id) != Result::Success) {
        return {};
    }
    return id;
}

RasterImage rasterImage()
{
    RasterImage image;
    image.size = {320, 180};
    image.activeLayer = 1;
    image.layers = {
        {QStringLiteral("Red"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")),
         {{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)}}},
        {QStringLiteral("Hidden blue"), false, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")),
         {{{1, -1}, QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4)}}}
    };
    image.layers[0].defaultPixel = {12, 34, 56, 78};
    return image;
}

RasterImage nestedImage()
{
    auto image = rasterImage();
    image.layers.resize(1);
    image.activeLayer = 0;
    RasterLayer inner{QStringLiteral("Inner"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}};
    RasterLayer outer{QStringLiteral("Outer"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}};
    inner.kind = outer.kind = LayerKind::Group;
    image.layers[0].parentUuid = inner.uuid;
    inner.parentUuid = outer.uuid;
    image.layers.append(inner);
    image.layers.append(outer);
    return image;
}

QList<LayerInsertion> insertionForest(const Snapshot &source, const QUuid &parent = {})
{
    QHash<QUuid, QUuid> identities;
    for (const auto &layer : source->layers)
        identities.insert(layer.uuid, QUuid::createUuidV5(layer.uuid, QByteArrayLiteral("Inserted")));
    QList<LayerInsertion> result;
    for (const auto &layer : source->layers) {
        RasterLayer properties;
        properties.name = layer.name;
        properties.uuid = identities.value(layer.uuid);
        properties.kind = layer.kind;
        properties.parentUuid = identities.value(layer.parentUuid, parent);
        properties.defaultPixel = layer.defaultPixel;
        properties.passThrough = layer.passThrough;
        properties.blendMode = layer.blendMode;
        properties.opacity = 173;
        properties.pinnedToTimeline = true;
        properties.channelWriteMask = layer.kind == LayerKind::Paint ? 7 : 15;
        properties.position = layer.kind == LayerKind::Paint ? QPoint(-256, 512) : QPoint();
        result.append({source, layer.id, properties});
    }
    auto secondRoot = result.first();
    secondRoot.properties.uuid = QUuid::createUuidV5(source->layers.first().uuid, QByteArrayLiteral("Second root"));
    secondRoot.properties.parentUuid = parent;
    secondRoot.properties.name = QStringLiteral("Second root");
    result.append(secondRoot);
    return result;
}
}

class KisGpuOperationDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void preparedLayerForestsKeepPixelsSelectionAndAtomicHistory();
    void layerForestInsertionCanClearPixelSelectionAtomically_data();
    void layerForestInsertionCanClearPixelSelectionAtomically();
    void rejectedLayerForestsPreservePendingInputAndHistory();
    void nestedLayerIdentitySurvivesPropertiesHistory();
    void colorLabelsPreserveFixedVersionsPendingInput();
    void projectionChannelsPreservePixelsPendingInputHistory();
    void groupPassThroughPreservesPinnedInputHistory();
    void groupPassThroughPreviewsAdoptAndCancelOneAttribute();
    void importedAndCopiedGroupsKeepPassThroughModes();
    void layerBlendModesKeepPixelsHistoryPendingInput_data();
    void layerBlendModesKeepPixelsHistoryPendingInput();
    void layerBlendModePreviewsAdoptAndCancelOneAttribute_data();
    void layerBlendModePreviewsAdoptAndCancelOneAttribute();
    void separableGroupDefaultsMatchNativeProjection();
    void layerPropertyPreviewsBecomeOneReversibleEdit();
    void rejectedLayerPropertyPreviewsRetainHistoryAndInput();
    void layerPropertyAcceptanceRetainsEditsCompletedDuringPreview();
    void layerPropertyCancellationKeepsExactValuesPendingInputAndRedo();
    void directHierarchyMovesAdoptTheirActiveRootAtomically();
    void movedSubtreesPreservePixelsSelectionHistory_data();
    void movedSubtreesPreservePixelsSelectionHistory();
    void rejectedHierarchyMovesPreserveRedoInputAndClosedState();
    void nestedRemovalOwnsDescendantsAndOneHistoryStep();
    void nestedDuplicationCopiesSubtreePixelsAndIndependentParents();
    void hierarchyDuplicationPreservesPlacementSelection_data();
    void hierarchyDuplicationPreservesPlacementSelection();
    void nestedCanvasChangesKeepGroupsDerivedAndUndoable();
    void groupProjectionDefaultsRespectIsolationAndQuantizedOpacity_data();
    void groupProjectionDefaultsRespectIsolationAndQuantizedOpacity();
    void canvasResizePreservesVersionSizesProperties();
    void canvasResizeKeepsEmptyDocumentsUndoable();
    void directLayerPositionKeepsCanvasHistoryBoundaries();
    void rejectedCanvasResizesKeepRedoPendingInputAndClosedState();
    void layerCropPreservesValuesHistory();
    void rejectedLayerCropsKeepRedoPendingInputAndClosedState();
    void invertColorsFreezesCoverageAndPreservesHistory();
    void rejectedInvertColorsPreserveHistoryAndInput();
    void grayscaleFilterFreezesCoverageAndPreservesHistory();
    void hsvAdjustmentFreezesSettingsAndPreservesHistory();
    void hsvAdjustmentFreezesLumaDependentModesAndPreservesHistory();
    void hsvAdjustmentRejectsInvalidSettingsAndPreservesHistory();
    void hsvAdjustmentRejectsUnfrozenLumaAndInvalidRanges();
    void layerTranslationPreservesInputVersionsAndOneHistoryEntry();
    void groupedRemovalPreservesPixelsSelectionAndAtomicHistory();
    void insertedLayersPreserveOrderPinsAndOneHistoryStep();
    void nestedPaintInsertionRejectsMissingAndNonGroupParents();
    void emptyGroupInsertionPreservesHierarchyHistoryAndRejectsSubtrees();
    void duplicatedLayersShareInputsAndPixelsWithIndependentHistory();
    void identicalLayerDuplicationsPreserveTheSamePersistentIdentities();
    void insertedForeignLayersSharePixelsAndInputsWithAtomicHistory();
    void selectionDuringInputPreservesPaintingAndHistory();
    void activeLayerAndSelectionRemainIndependentAndInput();
    void mergedPanelTogglesPreservePinsAndSaveBoundaries_data();
    void mergedPanelTogglesPreservePinsAndSaveBoundaries();
    void historyActionsSurviveDirectWritesMerging();
    void mergedPropertiesPreservePinsSaveBoundaries();
    void mergedNameResetRetainsItsRecordedBoundary();
    void mergedOpacityResetPreservesSelectionAndDirectWrites();
    void directLayerPropertiesPreserveOtherPropertyHistoryAndFixedVersions();
    void directOpacityWritesPreserveRecordedUndoValues();
    void layerOpacityAndLockShareHistory();
    void channelProtectionPreservesFrozenSettingsHistory();
    void symmetryFreezesAxesAndRejectsInvalidInput();
    void inputChunksRetainTheSameOperation();
    void shapePreviewCommitsAsOneHistoryEntry();
    void filledFiguresPreserveStylesHistory();
    void brushCanvasOpacitySurvivesPinnedHistory();
    void invalidCanvasOpacityPreservesRedo();
    void fillBlendModesFreezeHistory_data();
    void fillBlendModesFreezeHistory();
    void strokeBlendModesFreezeHistory_data();
    void strokeBlendModesFreezeHistory();
    void rejectedStrokeBlendModesPreserveHistoryAndInput();
    void patternFillBlendModeFreezesHistory();
    void rejectedFillBlendModesPreserveHistoryAndInput();
    void patternFillFreezesPixelsHistory();
    void emptyFigureIntentPreservesHistory();
    void pathsPreserveCommandsControlPoints();
    void pathCallsRetainInputHistory();
    void invalidPathCallsPreserveTheActiveDocument_data();
    void invalidPathCallsPreserveTheActiveDocument();
    void pixelCorrectionRetainsOriginalSensorsAndPendingInput();
    void selectionRectIsFrozenInOperations();
    void invertedSelectionPreservesHistory();
    void ellipticalSelectionPreservesHistory();
    void freehandSelectionFreezesCoverageHistory();
    void featheredSelectionAddsAlphaHistory();
    void basicSmoothingRetainsIdleBoundariesSensorsAndFinalIntent();
    void weightedSmoothingRetainsFrozenSettingsSpeed();
    void basicSmoothingRejectsConflictingModesAndShapeChanges();
    void emptyPathsPreserveHistoryAndPendingInput();
    void invalidPathsPreserveHistoryAndInputBudgets();
    void gaussianWashSettingsSurviveHistory();
    void stationaryBrushCommitInvalidatesPendingPixelsAndRetainsInput_data();
    void stationaryBrushCommitInvalidatesPendingPixelsAndRetainsInput();
    void invalidGaussianWashSettingsPreserveTheDocument();
    void transparencyMaskHistoryComposition();
    void transparencyMaskFromSelectionShapesAndRejections();
    void transparencyMaskCoverageReplacementPreservesHistory();
    void transparencyMaskFollowsCanvasResize();
    void transparencyMaskFollowsLayerCrop();
    void cancelAndRejectedInputPreserveRedo();
    void documentChangesShareOneHistory();
    void savesPinTheirVersionWhileEditingContinues();
    void historyRetains250Operations();
    void staleAndForeignEditsCannotChangeADocument();
    void frozenOperationsOutliveTheDocument();
    void limitsRejectWholeRequests();
    void closedDocumentPreservesSnapshots();
    void cancelledPreviewsCannotBecomeSavedVersions();
    void emptyAndInvalidRequestsLeaveHistoryUnchanged();
    void importedPixelsPreserveLayersAndStartWithoutUndo();
    void tileReplacementSharesHistoryAndRejectsWholeRequests();
    void layerStructureSharesPaintingHistoryAndPreservesPins();
    void rejectedLayerChangesKeepRedoAndPendingInput();
    void removingTheLastLayerPreservesAnEmptyUndoableDocument();
    void layerRemovalReusesCapacityAcrossHistory_data();
    void layerRemovalReusesCapacityAcrossHistory();
    void layerReorderingIsAtomicAndPreservesSelectionPixelsAndHistory();
    void rgba16TilesCannotEnterAnRgba8Document();
    void rgba16PixelsAndBrushValuesSurviveHistoryPins();
    void native16UnsupportedOperationsPreservePixelsAndHistory();
};

void KisGpuOperationDocumentTest::rgba16TilesCannotEnterAnRgba8Document()
{
    const RgbaTile tile{{-1, 0}, QByteArray(RgbaTile::byteCount(PixelFormat::Rgba16), '\0'), PixelFormat::Rgba16};
    RasterImage image;
    image.size = {64, 64};
    image.layers = {{QStringLiteral("Paint"), true, {}, {tile}}};
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument{image});

    OperationDocument document({64, 64});
    const auto before = document.snapshot();
    const auto stateBeforeRequest = document.preview();
    QCOMPARE(document.replaceTiles(document.activeLayer(), {tile}), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.preview(), stateBeforeRequest);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::native16UnsupportedOperationsPreservePixelsAndHistory()
{
    RasterImage input;
    input.size = {64, 48};
    input.format = PixelFormat::Rgba16;
    input.layers = {{QStringLiteral("Native paint"), true, {}, {}, {1, 257, 65534, 32768}}};
    for (const auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen,
                           LayerBlendMode::Darken, LayerBlendMode::Lighten,
                           LayerBlendMode::Addition, LayerBlendMode::LinearDodge}) {
        auto unsupported = input;
        unsupported.layers.first().blendMode = mode;
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument{unsupported});
    }
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerBlendMode(layer, LayerBlendMode::Multiply), Result::InvalidInput);
    QCOMPARE(document.writeLayerBlendMode(layer, LayerBlendMode::Screen), Result::InvalidInput);
    QCOMPARE(document.invertColors(layer), Result::InvalidInput);
    QCOMPARE(document.grayscale(layer), Result::InvalidInput);
    Brush brush;
    brush.rgba = {65534, 1, 257, 65535};
    brush.blendMode = LayerBlendMode::Multiply;
    QVERIFY(document.beginStroke(layer, brush, 42).isNull());
    brush.blendMode = LayerBlendMode::Normal;
    brush.gaussianWash.emplace();
    QVERIFY(document.beginStroke(layer, brush, 42).isNull());
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.undoCount(), 0);
    brush.gaussianWash.reset();
    const auto operation = document.beginStroke(layer, brush, 42);
    QVERIFY(!operation.isNull());
    QCOMPARE(document.appendSamples(operation, {sample(1, 1), sample(4, 4)}), Result::Success);
    const auto pending = document.preview();
    Shape rectangle;
    rectangle.kind = ShapeKind::Rectangle;
    rectangle.first = {1, 1}; rectangle.second = {4, 4};
    FigureStyle fill;
    fill.fill = RgbaPixel{1, 257, 65534, 65535};
    QCOMPARE(document.updateFigure(operation, rectangle, fill), Result::InvalidInput);
    QCOMPARE(document.preview(), pending);
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.cancelStroke(operation), Result::Success);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::rgba16PixelsAndBrushValuesSurviveHistoryPins()
{
    RasterImage image;
    image.size = {320, 180};
    image.format = PixelFormat::Rgba16;
    image.projectionBackground = {1, 255, 256, 0};
    RasterLayer layer;
    layer.name = QStringLiteral("16-bit paint");
    layer.uuid = QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}"));
    layer.defaultPixel = {257, 32767, 32768, 0};
    const auto pixels = QByteArray::fromHex("0001ff0001000000feff0080ff7fffff").repeated(256 * 256 / 2);
    layer.tiles = {{{-1, 0}, pixels, PixelFormat::Rgba16}};
    image.layers = {layer};
    Limits limits;
    limits.rasterBytes = 2 * RgbaTile::byteCount(PixelFormat::Rgba16);
    OperationDocument document(image, limits);
    const auto initial = document.snapshot();
    const auto retainedTile = std::get<TileWriteSnapshot>(initial->layers.first().operations.first())->tiles.first();
    QCOMPARE(initial->format, image.format);
    QCOMPARE(initial->projectionBackground, image.projectionBackground);
    QCOMPARE(initial->layers.first().defaultPixel, layer.defaultPixel);
    QCOMPARE(initial->projectionDefaultPixels().value(document.activeLayer()), layer.defaultPixel);
    QCOMPARE(initial->rasterBytes, pixels.size());
    QCOMPARE(retainedTile->rgba(), pixels);
    QCOMPARE(document.undoCount(), 0);

    auto replacement = layer.tiles.first();
    replacement.rgba.replace(0, 8, QByteArray::fromHex("0101feffffff0100"));
    QCOMPARE(document.replaceTiles(document.activeLayer(), {replacement}), Result::Success);
    const auto replaced = document.snapshot();
    QCOMPARE(replaced->rasterBytes, 2 * pixels.size());
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), replaced);
    QCOMPARE(retainedTile->rgba(), pixels);
    QCOMPARE(document.replaceTiles(document.activeLayer(), {replacement}), Result::CapacityExceeded);
    QCOMPARE(document.snapshot(), replaced);

    Brush brush;
    brush.rgba = {1, 257, 65534, 65535};
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 42);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(-0.5, 255.5, 0.75, 123)}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.snapshot()->format, PixelFormat::Rgba16);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), replaced);

    // A format mismatch is rejected before changing history, including a
    // compressed tile that is otherwise valid in its own precision.
    OperationDocument fresh(image);
    const auto before = fresh.snapshot();
    auto wrong = layer.tiles.first();
    wrong.format = PixelFormat::Rgba8;
    wrong.rgba = QByteArray(RgbaTile::ByteCount, '\0');
    QCOMPARE(fresh.replaceTiles(fresh.activeLayer(), {wrong}), Result::InvalidInput);
    QCOMPARE(fresh.snapshot(), before);
    image.layers.first().tiles = {wrong};
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument{image});
}

void KisGpuOperationDocumentTest::layerForestInsertionCanClearPixelSelectionAtomically_data()
{
    QTest::addColumn<bool>("clearPixelSelection");
    QTest::newRow("preserve") << false;
    QTest::newRow("paste-and-deselect") << true;
}

void KisGpuOperationDocumentTest::layerForestInsertionCanClearPixelSelectionAtomically()
{
    QFETCH(bool, clearPixelSelection);
    OperationDocument source(nestedImage());
    OperationDocument destination(nestedImage());
    const auto mask = SelectionMask::fromPolygon({{1, 1}, {30, 1}, {1, 15}}, QRect(0, 0, 64, 32));
    QVERIFY(mask);
    QCOMPARE(destination.setSelectionMask(mask, true), Result::Success);
    const auto before = destination.snapshot();
    const auto count = destination.undoCount();
    const auto inputs = insertionForest(source.snapshot(), before->layers[1].uuid);
    QCOMPARE(destination.insertLayers({}, 1, true, clearPixelSelection), Result::InvalidInput);
    QCOMPARE(destination.snapshot(), before);
    QCOMPARE(destination.undoCount(), count);
    const auto result = clearPixelSelection ? destination.insertLayers(inputs, 1, true, true)
                                           : destination.insertLayers(inputs, 1);
    QCOMPARE(result, Result::Success);
    QCOMPARE(destination.undoCount(), count + 1);
    QCOMPARE(destination.snapshot()->layers.size(), before->layers.size() + inputs.size());
}

void KisGpuOperationDocumentTest::preparedLayerForestsKeepPixelsSelectionAndAtomicHistory()
{
    OperationDocument source(nestedImage());
    QVERIFY(!paint(source, {sample(8, 9), sample(12, 13)}).isNull());
    const auto fixed = source.snapshot();
    OperationDocument destination(nestedImage());
    QCOMPARE(destination.selectLayers({1, 3}, 2), Result::Success);
    const auto before = destination.snapshot();
    auto inputs = insertionForest(fixed, before->layers[1].uuid);
    source.close();
    QCOMPARE(destination.insertLayers(inputs, 1), Result::Success);
    const auto after = destination.snapshot();
    QCOMPARE(after->layers.size(), 7);
    QCOMPARE(after->activeLayer, after->layers[3].id);
    QCOMPARE(after->selectedLayers, (QList<LayerId>{after->layers[3].id, after->layers[4].id}));
    QCOMPARE(after->layers[1].parentUuid, inputs[1].properties.uuid);
    QCOMPARE(after->layers[2].parentUuid, inputs[2].properties.uuid);
    QCOMPARE(after->layers[3].parentUuid, before->layers[1].uuid);
    QCOMPARE(after->layers[4].parentUuid, before->layers[1].uuid);
    QCOMPARE(after->sampleCount, fixed->sampleCount * 2);
    QCOMPARE(after->rasterBytes, before->rasterBytes + fixed->rasterBytes * 2);
    for (int index : {1, 4}) {
        const auto &copy = after->layers[index];
        QCOMPARE(copy.opacity, quint8(173));
        QVERIFY(copy.pinnedToTimeline);
        QCOMPARE(copy.position, QPoint(-256, 512));
        QCOMPARE(copy.channelWriteMask, quint8(7));
        QCOMPARE(std::get<TileWriteSnapshot>(copy.operations.first())->tiles,
                 std::get<TileWriteSnapshot>(fixed->layers.first().operations.first())->tiles);
        const auto input = std::get<PaintSnapshot>(copy.operations.last());
        QCOMPARE(input->sampleBlocks, std::get<PaintSnapshot>(fixed->layers.first().operations.last())->sampleBlocks);
        QCOMPARE(input->id.document, after->documentId());
        QCOMPARE(input->layer, copy.id);
    }
    QCOMPARE(destination.undoCount(), 1);
    QCOMPARE(destination.historyAction(1), HistoryAction::AddLayer);
}

void KisGpuOperationDocumentTest::rejectedLayerForestsPreservePendingInputAndHistory()
{
    OperationDocument source(nestedImage());
    QVERIFY(!paint(source, {sample(8, 9), sample(12, 13)}).isNull());
    const auto inputs = insertionForest(source.snapshot());
    OperationDocument destination({64, 32});
    QVERIFY(destination.addLayer(QStringLiteral("Redo")));
    const auto reserved = destination.snapshot()->layers.last().uuid;
    QCOMPARE(destination.undo(), Result::Success);
    const auto before = destination.snapshot();
    const auto retainedState = destination.preview();
    for (int failure = 0; failure < 6; ++failure) {
        auto invalid = inputs;
        if (failure == 0) invalid[3].sourceLayer = 999;
        if (failure == 1) invalid[3].properties.uuid = invalid[0].properties.uuid;
        if (failure == 2) invalid[1].properties.parentUuid = invalid[0].properties.uuid;
        if (failure == 3) invalid[2].properties.parentUuid = invalid[1].properties.uuid;
        if (failure == 4) invalid[3].properties.uuid = reserved;
        if (failure == 5) invalid[3].properties.parentUuid = QUuid::createUuid();
        QCOMPARE(destination.insertLayers(invalid, 0), failure == 0 ? Result::LayerUnavailable : Result::InvalidInput);
        QCOMPARE(destination.snapshot(), before);
        QCOMPARE(destination.preview(), retainedState);
        QCOMPARE(destination.redoCount(), 1);
    }
    QCOMPARE(destination.insertLayers({}, 0), Result::InvalidInput);
    QCOMPARE(destination.insertLayers(inputs, -1), Result::InvalidInput);
    const auto stroke = destination.beginStroke(1, Brush{}, 42);
    QVERIFY(!stroke.isNull());
    const auto preview = destination.preview();
    QCOMPARE(destination.insertLayers(inputs, 0), Result::EditInProgress);
    QCOMPARE(destination.preview(), preview);
    QCOMPARE(destination.snapshot(), before);
    QCOMPARE(destination.cancelStroke(stroke), Result::Success);
    for (int budget = 0; budget < 4; ++budget) {
        Limits limits;
        if (budget == 0) limits.layerCount = inputs.size();
        if (budget == 1) limits.rasterBytes = RgbaTile::ByteCount * 2 - 1;
        if (budget == 2) limits.samplesPerDocument = 3;
        if (budget == 3) limits.samplesPerStroke = 1;
        OperationDocument bounded({64, 32}, limits);
        const auto retained = bounded.preview();
        QCOMPARE(bounded.insertLayers(inputs, 0), Result::CapacityExceeded);
        QCOMPARE(bounded.preview(), retained);
    }
    destination.close();
    const auto closed = destination.snapshot();
    QCOMPARE(destination.insertLayers(inputs, 0), Result::Closed);
    QCOMPARE(destination.snapshot(), closed);
}

void KisGpuOperationDocumentTest::hierarchyDuplicationPreservesPlacementSelection_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QString>("layout");
    for (const auto &scenario : {QStringLiteral("nested-paint"), QStringLiteral("nested-group"),
                                QStringLiteral("cross-parent"), QStringLiteral("ancestor-and-child"),
                                QStringLiteral("locked-parent"), QStringLiteral("locked-ancestor")}) {
        for (const auto &layout : {QStringLiteral("child-first"), QStringLiteral("parent-first"), QStringLiteral("interleaved")})
            QTest::newRow(qPrintable(scenario + QLatin1Char('-') + layout)) << scenario << layout;
    }
}

void KisGpuOperationDocumentTest::hierarchyDuplicationPreservesPlacementSelection()
{
    QFETCH(QString, scenario);
    QFETCH(QString, layout);
    auto input = nestedImage();
    const auto leaf = input.layers[0], inner = input.layers[1], outer = input.layers[2];
    const RasterLayer base{QStringLiteral("Base"), true, QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}")), {}};
    const RasterLayer top{QStringLiteral("Top"), true, QUuid(QStringLiteral("{41424344-4546-4748-494a-4b4c4d4e4f50}")), {}};
    input.layers = layout == QLatin1String("child-first") ? QList<RasterLayer>{base, leaf, inner, outer, top}
        : layout == QLatin1String("parent-first") ? QList<RasterLayer>{base, outer, inner, leaf, top}
        : QList<RasterLayer>{base, outer, top, leaf, inner};
    input.activeLayer = 0;
    for (auto &layer : input.layers) {
        if (layer.uuid == inner.uuid) layer.userLocked = scenario == QLatin1String("locked-parent");
        if (layer.uuid == outer.uuid) layer.userLocked = scenario == QLatin1String("locked-ancestor");
    }
    OperationDocument document(input);
    const auto initial = document.snapshot();
    const auto sourceId = [&](const QUuid &uuid) {
        for (const auto &layer : initial->layers) if (layer.uuid == uuid) return layer.id;
        return LayerId(0);
    };
    QList<LayerId> selected{sourceId(leaf.uuid)};
    QStringList rootNames{leaf.name + QStringLiteral(" copy")};
    QUuid parent = inner.uuid, anchor = leaf.uuid;
    auto active = sourceId(leaf.uuid);
    if (scenario == QLatin1String("nested-group")) {
        selected = {sourceId(inner.uuid)}; rootNames = {QStringLiteral("Inner copy")};
        parent = outer.uuid; anchor = inner.uuid; active = sourceId(inner.uuid);
    } else if (scenario == QLatin1String("cross-parent")) {
        selected = {sourceId(leaf.uuid), sourceId(top.uuid)}; rootNames.append(QStringLiteral("Top copy"));
        parent = QUuid(); anchor = top.uuid;
    } else if (scenario == QLatin1String("ancestor-and-child")) {
        selected = {sourceId(leaf.uuid), sourceId(outer.uuid)}; rootNames = {QStringLiteral("Outer copy")};
        parent = QUuid(); anchor = outer.uuid;
    } else if (scenario == QLatin1String("locked-parent")) {
        parent = outer.uuid; anchor = inner.uuid;
    } else if (scenario == QLatin1String("locked-ancestor")) {
        parent = QUuid(); anchor = outer.uuid;
    }
    std::sort(selected.begin(), selected.end());
    QList<QString> names;
    for (const auto id : selected)
        for (const auto &layer : initial->layers) if (layer.id == id) names.append(layer.name + QStringLiteral(" copy"));
    QCOMPARE(document.selectLayers(selected, active), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.duplicateLayers(selected, names, active), Result::Success);
    const auto after = document.snapshot();
    QList<LayerId> copies;
    for (const auto &name : rootNames) {
        const auto found = std::find_if(after->layers.cbegin(), after->layers.cend(),
            [&](const Layer &layer) { return layer.id > 5 && layer.name == name; });
        QVERIFY2(found != after->layers.cend(), qPrintable(name));
        QCOMPARE(found->parentUuid, parent);
        copies.append(found->id);
    }
    QCOMPARE(after->selectedLayers, copies);
    QCOMPARE(after->activeLayer, copies.first());
    QList<LayerId> expectedOrder, actualOrder;
    for (const auto &layer : before->layers) if (layer.parentUuid == parent) expectedOrder.append(layer.id);
    const auto insertion = expectedOrder.indexOf(sourceId(anchor)) + 1;
    for (int i = 0; i < copies.size(); ++i) expectedOrder.insert(insertion + i, copies[i]);
    for (const auto &layer : after->layers) if (layer.parentUuid == parent) actualOrder.append(layer.id);
    QCOMPARE(actualOrder, expectedOrder);
    const auto sourceLeaf = std::find_if(before->layers.cbegin(), before->layers.cend(),
        [&](const Layer &layer) { return layer.uuid == leaf.uuid; });
    QVERIFY(sourceLeaf != before->layers.cend());
    QSet<QUuid> identities;
    int paintCopies = 0;
    for (const auto &layer : after->layers) {
        QVERIFY(!identities.contains(layer.uuid)); identities.insert(layer.uuid);
        if (layer.id <= 5 || layer.kind != LayerKind::Paint || layer.operations.isEmpty()) continue;
        ++paintCopies;
        QCOMPARE(std::get<TileWriteSnapshot>(layer.operations.first())->tiles.first(),
                 std::get<TileWriteSnapshot>(sourceLeaf->operations.first())->tiles.first());
        if (scenario == QLatin1String("ancestor-and-child") || scenario == QLatin1String("nested-group"))
            QCOMPARE(layer.name, sourceLeaf->name);
    }
    QCOMPARE(paintCopies, 1);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), after);
}

void KisGpuOperationDocumentTest::directHierarchyMovesAdoptTheirActiveRootAtomically()
{
    auto input = nestedImage();
    RasterLayer top{QStringLiteral("Top"), true, QUuid(QStringLiteral("{41424344-4546-4748-494a-4b4c4d4e4f50}")), {}};
    input.layers.append(top);
    OperationDocument document(input);
    QCOMPARE(document.selectLayers({3, 1}, 1), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.markSaved(before), Result::Success);
    const auto stateBeforeRequest = document.preview();
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 1), Result::InvalidInput);
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 99), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.preview(), stateBeforeRequest);
    QCOMPARE(document.moveLayers({3, 1}, {}, {}, 3), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.preview(), stateBeforeRequest);
    const auto stroke = document.beginStroke(1, Brush{}, 42);
    QVERIFY(!stroke.isNull());
    const auto preview = document.preview();
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 3), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 3), Result::Success);
    const auto after = document.snapshot();
    QCOMPARE(after->activeLayer, LayerId(3));
    QCOMPARE(after->selectedLayers, QList<LayerId>{3});
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::MoveLayer);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->activeLayer, LayerId(1));
    QCOMPARE(document.snapshot()->selectedLayers, QList<LayerId>{3});
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->activeLayer, LayerId(3));
    QCOMPARE(document.snapshot()->selectedLayers, QList<LayerId>{3});
    QVERIFY(document.isModified());

    QCOMPARE(document.snapshot(), after);
    QCOMPARE(before->activeLayer, LayerId(1));
    QCOMPARE(before->selectedLayers, (QList<LayerId>{1, 3}));
}

void KisGpuOperationDocumentTest::movedSubtreesPreservePixelsSelectionHistory_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<int>("layout");
    for (const auto &scenario : {QStringLiteral("group"), QStringLiteral("enter"), QStringLiteral("leave"),
                                QStringLiteral("cross-parent"), QStringLiteral("ancestor-and-child"),
                                QStringLiteral("parent-only"), QStringLiteral("locked-source")})
        for (int layout = 0; layout < 3; ++layout)
            QTest::newRow(qPrintable(scenario + QString::number(layout))) << scenario << layout;
}

void KisGpuOperationDocumentTest::movedSubtreesPreservePixelsSelectionHistory()
{
    QFETCH(QString, scenario);
    QFETCH(int, layout);
    auto input = nestedImage();
    auto leaf = input.layers[0], inner = input.layers[1], outer = input.layers[2];
    RasterLayer base{QStringLiteral("Base"), true, QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}")), {}};
    RasterLayer top{QStringLiteral("Top"), true, QUuid(QStringLiteral("{41424344-4546-4748-494a-4b4c4d4e4f50}")), {}};
    outer.userLocked = scenario == QLatin1String("locked-source");
    input.layers = layout == 0 ? QList<RasterLayer>{base, leaf, inner, outer, top}
        : layout == 1 ? QList<RasterLayer>{base, outer, inner, leaf, top}
        : QList<RasterLayer>{base, outer, top, leaf, inner};
    input.activeLayer = 0;
    OperationDocument document(input);
    const auto initial = document.snapshot();
    const auto id = [&](const QUuid &uuid) {
        for (const auto &layer : initial->layers) if (layer.uuid == uuid) return layer.id;
        return LayerId(0);
    };
    QList<LayerId> requested{id(outer.uuid)}, roots = requested;
    QUuid parent, above;
    auto active = id(outer.uuid);
    if (scenario == QLatin1String("enter")) {
        requested = roots = {id(base.uuid)}; parent = outer.uuid; active = id(base.uuid);
    } else if (scenario == QLatin1String("leave")) {
        requested = roots = {id(inner.uuid)}; above = outer.uuid; active = id(inner.uuid);
    } else if (scenario == QLatin1String("cross-parent")) {
        requested = {id(leaf.uuid), id(base.uuid)}; roots = {id(base.uuid), id(leaf.uuid)};
        parent = outer.uuid; above = inner.uuid; active = id(leaf.uuid);
    } else if (scenario == QLatin1String("ancestor-and-child")) {
        requested = {id(leaf.uuid), id(inner.uuid)}; roots = {id(inner.uuid)};
        above = outer.uuid; active = id(leaf.uuid);
    } else if (scenario == QLatin1String("parent-only")) {
        requested = roots = {id(leaf.uuid)}; parent = outer.uuid; active = id(leaf.uuid);
    }
    QCOMPARE(document.selectLayers(requested, active), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.markSaved(before), Result::Success);
    const auto hierarchy = [](const Snapshot &state) {
        QHash<QUuid, QList<LayerId>> result;
        for (const auto &layer : state->layers) result[layer.parentUuid].append(layer.id);
        return result;
    };
    auto expected = hierarchy(before);
    for (const auto root : roots) {
        const auto found = std::find_if(before->layers.cbegin(), before->layers.cend(),
            [&](const Layer &layer) { return layer.id == root; });
        expected[found->parentUuid].removeAll(root);
        if (expected.value(found->parentUuid).isEmpty()) expected.remove(found->parentUuid);
    }
    const auto anchor = above.isNull() ? LayerId(0) : id(above);
    auto insertion = expected.value(parent).indexOf(anchor) + 1;
    for (const auto root : roots) expected[parent].insert(insertion++, root);
    QCOMPARE(document.moveLayers(requested, parent, above), Result::Success);
    const auto after = document.snapshot();
    QCOMPARE(hierarchy(after), expected);
    QCOMPARE(after->layers.size(), before->layers.size());
    QCOMPARE(after->activeLayer, active);
    QCOMPARE(after->selectedLayers.size(), roots.size());
    for (const auto root : roots) QVERIFY(after->selectedLayers.contains(root));
    QCOMPARE(after->sampleCount, before->sampleCount);
    QCOMPARE(after->rasterBytes, before->rasterBytes);
    for (const auto &source : before->layers) {
        const auto found = std::find_if(after->layers.cbegin(), after->layers.cend(),
            [&](const Layer &layer) { return layer.id == source.id; });
        QVERIFY(found != after->layers.cend());
        QCOMPARE(found->uuid, source.uuid);
        QCOMPARE(found->operations, source.operations);
        QCOMPARE(found->defaultPixel, source.defaultPixel);
        QCOMPARE(found->position, source.position);
        QCOMPARE(found->name, source.name);
        QCOMPARE(found->opacity, source.opacity);
        QCOMPARE(found->userLocked, source.userLocked);
        QCOMPARE(found->parentUuid, roots.contains(source.id) ? parent : source.parentUuid);
    }
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::MoveLayer);
    QVERIFY(document.isModified());
}

void KisGpuOperationDocumentTest::rejectedHierarchyMovesPreserveRedoInputAndClosedState()
{
    auto input = nestedImage();
    RasterLayer top{QStringLiteral("Top"), true, QUuid(QStringLiteral("{41424344-4546-4748-494a-4b4c4d4e4f50}")), {}};
    input.layers.append(top);
    OperationDocument document(input);
    const auto inner = input.layers[1].uuid, outer = input.layers[2].uuid;
    QCOMPARE(document.renameLayer(1, QStringLiteral("Renamed")), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    const auto stateBeforeRequest = document.preview();
    const auto missing = QUuid(QStringLiteral("{51525354-5556-5758-595a-5b5c5d5e5f60}"));
    QCOMPARE(document.moveLayers({}, {}, {}), Result::InvalidInput);
    QCOMPARE(document.moveLayers({1, 1}, {}, {}), Result::InvalidInput);
    QCOMPARE(document.moveLayers({99}, {}, {}), Result::LayerUnavailable);
    QCOMPARE(document.moveLayers({4, 99}, outer, {}), Result::LayerUnavailable);
    QCOMPARE(document.moveLayers({1}, missing, {}), Result::LayerUnavailable);
    QCOMPARE(document.moveLayers({1}, input.layers[0].uuid, {}), Result::InvalidInput);
    QCOMPARE(document.moveLayers({1}, {}, missing), Result::LayerUnavailable);
    QCOMPARE(document.moveLayers({1}, {}, inner), Result::InvalidInput);
    QCOMPARE(document.moveLayers({1}, inner, input.layers[0].uuid), Result::InvalidInput);
    QCOMPARE(document.moveLayers({2}, {}, input.layers[0].uuid), Result::InvalidInput);
    QCOMPARE(document.moveLayers({3}, inner, {}), Result::InvalidInput);
    QCOMPARE(document.moveLayers({2}, inner, {}), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.preview(), stateBeforeRequest);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.moveLayers({1}, inner, {}), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.preview(), stateBeforeRequest);
    const auto stroke = document.beginStroke(1, Brush{}, 42);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(5, 6)}), Result::Success);
    const auto preview = document.preview();
    const auto pendingVersion = document.preview();
    QCOMPARE(document.moveLayers({1}, outer, {}), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.preview(), pendingVersion);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    for (const bool destination : {false, true}) {
        auto locked = input;
        locked.layers[destination ? 2 : 1].userLocked = true;
        OperationDocument value(locked);
        const auto untouched = value.snapshot();
        QCOMPARE(value.moveLayers({destination ? LayerId(4) : LayerId(1)}, outer, {}), Result::LayerUnavailable);
        QCOMPARE(value.snapshot(), untouched);
        QCOMPARE(value.undoCount(), 0);
    }
    document.close();
    const auto closed = document.snapshot();
    QCOMPARE(document.moveLayers({1}, outer, {}), Result::Closed);
    QCOMPARE(document.snapshot(), closed);
}

void KisGpuOperationDocumentTest::canvasResizePreservesVersionSizesProperties()
{
    auto source = rasterImage();
    source.activeLayer = 0;
    source.layers[1].userLocked = true;
    OperationDocument document(source);
    QCOMPARE(document.selectLayers({1, 2}, 1), Result::Success);
    const auto before = document.snapshot();
    const QRect canvas(8, 6, 160, 90), crop(-3, -5, 96, 64);
    QCOMPARE(document.resizeCanvas(canvas, false), Result::Success);
    const auto resized = document.snapshot();
    QCOMPARE(resized->size, canvas.size());
    QCOMPARE(document.historyAction(1), HistoryAction::ResizeCanvas);
    QCOMPARE(document.historyCanvasResize(1)->rect, canvas);
    QCOMPARE(document.markSaved(resized), Result::Success);
    QCOMPARE(document.resizeCanvas(crop, true), Result::Success);
    const auto cropped = document.snapshot();
    QCOMPARE(cropped->size, crop.size());
    QCOMPARE(before->size, source.size);
    QCOMPARE(document.historyAction(2), HistoryAction::CropImage);
    QVERIFY(document.historyCanvasResize(2)->cropLayers);
    QCOMPARE(cropped->selectedLayers, before->selectedLayers);
    QCOMPARE(cropped->activeLayer, before->activeLayer);
    QCOMPARE(cropped->rasterBytes, before->rasterBytes);
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(cropped->layers[i].position, before->layers[i].position - canvas.topLeft() - crop.topLeft());
        QCOMPARE(cropped->layers[i].uuid, before->layers[i].uuid);
        QCOMPARE(cropped->layers[i].defaultPixel, before->layers[i].defaultPixel);
        QCOMPARE(cropped->layers[i].userLocked, before->layers[i].userLocked);
        QCOMPARE(cropped->layers[i].visible, before->layers[i].visible);
        QCOMPARE(cropped->layers[i].operations.first(), before->layers[i].operations.first());
    }
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.resizeCanvas(QRect(QPoint(), canvas.size()), false), Result::Success);
    QCOMPARE(document.snapshot(), resized);
    QCOMPARE(document.redoCount(), 1);
}

void KisGpuOperationDocumentTest::canvasResizeKeepsEmptyDocumentsUndoable()
{
    RasterImage source;
    source.size = {64, 48};
    OperationDocument document(source);
    QCOMPARE(document.resizeCanvas({-8, -6, 128, 96}, true), Result::Success);
    QVERIFY(document.snapshot()->layers.isEmpty());
    QCOMPARE(document.snapshot()->size, QSize(128, 96));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->size, source.size);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(128, 96));
}

void KisGpuOperationDocumentTest::directLayerPositionKeepsCanvasHistoryBoundaries()
{
    OperationDocument document({64, 48});
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, false), Result::Success);
    QCOMPARE(document.writeLayerPosition(1, {11, 13}), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, QPoint());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(11, 13));
}

void KisGpuOperationDocumentTest::rejectedCanvasResizesKeepRedoPendingInputAndClosedState()
{
    OperationDocument document({64, 48});
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, true), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    const auto stateBeforeRequest = document.preview();
    for (const QRect rect : {QRect(0, 0, 0, 4), QRect(0, 0, -1, 4),
                             QRect(RgbaTile::CoordinateLimit, 0, 1, 1),
                             QRect(-RgbaTile::CoordinateLimit - 1, 0, 1, 1)}) {
        QCOMPARE(document.resizeCanvas(rect, false), Result::InvalidInput);
        QCOMPARE(document.preview(), stateBeforeRequest);
        QCOMPARE(document.snapshot(), before);
        QCOMPARE(document.redoCount(), 1);
    }
    const auto pending = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(pending, {sample(8, 9)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, true), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.cancelStroke(pending), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    document.close();
    const auto closed = document.snapshot();
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, true), Result::Closed);
    QCOMPARE(document.snapshot(), closed);
}

void KisGpuOperationDocumentTest::groupProjectionDefaultsRespectIsolationAndQuantizedOpacity_data()
{
    QTest::addColumn<QByteArray>("lower");
    QTest::addColumn<QByteArray>("upper");
    QTest::addColumn<bool>("includeUpper");
    QTest::addColumn<bool>("upperVisible");
    QTest::addColumn<int>("groupOpacity");
    QTest::addColumn<QByteArray>("innerPixel");
    QTest::addColumn<QByteArray>("outerPixel");
    QTest::addColumn<int>("upperMask");
    QTest::newRow("two-colour-defaults") << QByteArray::fromHex("ff0000ff") << QByteArray::fromHex("0000ffff")
        << true << true << 128 << QByteArray::fromHex("7f0080ff") << QByteArray::fromHex("7f008080") << 15;
    QTest::newRow("hidden-sibling-normalizes-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << true << false << 255 << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000") << 15;
    QTest::newRow("single-child-keeps-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << false << false << 128 << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000") << 15;
    QTest::newRow("nested-full-opacity-keeps-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << false << false << 255 << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300") << 15;
    QTest::newRow("nested-alpha-is-quantized-at-each-group") << QByteArray::fromHex("ff000080") << QByteArray::fromHex("00000000")
        << false << false << 128 << QByteArray::fromHex("ff000080") << QByteArray::fromHex("ff000040") << 15;
    QTest::newRow("small-alpha-keeps-normalized-source-opacity") << QByteArray::fromHex("99663301") << QByteArray::fromHex("56341201")
        << true << true << 255 << QByteArray::fromHex("83552801") << QByteArray::fromHex("83552801") << 15;
    // Fixed CPU scalar/SIMD corpus at 5cd3e87, RGBA ordering, opacity128.
    QTest::newRow("inherited-default-alpha") << QByteArray::fromHex("996633e2") << QByteArray::fromHex("56341286")
        << true << true << 255 << QByteArray::fromHex("87592ae2") << QByteArray::fromHex("87592ae2") << 7;
    QTest::newRow("selected-default-components") << QByteArray::fromHex("996633e2") << QByteArray::fromHex("56341286")
        << true << true << 255 << QByteArray::fromHex("866633ea") << QByteArray::fromHex("866633ea") << 9;
    QTest::newRow("selected-components-clear-transparent-default") << QByteArray::fromHex("99663300") << QByteArray::fromHex("56341286")
        << true << true << 255 << QByteArray::fromHex("56000043") << QByteArray::fromHex("56000043") << 9;
    QTest::newRow("inherited-transparent-default-keeps-hidden-rgb") << QByteArray::fromHex("99663300") << QByteArray::fromHex("56341286")
        << true << true << 255 << QByteArray::fromHex("170e0500") << QByteArray::fromHex("170e0500") << 7;
}

void KisGpuOperationDocumentTest::groupProjectionDefaultsRespectIsolationAndQuantizedOpacity()
{
    QFETCH(QByteArray, lower);
    QFETCH(QByteArray, upper);
    QFETCH(bool, includeUpper);
    QFETCH(bool, upperVisible);
    QFETCH(int, groupOpacity);
    QFETCH(QByteArray, innerPixel);
    QFETCH(QByteArray, outerPixel);
    QFETCH(int, upperMask);
    const QUuid inner(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    const QUuid outer(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
    RasterImage source;
    source.size = {64, 48};
    source.layers = {{QStringLiteral("Lower"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}}};
    if (includeUpper) source.layers.append({QStringLiteral("Upper"), upperVisible,
        QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}});
    for (int index = 0; index < source.layers.size(); ++index) {
        const auto &pixel = index == 0 ? lower : upper;
        for (int channel = 0; channel < 4; ++channel) source.layers[index].defaultPixel[channel] = quint8(pixel[channel]);
        source.layers[index].parentUuid = inner;
        if (index == 1) {
            source.layers[index].opacity = 128;
            source.layers[index].projectionChannelMask = quint8(upperMask);
        }
    }
    source.layers.append({QStringLiteral("Inner"), true, inner, {}});
    source.layers.last().kind = LayerKind::Group;
    source.layers.last().parentUuid = outer;
    source.layers.last().opacity = quint8(groupOpacity);
    source.layers.append({QStringLiteral("Outer"), true, outer, {}});
    source.layers.last().kind = LayerKind::Group;
    OperationDocument document(source);
    const auto defaults = document.snapshot()->projectionDefaultPixels();
    const LayerId innerId = source.layers.size() - 1, outerId = source.layers.size();
    QVERIFY(defaults.contains(innerId) && defaults.contains(outerId));
    const auto innerValue = defaults.value(innerId), outerValue = defaults.value(outerId);
    QCOMPARE(rgbaBytes(innerValue, PixelFormat::Rgba8), innerPixel);
    QCOMPARE(rgbaBytes(outerValue, PixelFormat::Rgba8), outerPixel);
}

void KisGpuOperationDocumentTest::nestedRemovalOwnsDescendantsAndOneHistoryStep()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    QCOMPARE(document.removeLayers({2}), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.snapshot()->layers.first().id, LayerId(3));
    QCOMPARE(document.activeLayer(), LayerId(3));
    QCOMPARE(document.snapshot()->selectedLayers, QList<LayerId>{3});
    QCOMPARE(document.snapshot()->rasterBytes, qsizetype(0));
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.redo(), Result::Success);
}

void KisGpuOperationDocumentTest::nestedDuplicationCopiesSubtreePixelsAndIndependentParents()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    QCOMPARE(document.duplicateLayers({2}, {QStringLiteral("Inner copy")}, 2), Result::Success);
    const auto copied = document.snapshot();
    QCOMPARE(copied->layers.size(), 5);
    const auto leaf = std::find_if(copied->layers.cbegin(), copied->layers.cend(),
        [](const Layer &layer) { return layer.id == 4; });
    const auto group = std::find_if(copied->layers.cbegin(), copied->layers.cend(),
        [](const Layer &layer) { return layer.id == 5; });
    QVERIFY(leaf != copied->layers.cend());
    QVERIFY(group != copied->layers.cend());
    QCOMPARE(leaf->parentUuid, group->uuid);
    QCOMPARE(group->parentUuid, fixed->layers[2].uuid);
    QCOMPARE(group->name, QStringLiteral("Inner copy"));
    QCOMPARE(std::get<TileWriteSnapshot>(leaf->operations.first())->tiles.first(),
             std::get<TileWriteSnapshot>(fixed->layers[0].operations.first())->tiles.first());
    QCOMPARE(copied->rasterBytes, 2 * fixed->rasterBytes);
    QCOMPARE(document.activeLayer(), group->id);
    QCOMPARE(copied->selectedLayers, QList<LayerId>{group->id});
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.redo(), Result::Success);
    Limits limits;
    limits.layerCount = 4;
    OperationDocument bounded(nestedImage(), limits);
    const auto unchanged = bounded.preview();
    QCOMPARE(bounded.duplicateLayers({2}, {QStringLiteral("Copy")}, 2), Result::CapacityExceeded);
    QCOMPARE(bounded.preview(), unchanged);
}

void KisGpuOperationDocumentTest::nestedCanvasChangesKeepGroupsDerivedAndUndoable()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, true), Result::Success);
    for (const auto &layer : document.snapshot()->layers) {
        if (layer.kind == LayerKind::Group) {
            QVERIFY(layer.operations.isEmpty());
            QCOMPARE(layer.position, QPoint());
        } else QCOMPARE(layer.position, QPoint(-8, -6));
    }
}

void KisGpuOperationDocumentTest::groupPassThroughPreservesPinnedInputHistory()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    QCOMPARE(document.setLayerPassThrough(2, true), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::ChangePassThrough);
    QVERIFY(document.snapshot()->layers[1].passThrough);
    QVERIFY(!fixed->layers[1].passThrough);
    QCOMPARE(document.snapshot()->layers[0].operations, fixed->layers[0].operations);
    const auto unchanged = document.snapshot();
    QCOMPARE(document.setLayerPassThrough(2, true), Result::Success);
    QCOMPARE(document.snapshot(), unchanged);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    const auto preview = document.preview();
    const auto operation = std::get<PaintSnapshot>(preview->layers[0].operations.last());
    QCOMPARE(document.writeLayerPassThrough(3, true), Result::Success);
    QVERIFY(!preview->layers[2].passThrough);
    QVERIFY(document.preview()->layers[2].passThrough);
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers[0].operations.last()), operation);
}

void KisGpuOperationDocumentTest::layerBlendModesKeepPixelsHistoryPendingInput_data()
{
    QTest::addColumn<int>("blendMode");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply);
    QTest::newRow("screen") << int(LayerBlendMode::Screen);
    QTest::newRow("darken") << int(LayerBlendMode::Darken);
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten);
    QTest::newRow("add") << int(LayerBlendMode::Addition);
    QTest::newRow("linear_dodge") << int(LayerBlendMode::LinearDodge);
}

void KisGpuOperationDocumentTest::layerBlendModesKeepPixelsHistoryPendingInput()
{
    QFETCH(int, blendMode);
    const auto mode = LayerBlendMode(blendMode);
    auto input = nestedImage();
    input.layers[0].blendMode = mode;
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    QCOMPARE(fixed->layers[0].blendMode, mode);
    QCOMPARE(document.setLayerBlendMode(2, mode), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::ChangeBlendMode);
    QCOMPARE(fixed->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(document.snapshot()->layers[0].operations, fixed->layers[0].operations);
    const auto unchanged = document.snapshot();
    QCOMPARE(document.setLayerBlendMode(2, mode), Result::Success);
    QCOMPARE(document.snapshot(), unchanged);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    const auto pending = document.preview();
    QCOMPARE(document.writeLayerBlendMode(3, mode), Result::Success);
    QCOMPARE(document.activeOperation(), stroke);
    QCOMPARE(pending->layers[2].blendMode, LayerBlendMode::Normal);
    QCOMPARE(document.preview()->layers[2].blendMode, mode);
}

void KisGpuOperationDocumentTest::separableGroupDefaultsMatchNativeProjection()
{
    for (auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen, LayerBlendMode::Darken, LayerBlendMode::Lighten, LayerBlendMode::Addition, LayerBlendMode::LinearDodge}) {
        const auto references = mode == LayerBlendMode::Multiply ? LibrePaint::Image::Gpu::Test::multiplyProjectionReferences() :
            mode == LayerBlendMode::Screen ? LibrePaint::Image::Gpu::Test::screenProjectionReferences() :
            mode == LayerBlendMode::Darken ? LibrePaint::Image::Gpu::Test::darkenProjectionReferences() :
            mode == LayerBlendMode::Lighten ? LibrePaint::Image::Gpu::Test::lightenProjectionReferences() :
            LibrePaint::Image::Gpu::Test::additionProjectionReferences();
        qsizetype row = 0;
        for (int destinationAlpha : {0, 173, 255}) for (int sourceAlpha : {0, 137, 255})
        for (int opacity : {137, 255}) for (int mask : {15, 9, 6, 0}) {
            const auto group = QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
            RasterImage input;
            input.size = {256, 256};
            input.layers = {{QStringLiteral("Destination"), true, {}, {}},
                            {QStringLiteral("Source"), true, {}, {}}, {QStringLiteral("Group"), true, group, {}}};
            input.layers[0].defaultPixel = {37, 89, 151, quint8(destinationAlpha)};
            input.layers[1].defaultPixel = mode >= LayerBlendMode::Addition ? RgbaPixel{208, 224, 240, quint8(sourceAlpha)} :
                RgbaPixel{201, 41, 93, quint8(sourceAlpha)};
            input.layers[1].opacity = quint8(opacity);
            input.layers[1].blendMode = mode;
            input.layers[1].projectionChannelMask = quint8(mask);
            input.layers[0].parentUuid = input.layers[1].parentUuid = group;
            input.layers[2].kind = LayerKind::Group;
            OperationDocument document(input);
            const auto value = document.snapshot()->projectionDefaultPixels().value(3);
            QCOMPARE(rgbaBytes(value, PixelFormat::Rgba8), references.mid(row++ * 4, 4));
        }
    }
}

void KisGpuOperationDocumentTest::layerBlendModePreviewsAdoptAndCancelOneAttribute_data()
{
    QTest::addColumn<int>("blendMode");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply);
    QTest::newRow("screen") << int(LayerBlendMode::Screen);
    QTest::newRow("darken") << int(LayerBlendMode::Darken);
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten);
    QTest::newRow("add") << int(LayerBlendMode::Addition);
    QTest::newRow("linear_dodge") << int(LayerBlendMode::LinearDodge);
}

void KisGpuOperationDocumentTest::layerBlendModePreviewsAdoptAndCancelOneAttribute()
{
    QFETCH(int, blendMode);
    const auto mode = LayerBlendMode(blendMode);
    OperationDocument document(nestedImage());
    const auto before = document.snapshot();
    QCOMPARE(document.writeLayerBlendMode(2, mode), Result::Success);
    QCOMPARE(document.writeLayerOpacity(2, 137), Result::Success);
    LayerPropertiesSelection fields;
    fields.layer = 2;
    fields.blendMode = true;
    QCOMPARE(document.recordLayerProperties(before, document.snapshot(), {fields}), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(document.snapshot()->layers[1].opacity, quint8(137));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers[1].blendMode, mode);
    QCOMPARE(document.undo(), Result::Success);
    const auto opening = document.snapshot();
    QCOMPARE(document.writeLayerBlendMode(2, mode), Result::Success);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    QCOMPARE(document.restoreLayerProperties(opening, {fields}), Result::Success);
    QCOMPARE(document.preview()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(document.activeOperation(), stroke);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 1);
}

void KisGpuOperationDocumentTest::groupPassThroughPreviewsAdoptAndCancelOneAttribute()
{
    OperationDocument document(nestedImage());
    const auto before = document.snapshot();
    QCOMPARE(document.writeLayerPassThrough(2, true), Result::Success);
    QCOMPARE(document.writeLayerOpacity(2, 137), Result::Success);
    LayerPropertiesSelection fields;
    fields.layer = 2;
    fields.passThrough = true;
    QCOMPARE(document.recordLayerProperties(before, document.snapshot(), {fields}), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.snapshot()->layers[1].passThrough);
    QCOMPARE(document.snapshot()->layers[1].opacity, quint8(137));
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.snapshot()->layers[1].passThrough);
    QCOMPARE(document.undo(), Result::Success);
    const auto opening = document.snapshot();
    QCOMPARE(document.writeLayerPassThrough(2, true), Result::Success);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    QCOMPARE(document.restoreLayerProperties(opening, {fields}), Result::Success);
    QVERIFY(!document.preview()->layers[1].passThrough);
    QCOMPARE(document.activeOperation(), stroke);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 1);
    const auto retainedState = document.preview();
    fields.layer = 1;
    QCOMPARE(document.restoreLayerProperties(opening, {fields}), Result::InvalidInput);
    QCOMPARE(document.preview(), retainedState);
}

void KisGpuOperationDocumentTest::importedAndCopiedGroupsKeepPassThroughModes()
{
    auto input = nestedImage();
    input.layers[1].passThrough = true;
    OperationDocument document(input);
    QVERIFY(document.snapshot()->layers[1].passThrough);
    QCOMPARE(document.duplicateLayers({2}, {QStringLiteral("Copy")}, 2), Result::Success);
    int groups = 0;
    for (const auto &layer : document.snapshot()->layers)
        if (layer.kind == LayerKind::Group && layer.passThrough) ++groups;
    QCOMPARE(groups, 2);
    OperationDocument target(QSize(64, 48));
    const auto forest = insertionForest(document.snapshot());
    QCOMPARE(target.insertLayers(forest, 1), Result::Success);
    groups = 0;
    for (const auto &layer : target.snapshot()->layers)
        if (layer.kind == LayerKind::Group && layer.passThrough) ++groups;
    QCOMPARE(groups, 2);
    input.layers[0].passThrough = true;
    QVERIFY_EXCEPTION_THROWN(OperationDocument invalid(input), std::invalid_argument);
}

void KisGpuOperationDocumentTest::layerPropertyPreviewsBecomeOneReversibleEdit()
{
    // The properties dialog applies direct previews, then adopts the changed
    // attributes as one edit. Pixels and unrelated direct writes survive undo.
    OperationDocument document(rasterImage());
    const auto before = document.snapshot();
    QCOMPARE(document.writeLayerName(1, QStringLiteral("Renamed")), Result::Success);
    QCOMPARE(document.writeLayerOpacity(1, 89), Result::Success);
    QCOMPARE(document.writeLayerVisible(1, false), Result::Success);
    QCOMPARE(document.writeLayerLocked(1, true), Result::Success);
    QCOMPARE(document.writeLayerChannelMask(1, 5), Result::Success);
    QCOMPARE(document.writeLayerProjectionMask(1, 9), Result::Success);
    QCOMPARE(document.writeLayerColorLabel(1, 5), Result::Success);
    QCOMPARE(document.writeLayerOpacity(2, 197), Result::Success);
    const auto after = document.snapshot();
    QCOMPARE(document.undoCount(), 0);
    QVERIFY(!document.isModified());
    const QList<LayerPropertiesSelection> selection{{1, true, true, true, true, true, 15, 13},
                                                   {2, false, true, false, false, false, 0, 0}};
    QCOMPARE(document.recordLayerProperties(before, after, selection), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.redoCount(), 0);
    QVERIFY(document.isModified());
    QCOMPARE(document.historyAction(1), HistoryAction::ChangeLayerProperties);
    QCOMPARE(document.snapshot()->layers[0].operations, before->layers[0].operations);
    QCOMPARE(before->layers[0].name, QStringLiteral("Red"));
    QCOMPARE(after->layers[0].name, QStringLiteral("Renamed"));
    QCOMPARE(document.writeLayerName(1, QStringLiteral("Later direct name")), Result::Success);
    QCOMPARE(document.writeLayerColorLabel(2, 7), Result::Success);
    QCOMPARE(document.writeLayerPinnedToTimeline(1, true), Result::Success);
    QCOMPARE(document.writeLayerProjectionMask(1, 11), Result::Success);
}

void KisGpuOperationDocumentTest::layerPropertyCancellationKeepsExactValuesPendingInputAndRedo()
{
    auto input = rasterImage();
    input.layers[0].opacity = 137;
    OperationDocument document(input);
    const auto before = document.snapshot();
    QCOMPARE(document.selectLayer(1), Result::Success);
    QVERIFY(!paint(document, {sample(12, 13)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.writeLayerOpacity(1, 89), Result::Success);
    QCOMPARE(document.writeLayerProjectionMask(1, 9), Result::Success);
    const auto operation = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!operation.isNull());
    QCOMPARE(document.appendSamples(operation, {sample(20, 21)}), Result::Success);
    const auto pending = document.preview();
    const auto paintOperation = std::get<PaintSnapshot>(pending->layers[0].operations.last());
    const auto modified = document.isModified();
    const QList<LayerPropertiesSelection> selection{{1, false, true, false, false, false, 0, 13}};
    QCOMPARE(document.restoreLayerProperties(before, selection), Result::Success);
    QCOMPARE(document.snapshot()->layers[0].opacity, quint8(137));
    QCOMPARE(document.preview()->layers[0].opacity, quint8(137));
    QCOMPARE(document.snapshot()->layers[0].projectionChannelMask, quint8(13));
    QCOMPARE(document.preview()->layers[0].projectionChannelMask, quint8(13));
    QCOMPARE(pending->layers[0].opacity, quint8(89));
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers[0].operations.last()), paintOperation);
    QCOMPARE(document.activeOperation(), operation);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.isModified(), modified);
    const auto stateBeforeRequest = document.preview();
    OperationDocument foreign(input);
    QCOMPARE(document.restoreLayerProperties(foreign.snapshot(), selection), Result::StaleVersion);
    auto invalid = selection.first();
    invalid.projectionChannels = 16;
    QCOMPARE(document.restoreLayerProperties(before, {invalid}), Result::InvalidInput);
    QCOMPARE(document.preview(), stateBeforeRequest);
    document.close();
    QCOMPARE(document.restoreLayerProperties(before, selection), Result::Closed);
}

void KisGpuOperationDocumentTest::layerPropertyAcceptanceRetainsEditsCompletedDuringPreview()
{
    OperationDocument document(rasterImage());
    const auto before = document.snapshot();
    QCOMPARE(document.writeLayerOpacity(1, 89), Result::Success);
    QCOMPARE(document.selectLayer(1), Result::Success);
    QVERIFY(!paint(document, {sample(12, 13)}).isNull());
    QCOMPARE(document.moveLayer(1, 1), Result::Success);
    QVERIFY(document.addLayer(QStringLiteral("Other edit")));
    const auto after = document.snapshot();
    const int history = document.undoCount();
    const QList<LayerPropertiesSelection> selection{{1, false, true, false, false, false, 0, 0}};
    QCOMPARE(document.recordLayerProperties(before, after, selection), Result::Success);
    QCOMPARE(document.undoCount(), history + 1);
    QCOMPARE(document.undo(), Result::Success);
    const auto restored = document.snapshot();
    QCOMPARE(restored->layers.size(), after->layers.size());
    for (qsizetype index = 0; index < after->layers.size(); ++index) {
        QCOMPARE(restored->layers[index].id, after->layers[index].id);
        QCOMPARE(restored->layers[index].operations, after->layers[index].operations);
        QCOMPARE(restored->layers[index].parentUuid, after->layers[index].parentUuid);
        QCOMPARE(restored->layers[index].opacity, restored->layers[index].id == 1 ? quint8(255) : after->layers[index].opacity);
    }
    QCOMPARE(document.redo(), Result::Success);
    for (const auto &layer : document.snapshot()->layers)
        if (layer.id == 1) QCOMPARE(layer.opacity, quint8(89));
}

void KisGpuOperationDocumentTest::rejectedLayerPropertyPreviewsRetainHistoryAndInput()
{
    OperationDocument document(rasterImage());
    const auto before = document.snapshot();
    QCOMPARE(document.writeLayerOpacity(1, 89), Result::Success);
    const auto after = document.snapshot();
    const QList<LayerPropertiesSelection> selection{{1, false, true, false, false, false, 0, 0}};
    OperationDocument foreign(rasterImage());
    QCOMPARE(document.recordLayerProperties(foreign.snapshot(), after, selection), Result::StaleVersion);
    QCOMPARE(document.recordLayerProperties(before, foreign.snapshot(), selection), Result::StaleVersion);
    QCOMPARE(document.recordLayerProperties(before, after, {selection.first(), selection.first()}), Result::InvalidInput);
    auto invalid = selection.first();
    invalid.projectionChannels = 16;
    QCOMPARE(document.recordLayerProperties(before, after, {invalid}), Result::InvalidInput);
    invalid = selection.first();
    invalid.layer = 999;
    QCOMPARE(document.recordLayerProperties(before, after, {invalid}), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), after);
    QCOMPARE(document.undoCount(), 0);
    const auto id = document.beginStroke(1, Brush{}, 42);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {sample(4, 5)}), Result::Success);
    const auto pending = document.preview();
    QCOMPARE(document.recordLayerProperties(before, after, selection), Result::EditInProgress);
    QCOMPARE(document.preview(), pending);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.writeLayerOpacity(2, 77), Result::Success);
    const auto current = document.snapshot();
    QCOMPARE(document.recordLayerProperties(before, after, selection), Result::StaleVersion);
    QCOMPARE(document.snapshot(), current);
    QCOMPARE(document.undoCount(), 0);
    document.close();
    QCOMPARE(document.recordLayerProperties(before, current, selection), Result::Closed);
}

void KisGpuOperationDocumentTest::colorLabelsPreserveFixedVersionsPendingInput()
{
    RasterImage input;
    input.size = {64, 48};
    input.layers = {{QStringLiteral("Paint"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}},
                    {QStringLiteral("Group"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}}};
    input.layers[0].parentUuid = input.layers[1].uuid;
    input.layers[0].colorLabelIndex = 2;
    input.layers[1].kind = LayerKind::Group;
    input.layers[1].colorLabelIndex = 6;
    OperationDocument document(input);
    const auto fixed = document.snapshot();
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.writeLayerColorLabel(1, 5), Result::Success);
    QCOMPARE(document.writeLayerColorLabel(2, 7), Result::Success);
    QCOMPARE(fixed->layers[0].colorLabelIndex, 2);
    QCOMPARE(fixed->layers[1].colorLabelIndex, 6);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 1);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers[0].colorLabelIndex, 5);
    QCOMPARE(document.snapshot()->layers[1].colorLabelIndex, 7);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.writeLayerColorLabel(1, 3), Result::Success);
    QCOMPARE(preview->layers[0].colorLabelIndex, 5);
    QCOMPARE(document.preview()->layers[0].colorLabelIndex, 3);
}

void KisGpuOperationDocumentTest::projectionChannelsPreservePixelsPendingInputHistory()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    QCOMPARE(document.setLayerProjectionMask(2, 7, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.setLayerProjectionMask(2, 1, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::ChangeProjectionChannels);
    QCOMPARE(fixed->layers[1].projectionChannelMask, quint8(15));
    QCOMPARE(document.snapshot()->layers[1].projectionChannelMask, quint8(1));
    QCOMPARE(document.snapshot()->layers[0].operations, fixed->layers[0].operations);
    const auto stroke = document.beginStroke(1, Brush{}, 43);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::Success);
    const auto preview = document.preview();
    const auto operation = std::get<PaintSnapshot>(preview->layers[0].operations.last());
    QCOMPARE(operation->brush.channelWriteMask, quint8(15));
    QCOMPARE(document.writeLayerProjectionMask(2, 5), Result::Success);
    QCOMPARE(document.writeLayerProjectionMask(1, 9), Result::Success);
    QCOMPARE(preview->layers[1].projectionChannelMask, quint8(1));
    QCOMPARE(document.preview()->layers[1].projectionChannelMask, quint8(5));
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers[0].operations.last()), operation);
}

void KisGpuOperationDocumentTest::nestedLayerIdentitySurvivesPropertiesHistory()
{
    RasterImage source;
    source.size = {64, 48};
    source.layers = {
        {QStringLiteral("Nested paint"), true, QUuid(QStringLiteral("{01020304-0506-0708-090a-0b0c0d0e0f10}")), {}},
        {QStringLiteral("Inner group"), true, QUuid(QStringLiteral("{11121314-1516-1718-191a-1b1c1d1e1f20}")), {}},
        {QStringLiteral("Outer group"), true, QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}")), {}}};
    source.layers[0].parentUuid = source.layers[1].uuid;
    source.layers[1].kind = LayerKind::Group;
    source.layers[1].parentUuid = source.layers[2].uuid;
    source.layers[2].kind = LayerKind::Group;
    OperationDocument document(source);
    const auto fixed = document.snapshot();
    QCOMPARE(document.activeLayer(), LayerId(1));
    const auto beforeRejected = document.preview();
    QVERIFY(document.beginStroke(2, Brush{}, 41).isNull());
    const QList<RgbaTile> pixels{{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')}};
    QCOMPARE(document.replaceTiles(2, pixels), Result::LayerUnavailable);
    QCOMPARE(document.cropLayer(2, {1, 2, 16, 16}), Result::LayerUnavailable);
    QCOMPARE(document.clearLayers({2}), Result::LayerUnavailable);
    QVERIFY(document.beginLayerTranslation(2).isNull());
    QCOMPARE(document.writeLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.setLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.preview(), beforeRejected);
    QCOMPARE(document.writeLayerPosition(2, {7, -9}), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.snapshot()->layers[1].position, QPoint(7, -9));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint());
    QVERIFY(document.snapshot()->layers[1].operations.isEmpty());
    QCOMPARE(fixed->layers[1].position, QPoint());
    QCOMPARE(document.setLayerOpacity(3, 128), Result::Success);
    const auto stroke = document.beginStroke(1, Brush{}, 42);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(12, 9), sample(20, 15)}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(fixed->layers[2].opacity, quint8(255));
}

void KisGpuOperationDocumentTest::layerCropPreservesValuesHistory()
{
    RasterImage source;
    source.size = {512, 64};
    source.layers = {{QStringLiteral("Hidden locked target"), false, {}, {}}, {QStringLiteral("Active layer"), true, {}, {}}};
    source.layers.first().defaultPixel = {17, 34, 51, 73};
    source.layers.first().position = {17, -9};
    source.layers.first().userLocked = true;
    source.activeLayer = 1;
    OperationDocument document(source);
    const auto before = document.snapshot();
    const auto layer = before->layers.first().id;
    const QRect requested(256, 16, -259, -11);
    QCOMPARE(document.cropLayer(layer, requested), Result::Success);
    const auto cropped = document.snapshot();
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::CropLayer);
    QCOMPARE(document.historyCrop(1)->rect, requested);
    QCOMPARE(document.historyCrop(1)->layer, layer);
    QVERIFY(!document.historyCrop(0));
    QVERIFY(!document.historyCrop(2));
    QCOMPARE(cropped->activeLayer, before->activeLayer);
    QCOMPARE(cropped->selectedLayers, before->selectedLayers);
    QCOMPARE(cropped->layers.first().position, before->layers.first().position);
    QCOMPARE(cropped->layers.first().defaultPixel, before->layers.first().defaultPixel);
    QVERIFY(cropped->layers.first().userLocked);
    QVERIFY(!cropped->layers.first().visible);
    QVERIFY(before->layers.first().operations.isEmpty());
    QVERIFY(cropped->layers.last().operations.isEmpty());
    QCOMPARE(document.writeLayerName(layer, QStringLiteral("Renamed after crop")), Result::Success);
    QCOMPARE(document.historyAction(1), HistoryAction::CropLayer);
    QCOMPARE(document.historyCrop(1)->rect, requested);
    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
}

void KisGpuOperationDocumentTest::rejectedLayerCropsKeepRedoPendingInputAndClosedState()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.cropLayer(layer, QRect(1, 2, 3, 4)), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.cropLayer(layer, QRect(-RgbaTile::CoordinateLimit - 1, 0, 1, 1)), Result::InvalidInput);
    QCOMPARE(document.cropLayer(layer + 1, QRect(1, 2, 3, 4)), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.redoCount(), 1);
    const auto pending = document.beginStroke(layer, Brush{}, 17);
    QVERIFY(!pending.isNull());
    QCOMPARE(document.appendSamples(pending, {sample(8, 9)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.cropLayer(layer, QRect(1, 2, 3, 4)), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.activeOperation(), pending);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.cancelStroke(pending), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.historyCrop(1)->rect, QRect(1, 2, 3, 4));
    document.close();
    const auto closed = document.snapshot();
    QCOMPARE(document.cropLayer(layer, QRect(1, 2, 3, 4)), Result::Closed);
    QCOMPARE(document.snapshot(), closed);
}

void KisGpuOperationDocumentTest::invertColorsFreezesCoverageAndPreservesHistory()
{
    RasterImage source;
    source.size = {256, 256};
    RasterLayer target;
    target.name = QStringLiteral("Target");
    target.uuid = QUuid::createUuid();
    target.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    source.layers.append(target);
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    QCOMPARE(document.setSelectionEllipse(QRect(32, 32, 64, 48), true), Result::Success);
    QCOMPARE(document.invertColors(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->layer, layer);
    QCOMPARE(operation->kind, FilterKind::InvertColors);
    QCOMPARE(operation->version, quint64(1));
    QCOMPARE(operation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(operation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(operation->selectionInverted);
    QVERIFY(!operation->selectionMask);
    QVERIFY(!std::holds_alternative<FilterSnapshot>(before->layers.first().operations.last()));

    // Later selection edits do not rewrite the frozen filter coverage.
    QCOMPARE(document.setSelectionRect(QRect(1, 1, 2, 2)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(32, 32, 64, 48));

    // Duplication copies the filter with a fresh operation identity.
    QCOMPARE(document.duplicateLayers({layer}, {QStringLiteral("Copy")}, layer), Result::Success);
    const auto copied = document.snapshot()->layers.last();
    const auto copiedFilter = std::get<FilterSnapshot>(copied.operations.last());
    QCOMPARE(copiedFilter->layer, copied.id);
    QVERIFY(copiedFilter->id.sequence != operation->id.sequence);
    QCOMPARE(copiedFilter->selectionRect, operation->selectionRect);
    QCOMPARE(document.undo(), Result::Success);

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!std::holds_alternative<FilterSnapshot>(document.snapshot()->layers.first().operations.last()));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(32, 32, 64, 48));

    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::InvertColors);
    const auto retainedOperation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retainedOperation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(retainedOperation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(retainedOperation->selectionInverted);
    QCOMPARE(document.savedHistoryIndex(), 2);

    // Alpha (feathered) freehand coverage is retained by the operation and its
    // retained versions, including the fractional bytes.
    const QRect clip(QPoint(), QSize(32, 32));
    QByteArray coverage(32 * 32, '\0');
    coverage[8 * 32 + 8] = char(96);
    const auto soft = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(soft);
    RasterImage maskSource;
    maskSource.size = {32, 32};
    RasterLayer maskLayer;
    maskLayer.name = QStringLiteral("Masked");
    maskLayer.uuid = QUuid::createUuid();
    maskLayer.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    maskSource.layers.append(maskLayer);
    OperationDocument masked(maskSource);
    QCOMPARE(masked.setSelectionMask(soft), Result::Success);
    QCOMPARE(masked.invertColors(masked.activeLayer()), Result::Success);
    QVERIFY(std::get<FilterSnapshot>(masked.snapshot()->layers.first().operations.last())->selectionMask == soft);
    const auto maskOperation = std::get<FilterSnapshot>(masked.snapshot()->layers.first().operations.last());
    QVERIFY(maskOperation->selectionMask);
    QVERIFY(*maskOperation->selectionMask == *soft);
    QVERIFY(maskOperation->selectionMask->isAlpha());
    QCOMPARE(maskOperation->selectionMask->coverage(8, 8), quint8(96));
}

void KisGpuOperationDocumentTest::rejectedInvertColorsPreserveHistoryAndInput()
{
    RasterImage source;
    source.size = {64, 64};
    RasterLayer paint;
    paint.name = QStringLiteral("Paint");
    paint.uuid = QUuid::createUuid();
    RasterLayer locked;
    locked.name = QStringLiteral("Locked");
    locked.uuid = QUuid::createUuid();
    locked.userLocked = true;
    RasterLayer group;
    group.name = QStringLiteral("Group");
    group.uuid = QUuid::createUuid();
    group.kind = LayerKind::Group;
    source.layers = {paint, locked, group};
    OperationDocument document(source);
    const auto before = document.snapshot();
    const auto paintId = before->layers[0].id;
    const auto lockedId = before->layers[1].id;
    const auto groupId = before->layers[2].id;
    QCOMPARE(document.invertColors(lockedId), Result::LayerUnavailable);
    QCOMPARE(document.invertColors(groupId), Result::LayerUnavailable);
    QCOMPARE(document.invertColors(999), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.undoCount(), 0);
    const auto pending = document.beginStroke(paintId, Brush{}, 7);
    QVERIFY(!pending.isNull());
    QCOMPARE(document.appendSamples(pending, {{{8, 8}, 1}}), Result::Success);
    QCOMPARE(document.invertColors(paintId), Result::EditInProgress);
    QCOMPARE(document.cancelStroke(pending), Result::Success);
    QCOMPARE(document.snapshot(), before);
    document.close();
    QCOMPARE(document.invertColors(paintId), Result::Closed);
    QCOMPARE(document.snapshot(), before);
}

void KisGpuOperationDocumentTest::grayscaleFilterFreezesCoverageAndPreservesHistory()
{
    RasterImage source;
    source.size = {256, 256};
    RasterLayer target;
    target.name = QStringLiteral("Target");
    target.uuid = QUuid::createUuid();
    target.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    source.layers.append(target);
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    QCOMPARE(document.grayscale(999), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.setSelectionEllipse(QRect(32, 32, 64, 48), true), Result::Success);
    QCOMPARE(document.grayscale(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->layer, layer);
    QCOMPARE(operation->kind, FilterKind::Grayscale);
    QCOMPARE(operation->version, quint64(1));
    QCOMPARE(operation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(operation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(operation->selectionInverted);
    QVERIFY(!operation->selectionMask);
    QVERIFY(!std::holds_alternative<FilterSnapshot>(before->layers.first().operations.last()));

    // Later selection edits do not rewrite the frozen filter coverage.
    QCOMPARE(document.setSelectionRect(QRect(1, 1, 2, 2)), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect,
             QRect(32, 32, 64, 48));

    // One history entry per filter; the selection entries remain separate.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::Grayscale);
    QVERIFY(std::holds_alternative<FilterSnapshot>(document.snapshot()->layers.first().operations.last()));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!std::holds_alternative<FilterSnapshot>(document.snapshot()->layers.first().operations.last()));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->kind,
             FilterKind::Grayscale);

    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::Grayscale);
    const auto retainedOperation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retainedOperation->kind, FilterKind::Grayscale);
    QCOMPARE(retainedOperation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(retainedOperation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(retainedOperation->selectionInverted);
    QCOMPARE(document.savedHistoryIndex(), 2);

    // Alpha (feathered) freehand coverage freezes with the Grayscale kind.
    const QRect clip(QPoint(), QSize(32, 32));
    QByteArray coverage(32 * 32, '\0');
    coverage[8 * 32 + 8] = char(96);
    const auto soft = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(soft);
    RasterImage maskSource;
    maskSource.size = {32, 32};
    RasterLayer maskLayer;
    maskLayer.name = QStringLiteral("Masked");
    maskLayer.uuid = QUuid::createUuid();
    maskLayer.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    maskSource.layers.append(maskLayer);
    OperationDocument masked(maskSource);
    QCOMPARE(masked.setSelectionMask(soft), Result::Success);
    QCOMPARE(masked.grayscale(masked.activeLayer()), Result::Success);
    QCOMPARE(masked.historyAction(2), HistoryAction::Grayscale);
    QVERIFY(std::get<FilterSnapshot>(masked.snapshot()->layers.first().operations.last())->selectionMask == soft);
    const auto maskOperation = std::get<FilterSnapshot>(masked.snapshot()->layers.first().operations.last());
    QCOMPARE(maskOperation->kind, FilterKind::Grayscale);
    QVERIFY(maskOperation->selectionMask);
    QVERIFY(*maskOperation->selectionMask == *soft);
    QVERIFY(maskOperation->selectionMask->isAlpha());
    QCOMPARE(maskOperation->selectionMask->coverage(8, 8), quint8(96));
}

void KisGpuOperationDocumentTest::hsvAdjustmentFreezesSettingsAndPreservesHistory()
{
    RasterImage source;
    source.size = {256, 256};
    RasterLayer target;
    target.name = QStringLiteral("Target");
    target.uuid = QUuid::createUuid();
    target.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    source.layers.append(target);
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    QCOMPARE(document.setSelectionEllipse(QRect(32, 32, 64, 48), true), Result::Success);
    const HsvAdjustment settings{45, -20, 30, HsvAdjustmentType::Hsl, true};
    QCOMPARE(document.hsvAdjustment(layer, settings), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto operation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->layer, layer);
    QCOMPARE(operation->kind, FilterKind::HsvAdjustment);
    QCOMPARE(operation->version, quint64(1));
    QCOMPARE(operation->hue, 45);
    QCOMPARE(operation->saturation, -20);
    QCOMPARE(operation->value, 30);
    QCOMPARE(operation->hsvType, HsvAdjustmentType::Hsl);
    QVERIFY(operation->compatibilityMode);
    QCOMPARE(operation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(operation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(operation->selectionInverted);
    QVERIFY(!operation->selectionMask);

    // Later selection edits do not rewrite the frozen filter settings or coverage.
    QCOMPARE(document.setSelectionRect(QRect(1, 1, 2, 2)), Result::Success);
    const auto frozen = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(frozen->hue, 45);
    QCOMPARE(frozen->saturation, -20);
    QCOMPARE(frozen->value, 30);
    QCOMPARE(frozen->hsvType, HsvAdjustmentType::Hsl);
    QCOMPARE(frozen->selectionRect, QRect(32, 32, 64, 48));

    // Duplication copies the settings with a fresh operation identity.
    QCOMPARE(document.duplicateLayers({layer}, {QStringLiteral("Copy")}, layer), Result::Success);
    const auto copied = document.snapshot()->layers.last();
    const auto copiedFilter = std::get<FilterSnapshot>(copied.operations.last());
    QCOMPARE(copiedFilter->layer, copied.id);
    QVERIFY(copiedFilter->id.sequence != operation->id.sequence);
    QCOMPARE(copiedFilter->hue, 45);
    QCOMPARE(copiedFilter->hsvType, HsvAdjustmentType::Hsl);
    // Undo the duplication and the later selection; the filter itself remains
    // one entry. Further Undo removes it and the first selection. Redo walks
    // back to the filter state.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(std::holds_alternative<FilterSnapshot>(document.snapshot()->layers.first().operations.last()));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!std::holds_alternative<FilterSnapshot>(document.snapshot()->layers.first().operations.last()));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->hue, 45);

    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::HsvAdjustment);
    const auto retainedOperation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(retainedOperation->kind, FilterKind::HsvAdjustment);
    QCOMPARE(retainedOperation->hue, 45);
    QCOMPARE(retainedOperation->saturation, -20);
    QCOMPARE(retainedOperation->value, 30);
    QCOMPARE(retainedOperation->hsvType, HsvAdjustmentType::Hsl);
    QVERIFY(retainedOperation->compatibilityMode);
    QCOMPARE(retainedOperation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(document.savedHistoryIndex(), 2);

    // Alpha (feathered) freehand coverage is retained with the settings.
    const QRect clip(QPoint(), QSize(32, 32));
    QByteArray coverage(32 * 32, '\0');
    coverage[8 * 32 + 8] = char(96);
    const auto soft = SelectionMask::fromAlphaCoverage(clip, coverage, clip);
    QVERIFY(soft);
    RasterImage maskSource;
    maskSource.size = {32, 32};
    RasterLayer maskLayer;
    maskLayer.name = QStringLiteral("Masked");
    maskLayer.uuid = QUuid::createUuid();
    maskLayer.tiles.append({{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')});
    maskSource.layers.append(maskLayer);
    OperationDocument masked(maskSource);
    QCOMPARE(masked.setSelectionMask(soft), Result::Success);
    QCOMPARE(masked.hsvAdjustment(masked.activeLayer(), {10, 20, -30, HsvAdjustmentType::Hsv, false}),
             Result::Success);
    const auto maskOperation = std::get<FilterSnapshot>(masked.snapshot()->layers.first().operations.last());
    QVERIFY(maskOperation->selectionMask);
    QVERIFY(*maskOperation->selectionMask == *soft);
    QCOMPARE(maskOperation->hue, 10);
    QCOMPARE(maskOperation->saturation, 20);
    QCOMPARE(maskOperation->value, -30);
    QCOMPARE(maskOperation->hsvType, HsvAdjustmentType::Hsv);
    QVERIFY(!maskOperation->compatibilityMode);
}

void KisGpuOperationDocumentTest::hsvAdjustmentFreezesLumaDependentModesAndPreservesHistory()
{
    RasterImage source;
    source.size = {256, 256};
    RasterLayer target;
    target.name = QStringLiteral("Target");
    target.uuid = QUuid::createUuid();
    source.layers.append(target);
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();

    // The operation preserves the document color space's original coefficients
    // across admission and retained history.
    const std::array<qreal, 3> supplied{0.212612345678901, 0.715287654321099, 0.072199999999999};
    std::array<double, 3> luma{};
    QVERIFY(freezeLumaCoefficients(supplied, &luma));
    QVERIFY(luma == supplied);

    QCOMPARE(document.setSelectionEllipse(QRect(16, 16, 64, 48), true), Result::Success);
    HsvAdjustment hsy{30, 20, -10, HsvAdjustmentType::Hsy, false};
    hsy.luma = luma;
    hsy.lumaFrozen = true;
    QVERIFY(hsy.isValid());
    QCOMPARE(document.hsvAdjustment(layer, hsy), Result::Success);
    const auto hsyOperation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(hsyOperation->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(!hsyOperation->colorize);
    QVERIFY(hsyOperation->luma == luma);
    QCOMPARE(hsyOperation->selectionRect, QRect(16, 16, 64, 48));
    QVERIFY(hsyOperation->selectionInverted);

    // YUV always uses the frozen luma coefficients.
    HsvAdjustment yuv{40, 15, 5, HsvAdjustmentType::Yuv, false};
    yuv.luma = luma;
    yuv.lumaFrozen = true;
    QVERIFY(yuv.isValid());
    QCOMPARE(document.hsvAdjustment(layer, yuv), Result::Success);
    const auto yuvOperation = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(yuvOperation->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(!yuvOperation->colorize);
    QVERIFY(yuvOperation->luma == luma);

    // Colorize keeps the mode only for slider scaling and is frozen with the
    // coefficients its luminance rule consumes.
    HsvAdjustment colorize{120, 80, -20, HsvAdjustmentType::Hsv, false};
    colorize.colorize = true;
    colorize.luma = luma;
    colorize.lumaFrozen = true;
    QVERIFY(colorize.isValid());
    QCOMPARE(document.hsvAdjustment(layer, colorize), Result::Success);
    const auto colorizeOperation =
        std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(colorizeOperation->hsvType, HsvAdjustmentType::Hsv);
    QVERIFY(colorizeOperation->colorize);
    QVERIFY(colorizeOperation->luma == luma);

    // Later color-space or selection changes do not rewrite the frozen values.
    QCOMPARE(document.setSelectionRect(QRect(1, 1, 2, 2)), Result::Success);
    const auto frozen = std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(frozen->luma == luma);
    QVERIFY(frozen->colorize);
    QCOMPARE(frozen->selectionRect, QRect(16, 16, 64, 48));

    // Undo/Redo walk the three filter entries and retain their settings.
    for (int i = 0; i < 5; ++i) QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    for (int i = 0; i < 5; ++i) QCOMPARE(document.redo(), Result::Success);
    QVERIFY(std::get<FilterSnapshot>(document.snapshot()->layers.first().operations.last())->luma == luma);

    const auto retainedOperations = document.snapshot()->layers.first().operations;
    QCOMPARE(retainedOperations.size(), 3);
    QCOMPARE(std::get<FilterSnapshot>(retainedOperations[0])->hsvType, HsvAdjustmentType::Hsy);
    QVERIFY(std::get<FilterSnapshot>(retainedOperations[0])->luma == luma);
    QCOMPARE(std::get<FilterSnapshot>(retainedOperations[1])->hsvType, HsvAdjustmentType::Yuv);
    QVERIFY(std::get<FilterSnapshot>(retainedOperations[1])->luma == luma);
    QVERIFY(std::get<FilterSnapshot>(retainedOperations[2])->colorize);
    QVERIFY(std::get<FilterSnapshot>(retainedOperations[2])->luma == luma);
}

void KisGpuOperationDocumentTest::hsvAdjustmentRejectsUnfrozenLumaAndInvalidRanges()
{
    // The product rule falls back to Rec.709 when any document coefficient is
    // non-positive. Coefficients that cannot be used by the YUV division are
    // rejected instead of approximated.
    std::array<double, 3> luma{};
    QVERIFY(freezeLumaCoefficients({0.0, 0.5, 0.5}, &luma));
    QVERIFY(luma == (std::array<double, 3>{0.2126, 0.7152, 0.0722}));
    QVERIFY(freezeLumaCoefficients({0.25, -0.25, 1.0}, &luma));
    QVERIFY(luma == (std::array<double, 3>{0.2126, 0.7152, 0.0722}));
    QVERIFY(!freezeLumaCoefficients({0.5, 1.0, 0.5}, &luma));
    QVERIFY(!freezeLumaCoefficients({0.5, 0.25, 1.5}, &luma));
    QVERIFY(!freezeLumaCoefficients({std::numeric_limits<qreal>::quiet_NaN(), 0.5, 0.5}, &luma));
    QVERIFY(!freezeLumaCoefficients({0.5, 0.25, std::numeric_limits<qreal>::infinity()}, &luma));

    const std::array<double, 3> rec709{0.2126f, 0.7152f, 0.0722f};

    // Modes that use luma are invalid until the caller freezes coefficients.
    HsvAdjustment hsy{0, 50, 0, HsvAdjustmentType::Hsy, false};
    QVERIFY(!hsy.isValid());
    hsy.luma = rec709;
    hsy.lumaFrozen = true;
    QVERIFY(hsy.isValid());

    HsvAdjustment yuv{0, 0, 0, HsvAdjustmentType::Yuv, false};
    QVERIFY(!yuv.isValid());
    yuv.luma = rec709;
    yuv.lumaFrozen = true;
    QVERIFY(yuv.isValid());
    yuv.hue = 101;
    QVERIFY(!yuv.isValid());
    yuv.hue = -100;
    QVERIFY(yuv.isValid());

    HsvAdjustment colorize{0, 0, 0, HsvAdjustmentType::Hsv, false};
    colorize.colorize = true;
    QVERIFY(!colorize.isValid());
    colorize.luma = rec709;
    colorize.lumaFrozen = true;
    QVERIFY(colorize.isValid());
    colorize.hue = 360;
    QVERIFY(colorize.isValid());
    colorize.hue = 361;
    QVERIFY(!colorize.isValid());
    colorize.hue = 100;
    colorize.type = HsvAdjustmentType::Yuv;
    QVERIFY(colorize.isValid());
    colorize.hue = 101;
    QVERIFY(!colorize.isValid());
    colorize.hue = 50;
    colorize.saturation = -1;
    QVERIFY(!colorize.isValid());

    HsvAdjustment unsupported{0, 0, 0, HsvAdjustmentType(5), false};
    QVERIFY(!unsupported.isValid());

    // Rejected settings leave history and the accepted operation untouched.
    RasterImage source;
    source.size = {64, 64};
    RasterLayer paint;
    paint.name = QStringLiteral("Paint");
    paint.uuid = QUuid::createUuid();
    source.layers.append(paint);
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    QCOMPARE(document.hsvAdjustment(layer, {0, 0, 0, HsvAdjustmentType::Hsv, false}), Result::Success);
    const auto before = document.snapshot();
    const auto count = document.undoCount();
    HsvAdjustment unfrozenHsy{0, 50, 0, HsvAdjustmentType::Hsy, false};
    QCOMPARE(document.hsvAdjustment(layer, unfrozenHsy), Result::InvalidInput);
    unfrozenHsy.colorize = true;
    unfrozenHsy.type = HsvAdjustmentType::Hsv;
    QCOMPARE(document.hsvAdjustment(layer, unfrozenHsy), Result::InvalidInput);
    HsvAdjustment unfrozenYuv{0, 50, 0, HsvAdjustmentType::Yuv, false};
    QCOMPARE(document.hsvAdjustment(layer, unfrozenYuv), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.undoCount(), count);
}

void KisGpuOperationDocumentTest::hsvAdjustmentRejectsInvalidSettingsAndPreservesHistory()
{
    RasterImage source;
    source.size = {64, 64};
    RasterLayer paint;
    paint.name = QStringLiteral("Paint");
    paint.uuid = QUuid::createUuid();
    RasterLayer locked;
    locked.name = QStringLiteral("Locked");
    locked.uuid = QUuid::createUuid();
    locked.userLocked = true;
    RasterLayer group;
    group.name = QStringLiteral("Group");
    group.uuid = QUuid::createUuid();
    group.kind = LayerKind::Group;
    source.layers = {paint, locked, group};
    OperationDocument document(source);
    const auto before = document.snapshot();
    const auto paintId = before->layers[0].id;
    const auto lockedId = before->layers[1].id;
    const auto groupId = before->layers[2].id;

    // The product default configuration is zero h/s/v, type 1 (HSL) and the
    // modern (non-legacy) rule.
    const HsvAdjustment defaults;
    QCOMPARE(defaults.hue, 0);
    QCOMPARE(defaults.saturation, 0);
    QCOMPARE(defaults.value, 0);
    QCOMPARE(defaults.type, HsvAdjustmentType::Hsl);
    QVERIFY(!defaults.compatibilityMode);
    QVERIFY(defaults.isValid());

    // Product slider ranges are [-180, 180] and [-100, 100]; invalid enum
    // values are rejected, and luma-dependent modes require frozen luma.
    const HsvAdjustment invalid[] = {
        {181, 0, 0, HsvAdjustmentType::Hsv, false},
        {-181, 0, 0, HsvAdjustmentType::Hsv, false},
        {0, 101, 0, HsvAdjustmentType::Hsv, false},
        {0, -101, 0, HsvAdjustmentType::Hsv, false},
        {0, 0, 101, HsvAdjustmentType::Hsv, false},
        {0, 0, -101, HsvAdjustmentType::Hsv, false},
        {0, 0, 0, HsvAdjustmentType(5), false},
    };
    for (const auto &settings : invalid) {
        QVERIFY(!settings.isValid());
        QCOMPARE(document.hsvAdjustment(paintId, settings), Result::InvalidInput);
        QCOMPARE(document.snapshot(), before);
        QCOMPARE(document.undoCount(), 0);
    }

    QCOMPARE(document.hsvAdjustment(lockedId, {}), Result::LayerUnavailable);
    QCOMPARE(document.hsvAdjustment(groupId, {}), Result::LayerUnavailable);
    QCOMPARE(document.hsvAdjustment(999, {}), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);

    const auto pending = document.beginStroke(paintId, Brush{}, 7);
    QVERIFY(!pending.isNull());
    QCOMPARE(document.appendSamples(pending, {{{8, 8}, 1}}), Result::Success);
    QCOMPARE(document.hsvAdjustment(paintId, {}), Result::EditInProgress);
    QCOMPARE(document.cancelStroke(pending), Result::Success);
    QCOMPARE(document.snapshot(), before);
    document.close();
    QCOMPARE(document.hsvAdjustment(paintId, {}), Result::Closed);
    QCOMPARE(document.snapshot(), before);

    // Product boundary values are accepted and frozen verbatim.
    OperationDocument bounds({64, 64});
    const auto boundsLayer = bounds.activeLayer();
    QCOMPARE(bounds.hsvAdjustment(boundsLayer, {-180, -100, -100, HsvAdjustmentType::Hsv, true}),
             Result::Success);
    QCOMPARE(bounds.hsvAdjustment(boundsLayer, {180, 100, 100, HsvAdjustmentType::Hsi, false}),
             Result::Success);
    QCOMPARE(bounds.undoCount(), 2);
    auto operations = bounds.snapshot()->layers.first().operations;
    QCOMPARE(std::get<FilterSnapshot>(operations[0])->hue, -180);
    QCOMPARE(std::get<FilterSnapshot>(operations[0])->saturation, -100);
    QCOMPARE(std::get<FilterSnapshot>(operations[0])->value, -100);
    QVERIFY(std::get<FilterSnapshot>(operations[0])->compatibilityMode);
    QCOMPARE(std::get<FilterSnapshot>(operations[1])->hue, 180);
    QCOMPARE(std::get<FilterSnapshot>(operations[1])->hsvType, HsvAdjustmentType::Hsi);
    QVERIFY(!std::get<FilterSnapshot>(operations[1])->compatibilityMode);
}

void KisGpuOperationDocumentTest::layerTranslationPreservesInputVersionsAndOneHistoryEntry()
{
    const QPoint initial(7, -3), offset(13, -7), direct(-12, 17);
    RasterImage input;
    input.size = {64, 64};
    input.layers = {{QStringLiteral("Moved layer"), true, {}, {}}};
    input.layers.first().position = initial;
    OperationDocument document(input);
    const auto layer = document.activeLayer();
    const QList<InputSample> samples{sample(12, 9, 0.5, 1), sample(14, 10, 1, 2)};
    QVERIFY(!paint(document, samples).isNull());
    const auto before = document.snapshot();
    QCOMPARE(document.markSaved(before), Result::Success);
    auto move = document.beginLayerTranslation(layer);
    QVERIFY(!move.isNull());
    QCOMPARE(document.activeOperation(), move);
    QCOMPARE(document.updateLayerTranslation(move, offset), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(preview->layers.first().position, initial + offset);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(std::get<PaintSnapshot>(preview->layers.first().operations.first())->inputSamples(), samples);
    QCOMPARE(document.writeLayerPosition(layer, direct), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelLayerTranslation(move), Result::Success);
    QCOMPARE(document.preview(), before);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(!document.isModified());
    QCOMPARE(document.updateLayerTranslation(move, offset), Result::StaleOperation);
    move = document.beginLayerTranslation(layer);
    QVERIFY(!move.isNull());
    QCOMPARE(document.updateLayerTranslation(move, offset), Result::Success);
    const auto moved = document.preview();
    QCOMPARE(document.commitLayerTranslation(move), Result::Success);
    QCOMPARE(document.snapshot(), moved);
    QVERIFY(moved->isCommitted());
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::TranslateLayer);
    QCOMPARE(document.markSaved(moved), Result::Success);
    QCOMPARE(document.writeLayerPosition(layer, direct), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, direct);
    QCOMPARE(document.undoCount(), 2);
    QVERIFY(!document.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, initial + offset);
    QCOMPARE(before->layers.first().position, initial);
    QCOMPARE(moved->layers.first().position, initial + offset);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.first())->inputSamples(), samples);
}

void KisGpuOperationDocumentTest::layerReorderingIsAtomicAndPreservesSelectionPixelsAndHistory()
{
    OperationDocument document({16, 16});
    const auto bottom = document.activeLayer();
    const auto middle = document.addLayer(QStringLiteral("Middle"));
    const auto top = document.addLayer(QStringLiteral("Top"));
    const RgbaTile pixels{{0, 0}, QByteArray(RgbaTile::ByteCount, '\x11')};
    QCOMPARE(document.replaceTiles(middle, {pixels}), Result::Success);
    QCOMPARE(document.selectLayer(middle), Result::Success);
    const auto pinned = document.snapshot();
    const auto before = document.undoCount();
    QCOMPARE(document.markSaved(pinned), Result::Success);
    QCOMPARE(document.reorderLayers({top, bottom}), Result::InvalidInput);
    QCOMPARE(document.reorderLayers({top, bottom, bottom}), Result::InvalidInput);
    QCOMPARE(document.reorderLayers({top, bottom, LayerId(9999)}), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), pinned);
    QCOMPARE(document.reorderLayers({bottom, middle, top}), Result::Success);
    QCOMPARE(document.snapshot(), pinned);
    QVERIFY(!document.isModified());
    QCOMPARE(document.reorderLayers({top, bottom, middle}), Result::Success);
    QCOMPARE(document.undoCount(), before + 1);
    QCOMPARE(document.historyAction(before + 1), HistoryAction::MoveLayer);
    QCOMPARE(document.activeLayer(), middle);
    QCOMPARE(document.snapshot()->layers[0].id, top);
    QCOMPARE(document.snapshot()->layers[1].id, bottom);
    QCOMPARE(document.snapshot()->layers[2].id, middle);
    QCOMPARE(document.snapshot()->layers[2].operations, pinned->layers[1].operations);
    QCOMPARE(document.snapshot()->rasterBytes, pinned->rasterBytes);
    QCOMPARE(pinned->layers[0].id, bottom);
    QCOMPARE(pinned->layers[1].id, middle);
    QCOMPARE(pinned->layers[2].id, top);
    QCOMPARE(document.snapshot()->layers[0].id, top);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::MoveLayer);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), pinned);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    const auto operation = document.beginStroke(middle, Brush{}, 0);
    QVERIFY(!operation.isNull());
    QCOMPARE(document.reorderLayers({bottom, middle, top}), Result::EditInProgress);
    QCOMPARE(document.cancelStroke(operation), Result::Success);
    QCOMPARE(document.snapshot()->layers[0].id, top);
    document.close();
    QCOMPARE(document.reorderLayers({bottom, middle, top}), Result::Closed);
}

void KisGpuOperationDocumentTest::mergedPropertiesPreservePinsSaveBoundaries()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128, PropertyHistory::MergeConsecutive), Result::Success);
    const auto pinned = document.snapshot();
    QCOMPARE(document.setLayerOpacity(layer, 64, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(pinned->layers.first().opacity, quint8(128));
    QCOMPARE(document.markSaved(pinned), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(document.savedHistoryIndex(), -1);
}

void KisGpuOperationDocumentTest::mergedNameResetRetainsItsRecordedBoundary()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot()->layers.first().name;
    QCOMPARE(document.renameLayer(layer, QStringLiteral("Adjusted"), PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.renameLayer(layer, initial, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.writeLayerName(layer, QStringLiteral("Direct")), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().name, QStringLiteral("Direct"));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().name, initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().name, initial);
}

void KisGpuOperationDocumentTest::groupedRemovalPreservesPixelsSelectionAndAtomicHistory()
{
    OperationDocument document(rasterImage());
    const auto top = document.addLayer(QStringLiteral("Top"));
    QVERIFY(top);
    QVERIFY(!paint(document, {sample(5, 5), sample(12, 12)}).isNull());
    const auto pinned = document.snapshot();
    const auto bottom = pinned->layers.first().id;
    const auto middle = pinned->layers[1].id;
    const auto before = document.undoCount();
    QCOMPARE(document.removeLayers({bottom, LayerId(999999)}), Result::LayerUnavailable);
    QCOMPARE(document.removeLayers({bottom, bottom}), Result::InvalidInput);
    QCOMPARE(document.snapshot(), pinned);
    QCOMPARE(document.undoCount(), before);
    QCOMPARE(document.removeLayers({bottom, top}), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.activeLayer(), middle);
    QCOMPARE(document.snapshot()->rasterBytes, qsizetype(RgbaTile::ByteCount));
    QCOMPARE(document.snapshot()->sampleCount, 0);
    QCOMPARE(document.undoCount(), before + 1);
    QCOMPARE(document.historyAction(before + 1), HistoryAction::RemoveLayer);
    const auto removed = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), pinned);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), removed);
    QCOMPARE(document.removeLayers({middle}, QStringLiteral("Replacement")), Result::Success);
    const auto replacement = document.snapshot();
    QCOMPARE(replacement->layers.size(), 1);
    QVERIFY(replacement->layers.first().id != middle);
    QCOMPARE(replacement->layers.first().name, QStringLiteral("Replacement"));
    QVERIFY(replacement->layers.first().operations.isEmpty());
    QCOMPARE(replacement->rasterBytes, 0);
    QCOMPARE(replacement->sampleCount, 0);
    QCOMPARE(document.activeLayer(), replacement->layers.first().id);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::RemoveLayer);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), removed);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), replacement);
}

void KisGpuOperationDocumentTest::insertedLayersPreserveOrderPinsAndOneHistoryStep()
{
    OperationDocument document(rasterImage());
    const auto original = document.snapshot();
    const auto id = document.addLayer(QStringLiteral("Inserted"), 1, true);
    QVERIFY(id);
    const auto inserted = document.snapshot();
    QCOMPARE(inserted->layers.size(), 3);
    QCOMPARE(inserted->layers[0].uuid, original->layers[0].uuid);
    QCOMPARE(inserted->layers[2].uuid, original->layers[1].uuid);
    QCOMPARE(inserted->layers[1].id, id);
    QVERIFY(inserted->layers[1].pinnedToTimeline);
    QCOMPARE(document.activeLayer(), id);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    const auto beforeInvalid = document.preview();
    QCOMPARE(document.addLayer(QStringLiteral("Invalid"), -1, false), LayerId(0));
    QCOMPARE(document.addLayer(QStringLiteral("Invalid"), 3, false), LayerId(0));
    QCOMPARE(document.preview(), beforeInvalid);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), inserted);
}

void KisGpuOperationDocumentTest::nestedPaintInsertionRejectsMissingAndNonGroupParents()
{
    OperationDocument document(nestedImage());
    const auto original = document.snapshot();
    const auto id = document.addLayer(QStringLiteral("Child"), 1, false, original->layers[1].uuid);
    QVERIFY(id);
    QCOMPARE(document.snapshot()->layers[1].parentUuid, original->layers[1].uuid);
    QCOMPARE(document.undo(), Result::Success);
    const auto beforeInvalid = document.preview();
    QCOMPARE(document.addLayer(QStringLiteral("Missing parent"), 1, false,
                              QUuid(QStringLiteral("{deadbeef-735b-47cf-93c5-7893d04916f0}"))), LayerId(0));
    QCOMPARE(document.addLayer(QStringLiteral("Paint parent"), 1, false, original->layers[0].uuid), LayerId(0));
    QCOMPARE(document.preview(), beforeInvalid);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), id);
}

void KisGpuOperationDocumentTest::emptyGroupInsertionPreservesHierarchyHistoryAndRejectsSubtrees()
{
    RasterImage input;
    input.size = {64, 32};
    RasterLayer group{QStringLiteral("Source group"), true,
        QUuid(QStringLiteral("{16171819-2021-2223-2425-262728293031}")), {}};
    group.kind = LayerKind::Group;
    input.layers = {group};
    OperationDocument source(input);
    const auto sourceVersion = source.snapshot();
    OperationDocument destination(nestedImage());
    const auto original = destination.snapshot();
    RasterLayer properties = group;
    properties.name = QStringLiteral("Inserted group");
    properties.uuid = QUuid(QStringLiteral("{41424344-4546-4748-4950-515253545556}"));
    properties.parentUuid = original->layers[1].uuid;
    properties.opacity = 173;
    properties.visible = false;
    properties.userLocked = true;
    properties.pinnedToTimeline = true;
    const auto id = destination.insertLayer(source, source.activeLayer(), properties, 1);
    QVERIFY(id);
    const auto inserted = destination.snapshot();
    const auto &created = inserted->layers[1];
    QCOMPARE(created.id, id);
    QCOMPARE(created.kind, LayerKind::Group);
    QCOMPARE(created.uuid, properties.uuid);
    QCOMPARE(created.parentUuid, properties.parentUuid);
    QCOMPARE(created.name, properties.name);
    QCOMPARE(created.opacity, properties.opacity);
    QVERIFY(!created.visible);
    QVERIFY(created.userLocked);
    QVERIFY(created.pinnedToTimeline);
    QVERIFY(created.operations.isEmpty());
    QCOMPARE(inserted->sampleCount, original->sampleCount);
    QCOMPARE(inserted->rasterBytes, original->rasterBytes);
    QCOMPARE(destination.activeLayer(), id);
    QCOMPARE(inserted->selectedLayers, QList<LayerId>{id});
    QCOMPARE(destination.undoCount(), 1);
    QCOMPARE(destination.undo(), Result::Success);
    QCOMPARE(destination.snapshot(), original);
    const auto beforeRejected = destination.preview();
    RasterLayer child{QStringLiteral("Child"), true,
        QUuid(QStringLiteral("{61626364-6566-6768-6970-717273747576}")), {}};
    child.parentUuid = group.uuid;
    input.layers.append(child);
    OperationDocument subtree(input);
    auto rejected = properties;
    rejected.uuid = QUuid(QStringLiteral("{81828384-8586-8788-8990-919293949596}"));
    QCOMPARE(destination.insertLayer(subtree, 1, rejected, 1), LayerId(0));
    rejected.kind = LayerKind::Paint;
    QCOMPARE(destination.insertLayer(source, source.activeLayer(), rejected, 1), LayerId(0));
    rejected.kind = LayerKind::Group;
    rejected.channelWriteMask = 7;
    QCOMPARE(destination.insertLayer(source, source.activeLayer(), rejected, 1), LayerId(0));
    QCOMPARE(destination.preview(), beforeRejected);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.snapshot(), inserted);
    QCOMPARE(destination.snapshot()->layers[1].kind, LayerKind::Group);
    QCOMPARE(destination.snapshot()->layers[1].uuid, properties.uuid);
    QCOMPARE(destination.snapshot()->layers[1].parentUuid, properties.parentUuid);
    QCOMPARE(source.snapshot(), sourceVersion);
}

void KisGpuOperationDocumentTest::insertedForeignLayersSharePixelsAndInputsWithAtomicHistory()
{
    OperationDocument source(rasterImage());
    QCOMPARE(source.selectLayer(1), Result::Success);
    QVERIFY(!paint(source, {sample(8, 9), sample(12, 13, 0.5)}).isNull());
    const auto sourceVersion = source.snapshot();
    const auto &original = sourceVersion->layers[0];
    RasterLayer properties;
    properties.name = QStringLiteral("Inserted");
    properties.uuid = QUuid(QStringLiteral("{21222324-2526-2728-292a-2b2c2d2e2f30}"));
    properties.defaultPixel = original.defaultPixel;
    properties.opacity = 173;
    properties.visible = false;
    properties.userLocked = true;
    properties.channelWriteMask = 7;
    RasterImage destinationInput;
    destinationInput.size = {64, 32};
    destinationInput.layers = {{QStringLiteral("Destination"), true,
        QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}")), {}}};
    OperationDocument destination(destinationInput);
    const auto before = destination.snapshot();
    const auto id = destination.insertLayer(source, 1, properties, 0);
    QVERIFY(id);
    const auto inserted = destination.snapshot();
    const auto &copy = inserted->layers[0];
    QCOMPARE(copy.id, id);
    QCOMPARE(copy.uuid, properties.uuid);
    QCOMPARE(copy.name, properties.name);
    QCOMPARE(copy.defaultPixel, original.defaultPixel);
    QCOMPARE(copy.opacity, properties.opacity);
    QVERIFY(!copy.visible);
    QVERIFY(copy.userLocked);
    QCOMPARE(copy.channelWriteMask, quint8(7));
    QCOMPARE(copy.operations.size(), original.operations.size());
    const auto pixels = std::get<TileWriteSnapshot>(copy.operations[0]);
    const auto input = std::get<PaintSnapshot>(copy.operations[1]);
    QCOMPARE(pixels->tiles, std::get<TileWriteSnapshot>(original.operations[0])->tiles);
    QCOMPARE(input->sampleBlocks, std::get<PaintSnapshot>(original.operations[1])->sampleBlocks);
    QCOMPARE(input->id.document, inserted->documentId());
    QVERIFY(input->id != std::get<PaintSnapshot>(original.operations[1])->id);
    QCOMPARE(input->layer, id);
    QCOMPARE(inserted->sampleCount, sourceVersion->sampleCount);
    QCOMPARE(inserted->rasterBytes, qsizetype(RgbaTile::ByteCount));
    QCOMPARE(destination.undoCount(), 1);
    QCOMPARE(destination.undo(), Result::Success);
    QCOMPARE(destination.snapshot(), before);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.snapshot(), inserted);
    QCOMPARE(destination.snapshot()->layers[0].uuid, properties.uuid);
    QCOMPARE(source.snapshot(), sourceVersion);
    const auto fixed = destination.preview();
    QCOMPARE(destination.insertLayer(source, 999, properties, 0), LayerId(0));
    QCOMPARE(destination.insertLayer(source, 1, properties, 0), LayerId(0));
    QCOMPARE(destination.preview(), fixed);
    Limits limits;
    limits.layerCount = 1;
    OperationDocument bounded({64, 32}, limits);
    const auto boundedVersion = bounded.snapshot();
    QCOMPARE(bounded.insertLayer(source, 1, properties, 0), LayerId(0));
    QCOMPARE(bounded.snapshot(), boundedVersion);
    for (int budget = 0; budget < 3; ++budget) {
        Limits capacity;
        if (budget == 0) capacity.rasterBytes = RgbaTile::ByteCount - 1;
        if (budget == 1) capacity.samplesPerDocument = 1;
        if (budget == 2) capacity.samplesPerStroke = 1;
        OperationDocument limited({64, 32}, capacity);
        const auto retained = limited.preview();
        QCOMPARE(limited.insertLayer(source, 1, properties, 0), LayerId(0));
        QCOMPARE(limited.preview(), retained);
    }
}

void KisGpuOperationDocumentTest::activeLayerAndSelectionRemainIndependentAndInput()
{
    OperationDocument document(rasterImage());
    const LayerId first = 1;
    const LayerId second = 2;
    const auto saved = document.snapshot();
    QCOMPARE(document.markSaved(saved), Result::Success);
    QCOMPARE(document.selectLayers({second}, first), Result::Success);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.snapshot()->selectedLayers, QList<LayerId>{second});
    QCOMPARE(saved->activeLayer, second);
    QCOMPARE(document.undoCount(), 0);
    QVERIFY(!document.isModified());
    const auto selected = document.snapshot();
    const auto stroke = document.beginStroke(first, {}, 17);
    QCOMPARE(document.appendSamples(stroke, {sample(8, 9)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.selectLayers({second, first}, first), Result::Success);
    QCOMPARE(document.preview()->selectedLayers, (QList<LayerId>{first, second}));
    QCOMPARE(document.preview()->pendingOperation(), stroke);
    QCOMPARE(preview->selectedLayers, QList<LayerId>{second});
    QCOMPARE(selected->selectedLayers, QList<LayerId>{second});
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.selectLayers({second}, first), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.snapshot()->selectedLayers, (QList<LayerId>{first, second}));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.snapshot()->selectedLayers, QList<LayerId>{second});
}

void KisGpuOperationDocumentTest::selectionDuringInputPreservesPaintingAndHistory()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Other"));
    QVERIFY(second);
    QCOMPARE(document.selectLayer(first), Result::Success);
    const auto saved = document.snapshot();
    QCOMPARE(document.markSaved(saved), Result::Success);
    const auto id = document.beginStroke(first, {}, 17);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {sample(8, 9)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.selectLayer(second), Result::Success);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.preview()->activeLayer, second);
    QCOMPARE(preview->activeLayer, first);
    QCOMPARE(saved->activeLayer, first);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(!document.isModified());
    QCOMPARE(document.preview()->pendingOperation(), id);
    QCOMPARE(document.appendSamples(id, {sample(12, 13)}), Result::Success);
    QCOMPARE(document.preview()->activeLayer, second);
    QCOMPARE(document.preview()->layers[0].operations.size(), 1);
    QVERIFY(document.preview()->layers[1].operations.empty());
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.activeLayer(), second);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    const auto committed = document.snapshot();
    const auto cancelled = document.beginStroke(first, {}, 18);
    QVERIFY(!cancelled.isNull());
    QCOMPARE(document.selectLayer(first), Result::Success);
    QCOMPARE(document.cancelStroke(cancelled), Result::Success);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(committed->activeLayer, second);
    const auto beforeInvalid = document.preview();
    QCOMPARE(document.selectLayer(0), Result::LayerUnavailable);
    QCOMPARE(document.preview(), beforeInvalid);
    document.close();
    QCOMPARE(document.selectLayer(first), Result::Closed);
}

void KisGpuOperationDocumentTest::mergedPanelTogglesPreservePinsAndSaveBoundaries_data()
{
    QTest::addColumn<bool>("locked");
    QTest::newRow("visibility") << false;
    QTest::newRow("lock") << true;
}

void KisGpuOperationDocumentTest::mergedPanelTogglesPreservePinsAndSaveBoundaries()
{
    QFETCH(bool, locked);
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Other"));
    QVERIFY(second);
    QCOMPARE(document.selectLayer(first), Result::Success);
    const auto saved = document.snapshot();
    QCOMPARE(document.markSaved(saved), Result::Success);
    const auto set = [&](OperationDocument &target, bool enabled, PropertyHistory history) {
        return locked ? target.setLayerLocked(first, enabled, history)
                      : target.setLayerVisible(first, !enabled, history);
    };
    const auto state = [&](const Snapshot &version) {
        return locked ? version->layers.first().userLocked : !version->layers.first().visible;
    };
    QCOMPARE(set(document, true, PropertyHistory::MergeConsecutive), Result::Success);
    const auto changed = document.snapshot();
    QCOMPARE(document.selectLayer(second), Result::Success);
    QCOMPARE(document.writeLayerName(first, QStringLiteral("Direct")), Result::Success);
    QCOMPARE(set(document, false, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.snapshot()->layers.first().name, QStringLiteral("Direct"));
    QVERIFY(!document.isModified());
    QVERIFY(!state(saved));
    QVERIFY(state(changed));
    QCOMPARE(set(document, true, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    QCOMPARE(set(document, false, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.savedHistoryIndex(), 2);
    QVERIFY(document.isModified());
    const auto beforeInvalid = document.preview();
    QCOMPARE(set(document, true, PropertyHistory(-1)), Result::InvalidInput);
    QCOMPARE(document.preview(), beforeInvalid);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(state(document.snapshot()));
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!state(document.snapshot()));
    const auto action = locked ? HistoryAction::ChangeLock : HistoryAction::ChangeVisibility;
    QCOMPARE(document.historyAction(2), action);
    QCOMPARE(document.historyAction(3), action);
}

void KisGpuOperationDocumentTest::historyActionsSurviveDirectWritesMerging()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    const auto name = document.snapshot()->layers.first().name;
    QCOMPARE(document.historyAction(0), HistoryAction::None);
    QCOMPARE(document.renameLayer(layer, QStringLiteral("Temporary"), PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.renameLayer(layer, name, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.writeLayerName(layer, QStringLiteral("Direct name")), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.setLayerVisible(layer, false), Result::Success);
    QCOMPARE(document.writeLayerVisible(layer, true), Result::Success);
    QCOMPARE(document.setLayerLocked(layer, true), Result::Success);
    QCOMPARE(document.writeLayerLocked(layer, false), Result::Success);
    const QList<HistoryAction> expected{HistoryAction::None, HistoryAction::RenameLayer,
        HistoryAction::ChangeOpacity, HistoryAction::ChangeVisibility, HistoryAction::ChangeLock};
}

void KisGpuOperationDocumentTest::mergedOpacityResetPreservesSelectionAndDirectWrites()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Other"));
    QVERIFY(second);
    QCOMPARE(document.selectLayer(first), Result::Success);
    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    const auto baseline = document.snapshot();
    QCOMPARE(document.setLayerOpacity(first, 128, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.selectLayer(second), Result::Success);
    QCOMPARE(document.writeLayerName(first, QStringLiteral("Direct")), Result::Success);
    QCOMPARE(document.setLayerOpacity(first, 255, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.snapshot()->layers.first().name, QStringLiteral("Direct"));
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(255));
    QVERIFY(!document.isModified());
    QCOMPARE(baseline->activeLayer, first);
    QCOMPARE(baseline->layers.first().name, QStringLiteral("Paint Layer 1"));
}

void KisGpuOperationDocumentTest::channelProtectionPreservesFrozenSettingsHistory()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();
    QCOMPARE(document.setLayerChannelMask(layer, 7, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.setLayerChannelMask(layer, 1, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::ChangeChannelLock);
    QCOMPARE(initial->layers.first().channelWriteMask, quint8(15));
    QCOMPARE(document.snapshot()->layers.first().channelWriteMask, quint8(1));
    const auto stroke = document.beginStroke(layer, Brush{}, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    const auto preview = document.preview();
    const auto paint = std::get<PaintSnapshot>(preview->layers.first().operations.last());
    QCOMPARE(paint->brush.channelWriteMask, quint8(1));
    QVERIFY(paint->brush.alphaLocked);
    QCOMPARE(document.writeLayerChannelMask(layer, 7), Result::Success);
    QCOMPARE(preview->layers.first().channelWriteMask, quint8(1));
    QCOMPARE(document.preview()->layers.first().channelWriteMask, quint8(7));
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->brush, paint->brush);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.setLayerChannelMask(layer, 16), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    Brush invalid;
    invalid.channelWriteMask = 16;
    QVERIFY(document.beginStroke(layer, invalid, 0).isNull());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().channelWriteMask, quint8(15));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().channelWriteMask, quint8(1));
    QCOMPARE(document.setLayerChannelMask(layer, 15, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::identicalLayerDuplicationsPreserveTheSamePersistentIdentities()
{
    OperationDocument first(rasterImage()), second(rasterImage());
    const QList<QString> names{QStringLiteral("Red copy"), QStringLiteral("Blue copy")};
    QCOMPARE(first.duplicateLayers({1, 2}, names, 1), Result::Success);
    QCOMPARE(second.duplicateLayers({1, 2}, names, 1), Result::Success);
    for (int i = 0; i < first.snapshot()->layers.size(); ++i) {
        QCOMPARE(first.snapshot()->layers[i].uuid, second.snapshot()->layers[i].uuid);
    }
    const auto firstCopy = first.snapshot()->layers[2].uuid;
    auto collisionImage = rasterImage();
    collisionImage.layers[1].uuid = firstCopy;
    OperationDocument collision(collisionImage);
    QCOMPARE(collision.duplicateLayers({1}, {QStringLiteral("Another copy")}, 1), Result::Success);
    QVERIFY(collision.snapshot()->layers[1].uuid != firstCopy);
    QVERIFY(collision.snapshot()->layers[1].uuid != collisionImage.layers.first().uuid);
    QCOMPARE(first.duplicateLayers({1}, {QStringLiteral("Another copy")}, 1), Result::Success);
    QVERIFY(first.snapshot()->layers[1].uuid != firstCopy);
}

void KisGpuOperationDocumentTest::duplicatedLayersShareInputsAndPixelsWithIndependentHistory()
{
    OperationDocument document(rasterImage());
    const auto first = document.snapshot()->layers.first().id;
    const auto last = document.addLayer(QStringLiteral("Top"));
    Brush brush;
    brush.mirrorHorizontal = true;
    brush.mirrorCenter = {160, 90};
    const auto stroke = document.beginStroke(first, brush, 7);
    QCOMPARE(document.appendSamples(stroke, {sample(16, 16)}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.writeLayerChannelMask(first, 7), Result::Success);
    QCOMPARE(document.writeLayerOpacity(first, 128), Result::Success);
    QCOMPARE(document.writeLayerPinnedToTimeline(first, true), Result::Success);
    QCOMPARE(document.selectLayers({first, last}, first), Result::Success);
    const auto fixed = document.snapshot();
    const auto count = document.undoCount();
    QCOMPARE(document.duplicateLayers({first, last}, {QStringLiteral("First copy"), QStringLiteral("Last copy")}, first), Result::Success);
    const auto copied = document.snapshot();
    QCOMPARE(copied->layers.size(), 5);
    QCOMPARE(document.undoCount(), count + 1);
    QCOMPARE(document.historyAction(count + 1), HistoryAction::DuplicateLayer);
    QCOMPARE(copied->layers[3].name, QStringLiteral("First copy"));
    QCOMPARE(copied->layers[4].name, QStringLiteral("Last copy"));
    const auto copy = copied->layers[3];
    QCOMPARE(copied->activeLayer, copy.id);
    QCOMPARE(copied->selectedLayers, (QList<LayerId>{copy.id, copied->layers[4].id}));
    QVERIFY(copy.id != first);
    QVERIFY(copy.uuid != fixed->layers.first().uuid);
    QCOMPARE(copy.opacity, quint8(128));
    QVERIFY(copy.pinnedToTimeline);
    QCOMPARE(copy.channelWriteMask, quint8(7));
    QCOMPARE(copy.operations.size(), fixed->layers.first().operations.size());
    const auto originalPaint = std::get<PaintSnapshot>(fixed->layers.first().operations.last());
    const auto copiedPaint = std::get<PaintSnapshot>(copy.operations.last());
    QCOMPARE(copiedPaint->brush, originalPaint->brush);
    QCOMPARE(copiedPaint->sampleBlocks, originalPaint->sampleBlocks);
    QVERIFY(copiedPaint->id != originalPaint->id);
    QCOMPARE(copiedPaint->layer, copy.id);
    const auto originalWrite = std::get<TileWriteSnapshot>(fixed->layers.first().operations.first());
    const auto copiedWrite = std::get<TileWriteSnapshot>(copy.operations.first());
    QCOMPARE(copiedWrite->tiles, originalWrite->tiles);
    QCOMPARE(copied->sampleCount, fixed->sampleCount * 2);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), fixed);
    QCOMPARE(document.snapshot()->selectedLayers, (QList<LayerId>{first, last}));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), copied);
    const auto before = document.preview();
    QCOMPARE(document.selectLayers({first, first}, first), Result::InvalidInput);
    QCOMPARE(document.selectLayers({}, first), Result::InvalidInput);

    QCOMPARE(document.duplicateLayers({first, first}, {QStringLiteral("A"), QStringLiteral("B")}, first), Result::InvalidInput);
    QCOMPARE(document.duplicateLayers({first}, {}, first), Result::InvalidInput);
    QCOMPARE(document.preview(), before);
    Limits limits;
    limits.layerCount = 3;
    OperationDocument bounded(rasterImage(), limits);
    const auto limited = bounded.snapshot();
    QCOMPARE(bounded.duplicateLayers({1, 2}, {QStringLiteral("A"), QStringLiteral("B")}, 1), Result::CapacityExceeded);
    QCOMPARE(bounded.snapshot(), limited);
    QCOMPARE(bounded.undoCount(), 0);
    limits.layerCount = 4096;
    limits.rasterBytes = 2 * RgbaTile::ByteCount;
    OperationDocument pixelBounded(rasterImage(), limits);
    const auto pixelLimit = pixelBounded.preview();
    QCOMPARE(pixelBounded.duplicateLayers({1}, {QStringLiteral("A")}, 1), Result::CapacityExceeded);
    QCOMPARE(pixelBounded.preview(), pixelLimit);
    limits.samplesPerStroke = limits.samplesPerDocument = 1;
    OperationDocument inputBounded({64, 64}, limits);
    QVERIFY(!paint(inputBounded, {sample(16, 16)}).isNull());
    const auto inputLimit = inputBounded.preview();
    QCOMPARE(inputBounded.duplicateLayers({inputBounded.activeLayer()}, {QStringLiteral("A")}, inputBounded.activeLayer()), Result::CapacityExceeded);
    QCOMPARE(inputBounded.preview(), inputLimit);
}

void KisGpuOperationDocumentTest::symmetryFreezesAxesAndRejectsInvalidInput()
{
    OperationDocument document({64, 64});
    Brush brush;
    brush.mirrorHorizontal = brush.mirrorVertical = true;
    brush.mirrorCenter = {24, 30};
    const auto stroke = document.beginStroke(document.activeLayer(), brush, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(16, 16)}), Result::Success);
    const auto fixed = document.preview();
    const auto settings = brush;
    brush.mirrorCenter = {8, 8};
    brush.mirrorHorizontal = brush.mirrorVertical = false;
    QCOMPARE(std::get<PaintSnapshot>(fixed->layers.first().operations.last())->brush, settings);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    const auto second = document.beginStroke(document.activeLayer(), brush, 8);
    QVERIFY(!second.isNull());
    QCOMPARE(document.appendSamples(second, {sample(20, 20)}), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->brush.mirrorCenter, QPoint());
    QCOMPARE(document.cancelStroke(second), Result::Success);
    const auto unchanged = document.snapshot();
    brush.mirrorHorizontal = true;
    brush.mirrorCenter = {std::numeric_limits<int>::min(), 0};
    QVERIFY(document.beginStroke(document.activeLayer(), brush, 9).isNull());
    QCOMPARE(document.snapshot(), unchanged);
}

void KisGpuOperationDocumentTest::directLayerPropertiesPreserveOtherPropertyHistoryAndFixedVersions()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(first, 128), Result::Success);
    const auto second = document.addLayer(QStringLiteral("Other"));
    QVERIFY(second);
    const auto pinned = document.snapshot();
    QCOMPARE(document.writeLayerName(first, {}), Result::Success);
    QCOMPARE(document.writeLayerVisible(first, false), Result::Success);
    QCOMPARE(document.writeLayerLocked(first, true), Result::Success);
    QCOMPARE(document.writeLayerPinnedToTimeline(first, true), Result::Success);
    QCOMPARE(document.writeLayerOpacity(second, 64), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(pinned->layers.first().name, QStringLiteral("Paint Layer 1"));
    QVERIFY(pinned->layers.first().visible);
    QVERIFY(!pinned->layers.first().userLocked);
    QVERIFY(!pinned->layers.first().pinnedToTimeline);
    QCOMPARE(pinned->layers.last().opacity, quint8(255));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(document.snapshot()->layers.first().name, QString());
    QVERIFY(!document.snapshot()->layers.first().visible);
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QVERIFY(document.snapshot()->layers.first().pinnedToTimeline);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.last().opacity, quint8(64));
}

void KisGpuOperationDocumentTest::directOpacityWritesPreserveRecordedUndoValues()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
}

void KisGpuOperationDocumentTest::layerOpacityAndLockShareHistory()
{
    auto source = rasterImage();
    source.activeLayer = 0;
    source.layers[0].opacity = 128;
    source.layers[0].userLocked = true;
    OperationDocument document(source);
    const auto layer = document.activeLayer();
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QVERIFY(document.beginStroke(layer, Brush{}, 1).isNull());
    QCOMPARE(document.undoCount(), 0);
    QVERIFY(!document.isModified());
    QCOMPARE(document.setLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.setLayerLocked(layer, false), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    const auto committed = document.snapshot();
    const auto stroke = document.beginStroke(layer, Brush{}, 1);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.setLayerOpacity(layer, 32), Result::EditInProgress);
    QCOMPARE(document.setLayerLocked(layer, true), Result::EditInProgress);
    QCOMPARE(document.snapshot(), committed);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.first().userLocked);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(128));
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityAcrossHistory_data()
{
    QTest::addColumn<bool>("raster");
    QTest::newRow("compressed pixels") << true;
    QTest::newRow("stroke input") << false;
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityAcrossHistory()
{
    QFETCH(bool, raster);
    Limits limits;
    limits.samplesPerStroke = 1;
    limits.samplesPerDocument = 1;
    limits.rasterBytes = RgbaTile::ByteCount;
    OperationDocument document(QSize(64, 64), limits);
    RgbaTile pixels{{0, 0}, QByteArray(RgbaTile::ByteCount, char(0))};
    for (int cycle = 0; cycle < 3; ++cycle) {
        if (cycle) {
            QCOMPARE(document.removeLayer(document.activeLayer()), Result::Success);
            QVERIFY(document.addLayer(QString::number(cycle)));
        }
        if (raster) {
            pixels.rgba.fill(char(cycle + 1));
            QCOMPARE(document.replaceTiles(document.activeLayer(), {pixels}), Result::Success);
        } else {
            QVERIFY(!paint(document, {sample(8 + cycle, 12)}).isNull());
        }
    }
    const auto versions = document.historyVersions();
    if (raster) {
        QCOMPARE(document.replaceTiles(document.activeLayer(), {pixels}), Result::CapacityExceeded);
    } else {
        const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 0);
        QCOMPARE(document.appendSamples(stroke, {sample(20, 20)}), Result::CapacityExceeded);
        QCOMPARE(document.cancelStroke(stroke), Result::Success);
    }
    QCOMPARE(document.historyVersions(), versions);
    for (int i = versions.size() - 1; i > 0; --i) {
        QCOMPARE(document.snapshot(), versions[i]);
        QCOMPARE(document.undo(), Result::Success);
    }
    QCOMPARE(document.snapshot(), versions.first());
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), versions.size() - 1);
}

void KisGpuOperationDocumentTest::layerStructureSharesPaintingHistoryAndPreservesPins()
{
    OperationDocument document(rasterImage());
    const auto initial = document.snapshot();
    const auto lower = initial->layers[0].id;
    const auto upper = initial->layers[1].id;
    QCOMPARE(document.renameLayer(lower, QStringLiteral("Renamed 下地")), Result::Success);
    const auto renamed = document.snapshot();
    QCOMPARE(renamed->layers[0].name, QStringLiteral("Renamed 下地"));
    QCOMPARE(renamed->layers[0].uuid, initial->layers[0].uuid);
    QCOMPARE(renamed->layers[0].operations, initial->layers[0].operations);
    QCOMPARE(document.moveLayer(lower, 1), Result::Success);
    const auto moved = document.snapshot();
    QCOMPARE(moved->layers[1].id, lower);
    QCOMPARE(document.activeLayer(), upper);
    QCOMPARE(document.removeLayer(upper), Result::Success);
    QCOMPARE(document.activeLayer(), lower);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.snapshot()->rasterBytes, RgbaTile::ByteCount);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), moved);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), renamed);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(initial->layers.size(), 2);
    QCOMPARE(initial->layers[0].name, QStringLiteral("Red"));
}

void KisGpuOperationDocumentTest::rejectedLayerChangesKeepRedoAndPendingInput()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Second"));
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    QCOMPARE(document.renameLayer(second, QStringLiteral("Gone")), Result::LayerUnavailable);
    QCOMPARE(document.moveLayer(layer, -1), Result::InvalidInput);
    QCOMPARE(document.moveLayer(layer, 1), Result::InvalidInput);
    QCOMPARE(document.removeLayer(second), Result::LayerUnavailable);
    QCOMPARE(document.renameLayer(layer, QString(65537, QLatin1Char('a'))), Result::InvalidInput);
    QCOMPARE(document.renameLayer(layer, before->layers[0].name), Result::Success);
    QCOMPARE(document.moveLayer(layer, 0), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.redoCount(), 1);
    const auto stroke = document.beginStroke(layer, Brush{}, 0);
    QCOMPARE(document.appendSamples(stroke, {sample(8, 12)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.renameLayer(layer, QStringLiteral("Busy")), Result::EditInProgress);
    QCOMPARE(document.moveLayer(layer, 0), Result::EditInProgress);
    QCOMPARE(document.removeLayer(layer), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.redoCount(), 1);
    document.close();
    QCOMPARE(document.renameLayer(layer, {}), Result::Closed);
    QCOMPARE(document.moveLayer(layer, 0), Result::Closed);
    QCOMPARE(document.removeLayer(layer), Result::Closed);
}

void KisGpuOperationDocumentTest::removingTheLastLayerPreservesAnEmptyUndoableDocument()
{
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.removeLayer(layer), Result::Success);
    const auto empty = document.snapshot();
    QVERIFY(empty->layers.isEmpty());
    QCOMPARE(empty->activeLayer, LayerId(0));
    QCOMPARE(empty->sampleCount, 0);
    QCOMPARE(empty->rasterBytes, 0);
    QVERIFY(document.beginStroke(layer, Brush{}, 0).isNull());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), empty);
    const auto added = document.addLayer(QStringLiteral("New"));
    QVERIFY(added > layer);
    QCOMPARE(document.activeLayer(), added);
    OperationDocument imported(RasterImage{QSize(64, 64), {}, 0});
    QVERIFY(imported.snapshot()->layers.isEmpty());
    QCOMPARE(imported.activeLayer(), LayerId(0));
    QVERIFY(!imported.isModified());
    QVERIFY(imported.addLayer(QStringLiteral("First")));
}

void KisGpuOperationDocumentTest::inputChunksRetainTheSameOperation()
{
    OperationDocument one({1024, 1024}), chunks({1024, 1024});
    QList<InputSample> input;
    for (int i = 0; i < 2049; ++i) {
        input.append({QPointF(i % 1024, (i * 7) % 1024), qreal(i % 101) / 100,
                      qreal(i % 91), -qreal(i % 91), qreal(i % 360), quint64(i)});
    }
    const auto a = one.beginStroke(one.activeLayer(), Brush{}, 193);
    const auto b = chunks.beginStroke(chunks.activeLayer(), Brush{}, 193);
    QCOMPARE(one.appendSamples(a, input), Result::Success);
    for (const auto &point : input) {
        QCOMPARE(chunks.appendSamples(b, {point}), Result::Success);
    }
    QCOMPARE(one.commitStroke(a), Result::Success);
    QCOMPARE(chunks.commitStroke(b), Result::Success);
    const auto left = std::get<PaintSnapshot>(one.snapshot()->layers.front().operations.back());
    const auto right = std::get<PaintSnapshot>(chunks.snapshot()->layers.front().operations.back());
    QCOMPARE(left->inputSamples(), right->inputSamples());
    QCOMPARE(left->inputSamples(), input);
    QCOMPARE(left->brush, right->brush);
    QCOMPARE(left->randomSeed, right->randomSeed);
    QCOMPARE(one.undoCount(), 1);
    QCOMPARE(chunks.undoCount(), 1);
}

void KisGpuOperationDocumentTest::shapePreviewCommitsAsOneHistoryEntry()
{
    OperationDocument document({1024, 1024});
    const auto original = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(id, {sample(1, 2), sample(30, 40)}), Result::Success);
    const auto freehand = document.preview();
    QCOMPARE(document.updateShape(id, {ShapeKind::Line, {1, 2}, {60, 70}, 0}), Result::Success);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 2}, {90, 70}, 0.25}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.snapshot(), preview);
    QCOMPARE(document.undoCount(), 1);
    const auto operation = std::get<PaintSnapshot>(preview->layers.front().operations.back());
    QCOMPARE(operation->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(operation->inputSamples().size(), 2);
    QCOMPARE(std::get<PaintSnapshot>(freehand->layers.front().operations.back())->shape.kind, ShapeKind::Freehand);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::cancelAndRejectedInputPreserveRedo()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(2, 3)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 1), sample(2, 2, -1)}), Result::InvalidInput);
    QCOMPARE(document.preview(), document.snapshot());
    QCOMPARE(document.appendSamples(id, {sample(5, 5)}), Result::Success);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!paint(document, {sample(8, 9)}).isNull());
    QCOMPARE(document.redoCount(), 0);
}

void KisGpuOperationDocumentTest::documentChangesShareOneHistory()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Ink"));
    QVERIFY(second != 0);
    QCOMPARE(document.selectLayer(second), Result::Success);
    QVERIFY(!paint(document, {sample(3, 4)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.setLayerVisible(second, false), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.back().operations.empty());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.snapshot()->layers.back().visible);
    QVERIFY(document.beginStroke(second, Brush{}, 0).isNull());
}

void KisGpuOperationDocumentTest::savesPinTheirVersionWhileEditingContinues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto saving = document.snapshot();
    QVERIFY(!paint(document, {sample(4, 5)}).isNull());
    const auto edited = document.snapshot();
    QCOMPARE(document.markSaved(saving), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(saving->layers.front().operations.size(), 1);
    QCOMPARE(edited->layers.front().operations.size(), 2);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.isModified());
    OperationDocument other({64, 64});
    QCOMPARE(document.markSaved(other.snapshot()), Result::StaleVersion);
    QCOMPARE(document.snapshot(), edited);
}

void KisGpuOperationDocumentTest::historyRetains250Operations()
{
    OperationDocument document({64, 64});
    for (int i = 0; i < 260; ++i) {
        QVERIFY(!paint(document, {sample(i, i)}).isNull());
    }
    QCOMPARE(document.undoCount(), 250);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.undo(), Result::Success);
    }
    QCOMPARE(document.undo(), Result::NothingToUndo);
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 10);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.redo(), Result::Success);
    }
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 260);
    QCOMPARE(document.redo(), Result::NothingToRedo);
}

void KisGpuOperationDocumentTest::staleAndForeignEditsCannotChangeADocument()
{
    OperationDocument document({64, 64}), other({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(other.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QVERIFY(next != id);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.commitStroke(id), Result::StaleOperation);
    QCOMPARE(document.undo(), Result::EditInProgress);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::EditInProgress);
    QCOMPARE(document.appendSamples(next, {sample(1, 2)}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
}

void KisGpuOperationDocumentTest::frozenOperationsOutliveTheDocument()
{
    Snapshot frozen;
    {
        OperationDocument document({64, 64});
        Brush brush;
        brush.rgba = {12, 34, 56, 78};
        const auto id = document.beginStroke(document.activeLayer(), brush, 123);
        brush.rgba = {0, 0, 0, 0};
        QCOMPARE(document.appendSamples(id, {sample(4, 8, 0.5)}), Result::Success);
        frozen = document.preview();
        QCOMPARE(document.commitStroke(id), Result::Success);
    }
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->brush.rgba[0], 12);
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->randomSeed, quint64(123));
    QCOMPARE(std::get<PaintSnapshot>(frozen->layers.front().operations.back())->inputSamples().front().pressure, qreal(0.5));
}

void KisGpuOperationDocumentTest::limitsRejectWholeRequests()
{
    Limits limits;
    limits.samplesPerStroke = 2;
    limits.layerCount = 1;
    OperationDocument document({64, 64}, limits);
    const auto initial = document.snapshot();
    QCOMPARE(document.addLayer(QStringLiteral("over limit")), LayerId(0));
    QCOMPARE(document.snapshot(), initial);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(3, 4), sample(5, 6)}), Result::CapacityExceeded);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.appendSamples(id, {sample(std::numeric_limits<qreal>::infinity(), 0)}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto committed = std::get<PaintSnapshot>(document.snapshot()->layers.front().operations.back());
    QCOMPARE(committed->inputSamples(), QList<InputSample>{sample(1, 2)});
    QCOMPARE(document.undoCount(), 1);
}

void KisGpuOperationDocumentTest::closedDocumentPreservesSnapshots()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto frozen = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(8, 9)}), Result::Success);
    document.close();
    QCOMPARE(document.snapshot(), frozen);
    QCOMPARE(document.appendSamples(id, {sample(10, 11)}), Result::Closed);
    QCOMPARE(document.commitStroke(id), Result::Closed);
    QCOMPARE(document.undo(), Result::Closed);
    QVERIFY(document.beginStroke(document.activeLayer(), Brush{}, 0).isNull());
    QCOMPARE(frozen->layers.front().operations.size(), 1);
}

void KisGpuOperationDocumentTest::cancelledPreviewsCannotBecomeSavedVersions()
{
    OperationDocument document({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto abandoned = document.preview();
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QVERIFY(!document.isModified());
}

void KisGpuOperationDocumentTest::emptyAndInvalidRequestsLeaveHistoryUnchanged()
{
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument(QSize(0, 1)));
    OperationDocument document({64, 64});
    const auto original = document.snapshot();
    const auto invalidHistory = static_cast<PropertyHistory>(255);
    QCOMPARE(document.renameLayer(document.activeLayer(), QStringLiteral("Invalid"), invalidHistory), Result::InvalidInput);
    QCOMPARE(document.setLayerOpacity(document.activeLayer(), 128, invalidHistory), Result::InvalidInput);
    QCOMPARE(document.snapshot(), original);
    Brush invalid;
    invalid.diameter = std::numeric_limits<qreal>::quiet_NaN();
    QVERIFY(document.beginStroke(document.activeLayer(), invalid, 0).isNull());
    QVERIFY(document.beginStroke(0, Brush{}, 0).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.commitStroke(id), Result::EmptyOperation);
    QCOMPARE(document.appendSamples(id, {}), Result::Success);
    QCOMPARE(document.appendSamples(id, {sample(1, 2, 1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(4, 5, 1, 1)}), Result::InvalidInput);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 1}, {1, 2}, 0}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), true), Result::Success);
    QCOMPARE(document.selectLayer(document.activeLayer()), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::importedPixelsPreserveLayersAndStartWithoutUndo()
{
    auto input = rasterImage();
    OperationDocument document(input);
    const auto frozen = document.snapshot();
    QCOMPARE(frozen->size, input.size);
    QCOMPARE(frozen->layers.size(), 2);
    QCOMPARE(document.activeLayer(), LayerId(2));
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.redoCount(), 0);
    QVERIFY(!document.isModified());
    QCOMPARE(frozen->rasterBytes, qsizetype(2 * RgbaTile::ByteCount));
    QCOMPARE(frozen->layers[0].uuid, input.layers[0].uuid);
    QCOMPARE(frozen->layers[0].defaultPixel, input.layers[0].defaultPixel);
    QCOMPARE(frozen->layers[1].uuid, input.layers[1].uuid);
    QVERIFY(!frozen->layers[1].visible);
    QCOMPARE(frozen->layers[1].name, input.layers[1].name);
    const auto checkpoint = std::get<TileWriteSnapshot>(frozen->layers[1].operations.first());
    QCOMPARE(checkpoint->tiles.first()->coordinate(), QPoint(1, -1));
    QCOMPARE(checkpoint->tiles.first()->rgba(), input.layers[1].tiles.first().rgba);
    input.layers[1].tiles[0].rgba.fill('\0');
    QCOMPARE(checkpoint->tiles.first()->rgba(), QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4));

    for (int invalid = 0; invalid < 5; ++invalid) {
        auto image = rasterImage();
        if (invalid == 0) image.layers.clear();
        if (invalid == 1) image.activeLayer = 2;
        if (invalid == 2) image.layers[1].uuid = image.layers[0].uuid;
        if (invalid == 3) image.layers[0].tiles.append(image.layers[0].tiles.first());
        if (invalid == 4) image.layers[0].name = QString(QChar(0xd800));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument{image});
    }
    QCOMPARE(document.addLayer(QString(QChar(0xd800))), LayerId(0));
    QCOMPARE(document.snapshot(), frozen);
    const auto added = document.addLayer(QStringLiteral("New"));
    QVERIFY(added);
    QVERIFY(!document.snapshot()->layers.last().uuid.isNull());
    QVERIFY(document.snapshot()->layers.last().uuid != frozen->layers.first().uuid);
}

void KisGpuOperationDocumentTest::tileReplacementSharesHistoryAndRejectsWholeRequests()
{
    Limits limits;
    limits.rasterBytes = 3 * RgbaTile::ByteCount;
    OperationDocument document({512, 64}, limits);
    const auto layer = document.activeLayer();
    const RgbaTile red{{0, 0}, QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(layer, {red}), Result::Success);
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    const auto original = document.snapshot();
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.replaceTiles(layer, {red, red}), Result::InvalidInput);
    QCOMPARE(document.replaceTiles(layer, {{{0, 0}, QByteArrayLiteral("short")}}), Result::InvalidInput);
    QCOMPARE(document.replaceTiles(layer, {}), Result::EmptyOperation);
    QCOMPARE(document.replaceTiles(999, {red}), Result::LayerUnavailable);
    QCOMPARE(document.replaceTiles(layer, {red, {{1, 0}, red.rgba}, {{2, 0}, red.rgba}}), Result::CapacityExceeded);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redoCount(), 1);
    const auto stroke = document.beginStroke(layer, Brush{}, 0);
    QCOMPARE(document.replaceTiles(layer, {red}), Result::EditInProgress);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    auto blue = red;
    blue.rgba = QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4);
    QCOMPARE(document.replaceTiles(layer, {blue}), Result::Success);
    const auto replaced = document.snapshot();
    blue.rgba.fill('\0');
    QCOMPARE(std::get<TileWriteSnapshot>(replaced->layers.first().operations.last())->tiles.first()->rgba(),
             QByteArray::fromHex("0000ff80").repeated(RgbaTile::ByteCount / 4));
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.redoCount(), 0);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), replaced);
    document.close();
    QCOMPARE(document.replaceTiles(layer, {red}), Result::Closed);
}

void KisGpuOperationDocumentTest::filledFiguresPreserveStylesHistory()
{
    OperationDocument document(QSize(64, 64));
    const auto initial = document.snapshot();
    Brush brush;
    brush.rgba = {17, 129, 241, 255};
    const auto id = document.beginStroke(document.activeLayer(), brush, 42);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {sample(12, 12), sample(52, 52)}), Result::Success);
    FigureStyle style;
    style.outline = false;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.updateFigure(id, {ShapeKind::Rectangle, {12, 12}, {52, 52}, 0}, style), Result::Success);
    const auto preview = document.preview();
    const auto operation = std::get<PaintSnapshot>(preview->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QCOMPARE(operation->figureStyle.fill, style.fill);
    QCOMPARE(operation->brush.rgba, brush.rgba);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::brushCanvasOpacitySurvivesPinnedHistory()
{
    OperationDocument document(QSize(64, 64));
    const auto initial = document.snapshot();
    const qreal opacity = .37123456789;
    Brush brush;
    brush.opacity = opacity;
    const auto id = document.beginStroke(document.activeLayer(), brush, 42);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {sample(12, 12), sample(52, 52)}), Result::Success);
    const auto pending = document.preview();
    const auto input = std::get<PaintSnapshot>(pending->layers.first().operations.last());
    QCOMPARE(input->brush.opacity, opacity);
    brush.opacity = .75;
    QCOMPARE(document.preview(), pending);
    QCOMPARE(input->brush.opacity, opacity);
    QCOMPARE(document.commitStroke(id), Result::Success);
    const auto committed = document.snapshot();
    QCOMPARE(std::get<PaintSnapshot>(committed->layers.first().operations.last())->brush.opacity, opacity);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), committed);
    QCOMPARE(input->brush.opacity, opacity);
}

void KisGpuOperationDocumentTest::invalidCanvasOpacityPreservesRedo()
{
    OperationDocument document(QSize(64, 64));
    QVERIFY(!paint(document, {sample(12, 12), sample(52, 52)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.preview();
    for (const qreal opacity : {qreal(-.1), qreal(1.01), std::numeric_limits<qreal>::infinity(),
                                std::numeric_limits<qreal>::quiet_NaN()}) {
        Brush brush;
        brush.opacity = opacity;
        QVERIFY(document.beginStroke(document.activeLayer(), brush, 43).isNull());
        QCOMPARE(document.fillLayer(document.activeLayer(), {7, 19, 31}, opacity), Result::InvalidInput);
        QCOMPARE(document.preview(), before);
        QCOMPARE(document.undoCount(), 0);
        QCOMPARE(document.redoCount(), 1);
    }
}

void KisGpuOperationDocumentTest::fillBlendModesFreezeHistory_data()
{
    QTest::addColumn<int>("mode");
    for (const auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen, LayerBlendMode::Darken,
                            LayerBlendMode::Lighten, LayerBlendMode::Addition, LayerBlendMode::LinearDodge}) {
        const auto name = mode == LayerBlendMode::Multiply ? QByteArray("multiply") :
            mode == LayerBlendMode::Screen ? QByteArray("screen") :
            mode == LayerBlendMode::Darken ? QByteArray("darken") :
            mode == LayerBlendMode::Lighten ? QByteArray("lighten") :
            mode == LayerBlendMode::Addition ? QByteArray("addition") : QByteArray("linear-dodge");
        QTest::newRow(name.constData()) << int(mode);
    }
}

void KisGpuOperationDocumentTest::fillBlendModesFreezeHistory()
{
    QFETCH(int, mode);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();
    QCOMPARE(document.fillLayer(layer, {221, 17, 44}, 255.0 / 255, false, false, LayerBlendMode(mode)),
             Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::FillLayer);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QCOMPARE(operation->figureStyle.fill, (RgbaPixel{221, 17, 44, 255}));
    QCOMPARE(operation->figureStyle.fillBlendMode, LayerBlendMode(mode));
    const auto committed = document.snapshot();

    // One fill is one history entry; undo removes it and redo restores the
    // same frozen mode.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), committed);
}

void KisGpuOperationDocumentTest::patternFillBlendModeFreezesHistory()
{
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray::fromHex("030507ff3f051bff7b0005ff0b141eff");
    QVERIFY(pattern.isValid());

    OperationDocument document({64, 32});
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayerPattern(layer, pattern, 255.0 / 255, false, false, LayerBlendMode::Multiply),
             Result::Success);
    const auto committed = document.snapshot();
    const auto operation = std::get<PaintSnapshot>(committed->layers.first().operations.last());
    QVERIFY(operation->figureStyle.pattern);
    QCOMPARE(*operation->figureStyle.pattern, pattern);
    QCOMPARE(operation->figureStyle.fillBlendMode, LayerBlendMode::Multiply);
    QCOMPARE(document.historyAction(1), HistoryAction::FillLayer);

    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), committed);
}

void KisGpuOperationDocumentTest::rejectedFillBlendModesPreserveHistoryAndInput()
{
    OperationDocument document(QSize(32, 32));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();

    QCOMPARE(document.fillLayer(layer, {17, 129, 241}, 255.0 / 255, false, false, LayerBlendMode(7)),
             Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.undoCount(), 0);

    const QList<InputSample> samples{{{4, 4}, 1}, {{28, 28}, 1}};
    FigureStyle outOfRange;
    outOfRange.outline = false;
    outOfRange.fill = RgbaPixel{17, 129, 241, 255};
    outOfRange.fillBlendMode = LayerBlendMode(7);
    QCOMPARE(document.paintFigure(layer, Brush{}, samples, {ShapeKind::Rectangle, {4, 4}, {28, 28}, 0},
                                  outOfRange, 42), Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);

    FigureStyle blendedOutline;
    blendedOutline.outline = true;
    blendedOutline.fill = RgbaPixel{17, 129, 241, 255};
    blendedOutline.fillBlendMode = LayerBlendMode::Multiply;
    QCOMPARE(document.paintFigure(layer, Brush{}, samples, {ShapeKind::Rectangle, {4, 4}, {28, 28}, 0},
                                  blendedOutline, 42), Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);

    FigureStyle modeWithoutFill;
    modeWithoutFill.outline = false;
    modeWithoutFill.fillBlendMode = LayerBlendMode::Multiply;
    QCOMPARE(document.paintFigure(layer, Brush{}, samples, {ShapeKind::Rectangle, {4, 4}, {28, 28}, 0},
                                  modeWithoutFill, 42), Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);

    // Erase remains coverage removal: the frozen mode is Normal.
    QCOMPARE(document.fillLayer(layer, {17, 129, 241}, 255.0 / 255, true, false, LayerBlendMode::Multiply),
             Result::Success);
    const auto erased = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(erased->figureStyle.fillBlendMode, LayerBlendMode::Normal);
}

void KisGpuOperationDocumentTest::strokeBlendModesFreezeHistory_data()
{
    QTest::addColumn<int>("mode");
    for (const auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen, LayerBlendMode::Darken,
                            LayerBlendMode::Lighten, LayerBlendMode::Addition, LayerBlendMode::LinearDodge}) {
        const auto name = mode == LayerBlendMode::Multiply ? QByteArray("multiply") :
            mode == LayerBlendMode::Screen ? QByteArray("screen") :
            mode == LayerBlendMode::Darken ? QByteArray("darken") :
            mode == LayerBlendMode::Lighten ? QByteArray("lighten") :
            mode == LayerBlendMode::Addition ? QByteArray("addition") : QByteArray("linear-dodge");
        QTest::newRow(name.constData()) << int(mode);
    }
}

void KisGpuOperationDocumentTest::strokeBlendModesFreezeHistory()
{
    QFETCH(int, mode);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();
    Brush brush;
    brush.rgba = {221, 17, 44, 255};
    brush.diameter = 24;
    brush.blendMode = LayerBlendMode(mode);
    const auto stroke = document.beginStroke(layer, brush, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);

    // The mode belongs to the accepted input, not to the caller's later values.
    brush.blendMode = LayerBlendMode::Normal;
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->brush.blendMode, LayerBlendMode(mode));
    QCOMPARE(pending->brush.rgba, (RgbaPixel{221, 17, 44, 255}));

    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::PaintStroke);
    const auto committed = document.snapshot();
    const auto operation = std::get<PaintSnapshot>(committed->layers.first().operations.last());
    QCOMPARE(operation->brush.blendMode, LayerBlendMode(mode));

    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), committed);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->brush.blendMode,
             LayerBlendMode(mode));
}

void KisGpuOperationDocumentTest::rejectedStrokeBlendModesPreserveHistoryAndInput()
{
    OperationDocument document(QSize(32, 32));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();

    Brush outOfRange;
    outOfRange.blendMode = LayerBlendMode(7);
    QVERIFY(document.beginStroke(layer, outOfRange, 0).isNull());

    // Erase remains coverage removal and rejects a blend mode before history.
    Brush erased;
    erased.erase = true;
    erased.blendMode = LayerBlendMode::Multiply;
    QVERIFY(document.beginStroke(layer, erased, 0).isNull());
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.undoCount(), 0);

    // A normal erase stroke remains accepted and freezes the Normal fallback.
    erased.blendMode = LayerBlendMode::Normal;
    const auto stroke = document.beginStroke(layer, erased, 0);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{16, 16}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->brush.erase, true);
    QCOMPARE(pending->brush.blendMode, LayerBlendMode::Normal);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QCOMPARE(document.snapshot(), initial);
}

void KisGpuOperationDocumentTest::patternFillFreezesPixelsHistory()
{
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray::fromHex("030507ff3f051bff7b0005ff0b141eff");
    QVERIFY(pattern.isValid());

    OperationDocument document({64, 32});
    const auto layer = document.activeLayer();
    const auto before = document.snapshot();
    PatternFill malformed{QSize(2, 2), QByteArray(3, '\0')};
    QCOMPARE(document.fillLayerPattern(layer, malformed), Result::InvalidInput);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.undoCount(), 0);

    QCOMPARE(document.fillLayerPattern(layer, pattern), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::FillLayer);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QVERIFY(!operation->figureStyle.fill);
    QVERIFY(operation->figureStyle.pattern);
    QCOMPARE(*operation->figureStyle.pattern, pattern);
    QCOMPARE(operation->selectionRect, QRect());

    // One history entry owns the frozen resource; undo removes the fill and
    // redo restores the same operation.
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(*std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->figureStyle.pattern,
             pattern);
}

void KisGpuOperationDocumentTest::emptyFigureIntentPreservesHistory()
{
    OperationDocument document(QSize(32, 32));
    const auto initial = document.snapshot();
    FigureStyle style;
    style.outline = false;
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, {{{0, 0}, 1}, {{16, 16}, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, {16, 16}, 0}, style, 42), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(document.isModified());
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QVERIFY(!operation->figureStyle.fill);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
}

void KisGpuOperationDocumentTest::pathsPreserveCommandsControlPoints()
{
    OperationDocument document(QSize(64, 64));
    const auto initial = document.snapshot();
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo, PathCommand::MoveTo, PathCommand::LineTo};
    shape.windingFill = true;
    const QList<InputSample> samples{{{12, 32}, 1}, {{20, 12}, 1}, {{40, 52}, 1},
                                    {{52, 32}, 1}, {{12, 52}, 1}, {{52, 52}, 1}};
    FigureStyle style;
    style.fill = RgbaPixel{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples, shape, style, 42), Result::Success);
    const auto pinned = document.snapshot();
    QCOMPARE(pinned->sampleCount, samples.size());
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::DrawPath);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), pinned);
}

void KisGpuOperationDocumentTest::pathCallsRetainInputHistory()
{
    OperationDocument document(QSize(128, 128));
    const QList<InputSample> samples{{{12, 24}, .2}, {{42, 24}, .8}, {{76, 96}, 1},
                                    {{80, 80}, 1}, {{100, 80}, 1}, {{112, 96}, 1}};
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::CubicTo};
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto pinned = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    shape.pathCalls = {{0, 2}, {2, 6}};
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    QVERIFY(pinned->shape.pathCalls.isEmpty());
    QCOMPARE(pinned->inputSamples(), samples);
}

void KisGpuOperationDocumentTest::invalidPathCallsPreserveTheActiveDocument_data()
{
    QTest::addColumn<int>("error");
    QTest::newRow("overlap") << 0;
    QTest::newRow("out-of-input") << 1;
    QTest::newRow("non-move-start") << 2;
    QTest::newRow("split-cubic") << 3;
    QTest::newRow("empty-range") << 4;
    QTest::newRow("non-path") << 5;
}

void KisGpuOperationDocumentTest::invalidPathCallsPreserveTheActiveDocument()
{
    QFETCH(int, error);
    OperationDocument document(QSize(128, 128));
    const QList<InputSample> samples{{{12, 24}, 1}, {{42, 24}, 1}, {{76, 96}, 1},
                                    {{80, 80}, 1}, {{100, 80}, 1}, {{112, 96}, 1}};
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::CubicTo};
    shape.pathCalls = {{0, 2}, {2, 6}};
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(stroke, samples), Result::Success);
    QCOMPARE(document.updateShape(stroke, shape), Result::Success);
    const auto pinned = document.preview();
    const auto stateBeforeRequest = document.preview();
    if (error == 0) shape.pathCalls = {{0, 6}, {2, 6}};
    else if (error == 1) shape.pathCalls = {{0, 7}};
    else if (error == 2) shape.pathCalls = {{1, 2}};
    else if (error == 3) shape.pathCalls = {{0, 5}};
    else if (error == 4) shape.pathCalls = {{2, 2}};
    else { shape.kind = ShapeKind::Line; shape.pathCommands.clear(); }
    QCOMPARE(document.updateShape(stroke, shape), Result::InvalidInput);
    QCOMPARE(document.preview(), pinned);
    QCOMPARE(document.preview(), stateBeforeRequest);
    QCOMPARE(document.undoCount(), 0);
    OperationDocument empty(QSize(128, 128));
    const auto before = empty.snapshot();
    QCOMPARE(empty.paintFigure(empty.activeLayer(), Brush{}, samples, shape, {}, 42), Result::InvalidInput);
    QCOMPARE(empty.snapshot(), before);
    QCOMPARE(empty.undoCount(), 0);
}

void KisGpuOperationDocumentTest::pixelCorrectionRetainsOriginalSensorsAndPendingInput()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.pixelPerfect = true;
    const auto id = document.beginStroke(document.activeLayer(), brush, 7);
    const QList<InputSample> samples{{{12.2, 12.7}, 0.4, 12, -9, 32, 4},
                                    {{13.8, 12.3}, 0.8, -12, 9, 64, 8},
                                    {{14.1, 13.9}, 1, 0, 0, 90, 12}};
    QCOMPARE(document.appendSamples(id, samples), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QVERIFY(pending->brush.pixelPerfect);
    QCOMPARE(pending->inputSamples(), samples);
}

void KisGpuOperationDocumentTest::selectionRectIsFrozenInOperations()
{
    OperationDocument document(QSize(128, 128));
    QCOMPARE(document.setSelectionRect(QRect(10, 20, 30, 40)), Result::Success);
    QCOMPARE(document.selectionRect(), QRect(10, 20, 30, 40));
    const auto first = document.beginStroke(document.activeLayer(), Brush{}, 1);
    QVERIFY(!first.isNull());
    QCOMPARE(document.appendSamples(first, {{{16, 32}, 1}, {{28, 48}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->selectionRect, QRect(10, 20, 30, 40));
    QCOMPARE(document.setSelectionRect(QRect(0, 0, 2, 2)), Result::EditInProgress);
    QCOMPARE(document.clearSelection(), Result::EditInProgress);
    QCOMPARE(document.commitStroke(first), Result::Success);

    QCOMPARE(document.setSelectionRect(QRect(50, 60, 20, 10)), Result::Success);
    const auto second = document.beginStroke(document.activeLayer(), Brush{}, 2);
    QVERIFY(!second.isNull());
    QCOMPARE(document.appendSamples(second, {{{54, 62}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(second), Result::Success);
    const auto &operations = document.snapshot()->layers.first().operations;
    QCOMPARE(operations.size(), 2);
    QCOMPARE(std::get<PaintSnapshot>(operations[0])->selectionRect, QRect(10, 20, 30, 40));
    QCOMPARE(std::get<PaintSnapshot>(operations[1])->selectionRect, QRect(50, 60, 20, 10));
}

void KisGpuOperationDocumentTest::invertedSelectionPreservesHistory()
{
    OperationDocument document(QSize(64, 64));
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 32, 32), true), Result::Success);
    QCOMPARE(document.selectionRect(), QRect(8, 8, 32, 32));
    QVERIFY(document.selectionInverted());
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 32, 32)), Result::Success);
    QVERIFY(!document.selectionInverted());
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 32, 32), true), Result::Success);
    QVERIFY(document.selectionInverted());
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);
    QCOMPARE(document.selectionRect(), QRect(8, 8, 32, 32));
    QVERIFY(document.selectionInverted());
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.selectionInverted());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionInverted());
}

void KisGpuOperationDocumentTest::ellipticalSelectionPreservesHistory()
{
    OperationDocument document(QSize(64, 64));
    QCOMPARE(document.setSelectionEllipse(QRect(8, 8, 32, 24)), Result::Success);
    QCOMPARE(document.selectionRect(), QRect(8, 8, 32, 24));
    QCOMPARE(document.selectionKind(), SelectionKind::Ellipse);
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 32, 24)), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Rectangle);
    QCOMPARE(document.setSelectionEllipse(QRect(8, 8, 32, 24)), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Ellipse);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);
    QCOMPARE(document.selectionRect(), QRect(8, 8, 32, 24));
    QCOMPARE(document.selectionKind(), SelectionKind::Ellipse);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Rectangle);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Ellipse);
}

void KisGpuOperationDocumentTest::freehandSelectionFreezesCoverageHistory()
{
    OperationDocument document(QSize(64, 64));
    const QList<QPointF> polygon{{8, 8}, {56, 8}, {8, 56}};
    const auto mask = SelectionMask::fromPolygon(polygon, QRect(QPoint(), QSize(64, 64)));
    QVERIFY(mask);
    QVERIFY(mask->contains(12, 12));
    QVERIFY(!mask->contains(48, 48));
    QCOMPARE(document.setSelectionMask(mask), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Freehand);
    QCOMPARE(document.selectionRect(), mask->bounds());
    QVERIFY(document.selectionMask() == mask);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);

    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 5);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{16, 16}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->selectionKind, SelectionKind::Freehand);
    QVERIFY(pending->selectionMask == mask);
    QCOMPARE(document.commitStroke(id), Result::Success);

    QCOMPARE(document.setSelectionRect(QRect(0, 0, 8, 8)), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Rectangle);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.selectionKind(), SelectionKind::Freehand);
    QVERIFY(document.selectionMask() == mask);

    const auto retainedState = document.preview();
    QCOMPARE(document.selectionKind(), SelectionKind::Freehand);
    QCOMPARE(document.selectionRect(), mask->bounds());
    QVERIFY(document.selectionMask());
    QVERIFY(*document.selectionMask() == *mask);
    QVERIFY(document.selectionMask()->contains(12, 12));
    QVERIFY(!document.selectionMask()->contains(48, 48));
    QCOMPARE(document.preview(), retainedState);
    const auto restoredPaint = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(restoredPaint->selectionKind, SelectionKind::Freehand);
    QVERIFY(restoredPaint->selectionMask);
    QVERIFY(restoredPaint->selectionMask && *restoredPaint->selectionMask == *mask);

    QCOMPARE(document.setSelectionMask(mask, true), Result::Success);
    QVERIFY(document.selectionInverted());
    QVERIFY(*document.selectionMask() == *mask);

    QCOMPARE(document.setSelectionMask(SelectionMaskSP{}), Result::Success);
    QVERIFY(document.selectionRect().isEmpty());
    QVERIFY(!document.selectionMask());
    QCOMPARE(document.selectionKind(), SelectionKind::Rectangle);
}

void KisGpuOperationDocumentTest::featheredSelectionAddsAlphaHistory()
{
    const QRect clip(QPoint(), QSize(96, 96));
    OperationDocument document(QSize(96, 96));
    const QList<QPointF> polygon{{16, 16}, {80, 16}, {80, 80}, {16, 80}};
    const auto hard = SelectionMask::fromPolygon(polygon, clip);
    QVERIFY(hard);
    QVERIFY(!hard->isAlpha());
    QCOMPARE(document.setSelectionMask(hard), Result::Success);
    QCOMPARE(document.undoCount(), 1);

    const auto soft = hard->feathered(6, clip);
    QVERIFY(soft);
    QVERIFY(soft->isAlpha());
    QCOMPARE(soft->bounds(), QRect(10, 10, 76, 76));
    QCOMPARE(soft->coverage(48, 48), 255);
    const quint8 edge = soft->coverage(16, 48);
    const quint8 outer = soft->coverage(10, 48);
    QVERIFY(edge > 0 && edge < 255);
    QVERIFY(outer > 0 && outer < edge);
    QCOMPARE(hard->feathered(0, clip), SelectionMaskSP{});

    // Feathering is exactly one undoable selection history entry.
    QCOMPARE(document.setSelectionMask(soft), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.selectionKind(), SelectionKind::Freehand);
    QCOMPARE(document.selectionRect(), soft->bounds());
    QVERIFY(document.selectionMask() == soft);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);

    // A stroke freezes the alpha coverage at its start.
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 3);
    QVERIFY(!id.isNull());
    QCOMPARE(document.appendSamples(id, {{{24, 48}, 1}}), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QVERIFY(pending->selectionMask == soft);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 3);

    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == soft);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask() == hard);
    QVERIFY(!document.selectionMask()->isAlpha());
    QCOMPARE(document.selectionRect(), hard->bounds());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionMask()->isAlpha());
    QCOMPARE(document.selectionMask()->coverage(16, 48), edge);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.undoCount(), 3);

    // Retained versions preserve both representations and the frozen stroke.
    const auto retainedState = document.preview();
    QVERIFY(document.selectionMask());
    QVERIFY(document.selectionMask()->isAlpha());
    QVERIFY(*document.selectionMask() == *soft);
    QCOMPARE(document.selectionMask()->coverage(10, 48), outer);
    QCOMPARE(document.preview(), retainedState);
    const auto restoredPaint = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(restoredPaint->selectionMask);
    QVERIFY(restoredPaint->selectionMask->isAlpha());
    QVERIFY(*restoredPaint->selectionMask == *soft);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask());
    QVERIFY(document.selectionMask()->isAlpha());
    QVERIFY(*document.selectionMask() == *soft);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionMask());
    QVERIFY(!document.selectionMask()->isAlpha());
    QVERIFY(*document.selectionMask() == *hard);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionMask()->isAlpha());
    QVERIFY(*document.selectionMask() == *soft);

    // An inverted alpha selection keeps its fractional coverage and flag.
    QCOMPARE(document.setSelectionMask(soft, true), Result::Success);
    QVERIFY(document.selectionInverted());
    QVERIFY(document.selectionMask()->isAlpha());
    QVERIFY(*document.selectionMask() == *soft);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);
}

void KisGpuOperationDocumentTest::basicSmoothingRetainsIdleBoundariesSensorsAndFinalIntent()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.basicSmoothing = true;
    const auto id = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.flushSmoothing(id), Result::EmptyOperation);
    const QList<InputSample> samples{{{8, 32}, 0.4, 12, -9, 32, 0},
                                    {{32, 8}, 0.8, -12, 9, 64, 8},
                                    {{56, 32}, 1, 0, 0, 90, 16}};
    QCOMPARE(document.appendSamples(id, samples.first(2)), Result::Success);
    const auto beforeIdle = document.preview();
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    const auto afterIdle = document.preview();
    QVERIFY(afterIdle != beforeIdle);
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    QCOMPARE(document.preview(), afterIdle);
    QCOMPARE(document.appendSamples(id, samples.last(1)), Result::Success);
    const auto pending = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(pending->inputSamples(), samples);
    QCOMPARE(pending->smoothingFlushes, QList<qsizetype>{2});
    QVERIFY(!pending->smoothingFinished);
    QCOMPARE(document.undoCount(), 0);
    QVERIFY(pending->brush.basicSmoothing);
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto final = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(final->smoothingFinished);
    QVERIFY(!pending->smoothingFinished);
    QCOMPARE(final->inputSamples(), samples);
    QCOMPARE(final->smoothingFlushes, pending->smoothingFlushes);
    QCOMPARE(document.undoCount(), 1);
    const auto painted = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
}

void KisGpuOperationDocumentTest::weightedSmoothingRetainsFrozenSettingsSpeed()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.basicSmoothing = true;
    brush.weightedSmoothing = WeightedSmoothing{24, 48, 0.01, true};
    const auto id = document.beginStroke(document.activeLayer(), brush, 7);
    QVERIFY(!id.isNull());
    const QList<InputSample> samples{{{8, 16}, 1, 12, -9, 32, 0, 0.25},
                                    {{12, 20}, 0.8, -12, 9, 64, 8, 0.75}};
    QCOMPARE(document.appendSamples(id, samples), Result::Success);
    const auto pinned = document.preview();
    const auto operation = std::get<PaintSnapshot>(pinned->layers.first().operations.last());
    brush.weightedSmoothing->distanceMax = 96;
    QCOMPARE(operation->brush.weightedSmoothing->distanceMax, 48.0);
    QCOMPARE(operation->inputSamples(), samples);
    for (const auto speed : {-0.1, 1.1, std::numeric_limits<qreal>::quiet_NaN()}) {
        auto invalid = samples.last();
        invalid.speed = speed;
        QCOMPARE(document.appendSamples(id, {invalid}), Result::InvalidInput);
        QCOMPARE(document.preview(), pinned);
    }
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    const auto retained = std::get<PaintSnapshot>(document.preview()->layers.first().operations.last());
    QCOMPARE(retained->inputSamples(), samples);
    QVERIFY(retained->brush == operation->brush);
    QCOMPARE(retained->smoothingFlushes, QList<qsizetype>{2});
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    const auto painted = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QVERIFY(!operation->smoothingFinished);
    QCOMPARE(operation->inputSamples(), samples);
    const auto original = document.preview();
    for (int invalid = 0; invalid < 5; ++invalid) {
        auto rejected = operation->brush;
        if (invalid == 0) rejected.basicSmoothing = false;
        if (invalid == 1) rejected.pixelPerfect = true;
        if (invalid == 2) rejected.weightedSmoothing->distanceMin = -1;
        if (invalid == 3) rejected.weightedSmoothing->distanceMax = std::numeric_limits<qreal>::infinity();
        if (invalid == 4) rejected.weightedSmoothing->tailAggressiveness = 1.1;
        QVERIFY(document.beginStroke(document.activeLayer(), rejected, 7).isNull());
        QCOMPARE(document.preview(), original);
    }
}

void KisGpuOperationDocumentTest::basicSmoothingRejectsConflictingModesAndShapeChanges()
{
    OperationDocument document(QSize(64, 64));
    const auto original = document.preview();
    Brush brush;
    brush.basicSmoothing = brush.pixelPerfect = true;
    QVERIFY(document.beginStroke(document.activeLayer(), brush, 7).isNull());
    QCOMPARE(document.preview(), original);
    brush.pixelPerfect = false;
    const auto id = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(id, {{{8, 32}, 1}, {{32, 8}, 1}}), Result::Success);
    const auto pending = document.preview();
    QCOMPARE(document.updateShape(id, {ShapeKind::Line, {8, 32}, {32, 8}}), Result::InvalidInput);
    QCOMPARE(document.preview(), pending);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    const auto plain = document.beginStroke(document.activeLayer(), Brush{}, 8);
    QCOMPARE(document.appendSamples(plain, {{{8, 32}, 1}, {{32, 8}, 1}}), Result::Success);
    const auto plainPreview = document.preview();
    QCOMPARE(document.flushSmoothing(plain), Result::InvalidInput);
    QCOMPARE(document.preview(), plainPreview);
    QCOMPARE(document.cancelStroke(plain), Result::Success);
}

void KisGpuOperationDocumentTest::emptyPathsPreserveHistoryAndPendingInput()
{
    OperationDocument document(QSize(64, 64));
    Shape shape;
    shape.kind = ShapeKind::Path;
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, {}, shape, FigureStyle{}, 42), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(document.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 43);
    QVERIFY(!id.isNull());
    QCOMPARE(document.updateFigure(id, shape, FigureStyle{}), Result::Success);
    QVERIFY(!document.activeOperation().isNull());
    QCOMPARE(std::get<PaintSnapshot>(document.preview()->layers.first().operations.last())->shape.kind, ShapeKind::Path);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.cancelStroke(id), Result::Success);
}

void KisGpuOperationDocumentTest::invalidPathsPreserveHistoryAndInputBudgets()
{
    OperationDocument document(QSize(64, 64));
    const auto before = document.preview();
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::LineTo};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, {{{12, 32}, 1}}, shape, FigureStyle{}, 42), Result::InvalidInput);
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
    const QList<InputSample> samples{{{12, 32}, 1}, {{20, 12}, 1}, {{40, 52}, 1}, {{52, 32}, 1}};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples.first(3), shape, FigureStyle{}, 42), Result::InvalidInput);
    shape.pathCommands = {PathCommand::MoveTo, static_cast<PathCommand>(255)};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples.first(2), shape, FigureStyle{}, 42), Result::InvalidInput);
    shape.kind = ShapeKind::Line;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples.first(2), shape, FigureStyle{}, 42), Result::InvalidInput);
    QCOMPARE(document.preview(), before);
    QVERIFY(document.activeOperation().isNull());
    Limits limits;
    limits.samplesPerStroke = limits.samplesPerDocument = 3;
    OperationDocument bounded(QSize(64, 64), limits);
    const auto boundedBefore = bounded.preview();
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
    QCOMPARE(bounded.paintFigure(bounded.activeLayer(), Brush{}, samples, shape, FigureStyle{}, 42), Result::CapacityExceeded);
    QCOMPARE(bounded.preview(), boundedBefore);
    QVERIFY(bounded.activeOperation().isNull());
}

void KisGpuOperationDocumentTest::gaussianWashSettingsSurviveHistory()
{
    OperationDocument document(QSize(128, 128));
    Brush brush;
    brush.sizeResponse = {PressureResponse::Kind::Curve, 1, QList<qreal>(256, 0)};
    GaussianWash settings;
    settings.opacityResponse = {PressureResponse::Kind::Curve, 1, QList<qreal>(256, 0)};
    settings.angle = 6.28319;
    brush.opacity = .57;
    settings.flow = .5;
    settings.creamy = false;
    for (qsizetype i = 0; i < brush.sizeResponse.curve.size(); ++i) {
        brush.sizeResponse.curve[i] = qreal(i * i) / (255 * 255);
        settings.opacityResponse.curve[i] = qreal(i) / 255;
    }
    const auto size = brush.sizeResponse;
    brush.gaussianWash = settings;
    Shape shape;
    shape.kind = ShapeKind::Line;
    shape.first = {12.25, 13.75};
    shape.second = {60.125, 31.5};
    QCOMPARE(document.paintFigure(document.activeLayer(), brush,
        {sample(12.25, 13.75, .12), sample(60.125, 31.5, .75)}, shape, FigureStyle{}, 1789), Result::Success);
    brush.gaussianWash->flow = 1;
    brush.sizeResponse.curve[128] = 1;
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(operation->brush.gaussianWash == std::optional<GaussianWash>(settings));
    QVERIFY(operation->brush.sizeResponse == size);
    const auto retainedState = document.preview();
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    const auto retained = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(retained->brush == operation->brush);
    QCOMPARE(document.preview(), retainedState);
}

void KisGpuOperationDocumentTest::stationaryBrushCommitInvalidatesPendingPixelsAndRetainsInput_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("gaussian");
    for (int count : {1, 2, 3, 8, 1025}) for (const bool gaussian : {false, true})
        QTest::newRow(qPrintable((gaussian ? QStringLiteral("gaussian-") : QStringLiteral("marker-")) +
            QString::number(count))) << count << gaussian;
}

void KisGpuOperationDocumentTest::stationaryBrushCommitInvalidatesPendingPixelsAndRetainsInput()
{
    QFETCH(int, count);
    QFETCH(bool, gaussian);
    OperationDocument document(QSize(128, 128));
    Brush brush;
    if (gaussian) brush.gaussianWash.emplace();
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QList<InputSample> input;
    for (int i = 0; i < count; ++i)
        input.append({{12.25, 13.75}, qreal(i + 1) / count,
                      qreal(i % 91), -qreal(i % 91), qreal(i % 360), quint64(i * 16), qreal(i) / count});
    QCOMPARE(document.appendSamples(id, input), Result::Success);
    const auto pending = document.preview();
    const auto operation = std::get<PaintSnapshot>(pending->layers.first().operations.last());
    const auto intent = document.preview();
    const auto before = std::get<PaintSnapshot>(intent->layers.first().operations.last());
    QCOMPARE(document.commitStroke(document.activeOperation()), Result::Success);
    const auto after = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    // Pixel caches distinguish the empty pending stroke from its final tap
    // by the operation version, while retained input remains immutable.
    QVERIFY(after->version > before->version);
    QCOMPARE(after->id, before->id);
    QCOMPARE(after->inputSamples(), input);
    QCOMPARE(before->inputSamples(), input);
    QCOMPARE(intent->pendingOperation(), before->id);
    QVERIFY(document.snapshot()->pendingOperation().isNull());
    QCOMPARE(document.undoCount(), 1);

    QCOMPARE(operation->inputSamples(), input);
    QCOMPARE(pending->pendingOperation(), id);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
}

void KisGpuOperationDocumentTest::transparencyMaskHistoryComposition()
{
    const QSize size(64, 64);
    const QRect clip(QPoint(), size);
    const QRect selection(8, 8, 32, 24);
    OperationDocument document(size);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(selection), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.historyAction(2), HistoryAction::AddTransparencyMask);
    const auto expected = SelectionMask::fromRectangle(selection, clip);
    QVERIFY(expected);
    const auto mask = document.layerTransparencyMask(layer);
    QVERIFY(mask);
    QVERIFY(*mask == *expected);
    QVERIFY(document.layerTransparencyMaskVisible(layer));

    // Painting retains the frozen mask and never synchronizes it per stroke.
    const auto stroke = document.beginStroke(layer, Brush{}, 7);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {{{16, 16}, 1}, {{40, 20}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.historyAction(3), HistoryAction::PaintStroke);
    QVERIFY(document.layerTransparencyMask(layer) == mask);
    QVERIFY(document.layerTransparencyMaskVisible(layer));

    QCOMPARE(document.setTransparencyMaskVisible(layer, false), Result::Success);
    QCOMPARE(document.historyAction(4), HistoryAction::ChangeMaskVisibility);
    QVERIFY(!document.layerTransparencyMaskVisible(layer));
    QVERIFY(document.layerTransparencyMask(layer) == mask);
    QCOMPARE(document.setTransparencyMaskVisible(layer, false), Result::Success);
    QCOMPARE(document.undoCount(), 4);

    QCOMPARE(document.removeTransparencyMask(layer), Result::Success);
    QCOMPARE(document.historyAction(5), HistoryAction::RemoveTransparencyMask);
    QVERIFY(!document.layerTransparencyMask(layer));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == mask);
    QVERIFY(!document.layerTransparencyMaskVisible(layer));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMaskVisible(layer));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.layerTransparencyMask(layer));
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == mask);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.layerTransparencyMask(layer));
    QCOMPARE(document.undoCount(), 5);

    // History retains masks on every version, their visibility and the
    // add/toggle/remove history actions.
    const auto retainedState = document.preview();
    QCOMPARE(document.undoCount(), 5);
    QCOMPARE(document.redoCount(), 0);
    QVERIFY(!document.layerTransparencyMask(document.snapshot()->layers.first().id));
    QCOMPARE(document.historyAction(2), HistoryAction::AddTransparencyMask);
    QCOMPARE(document.historyAction(3), HistoryAction::PaintStroke);
    QCOMPARE(document.historyAction(4), HistoryAction::ChangeMaskVisibility);
    QCOMPARE(document.historyAction(5), HistoryAction::RemoveTransparencyMask);
    QCOMPARE(document.undo(), Result::Success);
    const auto restoredLayer = document.snapshot()->layers.first().id;
    QVERIFY(document.layerTransparencyMask(restoredLayer));
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *mask);
    QVERIFY(!document.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.layerTransparencyMask(restoredLayer));
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *mask);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.layerTransparencyMask(restoredLayer));
    QCOMPARE(document.undoCount(), 5);
    QCOMPARE(document.preview(), retainedState);
}

void KisGpuOperationDocumentTest::transparencyMaskFollowsLayerCrop()
{
    const QSize size(64, 64);
    const QRect selection(8, 8, 32, 24);
    OperationDocument document(size);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(selection), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    const auto original = document.layerTransparencyMask(layer);
    QVERIFY(original);

    const QRect cropped(16, 16, 32, 32);
    QCOMPARE(document.cropLayer(layer, cropped), Result::Success);
    const auto clipped = document.layerTransparencyMask(layer);
    QVERIFY(clipped);
    QVERIFY(clipped->contains(16, 16));
    QVERIFY(!clipped->contains(8, 8));
    QVERIFY(!clipped->contains(40, 40));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == original);
}

void KisGpuOperationDocumentTest::transparencyMaskFollowsCanvasResize()
{
    const QSize size(64, 64);
    const QRect clip(QPoint(), size);
    const QRect selection(8, 8, 32, 24);
    OperationDocument document(size);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(selection), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    const auto original = document.layerTransparencyMask(layer);
    QVERIFY(original);

    const QRect resized(8, 8, 40, 40);
    QCOMPARE(document.resizeCanvas(resized, true), Result::Success);
    const auto moved = document.layerTransparencyMask(layer);
    QVERIFY(moved);
    QVERIFY(moved->contains(0, 0));
    QVERIFY(moved->contains(23, 15));
    QVERIFY(!moved->contains(24, 24));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->size, size);
    QVERIFY(document.layerTransparencyMask(layer) == original);
}

void KisGpuOperationDocumentTest::transparencyMaskFromSelectionShapesAndRejections()
{
    const QSize size(96, 96);
    const QRect clip(QPoint(), size);

    {
        OperationDocument document(size);
        const auto layer = document.activeLayer();
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::InvalidInput);
        QCOMPARE(document.setSelectionEllipse(QRect(16, 16, 48, 32)), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
        const auto expected = SelectionMask::fromEllipse(QRect(16, 16, 48, 32), clip);
        QVERIFY(expected);
        QVERIFY(document.layerTransparencyMask(layer));
        QVERIFY(*document.layerTransparencyMask(layer) == *expected);
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::InvalidInput);
        QCOMPARE(document.removeTransparencyMask(layer), Result::Success);
        QVERIFY(!document.layerTransparencyMask(layer));
        QCOMPARE(document.removeTransparencyMask(layer), Result::LayerUnavailable);
        QCOMPARE(document.setTransparencyMaskVisible(layer, true), Result::LayerUnavailable);
        QCOMPARE(document.undo(), Result::Success);
        QVERIFY(document.layerTransparencyMask(layer));
        QCOMPARE(document.undo(), Result::Success);
        QVERIFY(!document.layerTransparencyMask(layer));
        QCOMPARE(document.redo(), Result::Success);
        QVERIFY(document.layerTransparencyMask(layer));
        QCOMPARE(document.redo(), Result::Success);
        QVERIFY(!document.layerTransparencyMask(layer));
    }

    {
        // Alpha selection coverage becomes the mask's fractional alpha.
        OperationDocument document(size);
        const auto layer = document.activeLayer();
        const QRect bounds(20, 18, 28, 24);
        QByteArray alpha(bounds.width() * bounds.height(), '\0');
        for (int y = 0; y < bounds.height(); ++y)
            for (int x = 0; x < bounds.width(); ++x)
                alpha[y * bounds.width() + x] = char(quint8((x * 255) / (bounds.width() - 1)));
        const auto soft = SelectionMask::fromAlphaCoverage(bounds, alpha, clip);
        QVERIFY(soft && soft->isAlpha());
        QCOMPARE(document.setSelectionMask(soft), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
        QVERIFY(document.layerTransparencyMask(layer));
        QVERIFY(document.layerTransparencyMask(layer) == soft);
        QCOMPARE(document.layerTransparencyMask(layer)->coverage(20, 18), soft->coverage(20, 18));
        QCOMPARE(document.layerTransparencyMask(layer)->coverage(47, 18), soft->coverage(47, 18));
        QCOMPARE(document.undo(), Result::Success);
        QCOMPARE(document.undo(), Result::Success);
        QCOMPARE(document.setSelectionMask(soft, true), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
        const auto expected = soft->inverted(clip);
        QVERIFY(expected);
        QVERIFY(*document.layerTransparencyMask(layer) == *expected);
        QVERIFY(document.layerTransparencyMask(layer)->coverage(0, 0) > 0);
        QCOMPARE(document.layerTransparencyMask(layer)->coverage(34, 30),
                 quint8(255u - soft->coverage(34, 30)));
    }

    {
        // Inverted rectangular selections become the complement coverage.
        OperationDocument document(size);
        const auto layer = document.activeLayer();
        const QRect hole(24, 20, 16, 24);
        QCOMPARE(document.setSelectionRect(hole, true), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
        const auto expected = SelectionMask::fromRectangle(hole, clip)->inverted(clip);
        QVERIFY(expected);
        QVERIFY(*document.layerTransparencyMask(layer) == *expected);
        QCOMPARE(document.layerTransparencyMask(layer)->coverage(0, 0), quint8(255));
        QCOMPARE(document.layerTransparencyMask(layer)->coverage(30, 30), quint8(0));
    }

    {
        // Only unlocked paint layers own a transparency mask.
        OperationDocument document(nestedImage());
        const auto group = std::find_if(document.snapshot()->layers.cbegin(), document.snapshot()->layers.cend(),
            [](const Layer &layer) { return layer.kind == LayerKind::Group; });
        QVERIFY(group != document.snapshot()->layers.cend());
        QCOMPARE(document.setSelectionRect(QRect(4, 4, 16, 16)), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(group->id), Result::LayerUnavailable);
        const auto paint = document.activeLayer();
        QCOMPARE(document.setLayerLocked(paint, true), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(paint), Result::LayerUnavailable);
        QCOMPARE(document.setLayerLocked(paint, false), Result::Success);
        QCOMPARE(document.addTransparencyMaskFromSelection(paint), Result::Success);
    }
}

void KisGpuOperationDocumentTest::transparencyMaskCoverageReplacementPreservesHistory()
{
    const QSize size(96, 96);
    const QRect clip(QPoint(), size);
    OperationDocument document(size);
    const auto layer = document.activeLayer();
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 32, 32)), Result::Success);
    QCOMPARE(document.addTransparencyMaskFromSelection(layer), Result::Success);
    const auto original = document.layerTransparencyMask(layer);
    QVERIFY(original);
    const auto operationsBefore = document.snapshot()->layers.first().operations;
    const int undoBefore = document.undoCount();

    // Editing coverage leaves layer inputs untouched and appends exactly one
    // undoable version; the history action is derived from the changed mask.
    const QRect bounds(40, 36, 24, 20);
    QByteArray alpha(bounds.width() * bounds.height(), '\0');
    for (int y = 0; y < bounds.height(); ++y)
        for (int x = 0; x < bounds.width(); ++x)
            alpha[y * bounds.width() + x] = char(quint8(1 + (x * 253) / (bounds.width() - 1)));
    const auto edited = SelectionMask::fromAlphaCoverage(bounds, alpha, clip);
    QVERIFY(edited && edited->isAlpha());
    QCOMPARE(document.replaceTransparencyMask(layer, edited), Result::Success);
    QCOMPARE(document.undoCount(), undoBefore + 1);
    QCOMPARE(document.historyAction(undoBefore + 1), HistoryAction::PaintStroke);
    QVERIFY(document.layerTransparencyMask(layer) == edited);
    QVERIFY(document.layerTransparencyMaskVisible(layer));
    QCOMPARE(document.snapshot()->layers.first().operations, operationsBefore);
    QCOMPARE(document.replaceTransparencyMask(layer, edited), Result::Success);
    QCOMPARE(document.undoCount(), undoBefore + 1);
    QCOMPARE(document.replaceTransparencyMask(layer, SelectionMaskSP{}), Result::InvalidInput);
    QCOMPARE(document.replaceTransparencyMask(layer + 1, edited), Result::LayerUnavailable);

    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == original);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == edited);

    // A fill records its own history kind while retaining the same mask version.
    QCOMPARE(document.replaceTransparencyMask(layer, original, true), Result::Success);
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::FillLayer);
    QVERIFY(document.layerTransparencyMask(layer) == original);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == edited);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.layerTransparencyMask(layer) == original);

    const auto retainedState = document.preview();
    const auto restoredLayer = document.snapshot()->layers.first().id;
    QVERIFY(document.layerTransparencyMask(restoredLayer));
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(document.historyAction(undoBefore + 1), HistoryAction::PaintStroke);
    QCOMPARE(document.historyAction(undoBefore + 2), HistoryAction::FillLayer);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *edited);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(*document.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(document.preview(), retainedState);
}

void KisGpuOperationDocumentTest::invalidGaussianWashSettingsPreserveTheDocument()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.sizeResponse = {PressureResponse::Kind::Curve, 1, QList<qreal>(256, 1)};
    auto &wash = brush.gaussianWash.emplace();
    wash.opacityResponse = {PressureResponse::Kind::Curve, 1, QList<qreal>(256, 1)};
    const auto before = document.preview();
    for (int invalid = 0; invalid < 6; ++invalid) {
        auto bad = brush;
        switch (invalid) {
        case 0: bad.gaussianWash->flow = -1; break;
        case 1: bad.gaussianWash->angle = std::numeric_limits<qreal>::infinity(); break;
        case 2: bad.sizeResponse.curve[128] = std::numeric_limits<qreal>::quiet_NaN(); break;
        case 3: bad.gaussianWash->opacityResponse.curve[128] = 2; break;
        case 4: bad.sizeResponse.curve.removeLast(); break;
        case 5: bad.gaussianWash->opacityResponse.curve.append(1); break;
        }
        QVERIFY(document.beginStroke(document.activeLayer(), bad, 1789).isNull());
        QCOMPARE(document.preview(), before);
    }
}

QTEST_GUILESS_MAIN(KisGpuOperationDocumentTest)
#include "KisGpuOperationDocumentTest.moc"
