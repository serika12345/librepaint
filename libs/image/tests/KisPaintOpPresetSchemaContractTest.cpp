/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paintop_preset.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PAINTOP_PRESET_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpPreset::method)), signature>)
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

QTEST_GUILESS_MAIN(KisPaintOpPresetSchemaContractTest)

#include "KisPaintOpPresetSchemaContractTest.moc"
