/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "document/KisDocument.h"

#include <QTest>

#include <type_traits>

#define ASSERT_DOCUMENT_CALLABLE(method)                                                                               \
    static_assert(std::is_pointer_v<decltype(&KisDocument::method)>                                                    \
                  || std::is_member_function_pointer_v<decltype(&KisDocument::method)>)

#define ASSERT_DOCUMENT_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDocument::method)), signature>)

class KisDocumentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeFlagsRecoveryAndLifetimeSchemaRemainStable();
    void storageMimeAndDiagnosticSignaturesRemainStable();
    void importExportAndSaveSignaturesRemainStable();
    void autosaveAndStateSignaturesRemainStable();
    void documentInfoPathAndUnitSignaturesRemainStable();
    void resourceStoryboardAndAudioSignaturesRemainStable();
    void imageLifetimeAndStateSignaturesRemainStable();
    void imageProgressAndAnimationSignaturesRemainStable();
    void shapeNodeAssistantAndReferenceSignaturesRemainStable();
    void cloneSnapshotAndUndoSignaturesRemainStable();
    void presentationConfigurationAndColorHistorySignaturesRemainStable();
    void generalNotificationSignaturesRemainStable();
    void savingNotificationSignaturesRemainStable();
    void resourceAndPresentationNotificationSignaturesRemainStable();
    void storyboardAudioAndCompletionNotificationSignaturesRemainStable();
};

// clang-format off
void KisDocumentSchemaContractTest::typeFlagsRecoveryAndLifetimeSchemaRemainStable()
{
    using Document = KisDocument;
    static_assert(std::is_class_v<Document>);
    static_assert(std::is_same_v<Document::OpenFlags, QFlags<Document::OpenFlag>>);
    static_assert(std::is_enum_v<Document::OpenFlag>);
    static_assert(Document::None == 0);
    static_assert(Document::DontAddToRecent == 0x1);
    static_assert(Document::RecoveryFile == 0x2);
    static_assert(std::is_enum_v<Document::RecoveryAutoSaveStartResult>);
    static_assert(static_cast<int>(Document::RecoveryAutoSaveStartResult::NoChanges) == 0);
    static_assert(static_cast<int>(Document::RecoveryAutoSaveStartResult::Started) == 1);
    static_assert(static_cast<int>(Document::RecoveryAutoSaveStartResult::AlreadySaving) == 2);
    static_assert(static_cast<int>(Document::RecoveryAutoSaveStartResult::Failed) == 3);
    static_assert(std::has_virtual_destructor_v<Document>);
}

void KisDocumentSchemaContractTest::storageMimeAndDiagnosticSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(embeddedResourcesStorageId);
    ASSERT_DOCUMENT_CALLABLE(linkedResourcesStorageId);
    ASSERT_DOCUMENT_CALLABLE(nativeFormatMimeType);
    ASSERT_DOCUMENT_CALLABLE(extraNativeMimeTypes);
    ASSERT_DOCUMENT_CALLABLE(isNativeFormat);
    ASSERT_DOCUMENT_CALLABLE(mimeType);
    ASSERT_DOCUMENT_CALLABLE(setMimeType);
    ASSERT_DOCUMENT_CALLABLE(fileBatchMode);
    ASSERT_DOCUMENT_CALLABLE(setFileBatchMode);
    ASSERT_DOCUMENT_CALLABLE(errorMessage);
    ASSERT_DOCUMENT_CALLABLE(setErrorMessage);
    ASSERT_DOCUMENT_CALLABLE(warningMessage);
    ASSERT_DOCUMENT_CALLABLE(setWarningMessage);
}

void KisDocumentSchemaContractTest::importExportAndSaveSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(openPath);
    ASSERT_DOCUMENT_CALLABLE(importDocument);
    ASSERT_DOCUMENT_CALLABLE(exportDocument);
    ASSERT_DOCUMENT_CALLABLE(exportDocumentSync);
    ASSERT_DOCUMENT_CALLABLE(loadNativeFormat);
    ASSERT_DOCUMENT_CALLABLE(closePath);
    ASSERT_DOCUMENT_CALLABLE(saveAs);
    ASSERT_DOCUMENT_CALLABLE(save);
    ASSERT_DOCUMENT_CALLABLE(importExportManager);
    ASSERT_DOCUMENT_CALLABLE(serializeToNativeByteArray);
}

void KisDocumentSchemaContractTest::autosaveAndStateSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(setAutoSaveActive);
    ASSERT_DOCUMENT_CALLABLE(isAutoSaveActive);
    ASSERT_DOCUMENT_CALLABLE(setNormalAutoSaveInterval);
    ASSERT_DOCUMENT_CALLABLE(setEmergencyAutoSaveInterval);
    ASSERT_DOCUMENT_CALLABLE(setInfiniteAutoSaveInterval);
    ASSERT_DOCUMENT_CALLABLE(isAutosaving);
    ASSERT_DOCUMENT_CALLABLE(isInSaving);
    ASSERT_DOCUMENT_CALLABLE(isSaving);
    ASSERT_DOCUMENT_CALLABLE(waitForSavingToComplete);
    ASSERT_DOCUMENT_CALLABLE(autoSaveOnPause);
    ASSERT_DOCUMENT_CALLABLE(requestRecoveryAutoSave);
    ASSERT_DOCUMENT_CALLABLE(setModified);
    ASSERT_DOCUMENT_CALLABLE(isModified);
    ASSERT_DOCUMENT_CALLABLE(setRecovered);
    ASSERT_DOCUMENT_CALLABLE(isRecovered);
    ASSERT_DOCUMENT_CALLABLE(setEmpty);
    ASSERT_DOCUMENT_CALLABLE(updateEditingTime);
    ASSERT_DOCUMENT_CALLABLE(setReadWrite);
    ASSERT_DOCUMENT_CALLABLE(isReadWrite);
}

void KisDocumentSchemaContractTest::documentInfoPathAndUnitSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(createDocumentInfoDialog);
    ASSERT_DOCUMENT_CALLABLE(documentInfo);
    ASSERT_DOCUMENT_CALLABLE(caption);
    ASSERT_DOCUMENT_CALLABLE(resetPath);
    ASSERT_DOCUMENT_CALLABLE(setMimeTypeAfterLoading);
    ASSERT_DOCUMENT_CALLABLE(unit);
    ASSERT_DOCUMENT_CALLABLE(setUnit);
    ASSERT_DOCUMENT_CALLABLE(localFilePath);
    ASSERT_DOCUMENT_CALLABLE(setLocalFilePath);
    ASSERT_DOCUMENT_CALLABLE(path);
    ASSERT_DOCUMENT_CALLABLE(setPath);
    ASSERT_DOCUMENT_SIGNATURE(createDomDocument, QDomDocument (KisDocument::*)(const QString &, const QString &) const);
    ASSERT_DOCUMENT_SIGNATURE(createDomDocument, QDomDocument (*)(const QString &, const QString &, const QString &));
}

void KisDocumentSchemaContractTest::resourceStoryboardAndAudioSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(linkedDocumentResources);
    ASSERT_DOCUMENT_CALLABLE(setPaletteList);
    ASSERT_DOCUMENT_CALLABLE(getStoryboardItemList);
    ASSERT_DOCUMENT_CALLABLE(setStoryboardItemList);
    ASSERT_DOCUMENT_CALLABLE(getStoryboardCommentsList);
    ASSERT_DOCUMENT_CALLABLE(setStoryboardCommentList);
    ASSERT_DOCUMENT_CALLABLE(getAudioTracks);
    ASSERT_DOCUMENT_CALLABLE(setAudioTracks);
    ASSERT_DOCUMENT_CALLABLE(setAudioVolume);
    ASSERT_DOCUMENT_CALLABLE(getAudioLevel);
}

void KisDocumentSchemaContractTest::imageLifetimeAndStateSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(newImage);
    ASSERT_DOCUMENT_CALLABLE(image);
    ASSERT_DOCUMENT_CALLABLE(hasImage);
    ASSERT_DOCUMENT_CALLABLE(imageObjectName);
    ASSERT_DOCUMENT_CALLABLE(imageMemorySize);
    ASSERT_DOCUMENT_CALLABLE(savingImage);
    ASSERT_DOCUMENT_CALLABLE(setCurrentImage);
    ASSERT_DOCUMENT_CALLABLE(hackPreliminarySetImage);
}

void KisDocumentSchemaContractTest::imageProgressAndAnimationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(connectImageMemoryStatisticsUpdates);
    ASSERT_DOCUMENT_CALLABLE(addImageProgressProxy);
    ASSERT_DOCUMENT_CALLABLE(removeImageProgressProxy);
    ASSERT_DOCUMENT_CALLABLE(imageUndoAdapter);
    ASSERT_DOCUMENT_CALLABLE(hasAnimation);
    ASSERT_DOCUMENT_CALLABLE(animationLength);
    ASSERT_DOCUMENT_CALLABLE(setAnimationRange);
    ASSERT_DOCUMENT_CALLABLE(setAnimationTiming);
    ASSERT_DOCUMENT_CALLABLE(refreshProjectionAndWait);
    ASSERT_DOCUMENT_CALLABLE(translateTemplateRootLayerName);
}

void KisDocumentSchemaContractTest::shapeNodeAssistantAndReferenceSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(shapeController);
    ASSERT_DOCUMENT_CALLABLE(shapeForNode);
    ASSERT_DOCUMENT_CALLABLE(setPreActivatedNode);
    ASSERT_DOCUMENT_CALLABLE(preActivatedNode);
    ASSERT_DOCUMENT_CALLABLE(assistants);
    ASSERT_DOCUMENT_CALLABLE(setAssistants);
    ASSERT_DOCUMENT_CALLABLE(setAssistantsGlobalColor);
    ASSERT_DOCUMENT_CALLABLE(assistantsGlobalColor);
    ASSERT_DOCUMENT_CALLABLE(referenceImagesLayer);
    ASSERT_DOCUMENT_CALLABLE(setReferenceImagesLayer);
}

void KisDocumentSchemaContractTest::cloneSnapshotAndUndoSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(clone);
    ASSERT_DOCUMENT_CALLABLE(lockAndCloneForSaving);
    ASSERT_DOCUMENT_CALLABLE(lockAndCreateSnapshot);
    ASSERT_DOCUMENT_CALLABLE(cloneWithImageReadLock);
    ASSERT_DOCUMENT_CALLABLE(copyFromDocument);
    ASSERT_DOCUMENT_CALLABLE(createUndoStore);
    ASSERT_DOCUMENT_CALLABLE(clearUndoHistory);
    ASSERT_DOCUMENT_CALLABLE(undoStack);
}

void KisDocumentSchemaContractTest::presentationConfigurationAndColorHistorySignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(generatePreview);
    ASSERT_DOCUMENT_CALLABLE(documentBounds);
    ASSERT_DOCUMENT_CALLABLE(gridConfig);
    ASSERT_DOCUMENT_CALLABLE(setGridConfig);
    ASSERT_DOCUMENT_CALLABLE(guidesConfig);
    ASSERT_DOCUMENT_CALLABLE(setGuidesConfig);
    ASSERT_DOCUMENT_CALLABLE(mirrorAxisConfig);
    ASSERT_DOCUMENT_CALLABLE(setMirrorAxisConfig);
    ASSERT_DOCUMENT_CALLABLE(setColorHistoryColors);
    ASSERT_DOCUMENT_CALLABLE(colorHistoryColors);
    ASSERT_DOCUMENT_CALLABLE(colorHistoryModel);
}

void KisDocumentSchemaContractTest::generalNotificationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(unitChanged);
    ASSERT_DOCUMENT_CALLABLE(statusBarMessage);
    ASSERT_DOCUMENT_CALLABLE(clearStatusBarMessage);
    ASSERT_DOCUMENT_CALLABLE(modified);
    ASSERT_DOCUMENT_CALLABLE(sigReadWriteChanged);
    ASSERT_DOCUMENT_CALLABLE(sigRecoveredChanged);
    ASSERT_DOCUMENT_CALLABLE(sigPathChanged);
    ASSERT_DOCUMENT_CALLABLE(sigLoadingFinished);
}

void KisDocumentSchemaContractTest::savingNotificationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(sigSavingFinished);
    ASSERT_DOCUMENT_CALLABLE(sigBackgroundSavingFinished);
    ASSERT_DOCUMENT_CALLABLE(sigCompleteBackgroundSaving);
    ASSERT_DOCUMENT_CALLABLE(sigRecoveryAutoSaveFinished);
}

void KisDocumentSchemaContractTest::resourceAndPresentationNotificationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(sigGuidesConfigChanged);
    ASSERT_DOCUMENT_CALLABLE(sigMirrorAxisConfigChanged);
    ASSERT_DOCUMENT_CALLABLE(sigGridConfigChanged);
    ASSERT_DOCUMENT_CALLABLE(sigReferenceImagesChanged);
    ASSERT_DOCUMENT_CALLABLE(sigReferenceImagesLayerChanged);
    ASSERT_DOCUMENT_CALLABLE(sigPaletteListChanged);
    ASSERT_DOCUMENT_CALLABLE(sigAssistantsChanged);
}

void KisDocumentSchemaContractTest::storyboardAudioAndCompletionNotificationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_CALLABLE(sigStoryboardItemListChanged);
    ASSERT_DOCUMENT_CALLABLE(sigStoryboardCommentListChanged);
    ASSERT_DOCUMENT_CALLABLE(sigAudioTracksChanged);
    ASSERT_DOCUMENT_CALLABLE(sigAudioLevelChanged);
    ASSERT_DOCUMENT_CALLABLE(completed);
    ASSERT_DOCUMENT_CALLABLE(canceled);
}
// clang-format on

#undef ASSERT_DOCUMENT_SIGNATURE
#undef ASSERT_DOCUMENT_CALLABLE

QTEST_APPLESS_MAIN(KisDocumentSchemaContractTest)

#include "KisDocumentSchemaContractTest.moc"
