/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <info/KoDocumentInfoDlg.h>
#include <io/kis_document_io_presentation.h>
#include <recovery/KisAutoSaveRecoveryDialog.h>
#include <recovery/KisRecoverNamedAutosaveDialog.h>
#include <undo/kundo2view.h>

#include <QTest>

#include <type_traits>

class KisDocumentUiLifecycleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void undoViewLifetimeSchemaRemainsStable();
    void autoSaveRecoveryLifetimeSchemaRemainsStable();
    void ioPresentationNotificationSchemaRemainsStable();
    void namedAutoSaveRecoveryLifetimeSchemaRemainsStable();
    void documentInfoLifetimeAndNotificationSchemaRemainStable();
};

void KisDocumentUiLifecycleSchemaContractTest::undoViewLifetimeSchemaRemainsStable()
{
    static_assert(std::has_virtual_destructor_v<KUndo2View>);
}

void KisDocumentUiLifecycleSchemaContractTest::autoSaveRecoveryLifetimeSchemaRemainsStable()
{
    static_assert(std::has_virtual_destructor_v<KisAutoSaveRecoveryDialog>);
}

void KisDocumentUiLifecycleSchemaContractTest::ioPresentationNotificationSchemaRemainsStable()
{
    using StatusBarMessageSignature = void (KisDocumentIoPresentation::*)(const QString &, int);

    static_assert(
        std::is_same_v<decltype(static_cast<StatusBarMessageSignature>(&KisDocumentIoPresentation::statusBarMessage)),
                       StatusBarMessageSignature>);
}

void KisDocumentUiLifecycleSchemaContractTest::namedAutoSaveRecoveryLifetimeSchemaRemainsStable()
{
    static_assert(std::has_virtual_destructor_v<KisRecoverNamedAutosaveDialog>);
}

void KisDocumentUiLifecycleSchemaContractTest::documentInfoLifetimeAndNotificationSchemaRemainStable()
{
    using SaveRequestedSignature = void (KoDocumentInfoDlg::*)();

    static_assert(std::has_virtual_destructor_v<KoDocumentInfoDlg>);
    static_assert(std::is_same_v<decltype(static_cast<SaveRequestedSignature>(&KoDocumentInfoDlg::saveRequested)),
                                 SaveRequestedSignature>);
}

QTEST_GUILESS_MAIN(KisDocumentUiLifecycleSchemaContractTest)

#include "KisDocumentUiLifecycleSchemaContractTest.moc"
