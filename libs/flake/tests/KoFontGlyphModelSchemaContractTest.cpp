/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <text/KoFontGlyphModel.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using FeatureInfoMap = QMap<QString, KoOpenTypeFeatureInfo>;
using RoleNameMap = QHash<int, QByteArray>;

#define ASSERT_GLYPH_MODEL_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoFontGlyphModel::method)), signature>)
} // namespace

class KoFontGlyphModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontGlyphModelIdentityAndLifecycleSignaturesRemainStable();
    void fontGlyphModelRoleAndGlyphTypeValuesRemainStable();
    void fontGlyphModelTreeNavigationSignaturesRemainStable();
    void fontGlyphModelDataAndMetadataSignaturesRemainStable();
    void fontGlyphModelFaceSelectionSignatureRemainsStable();
};

void KoFontGlyphModelSchemaContractTest::fontGlyphModelIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoFontGlyphModel>);
    static_assert(std::is_base_of_v<QAbstractItemModel, KoFontGlyphModel>);
    static_assert(std::is_constructible_v<KoFontGlyphModel>);
    static_assert(std::is_constructible_v<KoFontGlyphModel, QObject *>);
    static_assert(std::has_virtual_destructor_v<KoFontGlyphModel>);
}

void KoFontGlyphModelSchemaContractTest::fontGlyphModelRoleAndGlyphTypeValuesRemainStable()
{
    static_assert(std::is_enum_v<KoFontGlyphModel::GlyphType>);
    QCOMPARE(int(KoFontGlyphModel::Base), 0);
    QCOMPARE(int(KoFontGlyphModel::UnicodeVariationSelector), 1);
    QCOMPARE(int(KoFontGlyphModel::OpenType), 2);

    static_assert(std::is_enum_v<KoFontGlyphModel::Roles>);
    QCOMPARE(int(KoFontGlyphModel::OpenTypeFeatures), int(Qt::UserRole) + 1);
    QCOMPARE(int(KoFontGlyphModel::GlyphLabel), int(Qt::UserRole) + 2);
    QCOMPARE(int(KoFontGlyphModel::ChildCount), int(Qt::UserRole) + 3);
}

void KoFontGlyphModelSchemaContractTest::fontGlyphModelTreeNavigationSignaturesRemainStable()
{
    ASSERT_GLYPH_MODEL_SIGNATURE(index, QModelIndex (KoFontGlyphModel::*)(int, int, const QModelIndex &) const);
    ASSERT_GLYPH_MODEL_SIGNATURE(parent, QModelIndex (KoFontGlyphModel::*)(const QModelIndex &) const);
    ASSERT_GLYPH_MODEL_SIGNATURE(rowCount, int (KoFontGlyphModel::*)(const QModelIndex &) const);
    ASSERT_GLYPH_MODEL_SIGNATURE(columnCount, int (KoFontGlyphModel::*)(const QModelIndex &) const);
    ASSERT_GLYPH_MODEL_SIGNATURE(hasChildren, bool (KoFontGlyphModel::*)(const QModelIndex &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KoFontGlyphModel &>().index(0, 0)), QModelIndex>);
    static_assert(std::is_same_v<decltype(std::declval<const KoFontGlyphModel &>().rowCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KoFontGlyphModel &>().columnCount()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KoFontGlyphModel &>().hasChildren()), bool>);
}

void KoFontGlyphModelSchemaContractTest::fontGlyphModelDataAndMetadataSignaturesRemainStable()
{
    ASSERT_GLYPH_MODEL_SIGNATURE(data, QVariant (KoFontGlyphModel::*)(const QModelIndex &, int) const);
    ASSERT_GLYPH_MODEL_SIGNATURE(roleNames, RoleNameMap (KoFontGlyphModel::*)() const);
    ASSERT_GLYPH_MODEL_SIGNATURE(blocks, QVector<KoUnicodeBlockData> (KoFontGlyphModel::*)() const);
    ASSERT_GLYPH_MODEL_SIGNATURE(featureInfo, FeatureInfoMap (KoFontGlyphModel::*)() const);
    ASSERT_GLYPH_MODEL_SIGNATURE(indexForString, QModelIndex (KoFontGlyphModel::*)(QString));

    static_assert(
        std::is_same_v<decltype(std::declval<const KoFontGlyphModel &>().data(std::declval<const QModelIndex &>())),
                       QVariant>);
}

void KoFontGlyphModelSchemaContractTest::fontGlyphModelFaceSelectionSignatureRemainsStable()
{
    ASSERT_GLYPH_MODEL_SIGNATURE(setFace, void (KoFontGlyphModel::*)(FT_FaceSP, QLatin1String, bool));
    static_assert(
        std::is_same_v<decltype(std::declval<KoFontGlyphModel &>().setFace(std::declval<FT_FaceSP>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<KoFontGlyphModel &>().setFace(std::declval<FT_FaceSP>(),
                                                                                     std::declval<QLatin1String>())),
                                 void>);
}

QTEST_APPLESS_MAIN(KoFontGlyphModelSchemaContractTest)

#include "KoFontGlyphModelSchemaContractTest.moc"
