/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "resources/kis_derived_resources.h"

#define ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(Type)                                                                 \
    static_assert(std::is_class_v<Type>);                                                                              \
    static_assert(std::is_base_of_v<KoDerivedResourceConverter, Type>);                                                \
    static_assert(std::is_default_constructible_v<Type>);                                                              \
    static_assert(std::is_same_v<decltype(&Type::fromSource), QVariant (Type::*)(const QVariant &)>);                  \
    static_assert(std::is_same_v<decltype(&Type::toSource), QVariant (Type::*)(const QVariant &, const QVariant &)>)

class KisDerivedResourcesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void presetUpdateMediatorSchemaRemainsStable();
    void compositeAndOpacityConverterSchemaRemainsStable();
    void paintingDynamicsConverterSchemaRemainsStable();
    void brushGeometryConverterSchemaRemainsStable();
    void lodAndEraserConverterSchemaRemainsStable();
};

void KisDerivedResourcesSchemaContractTest::presetUpdateMediatorSchemaRemainsStable()
{
    using ConnectResource = void (KisPresetUpdateMediator::*)(QVariant);

    static_assert(std::is_class_v<KisPresetUpdateMediator>);
    static_assert(std::is_base_of_v<KoResourceUpdateMediator, KisPresetUpdateMediator>);
    static_assert(std::is_default_constructible_v<KisPresetUpdateMediator>);
    static_assert(std::has_virtual_destructor_v<KisPresetUpdateMediator>);
    static_assert(std::is_same_v<decltype(&KisPresetUpdateMediator::connectResource), ConnectResource>);

    QVERIFY(true);
}

void KisDerivedResourcesSchemaContractTest::compositeAndOpacityConverterSchemaRemainsStable()
{
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisCompositeOpResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisEffectiveCompositeOpResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisOpacityToPresetOpacityResourceConverter);

    QVERIFY(true);
}

void KisDerivedResourcesSchemaContractTest::paintingDynamicsConverterSchemaRemainsStable()
{
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisFlowResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisFadeResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisScatterResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisSizeResourceConverter);

    QVERIFY(true);
}

void KisDerivedResourcesSchemaContractTest::brushGeometryConverterSchemaRemainsStable()
{
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisBrushRotationResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisPatternSizeResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisBrushNameResourceConverter);

    QVERIFY(true);
}

void KisDerivedResourcesSchemaContractTest::lodAndEraserConverterSchemaRemainsStable()
{
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisLodAvailabilityResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisLodSizeThresholdResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisLodSizeThresholdSupportedResourceConverter);
    ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA(KisEraserModeResourceConverter);

    QVERIFY(true);
}

#undef ASSERT_DERIVED_RESOURCE_CONVERTER_SCHEMA

QTEST_MAIN(KisDerivedResourcesSchemaContractTest)

#include "KisDerivedResourcesSchemaContractTest.moc"
