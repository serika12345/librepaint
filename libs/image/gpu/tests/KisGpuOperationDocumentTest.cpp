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
    void emptyFigureIntentPreservesHistoryAndRecovery();
    void pathsPreserveCommandsControlPointsAndRecovery();
    void pixelCorrectionRetainsOriginalSensorsAndPendingRecovery();
    void basicSmoothingRetainsIdleBoundariesSensorsAndFinalIntent();
    void weightedSmoothingRetainsFrozenSettingsSpeedAndRecovery();
    void basicSmoothingRejectsConflictingModesAndShapeChanges();
    void damagedSmoothingRecoveryPreservesTheActiveDocument();
    void emptyPathsPreserveHistoryAndPendingRecovery();
    void invalidPathsPreserveHistoryAndInputBudgets();
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
    QTest::newRow("two-colour-defaults") << QByteArray::fromHex("ff0000ff") << QByteArray::fromHex("0000ffff")
        << true << true << 128 << QByteArray::fromHex("7f0080ff") << QByteArray::fromHex("7f008080");
    QTest::newRow("hidden-sibling-normalizes-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << true << false << 255 << QByteArray::fromHex("00000000") << QByteArray::fromHex("00000000");
    QTest::newRow("single-child-keeps-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << false << false << 128 << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000");
    QTest::newRow("nested-full-opacity-keeps-transparent-rgb") << QByteArray::fromHex("11223300") << QByteArray::fromHex("00000000")
        << false << false << 255 << QByteArray::fromHex("11223300") << QByteArray::fromHex("11223300");
    QTest::newRow("nested-alpha-is-quantized-at-each-group") << QByteArray::fromHex("ff000080") << QByteArray::fromHex("00000000")
        << false << false << 128 << QByteArray::fromHex("ff000080") << QByteArray::fromHex("ff000040");
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
        if (index == 1) source.layers[index].opacity = 128;
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
    QCOMPARE(document.writeLayerPosition(2, {7, -9}), Result::LayerUnavailable);
    QCOMPARE(document.writeLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.setLayerChannelMask(2, 7), Result::LayerUnavailable);
    QCOMPARE(document.recoveryData(), beforeRejected);
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
    QVERIFY(reader.seek(50)); // The fixed header includes the tile count.
    QDataStream input(&reader);
    input.setVersion(QDataStream::Qt_5_15);
    input.setByteOrder(QDataStream::BigEndian);
    for (int i = 0; i < 2; ++i) {
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
            QVERIFY(buffer.seek(58));
            output << quint32(RasterTile::maximumCompressedBytes() + 1);
        } else if (damage == 1) {
            payload[62] ^= char(0xff); // Invalid zlib header with a valid outer digest.
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

QTEST_GUILESS_MAIN(KisGpuOperationDocumentTest)
#include "KisGpuOperationDocumentTest.moc"
