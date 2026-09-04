/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Document.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_DOCUMENT_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Document::method)), signature>)

} // namespace

class DocumentLifecycleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentIdentificationAndMetadataSignaturesRemainStable();
    void documentExecutionAndModificationStateSignaturesRemainStable();
    void documentSaveAndExportSignaturesRemainStable();
    void documentCloseAndAutosaveSignaturesRemainStable();
    void documentProcessingSynchronizationSignaturesRemainStable();
};

void DocumentLifecycleSchemaContractTest::documentIdentificationAndMetadataSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(fileName, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setFileName, void (Document::*)(QString));
    ASSERT_DOCUMENT_SIGNATURE(name, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setName, void (Document::*)(QString));
    ASSERT_DOCUMENT_SIGNATURE(documentInfo, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setDocumentInfo, void (Document::*)(const QString &));
}

void DocumentLifecycleSchemaContractTest::documentExecutionAndModificationStateSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(batchmode, bool (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setBatchmode, void (Document::*)(bool));
    ASSERT_DOCUMENT_SIGNATURE(modified, bool (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setModified, void (Document::*)(bool));
}

void DocumentLifecycleSchemaContractTest::documentSaveAndExportSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(save, bool (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(saveAs, bool (Document::*)(const QString &));
    ASSERT_DOCUMENT_SIGNATURE(exportImage, bool (Document::*)(const QString &, const InfoObject &));
}

void DocumentLifecycleSchemaContractTest::documentCloseAndAutosaveSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(close, bool (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(setAutosave, void (Document::*)(bool));
    ASSERT_DOCUMENT_SIGNATURE(autosave, bool (Document::*)());
}

void DocumentLifecycleSchemaContractTest::documentProcessingSynchronizationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(lock, void (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(unlock, void (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(waitForDone, void (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(tryBarrierLock, bool (Document::*)());
}

#undef ASSERT_DOCUMENT_SIGNATURE

QTEST_APPLESS_MAIN(DocumentLifecycleSchemaContractTest)

#include "DocumentLifecycleSchemaContractTest.moc"
