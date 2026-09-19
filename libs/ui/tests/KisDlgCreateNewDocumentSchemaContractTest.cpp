/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/KisDlgCreateNewDocument.h"

#include <QTest>


class KisDlgCreateNewDocumentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createNewDocumentDialogSchemaRemainsStable();
};

void KisDlgCreateNewDocumentSchemaContractTest::createNewDocumentDialogSchemaRemainsStable()
{
    using Dialog = KisDlgCreateNewDocument;


    QCOMPARE(int(Dialog::CreateNewDocument), 0);
    QCOMPARE(int(Dialog::CreateFromClipboard), 1);
}

QTEST_GUILESS_MAIN(KisDlgCreateNewDocumentSchemaContractTest)

#include "KisDlgCreateNewDocumentSchemaContractTest.moc"
