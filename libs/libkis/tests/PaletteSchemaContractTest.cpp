/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Palette.h>
#include <QTest>
#include <type_traits>
#include <utility>

#define ASSERT_PALETTE_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Palette::method)), signature>)
#define ASSERT_MANAGED_COLOR_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&ManagedColor::method)), signature>)
#define ASSERT_SWATCH_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Swatch::method)), signature>)

class PaletteSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void paletteOwnershipLifetimeAndEqualitySchemaRemainsStable();
    void paletteGridAndCountSignaturesRemainStable();
    void paletteGroupOperationSignaturesRemainStable();
    void paletteEntryOperationSignaturesRemainStable();
    void paletteMetadataAndPersistenceSignaturesRemainStable();
    void managedColorTypeLifetimeAndEqualitySchemaRemainStable();
    void managedColorColorSpaceAndCanvasSignaturesRemainStable();
    void managedColorComponentsAndSerializationSignaturesRemainStable();
    void swatchTypeLifetimeCopyAndEqualitySchemaRemainStable();
    void swatchMetadataColorAndValiditySignaturesRemainStable();
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

void PaletteSchemaContractTest::managedColorTypeLifetimeAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<ManagedColor> && std::is_base_of_v<QObject, ManagedColor>);
    static_assert(std::is_default_constructible_v<ManagedColor> && std::is_constructible_v<ManagedColor, QObject *>);
    static_assert(
        std::is_constructible_v<ManagedColor, const QString &, const QString &, const QString &>
        && std::is_constructible_v<ManagedColor, const QString &, const QString &, const QString &, QObject *>);
    static_assert(std::is_constructible_v<ManagedColor, KoColor>
                  && std::is_constructible_v<ManagedColor, KoColor, QObject *>);
    static_assert(std::has_virtual_destructor_v<ManagedColor>);
    ASSERT_MANAGED_COLOR_SIGNATURE(operator==, bool (ManagedColor::*)(const ManagedColor &) const);
}

void PaletteSchemaContractTest::managedColorColorSpaceAndCanvasSignaturesRemainStable()
{
    ASSERT_MANAGED_COLOR_SIGNATURE(colorDepth, QString (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(colorForCanvas, QColor (ManagedColor::*)(Canvas *) const);
    ASSERT_MANAGED_COLOR_SIGNATURE(colorModel, QString (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(colorProfile, QString (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(fromQColor, ManagedColor * (*)(const QColor &, Canvas *));
    static_assert(std::is_same_v<decltype(ManagedColor::fromQColor(std::declval<const QColor &>())), ManagedColor *>);
    ASSERT_MANAGED_COLOR_SIGNATURE(setColorProfile, bool (ManagedColor::*)(const QString &));
    ASSERT_MANAGED_COLOR_SIGNATURE(setColorSpace,
                                   bool (ManagedColor::*)(const QString &, const QString &, const QString &));
}

void PaletteSchemaContractTest::managedColorComponentsAndSerializationSignaturesRemainStable()
{
    ASSERT_MANAGED_COLOR_SIGNATURE(components, QVector<float> (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(componentsOrdered, QVector<float> (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(setComponents, void (ManagedColor::*)(const QVector<float> &));
    ASSERT_MANAGED_COLOR_SIGNATURE(fromXML, void (ManagedColor::*)(const QString &));
    ASSERT_MANAGED_COLOR_SIGNATURE(toXML, QString (ManagedColor::*)() const);
    ASSERT_MANAGED_COLOR_SIGNATURE(toQString, QString (ManagedColor::*)());
}

void PaletteSchemaContractTest::swatchTypeLifetimeCopyAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<Swatch> && std::is_base_of_v<QObject, Swatch>);
    static_assert(std::is_default_constructible_v<Swatch> && std::is_constructible_v<Swatch, QObject *>);
    static_assert(std::is_copy_constructible_v<Swatch> && std::is_constructible_v<Swatch, const Swatch &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Swatch>);
    ASSERT_SWATCH_SIGNATURE(operator=, Swatch & (Swatch::*)(const Swatch &));
    ASSERT_SWATCH_SIGNATURE(operator==, bool (Swatch::*)(const Swatch &) const);
    ASSERT_SWATCH_SIGNATURE(operator!=, bool (Swatch::*)(const Swatch &) const);
}

void PaletteSchemaContractTest::swatchMetadataColorAndValiditySignaturesRemainStable()
{
    ASSERT_SWATCH_SIGNATURE(color, ManagedColor * (Swatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(id, QString (Swatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(isValid, bool (Swatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(name, QString (Swatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(setColor, void (Swatch::*)(ManagedColor *));
    ASSERT_SWATCH_SIGNATURE(setId, void (Swatch::*)(const QString &));
    ASSERT_SWATCH_SIGNATURE(setName, void (Swatch::*)(const QString &));
    ASSERT_SWATCH_SIGNATURE(setSpotColor, void (Swatch::*)(bool));
    ASSERT_SWATCH_SIGNATURE(spotColor, bool (Swatch::*)() const);
}

QTEST_GUILESS_MAIN(PaletteSchemaContractTest)
#include "PaletteSchemaContractTest.moc"
