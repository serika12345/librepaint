/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Boudewijn Rempt

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef TESTDOCUMENT_H
#define TESTDOCUMENT_H

#include <QObject>

class TestDocument : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testColorHistoryReplacementAndCopyKeepNewestFirst_data();
    void testColorHistoryReplacementAndCopyKeepNewestFirst();
    void testNativeSavingKeepsColorHistoryOrderAndSelectedColor_data();
    void testNativeSavingKeepsColorHistoryOrderAndSelectedColor();
    void testExportImageWaitsForImageProcessingAndPreservesSource();
    void testCanvasResizePreservesPixelsHistoryAndSaving_data();
    void testCanvasResizePreservesPixelsHistoryAndSaving();
    void testGpuCanvasResizePreservesRejectedInputsAndNotificationLifetimes();
    void testOrdinaryCreationKeepsPngExportAndReopen();
    void testOrdinaryKraKeepsLayerHierarchyThroughResaving();
    void testGpuHierarchyKeepsPublicEditingHistoryAndNativeFiles_data();
    void testGpuHierarchyKeepsPublicEditingHistoryAndNativeFiles();
    void testBackgroundApiKeepsHistoryAndPinnedPixels();
    void testBackgroundNotificationLifetimes_data();
    void testBackgroundNotificationLifetimes();
    void testOrdinaryCreationPreservesInitialDocumentState_data();
    void testOrdinaryCreationPreservesInitialDocumentState();
    void testGpuSessionNativeImportRetainsFormatDataAndRejectsUnsupportedEdits();
    void testGpuSessionOpensExistingStaticNativeDocuments_data();
    void testGpuSessionOpensExistingStaticNativeDocuments();
    void testCanvasBackgroundReferencePixels_data();
    void testCanvasBackgroundReferencePixels();
    void testGpuDocumentPreviewsTransferOnlyRequestedPixels_data();
    void testGpuDocumentPreviewsTransferOnlyRequestedPixels();
    void testSetColorSpace();
    void testSetColorProfile();
    void testPixelData();
    void testGpuImagePixelDataModifiedStateAndRetainedNode();
    void testGpuSaveAndExportPreserveHistoryAndFileIdentity();
    void testSavedPngDensityUsesDocumentResolution_data();
    void testSavedPngDensityUsesDocumentResolution();
    void testPngWithoutPhysicalResolutionKeepsImageDefault_data();
    void testPngWithoutPhysicalResolutionKeepsImageDefault();
    void testGpuSessionOpensPngWithGpuPixelOwnership_data();
    void testGpuSessionOpensPngWithGpuPixelOwnership();
    void testGpuSessionOpensJpegWithGpuPixelOwnership_data();
    void testGpuSessionOpensJpegWithGpuPixelOwnership();
    void testJpegRejectedOpenKeepsDocument_data();
    void testJpegRejectedOpenKeepsDocument();
    void testGpuPngRejectedOpenKeepsDocument_data();
    void testGpuPngRejectedOpenKeepsDocument();
    void testSavedSourceOverPixelsAgreeAcrossBackends_data();
    void testSavedSourceOverPixelsAgreeAcrossBackends();
    void testSavedKraEmbeddedPngsAgreeAcrossBackends_data();
    void testSavedKraEmbeddedPngsAgreeAcrossBackends();
    void testSavedKraPreviewKeepsOtherColorSpaces_data();
    void testSavedKraPreviewKeepsOtherColorSpaces();
    void testGpuPngSaveAsPreservesHistoryAndAdoptsTheSavedFile();
    void testGpuPngExportPreservesPixelsAndDocument_data();
    void testGpuPngExportPreservesPixelsAndDocument();
    void testGpuJpegExportPreservesOptionsAndDocument_data();
    void testGpuJpegExportPreservesOptionsAndDocument();
    void testGpuJpegExportDialogLifetimes_data();
    void testGpuJpegExportDialogLifetimes();
    void testGpuJpegExportStreamsLargeFixedDocuments_data();
    void testGpuJpegExportStreamsLargeFixedDocuments();
    void testGpuPngExportDialogLifetimes_data();
    void testGpuPngExportDialogLifetimes();
    void testGpuExportRowsRespectMemoryBudgetAndRetainTheirVersion();
    void testGpuExportPreparationWaitsForAnOccupiedQueue();
    void testGpuExportRowsCancelPendingWorkerReads();
    void testGpuPngExportRejectsExcessiveWorkingMemoryBeforeChangingFiles();
    void testGpuPngExportStreamsLargeFixedDocuments();
    void testGpuPngExportRequiresAtomicReplacementForExistingFiles();
    void testGpuSaveNotificationLifetimes_data();
    void testGpuSaveNotificationLifetimes();
    void testGpuSnapshotKeepsIndependentPixelsAndDocumentMetadata();
    void testGpuDocumentCopyKeepsLayerImageMetadata_data();
    void testGpuDocumentCopyKeepsLayerImageMetadata();
    void testGpuRasterExportKeepsLayerImageMetadata_data();
    void testGpuRasterExportKeepsLayerImageMetadata();
    void testNativeLayerImageMetadataSurvivesEditing_data();
    void testNativeLayerImageMetadataSurvivesEditing();
    void testNativeLayerRemovalDoesNotReassignMetadata_data();
    void testNativeLayerRemovalDoesNotReassignMetadata();
    void testNativeMalformedLayerMetadataKeepsWarningAndOtherValues_data();
    void testNativeMalformedLayerMetadataKeepsWarningAndOtherValues();
    void testGpuNativeAutoSavePinsMetadataAndRecoveryKeepsLatest();
    void testThumbnail();
    void testCreateFillLayer();
    void testCreateCloneLayer();
    void testCreateTransparencyMask();
    void testCreateColorizeMask();
    void testAnnotations();
    void testNodeByName();
    void testNodeByUniqueId();
};

#endif

