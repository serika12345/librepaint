/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KoColorSet.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_COLOR_SET_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorSet::method)), signature>)
#define ASSERT_COLOR_SET_STRING_MEMBER(member)                                                                         \
    static_assert(std::is_same_v<decltype(KoColorSet::member), const QString>)
} // namespace

class KoColorSetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSetTypeFormatAndLifetimeSchemaRemainsStable();
    void colorSetSerializationSchemaRemainsStable();
    void colorSetStateAndLayoutSchemaRemainsStable();
    void colorSetLookupSchemaRemainsStable();
    void colorSetMutationAndNotificationSchemaRemainsStable();
};

void KoColorSetSchemaContractTest::colorSetTypeFormatAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KoColorSetSP, QSharedPointer<KoColorSet>>);
    static_assert(std::is_class_v<KoColorSet>);
    static_assert(std::is_base_of_v<QObject, KoColorSet>);
    static_assert(std::is_base_of_v<KoResource, KoColorSet>);
    static_assert(std::is_enum_v<KoColorSet::PaletteType>);

    static_assert(KoColorSet::UNKNOWN == 0);
    static_assert(KoColorSet::GPL == 1);
    static_assert(KoColorSet::RIFF_PAL == 2);
    static_assert(KoColorSet::ACT == 3);
    static_assert(KoColorSet::PSP_PAL == 4);
    static_assert(KoColorSet::ACO == 5);
    static_assert(KoColorSet::XML == 6);
    static_assert(KoColorSet::KPL == 7);
    static_assert(KoColorSet::SBZ == 8);
    static_assert(KoColorSet::ASE == 9);
    static_assert(KoColorSet::ACB == 10);
    static_assert(KoColorSet::CSS == 11);

    static_assert(std::is_default_constructible_v<KoColorSet>);
    static_assert(std::is_constructible_v<KoColorSet, const QString &>);
    static_assert(std::is_copy_constructible_v<KoColorSet>);
    static_assert(std::is_destructible_v<KoColorSet>);
    static_assert(std::has_virtual_destructor_v<KoColorSet>);
    static_assert(!std::is_copy_assignable_v<KoColorSet>);
    ASSERT_COLOR_SET_SIGNATURE(clone, KoResourceSP (KoColorSet::*)() const);

    QVERIFY(true);
}

void KoColorSetSchemaContractTest::colorSetSerializationSchemaRemainsStable()
{
    using ResourceType = QPair<QString, QString>;

    ASSERT_COLOR_SET_STRING_MEMBER(GLOBAL_GROUP_NAME);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_COLOR_DEPTH_ID_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_COLOR_MODEL_ID_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_GROUP_NAME_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_GROUP_ROW_COUNT_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_GROUP_TAG);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_COLUMN_COUNT_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_COMMENT_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_FILENAME_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_NAME_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_PROFILE_TAG);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_READONLY_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_PALETTE_TAG);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_BITDEPTH_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_COL_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_ID_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_NAME_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_POS_TAG);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_ROW_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_SPOT_ATTR);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_SWATCH_TAG);
    ASSERT_COLOR_SET_STRING_MEMBER(KPL_VERSION_ATTR);

    ASSERT_COLOR_SET_SIGNATURE(defaultFileExtension, QString (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(fromByteArray, bool (KoColorSet::*)(QByteArray &, KisResourcesInterfaceSP));
    ASSERT_COLOR_SET_SIGNATURE(loadFromDevice, bool (KoColorSet::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_COLOR_SET_SIGNATURE(resourceType, ResourceType (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(saveToDevice, bool (KoColorSet::*)(QIODevice *) const);

    QVERIFY(true);
}

void KoColorSetSchemaContractTest::colorSetStateAndLayoutSchemaRemainsStable()
{
    ASSERT_COLOR_SET_SIGNATURE(clear, void (KoColorSet::*)());
    ASSERT_COLOR_SET_SIGNATURE(colorCount, quint32 (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(columnCount, int (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(comment, QString (KoColorSet::*)());
    ASSERT_COLOR_SET_SIGNATURE(isLocked, bool (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(paletteType, KoColorSet::PaletteType (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(rowCount, int (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(rowCountWithTitles, int (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(setColumnCount, void (KoColorSet::*)(int));
    ASSERT_COLOR_SET_SIGNATURE(setComment, void (KoColorSet::*)(QString));
    ASSERT_COLOR_SET_SIGNATURE(setLocked, void (KoColorSet::*)(bool));
    ASSERT_COLOR_SET_SIGNATURE(setPaletteType, void (KoColorSet::*)(KoColorSet::PaletteType));
    ASSERT_COLOR_SET_SIGNATURE(slotCount, quint32 (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(undoStack, KUndo2Stack * (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(updateThumbnail, void (KoColorSet::*)());

    QVERIFY(true);
}

void KoColorSetSchemaContractTest::colorSetLookupSchemaRemainsStable()
{
    ASSERT_COLOR_SET_SIGNATURE(getClosestSwatchInfo, KisSwatchGroup::SwatchInfo (KoColorSet::*)(KoColor, bool) const);
    ASSERT_COLOR_SET_SIGNATURE(getColorGlobal, KisSwatch (KoColorSet::*)(quint32, quint32) const);
    ASSERT_COLOR_SET_SIGNATURE(getGlobalGroup, KisSwatchGroupSP (KoColorSet::*)() const);
    ASSERT_COLOR_SET_SIGNATURE(getGroup, KisSwatchGroupSP (KoColorSet::*)(const QString &) const);
    ASSERT_COLOR_SET_SIGNATURE(getGroup, KisSwatchGroupSP (KoColorSet::*)(int) const);
    ASSERT_COLOR_SET_SIGNATURE(getSwatchFromGroup, KisSwatch (KoColorSet::*)(quint32, quint32, QString) const);
    ASSERT_COLOR_SET_SIGNATURE(isGroupTitleRow, bool (KoColorSet::*)(int) const);
    ASSERT_COLOR_SET_SIGNATURE(rowNumberInGroup, int (KoColorSet::*)(int) const);
    ASSERT_COLOR_SET_SIGNATURE(startRowForGroup, int (KoColorSet::*)(const QString &) const);
    ASSERT_COLOR_SET_SIGNATURE(swatchGroupNames, QStringList (KoColorSet::*)() const);

    static_assert(
        std::is_same_v<decltype(std::declval<const KoColorSet &>().getClosestSwatchInfo(std::declval<KoColor>())),
                       KisSwatchGroup::SwatchInfo>);
    static_assert(std::is_same_v<decltype(std::declval<const KoColorSet &>().getSwatchFromGroup(0, 0)), KisSwatch>);

    QVERIFY(true);
}

void KoColorSetSchemaContractTest::colorSetMutationAndNotificationSchemaRemainsStable()
{
    ASSERT_COLOR_SET_SIGNATURE(addGroup, void (KoColorSet::*)(const QString &, int, int));
    ASSERT_COLOR_SET_SIGNATURE(addSwatch, void (KoColorSet::*)(const KisSwatch &, const QString &, int, int));
    ASSERT_COLOR_SET_SIGNATURE(changeGroupName, void (KoColorSet::*)(const QString &, const QString &));
    ASSERT_COLOR_SET_SIGNATURE(moveGroup, void (KoColorSet::*)(const QString &, const QString &));
    ASSERT_COLOR_SET_SIGNATURE(removeGroup, void (KoColorSet::*)(const QString &, bool));
    ASSERT_COLOR_SET_SIGNATURE(removeSwatch, void (KoColorSet::*)(int, int, KisSwatchGroupSP));
    ASSERT_COLOR_SET_SIGNATURE(entryChanged, void (KoColorSet::*)(int, int));
    ASSERT_COLOR_SET_SIGNATURE(layoutAboutToChange, void (KoColorSet::*)());
    ASSERT_COLOR_SET_SIGNATURE(layoutChanged, void (KoColorSet::*)());
    ASSERT_COLOR_SET_SIGNATURE(modified, void (KoColorSet::*)());

    static_assert(
        std::is_same_v<decltype(std::declval<KoColorSet &>().addGroup(std::declval<const QString &>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoColorSet &>().addSwatch(std::declval<const KisSwatch &>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoColorSet &>().moveGroup(std::declval<const QString &>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoColorSet &>().removeGroup(std::declval<const QString &>())), void>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KoColorSetSchemaContractTest)

#include "KoColorSetSchemaContractTest.moc"
