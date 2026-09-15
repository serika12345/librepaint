/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/KisDlgCreateNewDocument.h"

#include <QTest>

#include <type_traits>

class KisDlgCreateNewDocumentSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createNewDocumentDialogSchemaRemainsStable();
};

void KisDlgCreateNewDocumentSchemaContractTest::createNewDocumentDialogSchemaRemainsStable()
{
    using Dialog = KisDlgCreateNewDocument;

    static_assert(std::is_base_of_v<KisOpenPane, Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *>);
    static_assert(std::is_same_v<decltype(&Dialog::SelectPage), void (Dialog::*)(Dialog::Page)>);
    static_assert(std::is_same_v<std::underlying_type_t<Dialog::Page>, unsigned int>);

    QCOMPARE(int(Dialog::CreateNewDocument), 0);
    QCOMPARE(int(Dialog::CreateFromClipboard), 1);
}

QTEST_GUILESS_MAIN(KisDlgCreateNewDocumentSchemaContractTest)

#include "KisDlgCreateNewDocumentSchemaContractTest.moc"
