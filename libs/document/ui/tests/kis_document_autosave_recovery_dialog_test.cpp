/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <recovery/KisAutoSaveRecoveryDialog.h>

class KisDocumentAutoSaveRecoveryDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsRecoverableFilesSelectedUntilTheyAreDiscarded();
};

void KisDocumentAutoSaveRecoveryDialogTest::keepsRecoverableFilesSelectedUntilTheyAreDiscarded()
{
    const QList<Krita::Document::KisDocumentAutoSaveFile> files {
        {QStringLiteral("first-autosave.kra"), {}, {}, {}},
        {QStringLiteral("second-autosave.kra"), {}, {}, {}},
    };
    KisAutoSaveRecoveryDialog dialog(files);

    QCOMPARE(dialog.recoverableFiles(),
             QStringList({QStringLiteral("first-autosave.kra"),
                          QStringLiteral("second-autosave.kra")}));

    dialog.slotDeleteAll();

    QVERIFY(dialog.recoverableFiles().isEmpty());
}

QTEST_MAIN(KisDocumentAutoSaveRecoveryDialogTest)

#include "kis_document_autosave_recovery_dialog_test.moc"
