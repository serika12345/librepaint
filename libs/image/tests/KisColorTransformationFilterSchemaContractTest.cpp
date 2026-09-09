/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filter/kis_color_transformation_configuration.h"
#include "filter/kis_color_transformation_filter.h"
#include "filter/kis_filter_registry.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

class ColorFilterConstructionProbe : public KisColorTransformationFilter
{
public:
    using KisColorTransformationFilter::KisColorTransformationFilter;

    KoColorTransformation *createTransformation(const KoColorSpace *, const KisFilterConfigurationSP) const override;
};

} // namespace

class KisColorTransformationFilterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterTypeConstructionAndLifetimeSchemaRemainStable();
    void filterProcessingAndConfigurationSignaturesRemainStable();
    void configurationTypeConstructionAndLifetimeSchemaRemainStable();
    void configurationCloneAndMutationSignaturesRemainStable();
    void configurationTransformationCacheSignaturesRemainStable();
    void registryTypeLifetimeAndQuerySchemaRemainStable();
    void registryMutationAndNotificationSignaturesRemainStable();
};

void KisColorTransformationFilterSchemaContractTest::filterTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Filter = KisColorTransformationFilter;

    static_assert(std::is_class_v<Filter>);
    static_assert(std::is_base_of_v<KisFilter, Filter>);
    static_assert(std::is_constructible_v<ColorFilterConstructionProbe, const KoID &, const KoID &, const QString &>);
    static_assert(std::has_virtual_destructor_v<Filter>);
}

void KisColorTransformationFilterSchemaContractTest::filterProcessingAndConfigurationSignaturesRemainStable()
{
    using Filter = KisColorTransformationFilter;

    ASSERT_SIGNATURE(Filter,
                     processImpl,
                     void (Filter::*)(KisPaintDeviceSP, const QRect &, const KisFilterConfigurationSP, KoUpdater *)
                         const);
    ASSERT_SIGNATURE(Filter,
                     createTransformation,
                     KoColorTransformation * (Filter::*)(const KoColorSpace *, const KisFilterConfigurationSP) const);
    ASSERT_SIGNATURE(Filter, factoryConfiguration, KisFilterConfigurationSP (Filter::*)(KisResourcesInterfaceSP) const);
}

void KisColorTransformationFilterSchemaContractTest::configurationTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Configuration = KisColorTransformationConfiguration;

    static_assert(std::is_same_v<KisColorTransformationConfigurationSP, KisSharedPtr<Configuration>>);
    static_assert(std::is_class_v<Configuration>);
    static_assert(std::is_base_of_v<KisFilterConfiguration, Configuration>);
    static_assert(std::is_constructible_v<Configuration, const QString &, qint32, KisResourcesInterfaceSP>);
    static_assert(std::is_constructible_v<Configuration, const Configuration &>);
    static_assert(std::has_virtual_destructor_v<Configuration>);
}

void KisColorTransformationFilterSchemaContractTest::configurationCloneAndMutationSignaturesRemainStable()
{
    using Configuration = KisColorTransformationConfiguration;

    ASSERT_SIGNATURE(Configuration, clone, KisFilterConfigurationSP (Configuration::*)() const);
    ASSERT_SIGNATURE(Configuration, setProperty, void (Configuration::*)(const QString &, const QVariant &));
}

void KisColorTransformationFilterSchemaContractTest::configurationTransformationCacheSignaturesRemainStable()
{
    using Configuration = KisColorTransformationConfiguration;

    ASSERT_SIGNATURE(Configuration,
                     colorTransformation,
                     KoColorTransformation
                         * (Configuration::*)(const KoColorSpace *, const KisColorTransformationFilter *) const);
    ASSERT_SIGNATURE(Configuration, invalidateColorTransformationCache, void (Configuration::*)());
}

void KisColorTransformationFilterSchemaContractTest::registryTypeLifetimeAndQuerySchemaRemainStable()
{
    using Registry = KisFilterRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<QObject, Registry>);
    static_assert(std::is_base_of_v<KoGenericRegistry<KisFilterSP>, Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    ASSERT_SIGNATURE(Registry, instance, Registry * (*)());
    ASSERT_SIGNATURE(Registry, fallbackFilter, KisFilterSP (Registry::*)() const);
}

void KisColorTransformationFilterSchemaContractTest::registryMutationAndNotificationSignaturesRemainStable()
{
    using Registry = KisFilterRegistry;

    ASSERT_SIGNATURE(Registry, add, void (Registry::*)(KisFilterSP));
    ASSERT_SIGNATURE(Registry, add, void (Registry::*)(const QString &, KisFilterSP));
    ASSERT_SIGNATURE(Registry, filterAdded, void (Registry::*)(QString));
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisColorTransformationFilterSchemaContractTest)

#include "KisColorTransformationFilterSchemaContractTest.moc"
