/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KoFontFamily.h"

#include <QTest>

#include <type_traits>

namespace
{
using FontFamily = KoFontFamily;
using AxisList = QList<KoSvgText::FontFamilyAxis>;
using StyleList = QList<KoSvgText::FontFamilyStyleInfo>;
using ResourceTypePair = QPair<QString, QString>;

#define ASSERT_FONT_FAMILY_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&FontFamily::method)), signature>)
} // namespace

class KoFontFamilySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontFamilyTypeLifetimeAndConstructionSchemaRemainStable();
    void fontFamilyIdentityAndLocalizationSignaturesRemainStable();
    void fontFamilyAxesAndStylesSignaturesRemainStable();
    void fontFamilyCapabilitySignaturesRemainStable();
    void fontFamilyResourcePersistenceSignaturesRemainStable();
};

void KoFontFamilySchemaContractTest::fontFamilyTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_same_v<KoFontFamilySP, QSharedPointer<FontFamily>>);
    static_assert(std::is_class_v<FontFamily>);
    static_assert(std::is_base_of_v<KoResource, FontFamily>);
    static_assert(std::has_virtual_destructor_v<FontFamily>);
    static_assert(std::is_constructible_v<FontFamily, KoFontFamilyWWSRepresentation>);
    static_assert(std::is_constructible_v<FontFamily, const QString &>);
    static_assert(std::is_copy_constructible_v<FontFamily>);
    static_assert(!std::is_copy_assignable_v<FontFamily>);
}

void KoFontFamilySchemaContractTest::fontFamilyIdentityAndLocalizationSignaturesRemainStable()
{
    ASSERT_FONT_FAMILY_SIGNATURE(typographicFamily, QString (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(translatedFontName, QString (FontFamily::*)(QStringList) const);
    ASSERT_FONT_FAMILY_SIGNATURE(lastModified, QDateTime (FontFamily::*)() const);
}

void KoFontFamilySchemaContractTest::fontFamilyAxesAndStylesSignaturesRemainStable()
{
    ASSERT_FONT_FAMILY_SIGNATURE(axes, AxisList (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(styles, StyleList (FontFamily::*)() const);
}

void KoFontFamilySchemaContractTest::fontFamilyCapabilitySignaturesRemainStable()
{
    ASSERT_FONT_FAMILY_SIGNATURE(isVariable, bool (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(colorBitmap, bool (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(colorClrV0, bool (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(colorClrV1, bool (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(colorSVG, bool (FontFamily::*)() const);
}

void KoFontFamilySchemaContractTest::fontFamilyResourcePersistenceSignaturesRemainStable()
{
    ASSERT_FONT_FAMILY_SIGNATURE(clone, KoResourceSP (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(loadFromDevice, bool (FontFamily::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_FONT_FAMILY_SIGNATURE(isSerializable, bool (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(resourceType, ResourceTypePair (FontFamily::*)() const);
    ASSERT_FONT_FAMILY_SIGNATURE(updateThumbnail, void (FontFamily::*)());
}

QTEST_APPLESS_MAIN(KoFontFamilySchemaContractTest)

#include "KoFontFamilySchemaContractTest.moc"
