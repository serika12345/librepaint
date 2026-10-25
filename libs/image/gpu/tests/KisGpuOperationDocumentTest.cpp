/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <limits>
#include <stdexcept>

#include <gpu/KisGpuOperationDocument.h>
#include "KisGpuLegacyRecoveryFixture_p.h"
#include "KisGpuMultiplyProjectionFixture_p.h"
#include "KisGpuScreenProjectionFixture_p.h"
#include "KisGpuMinMaxProjectionFixture_p.h"
#include "KisGpuAdditionProjectionFixture_p.h"

using namespace LibrePaint::Image::Gpu;
using LibrePaint::Image::Gpu::Test::legacyRecovery;

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
    void rejectedLayerForestsPreservePendingInputAndHistory();
    void nestedLayerIdentitySurvivesPropertiesHistoryAndRecovery();
    void colorLabelsPreserveFixedVersionsPendingInputAndRecovery();
    void projectionChannelsPreservePixelsPendingInputHistoryAndRecovery();
    void groupPassThroughPreservesPinnedInputHistoryAndRecovery();
    void groupPassThroughPreviewsAdoptAndCancelOneAttribute();
    void importedAndCopiedGroupsKeepPassThroughModes();
    void layerBlendModesKeepPixelsHistoryPendingInputAndRecovery_data();
    void layerBlendModesKeepPixelsHistoryPendingInputAndRecovery();
    void layerBlendModePreviewsAdoptAndCancelOneAttribute_data();
    void layerBlendModePreviewsAdoptAndCancelOneAttribute();
    void separableGroupDefaultsMatchNativeProjection();
    void layerPropertyPreviewsBecomeOneReversibleEdit();
    void rejectedLayerPropertyPreviewsRetainHistoryAndInput();
    void layerPropertyAcceptanceRetainsEditsCompletedDuringPreview();
    void layerPropertyCancellationKeepsExactValuesPendingInputAndRedo();
    void directHierarchyMovesAdoptTheirActiveRootAtomically();
    void movedSubtreesPreservePixelsSelectionHistoryAndRecovery_data();
    void movedSubtreesPreservePixelsSelectionHistoryAndRecovery();
    void rejectedHierarchyMovesPreserveRedoInputAndClosedState();
    void nestedRemovalOwnsDescendantsAndOneHistoryStep();
    void nestedDuplicationCopiesSubtreePixelsAndIndependentParents();
    void hierarchyDuplicationPreservesPlacementSelectionAndRecovery_data();
    void hierarchyDuplicationPreservesPlacementSelectionAndRecovery();
    void nestedCanvasChangesKeepGroupsDerivedAndRecoverable();
    void previousShiftedWritesRecoverPixelsAndPendingInput();
    void groupProjectionDefaultsRespectIsolationAndQuantizedOpacity_data();
    void groupProjectionDefaultsRespectIsolationAndQuantizedOpacity();
    void previousCanvasRecoveryPreservesSizesAndInput();
    void canvasResizePreservesVersionSizesPropertiesAndRecovery();
    void canvasResizeKeepsEmptyDocumentsRecoverable();
    void directLayerPositionKeepsCanvasHistoryBoundaries();
    void rejectedCanvasResizesKeepRedoPendingInputAndClosedState();
    void previousCroppedRecoveryPreservesInputAndCanvasHistory();
    void layerCropPreservesValuesHistoryAndRecovery();
    void rejectedLayerCropsKeepRedoPendingInputAndClosedState();
    void invertColorsFreezesCoverageAndRecovers();
    void rejectedInvertColorsPreserveHistoryAndInput();
    void grayscaleFilterFreezesCoverageAndRecovers();
    void grayscaleRecoveryRequiresFormat51();
    void previousTranslatedRecoveryPreservesPendingInputAndCropHistory();
    void layerTranslationPreservesInputVersionsAndOneHistoryEntry();
    void previousPlacedRecoveryPreservesPendingInputAndPosition();
    void groupedRemovalPreservesPixelsSelectionAndAtomicHistory();
    void insertedLayersPreserveOrderPinsAndOneHistoryStep();
    void nestedPaintInsertionRejectsMissingAndNonGroupParents();
    void emptyGroupInsertionPreservesHierarchyHistoryAndRejectsSubtrees();
    void duplicatedLayersShareInputsAndPixelsWithIndependentHistory();
    void identicalLayerDuplicationsPreserveTheSamePersistentIdentities();
    void insertedForeignLayersSharePixelsAndInputsWithAtomicHistory();
    void selectionDuringInputPreservesPaintingAndHistory();
    void activeLayerAndSelectionRemainIndependentThroughRecoveryAndInput();
    void mergedPanelTogglesPreservePinsAndSaveBoundaries_data();
    void mergedPanelTogglesPreservePinsAndSaveBoundaries();
    void historyActionsSurviveDirectWritesMergingAndRecovery();
    void mergedPropertiesPreservePinsSaveBoundariesAndRecovery();
    void mergedNameResetRetainsItsRecordedBoundary();
    void mergedOpacityResetPreservesSelectionAndDirectWrites();
    void previousPropertyRecoveryRemainsReadable_data();
    void previousPropertyRecoveryRemainsReadable();
    void previousPositionlessRecoveryPreservesLayerEdits();
    void directLayerPropertiesPreserveOtherPropertyHistoryAndFixedVersions();
    void propertyRecoveryPreservesRecordedValuesAndRejectsDamagedChanges();
    void failedHistoryMovesPreserveEveryVersionAndCounter();
    void layerOpacityAndLockShareHistoryAndRecovery();
    void channelProtectionPreservesFrozenSettingsHistoryAndRecovery();
    void symmetryFreezesAxesAndRejectsInvalidRecoveryAtomically();
    void inputChunksRetainTheSameOperation();
    void shapePreviewCommitsAsOneHistoryEntry();
    void filledFiguresPreserveStylesHistoryAndRecovery();
    void fillBlendModesFreezeHistoryAndRecovery_data();
    void fillBlendModesFreezeHistoryAndRecovery();
    void strokeBlendModesFreezeHistoryAndRecovery_data();
    void strokeBlendModesFreezeHistoryAndRecovery();
    void rejectedStrokeBlendModesPreserveHistoryAndInput();
    void patternFillBlendModeFreezesHistoryAndRecovery();
    void rejectedFillBlendModesPreserveHistoryAndInput();
    void patternFillFreezesPixelsHistoryAndRecovery();
    void emptyFigureIntentPreservesHistoryAndRecovery();
    void pathsPreserveCommandsControlPointsAndRecovery();
    void pathCallsRetainInputHistoryAndRecovery();
    void invalidPathCallsPreserveTheActiveDocument_data();
    void invalidPathCallsPreserveTheActiveDocument();
    void legacyGaussianPathKeepsItsRecordedInput();
    void damagedPathCallRecoveryPreservesTheActiveDocument();
    void pixelCorrectionRetainsOriginalSensorsAndPendingRecovery();
    void selectionRectIsFrozenInOperationsAndRecovery();
    void invertedSelectionSurvivesRecovery();
    void ellipticalSelectionSurvivesRecovery();
    void freehandSelectionFreezesCoverageHistoryAndRecovery();
    void featheredSelectionAddsAlphaHistoryAndRecovery();
    void basicSmoothingRetainsIdleBoundariesSensorsAndFinalIntent();
    void weightedSmoothingRetainsFrozenSettingsSpeedAndRecovery();
    void basicSmoothingRejectsConflictingModesAndShapeChanges();
    void damagedSmoothingRecoveryPreservesTheActiveDocument();
    void emptyPathsPreserveHistoryAndPendingRecovery();
    void invalidPathsPreserveHistoryAndInputBudgets();
    void gaussianWashSettingsSurviveHistoryAndRecovery();
    void stationaryGaussianCommitInvalidatesPendingPixelsAndRetainsInput_data();
    void stationaryGaussianCommitInvalidatesPendingPixelsAndRetainsInput();
    void invalidGaussianWashSettingsPreserveTheDocument();
    void transparencyMaskHistoryCompositionAndRecovery();
    void transparencyMaskFromSelectionShapesAndRejections();
    void transparencyMaskCoverageReplacementSurvivesRecovery();
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
    void recoveryPreservesHistoryAndUncommittedIntent();
    void invalidRecoveryLeavesTheLiveDocumentUntouched();
    void checkedRecoveryRejectsInvalidCountsAndValues();
    void importedPixelsPreserveLayersAndStartWithoutUndo();
    void tileReplacementSharesHistoryAndRejectsWholeRequests();
    void rasterRecoveryPreservesPixelsIdentityAndPendingInput();
    void damagedRasterRecoveryPreservesTheLiveDocument();
    void layerStructureSharesPaintingHistoryAndPreservesPins();
    void rejectedLayerChangesKeepRedoAndPendingInput();
    void removingTheLastLayerPreservesAnEmptyRecoverableDocument();
    void layerRemovalReusesCapacityWithoutLosingRecoveryHistory_data();
    void layerRemovalReusesCapacityWithoutLosingRecoveryHistory();
    void layerReorderingIsAtomicAndPreservesSelectionPixelsAndHistory();
};

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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(destination.recoveryData()), Result::Success);
    for (auto *value : {&destination, &recovered}) {
        QCOMPARE(value->undo(), Result::Success);
        QCOMPARE(value->snapshot()->activeLayer, before->activeLayer);
        QCOMPARE(value->snapshot()->selectedLayers, before->selectedLayers);
        QCOMPARE(value->snapshot()->layers.size(), before->layers.size());
        QCOMPARE(value->redo(), Result::Success);
        QCOMPARE(value->snapshot()->activeLayer, after->activeLayer);
        QCOMPARE(value->snapshot()->selectedLayers, after->selectedLayers);
        for (qsizetype i = 0; i < after->layers.size(); ++i)
            QCOMPARE(value->snapshot()->layers[i].uuid, after->layers[i].uuid);
    }
    QCOMPARE(destination.undo(), Result::Success);
    QCOMPARE(destination.snapshot(), before);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.snapshot(), after);
    QCOMPARE(fixed->layers.size(), 3);
    QCOMPARE(before->layers.size(), 3);
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
    const auto bytes = destination.recoveryData();
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
        QCOMPARE(destination.recoveryData(), bytes);
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
        const auto retained = bounded.recoveryData();
        QCOMPARE(bounded.insertLayers(inputs, 0), Result::CapacityExceeded);
        QCOMPARE(bounded.recoveryData(), retained);
    }
    destination.close();
    const auto closed = destination.snapshot();
    QCOMPARE(destination.insertLayers(inputs, 0), Result::Closed);
    QCOMPARE(destination.snapshot(), closed);
}

void KisGpuOperationDocumentTest::hierarchyDuplicationPreservesPlacementSelectionAndRecovery_data()
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

void KisGpuOperationDocumentTest::hierarchyDuplicationPreservesPlacementSelectionAndRecovery()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->selectedLayers, copies);
    QCOMPARE(recovered.snapshot()->activeLayer, copies.first());
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers.size(), before->layers.size());
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers.size(), after->layers.size());
    for (int i = 0; i < after->layers.size(); ++i) {
        QCOMPARE(recovered.snapshot()->layers[i].uuid, after->layers[i].uuid);
        QCOMPARE(recovered.snapshot()->layers[i].parentUuid, after->layers[i].parentUuid);
    }
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
    const auto recovery = document.recoveryData();
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 1), Result::InvalidInput);
    QCOMPARE(document.moveLayers({3, 1}, {}, top.uuid, 99), Result::LayerUnavailable);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
    QCOMPARE(document.moveLayers({3, 1}, {}, {}, 3), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    for (auto *value : {&document, &recovered}) {
        QCOMPARE(value->undo(), Result::Success);
        QCOMPARE(value->snapshot()->activeLayer, LayerId(1));
        QCOMPARE(value->snapshot()->selectedLayers, QList<LayerId>{3});
        QVERIFY(!value->isModified());
        QCOMPARE(value->redo(), Result::Success);
        QCOMPARE(value->snapshot()->activeLayer, LayerId(3));
        QCOMPARE(value->snapshot()->selectedLayers, QList<LayerId>{3});
        QVERIFY(value->isModified());
    }
    QCOMPARE(document.snapshot(), after);
    QCOMPARE(before->activeLayer, LayerId(1));
    QCOMPARE(before->selectedLayers, (QList<LayerId>{1, 3}));
}

void KisGpuOperationDocumentTest::movedSubtreesPreservePixelsSelectionHistoryAndRecovery_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<int>("layout");
    for (const auto &scenario : {QStringLiteral("group"), QStringLiteral("enter"), QStringLiteral("leave"),
                                QStringLiteral("cross-parent"), QStringLiteral("ancestor-and-child"),
                                QStringLiteral("parent-only"), QStringLiteral("locked-source")})
        for (int layout = 0; layout < 3; ++layout)
            QTest::newRow(qPrintable(scenario + QString::number(layout))) << scenario << layout;
}

void KisGpuOperationDocumentTest::movedSubtreesPreservePixelsSelectionHistoryAndRecovery()
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
    OperationDocument recovered({1, 1});
    const auto bytes = document.recoveryData();
    QCOMPARE(recovered.restoreRecovery(bytes), Result::Success);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.historyAction(1), HistoryAction::MoveLayer);
    for (auto *value : {&document, &recovered}) {
        QCOMPARE(value->undo(), Result::Success);
        QCOMPARE(hierarchy(value->snapshot()), hierarchy(before));
        QCOMPARE(value->snapshot()->activeLayer, active);
        QCOMPARE(value->snapshot()->selectedLayers.size(), roots.size());
        for (const auto root : roots) QVERIFY(value->snapshot()->selectedLayers.contains(root));
        QVERIFY(!value->isModified());
        QCOMPARE(value->redo(), Result::Success);
        QCOMPARE(hierarchy(value->snapshot()), expected);
        QCOMPARE(value->snapshot()->activeLayer, active);
    }
    QCOMPARE(document.snapshot(), after);
    QCOMPARE(hierarchy(before), hierarchy(initial));
    QCOMPARE(before->selectedLayers.size(), requested.size());
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
    const auto recovery = document.recoveryData();
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
    QCOMPARE(document.recoveryData(), recovery);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.moveLayers({1}, inner, {}), Result::Success);
    QCOMPARE(document.snapshot(), before);
    QCOMPARE(document.recoveryData(), recovery);
    const auto stroke = document.beginStroke(1, Brush{}, 42);
    QVERIFY(!stroke.isNull());
    QCOMPARE(document.appendSamples(stroke, {sample(5, 6)}), Result::Success);
    const auto preview = document.preview();
    const auto pendingRecovery = document.recoveryData();
    QCOMPARE(document.moveLayers({1}, outer, {}), Result::EditInProgress);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.recoveryData(), pendingRecovery);
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

void KisGpuOperationDocumentTest::canvasResizePreservesVersionSizesPropertiesAndRecovery()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->size, canvas.size());
    QCOMPARE(recovered.savedHistoryIndex(), 1);
    QCOMPARE(recovered.historyCanvasResize(2)->rect, crop);
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, crop.size());
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, source.size);
    QCOMPARE(recovered.snapshot()->layers.first().position, source.layers.first().position);
    auto damaged = document.recoveryData().chopped(32);
    QByteArray prefix;
    QDataStream writer(&prefix, QIODevice::WriteOnly);
    writer.setVersion(QDataStream::Qt_5_15);
    writer << qint32(before->size.width()) << qint32(before->size.height());
    for (const auto component : before->projectionBackground) writer << component;
    writer << before->revision() << before->historyRevision();
    const auto offset = damaged.indexOf(prefix);
    QVERIFY(offset > 0);
    damaged.replace(offset, 4, QByteArray(4, '\0'));
    damaged.append(QCryptographicHash::hash(damaged, QCryptographicHash::Sha256));
    const auto unchanged = recovered.recoveryData();
    QCOMPARE(recovered.restoreRecovery(damaged), Result::InvalidRecovery);
    QCOMPARE(recovered.recoveryData(), unchanged);
}

void KisGpuOperationDocumentTest::canvasResizeKeepsEmptyDocumentsRecoverable()
{
    RasterImage source;
    source.size = {64, 48};
    OperationDocument document(source);
    QCOMPARE(document.resizeCanvas({-8, -6, 128, 96}, true), Result::Success);
    QVERIFY(document.snapshot()->layers.isEmpty());
    QCOMPARE(document.snapshot()->size, QSize(128, 96));
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, source.size);
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, QSize(128, 96));
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers.first().position, QPoint());
}

void KisGpuOperationDocumentTest::rejectedCanvasResizesKeepRedoPendingInputAndClosedState()
{
    OperationDocument document({64, 48});
    QCOMPARE(document.resizeCanvas({8, 6, 32, 24}, true), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    const auto before = document.snapshot();
    const auto recovery = document.recoveryData();
    for (const QRect rect : {QRect(0, 0, 0, 4), QRect(0, 0, -1, 4),
                             QRect(RgbaTile::CoordinateLimit, 0, 1, 1),
                             QRect(-RgbaTile::CoordinateLimit - 1, 0, 1, 1)}) {
        QCOMPARE(document.resizeCanvas(rect, false), Result::InvalidInput);
        QCOMPARE(document.recoveryData(), recovery);
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
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(innerValue.data()), 4), innerPixel);
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(outerValue.data()), 4), outerPixel);
}

void KisGpuOperationDocumentTest::previousShiftedWritesRecoverPixelsAndPendingInput()
{
    const auto record = LibrePaint::Image::Gpu::Test::shiftedPixelRecoveryV28();
    QCOMPARE(QCryptographicHash::hash(record, QCryptographicHash::Sha256).toHex(),
             QByteArrayLiteral("58b55eea74b81547c4710c318601eb4398ed25fd225bcfe0858b71e8cec0ab01"));
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(record), Result::Success);
    const auto layer = document.snapshot()->layers.first();
    QCOMPARE(layer.kind, LayerKind::Paint);
    QVERIFY(layer.parentUuid.isNull());
    QCOMPARE(layer.position, QPoint(7, -9));
    TileWriteSnapshot write;
    for (const auto &operation : layer.operations)
        if (const auto pixels = std::get_if<TileWriteSnapshot>(&operation)) write = *pixels;
    QVERIFY(write);
    QRect written;
    int pixels = 0;
    for (const auto &area : write->writtenAreas) {
        written = written.united(area);
        pixels += area.width() * area.height();
    }
    QCOMPARE(written, QRect(0, 0, 256, 256));
    QCOMPARE(pixels, 256 * 256);
    QCOMPARE(write->tiles.first()->coordinate(), QPoint(0, 0));
    QCOMPARE(write->tiles.first()->rgba(), QByteArray::fromHex("11223300").repeated(256 * 256));
    QVERIFY(document.hasActiveStroke());
    QCOMPARE(document.cancelStroke(document.activeOperation()), Result::Success);
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].parentUuid, fixed->layers[1].uuid);
    QCOMPARE(recovered.removeLayers({3}, QStringLiteral("Replacement")), Result::Success);
    QCOMPARE(recovered.snapshot()->layers.size(), 1);
    QCOMPARE(recovered.snapshot()->layers.first().kind, LayerKind::Paint);
    QVERIFY(recovered.snapshot()->layers.first().parentUuid.isNull());
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    Limits limits;
    limits.layerCount = 4;
    OperationDocument bounded(nestedImage(), limits);
    const auto unchanged = bounded.recoveryData();
    QCOMPARE(bounded.duplicateLayers({2}, {QStringLiteral("Copy")}, 2), Result::CapacityExceeded);
    QCOMPARE(bounded.recoveryData(), unchanged);
}

void KisGpuOperationDocumentTest::nestedCanvasChangesKeepGroupsDerivedAndRecoverable()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, fixed->size);
    QCOMPARE(recovered.redo(), Result::Success);
}

void KisGpuOperationDocumentTest::groupPassThroughPreservesPinnedInputHistoryAndRecovery()
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
    const auto bytes = document.recoveryData();
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile artifact(QDir(results).filePath(QStringLiteral("group-pass-through.recovery")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
    }
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(bytes), Result::Success);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.activeOperation().sequence, stroke.sequence);
    QVERIFY(recovered.preview()->layers[1].passThrough);
    QVERIFY(recovered.preview()->layers[2].passThrough);
    QCOMPARE(recovered.cancelStroke(recovered.activeOperation()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(!recovered.snapshot()->layers[1].passThrough);
    QVERIFY(recovered.snapshot()->layers[2].passThrough);
    QCOMPARE(recovered.redo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers[1].passThrough);
    const auto current = recovered.recoveryData();
    for (int index = 0; index < 2; ++index) {
        const auto &layer = recovered.snapshot()->layers[index];
        QByteArray prefix;
        QDataStream stream(&prefix, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_15);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << layer.uuid;
        for (const auto component : layer.defaultPixel) stream << component;
        stream << layer.opacity << quint8(layer.userLocked)
               << quint8(quint8(layer.pinnedToTimeline) | ((15u ^ layer.channelWriteMask) << 1))
               << qint32(layer.position.x()) << qint32(layer.position.y()) << quint8(layer.kind)
               << layer.parentUuid << qint32(layer.colorLabelIndex) << layer.projectionChannelMask;
        auto invalid = current.chopped(32);
        const auto offset = invalid.indexOf(prefix);
        QVERIFY(offset >= 0);
        invalid[offset + prefix.size()] = index == 0 ? 1 : 2;
        invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
        QCOMPARE(recovered.restoreRecovery(invalid), Result::InvalidRecovery);
        QCOMPARE(recovered.recoveryData(), current);
    }
    QCOMPARE(recovered.writeLayerPassThrough(1, true), Result::LayerUnavailable);
    QCOMPARE(recovered.setLayerPassThrough(999, true), Result::LayerUnavailable);
    QCOMPARE(recovered.recoveryData(), current);
    OperationDocument previous({1, 1});
    QCOMPARE(previous.restoreRecovery(LibrePaint::Image::Gpu::Test::layerPropertiesRecoveryV32()), Result::Success);
    for (const auto &state : previous.historyVersions())
        for (const auto &layer : state->layers) QVERIFY(!layer.passThrough);
    QCOMPARE(previous.undo(), Result::Success);
    QCOMPARE(previous.snapshot()->layers[0].name, QStringLiteral("Red"));
    QCOMPARE(previous.redo(), Result::Success);
    QCOMPARE(previous.snapshot()->layers[0].name, QStringLiteral("Renamed"));
    recovered.close();
    QCOMPARE(recovered.writeLayerPassThrough(2, false), Result::Closed);
}

void KisGpuOperationDocumentTest::layerBlendModesKeepPixelsHistoryPendingInputAndRecovery_data()
{
    QTest::addColumn<int>("blendMode");
    QTest::newRow("multiply") << int(LayerBlendMode::Multiply);
    QTest::newRow("screen") << int(LayerBlendMode::Screen);
    QTest::newRow("darken") << int(LayerBlendMode::Darken);
    QTest::newRow("lighten") << int(LayerBlendMode::Lighten);
    QTest::newRow("add") << int(LayerBlendMode::Addition);
    QTest::newRow("linear_dodge") << int(LayerBlendMode::LinearDodge);
}

void KisGpuOperationDocumentTest::layerBlendModesKeepPixelsHistoryPendingInputAndRecovery()
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
    const auto bytes = document.recoveryData();
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile artifact(QDir(results).filePath((mode == LayerBlendMode::Screen ? QStringLiteral("layer-screen-mode.recovery") :
            mode == LayerBlendMode::Darken ? QStringLiteral("layer-darken-mode.recovery") :
            mode == LayerBlendMode::Lighten ? QStringLiteral("layer-lighten-mode.recovery") :
            mode == LayerBlendMode::Addition ? QStringLiteral("layer-add-mode.recovery") :
            mode == LayerBlendMode::LinearDodge ? QStringLiteral("layer-linear-dodge-mode.recovery") : QStringLiteral("layer-blend-modes.recovery"))));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
    }
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(bytes), Result::Success);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.cancelStroke(recovered.activeOperation()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(recovered.snapshot()->layers[2].blendMode, mode);
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].blendMode, mode);
    const auto current = recovered.recoveryData();
    if (mode != LayerBlendMode::Multiply) {
        auto earlier = current.chopped(32);
        earlier[5] = mode == LayerBlendMode::Screen ? 34 : mode <= LayerBlendMode::Lighten ? 35 : 36;
        earlier.append(QCryptographicHash::hash(earlier, QCryptographicHash::Sha256));
        QCOMPARE(recovered.restoreRecovery(earlier), Result::InvalidRecovery);
        QCOMPARE(recovered.recoveryData(), current);
    }
    const auto &group = recovered.snapshot()->layers[1];
    QByteArray prefix;
    QDataStream stream(&prefix, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << group.uuid;
    for (auto value : group.defaultPixel) stream << value;
    stream << group.opacity << quint8(group.userLocked)
           << quint8(quint8(group.pinnedToTimeline) | ((15u ^ group.channelWriteMask) << 1))
           << qint32(group.position.x()) << qint32(group.position.y()) << quint8(group.kind)
           << group.parentUuid << qint32(group.colorLabelIndex) << group.projectionChannelMask << quint8(group.passThrough);
    auto invalid = current.chopped(32);
    const auto offset = invalid.indexOf(prefix);
    QVERIFY(offset >= 0);
    invalid[offset + prefix.size()] = 7;
    invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
    QCOMPARE(recovered.restoreRecovery(invalid), Result::InvalidRecovery);
    QCOMPARE(recovered.recoveryData(), current);
    QCOMPARE(recovered.writeLayerBlendMode(2, LayerBlendMode(7)), Result::InvalidInput);
    QCOMPARE(recovered.setLayerBlendMode(2, LayerBlendMode(7)), Result::InvalidInput);
    QCOMPARE(recovered.setLayerBlendMode(999, mode), Result::LayerUnavailable);
    QCOMPARE(recovered.recoveryData(), current);
    QCOMPARE(recovered.duplicateLayers({1}, {QStringLiteral("Copy")}, 3), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[3].blendMode, mode);
    OperationDocument target(QSize(320, 180));
    QCOMPARE(target.insertLayers(insertionForest(document.snapshot()), 1), Result::Success);
    for (const auto &layer : target.snapshot()->layers)
        if (layer.name == QLatin1String("Red") || layer.name == QLatin1String("Inner") || layer.name == QLatin1String("Outer"))
            QCOMPARE(layer.blendMode, mode);
    OperationDocument previous({1, 1});
    QCOMPARE(previous.restoreRecovery(LibrePaint::Image::Gpu::Test::passThroughRecoveryV33()), Result::Success);
    for (const auto &state : previous.historyVersions())
        for (const auto &layer : state->layers) QCOMPARE(layer.blendMode, LayerBlendMode::Normal);
    OperationDocument previous34({1, 1});
    QCOMPARE(previous34.restoreRecovery(LibrePaint::Image::Gpu::Test::blendModesRecoveryV34()), Result::Success);
    QCOMPARE(previous34.snapshot()->layers[1].blendMode, LayerBlendMode::Multiply);
    QVERIFY(!previous34.activeOperation().isNull());
    QCOMPARE(previous34.cancelStroke(previous34.activeOperation()), Result::Success);
    QCOMPARE(previous34.undo(), Result::Success);
    QCOMPARE(previous34.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    OperationDocument previous35({1, 1});
    QCOMPARE(previous35.restoreRecovery(LibrePaint::Image::Gpu::Test::screenRecoveryV35()), Result::Success);
    for (const auto &layer : previous35.snapshot()->layers) QCOMPARE(layer.blendMode, LayerBlendMode::Screen);
    QVERIFY(!previous35.activeOperation().isNull());
    QCOMPARE(previous35.cancelStroke(previous35.activeOperation()), Result::Success);
    QCOMPARE(previous35.undo(), Result::Success);
    QCOMPARE(previous35.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(previous35.snapshot()->layers[0].blendMode, LayerBlendMode::Screen);
    QCOMPARE(previous35.snapshot()->layers[2].blendMode, LayerBlendMode::Screen);
    QCOMPARE(previous35.redo(), Result::Success);
    QCOMPARE(previous35.snapshot()->layers[1].blendMode, LayerBlendMode::Screen);
    OperationDocument previous36({1, 1});
    QCOMPARE(previous36.restoreRecovery(LibrePaint::Image::Gpu::Test::lightenRecoveryV36()), Result::Success);
    for (const auto &layer : previous36.snapshot()->layers) QCOMPARE(layer.blendMode, LayerBlendMode::Lighten);
    QVERIFY(!previous36.activeOperation().isNull());
    QCOMPARE(previous36.cancelStroke(previous36.activeOperation()), Result::Success);
    QCOMPARE(previous36.undo(), Result::Success);
    QCOMPARE(previous36.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(previous36.redo(), Result::Success);
    QCOMPARE(previous36.snapshot()->layers[1].blendMode, LayerBlendMode::Lighten);
    recovered.close();
    QCOMPARE(recovered.writeLayerBlendMode(2, LayerBlendMode::Normal), Result::Closed);
    input.layers[0].blendMode = LayerBlendMode(7);
    QVERIFY_EXCEPTION_THROWN(OperationDocument invalid(input), std::invalid_argument);
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
            input.layers[1].defaultPixel = mode >= LayerBlendMode::Addition ? std::array<quint8, 4>{208, 224, 240, quint8(sourceAlpha)} :
                std::array<quint8, 4>{201, 41, 93, quint8(sourceAlpha)};
            input.layers[1].opacity = quint8(opacity);
            input.layers[1].blendMode = mode;
            input.layers[1].projectionChannelMask = quint8(mask);
            input.layers[0].parentUuid = input.layers[1].parentUuid = group;
            input.layers[2].kind = LayerKind::Group;
            OperationDocument document(input);
            const auto value = document.snapshot()->projectionDefaultPixels().value(3);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(value.data()), 4), references.mid(row++ * 4, 4));
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].blendMode, LayerBlendMode::Normal);
    QCOMPARE(recovered.snapshot()->layers[1].opacity, quint8(137));
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].blendMode, mode);
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(!recovered.snapshot()->layers[1].passThrough);
    QCOMPARE(recovered.snapshot()->layers[1].opacity, quint8(137));
    QCOMPARE(recovered.redo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers[1].passThrough);
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
    const auto bytes = document.recoveryData();
    fields.layer = 1;
    QCOMPARE(document.restoreLayerProperties(opening, {fields}), Result::InvalidInput);
    QCOMPARE(document.recoveryData(), bytes);
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
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.historyAction(1), HistoryAction::ChangeLayerProperties);
    const auto recovery = recovered.recoveryData();
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        QFile artifact(QDir(resultDirectory).filePath(QStringLiteral("layer-properties.recovery")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(recovery), recovery.size());
    }
    auto corrupt = recovery.first(recovery.size() - 32);
    QByteArray descriptor;
    QDataStream descriptorStream(&descriptor, QIODevice::WriteOnly);
    descriptorStream.setByteOrder(QDataStream::BigEndian);
    descriptorStream << quint8(254) << quint32(8) << quint8(1) << quint64(1);
    const auto offset = corrupt.indexOf(descriptor);
    QVERIFY(offset >= 0 && offset == corrupt.lastIndexOf(descriptor));
    corrupt[offset + 5] = 7; // Placement belongs to the separate movement operation.
    corrupt.append(QCryptographicHash::hash(corrupt, QCryptographicHash::Sha256));
    QCOMPARE(recovered.restoreRecovery(corrupt), Result::InvalidRecovery);
    QCOMPARE(recovered.recoveryData(), recovery);
    QByteArray componentDescriptor;
    QDataStream componentStream(&componentDescriptor, QIODevice::WriteOnly);
    componentStream.setByteOrder(QDataStream::BigEndian);
    componentStream << quint8(8) << quint8(13) << quint64(1) << quint8(15) << quint8(9);
    corrupt = recovery.first(recovery.size() - 32);
    const auto componentOffset = corrupt.indexOf(componentDescriptor);
    QVERIFY(componentOffset >= 0 && componentOffset == corrupt.lastIndexOf(componentDescriptor));
    corrupt[componentOffset + 1] = 0;
    corrupt.append(QCryptographicHash::hash(corrupt, QCryptographicHash::Sha256));
    QCOMPARE(recovered.restoreRecovery(corrupt), Result::InvalidRecovery);
    QCOMPARE(recovered.recoveryData(), recovery);
    for (auto *owner : {&document, &recovered}) {
        QCOMPARE(owner->undo(), Result::Success);
        const auto old = owner->snapshot();
        QCOMPARE(old->layers[0].name, QStringLiteral("Red"));
        QCOMPARE(old->layers[0].opacity, quint8(255));
        QVERIFY(old->layers[0].visible);
        QVERIFY(!old->layers[0].userLocked);
        QVERIFY(old->layers[0].pinnedToTimeline);
        QCOMPARE(old->layers[0].channelWriteMask, quint8(15));
        QCOMPARE(old->layers[0].projectionChannelMask, quint8(15));
        QCOMPARE(old->layers[0].colorLabelIndex, 0);
        QCOMPARE(old->layers[1].opacity, quint8(255));
        QCOMPARE(old->layers[1].colorLabelIndex, 7);
        QVERIFY(!owner->isModified());
        QCOMPARE(owner->redo(), Result::Success);
        const auto current = owner->snapshot();
        QCOMPARE(current->layers[0].name, QStringLiteral("Renamed"));
        QCOMPARE(current->layers[0].opacity, quint8(89));
        QVERIFY(!current->layers[0].visible);
        QVERIFY(current->layers[0].userLocked);
        QVERIFY(current->layers[0].pinnedToTimeline);
        QCOMPARE(current->layers[0].channelWriteMask, quint8(5));
        QCOMPARE(current->layers[0].projectionChannelMask, quint8(11));
        QCOMPARE(current->layers[0].colorLabelIndex, 5);
        QCOMPARE(current->layers[1].opacity, quint8(197));
        QCOMPARE(current->layers[1].colorLabelIndex, 7);
    }
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
    const auto recovery = document.recoveryData();
    OperationDocument foreign(input);
    QCOMPARE(document.restoreLayerProperties(foreign.snapshot(), selection), Result::StaleVersion);
    auto invalid = selection.first();
    invalid.projectionChannels = 16;
    QCOMPARE(document.restoreLayerProperties(before, {invalid}), Result::InvalidInput);
    QCOMPARE(document.recoveryData(), recovery);
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

void KisGpuOperationDocumentTest::colorLabelsPreserveFixedVersionsPendingInputAndRecovery()
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
    const auto bytes = document.recoveryData();
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        QFile artifact(QDir(results).filePath(QStringLiteral("color-labels.recovery")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), qint64(bytes.size()));
    }
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(bytes), Result::Success);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.snapshot()->layers[0].colorLabelIndex, 3);
    QCOMPARE(recovered.snapshot()->layers[1].colorLabelIndex, 7);
    QCOMPARE(recovered.preview()->layers[0].colorLabelIndex, 3);
    QCOMPARE(recovered.writeLayerColorLabel(999, 4), Result::LayerUnavailable);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.cancelStroke(recovered.activeOperation()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].colorLabelIndex, 3);
    QCOMPARE(recovered.snapshot()->layers[1].colorLabelIndex, 7);
    recovered.close();
    QCOMPARE(recovered.writeLayerColorLabel(1, 4), Result::Closed);
}

void KisGpuOperationDocumentTest::projectionChannelsPreservePixelsPendingInputHistoryAndRecovery()
{
    OperationDocument document(nestedImage());
    const auto fixed = document.snapshot();
    const auto oldRecovery = LibrePaint::Image::Gpu::Test::layerMetadataRecoveryV30();
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
    OperationDocument recovered({1, 1});
    const auto bytes = document.recoveryData();
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile artifact(QDir(directory).filePath(QStringLiteral("projection-channels.recovery")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(bytes), bytes.size());
    }
    QCOMPARE(recovered.restoreRecovery(bytes), Result::Success);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.preview()->layers[1].projectionChannelMask, quint8(5));
    QByteArray prefix;
    QDataStream stream(&prefix, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setByteOrder(QDataStream::BigEndian);
    const auto &group = document.snapshot()->layers[1];
    stream << group.uuid;
    for (const auto component : group.defaultPixel) stream << component;
    stream << group.opacity << quint8(group.userLocked)
           << quint8(quint8(group.pinnedToTimeline) | ((15u ^ group.channelWriteMask) << 1))
           << qint32(group.position.x()) << qint32(group.position.y()) << quint8(group.kind)
           << group.parentUuid << qint32(group.colorLabelIndex);
    auto invalidRecovery = bytes.first(bytes.size() - 32);
    const auto flagOffset = invalidRecovery.indexOf(prefix) + prefix.size();
    QVERIFY(flagOffset >= prefix.size() && flagOffset < invalidRecovery.size());
    invalidRecovery[flagOffset] = 16;
    invalidRecovery.append(QCryptographicHash::hash(invalidRecovery, QCryptographicHash::Sha256));
    QCOMPARE(recovered.restoreRecovery(invalidRecovery), Result::InvalidRecovery);
    QCOMPARE(recovered.recoveryData(), bytes);
    QCOMPARE(recovered.cancelStroke(recovered.activeOperation()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].projectionChannelMask, quint8(15));
    QCOMPARE(recovered.snapshot()->layers[0].projectionChannelMask, quint8(9));
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].projectionChannelMask, quint8(1));
    const auto beforeRejected = recovered.recoveryData();
    QCOMPARE(recovered.setLayerProjectionMask(2, 16), Result::InvalidInput);
    QCOMPARE(recovered.writeLayerProjectionMask(2, 16), Result::InvalidInput);
    QCOMPARE(recovered.writeLayerProjectionMask(999, 7), Result::LayerUnavailable);
    QCOMPARE(recovered.recoveryData(), beforeRejected);
    QCOMPARE(recovered.restoreRecovery(oldRecovery), Result::Success);
    for (const auto &layer : recovered.snapshot()->layers) QCOMPARE(layer.projectionChannelMask, quint8(15));
    QCOMPARE(recovered.snapshot()->layers[0].colorLabelIndex, 3);
    QCOMPARE(recovered.snapshot()->layers[1].colorLabelIndex, 7);
    QVERIFY(recovered.hasActiveStroke());
    QCOMPARE(recovered.restoreRecovery(LibrePaint::Image::Gpu::Test::projectionChannelsRecoveryV31()), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].projectionChannelMask, quint8(9));
    QCOMPARE(recovered.snapshot()->layers[1].projectionChannelMask, quint8(5));
    QCOMPARE(recovered.preview()->layers[1].projectionChannelMask, quint8(5));
    QCOMPARE(recovered.cancelStroke(recovered.activeOperation()), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].projectionChannelMask, quint8(15));
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].projectionChannelMask, quint8(1));
    auto invalid = nestedImage();
    invalid.layers[1].projectionChannelMask = 16;
    QVERIFY_EXCEPTION_THROWN(OperationDocument{invalid}, std::invalid_argument);
    recovered.close();
    QCOMPARE(recovered.writeLayerProjectionMask(1, 7), Result::Closed);
}

void KisGpuOperationDocumentTest::nestedLayerIdentitySurvivesPropertiesHistoryAndRecovery()
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
    const auto beforeRejected = document.recoveryData();
    QVERIFY(document.beginStroke(2, Brush{}, 41).isNull());
    const QList<RgbaTile> pixels{{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')}};
    QCOMPARE(document.replaceTiles(2, pixels), Result::LayerUnavailable);
    QCOMPARE(document.cropLayer(2, {1, 2, 16, 16}), Result::LayerUnavailable);
    QCOMPARE(document.clearLayers({2}), Result::LayerUnavailable);
    QVERIFY(document.beginLayerTranslation(2).isNull());
    QCOMPARE(document.writeLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.setLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.recoveryData(), beforeRejected);
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    for (int index = 0; index < source.layers.size(); ++index) {
        const auto &actual = recovered.snapshot()->layers[index];
        QCOMPARE(actual.uuid, source.layers[index].uuid);
        QCOMPARE(actual.parentUuid, source.layers[index].parentUuid);
        QCOMPARE(actual.kind, source.layers[index].kind);
    }
    QCOMPARE(recovered.snapshot()->layers[2].opacity, quint8(128));
    QCOMPARE(recovered.snapshot()->layers[1].position, QPoint(7, -9));
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers[0].operations.isEmpty());
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[2].opacity, quint8(255));
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].operations.size(), 1);
    for (int damage = 0; damage < 6; ++damage) {
        auto invalid = source;
        if (damage == 0) invalid.layers[0].parentUuid = QUuid(QStringLiteral("{31323334-3536-3738-393a-3b3c3d3e3f40}"));
        if (damage == 1) invalid.layers[1].parentUuid = invalid.layers[0].uuid;
        if (damage == 2) invalid.layers[2].parentUuid = invalid.layers[1].uuid;
        if (damage == 3) invalid.layers[1].parentUuid = invalid.layers[1].uuid;
        if (damage == 4) invalid.layers[1].kind = static_cast<LayerKind>(255);
        if (damage == 5) invalid.layers[1].tiles = {{{0, 0}, QByteArray(RgbaTile::ByteCount, '\0')}};
        bool rejected = false;
        try { OperationDocument candidate(invalid); } catch (const std::invalid_argument &) { rejected = true; }
        QVERIFY2(rejected, qPrintable(QStringLiteral("Invalid hierarchy case %1 was accepted").arg(damage)));
    }
}

void KisGpuOperationDocumentTest::previousCanvasRecoveryPreservesSizesAndInput()
{
    const auto record = LibrePaint::Image::Gpu::Test::resizedCanvasRecoveryV27();
    QCOMPARE(QCryptographicHash::hash(record, QCryptographicHash::Sha256).toHex(),
             QByteArrayLiteral("9652ea3870203cd85a85de7dddfc7645c0eef31390794dd5f922deff5a92120c"));
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(record), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(80, 64));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-76, 20));
    QCOMPARE(document.historyAction(3), HistoryAction::CropImage);
    QCOMPARE(document.historyAction(4), HistoryAction::ResizeCanvas);
    QVERIFY(document.hasActiveStroke());
    QCOMPARE(document.preview()->size, QSize(80, 64));
    QCOMPARE(document.cancelStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(32, 24));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-81, 13));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(64, 64));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-73, 19));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(80, 64));
}

void KisGpuOperationDocumentTest::previousCroppedRecoveryPreservesInputAndCanvasHistory()
{
    const auto bytes = LibrePaint::Image::Gpu::Test::croppedLayerRecoveryV26();
    QCOMPARE(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex(),
             QByteArrayLiteral("90aa7aeb29165403c9364ceff682f35b292daeeab1750cd72877c95f44b8bf94"));
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(bytes), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(64, 64));
    QCOMPARE(document.undoCount(), 3);
    QVERIFY(document.hasActiveStroke());
    QCOMPARE(document.preview()->sampleCount, 3);
    QCOMPARE(document.historyCrop(3)->rect, QRect(-80, 10, 40, 40));
    QCOMPARE(document.cancelStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.resizeCanvas({-8, -6, 32, 24}, false), Result::Success);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->size, QSize(32, 24));
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->size, QSize(64, 64));
    QCOMPARE(recovered.historyAction(3), HistoryAction::CropLayer);
}

void KisGpuOperationDocumentTest::layerCropPreservesValuesHistoryAndRecovery()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.historyAction(1), HistoryAction::CropLayer);
    QCOMPARE(recovered.historyCrop(1)->rect, requested);
    QCOMPARE(recovered.historyCrop(1)->layer, layer);
    QCOMPARE(recovered.savedHistoryIndex(), 1);
    QCOMPARE(recovered.snapshot()->layers.first().name, QStringLiteral("Renamed after crop"));
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(recovered.redo(), Result::Success);
    QCOMPARE(std::get<CropSnapshot>(recovered.snapshot()->layers.first().operations.last())->rect, requested);
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

void KisGpuOperationDocumentTest::invertColorsFreezesCoverageAndRecovers()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.historyAction(2), HistoryAction::InvertColors);
    const auto recoveredOperation = std::get<FilterSnapshot>(recovered.snapshot()->layers.first().operations.last());
    QCOMPARE(recoveredOperation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(recoveredOperation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(recoveredOperation->selectionInverted);
    QCOMPARE(recovered.savedHistoryIndex(), 2);

    // Alpha (feathered) freehand coverage is retained by the operation and its
    // recovery record, including the fractional bytes.
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
    OperationDocument maskRecovered({1, 1});
    QCOMPARE(maskRecovered.restoreRecovery(masked.recoveryData()), Result::Success);
    const auto maskOperation = std::get<FilterSnapshot>(maskRecovered.snapshot()->layers.first().operations.last());
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

void KisGpuOperationDocumentTest::grayscaleFilterFreezesCoverageAndRecovers()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.historyAction(2), HistoryAction::Grayscale);
    const auto recoveredOperation = std::get<FilterSnapshot>(recovered.snapshot()->layers.first().operations.last());
    QCOMPARE(recoveredOperation->kind, FilterKind::Grayscale);
    QCOMPARE(recoveredOperation->selectionRect, QRect(32, 32, 64, 48));
    QCOMPARE(recoveredOperation->selectionKind, SelectionKind::Ellipse);
    QVERIFY(recoveredOperation->selectionInverted);
    QCOMPARE(recovered.savedHistoryIndex(), 2);

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
    OperationDocument maskRecovered({1, 1});
    QCOMPARE(maskRecovered.restoreRecovery(masked.recoveryData()), Result::Success);
    const auto maskOperation = std::get<FilterSnapshot>(maskRecovered.snapshot()->layers.first().operations.last());
    QCOMPARE(maskOperation->kind, FilterKind::Grayscale);
    QVERIFY(maskOperation->selectionMask);
    QVERIFY(*maskOperation->selectionMask == *soft);
    QVERIFY(maskOperation->selectionMask->isAlpha());
    QCOMPARE(maskOperation->selectionMask->coverage(8, 8), quint8(96));
}

void KisGpuOperationDocumentTest::grayscaleRecoveryRequiresFormat51()
{
    RasterImage source;
    source.size = {64, 64};
    RasterLayer target;
    target.name = QStringLiteral("Target");
    target.uuid = QUuid::createUuid();
    source.layers.append(target);
    OperationDocument document(source);
    QCOMPARE(document.setSelectionRect(QRect(8, 8, 16, 16)), Result::Success);
    QCOMPARE(document.grayscale(document.activeLayer()), Result::Success);
    const auto bytes = document.recoveryData();
    QVERIFY(!bytes.isEmpty());

    // The record carries the bumped format number.
    QBuffer header;
    header.setData(bytes);
    QVERIFY(header.open(QIODevice::ReadOnly));
    QDataStream headerStream(&header);
    headerStream.setVersion(QDataStream::Qt_5_15);
    headerStream.setByteOrder(QDataStream::BigEndian);
    quint32 magic = 0;
    quint16 format = 0;
    headerStream >> magic >> format;
    QCOMPARE(magic, quint32(0x4c504f52));
    QCOMPARE(format, quint16(51));

    // The same record restores at format 51.
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.historyAction(restored.undoCount()), HistoryAction::Grayscale);
    QCOMPARE(std::get<FilterSnapshot>(restored.snapshot()->layers.first().operations.last())->kind,
             FilterKind::Grayscale);

    // Re-labelling the payload as format 50 must reject the Grayscale kind,
    // because format 50 knew only Invert Colors.
    auto payload = bytes.first(bytes.size() - 32);
    QBuffer patcher(&payload);
    QVERIFY(patcher.open(QIODevice::ReadWrite));
    QDataStream patchStream(&patcher);
    patchStream.setVersion(QDataStream::Qt_5_15);
    patchStream.setByteOrder(QDataStream::BigEndian);
    QVERIFY(patcher.seek(4));
    patchStream << quint16(50);
    patcher.close();
    payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
    OperationDocument live({64, 64});
    const auto liveBefore = live.snapshot();
    QCOMPARE(live.restoreRecovery(payload), Result::InvalidRecovery);
    QCOMPARE(live.snapshot(), liveBefore);
    QCOMPARE(live.undoCount(), 0);
}

void KisGpuOperationDocumentTest::previousTranslatedRecoveryPreservesPendingInputAndCropHistory()
{
    const auto bytes = LibrePaint::Image::Gpu::Test::translatedLayerRecoveryV25();
    QCOMPARE(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex(),
             QByteArrayLiteral("c556bf45c01accefd2338475dcecafc5c023a15683084bd962732b4626d08bc3"));
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(bytes), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(64, 64));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-73, 19));
    QCOMPARE(document.undoCount(), 2);
    QVERIFY(document.hasActiveStroke());
    QCOMPARE(document.preview()->sampleCount, 3);
    QCOMPARE(document.cancelStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.cropLayer(document.activeLayer(), QRect(-70, 20, 10, 12)), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.historyAction(3), HistoryAction::CropLayer);
    QCOMPARE(recovered.historyCrop(3)->rect, QRect(-70, 20, 10, 12));
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers.first().position, QPoint());
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
    QByteArray recovery;
    try {
        recovery = document.recoveryData();
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Recovery of a moved layer failed: ") + QString::fromUtf8(error.what())));
    }
    QVERIFY(!recovery.isEmpty());
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().position, initial + offset);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().position, initial);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().position, initial + offset);
    QCOMPARE(restored.undo(), Result::Success);
    const auto pending = restored.beginLayerTranslation(layer);
    QVERIFY(!pending.isNull());
    QCOMPARE(restored.updateLayerTranslation(pending, {-5, 3}), Result::Success);
    OperationDocument pendingCopy({1, 1});
    QCOMPARE(pendingCopy.restoreRecovery(restored.recoveryData()), Result::Success);
    QVERIFY(pendingCopy.hasActiveStroke());
    QCOMPARE(pendingCopy.snapshot()->layers.first().position, initial);
    QCOMPARE(pendingCopy.preview()->layers.first().position, initial + QPoint(-5, 3));
    QCOMPARE(pendingCopy.redoCount(), 1);
    QCOMPARE(pendingCopy.cancelLayerTranslation(pendingCopy.activeOperation()), Result::Success);
    QCOMPARE(pendingCopy.redo(), Result::Success);
    QCOMPARE(pendingCopy.snapshot()->layers.first().position, initial + offset);
    const auto source = document.snapshot();
    try {
        QCOMPARE(document.duplicateLayers({layer}, {QStringLiteral("Moved copy")}, layer), Result::Success);
    } catch (const std::exception &error) {
        QFAIL(qPrintable(QStringLiteral("Copying a moved layer failed: ") + QString::fromUtf8(error.what())));
    }
    const auto &copy = document.snapshot()->layers.last();
    QCOMPARE(copy.position, initial + offset);
    QCOMPARE(std::get<PaintSnapshot>(copy.operations.first())->inputSamples(), samples);
    const auto translated = std::get_if<TranslationSnapshot>(&copy.operations.last());
    QVERIFY(translated);
    QCOMPARE((*translated)->offset, offset);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), source);
}

void KisGpuOperationDocumentTest::previousPlacedRecoveryPreservesPendingInputAndPosition()
{
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(Test::placedLayerRecoveryV24()), Result::Success);
    QCOMPARE(document.snapshot()->size, QSize(64, 64));
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-73, 19));
    QCOMPARE(document.preview()->layers.first().position, QPoint(-73, 19));
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.redoCount(), 1);
    QVERIFY(document.hasActiveStroke());
    QVERIFY(!document.isModified());
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.first())->inputSamples(),
             QList<InputSample>({sample(12, 9, 0.5, 1), sample(14, 10, 1, 2)}));
    QCOMPARE(document.cancelStroke(document.activeOperation()), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.first().position, QPoint(-73, 19));
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(129));
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->layers[0].id, top);
    QCOMPARE(restored.historyAction(restored.undoCount()), HistoryAction::MoveLayer);
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

void KisGpuOperationDocumentTest::mergedPropertiesPreservePinsSaveBoundariesAndRecovery()
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(document.markSaved(document.snapshot()), Result::Success);
    QCOMPARE(document.setLayerOpacity(layer, 32, PropertyHistory::MergeConsecutive), Result::Success);
    QCOMPARE(document.undoCount(), 2);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.snapshot()->layers.first().opacity, quint8(64));
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
    OperationDocument restored({1, 1});
    const auto recovery = document.recoveryData();
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    QCOMPARE(restored.snapshot()->layers.first().name, QStringLiteral("Direct"));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().name, initial);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().name, initial);
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.historyAction(restored.undoCount()), HistoryAction::RemoveLayer);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().uuid, removed->layers.first().uuid);
    QCOMPARE(restored.snapshot()->rasterBytes, removed->rasterBytes);
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
    const auto beforeInvalid = document.recoveryData();
    QCOMPARE(document.addLayer(QStringLiteral("Invalid"), -1, false), LayerId(0));
    QCOMPARE(document.addLayer(QStringLiteral("Invalid"), 3, false), LayerId(0));
    QCOMPARE(document.recoveryData(), beforeInvalid);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), inserted);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].uuid, inserted->layers[1].uuid);
    QVERIFY(recovered.snapshot()->layers[1].pinnedToTimeline);
    const auto pending = document.beginStroke(id, {}, 17);
    QVERIFY(!pending.isNull());
    QCOMPARE(document.addLayer(QStringLiteral("Busy"), 0, false), LayerId(0));
    QCOMPARE(document.cancelStroke(pending), Result::Success);
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
    const auto beforeInvalid = document.recoveryData();
    QCOMPARE(document.addLayer(QStringLiteral("Missing parent"), 1, false,
                              QUuid(QStringLiteral("{deadbeef-735b-47cf-93c5-7893d04916f0}"))), LayerId(0));
    QCOMPARE(document.addLayer(QStringLiteral("Paint parent"), 1, false, original->layers[0].uuid), LayerId(0));
    QCOMPARE(document.recoveryData(), beforeInvalid);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), id);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].parentUuid, original->layers[1].uuid);
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
    const auto beforeRejected = destination.recoveryData();
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
    QCOMPARE(destination.recoveryData(), beforeRejected);
    QCOMPARE(destination.redo(), Result::Success);
    QCOMPARE(destination.snapshot(), inserted);
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(destination.recoveryData()), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[1].kind, LayerKind::Group);
    QCOMPARE(recovered.snapshot()->layers[1].uuid, properties.uuid);
    QCOMPARE(recovered.snapshot()->layers[1].parentUuid, properties.parentUuid);
    QCOMPARE(source.snapshot(), sourceVersion);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile output(QDir(directory).filePath(QStringLiteral("empty-group-insertion.recovery")));
        QVERIFY(output.open(QIODevice::WriteOnly));
        const auto fixed = destination.recoveryData();
        QCOMPARE(output.write(fixed), qint64(fixed.size()));
    }
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(destination.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->layers[0].uuid, properties.uuid);
    QCOMPARE(source.snapshot(), sourceVersion);
    const auto fixed = destination.recoveryData();
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile output(QDir(directory).filePath(QStringLiteral("layer-insertion.recovery")));
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(output.write(fixed), qint64(fixed.size()));
    }
    QCOMPARE(destination.insertLayer(source, 999, properties, 0), LayerId(0));
    QCOMPARE(destination.insertLayer(source, 1, properties, 0), LayerId(0));
    QCOMPARE(destination.recoveryData(), fixed);
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
        const auto retained = limited.recoveryData();
        QCOMPARE(limited.insertLayer(source, 1, properties, 0), LayerId(0));
        QCOMPARE(limited.recoveryData(), retained);
    }
}

void KisGpuOperationDocumentTest::activeLayerAndSelectionRemainIndependentThroughRecoveryAndInput()
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
    const auto bytes = document.recoveryData();
    const auto output = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!output.isEmpty()) {
        QVERIFY(QDir().mkpath(output));
        QFile file(QDir(output).filePath(QStringLiteral("layer-selection.recovery")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), bytes.size());
    }
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.activeLayer(), first);
    QCOMPARE(restored.snapshot()->selectedLayers, QList<LayerId>{second});
    const auto oldFormat = legacyRecovery(document, 15);
    const auto before = restored.snapshot();
    QCOMPARE(restored.restoreRecovery(oldFormat), Result::InvalidRecovery);
    QCOMPARE(restored.snapshot(), before);
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
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->selectedLayers, QList<LayerId>{second});
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
    const auto beforeInvalid = document.recoveryData();
    QCOMPARE(document.selectLayer(0), Result::LayerUnavailable);
    QCOMPARE(document.recoveryData(), beforeInvalid);
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
    const auto beforeInvalid = document.recoveryData();
    QCOMPARE(set(document, true, PropertyHistory(-1)), Result::InvalidInput);
    QCOMPARE(document.recoveryData(), beforeInvalid);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(state(restored.snapshot()));
    QVERIFY(!restored.isModified());
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(!state(restored.snapshot()));
    const auto action = locked ? HistoryAction::ChangeLock : HistoryAction::ChangeVisibility;
    QCOMPARE(restored.historyAction(2), action);
    QCOMPARE(restored.historyAction(3), action);
}

void KisGpuOperationDocumentTest::historyActionsSurviveDirectWritesMergingAndRecovery()
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    for (int index = 0; index < expected.size(); ++index) {
        QCOMPARE(document.historyAction(index), expected[index]);
        QCOMPARE(restored.historyAction(index), expected[index]);
    }
    QCOMPARE(document.historyAction(-1), HistoryAction::None);
    QCOMPARE(document.historyAction(expected.size()), HistoryAction::None);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.historyAction(4), HistoryAction::ChangeLock);
    QCOMPARE(restored.renameLayer(layer, QStringLiteral("Branched")), Result::Success);
    QCOMPARE(restored.historyAction(4), HistoryAction::RenameLayer);
    QCOMPARE(restored.redoCount(), 0);
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.activeLayer(), second);
    QVERIFY(!restored.isModified());
}

void KisGpuOperationDocumentTest::channelProtectionPreservesFrozenSettingsHistoryAndRecovery()
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.preview()->layers.first().channelWriteMask, quint8(7));
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last())->brush, paint->brush);
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
    const auto bytes = first.recoveryData();
    QCOMPARE(bytes, second.recoveryData());
    const auto firstCopy = first.snapshot()->layers[2].uuid;
    auto collisionImage = rasterImage();
    collisionImage.layers[1].uuid = firstCopy;
    OperationDocument collision(collisionImage);
    QCOMPARE(collision.duplicateLayers({1}, {QStringLiteral("Another copy")}, 1), Result::Success);
    QVERIFY(collision.snapshot()->layers[1].uuid != firstCopy);
    QVERIFY(collision.snapshot()->layers[1].uuid != collisionImage.layers.first().uuid);
    QCOMPARE(first.duplicateLayers({1}, {QStringLiteral("Another copy")}, 1), Result::Success);
    QVERIFY(first.snapshot()->layers[1].uuid != firstCopy);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile output(QDir(directory).filePath(QStringLiteral("layer-duplication.recovery")));
        QVERIFY(output.open(QIODevice::WriteOnly));
        QCOMPARE(output.write(bytes), qint64(bytes.size()));
    }
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->layers[3].id, copy.id);
    QCOMPARE(restored.historyAction(count + 1), HistoryAction::DuplicateLayer);
    QCOMPARE(restored.snapshot()->selectedLayers, copied->selectedLayers);
    QCOMPARE(restored.snapshot()->layers[3].channelWriteMask, quint8(7));
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers[3].operations.last())->brush, brush);
    const auto before = document.recoveryData();
    const auto previousFormat = legacyRecovery(document, 15);
    QCOMPARE(restored.restoreRecovery(previousFormat), Result::Success);
    QCOMPARE(restored.snapshot()->selectedLayers, copied->selectedLayers);
    QCOMPARE(restored.historyAction(count + 1), HistoryAction::DuplicateLayer);
    const auto oldFormat = legacyRecovery(document, 14);
    const auto restoredVersion = restored.snapshot();
    QCOMPARE(restored.restoreRecovery(oldFormat), Result::InvalidRecovery);
    QCOMPARE(restored.snapshot(), restoredVersion);
    QCOMPARE(document.selectLayers({first, first}, first), Result::InvalidInput);
    QCOMPARE(document.selectLayers({}, first), Result::InvalidInput);

    QCOMPARE(document.duplicateLayers({first, first}, {QStringLiteral("A"), QStringLiteral("B")}, first), Result::InvalidInput);
    QCOMPARE(document.duplicateLayers({first}, {}, first), Result::InvalidInput);
    QCOMPARE(document.recoveryData(), before);
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
    const auto pixelLimit = pixelBounded.recoveryData();
    QCOMPARE(pixelBounded.duplicateLayers({1}, {QStringLiteral("A")}, 1), Result::CapacityExceeded);
    QCOMPARE(pixelBounded.recoveryData(), pixelLimit);
    limits.samplesPerStroke = limits.samplesPerDocument = 1;
    OperationDocument inputBounded({64, 64}, limits);
    QVERIFY(!paint(inputBounded, {sample(16, 16)}).isNull());
    const auto inputLimit = inputBounded.recoveryData();
    QCOMPARE(inputBounded.duplicateLayers({inputBounded.activeLayer()}, {QStringLiteral("A")}, inputBounded.activeLayer()), Result::CapacityExceeded);
    QCOMPARE(inputBounded.recoveryData(), inputLimit);
}

void KisGpuOperationDocumentTest::symmetryFreezesAxesAndRejectsInvalidRecoveryAtomically()
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
    const auto recovery = document.recoveryData();
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last())->brush, settings);
    const auto retained = restored.preview();
    const auto invalid = legacyRecovery(document, 13);
    QCOMPARE(restored.restoreRecovery(invalid), Result::InvalidRecovery);
    QCOMPARE(restored.preview(), retained);
    QCOMPARE(restored.recoveryData(), recovery);
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

void KisGpuOperationDocumentTest::previousPropertyRecoveryRemainsReadable_data()
{
    QTest::addColumn<quint8>("format");
    QTest::addColumn<bool>("resetName");
    QTest::newRow("version-nine") << quint8(9) << false;
    QTest::newRow("version-ten") << quint8(10) << false;
    QTest::newRow("version-ten-reset-name") << quint8(10) << true;
    QTest::newRow("version-eleven") << quint8(11) << false;
    QTest::newRow("version-twelve") << quint8(12) << false;
    QTest::newRow("version-thirteen") << quint8(13) << false;
    QTest::newRow("version-fourteen") << quint8(14) << false;
    QTest::newRow("version-fifteen") << quint8(15) << false;
    QTest::newRow("version-sixteen") << quint8(16) << false;
    QTest::newRow("version-seventeen") << quint8(17) << false;
    QTest::newRow("version-eighteen") << quint8(18) << false;
    QTest::newRow("version-nineteen") << quint8(19) << false;
    QTest::newRow("version-twenty") << quint8(20) << false;
    QTest::newRow("version-twenty-one") << quint8(21) << false;
    QTest::newRow("version-twenty-two") << quint8(22) << false;
    QTest::newRow("version-twenty-three") << quint8(23) << false;
}

void KisGpuOperationDocumentTest::previousPositionlessRecoveryPreservesLayerEdits()
{
    OperationDocument document({1, 1});
    QCOMPARE(document.restoreRecovery(LibrePaint::Image::Gpu::Test::positionlessRecoveryV23()), Result::Success);
    const auto saved = document.snapshot();
    QCOMPARE(saved->size, QSize(64, 32));
    QCOMPARE(saved->layers.size(), 2);
    QCOMPARE(saved->layers.first().name, QStringLiteral("Inserted"));
    QCOMPARE(saved->layers.first().opacity, quint8(173));
    QCOMPARE(saved->layers.first().channelWriteMask, quint8(7));
    QVERIFY(saved->layers.first().userLocked);
    QVERIFY(!saved->layers.first().visible);
    QCOMPARE(saved->layers.first().operations.size(), 2);
    const auto pixels = std::get<TileWriteSnapshot>(saved->layers.first().operations.first());
    QCOMPARE(pixels->tiles.first()->rgba(), QByteArray::fromHex("ff0000ff").repeated(RgbaTile::ByteCount / 4));
    for (const auto &version : document.historyVersions())
        for (const auto &layer : version->layers) QCOMPARE(layer.position, QPoint());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.snapshot()->layers.first().name, QStringLiteral("Destination"));
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), saved);
}

void KisGpuOperationDocumentTest::previousPropertyRecoveryRemainsReadable()
{
    QFETCH(quint8, format);
    QFETCH(bool, resetName);
    OperationDocument document({64, 64});
    const auto stroke = document.beginStroke(document.activeLayer(), Brush{}, 7);
    QCOMPARE(document.appendSamples(stroke, {{{32, 32}, 1}}), Result::Success);
    QCOMPARE(document.updateShape(stroke, {ShapeKind::Ellipse, {16, 16}, {48, 48}, 0}), Result::Success);
    QCOMPARE(document.commitStroke(stroke), Result::Success);
    QCOMPARE(document.renameLayer(document.activeLayer(), QStringLiteral("Retained"), PropertyHistory::MergeConsecutive), Result::Success);
    if (resetName) QCOMPARE(document.renameLayer(document.activeLayer(), QStringLiteral("Paint Layer 1"), PropertyHistory::MergeConsecutive), Result::Success);
    const auto payload = legacyRecovery(document, format);
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(payload), Result::Success);
    QCOMPARE(restored.undoCount(), 2);
    QCOMPARE(restored.snapshot()->layers.first().name, resetName ? QStringLiteral("Paint Layer 1") : QStringLiteral("Retained"));
    const auto paint = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.first());
    QVERIFY(paint->figureStyle.outline);
    QVERIFY(!paint->figureStyle.fill);
    QCOMPARE(paint->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(paint->shape.cornerRadii, QSizeF(0, 0));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().name, QStringLiteral("Paint Layer 1"));
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
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().name, QString());
    QVERIFY(!restored.snapshot()->layers.first().visible);
    QVERIFY(restored.snapshot()->layers.first().userLocked);
    QVERIFY(restored.snapshot()->layers.first().pinnedToTimeline);
    QCOMPARE(restored.snapshot()->layers.last().opacity, quint8(64));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(255));
}

void KisGpuOperationDocumentTest::propertyRecoveryPreservesRecordedValuesAndRejectsDamagedChanges()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    const auto bytes = document.recoveryData();
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(64));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(255));
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.first().opacity, quint8(128));
    QByteArray record;
    QDataStream encoded(&record, QIODevice::WriteOnly);
    encoded.setByteOrder(QDataStream::BigEndian);
    encoded << quint8(2) << layer << quint8(255) << quint8(128);
    const auto payload = bytes.first(bytes.size() - 32);
    const auto offset = payload.lastIndexOf(record);
    QVERIFY(offset >= 0);
    const auto retained = restored.snapshot();
    const auto retainedHistory = restored.recoveryData();
    for (int damage = 0; damage < 3; ++damage) {
        auto invalid = payload;
        if (damage == 0) invalid[offset] = char(255);
        if (damage == 1) invalid[offset + 8] = char(2);
        if (damage == 2) invalid[offset + 10] = char(255);
        invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
        QCOMPARE(restored.restoreRecovery(invalid), Result::InvalidRecovery);
        QCOMPARE(restored.snapshot(), retained);
        QCOMPARE(restored.recoveryData(), retainedHistory);
    }
}

void KisGpuOperationDocumentTest::failedHistoryMovesPreserveEveryVersionAndCounter()
{
    OperationDocument document({64, 64});
    const auto layer = document.activeLayer();
    QCOMPARE(document.setLayerOpacity(layer, 128), Result::Success);
    QCOMPARE(document.setLayerOpacity(layer, 32), Result::Success);
    QCOMPARE(document.setHistoryIndex(0), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 16), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.writeLayerOpacity(layer, 64), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    auto bytes = document.recoveryData();
    bytes.chop(32);
    QBuffer buffer(&bytes);
    QVERIFY(buffer.open(QIODevice::ReadWrite));
    QVERIFY(buffer.seek(14)); // LPOR dimensions precede the next content revision.
    QDataStream encoded(&buffer);
    encoded.setByteOrder(QDataStream::BigEndian);
    encoded << quint64(std::numeric_limits<quint64>::max() - 1);
    buffer.close();
    bytes.append(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    const auto retained = restored.snapshot();
    const auto history = restored.recoveryData();
    QCOMPARE(restored.setHistoryIndex(0), Result::CapacityExceeded);
    QCOMPARE(restored.undoCount(), 2);
    QCOMPARE(restored.snapshot(), retained);
    QCOMPARE(restored.recoveryData(), history);
}

void KisGpuOperationDocumentTest::layerOpacityAndLockShareHistoryAndRecovery()
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
    OperationDocument recovered({1, 1});
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.snapshot()->layers.first().opacity, quint8(64));
    QVERIFY(!recovered.snapshot()->layers.first().userLocked);
    QCOMPARE(recovered.undo(), Result::Success);
    QVERIFY(recovered.snapshot()->layers.first().userLocked);
    auto invalid = document.recoveryData();
    invalid.chop(32);
    const auto identity = source.layers[0].uuid.toRfc4122();
    const auto offset = invalid.indexOf(identity);
    QVERIFY(offset >= 0);
    const auto lockOffset = offset + identity.size() + 4 + 1;
    QCOMPARE(invalid[lockOffset], char(1));
    invalid[lockOffset] = char(2);
    invalid.append(QCryptographicHash::hash(invalid, QCryptographicHash::Sha256));
    const auto retained = recovered.snapshot();
    QCOMPARE(recovered.restoreRecovery(invalid), Result::InvalidRecovery);
    QCOMPARE(recovered.snapshot(), retained);
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityWithoutLosingRecoveryHistory_data()
{
    QTest::addColumn<bool>("raster");
    QTest::newRow("compressed pixels") << true;
    QTest::newRow("stroke input") << false;
}

void KisGpuOperationDocumentTest::layerRemovalReusesCapacityWithoutLosingRecoveryHistory()
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
    const auto bytes = document.recoveryData();
    QVERIFY(!bytes.isEmpty());
    OperationDocument restored(QSize(1, 1), limits);
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.undoCount(), document.undoCount());
    if (raster) {
        QCOMPARE(restored.replaceTiles(restored.activeLayer(), {pixels}), Result::CapacityExceeded);
    } else {
        const auto stroke = restored.beginStroke(restored.activeLayer(), Brush{}, 0);
        QCOMPARE(restored.appendSamples(stroke, {sample(20, 20)}), Result::CapacityExceeded);
        QCOMPARE(restored.cancelStroke(stroke), Result::Success);
    }
    for (;;) {
        const auto actual = restored.snapshot();
        const auto expected = document.snapshot();
        QCOMPARE(actual->revision(), expected->revision());
        QCOMPARE(actual->activeLayer, expected->activeLayer);
        QCOMPARE(actual->sampleCount, expected->sampleCount);
        QCOMPARE(actual->rasterBytes, expected->rasterBytes);
        QCOMPARE(actual->layers.size(), expected->layers.size());
        if (!actual->layers.isEmpty()) {
            QCOMPARE(actual->layers[0].uuid, expected->layers[0].uuid);
            QCOMPARE(actual->layers[0].name, expected->layers[0].name);
            if (!actual->layers[0].operations.isEmpty()) {
                if (raster) {
                    QCOMPARE(std::get<TileWriteSnapshot>(actual->layers[0].operations[0])->tiles[0]->rgba(),
                             std::get<TileWriteSnapshot>(expected->layers[0].operations[0])->tiles[0]->rgba());
                } else {
                    QCOMPARE(std::get<PaintSnapshot>(actual->layers[0].operations[0])->inputSamples(),
                             std::get<PaintSnapshot>(expected->layers[0].operations[0])->inputSamples());
                }
            }
        }
        if (!document.undoCount()) break;
        QCOMPARE(restored.undo(), document.undo());
    }
    QCOMPARE(restored.undoCount(), 0);
    QCOMPARE(restored.redoCount(), document.redoCount());
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

void KisGpuOperationDocumentTest::removingTheLastLayerPreservesAnEmptyRecoverableDocument()
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
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(recovered.snapshot()->layers.isEmpty());
    QCOMPARE(recovered.activeLayer(), LayerId(0));
    QCOMPARE(recovered.undoCount(), 2);
    QCOMPARE(recovered.undo(), Result::Success);
    QCOMPARE(recovered.snapshot()->layers[0].uuid, painted->layers[0].uuid);
    QCOMPARE(recovered.snapshot()->sampleCount, painted->sampleCount);
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
    QCOMPARE(recovered.restoreRecovery(imported.recoveryData()), Result::Success);
    QVERIFY(recovered.snapshot()->layers.isEmpty());
    QVERIFY(recovered.addLayer(QStringLiteral("First")));
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
    const auto recovered = document.recoveryData();
    QVERIFY(!recovered.isEmpty());
    QCOMPARE(document.restoreRecovery(recovered), Result::Success);
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
    QCOMPARE(document.snapshot(), preview);
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

void KisGpuOperationDocumentTest::recoveryPreservesHistoryAndUncommittedIntent()
{
    OperationDocument source({128, 256});
    QVERIFY(!paint(source, {sample(3, 4, 0.5, 1), sample(5, 6, 0.8, 2)}).isNull());
    QCOMPARE(source.markSaved(source.snapshot()), Result::Success);
    const auto layer = source.addLayer(QStringLiteral("線画 🖌"));
    QVERIFY(!paint(source, {sample(7, 8)}).isNull());
    QCOMPARE(source.undo(), Result::Success);
    const auto pending = source.beginStroke(layer, Brush{}, 903);
    QCOMPARE(source.appendSamples(pending, {sample(9, 10), sample(15, 20)}), Result::Success);
    QCOMPARE(source.updateShape(pending, {ShapeKind::Ellipse, {9, 10}, {15, 20}, 0.3}), Result::Success);
    const QByteArray recovery = source.recoveryData();
    QVERIFY(!recovery.isEmpty());

    OperationDocument restored({1, 1});
    const auto previous = restored.beginStroke(restored.activeLayer(), Brush{}, 0);
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    qInfo() << "recovery SHA-256:" << QCryptographicHash::hash(recovery, QCryptographicHash::Sha256).toHex();
    QCOMPARE(restored.snapshot()->size, QSize(128, 256));
    QCOMPARE(restored.undoCount(), source.undoCount());
    QCOMPARE(restored.redoCount(), source.redoCount());
    QCOMPARE(restored.snapshot()->layers.back().name, QStringLiteral("線画 🖌"));
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->randomSeed, quint64(903));
    QCOMPARE(std::get<PaintSnapshot>(restored.preview()->layers.back().operations.back())->inputSamples(),
             std::get<PaintSnapshot>(source.preview()->layers.back().operations.back())->inputSamples());
    QCOMPARE(restored.appendSamples(previous, {sample(0, 0)}), Result::StaleOperation);
    QCOMPARE(restored.appendSamples(pending, {sample(0, 0)}), Result::StaleOperation);
    const auto recoveredEdit = restored.activeOperation();
    QVERIFY(!recoveredEdit.isNull());
    QCOMPARE(restored.cancelStroke(recoveredEdit), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers.back().operations.back())->inputSamples(), QList<InputSample>{sample(7, 8)});
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.isModified());
}

void KisGpuOperationDocumentTest::invalidRecoveryLeavesTheLiveDocumentUntouched()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(3, 4)}), Result::Success);
    const auto committed = document.snapshot();
    const auto preview = document.preview();
    const auto good = document.recoveryData();
    QVERIFY(!good.isEmpty());
    QList<QByteArray> damaged{{}, good.first(good.size() - 1), QByteArray(256, char(0xff))};
    auto flipped = good;
    flipped[flipped.size() / 2] ^= 1;
    damaged.append(flipped);
    for (const auto &bytes : damaged) {
        QCOMPARE(document.restoreRecovery(bytes), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), committed);
        QCOMPARE(document.preview(), preview);
        QCOMPARE(document.activeOperation(), id);
    }
    Limits tiny;
    tiny.recoveryBytes = 64;
    OperationDocument constrained({1, 1}, tiny);
    QCOMPARE(constrained.restoreRecovery(good), Result::CapacityExceeded);
    QVERIFY(constrained.recoveryData().isEmpty());
}

void KisGpuOperationDocumentTest::checkedRecoveryRejectsInvalidCountsAndValues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto original = document.snapshot();
    const auto good = document.recoveryData();
    // The header operation count precedes the first paint record. Locate its
    // sensor payload so optional painting settings do not shift the mutation.
    QByteArray sensor;
    QDataStream sensorStream(&sensor, QIODevice::WriteOnly);
    sensorStream.setVersion(QDataStream::Qt_5_15);
    sensorStream.setByteOrder(QDataStream::BigEndian);
    sensorStream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    sensorStream << qreal(1) << qreal(2) << qreal(1) << qreal(0) << qreal(0) << qreal(0) << quint64(0);
    const auto sensorPosition = good.indexOf(sensor);
    QVERIFY(sensorPosition >= 0);
    QCOMPARE(sensorPosition, good.lastIndexOf(sensor));
    for (const int corruption : {0, 1, 2}) {
        auto payload = good.first(good.size() - 32);
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QVERIFY(buffer.seek(corruption == 0 ? 50 : corruption == 1 ? sensorPosition + 16 : sensorPosition - 5));
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_15);
        stream.setByteOrder(QDataStream::BigEndian);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        if (corruption == 0) stream << std::numeric_limits<quint32>::max();
        else if (corruption == 1) stream << std::numeric_limits<qreal>::quiet_NaN();
        else stream << quint8(8);
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(document.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), original);
    }
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

void KisGpuOperationDocumentTest::rasterRecoveryPreservesPixelsIdentityAndPendingInput()
{
    auto image = rasterImage();
    image.projectionBackground = {17, 34, 51, 128};
    image.layers[0].pinnedToTimeline = true;
    OperationDocument document(image);
    QCOMPARE(document.selectLayer(1), Result::Success);
    const RgbaTile green{{0, 0}, QByteArray::fromHex("00ff00ff").repeated(RgbaTile::ByteCount / 4)};
    QCOMPARE(document.replaceTiles(1, {green}), Result::Success);
    const auto replaced = document.snapshot();
    QCOMPARE(document.markSaved(replaced), Result::Success);
    QVERIFY(!paint(document, {sample(8, 12)}).isNull());
    QCOMPARE(document.undo(), Result::Success);
    const auto pending = document.beginStroke(1, Brush{}, 17);
    QCOMPARE(document.appendSamples(pending, {sample(10, 20)}), Result::Success);
    const auto bytes = document.recoveryData();
    QVERIFY(!bytes.isEmpty());
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.recoveryData(), bytes);
    QCOMPARE(restored.snapshot()->projectionBackground, image.projectionBackground);
    QVERIFY(restored.snapshot()->layers[0].pinnedToTimeline);
    QCOMPARE(restored.snapshot()->rasterBytes, replaced->rasterBytes);
    QCOMPARE(restored.snapshot()->layers[0].uuid, image.layers[0].uuid);
    QCOMPARE(restored.snapshot()->layers[0].defaultPixel, image.layers[0].defaultPixel);
    QCOMPARE(restored.snapshot()->layers[1].uuid, image.layers[1].uuid);
    const auto initial = std::get<TileWriteSnapshot>(restored.snapshot()->layers[1].operations.first());
    QCOMPARE(initial->tiles.first()->rgba(), image.layers[1].tiles.first().rgba);
    const auto current = std::get<TileWriteSnapshot>(restored.snapshot()->layers[0].operations.last());
    QCOMPARE(current->tiles.first()->rgba(), green.rgba);
    QVERIFY(!restored.isModified());
    QCOMPARE(restored.undoCount(), 1);
    QCOMPARE(restored.redoCount(), 1);
    QCOMPARE(restored.cancelStroke(restored.activeOperation()), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    const auto old = std::get<TileWriteSnapshot>(restored.snapshot()->layers[0].operations.first());
    QCOMPARE(old->tiles.first()->rgba(), image.layers[0].tiles.first().rgba);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->sampleCount, qsizetype(1));
    qInfo() << "raster recovery SHA-256:" << QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

void KisGpuOperationDocumentTest::damagedRasterRecoveryPreservesTheLiveDocument()
{
    OperationDocument source(rasterImage());
    const auto good = source.recoveryData();
    const auto originalPayload = good.first(good.size() - 32);
    QBuffer reader;
    reader.setData(originalPayload);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QVERIFY(reader.seek(46)); // magic, format, size, revisions, saved revision.
    QDataStream input(&reader);
    input.setVersion(QDataStream::Qt_5_15);
    input.setByteOrder(QDataStream::BigEndian);
    // Format 40 stores the current selection rectangle before the tile count.
    qint32 selectionX = 0, selectionY = 0, selectionWidth = 0, selectionHeight = 0;
    quint8 selectionInverted = 0, selectionKind = 0;
    input >> selectionX >> selectionY >> selectionWidth >> selectionHeight >> selectionInverted >> selectionKind;
    quint32 tileCount = 0;
    input >> tileCount;
    QCOMPARE(tileCount, quint32(2));
    const auto firstTile = reader.pos();
    for (quint32 i = 0; i < tileCount; ++i) {
        qint32 x = 0, y = 0;
        quint32 size = 0;
        input >> x >> y >> size;
        QVERIFY(reader.seek(reader.pos() + size));
    }
    const auto firstTileReference = reader.pos() + 4 + 25 + 4;
    OperationDocument live({64, 64});
    QVERIFY(!paint(live, {sample(3, 4)}).isNull());
    const auto pending = live.beginStroke(live.activeLayer(), Brush{}, 23);
    QCOMPARE(live.appendSamples(pending, {sample(10, 12)}), Result::Success);
    const auto committed = live.snapshot();
    const auto preview = live.preview();
    const auto recovery = live.recoveryData();
    for (int damage = 0; damage < 4; ++damage) {
        auto payload = originalPayload;
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QDataStream output(&buffer);
        output.setVersion(QDataStream::Qt_5_15);
        output.setByteOrder(QDataStream::BigEndian);
        if (damage == 0) {
            QVERIFY(buffer.seek(firstTile + 8));
            output << quint32(RasterTile::maximumCompressedBytes() + 1);
        } else if (damage == 1) {
            payload[firstTile + 12] ^= char(0xff); // Invalid zlib header with a valid outer digest.
        } else if (damage == 2) {
            QVERIFY(buffer.seek(firstTileReference));
            output << quint32(2); // Out-of-range index in the first tile-write operation.
        } else {
            const auto uuid = rasterImage().layers[1].uuid.toRfc4122();
            const auto offset = payload.indexOf(uuid);
            QVERIFY(offset >= 0);
            payload.replace(offset, uuid.size(), rasterImage().layers[0].uuid.toRfc4122());
        }
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(live.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(live.snapshot(), committed);
        QCOMPARE(live.preview(), preview);
        QCOMPARE(live.activeOperation(), pending);
        QCOMPARE(live.recoveryData(), recovery);
    }
    Limits limits;
    limits.rasterBytes = RgbaTile::ByteCount;
    OperationDocument small({64, 64}, limits);
    const auto unchanged = small.snapshot();
    QCOMPARE(small.restoreRecovery(good), Result::CapacityExceeded);
    QCOMPARE(small.snapshot(), unchanged);
}


void KisGpuOperationDocumentTest::filledFiguresPreserveStylesHistoryAndRecovery()
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
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(!recovered->figureStyle.outline);
    QCOMPARE(recovered->figureStyle.fill, style.fill);
    QCOMPARE(recovered->shape.kind, ShapeKind::Rectangle);
    QCOMPARE(recovered->brush.rgba, brush.rgba);
    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 17)), Result::Success);
    const auto previousFigure = std::get<PaintSnapshot>(previous.snapshot()->layers.first().operations.last());
    QCOMPARE(previousFigure->figureStyle.fill, style.fill);
    QVERIFY(!previousFigure->figureStyle.outline);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last())->figureStyle.fill, style.fill);
}

void KisGpuOperationDocumentTest::fillBlendModesFreezeHistoryAndRecovery_data()
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

void KisGpuOperationDocumentTest::fillBlendModesFreezeHistoryAndRecovery()
{
    QFETCH(int, mode);
    OperationDocument document(QSize(64, 64));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();
    QCOMPARE(document.fillLayer(layer, {221, 17, 44}, 255, false, false, LayerBlendMode(mode)),
             Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::FillLayer);
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QCOMPARE(operation->figureStyle.fill, (std::array<quint8, 4>{221, 17, 44, 255}));
    QCOMPARE(operation->figureStyle.fillBlendMode, LayerBlendMode(mode));
    const auto committed = document.snapshot();

    // One fill is one history entry; undo removes it and redo restores the
    // same frozen mode.
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), committed);

    // Recovery persists the mode and reproduces the same record.
    const auto recovery = document.recoveryData();
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(recovered->figureStyle.fillBlendMode, LayerBlendMode(mode));

    // Records predating format 48 have no fill mode and read as Normal.
    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 47)), Result::Success);
    const auto previousFill = std::get<PaintSnapshot>(previous.snapshot()->layers.first().operations.last());
    QCOMPARE(previousFill->figureStyle.fillBlendMode, LayerBlendMode::Normal);
    QCOMPARE(previousFill->figureStyle.fill, (std::array<quint8, 4>{221, 17, 44, 255}));
}

void KisGpuOperationDocumentTest::patternFillBlendModeFreezesHistoryAndRecovery()
{
    PatternFill pattern;
    pattern.size = QSize(2, 2);
    pattern.rgba = QByteArray::fromHex("030507ff3f051bff7b0005ff0b141eff");
    QVERIFY(pattern.isValid());

    OperationDocument document({64, 32});
    const auto layer = document.activeLayer();
    QCOMPARE(document.fillLayerPattern(layer, pattern, 255, false, false, LayerBlendMode::Multiply),
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

    const auto recovery = document.recoveryData();
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(recovered->figureStyle.pattern);
    QCOMPARE(*recovered->figureStyle.pattern, pattern);
    QCOMPARE(recovered->figureStyle.fillBlendMode, LayerBlendMode::Multiply);
}

void KisGpuOperationDocumentTest::rejectedFillBlendModesPreserveHistoryAndInput()
{
    OperationDocument document(QSize(32, 32));
    const auto layer = document.activeLayer();
    const auto initial = document.snapshot();

    QCOMPARE(document.fillLayer(layer, {17, 129, 241}, 255, false, false, LayerBlendMode(7)),
             Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.undoCount(), 0);

    const QList<InputSample> samples{{{4, 4}, 1}, {{28, 28}, 1}};
    FigureStyle outOfRange;
    outOfRange.outline = false;
    outOfRange.fill = std::array<quint8, 4>{17, 129, 241, 255};
    outOfRange.fillBlendMode = LayerBlendMode(7);
    QCOMPARE(document.paintFigure(layer, Brush{}, samples, {ShapeKind::Rectangle, {4, 4}, {28, 28}, 0},
                                  outOfRange, 42), Result::InvalidInput);
    QCOMPARE(document.snapshot(), initial);

    FigureStyle blendedOutline;
    blendedOutline.outline = true;
    blendedOutline.fill = std::array<quint8, 4>{17, 129, 241, 255};
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
    QCOMPARE(document.fillLayer(layer, {17, 129, 241}, 255, true, false, LayerBlendMode::Multiply),
             Result::Success);
    const auto erased = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QCOMPARE(erased->figureStyle.fillBlendMode, LayerBlendMode::Normal);
}

void KisGpuOperationDocumentTest::strokeBlendModesFreezeHistoryAndRecovery_data()
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

void KisGpuOperationDocumentTest::strokeBlendModesFreezeHistoryAndRecovery()
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
    QCOMPARE(pending->brush.rgba, (std::array<quint8, 4>{221, 17, 44, 255}));

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

    // Recovery persists the mode and reproduces the same record.
    const auto recovery = document.recoveryData();
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(recovered->brush.blendMode, LayerBlendMode(mode));

    // Records predating format 49 have no stroke mode and read as Normal.
    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 48)), Result::Success);
    const auto previousStroke = std::get<PaintSnapshot>(previous.snapshot()->layers.first().operations.last());
    QCOMPARE(previousStroke->brush.blendMode, LayerBlendMode::Normal);
    QCOMPARE(previousStroke->brush.rgba, (std::array<quint8, 4>{221, 17, 44, 255}));
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

void KisGpuOperationDocumentTest::patternFillFreezesPixelsHistoryAndRecovery()
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

    // Recovery persists the frozen RGBA pixels with the same operation values.
    const auto recovery = document.recoveryData();
    OperationDocument restored({1, 1});
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(recovered->figureStyle.pattern);
    QCOMPARE(*recovered->figureStyle.pattern, pattern);
    QCOMPARE(restored.historyAction(1), HistoryAction::FillLayer);

    // The frozen resource participates in the raster capacity limit.
    Limits limits;
    limits.rasterBytes = pattern.byteCount() - 1;
    OperationDocument bounded({64, 32}, limits);
    QCOMPARE(bounded.fillLayerPattern(bounded.activeLayer(), pattern), Result::CapacityExceeded);
    QCOMPARE(bounded.undoCount(), 0);
    QVERIFY(bounded.snapshot()->layers.first().operations.isEmpty());

    // Pattern coverage is filled interior coverage, not an outline stroke.
    FigureStyle style;
    style.outline = false;
    style.pattern = pattern;
    Shape freehand;
    freehand.kind = ShapeKind::Freehand;
    QCOMPARE(document.paintFigure(layer, Brush{}, {{{1, 1}, 1}}, freehand, style, 0), Result::InvalidInput);
    QCOMPARE(document.recoveryData(), recovery);
}

void KisGpuOperationDocumentTest::emptyFigureIntentPreservesHistoryAndRecovery()
{
    OperationDocument document(QSize(32, 32));
    const auto initial = document.snapshot();
    FigureStyle style;
    style.outline = false;
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, {{{0, 0}, 1}, {{16, 16}, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, {16, 16}, 0}, style, 42), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(document.isModified());
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto operation = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(!operation->figureStyle.outline);
    QVERIFY(!operation->figureStyle.fill);
    QCOMPARE(restored.undoCount(), 1);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.snapshot()->layers.first().operations.isEmpty());
    QVERIFY(!restored.isModified());
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(restored.isModified());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
}

void KisGpuOperationDocumentTest::pathsPreserveCommandsControlPointsAndRecovery()
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
    style.fill = std::array<quint8, 4>{221, 17, 44, 255};
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, samples, shape, style, 42), Result::Success);
    const auto pinned = document.snapshot();
    QCOMPARE(pinned->sampleCount, samples.size());
    QCOMPARE(document.undoCount(), 1);
    QCOMPARE(document.historyAction(1), HistoryAction::DrawPath);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), initial);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), pinned);
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto operation = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->shape.pathCommands, shape.pathCommands);
    QVERIFY(operation->shape.windingFill);
    QCOMPARE(operation->inputSamples(), samples);
    QCOMPARE(operation->figureStyle.fill, style.fill);
    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 18)), Result::Success);
    const auto previousPath = std::get<PaintSnapshot>(previous.snapshot()->layers.first().operations.last());
    QCOMPARE(previousPath->shape.pathCommands, shape.pathCommands);
    QCOMPARE(previousPath->inputSamples(), samples);
    QVERIFY(!previousPath->brush.pixelPerfect);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last())->inputSamples(), samples);
}

void KisGpuOperationDocumentTest::pathCallsRetainInputHistoryAndRecovery()
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(restored.hasActiveStroke());
    const auto recovered = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    QCOMPARE(recovered->shape.pathCalls, shape.pathCalls);
    QCOMPARE(recovered->inputSamples(), samples);
    QCOMPARE(restored.commitStroke(restored.activeOperation()), Result::Success);
    QCOMPARE(restored.undoCount(), 1);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.snapshot()->layers.first().operations.isEmpty());
    QCOMPARE(restored.redo(), Result::Success);
    const auto redone = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(redone->shape.pathCalls, shape.pathCalls);
    QCOMPARE(redone->inputSamples(), samples);
    QCOMPARE(document.cancelStroke(stroke), Result::Success);
    QVERIFY(document.snapshot()->layers.first().operations.isEmpty());
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
    const auto recovery = document.recoveryData();
    if (error == 0) shape.pathCalls = {{0, 6}, {2, 6}};
    else if (error == 1) shape.pathCalls = {{0, 7}};
    else if (error == 2) shape.pathCalls = {{1, 2}};
    else if (error == 3) shape.pathCalls = {{0, 5}};
    else if (error == 4) shape.pathCalls = {{2, 2}};
    else { shape.kind = ShapeKind::Line; shape.pathCommands.clear(); }
    QCOMPARE(document.updateShape(stroke, shape), Result::InvalidInput);
    QCOMPARE(document.preview(), pinned);
    QCOMPARE(document.recoveryData(), recovery);
    QCOMPARE(document.undoCount(), 0);
    OperationDocument empty(QSize(128, 128));
    const auto before = empty.snapshot();
    QCOMPARE(empty.paintFigure(empty.activeLayer(), Brush{}, samples, shape, {}, 42), Result::InvalidInput);
    QCOMPARE(empty.snapshot(), before);
    QCOMPARE(empty.undoCount(), 0);
}

void KisGpuOperationDocumentTest::legacyGaussianPathKeepsItsRecordedInput()
{
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(LibrePaint::Image::Gpu::Test::gaussianCompoundPathRecoveryV38()), Result::Success);
    QCOMPARE(restored.snapshot()->size, QSize(128, 128));
    const auto operation = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(operation->shape.kind, ShapeKind::Path);
    QVERIFY(operation->shape.pathCalls.isEmpty());
    const QList<InputSample> samples{{{12.25, 13.75}, 1}, {{105.5, 25.75}, 1},
                                    {{18.125, 91.5}, 1}, {{112.5, 78.25}, 1}};
    QCOMPARE(operation->inputSamples(), samples);
    QVERIFY(operation->brush.gaussianWash);
    // The record identifies the frozen settings XML, including its half flow.
    QCOMPARE(operation->brush.gaussianWash->resourceDigest.toHex(), QByteArrayLiteral("1167275f113c8d34d6915343428acc1831e2f19b96ff42614d64a9c30ce6116d"));
    OperationDocument again(QSize(1, 1));
    QCOMPARE(again.restoreRecovery(restored.recoveryData()), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(again.snapshot()->layers.first().operations.last())->inputSamples(), samples);
    QCOMPARE(again.undo(), Result::Success);
    QCOMPARE(again.redo(), Result::Success);
}

void KisGpuOperationDocumentTest::damagedPathCallRecoveryPreservesTheActiveDocument()
{
    OperationDocument source(QSize(128, 128)), target(QSize(128, 128));
    Shape shape;
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::LineTo, PathCommand::MoveTo, PathCommand::LineTo};
    shape.pathCalls = {{0, 2}, {2, 4}};
    const QList<InputSample> samples{{{12, 24}, 1}, {{42, 24}, 1}, {{76, 96}, 1}, {{112, 96}, 1}};
    QCOMPARE(source.paintFigure(source.activeLayer(), Brush{}, samples, shape, {}, 42), Result::Success);
    const auto stroke = target.beginStroke(target.activeLayer(), Brush{}, 7);
    QCOMPARE(target.appendSamples(stroke, {{{16, 32}, .5}}), Result::Success);
    const auto pinned = target.preview();
    const auto recovery = target.recoveryData();
    const auto payload = source.recoveryData().chopped(32);
    QByteArray pathRecord;
    QDataStream pathStream(&pathRecord, QIODevice::WriteOnly);
    pathStream << quint8(0) << quint32(4) << quint8(0) << quint8(1) << quint8(0) << quint8(1)
               << quint32(2) << quint32(0) << quint32(2) << quint32(2) << quint32(4);
    const auto position = payload.indexOf(pathRecord);
    QVERIFY(position >= 0 && position == payload.lastIndexOf(pathRecord));
    const QList<QPair<qsizetype, quint32>> damage{{9, std::numeric_limits<quint32>::max()},
                                                {13, 1}, {17, 3}, {25, 5}};
    for (const auto &field : damage) {
        auto corrupted = payload;
        QByteArray word;
        QDataStream stream(&word, QIODevice::WriteOnly);
        stream << field.second;
        corrupted.replace(position + field.first, 4, word);
        corrupted.append(QCryptographicHash::hash(corrupted, QCryptographicHash::Sha256));
        QCOMPARE(target.restoreRecovery(corrupted), Result::InvalidRecovery);
        QCOMPARE(target.preview(), pinned);
        QCOMPARE(target.recoveryData(), recovery);
        QVERIFY(target.hasActiveStroke());
    }
}

void KisGpuOperationDocumentTest::pixelCorrectionRetainsOriginalSensorsAndPendingRecovery()
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recovered = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 19)), Result::Success);
    QVERIFY(std::get<PaintSnapshot>(previous.preview()->layers.first().operations.last())->brush.pixelPerfect);
    QVERIFY(recovered->brush.pixelPerfect);
    QCOMPARE(recovered->inputSamples(), samples);
    QCOMPARE(restored.commitStroke(restored.activeOperation()), Result::Success);
    QCOMPARE(restored.undoCount(), 1);
    const auto painted = restored.snapshot();
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot(), painted);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::selectionRectIsFrozenInOperationsAndRecovery()
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

    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.selectionRect(), QRect(50, 60, 20, 10));
    const auto &recovered = restored.snapshot()->layers.first().operations;
    QCOMPARE(std::get<PaintSnapshot>(recovered[0])->selectionRect, QRect(10, 20, 30, 40));
    QCOMPARE(std::get<PaintSnapshot>(recovered[1])->selectionRect, QRect(50, 60, 20, 10));

    QCOMPARE(document.setSelectionRect(QRect(OperationDocument::CoordinateLimit, 0, 2, 2)), Result::InvalidInput);
    QCOMPARE(document.selectionRect(), QRect(50, 60, 20, 10));
    QCOMPARE(document.clearSelection(), Result::Success);
    QVERIFY(document.selectionRect().isEmpty());
    const auto later = document.beginStroke(document.activeLayer(), Brush{}, 3);
    QVERIFY(!later.isNull());
    QCOMPARE(document.appendSamples(later, {{{8, 8}, 1}}), Result::Success);
    QCOMPARE(document.commitStroke(later), Result::Success);
    QVERIFY(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->selectionRect.isEmpty());
    QCOMPARE(std::get<PaintSnapshot>(recovered[0])->selectionRect, QRect(10, 20, 30, 40));
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.selectionRect().isEmpty());
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.selectionRect(), QRect(50, 60, 20, 10));
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.selectionRect(), QRect(50, 60, 20, 10));
    QCOMPARE(document.historyAction(document.undoCount()), HistoryAction::ChangeSelection);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.selectionRect().isEmpty());
}

void KisGpuOperationDocumentTest::invertedSelectionSurvivesRecovery()
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.selectionRect(), QRect(8, 8, 32, 32));
    QVERIFY(restored.selectionInverted());
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.selectionInverted());
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(restored.selectionInverted());
}

void KisGpuOperationDocumentTest::ellipticalSelectionSurvivesRecovery()
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.selectionRect(), QRect(8, 8, 32, 24));
    QCOMPARE(restored.selectionKind(), SelectionKind::Ellipse);
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.selectionKind(), SelectionKind::Rectangle);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.selectionKind(), SelectionKind::Ellipse);

    OperationDocument previous(QSize(1, 1));
    QCOMPARE(previous.restoreRecovery(legacyRecovery(document, 43)), Result::Success);
    QCOMPARE(previous.selectionRect(), QRect(8, 8, 32, 24));
    QCOMPARE(previous.selectionKind(), SelectionKind::Ellipse);
    QCOMPARE(previous.undo(), Result::Success);
    QCOMPARE(previous.selectionKind(), SelectionKind::Rectangle);
    QCOMPARE(previous.redo(), Result::Success);
    QCOMPARE(previous.selectionKind(), SelectionKind::Ellipse);
}

void KisGpuOperationDocumentTest::freehandSelectionFreezesCoverageHistoryAndRecovery()
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

    const auto bytes = document.recoveryData();
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.selectionKind(), SelectionKind::Freehand);
    QCOMPARE(restored.selectionRect(), mask->bounds());
    QVERIFY(restored.selectionMask());
    QVERIFY(*restored.selectionMask() == *mask);
    QVERIFY(restored.selectionMask()->contains(12, 12));
    QVERIFY(!restored.selectionMask()->contains(48, 48));
    QCOMPARE(restored.recoveryData(), bytes);
    const auto restoredPaint = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QCOMPARE(restoredPaint->selectionKind, SelectionKind::Freehand);
    QVERIFY(restoredPaint->selectionMask);
    QVERIFY(restoredPaint->selectionMask && *restoredPaint->selectionMask == *mask);

    QCOMPARE(restored.setSelectionMask(mask, true), Result::Success);
    QVERIFY(restored.selectionInverted());
    const auto invertedBytes = restored.recoveryData();
    OperationDocument inverted(QSize(1, 1));
    QCOMPARE(inverted.restoreRecovery(invertedBytes), Result::Success);
    QVERIFY(inverted.selectionInverted());
    QVERIFY(*inverted.selectionMask() == *mask);

    QCOMPARE(inverted.setSelectionMask(SelectionMaskSP{}), Result::Success);
    QVERIFY(inverted.selectionRect().isEmpty());
    QVERIFY(!inverted.selectionMask());
    QCOMPARE(inverted.selectionKind(), SelectionKind::Rectangle);
}

void KisGpuOperationDocumentTest::featheredSelectionAddsAlphaHistoryAndRecovery()
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
    QCOMPARE(soft->format(), SelectionMask::CoverageFormat::Alpha);
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

    // Recovery preserves both representations, their order and the frozen stroke.
    const auto bytes = document.recoveryData();
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QVERIFY(restored.selectionMask());
    QVERIFY(restored.selectionMask()->isAlpha());
    QVERIFY(*restored.selectionMask() == *soft);
    QCOMPARE(restored.selectionMask()->coverage(10, 48), outer);
    QCOMPARE(restored.recoveryData(), bytes);
    const auto restoredPaint = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(restoredPaint->selectionMask);
    QVERIFY(restoredPaint->selectionMask->isAlpha());
    QVERIFY(*restoredPaint->selectionMask == *soft);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.selectionMask());
    QVERIFY(restored.selectionMask()->isAlpha());
    QVERIFY(*restored.selectionMask() == *soft);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.selectionMask());
    QVERIFY(!restored.selectionMask()->isAlpha());
    QVERIFY(*restored.selectionMask() == *hard);
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(restored.selectionMask()->isAlpha());
    QVERIFY(*restored.selectionMask() == *soft);

    // An inverted alpha selection keeps its fractional coverage and flag.
    QCOMPARE(restored.setSelectionMask(soft, true), Result::Success);
    QVERIFY(restored.selectionInverted());
    const auto invertedBytes = restored.recoveryData();
    OperationDocument inverted(QSize(1, 1));
    QCOMPARE(inverted.restoreRecovery(invertedBytes), Result::Success);
    QVERIFY(inverted.selectionInverted());
    QVERIFY(inverted.selectionMask()->isAlpha());
    QVERIFY(*inverted.selectionMask() == *soft);
    QCOMPARE(inverted.historyAction(inverted.undoCount()), HistoryAction::ChangeSelection);
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
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recovered = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    QVERIFY(recovered->brush.basicSmoothing);
    QCOMPARE(recovered->inputSamples(), samples);
    QCOMPARE(recovered->smoothingFlushes, pending->smoothingFlushes);
    QVERIFY(!recovered->smoothingFinished);
    QCOMPARE(restored.commitStroke(restored.activeOperation()), Result::Success);
    const auto final = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(final->smoothingFinished);
    QVERIFY(!pending->smoothingFinished);
    QCOMPARE(final->inputSamples(), samples);
    QCOMPARE(final->smoothingFlushes, pending->smoothingFlushes);
    QCOMPARE(restored.undoCount(), 1);
    const auto painted = restored.snapshot();
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot(), painted);
    OperationDocument completed(QSize(1, 1));
    QCOMPARE(completed.restoreRecovery(restored.recoveryData()), Result::Success);
    QVERIFY(std::get<PaintSnapshot>(completed.snapshot()->layers.first().operations.last())->smoothingFinished);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::weightedSmoothingRetainsFrozenSettingsSpeedAndRecovery()
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
    const auto beforeInvalid = document.recoveryData();
    for (const auto speed : {-0.1, 1.1, std::numeric_limits<qreal>::quiet_NaN()}) {
        auto invalid = samples.last();
        invalid.speed = speed;
        QCOMPARE(document.appendSamples(id, {invalid}), Result::InvalidInput);
        QCOMPARE(document.preview(), pinned);
        QCOMPARE(document.recoveryData(), beforeInvalid);
    }
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    const auto recovered = std::get<PaintSnapshot>(restored.preview()->layers.first().operations.last());
    QCOMPARE(recovered->inputSamples(), samples);
    QVERIFY(recovered->brush == operation->brush);
    QCOMPARE(recovered->smoothingFlushes, QList<qsizetype>{2});
    QCOMPARE(restored.commitStroke(restored.activeOperation()), Result::Success);
    QCOMPARE(restored.undoCount(), 1);
    const auto painted = restored.snapshot();
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot(), painted);
    QVERIFY(!operation->smoothingFinished);
    QCOMPARE(operation->inputSamples(), samples);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.undoCount(), 0);
    const auto original = document.recoveryData();
    for (int invalid = 0; invalid < 5; ++invalid) {
        auto rejected = operation->brush;
        if (invalid == 0) rejected.basicSmoothing = false;
        if (invalid == 1) rejected.pixelPerfect = true;
        if (invalid == 2) rejected.weightedSmoothing->distanceMin = -1;
        if (invalid == 3) rejected.weightedSmoothing->distanceMax = std::numeric_limits<qreal>::infinity();
        if (invalid == 4) rejected.weightedSmoothing->tailAggressiveness = 1.1;
        QVERIFY(document.beginStroke(document.activeLayer(), rejected, 7).isNull());
        QCOMPARE(document.recoveryData(), original);
    }
    // Version 20 has no drawing-speed field; its existing input defaults to zero.
    OperationDocument previous(QSize(64, 64));
    const auto plain = previous.beginStroke(previous.activeLayer(), Brush{}, 7);
    QCOMPARE(previous.appendSamples(plain, {{{8, 16}, 1}, {{12, 20}, 0.8}}), Result::Success);
    QCOMPARE(previous.commitStroke(plain), Result::Success);
    QCOMPARE(document.restoreRecovery(legacyRecovery(previous, 20)), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples().last().speed, 0.0);
}

void KisGpuOperationDocumentTest::basicSmoothingRejectsConflictingModesAndShapeChanges()
{
    OperationDocument document(QSize(64, 64));
    const auto original = document.recoveryData();
    Brush brush;
    brush.basicSmoothing = brush.pixelPerfect = true;
    QVERIFY(document.beginStroke(document.activeLayer(), brush, 7).isNull());
    QCOMPARE(document.recoveryData(), original);
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

void KisGpuOperationDocumentTest::damagedSmoothingRecoveryPreservesTheActiveDocument()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.basicSmoothing = true;
    const auto id = document.beginStroke(document.activeLayer(), brush, 7);
    QCOMPARE(document.appendSamples(id, {{{8, 32}, 1}, {{32, 8}, 1}}), Result::Success);
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    QCOMPARE(document.appendSamples(id, {{{56, 32}, 1}}), Result::Success);
    QCOMPARE(document.flushSmoothing(id), Result::Success);
    const auto original = document.recoveryData();
    const auto snapshot = document.snapshot();
    const auto preview = document.preview();
    QByteArray prefix;
    QDataStream prefixStream(&prefix, QIODevice::WriteOnly);
    prefixStream << quint8(2) << quint32(2) << quint32(2) << quint32(3) << quint32(3) << qreal(8) << qreal(32);
    const auto position = original.indexOf(prefix);
    QVERIFY(position >= 0);
    QCOMPARE(position, original.lastIndexOf(prefix));
    for (const int corruption : {0, 1, 2, 3, 4, 5}) {
        auto payload = original.chopped(32);
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QVERIFY(buffer.seek(position + (corruption < 2 ? 0 : corruption == 2 ? 1 : corruption == 3 ? 5 : 9)));
        QDataStream stream(&buffer);
        if (corruption == 0) stream << quint8(3);
        else if (corruption == 1) stream << quint8(6);
        else if (corruption == 2) stream << std::numeric_limits<quint32>::max();
        else stream << quint32(corruption == 3 ? 0 : corruption == 4 ? 2 : 4);
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(document.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), snapshot);
        QCOMPARE(document.preview(), preview);
        QCOMPARE(document.activeOperation(), id);
    }
    OperationDocument completed(QSize(1, 1));
    QCOMPARE(completed.restoreRecovery(original), Result::Success);
    QCOMPARE(completed.commitStroke(completed.activeOperation()), Result::Success);
    auto payload = completed.recoveryData().chopped(32);
    prefix[0] = char(6);
    const auto finalPosition = payload.indexOf(prefix);
    QVERIFY(finalPosition >= 0);
    payload[finalPosition] = char(2);
    payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
    QCOMPARE(document.restoreRecovery(payload), Result::InvalidRecovery);
    QCOMPARE(document.snapshot(), snapshot);
    QCOMPARE(document.preview(), preview);
}

void KisGpuOperationDocumentTest::emptyPathsPreserveHistoryAndPendingRecovery()
{
    OperationDocument document(QSize(64, 64));
    Shape shape;
    shape.kind = ShapeKind::Path;
    QCOMPARE(document.paintFigure(document.activeLayer(), Brush{}, {}, shape, FigureStyle{}, 42), Result::Success);
    QCOMPARE(document.undoCount(), 1);
    QVERIFY(document.isModified());
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.undoCount(), 1);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.isModified());
    QCOMPARE(restored.redo(), Result::Success);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 43);
    QVERIFY(!id.isNull());
    QCOMPARE(document.updateFigure(id, shape, FigureStyle{}), Result::Success);
    OperationDocument pending(QSize(1, 1));
    QCOMPARE(pending.restoreRecovery(document.recoveryData()), Result::Success);
    QVERIFY(!pending.activeOperation().isNull());
    QCOMPARE(std::get<PaintSnapshot>(pending.preview()->layers.first().operations.last())->shape.kind, ShapeKind::Path);
    QCOMPARE(pending.cancelStroke(pending.activeOperation()), Result::Success);
    QCOMPARE(pending.undoCount(), 1);
    QCOMPARE(document.cancelStroke(id), Result::Success);
}

void KisGpuOperationDocumentTest::invalidPathsPreserveHistoryAndInputBudgets()
{
    OperationDocument document(QSize(64, 64));
    const auto before = document.recoveryData();
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
    QCOMPARE(document.recoveryData(), before);
    QVERIFY(document.activeOperation().isNull());
    Limits limits;
    limits.samplesPerStroke = limits.samplesPerDocument = 3;
    OperationDocument bounded(QSize(64, 64), limits);
    const auto boundedBefore = bounded.recoveryData();
    shape.kind = ShapeKind::Path;
    shape.pathCommands = {PathCommand::MoveTo, PathCommand::CubicTo};
    QCOMPARE(bounded.paintFigure(bounded.activeLayer(), Brush{}, samples, shape, FigureStyle{}, 42), Result::CapacityExceeded);
    QCOMPARE(bounded.recoveryData(), boundedBefore);
    QVERIFY(bounded.activeOperation().isNull());
}

void KisGpuOperationDocumentTest::gaussianWashSettingsSurviveHistoryAndRecovery()
{
    OperationDocument document(QSize(128, 128));
    Brush brush;
    GaussianWash settings;
    settings.resourceDigest = QByteArray(32, 'r');
    settings.angle = 6.28319;
    settings.canvasOpacity = .57;
    settings.flow = .5;
    settings.creamy = false;
    for (std::size_t i = 0; i < settings.sizePressure.size(); ++i) {
        settings.sizePressure[i] = qreal(i * i) / (255 * 255);
        settings.opacityPressure[i] = qreal(i) / 255;
    }
    brush.gaussianWash = settings;
    Shape shape;
    shape.kind = ShapeKind::Line;
    shape.first = {12.25, 13.75};
    shape.second = {60.125, 31.5};
    QCOMPARE(document.paintFigure(document.activeLayer(), brush,
        {sample(12.25, 13.75, .12), sample(60.125, 31.5, .75)}, shape, FigureStyle{}, 1789), Result::Success);
    brush.gaussianWash->flow = 1;
    const auto operation = std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last());
    QVERIFY(operation->brush.gaussianWash == std::optional<GaussianWash>(settings));
    const auto record = document.recoveryData();
    QVERIFY(!record.isEmpty());
    QCOMPARE(document.undo(), Result::Success);
    OperationDocument restored(QSize(1, 1));
    QCOMPARE(restored.restoreRecovery(document.recoveryData()), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    const auto recovered = std::get<PaintSnapshot>(restored.snapshot()->layers.first().operations.last());
    QVERIFY(recovered->brush == operation->brush);
    QCOMPARE(restored.recoveryData(), record);
}

void KisGpuOperationDocumentTest::stationaryGaussianCommitInvalidatesPendingPixelsAndRetainsInput_data()
{
    QTest::addColumn<int>("count");
    for (int count : {1, 2, 3, 8, 1025})
        QTest::newRow(qPrintable(QString::number(count))) << count;
}

void KisGpuOperationDocumentTest::stationaryGaussianCommitInvalidatesPendingPixelsAndRetainsInput()
{
    QFETCH(int, count);
    OperationDocument document(QSize(128, 128));
    Brush brush;
    brush.gaussianWash.emplace();
    brush.gaussianWash->resourceDigest = QByteArray(32, 'r');
    const auto id = document.beginStroke(document.activeLayer(), brush, 1789);
    QList<InputSample> input;
    for (int i = 0; i < count; ++i)
        input.append({{12.25, 13.75}, qreal(i + 1) / count,
                      qreal(i % 91), -qreal(i % 91), qreal(i % 360), quint64(i * 16), qreal(i) / count});
    QCOMPARE(document.appendSamples(id, input), Result::Success);
    const auto pending = document.preview();
    const auto operation = std::get<PaintSnapshot>(pending->layers.first().operations.last());
    const auto record = document.recoveryData();
    OperationDocument recovered(QSize(1, 1));
    QCOMPARE(recovered.restoreRecovery(record), Result::Success);
    for (auto *value : {&document, &recovered}) {
        const auto intent = value->preview();
        const auto before = std::get<PaintSnapshot>(intent->layers.first().operations.last());
        QCOMPARE(value->commitStroke(value->activeOperation()), Result::Success);
        const auto after = std::get<PaintSnapshot>(value->snapshot()->layers.first().operations.last());
        // Pixel caches distinguish the empty pending stroke from its final tap
        // by the operation version, while retained input remains immutable.
        QVERIFY(after->version > before->version);
        QCOMPARE(after->id, before->id);
        QCOMPARE(after->inputSamples(), input);
        QCOMPARE(before->inputSamples(), input);
        QCOMPARE(intent->pendingOperation(), before->id);
        QVERIFY(value->snapshot()->pendingOperation().isNull());
        QCOMPARE(value->undoCount(), 1);
    }
    QCOMPARE(operation->inputSamples(), input);
    QCOMPARE(pending->pendingOperation(), id);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(std::get<PaintSnapshot>(document.snapshot()->layers.first().operations.last())->inputSamples(), input);
}

void KisGpuOperationDocumentTest::transparencyMaskHistoryCompositionAndRecovery()
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

    // Recovery retains masks on every version, their visibility and the
    // add/toggle/remove history actions.
    const auto bytes = document.recoveryData();
    OperationDocument restored(size);
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.undoCount(), 5);
    QCOMPARE(restored.redoCount(), 0);
    QVERIFY(!restored.layerTransparencyMask(restored.snapshot()->layers.first().id));
    QCOMPARE(restored.historyAction(2), HistoryAction::AddTransparencyMask);
    QCOMPARE(restored.historyAction(3), HistoryAction::PaintStroke);
    QCOMPARE(restored.historyAction(4), HistoryAction::ChangeMaskVisibility);
    QCOMPARE(restored.historyAction(5), HistoryAction::RemoveTransparencyMask);
    QCOMPARE(restored.undo(), Result::Success);
    const auto restoredLayer = restored.snapshot()->layers.first().id;
    QVERIFY(restored.layerTransparencyMask(restoredLayer));
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *mask);
    QVERIFY(!restored.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(restored.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.layerTransparencyMask(restoredLayer));
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *mask);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(!restored.layerTransparencyMaskVisible(restoredLayer));
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(!restored.layerTransparencyMask(restoredLayer));
    QCOMPARE(restored.undoCount(), 5);
    QCOMPARE(restored.recoveryData(), bytes);
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

void KisGpuOperationDocumentTest::transparencyMaskCoverageReplacementSurvivesRecovery()
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

    const auto bytes = document.recoveryData();
    OperationDocument restored(size);
    QCOMPARE(restored.restoreRecovery(bytes), Result::Success);
    QCOMPARE(restored.undoCount(), document.undoCount());
    const auto restoredLayer = restored.snapshot()->layers.first().id;
    QVERIFY(restored.layerTransparencyMask(restoredLayer));
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(restored.historyAction(undoBefore + 1), HistoryAction::PaintStroke);
    QCOMPARE(restored.historyAction(undoBefore + 2), HistoryAction::FillLayer);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *edited);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QVERIFY(*restored.layerTransparencyMask(restoredLayer) == *original);
    QCOMPARE(restored.recoveryData(), bytes);
}

void KisGpuOperationDocumentTest::invalidGaussianWashSettingsPreserveTheDocument()
{
    OperationDocument document(QSize(64, 64));
    Brush brush;
    brush.gaussianWash.emplace();
    brush.gaussianWash->resourceDigest = QByteArray(32, 'r');
    const auto before = document.recoveryData();
    for (int invalid = 0; invalid < 8; ++invalid) {
        auto bad = brush;
        switch (invalid) {
        case 0: bad.gaussianWash->flow = -1; break;
        case 1: bad.gaussianWash->canvasOpacity = 1.01; break;
        case 2: bad.gaussianWash->angle = std::numeric_limits<qreal>::infinity(); break;
        case 3: bad.gaussianWash->sizePressure[128] = std::numeric_limits<qreal>::quiet_NaN(); break;
        case 4: bad.gaussianWash->opacityPressure[128] = 2; break;
        case 5: bad.gaussianWash->resourceDigest.clear(); break;
        case 6: bad.gaussianWash->sizePressure.removeLast(); break;
        case 7: bad.gaussianWash->opacityPressure.append(1); break;
        }
        QVERIFY(document.beginStroke(document.activeLayer(), bad, 1789).isNull());
        QCOMPARE(document.recoveryData(), before);
    }
}

QTEST_GUILESS_MAIN(KisGpuOperationDocumentTest)
#include "KisGpuOperationDocumentTest.moc"
