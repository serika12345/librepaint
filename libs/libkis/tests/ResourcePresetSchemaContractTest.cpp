/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Preset.h>
#include <PresetChooser.h>
#include <Resource.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_RESOURCE_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Resource::method)), signature>)

#define ASSERT_PRESET_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Preset::method)), signature>)

#define ASSERT_PRESET_CHOOSER_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PresetChooser::method)), signature>)

} // namespace

class ResourcePresetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceTypeLifetimeAndValueSemanticsSchemaRemainStable();
    void resourceIdentityAndPresentationSignaturesRemainStable();
    void presetTypeLifetimeAndPersistenceSchemaRemainStable();
    void presetChooserTypeLifetimeAndSelectionSchemaRemainStable();
    void presetChooserNotificationSignaturesRemainStable();
};

void ResourcePresetSchemaContractTest::resourceTypeLifetimeAndValueSemanticsSchemaRemainStable()
{
    static_assert(std::is_class_v<Resource>);
    static_assert(std::is_base_of_v<QObject, Resource>);
    static_assert(
        std::is_constructible_v<Resource, int, const QString &, const QString &, const QString &, const QImage &>);
    static_assert(std::is_constructible_v<Resource,
                                          int,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QImage &,
                                          QObject *>);
    static_assert(std::is_constructible_v<Resource, KoResourceSP, const QString &>);
    static_assert(std::is_constructible_v<Resource, KoResourceSP, const QString &, QObject *>);
    static_assert(std::is_copy_constructible_v<Resource>);
    static_assert(std::has_virtual_destructor_v<Resource>);
    ASSERT_RESOURCE_SIGNATURE(operator=, Resource (Resource::*)(const Resource &));
    ASSERT_RESOURCE_SIGNATURE(operator==, bool (Resource::*)(const Resource &) const);
    ASSERT_RESOURCE_SIGNATURE(operator!=, bool (Resource::*)(const Resource &) const);
}

void ResourcePresetSchemaContractTest::resourceIdentityAndPresentationSignaturesRemainStable()
{
    ASSERT_RESOURCE_SIGNATURE(filename, QString (Resource::*)() const);
    ASSERT_RESOURCE_SIGNATURE(image, QImage (Resource::*)() const);
    ASSERT_RESOURCE_SIGNATURE(name, QString (Resource::*)() const);
    ASSERT_RESOURCE_SIGNATURE(setImage, void (Resource::*)(QImage));
    ASSERT_RESOURCE_SIGNATURE(setName, void (Resource::*)(QString));
    ASSERT_RESOURCE_SIGNATURE(type, QString (Resource::*)() const);
}

void ResourcePresetSchemaContractTest::presetTypeLifetimeAndPersistenceSchemaRemainStable()
{
    static_assert(std::is_class_v<Preset>);
    static_assert(std::is_base_of_v<QObject, Preset>);
    static_assert(std::is_constructible_v<Preset, Resource *>);
    static_assert(std::has_virtual_destructor_v<Preset>);
    ASSERT_PRESET_SIGNATURE(fromXML, void (Preset::*)(const QString &));
    ASSERT_PRESET_SIGNATURE(toXML, QString (Preset::*)() const);
}

void ResourcePresetSchemaContractTest::presetChooserTypeLifetimeAndSelectionSchemaRemainStable()
{
    static_assert(std::is_class_v<PresetChooser>);
    static_assert(std::is_base_of_v<KisPresetChooser, PresetChooser>);
    static_assert(std::is_default_constructible_v<PresetChooser>);
    static_assert(std::is_constructible_v<PresetChooser, QWidget *>);
    static_assert(std::has_virtual_destructor_v<PresetChooser>);
    ASSERT_PRESET_CHOOSER_SIGNATURE(currentPreset, Resource * (PresetChooser::*)() const);
    ASSERT_PRESET_CHOOSER_SIGNATURE(setCurrentPreset, void (PresetChooser::*)(Resource *));
}

void ResourcePresetSchemaContractTest::presetChooserNotificationSignaturesRemainStable()
{
    ASSERT_PRESET_CHOOSER_SIGNATURE(presetClicked, void (PresetChooser::*)(Resource));
    ASSERT_PRESET_CHOOSER_SIGNATURE(presetSelected, void (PresetChooser::*)(Resource));
}

QTEST_APPLESS_MAIN(ResourcePresetSchemaContractTest)

#include "ResourcePresetSchemaContractTest.moc"
