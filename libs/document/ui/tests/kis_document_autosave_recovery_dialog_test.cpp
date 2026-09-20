/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <recovery/KisAutoSaveRecoveryDialog.h>

#include <QCheckBox>

class KisDocumentAutoSaveRecoveryDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsRecoverableFilesSelectedUntilTheyAreDiscarded();
    void togglesRecoverableFilesFromVisibleItems();
};

void KisDocumentAutoSaveRecoveryDialogTest::keepsRecoverableFilesSelectedUntilTheyAreDiscarded()
{
    const QList<Krita::Document::KisDocumentAutoSaveFile> files{
        {QStringLiteral("first-autosave.kra"), {}, {}, {}},
        {QStringLiteral("second-autosave.kra"), {}, {}, {}},
    };
    KisAutoSaveRecoveryDialog dialog(files);

    QCOMPARE(dialog.recoverableFiles(),
             QStringList({QStringLiteral("first-autosave.kra"), QStringLiteral("second-autosave.kra")}));

    dialog.slotDeleteAll();

    QVERIFY(dialog.recoverableFiles().isEmpty());
}

void KisDocumentAutoSaveRecoveryDialogTest::togglesRecoverableFilesFromVisibleItems()
{
    const QList<Krita::Document::KisDocumentAutoSaveFile> files{
        {QStringLiteral("first-autosave.kra"), {}, {}, {}},
        {QStringLiteral("second-autosave.kra"), {}, {}, {}},
    };
    KisAutoSaveRecoveryDialog dialog(files);
    dialog.show();

    QTRY_COMPARE_WITH_TIMEOUT(dialog.findChildren<QCheckBox *>().size(), 2, 1000);
    const QList<QCheckBox *> checkBoxes = dialog.findChildren<QCheckBox *>();
    for (QCheckBox *checkBox : checkBoxes) {
        QVERIFY(checkBox->isChecked());
        checkBox->setChecked(false);
    }

    QVERIFY(dialog.recoverableFiles().isEmpty());
}

QTEST_MAIN(KisDocumentAutoSaveRecoveryDialogTest)

#include "kis_document_autosave_recovery_dialog_test.moc"
