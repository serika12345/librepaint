/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Boudewijn Rempt

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef TESTNODE_H
#define TESTNODE_H

#include <QObject>

class TestNode : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testClipboardLayersSurviveSourceEditsAndClosure_data();
    void testClipboardLayersSurviveSourceEditsAndClosure();
    void testClipboardLayersProvideImageAndNativeData_data();
    void testClipboardLayersProvideImageAndNativeData();
    void testGpuClipboardRefusalKeepsCapturedLayers();
    void testGpuCutWaitsForClipboardCapture();
    void testMergedClipboardPreservesProjectionAndSource_data();
    void testMergedClipboardPreservesProjectionAndSource();
    void testMergedClipboardSurvivesImageReplacement_data();
    void testMergedClipboardSurvivesImageReplacement();
    void testNormalPasteRetainsMultipleClipboardRoots_data();
    void testNormalPasteRetainsMultipleClipboardRoots();
    void testPasteNewKeepsClipboardAndSource_data();
    void testPasteNewKeepsClipboardAndSource();
    void testHierarchyOrderingPreservesPlacementPixelsHistoryAndSaving_data();
    void testHierarchyOrderingPreservesPlacementPixelsHistoryAndSaving();
    void testRootGroupDuplicationKeepsSelectionAndHistory_data();
    void testRootGroupDuplicationKeepsSelectionAndHistory();
    void testHierarchyDuplicationPreservesPlacementPixelsHistoryAndSaving_data();
    void testHierarchyDuplicationPreservesPlacementPixelsHistoryAndSaving();
    void testGroupCreationKeepsIndependentPixelsChildrenAndHistory_data();
    void testGroupCreationKeepsIndependentPixelsChildrenAndHistory();
    void testExportImageCancelsWhenItsSourceChangesBeforeCapture_data();
    void testExportImageCancelsWhenItsSourceChangesBeforeCapture();
    void testLayerCropPreservesPublicPixelsAndHistory_data();
    void testLayerCropPreservesPublicPixelsAndHistory();
    void testGpuLayerCropKeepsUnrelatedDirectWritesAcrossUndo();
    void testLayerCropCanReleaseItsPublicCallerDuringNotification();
    void testLayerCropToolPreservesConfirmationResumptionAndCancellation_data();
    void testLayerCropToolPreservesConfirmationResumptionAndCancellation();
    void testImportedLayerPositionSurvivesPublicCopiesSavingAndRecovery_data();
    void testImportedLayerPositionSurvivesPublicCopiesSavingAndRecovery();
    void testAbsoluteLayerMovementPreservesPixelsAndDirectHistory_data();
    void testAbsoluteLayerMovementPreservesPixelsAndDirectHistory();
    void testGpuBoundsUseOnlyScalarReadback_data();
    void testGpuBoundsUseOnlyScalarReadback();
    void testLayerPropertiesPreservePublicHistorySemantics_data();
    void testLayerPropertiesPreservePublicHistorySemantics();
    void testPassThroughGroupsKeepPublicReadsAndPropertyHistory();
    void testLayerPropertiesDialogPreservesPreviewCancellationAndHistory_data();
    void testLayerPropertiesDialogPreservesPreviewCancellationAndHistory();
    void testGpuNodeNotificationKeepsReentrantPropertyWrites();
    void testDirectOpacityWritesPreserveRecordedPropertyUndo_data();
    void testDirectOpacityWritesPreserveRecordedPropertyUndo();
    void testSetColorSpace();
    void testSetColorProfile();
    void testPixelData();
    void testRawPixelWritesPreserveFormatHistoryAndFailures();
    void testRawPixelWritesPreserveOtherLayerHistoryAndRedo();
    void testGpuPixelDataPreservesFormatHistoryAndSharedLifetime();
    void testPixelCopiesAndNodesOutliveTheImageOwner();
    void testDetachedLayerCopiesPreserveIndependentPixels_data();
    void testDetachedLayerCopiesPreserveIndependentPixels();
    void testInsertedLayersPreserveIdentityPixelsAndHistory_data();
    void testInsertedLayersPreserveIdentityPixelsAndHistory();
    void testGpuLayerInsertionPreservesNativeIdentityAndRejectsInvalidInputs();
    void testPaintLayerCreationInGroupsPreservesPlacementAndHistory_data();
    void testPaintLayerCreationInGroupsPreservesPlacementAndHistory();
    void testGpuLayerSelectionKeepsProjectionNotificationsForPaint();
    void testLayerRemovalPreservesPixelsAndHistory_data();
    void testLayerRemovalPreservesPixelsAndHistory();
    void testHistoryNavigationWaitsForImageJobs_data();
    void testHistoryNavigationWaitsForImageJobs();
    void testDeferredHistoryRequestsPreserveOrderAndLifetime_data();
    void testDeferredHistoryRequestsPreserveOrderAndLifetime();
    void testLastLayerRemovalInWindowClearsSelectionAndRestoresHistory_data();
    void testLastLayerRemovalInWindowClearsSelectionAndRestoresHistory();
    void testScriptedFigurePaintsRequestedLayerAndPreservesHistory_data();
    void testScriptedFigurePaintsRequestedLayerAndPreservesHistory();
    void testLayerAndDocumentProjectionsRemainDistinct();
    void testProjectionPixelData();
    void testThumbnail();
    void testGpuBoundsAndThumbnailMatchNativePixels_data();
    void testGpuBoundsAndThumbnailMatchNativePixels();
    void testGpuThumbnailTransfersOnlyRequestedPixels();
    void testMergeDown();
    void testFindChildNodes();
};

#endif

