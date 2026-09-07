/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image.h"

#include <QTest>

#include <type_traits>

#define ASSERT_IMAGE_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImage::method)), signature>)

class KisImageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionFactoryAndCopySchemaRemainStable();
    void graphListenerSignaturesRemainStable();
    void lockingAndSchedulerStateSignaturesRemainStable();
    void strokeAndUndoRequestSignaturesRemainStable();
    void updateFilteringAndRefreshSignaturesRemainStable();
    void geometryResolutionAndCoordinateSignaturesRemainStable();
    void imageAndNodeTransformSignaturesRemainStable();
    void colorProfileAndProofingSignaturesRemainStable();
    void layerHierarchyAndSelectionSignaturesRemainStable();
    void annotationAndCompositionSignaturesRemainStable();
    void wraparoundLodAndIsolationSignaturesRemainStable();
    void undoAdapterAndModificationSignaturesRemainStable();
    void imageAndLifecycleSignalSignaturesRemainStable();
    void nodeAndIsolationSignalSignaturesRemainStable();
    void strokeRequestSignalSignaturesRemainStable();
};

// clang-format off
void KisImageSchemaContractTest::typeConstructionFactoryAndCopySchemaRemainStable()
{
    using Image = KisImage;
    static_assert(std::is_class_v<Image>);
    static_assert(std::is_constructible_v<Image, KisUndoStore *, qint32, qint32, const KoColorSpace *, const QString &>);
    static_assert(std::has_virtual_destructor_v<Image>);
    ASSERT_IMAGE_SIGNATURE(fromQImage, KisImageSP (*)(const QImage &, KisUndoStore *));
    ASSERT_IMAGE_SIGNATURE(clone, Image *(Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(copyFromImage, void (Image::*)(const Image &));
}

void KisImageSchemaContractTest::graphListenerSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(aboutToAddANode, void (Image::*)(KisNode *, int));
    ASSERT_IMAGE_SIGNATURE(aboutToRemoveANode, void (Image::*)(KisNode *, int));
    ASSERT_IMAGE_SIGNATURE(graphOverlayNode, KisNode *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(invalidateAllFrames, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(invalidateFrames, void (Image::*)(const KisTimeSpan &, const QRect &));
    ASSERT_IMAGE_SIGNATURE(keyframeChannelAboutToBeRemoved, void (Image::*)(KisNode *, KisKeyframeChannel *));
    ASSERT_IMAGE_SIGNATURE(keyframeChannelHasBeenAdded, void (Image::*)(KisNode *, KisKeyframeChannel *));
    ASSERT_IMAGE_SIGNATURE(nodeChanged, void (Image::*)(KisNode *));
    ASSERT_IMAGE_SIGNATURE(nodeCollapsedChanged, void (Image::*)(KisNode *));
    ASSERT_IMAGE_SIGNATURE(nodeHasBeenAdded, void (Image::*)(KisNode *, int, KisNodeAdditionFlags));
    ASSERT_IMAGE_SIGNATURE(notifyProjectionUpdated, void (Image::*)(const QRect &));
    ASSERT_IMAGE_SIGNATURE(notifySelectionChanged, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(requestProjectionUpdate, void (Image::*)(KisNode *, const QVector<QRect> &, KisProjectionUpdateFlags));
    ASSERT_IMAGE_SIGNATURE(requestTimeSwitch, void (Image::*)(int));
}

void KisImageSchemaContractTest::lockingAndSchedulerStateSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(barrierLock, void (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(immediateLockForReadOnly, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(isIdle, bool (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(locked, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(setWorkingThreadsLimit, void (Image::*)(int));
    ASSERT_IMAGE_SIGNATURE(tryBarrierLock, bool (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(unlock, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(waitForDone, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(workingThreadsLimit, int (Image::*)() const);
}

void KisImageSchemaContractTest::strokeAndUndoRequestSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(addJob, void (Image::*)(KisStrokeId, KisStrokeJobData *));
    ASSERT_IMAGE_SIGNATURE(cancelStroke, bool (Image::*)(KisStrokeId));
    ASSERT_IMAGE_SIGNATURE(endStroke, void (Image::*)(KisStrokeId));
    ASSERT_IMAGE_SIGNATURE(requestRedoDuringStroke, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(requestStrokeCancellation, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(requestStrokeEnd, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(requestStrokeEndActiveNode, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(requestUndoDuringStroke, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(startStroke, KisStrokeId (Image::*)(KisStrokeStrategy *));
    ASSERT_IMAGE_SIGNATURE(tryUndoUnfinishedLod0Stroke, UndoResult (Image::*)());
}

void KisImageSchemaContractTest::updateFilteringAndRefreshSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(addProjectionUpdatesFilter, KisProjectionUpdatesFilterCookie (Image::*)(KisProjectionUpdatesFilterSP));
    ASSERT_IMAGE_SIGNATURE(addSpontaneousJob, void (Image::*)(KisSpontaneousJob *));
    ASSERT_IMAGE_SIGNATURE(blockUpdates, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(currentProjectionUpdatesFilter, KisProjectionUpdatesFilterCookie (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(disableDirtyRequests, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(disableUIUpdates, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(enableDirtyRequests, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(enableUIUpdates, QVector<QRect> (Image::*)());
    ASSERT_IMAGE_SIGNATURE(hasUpdatesRunning, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(initialRefreshGraph, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(notifyBatchUpdateEnded, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(notifyBatchUpdateStarted, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(notifyUIUpdateCompleted, void (Image::*)(const QRect &));
    ASSERT_IMAGE_SIGNATURE(refreshGraphAsync, void (Image::*)(KisNodeSP, const QVector<QRect> &, const QRect &, KisProjectionUpdateFlags));
    ASSERT_IMAGE_SIGNATURE(removeProjectionUpdatesFilter, KisProjectionUpdatesFilterSP (Image::*)(KisProjectionUpdatesFilterCookie));
    ASSERT_IMAGE_SIGNATURE(unblockUpdates, void (Image::*)());
}

void KisImageSchemaContractTest::geometryResolutionAndCoordinateSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(bounds, QRect (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(documentToImagePixelFloored, QPoint (Image::*)(const QPointF &) const);
    ASSERT_IMAGE_SIGNATURE(documentToPixel, QPointF (Image::*)(const QPointF &) const);
    ASSERT_IMAGE_SIGNATURE(documentToPixel, QRectF (Image::*)(const QRectF &) const);
    ASSERT_IMAGE_SIGNATURE(effectiveLodBounds, QRect (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(height, qint32 (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(pixelToDocument, QPointF (Image::*)(const QPoint &) const);
    ASSERT_IMAGE_SIGNATURE(pixelToDocument, QPointF (Image::*)(const QPointF &) const);
    ASSERT_IMAGE_SIGNATURE(pixelToDocument, QRectF (Image::*)(const QRectF &) const);
    ASSERT_IMAGE_SIGNATURE(setResolution, void (Image::*)(double, double));
    ASSERT_IMAGE_SIGNATURE(size, QSize (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(width, qint32 (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(xRes, double (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(yRes, double (Image::*)() const);
}

void KisImageSchemaContractTest::imageAndNodeTransformSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(cropImage, void (Image::*)(const QRect &));
    ASSERT_IMAGE_SIGNATURE(cropNode, void (Image::*)(KisNodeSP, const QRect &, const bool));
    ASSERT_IMAGE_SIGNATURE(cropNodes, void (Image::*)(KisNodeList, const QRect &, const bool));
    ASSERT_IMAGE_SIGNATURE(flatten, void (Image::*)(KisNodeSP));
    ASSERT_IMAGE_SIGNATURE(flattenLayer, void (Image::*)(KisLayerSP));
    ASSERT_IMAGE_SIGNATURE(mergeDown, void (Image::*)(KisLayerSP, const KisMetaData::MergeStrategy *));
    ASSERT_IMAGE_SIGNATURE(mergeMultipleLayers, void (Image::*)(QList<KisNodeSP>, KisNodeSP));
    ASSERT_IMAGE_SIGNATURE(purgeUnusedData, void (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(resizeImage, void (Image::*)(const QRect &));
    ASSERT_IMAGE_SIGNATURE(rotateImage, void (Image::*)(double));
    ASSERT_IMAGE_SIGNATURE(rotateNode, void (Image::*)(KisNodeSP, double, KisSelectionSP));
    ASSERT_IMAGE_SIGNATURE(rotateNodes, void (Image::*)(KisNodeList, double, KisSelectionSP));
    ASSERT_IMAGE_SIGNATURE(scaleImage, void (Image::*)(const QSize &, qreal, qreal, KisFilterStrategy *));
    ASSERT_IMAGE_SIGNATURE(scaleNode, void (Image::*)(KisNodeSP, const QPointF &, qreal, qreal, KisFilterStrategy *, KisSelectionSP));
    ASSERT_IMAGE_SIGNATURE(scaleNodes, void (Image::*)(KisNodeList, const QPointF &, qreal, qreal, KisFilterStrategy *, KisSelectionSP));
    ASSERT_IMAGE_SIGNATURE(shear, void (Image::*)(double, double));
    ASSERT_IMAGE_SIGNATURE(shearNode, void (Image::*)(KisNodeSP, double, double, KisSelectionSP));
    ASSERT_IMAGE_SIGNATURE(shearNodes, void (Image::*)(KisNodeList, double, double, KisSelectionSP));
}

void KisImageSchemaContractTest::colorProfileAndProofingSignaturesRemainStable()
{
    using Image = KisImage;
    using Intent = KoColorConversionTransformation::Intent;
    using Flags = KoColorConversionTransformation::ConversionFlags;
    ASSERT_IMAGE_SIGNATURE(assignImageProfile, bool (Image::*)(const KoColorProfile *, bool));
    ASSERT_IMAGE_SIGNATURE(assignLayerProfile, bool (Image::*)(KisNodeSP, const KoColorProfile *));
    ASSERT_IMAGE_SIGNATURE(colorSpace, const KoColorSpace *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(convertImageColorSpace, void (Image::*)(const KoColorSpace *, Intent, Flags));
    ASSERT_IMAGE_SIGNATURE(convertImageProjectionColorSpace, void (Image::*)(const KoColorSpace *));
    ASSERT_IMAGE_SIGNATURE(convertLayerColorSpace, void (Image::*)(KisNodeSP, const KoColorSpace *, Intent, Flags));
    ASSERT_IMAGE_SIGNATURE(convertToQImage, QImage (Image::*)(QRect, const KoColorProfile *));
    ASSERT_IMAGE_SIGNATURE(convertToQImage, QImage (Image::*)(const QSize &, const KoColorProfile *));
    ASSERT_IMAGE_SIGNATURE(convertToQImage, QImage (Image::*)(qint32, qint32, qint32, qint32, const KoColorProfile *));
    ASSERT_IMAGE_SIGNATURE(defaultProjectionColor, KoColor (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(profile, const KoColorProfile *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(proofingConfiguration, KisProofingConfigurationSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(setDefaultProjectionColor, void (Image::*)(const KoColor &));
    ASSERT_IMAGE_SIGNATURE(setProofingConfiguration, void (Image::*)(KisProofingConfigurationSP));
    ASSERT_IMAGE_SIGNATURE(unifyLayersColorSpace, void (Image::*)());
}

void KisImageSchemaContractTest::layerHierarchyAndSelectionSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(allowMasksOnRootNode, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(canReselectGlobalSelection, bool (Image::*)());
    ASSERT_IMAGE_SIGNATURE(globalSelection, KisSelectionSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(globalSelectionManagementInterface, KisImageGlobalSelectionManagementInterface *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(hasOverlaySelectionMask, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(nChildLayers, qint32 (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(nHiddenLayers, qint32 (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(nextLayerName, QString (Image::*)(const QString &) const);
    ASSERT_IMAGE_SIGNATURE(nlayers, qint32 (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(notifyLayersChanged, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(overlaySelectionMask, KisSelectionMaskSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(projection, KisPaintDeviceSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(rootLayer, KisGroupLayerSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(setAllowMasksOnRootNode, void (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(setOverlaySelectionMask, void (Image::*)(KisSelectionMaskSP));
    ASSERT_IMAGE_SIGNATURE(setRootLayer, void (Image::*)(KisGroupLayerSP));
}

void KisImageSchemaContractTest::annotationAndCompositionSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(addAnnotation, void (Image::*)(KisAnnotationSP));
    ASSERT_IMAGE_SIGNATURE(addComposition, void (Image::*)(KisLayerCompositionSP));
    ASSERT_IMAGE_SIGNATURE(annotation, KisAnnotationSP (Image::*)(const QString &));
    ASSERT_IMAGE_SIGNATURE(beginAnnotations, vKisAnnotationSP_it (Image::*)());
    ASSERT_IMAGE_SIGNATURE(compositions, QList<KisLayerCompositionSP> (Image::*)());
    ASSERT_IMAGE_SIGNATURE(endAnnotations, vKisAnnotationSP_it (Image::*)());
    ASSERT_IMAGE_SIGNATURE(moveCompositionDown, void (Image::*)(KisLayerCompositionSP));
    ASSERT_IMAGE_SIGNATURE(moveCompositionUp, void (Image::*)(KisLayerCompositionSP));
    ASSERT_IMAGE_SIGNATURE(removeAnnotation, void (Image::*)(const QString &));
    ASSERT_IMAGE_SIGNATURE(removeComposition, void (Image::*)(KisLayerCompositionSP));
}

void KisImageSchemaContractTest::wraparoundLodAndIsolationSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(animationInterface, KisImageAnimationInterface *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(currentLevelOfDetail, int (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(explicitRegenerateLevelOfDetail, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(isIsolatingGroup, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(isIsolatingLayer, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(isolationRootNode, KisNodeSP (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(lodPreferences, KisLodPreferences (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(mirrorAxesCenter, QPointF (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(setLodPreferences, void (Image::*)(const KisLodPreferences &));
    ASSERT_IMAGE_SIGNATURE(setMirrorAxesCenter, void (Image::*)(const QPointF &) const);
    ASSERT_IMAGE_SIGNATURE(setWrapAroundModeAxis, void (Image::*)(WrapAroundAxis));
    ASSERT_IMAGE_SIGNATURE(setWrapAroundModePermitted, void (Image::*)(bool));
    ASSERT_IMAGE_SIGNATURE(startIsolatedMode, bool (Image::*)(KisNodeSP, bool, bool));
    ASSERT_IMAGE_SIGNATURE(stopIsolatedMode, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(wrapAroundModeActive, bool (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(wrapAroundModeAxis, WrapAroundAxis (Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(wrapAroundModePermitted, bool (Image::*)() const);
}

void KisImageSchemaContractTest::undoAdapterAndModificationSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(compositeProgressProxy, KisCompositeProgressProxy *(Image::*)());
    ASSERT_IMAGE_SIGNATURE(lastExecutedCommand, const KUndo2Command *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(notifyAboutToBeDeleted, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(postExecutionUndoAdapter, KisPostExecutionUndoAdapter *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(setModifiedWithoutUndo, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(setUndoStore, void (Image::*)(KisUndoStore *));
    ASSERT_IMAGE_SIGNATURE(signalRouter, KisImageSignalRouter *(Image::*)());
    ASSERT_IMAGE_SIGNATURE(undoAdapter, KisUndoAdapter *(Image::*)() const);
    ASSERT_IMAGE_SIGNATURE(undoStore, KisUndoStore *(Image::*)());
}

void KisImageSchemaContractTest::imageAndLifecycleSignalSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(sigAboutToBeDeleted, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigColorSpaceChanged, void (Image::*)(const KoColorSpace *));
    ASSERT_IMAGE_SIGNATURE(sigImageModified, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigImageModifiedWithoutUndo, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigImageUpdated, void (Image::*)(const QRect &));
    ASSERT_IMAGE_SIGNATURE(sigProfileChanged, void (Image::*)(const KoColorProfile *));
    ASSERT_IMAGE_SIGNATURE(sigProofingConfigChanged, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigResolutionChanged, void (Image::*)(double, double));
    ASSERT_IMAGE_SIGNATURE(sigSizeChanged, void (Image::*)(const QPointF &, const QPointF &));
}

void KisImageSchemaContractTest::nodeAndIsolationSignalSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(sigInternalStopIsolatedModeRequested, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigIsolatedModeChanged, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigLayersChangedAsync, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigNodeAddedAsync, void (Image::*)(KisNodeSP, KisNodeAdditionFlags));
    ASSERT_IMAGE_SIGNATURE(sigNodeChanged, void (Image::*)(KisNodeSP));
    ASSERT_IMAGE_SIGNATURE(sigNodeCollapsedChanged, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigRemoveNodeAsync, void (Image::*)(KisNodeSP));
    ASSERT_IMAGE_SIGNATURE(sigRequestNodeReselection, void (Image::*)(KisNodeSP, const KisNodeList &));
}

void KisImageSchemaContractTest::strokeRequestSignalSignaturesRemainStable()
{
    using Image = KisImage;
    ASSERT_IMAGE_SIGNATURE(sigRedoDuringStrokeRequested, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigStrokeCancellationRequested, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigStrokeEndRequested, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigStrokeEndRequestedActiveNodeFiltered, void (Image::*)());
    ASSERT_IMAGE_SIGNATURE(sigUndoDuringStrokeRequested, void (Image::*)());
}
// clang-format on

QTEST_APPLESS_MAIN(KisImageSchemaContractTest)

#include "KisImageSchemaContractTest.moc"
