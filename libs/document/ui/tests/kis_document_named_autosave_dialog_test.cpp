/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QRadioButton>
#include <QTest>

#include <recovery/KisRecoverNamedAutosaveDialog.h>

class KisDocumentNamedAutosaveDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAutosaveIsSelectedByDefault();
    void testPreviewIconsAreProvidedAsValues();
};

void KisDocumentNamedAutosaveDialogTest::testAutosaveIsSelectedByDefault()
{
    KisRecoverNamedAutosaveDialog dialog;
    QRadioButton *openAutosave = dialog.findChild<QRadioButton *>("rbOpenAutosave");

    QVERIFY(openAutosave);
    QVERIFY(openAutosave->isChecked());
}

void KisDocumentNamedAutosaveDialogTest::testPreviewIconsAreProvidedAsValues()
{
    KisRecoverNamedAutosaveDialog dialog;
    QRadioButton *openAutosave = dialog.findChild<QRadioButton *>("rbOpenAutosave");
    QRadioButton *openMainFile = dialog.findChild<QRadioButton *>("rbDiscardAutosave");
    QVERIFY(openAutosave);
    QVERIFY(openMainFile);

    QPixmap autosavePixmap(4, 4);
    autosavePixmap.fill(Qt::red);
    QPixmap mainFilePixmap(4, 4);
    mainFilePixmap.fill(Qt::blue);
    const QIcon autosaveIcon(autosavePixmap);
    const QIcon mainFileIcon(mainFilePixmap);

    dialog.setAutosaveFileIcon(autosaveIcon);
    dialog.setMainFileIcon(mainFileIcon);

    QCOMPARE(openAutosave->icon().cacheKey(), autosaveIcon.cacheKey());
    QCOMPARE(openMainFile->icon().cacheKey(), mainFileIcon.cacheKey());
    QCOMPARE(openAutosave->iconSize(), dialog.filePreviewIconSize());
    QCOMPARE(openMainFile->iconSize(), dialog.filePreviewIconSize());
}

QTEST_MAIN(KisDocumentNamedAutosaveDialogTest)

#include "kis_document_named_autosave_dialog_test.moc"
