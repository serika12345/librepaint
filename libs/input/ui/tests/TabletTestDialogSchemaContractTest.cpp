/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "wintab/drawpile_tablettester/tablettester.h"

#include <QTest>

#include <type_traits>

class TabletTestDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tabletTestDialogPublicSchemaRemainsStable();
};

void TabletTestDialogSchemaContractTest::tabletTestDialogPublicSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<KoDialog, TabletTestDialog>);
    static_assert(std::is_constructible_v<TabletTestDialog, QWidget *>);
    static_assert(std::has_virtual_destructor_v<TabletTestDialog>);
    static_assert(
        std::is_same_v<decltype(&TabletTestDialog::eventFilter), bool (TabletTestDialog::*)(QObject *, QEvent *)>);
}

QTEST_GUILESS_MAIN(TabletTestDialogSchemaContractTest)

#include "TabletTestDialogSchemaContractTest.moc"
