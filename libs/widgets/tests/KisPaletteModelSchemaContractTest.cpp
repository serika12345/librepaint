/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPaletteModel.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PALETTE_MODEL_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaletteModel::method)), signature>)
} // namespace

class KisPaletteModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paletteModelIdentityAndRoleSchemaRemainsStable();
    void paletteModelQueryAndDragDropSignaturesRemainStable();
    void paletteModelPaletteConnectionSignaturesRemainStable();
    void paletteModelSwatchMutationSignaturesRemainStable();
    void paletteModelGroupAndLayoutSignaturesRemainStable();
};

void KisPaletteModelSchemaContractTest::paletteModelIdentityAndRoleSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisPaletteModel>);
    static_assert(std::is_enum_v<KisPaletteModel::AdditionalRoles>);
    static_assert(std::is_constructible_v<KisPaletteModel>);
    static_assert(std::is_constructible_v<KisPaletteModel, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisPaletteModel>);

    QCOMPARE(int(KisPaletteModel::IsGroupNameRole), int(Qt::UserRole) + 1);
    QCOMPARE(int(KisPaletteModel::CheckSlotRole), int(Qt::UserRole) + 2);
    QCOMPARE(int(KisPaletteModel::GroupNameRole), int(Qt::UserRole) + 3);
    QCOMPARE(int(KisPaletteModel::RowInGroupRole), int(Qt::UserRole) + 4);
}

void KisPaletteModelSchemaContractTest::paletteModelQueryAndDragDropSignaturesRemainStable()
{
    ASSERT_PALETTE_MODEL_SIGNATURE(columnCount, int (KisPaletteModel::*)(const QModelIndex &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(data, QVariant (KisPaletteModel::*)(const QModelIndex &, int) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(
        dropMimeData,
        bool (KisPaletteModel::*)(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &));
    ASSERT_PALETTE_MODEL_SIGNATURE(flags, Qt::ItemFlags (KisPaletteModel::*)(const QModelIndex &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(getSwatch, KisSwatch (KisPaletteModel::*)(const QModelIndex &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(index, QModelIndex (KisPaletteModel::*)(int, int, const QModelIndex &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(indexForClosest, QModelIndex (KisPaletteModel::*)(const KoColor &));
    ASSERT_PALETTE_MODEL_SIGNATURE(mimeData, QMimeData * (KisPaletteModel::*)(const QModelIndexList &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(mimeTypes, QStringList (KisPaletteModel::*)() const);
    ASSERT_PALETTE_MODEL_SIGNATURE(rowCount, int (KisPaletteModel::*)(const QModelIndex &) const);
    ASSERT_PALETTE_MODEL_SIGNATURE(supportedDropActions, Qt::DropActions (KisPaletteModel::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<const KisPaletteModel &>().columnCount()), int>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisPaletteModel &>().data(std::declval<const QModelIndex &>())),
                       QVariant>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPaletteModel &>().index(0, 0)), QModelIndex>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPaletteModel &>().rowCount()), int>);
}

void KisPaletteModelSchemaContractTest::paletteModelPaletteConnectionSignaturesRemainStable()
{
    ASSERT_PALETTE_MODEL_SIGNATURE(colorSet, KoColorSetSP (KisPaletteModel::*)() const);
    ASSERT_PALETTE_MODEL_SIGNATURE(setColorSet, void (KisPaletteModel::*)(KoColorSetSP));
    ASSERT_PALETTE_MODEL_SIGNATURE(setDisplayRenderer,
                                   void (KisPaletteModel::*)(const KoColorDisplayRendererInterface *));
    ASSERT_PALETTE_MODEL_SIGNATURE(sigPaletteChanged, void (KisPaletteModel::*)());
    ASSERT_PALETTE_MODEL_SIGNATURE(sigPaletteModified, void (KisPaletteModel::*)());
    ASSERT_PALETTE_MODEL_SIGNATURE(slotExternalPaletteModified, void (KisPaletteModel::*)(QSharedPointer<KoColorSet>));
}

void KisPaletteModelSchemaContractTest::paletteModelSwatchMutationSignaturesRemainStable()
{
    ASSERT_PALETTE_MODEL_SIGNATURE(addSwatch, void (KisPaletteModel::*)(const KisSwatch &, const QString &));
    ASSERT_PALETTE_MODEL_SIGNATURE(clear, void (KisPaletteModel::*)());
    ASSERT_PALETTE_MODEL_SIGNATURE(clear, void (KisPaletteModel::*)(int));
    ASSERT_PALETTE_MODEL_SIGNATURE(removeSwatch, void (KisPaletteModel::*)(const QModelIndex &, bool));
    ASSERT_PALETTE_MODEL_SIGNATURE(setSwatch, void (KisPaletteModel::*)(const KisSwatch &, const QModelIndex &));

    static_assert(
        std::is_same_v<decltype(std::declval<KisPaletteModel &>().addSwatch(std::declval<const KisSwatch &>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisPaletteModel &>().removeSwatch(std::declval<const QModelIndex &>())),
                       void>);
}

void KisPaletteModelSchemaContractTest::paletteModelGroupAndLayoutSignaturesRemainStable()
{
    ASSERT_PALETTE_MODEL_SIGNATURE(addGroup, KisSwatchGroupSP (KisPaletteModel::*)(const QString &, int, int));
    ASSERT_PALETTE_MODEL_SIGNATURE(changeGroupName, void (KisPaletteModel::*)(const QString &, const QString &));
    ASSERT_PALETTE_MODEL_SIGNATURE(removeGroup, void (KisPaletteModel::*)(const QString &, bool));
    ASSERT_PALETTE_MODEL_SIGNATURE(setColumnCount, void (KisPaletteModel::*)(int));
    ASSERT_PALETTE_MODEL_SIGNATURE(setRowCountForGroup, void (KisPaletteModel::*)(const QString &, int));

    static_assert(std::is_same_v<decltype(std::declval<KisPaletteModel &>().addGroup(std::declval<const QString &>())),
                                 KisSwatchGroupSP>);
    static_assert(std::is_same_v<decltype(std::declval<KisPaletteModel &>().addGroup(std::declval<const QString &>(),
                                                                                     std::declval<int>())),
                                 KisSwatchGroupSP>);
}

QTEST_APPLESS_MAIN(KisPaletteModelSchemaContractTest)

#include "KisPaletteModelSchemaContractTest.moc"
