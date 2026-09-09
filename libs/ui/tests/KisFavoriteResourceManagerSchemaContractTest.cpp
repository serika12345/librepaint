/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/kis_favorite_resource_manager.h>
#include <tool/kis_paintop_box.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(method, signature)                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisFavoriteResourceManager::method)), signature>)
} // namespace

class KisFavoriteResourceManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void favoriteResourceManagerTypeLifetimeAndServerSchemaRemainStable();
    void favoriteResourceManagerPresetAndTagSignaturesRemainStable();
    void favoriteResourceManagerRecentAndBackgroundColorSignaturesRemainStable();
    void favoriteResourceManagerResourceAndPaintopSignaturesRemainStable();
    void favoriteResourceManagerPaletteNotificationSignaturesRemainStable();
    void paintopBoxTypeLifetimeAndManagerSchemaRemainStable();
    void paintopBoxResourceSignaturesRemainStable();
    void paintopBoxConfigurationInputSignaturesRemainStable();
    void paintopBoxPresetAndCanvasNotificationSignaturesRemainStable();
};

void KisFavoriteResourceManagerSchemaContractTest::favoriteResourceManagerTypeLifetimeAndServerSchemaRemainStable()
{
    using Manager = KisFavoriteResourceManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_constructible_v<Manager, KisPaintopBox *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(unsetResourceServer, void (Manager::*)());
}

void KisFavoriteResourceManagerSchemaContractTest::favoriteResourceManagerPresetAndTagSignaturesRemainStable()
{
    using Manager = KisFavoriteResourceManager;

    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(favoritePresetImages, QList<QImage> (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(favoritePresetNamesList, QVector<QString> (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(setCurrentTag, void (Manager::*)(KisTagSP));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(numFavoritePresets, int (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(updateFavoritePresets, void (Manager::*)());
}

// clang-format off
void KisFavoriteResourceManagerSchemaContractTest::favoriteResourceManagerRecentAndBackgroundColorSignaturesRemainStable()
// clang-format on
{
    using Manager = KisFavoriteResourceManager;

    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(bgColor, KoColor (Manager::*)() const);
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(recentColorAt, KoColor (Manager::*)(int));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(recentColorsTotal, int (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotAddRecentColor, void (Manager::*)(const KoColor &));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotClearHistory, void (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotSetBGColor, void (Manager::*)(KoColor));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotUpdateRecentColor, void (Manager::*)(int));
}

void KisFavoriteResourceManagerSchemaContractTest::favoriteResourceManagerResourceAndPaintopSignaturesRemainStable()
{
    using Manager = KisFavoriteResourceManager;
    using Preset = QSharedPointer<KisPaintOpPreset>;

    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(removingResource, void (Manager::*)(Preset));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(resourceAdded, void (Manager::*)(Preset));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(resourceChanged, void (Manager::*)(Preset));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotChangeActivePaintop, void (Manager::*)(int));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(slotChangeFGColorSelector, void (Manager::*)(KoColor));
}

void KisFavoriteResourceManagerSchemaContractTest::favoriteResourceManagerPaletteNotificationSignaturesRemainStable()
{
    using Manager = KisFavoriteResourceManager;

    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(hidePalettes, void (Manager::*)());
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(setSelectedColor, void (Manager::*)(int));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(sigChangeFGColorSelector, void (Manager::*)(const KoColor &));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(sigSetBGColor, void (Manager::*)(const KoColor &));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(sigSetFGColor, void (Manager::*)(const KoColor &));
    ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE(updatePalettes, void (Manager::*)());
}

void KisFavoriteResourceManagerSchemaContractTest::paintopBoxTypeLifetimeAndManagerSchemaRemainStable()
{
    using Box = KisPaintopBox;

    static_assert(std::is_class_v<Box>);
    static_assert(std::is_constructible_v<Box, KisViewManager *, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Box>);
    static_assert(std::is_same_v<decltype(&Box::favoriteResourcesManager), KisFavoriteResourceManager *(Box::*)()>);
}

void KisFavoriteResourceManagerSchemaContractTest::paintopBoxResourceSignaturesRemainStable()
{
    using Box = KisPaintopBox;

    static_assert(std::is_same_v<decltype(&Box::restoreResource), void (Box::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Box::resourceSelected), void (Box::*)(KoResourceSP)>);
}

void KisFavoriteResourceManagerSchemaContractTest::paintopBoxConfigurationInputSignaturesRemainStable()
{
    using Box = KisPaintopBox;

    static_assert(std::is_same_v<decltype(&Box::newOptionWidgets), void (Box::*)(const QList<QPointer<QWidget>> &)>);
    static_assert(std::is_same_v<decltype(&Box::slotColorSpaceChanged), void (Box::*)(const KoColorSpace *)>);
    static_assert(std::is_same_v<decltype(&Box::slotInputDeviceChanged), void (Box::*)(const KoInputDevice &)>);
}

void KisFavoriteResourceManagerSchemaContractTest::paintopBoxPresetAndCanvasNotificationSignaturesRemainStable()
{
    using Box = KisPaintopBox;

    static_assert(std::is_same_v<decltype(&Box::slotToggleEraserPreset), void (Box::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Box::slotSelectEraserPreset), void (Box::*)()>);
    static_assert(std::is_same_v<decltype(&Box::slotSelectBrushPreset), void (Box::*)()>);
    static_assert(
        std::is_same_v<decltype(&Box::slotCanvasResourceChangeAttempted), void (Box::*)(int, const QVariant &)>);
    static_assert(std::is_same_v<decltype(&Box::slotCanvasResourceChanged), void (Box::*)(int, const QVariant &)>);
    static_assert(std::is_same_v<decltype(&Box::slotCreatePresetFromScratch), void (Box::*)(QString)>);
}

#undef ASSERT_FAVORITE_RESOURCE_MANAGER_SIGNATURE

QTEST_APPLESS_MAIN(KisFavoriteResourceManagerSchemaContractTest)

#include "KisFavoriteResourceManagerSchemaContractTest.moc"
