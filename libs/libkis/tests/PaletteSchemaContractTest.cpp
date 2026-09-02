/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Palette.h>
#include <QTest>
#include <type_traits>
#include <utility>

#define ASSERT_PALETTE_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Palette::method)), signature>)

class PaletteSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void paletteOwnershipLifetimeAndEqualitySchemaRemainsStable();
    void paletteGridAndCountSignaturesRemainStable();
    void paletteGroupOperationSignaturesRemainStable();
    void paletteEntryOperationSignaturesRemainStable();
    void paletteMetadataAndPersistenceSignaturesRemainStable();
};

void PaletteSchemaContractTest::paletteOwnershipLifetimeAndEqualitySchemaRemainsStable()
{
    static_assert(std::is_class_v<Palette>);
    static_assert(std::is_base_of_v<QObject, Palette>);
    static_assert(std::is_constructible_v<Palette, Resource *>);
    static_assert(std::is_constructible_v<Palette, Resource *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Palette>);
    ASSERT_PALETTE_SIGNATURE(operator==, bool (Palette::*)(const Palette &) const);
    ASSERT_PALETTE_SIGNATURE(operator!=, bool (Palette::*)(const Palette &) const);
}

void PaletteSchemaContractTest::paletteGridAndCountSignaturesRemainStable()
{
    ASSERT_PALETTE_SIGNATURE(colorsCountGroup, int (Palette::*)(QString));
    ASSERT_PALETTE_SIGNATURE(colorsCountTotal, int (Palette::*)());
    ASSERT_PALETTE_SIGNATURE(columnCount, int (Palette::*)());
    ASSERT_PALETTE_SIGNATURE(numberOfEntries, int (Palette::*)() const);
    ASSERT_PALETTE_SIGNATURE(rowCount, int (Palette::*)());
    ASSERT_PALETTE_SIGNATURE(rowCountGroup, int (Palette::*)(QString));
    ASSERT_PALETTE_SIGNATURE(setColumnCount, void (Palette::*)(int));
    ASSERT_PALETTE_SIGNATURE(setRowCountGroup, void (Palette::*)(int, QString));
    ASSERT_PALETTE_SIGNATURE(slotCount, int (Palette::*)());
    ASSERT_PALETTE_SIGNATURE(slotCountGroup, int (Palette::*)(QString));
}

void PaletteSchemaContractTest::paletteGroupOperationSignaturesRemainStable()
{
    ASSERT_PALETTE_SIGNATURE(addGroup, void (Palette::*)(QString));
    ASSERT_PALETTE_SIGNATURE(changeGroupName, void (Palette::*)(QString, QString));
    ASSERT_PALETTE_SIGNATURE(groupNames, QStringList (Palette::*)() const);
    ASSERT_PALETTE_SIGNATURE(moveGroup, void (Palette::*)(const QString &, const QString &));
    ASSERT_PALETTE_SIGNATURE(removeGroup, void (Palette::*)(QString, bool));
    ASSERT_PALETTE_SIGNATURE(renameGroup, void (Palette::*)(QString, QString));
    static_assert(std::is_same_v<decltype(std::declval<Palette &>().moveGroup(std::declval<const QString &>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Palette &>().removeGroup(std::declval<QString>())), void>);
}

void PaletteSchemaContractTest::paletteEntryOperationSignaturesRemainStable()
{
    ASSERT_PALETTE_SIGNATURE(addEntry, void (Palette::*)(Swatch, QString));
    ASSERT_PALETTE_SIGNATURE(colorSetEntryByIndex, Swatch * (Palette::*)(int));
    ASSERT_PALETTE_SIGNATURE(colorSetEntryFromGroup, Swatch * (Palette::*)(int, const QString &));
    ASSERT_PALETTE_SIGNATURE(entryByIndex, Swatch * (Palette::*)(int));
    ASSERT_PALETTE_SIGNATURE(entryByIndexFromGroup, Swatch * (Palette::*)(int, const QString &));
    ASSERT_PALETTE_SIGNATURE(removeEntry, void (Palette::*)(int));
    ASSERT_PALETTE_SIGNATURE(removeEntryFromGroup, void (Palette::*)(int, const QString &));
    static_assert(std::is_same_v<decltype(std::declval<Palette &>().addEntry(std::declval<Swatch>())), void>);
}

void PaletteSchemaContractTest::paletteMetadataAndPersistenceSignaturesRemainStable()
{
    ASSERT_PALETTE_SIGNATURE(comment, QString (Palette::*)());
    ASSERT_PALETTE_SIGNATURE(setComment, void (Palette::*)(QString));
    ASSERT_PALETTE_SIGNATURE(save, bool (Palette::*)());
}

QTEST_GUILESS_MAIN(PaletteSchemaContractTest)
#include "PaletteSchemaContractTest.moc"
