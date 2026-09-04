/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KoFontFamily.h"
#include "resources/KoFontStorage.h"
#include "text/KoFontRegistry.h"

#include <QTest>

#include <type_traits>
#include <utility>

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
    void fontRegistryTypeLifetimeAndAccessSchemaRemainStable();
    void fontRegistryFaceResolutionSignaturesRemainStable();
    void fontRegistryRepresentationAndMetricsSignaturesRemainStable();
    void fontStorageTypeLifetimeAndValiditySchemaRemainStable();
    void fontStorageResourceAccessSignaturesRemainStable();
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

void KoFontFamilySchemaContractTest::fontRegistryTypeLifetimeAndAccessSchemaRemainStable()
{
    using InstanceSignature = KoFontRegistry *(*)();

    static_assert(std::is_base_of_v<QObject, KoFontRegistry>);
    static_assert(std::is_constructible_v<KoFontRegistry, QObject *>);
    static_assert(std::is_default_constructible_v<KoFontRegistry>);
    static_assert(std::has_virtual_destructor_v<KoFontRegistry>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::instance), InstanceSignature>);
}

void KoFontFamilySchemaContractTest::fontRegistryFaceResolutionSignaturesRemainStable()
{
    using FacesSignature = std::vector<FT_FaceSP> (
        KoFontRegistry::*)(QVector<int> &, KoCSSFontInfo, const QString &, quint32, quint32, bool, const QString &);
    using ConfigureSignature = bool (KoFontRegistry::*)(const std::vector<FT_FaceSP> &,
                                                        qreal,
                                                        qreal,
                                                        quint32,
                                                        quint32,
                                                        const QMap<QString, qreal> &);
    using CssDataSignature = KoCSSFontInfo (KoFontRegistry::*)(const QString, QString *);
    using SlantSignature = QFont::Style (*)(FT_FaceSP);
    using LoadFlagsSignature = int32_t (*)(FT_Face, bool, int32_t, KoSvgText::TextRendering);

    static_assert(std::is_same_v<decltype(&KoFontRegistry::facesForCSSValues), FacesSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::configureFaces), ConfigureSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::getCssDataForPostScriptName), CssDataSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::slantMode), SlantSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::loadFlagsForFace), LoadFlagsSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoFontRegistry &>().facesForCSSValues(std::declval<QVector<int> &>())),
                       std::vector<FT_FaceSP>>);
    static_assert(std::is_same_v<decltype(KoFontRegistry::loadFlagsForFace(std::declval<FT_Face>())), int32_t>);
}

void KoFontFamilySchemaContractTest::fontRegistryRepresentationAndMetricsSignaturesRemainStable()
{
    using CollectSignature = QList<KoFontFamilyWWSRepresentation> (KoFontRegistry::*)() const;
    using RepresentationSignature =
        std::optional<KoFontFamilyWWSRepresentation> (KoFontRegistry::*)(const QString &) const;
    using WwsNameSignature = std::optional<QString> (KoFontRegistry::*)(const QString) const;
    using MetricsSignature = KoSvgText::FontMetrics (KoFontRegistry::*)(KoCSSFontInfo,
                                                                        bool,
                                                                        KoSvgText::TextRendering,
                                                                        const QString &,
                                                                        quint32,
                                                                        quint32,
                                                                        bool,
                                                                        const QString &);
    using GenerateMetricsSignature = KoSvgText::FontMetrics (*)(FT_FaceSP, bool, QString, KoSvgText::TextRendering);

    static_assert(std::is_same_v<decltype(&KoFontRegistry::collectRepresentations), CollectSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::representationByFamilyName), RepresentationSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::wwsNameByFamilyName), WwsNameSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::fontMetricsForCSSValues), MetricsSignature>);
    static_assert(std::is_same_v<decltype(&KoFontRegistry::generateFontMetrics), GenerateMetricsSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoFontRegistry &>().fontMetricsForCSSValues()), KoSvgText::FontMetrics>);
    static_assert(std::is_same_v<decltype(KoFontRegistry::generateFontMetrics(std::declval<FT_FaceSP>())),
                                 KoSvgText::FontMetrics>);
}

void KoFontFamilySchemaContractTest::fontStorageTypeLifetimeAndValiditySchemaRemainStable()
{
    using ValiditySignature = bool (KoFontStorage::*)() const;

    static_assert(std::is_base_of_v<KisStoragePlugin, KoFontStorage>);
    static_assert(std::is_constructible_v<KoFontStorage, const QString &>);
    static_assert(std::is_default_constructible_v<KoFontStorage>);
    static_assert(std::has_virtual_destructor_v<KoFontStorage>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::isValid), ValiditySignature>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::supportsVersioning), ValiditySignature>);
}

void KoFontFamilySchemaContractTest::fontStorageResourceAccessSignaturesRemainStable()
{
    using LoadVersionedSignature = bool (KoFontStorage::*)(KoResourceSP);
    using ResourceSignature = KoResourceSP (KoFontStorage::*)(const QString &);
    using ResourceItemSignature = KisResourceStorage::ResourceItem (KoFontStorage::*)(const QString &);
    using ResourcesSignature = QSharedPointer<KisResourceStorage::ResourceIterator> (KoFontStorage::*)(const QString &);
    using TagsSignature = QSharedPointer<KisResourceStorage::TagIterator> (KoFontStorage::*)(const QString &);

    static_assert(std::is_same_v<decltype(&KoFontStorage::loadVersionedResource), LoadVersionedSignature>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::resource), ResourceSignature>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::resourceItem), ResourceItemSignature>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::resources), ResourcesSignature>);
    static_assert(std::is_same_v<decltype(&KoFontStorage::tags), TagsSignature>);
}

QTEST_APPLESS_MAIN(KoFontFamilySchemaContractTest)

#include "KoFontFamilySchemaContractTest.moc"
