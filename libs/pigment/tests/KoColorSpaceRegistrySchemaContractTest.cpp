/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorSpaceRegistry.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_REGISTRY_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorSpaceRegistry::method)), signature>)

} // namespace

class KoColorSpaceRegistrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSpaceRegistryTypeLifetimeAndPolicySchemaRemainStable();
    void colorSpaceRegistryFactoryAndProfileMutationSignaturesRemainStable();
    void colorSpaceRegistryProfileAndCatalogQuerySignaturesRemainStable();
    void colorSpaceRegistryResolutionAndConversionSignaturesRemainStable();
    void colorSpaceRegistryDefaultColorSpaceSignaturesRemainStable();
};

void KoColorSpaceRegistrySchemaContractTest::colorSpaceRegistryTypeLifetimeAndPolicySchemaRemainStable()
{
    using Registry = KoColorSpaceRegistry;
    using InstanceSignature = Registry *(*)();

    static_assert(std::is_same_v<Registry::CustomProfileNameAlias, QHash<QString, QString>>);
    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_enum_v<Registry::ColorSpaceListProfilesSelection>);
    static_assert(std::is_enum_v<Registry::ColorSpaceListVisibility>);
    static_assert(Registry::ColorSpaceListProfilesSelection::AllProfiles == 4);
    static_assert(Registry::ColorSpaceListProfilesSelection::OnlyDefaultProfile == 1);
    static_assert(Registry::ColorSpaceListVisibility::AllColorSpaces == 4);
    static_assert(Registry::ColorSpaceListVisibility::OnlyUserVisible == 1);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    static_assert(std::is_same_v<decltype(static_cast<InstanceSignature>(&Registry::instance)), InstanceSignature>);
}

void KoColorSpaceRegistrySchemaContractTest::colorSpaceRegistryFactoryAndProfileMutationSignaturesRemainStable()
{
    using Registry = KoColorSpaceRegistry;
    using CreateProfileSignature =
        const KoColorProfile *(Registry::*)(const QString &, const QString &, const QByteArray &);
    using CreateAliasedProfileSignature = const KoColorProfile *(
        Registry::*)(const QString &, const QString &, const QByteArray &, Registry::CustomProfileNameAlias);

    ASSERT_REGISTRY_SIGNATURE(add, void (Registry::*)(KoColorSpaceFactory *));
    ASSERT_REGISTRY_SIGNATURE(addProfile, void (Registry::*)(KoColorProfile *));
    ASSERT_REGISTRY_SIGNATURE(addProfile, void (Registry::*)(const KoColorProfile *));
    ASSERT_REGISTRY_SIGNATURE(addProfileAlias, void (Registry::*)(const QString &, const QString &));
    ASSERT_REGISTRY_SIGNATURE(addProfileToMap, void (Registry::*)(KoColorProfile *));
    static_assert(std::is_same_v<decltype(static_cast<CreateProfileSignature>(&Registry::createColorProfile)),
                                 CreateProfileSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CreateAliasedProfileSignature>(&Registry::createColorProfile)),
                                 CreateAliasedProfileSignature>);
    ASSERT_REGISTRY_SIGNATURE(remove, void (Registry::*)(KoColorSpaceFactory *));
    ASSERT_REGISTRY_SIGNATURE(removeProfile, void (Registry::*)(KoColorProfile *));
    ASSERT_REGISTRY_SIGNATURE(removeProfileAlias, void (Registry::*)(const QString &));
}

void KoColorSpaceRegistrySchemaContractTest::colorSpaceRegistryProfileAndCatalogQuerySignaturesRemainStable()
{
    using Registry = KoColorSpaceRegistry;
    using ColorDepthByIdSignature = QList<KoID> (Registry::*)(const KoID &, Registry::ColorSpaceListVisibility) const;
    using ColorDepthByNameSignature =
        QList<KoID> (Registry::*)(const QString &, Registry::ColorSpaceListVisibility) const;

    ASSERT_REGISTRY_SIGNATURE(QColorSpaceForProfile, QColorSpace (Registry::*)(const KoColorProfile *) const);
    static_assert(std::is_same_v<decltype(static_cast<ColorDepthByIdSignature>(&Registry::colorDepthList)),
                                 ColorDepthByIdSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorDepthByNameSignature>(&Registry::colorDepthList)),
                                 ColorDepthByNameSignature>);
    ASSERT_REGISTRY_SIGNATURE(colorModelsList, QList<KoID> (Registry::*)(Registry::ColorSpaceListVisibility) const);
    ASSERT_REGISTRY_SIGNATURE(defaultProfileForColorSpace, QString (Registry::*)(const QString &) const);
    ASSERT_REGISTRY_SIGNATURE(listKeys, QList<KoID> (Registry::*)() const);
    ASSERT_REGISTRY_SIGNATURE(profileAlias, QString (Registry::*)(const QString &) const);
    ASSERT_REGISTRY_SIGNATURE(profileByName, const KoColorProfile *(Registry::*)(const QString &) const);
    ASSERT_REGISTRY_SIGNATURE(profileByUniqueId, const KoColorProfile *(Registry::*)(const QByteArray &) const);
    ASSERT_REGISTRY_SIGNATURE(profileFor, const KoColorProfile *(Registry::*)(const KoColorProfileQuery &, bool) const);
    ASSERT_REGISTRY_SIGNATURE(profileForQColorSpace, const KoColorProfile *(Registry::*)(const QColorSpace &));
    ASSERT_REGISTRY_SIGNATURE(profileIsCompatible, bool (Registry::*)(const KoColorProfile *, const QString &));
    ASSERT_REGISTRY_SIGNATURE(profilesFor, QList<const KoColorProfile *> (Registry::*)(const QString &) const);

    using ProfileForWithDefaultGeneration =
        decltype(std::declval<const Registry &>().profileFor(std::declval<const KoColorProfileQuery &>()));
    static_assert(std::is_same_v<ProfileForWithDefaultGeneration, const KoColorProfile *>);
}

void KoColorSpaceRegistrySchemaContractTest::colorSpaceRegistryResolutionAndConversionSignaturesRemainStable()
{
    using Registry = KoColorSpaceRegistry;
    using ColorSpaceByProfileSignature =
        const KoColorSpace *(Registry::*)(const QString &, const QString &, const KoColorProfile *);
    using ColorSpaceByProfileNameSignature =
        const KoColorSpace *(Registry::*)(const QString &, const QString &, const QString &);
    using ColorSpaceByIdsSignature = const KoColorSpace *(Registry::*)(const QString &, const QString &);
    using ColorSpaceIdByIdsSignature = QString (Registry::*)(const KoID &, const KoID &) const;
    using ColorSpaceIdByNamesSignature = QString (Registry::*)(const QString &, const QString &) const;

    ASSERT_REGISTRY_SIGNATURE(colorConversionCache, KoColorConversionCache * (Registry::*)() const);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::colorSpace)),
                                 ColorSpaceByProfileSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileNameSignature>(&Registry::colorSpace)),
                                 ColorSpaceByProfileNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByIdsSignature>(&Registry::colorSpace)),
                                 ColorSpaceByIdsSignature>);
    ASSERT_REGISTRY_SIGNATURE(colorSpaceColorDepthId, KoID (Registry::*)(const QString &) const);
    ASSERT_REGISTRY_SIGNATURE(colorSpaceColorModelId, KoID (Registry::*)(const QString &) const);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceIdByIdsSignature>(&Registry::colorSpaceId)),
                                 ColorSpaceIdByIdsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceIdByNamesSignature>(&Registry::colorSpaceId)),
                                 ColorSpaceIdByNamesSignature>);
    ASSERT_REGISTRY_SIGNATURE(createColorConverter,
                              KoColorConversionTransformation
                                  * (Registry::*)(const KoColorSpace *,
                                                  const KoColorSpace *,
                                                  KoColorConversionTransformation::Intent,
                                                  KoColorConversionTransformation::ConversionFlags) const);
    ASSERT_REGISTRY_SIGNATURE(createColorConverters,
                              void (Registry::*)(const KoColorSpace *,
                                                 const QList<QPair<KoID, KoID>> &,
                                                 KoColorConversionTransformation *&,
                                                 KoColorConversionTransformation *&) const);
    ASSERT_REGISTRY_SIGNATURE(permanentColorspace, const KoColorSpace *(Registry::*)(const KoColorSpace *));
}

void KoColorSpaceRegistrySchemaContractTest::colorSpaceRegistryDefaultColorSpaceSignaturesRemainStable()
{
    using Registry = KoColorSpaceRegistry;
    using DefaultColorSpaceSignature = const KoColorSpace *(Registry::*)();
    using ColorSpaceByNameSignature = const KoColorSpace *(Registry::*)(const QString &);
    using ColorSpaceByProfileSignature = const KoColorSpace *(Registry::*)(const KoColorProfile *);

    ASSERT_REGISTRY_SIGNATURE(alpha8, DefaultColorSpaceSignature);
    ASSERT_REGISTRY_SIGNATURE(alpha16, DefaultColorSpaceSignature);
    ASSERT_REGISTRY_SIGNATURE(alpha16f, DefaultColorSpaceSignature);
    ASSERT_REGISTRY_SIGNATURE(alpha32f, DefaultColorSpaceSignature);
    static_assert(
        std::is_same_v<decltype(static_cast<ColorSpaceByNameSignature>(&Registry::graya8)), ColorSpaceByNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::graya8)),
                                 ColorSpaceByProfileSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByNameSignature>(&Registry::graya16)),
                                 ColorSpaceByNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::graya16)),
                                 ColorSpaceByProfileSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ColorSpaceByNameSignature>(&Registry::lab16)), ColorSpaceByNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::lab16)),
                                 ColorSpaceByProfileSignature>);
    ASSERT_REGISTRY_SIGNATURE(p2020G10Profile, const KoColorProfile *(Registry::*)() const);
    ASSERT_REGISTRY_SIGNATURE(p2020PQProfile, const KoColorProfile *(Registry::*)() const);
    ASSERT_REGISTRY_SIGNATURE(p709G10Profile, const KoColorProfile *(Registry::*)() const);
    ASSERT_REGISTRY_SIGNATURE(p709SRGBProfile, const KoColorProfile *(Registry::*)() const);
    static_assert(
        std::is_same_v<decltype(static_cast<ColorSpaceByNameSignature>(&Registry::rgb8)), ColorSpaceByNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::rgb8)),
                                 ColorSpaceByProfileSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ColorSpaceByNameSignature>(&Registry::rgb16)), ColorSpaceByNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ColorSpaceByProfileSignature>(&Registry::rgb16)),
                                 ColorSpaceByProfileSignature>);

    static_assert(std::is_same_v<decltype(std::declval<Registry &>().graya8()), const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(std::declval<Registry &>().graya16()), const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(std::declval<Registry &>().lab16()), const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(std::declval<Registry &>().rgb8()), const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(std::declval<Registry &>().rgb16()), const KoColorSpace *>);
}

QTEST_GUILESS_MAIN(KoColorSpaceRegistrySchemaContractTest)

#include "KoColorSpaceRegistrySchemaContractTest.moc"
