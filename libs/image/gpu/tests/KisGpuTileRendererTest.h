/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISGPUTILERENDERERTEST_H
#define KISGPUTILERENDERERTEST_H

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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <gpu/KisGpuTileRenderer.h>
#include <gpu/KisGpuDevice.h>
#include <gpu/KisGpuReadbackBatch_p.h>
#include <webgpu/wgpu.h>
#include <KisGpuPixelTolerance.h>
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
    void convolutionCapacityKeepsReadersAndSparseSelectionRetry_data();
    void convolutionCapacityKeepsReadersAndSparseSelectionRetry();
    void retainedFiltersFollowSourceEditsWithoutBakingPixels_data();
    void retainedFiltersFollowSourceEditsWithoutBakingPixels();
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
    void normalProjectionStaysWithinOneStep_data();
    void normalProjectionStaysWithinOneStep();
    void normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries_data();
    void normalProjectionKeepsDefaultAndTileColorsAtRoundingBoundaries();
    void explicitPassThroughReadAfterParentKeepsSubtreePixels();
    void passThroughHierarchyMatchesNativeRootPixels_data();
    void passThroughHierarchyMatchesNativeRootPixels();
    void separableLayerProjectionMatchesNativePixels_data();
    void separableLayerProjectionMatchesNativePixels();
    void channelCopyGroupsKeepHiddenRgbAndNormalizeRoot();
    void minimumNonlinearPaintOpacityKeepsNativeQuantization();
    void wordBlendOperationsFitWithoutByteLookupResources();
    void coupledProjectionRetainsOtherBlendOperations();
    void transparentSourcesReplaceProjectionAlpha_data();
    void transparentSourcesReplaceProjectionAlpha();
    void threeLayerProjectionFitsOneOutputTile_data();
    void threeLayerProjectionFitsOneOutputTile();
    void multiPassGroupsKeepDistinctLayerParameters_data();
    void multiPassGroupsKeepDistinctLayerParameters();
    void previewProjectionTracksBackgroundPropertiesAndRetainedPixels();
    void switchingDocumentsReleasesPreviewDerivedPixelsAtCapacity();
    void discardedAffinePreviewKeepsWorkingBytesUntilSubmissionCompletes();
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
    void maskedCopyKeepsNativeTransparentComponents();
    void autoBrushProducesCoverageAndRetainsPixels();
    void paperResourceKeepsFrozenPixelsHistoryAndCapacity();
    void additiveMaskingBrushKeepsFrozenInputAndHistory();
    void gaussianEraseKeepsColorsHistory_data();
    void gaussianEraseKeepsColorsHistory();
    void gaussianFreehandKeepsInitialDistanceAndFinalTap_data();
    void gaussianFreehandKeepsInitialDistanceAndFinalTap();
    void autoBrushSkipsDabsBelowTheBrushMinimum();
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
    void scatteredInputKeepsFrozenSeedAndPartitions_data();
    void scatteredInputKeepsFrozenSeedAndPartitions();
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

#endif
