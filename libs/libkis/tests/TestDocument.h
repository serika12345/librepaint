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
    void testCanvasBackgroundReferencePixels_data();
    void testCanvasBackgroundReferencePixels();
    void testGpuDocumentPreviewsTransferOnlyRequestedPixels_data();
    void testGpuDocumentPreviewsTransferOnlyRequestedPixels();
    void testSetColorSpace();
    void testSetColorProfile();
    void testPixelData();
    void testGpuImagePixelDataModifiedStateAndRetainedNode();
    void testGpuSaveAndExportPreserveHistoryAndFileIdentity();
    void testGpuPngSaveAsPreservesHistoryAndAdoptsTheSavedFile();
    void testGpuPngExportPreservesPixelsAndDocument_data();
    void testGpuPngExportPreservesPixelsAndDocument();
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

