/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_locked_properties.h"
#include "brushengine/kis_locked_properties_proxy.h"
#include "brushengine/kis_locked_properties_server.h"
#include "brushengine/kis_paintop_preset.h"
#define HAVE_THREADED_TEXT_RENDERING_WORKAROUND
#include "brushengine/kis_paintop_registry.h"
#undef HAVE_THREADED_TEXT_RENDERING_WORKAROUND

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PAINTOP_PRESET_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpPreset::method)), signature>)
#define ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLockedPropertiesServer::method)), signature>)
#define ASSERT_LOCKED_PROPERTIES_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLockedProperties::method)), signature>)
#define ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLockedPropertiesProxy::method)), signature>)
#define ASSERT_PAINTOP_REGISTRY_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpRegistry::method)), signature>)
} // namespace

class KisPaintOpPresetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paintOpPresetTypeLifetimeAndUpdateSuppressionSchemaRemainStable();
    void paintOpPresetIdentitySettingsAndSerializationSignaturesRemainStable();
    void paintOpPresetCloneAndUpdateInterfaceSignaturesRemainStable();
    void paintOpPresetMaskingAndResourceInterfaceSignaturesRemainStable();
    void paintOpPresetResourceSnapshotAndCacheSignaturesRemainStable();
    void lockedPropertiesServerTypeLifetimeAndAccessSchemaRemainStable();
    void lockedPropertiesServerCollectionSignaturesRemainStable();
    void lockedPropertiesServerSourceAndQuerySignaturesRemainStable();
    void lockedPropertiesServerProxyCreationSignaturesRemainStable();
    void lockedPropertiesTypeConstructionAndLifetimeSchemaRemainStable();
    void lockedPropertiesMutationSignaturesRemainStable();
    void lockedPropertiesObservationSignaturesRemainStable();
    void lockedPropertiesProxyTypeConstructionAndLifetimeSchemaRemainStable();
    void lockedPropertiesProxyAccessSignaturesRemainStable();
    void paintOpRegistryTypeLifetimeAndGlobalAccessSchemaRemainStable();
    void paintOpRegistryPaintOpCreationSignaturesRemainStable();
    void paintOpRegistryConfigurationAndPresentationSignaturesRemainStable();
    void paintOpRegistryPreinitializationSignatureRemainsStable();
};

void KisPaintOpPresetSchemaContractTest::paintOpPresetTypeLifetimeAndUpdateSuppressionSchemaRemainStable()
{
    using Preset = KisPaintOpPreset;
    using UpdatedPostponer = Preset::UpdatedPostponer;

    static_assert(std::is_class_v<Preset>);
    static_assert(std::is_default_constructible_v<Preset>);
    static_assert(std::is_constructible_v<Preset, const QString &>);
    static_assert(std::is_copy_constructible_v<Preset>);
    static_assert(std::is_destructible_v<Preset>);
    static_assert(std::has_virtual_destructor_v<Preset>);
    static_assert(!std::is_copy_assignable_v<Preset>);

    static_assert(std::is_class_v<UpdatedPostponer>);
    static_assert(std::is_constructible_v<UpdatedPostponer, KisPaintOpPresetSP>);
    static_assert(std::is_destructible_v<UpdatedPostponer>);
}

void KisPaintOpPresetSchemaContractTest::paintOpPresetIdentitySettingsAndSerializationSignaturesRemainStable()
{
    using ResourceType = QPair<QString, QString>;

    ASSERT_PAINTOP_PRESET_SIGNATURE(setPaintOp, void (KisPaintOpPreset::*)(const KoID &));
    ASSERT_PAINTOP_PRESET_SIGNATURE(paintOp, KoID (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(name, QString (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(setSettings, void (KisPaintOpPreset::*)(KisPaintOpSettingsSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(settings, KisPaintOpSettingsSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(loadFromDevice, bool (KisPaintOpPreset::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(saveToDevice, bool (KisPaintOpPreset::*)(QIODevice *) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(resourceType, ResourceType (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(toXML, void (KisPaintOpPreset::*)(QDomDocument &, QDomElement &) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(fromXML, void (KisPaintOpPreset::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(removable, bool (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(defaultFileExtension, QString (KisPaintOpPreset::*)() const);
}

void KisPaintOpPresetSchemaContractTest::paintOpPresetCloneAndUpdateInterfaceSignaturesRemainStable()
{
    ASSERT_PAINTOP_PRESET_SIGNATURE(clone, KoResourceSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(updateLinkedResourcesMetaData, void (KisPaintOpPreset::*)());
    ASSERT_PAINTOP_PRESET_SIGNATURE(updateProxy, QPointer<KisPaintOpPresetUpdateProxy> (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(updateProxyNoCreate,
                                    QPointer<KisPaintOpPresetUpdateProxy> (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(uniformProperties, QList<KisUniformPaintOpPropertySP> (KisPaintOpPreset::*)());
}

void KisPaintOpPresetSchemaContractTest::paintOpPresetMaskingAndResourceInterfaceSignaturesRemainStable()
{
    ASSERT_PAINTOP_PRESET_SIGNATURE(hasMaskingPreset, bool (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(createMaskingPreset, KisPaintOpPresetSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(resourcesInterface, KisResourcesInterfaceSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(setResourcesInterface, void (KisPaintOpPreset::*)(KisResourcesInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(canvasResourcesInterface,
                                    KoCanvasResourcesInterfaceSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(setCanvasResourcesInterface,
                                    void (KisPaintOpPreset::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(requiredCanvasResources, QList<int> (KisPaintOpPreset::*)() const);
}

void KisPaintOpPresetSchemaContractTest::paintOpPresetResourceSnapshotAndCacheSignaturesRemainStable()
{
    ASSERT_PAINTOP_PRESET_SIGNATURE(hasLocalResourcesSnapshot, bool (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(cloneWithResourcesSnapshot,
                                    KisPaintOpPresetSP (KisPaintOpPreset::*)(KisResourcesInterfaceSP,
                                                                             KoCanvasResourcesInterfaceSP,
                                                                             KoResourceCacheInterfaceSP) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(linkedResources,
                                    QList<KoResourceLoadResult> (KisPaintOpPreset::*)(KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(embeddedResources,
                                    QList<KoResourceLoadResult> (KisPaintOpPreset::*)(KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(sideLoadedResources,
                                    QList<KoResourceLoadResult> (KisPaintOpPreset::*)(KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(clearSideLoadedResources, void (KisPaintOpPreset::*)());
    ASSERT_PAINTOP_PRESET_SIGNATURE(setResourceCacheInterface, void (KisPaintOpPreset::*)(KoResourceCacheInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(resourceCacheInterface, KoResourceCacheInterfaceSP (KisPaintOpPreset::*)() const);
    ASSERT_PAINTOP_PRESET_SIGNATURE(regenerateResourceCache, void (KisPaintOpPreset::*)(KoResourceCacheInterfaceSP));
    ASSERT_PAINTOP_PRESET_SIGNATURE(sanityCheckResourceCacheIsValid,
                                    bool (KisPaintOpPreset::*)(KoResourceCacheInterfaceSP) const);
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesServerTypeLifetimeAndAccessSchemaRemainStable()
{
    using Server = KisLockedPropertiesServer;

    static_assert(std::is_class_v<Server>);
    static_assert(std::is_base_of_v<QObject, Server>);
    static_assert(std::is_default_constructible_v<Server>);
    static_assert(std::has_virtual_destructor_v<Server>);
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(instance, Server * (*)());

    QVERIFY(true);
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesServerCollectionSignaturesRemainStable()
{
    using Server = KisLockedPropertiesServer;

    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(lockedProperties, KisLockedPropertiesSP (Server::*)());
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(addToLockedProperties, void (Server::*)(KisPropertiesConfigurationSP));
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(removeFromLockedProperties,
                                              void (Server::*)(KisPropertiesConfigurationSP));
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesServerSourceAndQuerySignaturesRemainStable()
{
    using Server = KisLockedPropertiesServer;

    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(setPropertiesFromLocked, void (Server::*)(bool));
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(propertiesFromLocked, bool (Server::*)());
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(hasProperty, bool (Server::*)(const QString &));
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesServerProxyCreationSignaturesRemainStable()
{
    using Server = KisLockedPropertiesServer;

    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(createLockedPropertiesProxy,
                                              KisLockedPropertiesProxySP (Server::*)(KisPropertiesConfiguration *));
    ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE(createLockedPropertiesProxy,
                                              KisLockedPropertiesProxySP (Server::*)(KisPropertiesConfigurationSP));
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Locked = KisLockedProperties;

    static_assert(std::is_class_v<Locked>);
    static_assert(std::is_base_of_v<KisShared, Locked>);
    static_assert(std::is_default_constructible_v<Locked>);
    static_assert(std::is_destructible_v<Locked>);
    static_assert(!std::is_copy_constructible_v<Locked>);

    QVERIFY(true);
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesMutationSignaturesRemainStable()
{
    using Locked = KisLockedProperties;

    ASSERT_LOCKED_PROPERTIES_SIGNATURE(addToLockedProperties, void (Locked::*)(KisPropertiesConfigurationSP));
    ASSERT_LOCKED_PROPERTIES_SIGNATURE(addToLockedProperties, void (Locked::*)(const KisPropertiesConfiguration *));
    ASSERT_LOCKED_PROPERTIES_SIGNATURE(removeFromLockedProperties, void (Locked::*)(KisPropertiesConfigurationSP));
    ASSERT_LOCKED_PROPERTIES_SIGNATURE(removeFromLockedProperties,
                                       void (Locked::*)(const KisPropertiesConfiguration *));
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesObservationSignaturesRemainStable()
{
    using Locked = KisLockedProperties;

    ASSERT_LOCKED_PROPERTIES_SIGNATURE(hasProperty, bool (Locked::*)(const QString &));
    ASSERT_LOCKED_PROPERTIES_SIGNATURE(lockedProperties, KisPropertiesConfigurationSP (Locked::*)());
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesProxyTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Proxy = KisLockedPropertiesProxy;

    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<KisPropertiesConfiguration, Proxy>);
    static_assert(std::is_constructible_v<Proxy, KisPropertiesConfiguration *, KisLockedPropertiesSP>);
    static_assert(std::has_virtual_destructor_v<Proxy>);

    QVERIFY(true);
}

void KisPaintOpPresetSchemaContractTest::lockedPropertiesProxyAccessSignaturesRemainStable()
{
    using Proxy = KisLockedPropertiesProxy;

    ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(getProperty, QVariant (Proxy::*)(const QString &) const);
    ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(setProperty, void (Proxy::*)(const QString &, const QVariant &));
    ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(hasProperty, bool (Proxy::*)(const QString &) const);
    ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(getPropertiesKeys, QList<QString> (Proxy::*)() const);
    ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE(dump, void (Proxy::*)() const);
}

void KisPaintOpPresetSchemaContractTest::paintOpRegistryTypeLifetimeAndGlobalAccessSchemaRemainStable()
{
    using Registry = KisPaintOpRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<QObject, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(instance, Registry * (*)());
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(registerResourceLoader, void (*)(KisResourceLoaderRegistry &));

    QVERIFY(true);
}

void KisPaintOpPresetSchemaContractTest::paintOpRegistryPaintOpCreationSignaturesRemainStable()
{
    using Registry = KisPaintOpRegistry;

    ASSERT_PAINTOP_REGISTRY_SIGNATURE(
        paintOp,
        KisPaintOp * (Registry::*)(const KisPaintOpPresetSP, KisPainter *, KisNodeSP, KisImageSP) const);
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(createInterstrokeDataFactory,
                                      KisInterstrokeDataFactory * (Registry::*)(KisPaintOpPresetSP) const);
}

void KisPaintOpPresetSchemaContractTest::paintOpRegistryConfigurationAndPresentationSignaturesRemainStable()
{
    using Registry = KisPaintOpRegistry;

    ASSERT_PAINTOP_REGISTRY_SIGNATURE(createSettings,
                                      KisPaintOpSettingsSP (Registry::*)(const KoID &, KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(defaultPreset,
                                      KisPaintOpPresetSP (Registry::*)(const KoID &, KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(icon, QIcon (Registry::*)(const KoID &) const);
    ASSERT_PAINTOP_REGISTRY_SIGNATURE(listKeys, QList<KoID> (Registry::*)() const);
}

void KisPaintOpPresetSchemaContractTest::paintOpRegistryPreinitializationSignatureRemainsStable()
{
    using Registry = KisPaintOpRegistry;

    ASSERT_PAINTOP_REGISTRY_SIGNATURE(preinitializePaintOpIfNeeded, void (Registry::*)(const KisPaintOpPresetSP));
}

#undef ASSERT_PAINTOP_REGISTRY_SIGNATURE
#undef ASSERT_LOCKED_PROPERTIES_PROXY_SIGNATURE
#undef ASSERT_LOCKED_PROPERTIES_SIGNATURE
#undef ASSERT_LOCKED_PROPERTIES_SERVER_SIGNATURE

QTEST_GUILESS_MAIN(KisPaintOpPresetSchemaContractTest)

#include "KisPaintOpPresetSchemaContractTest.moc"
