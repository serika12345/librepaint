/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPaletteComboBox.h>

#include <QTest>

#include <type_traits>

class KisPaletteComboBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paletteComboBoxSchemaRemainsStable();
};

void KisPaletteComboBoxSchemaContractTest::paletteComboBoxSchemaRemainsStable()
{
    using ColorSelected = void (KisPaletteComboBox::*)(const KoColor &);
    using SetCompanionView = void (KisPaletteComboBox::*)(KisPaletteView *);

    static_assert(std::is_base_of_v<KisSqueezedComboBox, KisPaletteComboBox>);
    static_assert(std::is_constructible_v<KisPaletteComboBox, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisPaletteComboBox>);
    static_assert(std::is_same_v<decltype(&KisPaletteComboBox::sigColorSelected), ColorSelected>);
    static_assert(std::is_same_v<decltype(&KisPaletteComboBox::setCompanionView), SetCompanionView>);
}

QTEST_APPLESS_MAIN(KisPaletteComboBoxSchemaContractTest)

#include "KisPaletteComboBoxSchemaContractTest.moc"
