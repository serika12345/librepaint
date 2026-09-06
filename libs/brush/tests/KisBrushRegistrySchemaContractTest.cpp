/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisBrushServerProvider.h>
#include <kis_brush_registry.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_BRUSH_REGISTRY_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBrushRegistry::method)), signature>)
#define ASSERT_BRUSH_SERVER_PROVIDER_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBrushServerProvider::method)), signature>)
} // namespace

class KisBrushRegistrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void registryTypeLifetimeAndSingletonSchemaRemainStable();
    void registryLoaderRegistrationSchemaRemainsStable();
    void registryBrushCreationSchemaRemainsStable();
    void registryModelAndSerializationSchemaRemainStable();
    void serverProviderSchemaRemainsStable();
};

void KisBrushRegistrySchemaContractTest::registryTypeLifetimeAndSingletonSchemaRemainStable()
{
    static_assert(std::is_class_v<KisBrushRegistry>);
    static_assert(std::is_default_constructible_v<KisBrushRegistry>);
    static_assert(std::has_virtual_destructor_v<KisBrushRegistry>);
    ASSERT_BRUSH_REGISTRY_SIGNATURE(instance, KisBrushRegistry * (*)());
}

void KisBrushRegistrySchemaContractTest::registryLoaderRegistrationSchemaRemainsStable()
{
    ASSERT_BRUSH_REGISTRY_SIGNATURE(registerResourceLoaders, void (*)(KisResourceLoaderRegistry &));
    ASSERT_BRUSH_REGISTRY_SIGNATURE(registerResourceCacheFixup, void (*)(KisResourceLoaderRegistry &));
}

void KisBrushRegistrySchemaContractTest::registryBrushCreationSchemaRemainsStable()
{
    ASSERT_BRUSH_REGISTRY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisBrushRegistry::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_BRUSH_REGISTRY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisBrushRegistry::*)(const KisBrushModel::BrushData &, KisResourcesInterfaceSP));
}

void KisBrushRegistrySchemaContractTest::registryModelAndSerializationSchemaRemainStable()
{
    ASSERT_BRUSH_REGISTRY_SIGNATURE(
        createBrushModel,
        std::optional<KisBrushModel::BrushData> (KisBrushRegistry::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_BRUSH_REGISTRY_SIGNATURE(
        toXML,
        void (KisBrushRegistry::*)(QDomDocument &, QDomElement &, const KisBrushModel::BrushData &));
}

void KisBrushRegistrySchemaContractTest::serverProviderSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisBrushServerProvider>);
    static_assert(std::is_default_constructible_v<KisBrushServerProvider>);
    static_assert(std::has_virtual_destructor_v<KisBrushServerProvider>);
    ASSERT_BRUSH_SERVER_PROVIDER_SIGNATURE(instance, KisBrushServerProvider * (*)());
    ASSERT_BRUSH_SERVER_PROVIDER_SIGNATURE(brushServer, KoResourceServer<KisBrush> * (KisBrushServerProvider::*)());
}

QTEST_GUILESS_MAIN(KisBrushRegistrySchemaContractTest)

#include "KisBrushRegistrySchemaContractTest.moc"
