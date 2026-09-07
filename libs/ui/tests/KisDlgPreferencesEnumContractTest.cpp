/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <dialogs/kis_dlg_preferences.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_GENERAL_TAB_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&GeneralTab::method)), signature>)

#define ASSERT_PREFERENCE_MEMBER(type, member) static_assert(std::is_member_object_pointer_v<decltype(&type::member)>)
#define ASSERT_PREFERENCE_CALLABLE(type, method)                                                                       \
    static_assert(std::is_pointer_v<decltype(&type::method)>                                                           \
                  || std::is_member_function_pointer_v<decltype(&type::method)>)
} // namespace

class KisDlgPreferencesEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preferredSpaceValuesRemainStable();
    void canvasSurfaceAliasesRemainStable();
    void pageValuesRemainStable();
    void generalTabValuesRemainStable();
    void colorTabValuesRemainStable();
    void displayTabValuesRemainStable();
    void performanceTabValuesRemainStable();
    void generalTabTypeAndStateSchemaRemainsStable();
    void generalTabCursorAndSamplerSchemaRemainsStable();
    void generalTabSessionAndDocumentSchemaRemainsStable();
    void generalTabInteractionAndNavigationSchemaRemainsStable();
    void generalTabAnimationSchemaRemainsStable();
    void importExportOptionsSchemaRemainsStable();
    void generatedSettingsWidgetSchemaRemainsStable();
    void colorSettingsStructureSchemaRemainsStable();
    void shortcutAndTabletStructureSchemaRemainsStable();
    void performanceAndDisplayStructureSchemaRemainsStable();
    void fullscreenPopupAndDialogStructureSchemaRemainsStable();
};

void KisDlgPreferencesEnumContractTest::preferredSpaceValuesRemainStable()
{
    QCOMPARE(int(ColorSettingsTab::PreferredSpace), 0);
    QCOMPARE(int(ColorSettingsTab::MasteringSpace), 1);
}

void KisDlgPreferencesEnumContractTest::canvasSurfaceAliasesRemainStable()
{
    QVERIFY((std::is_same_v<ColorSettingsTab::CanvasSurfaceMode, KisConfig::CanvasSurfaceMode>));
    QVERIFY((std::is_same_v<ColorSettingsTab::CanvasSurfaceBitDepthMode, KisConfig::CanvasSurfaceBitDepthMode>));
}

void KisDlgPreferencesEnumContractTest::pageValuesRemainStable()
{
    const std::array<KisDlgPreferences::Page, 9> pages{{
        KisDlgPreferences::General,
        KisDlgPreferences::Shortucts,
        KisDlgPreferences::Color,
        KisDlgPreferences::Performance,
        KisDlgPreferences::Display,
        KisDlgPreferences::Tablet,
        KisDlgPreferences::Fullscreen,
        KisDlgPreferences::Input,
        KisDlgPreferences::PopupPalette,
    }};

    for (std::size_t index = 0; index < pages.size(); ++index) {
        QCOMPARE(int(pages[index]), int(index));
    }
}

void KisDlgPreferencesEnumContractTest::generalTabValuesRemainStable()
{
    const std::array<KisDlgPreferences::GeneralTabs, 8> tabs{{
        KisDlgPreferences::File,
        KisDlgPreferences::Pasting,
        KisDlgPreferences::Window,
        KisDlgPreferences::Cursor,
        KisDlgPreferences::Tools,
        KisDlgPreferences::Animation,
        KisDlgPreferences::Resources,
        KisDlgPreferences::MiscellaneousGeneral,
    }};

    for (std::size_t index = 0; index < tabs.size(); ++index) {
        QCOMPARE(int(tabs[index]), int(index));
    }
}

void KisDlgPreferencesEnumContractTest::colorTabValuesRemainStable()
{
    const std::array<KisDlgPreferences::ColorTabs, 3> tabs{{
        KisDlgPreferences::GeneralColor,
        KisDlgPreferences::DisplayTab,
        KisDlgPreferences::SoftProofing,
    }};

    for (std::size_t index = 0; index < tabs.size(); ++index) {
        QCOMPARE(int(tabs[index]), int(index));
    }
}

void KisDlgPreferencesEnumContractTest::displayTabValuesRemainStable()
{
    const std::array<KisDlgPreferences::DisplayTabs, 4> tabs{{
        KisDlgPreferences::CanvasAcceleration,
        KisDlgPreferences::HDR,
        KisDlgPreferences::CanvasDecoration,
        KisDlgPreferences::MiscellaneousDisplay,
    }};

    for (std::size_t index = 0; index < tabs.size(); ++index) {
        QCOMPARE(int(tabs[index]), int(index));
    }
}

void KisDlgPreferencesEnumContractTest::performanceTabValuesRemainStable()
{
    const std::array<KisDlgPreferences::PerformaceTabs, 4> tabs{{
        KisDlgPreferences::GeneralPerformance,
        KisDlgPreferences::Advanced,
        KisDlgPreferences::AnimationCache,
        KisDlgPreferences::InstantPreview,
    }};

    for (std::size_t index = 0; index < tabs.size(); ++index) {
        QCOMPARE(int(tabs[index]), int(index));
    }
}

void KisDlgPreferencesEnumContractTest::generalTabTypeAndStateSchemaRemainsStable()
{
    static_assert(std::is_class_v<GeneralTab>);
    static_assert(
        std::is_constructible_v<GeneralTab, QWidget *, const KisImportExportPreferenceOptions &, const char *>);
    static_assert(std::is_constructible_v<GeneralTab, QWidget *, const KisImportExportPreferenceOptions &>);
    ASSERT_GENERAL_TAB_SIGNATURE(setDefault, void (GeneralTab::*)());
    static_assert(std::is_same_v<decltype(&GeneralTab::m_pasteFormatGroup), QButtonGroup GeneralTab::*>);
    static_assert(std::is_same_v<decltype(&GeneralTab::m_cumulativeUndoData), KisCumulativeUndoData GeneralTab::*>);
    ASSERT_GENERAL_TAB_SIGNATURE(mdiMode, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(forcedFontDpi, int (GeneralTab::*)());
}

void KisDlgPreferencesEnumContractTest::generalTabCursorAndSamplerSchemaRemainsStable()
{
    ASSERT_GENERAL_TAB_SIGNATURE(cursorStyle, CursorStyle (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(outlineStyle, OutlineStyle (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(eraserCursorStyle, CursorStyle (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(eraserOutlineStyle, OutlineStyle (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(colorSamplerPreviewStyle, KisConfig::ColorSamplerPreviewStyle (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(colorSamplerPreviewCircleDiameter, int (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(colorSamplerPreviewCircleThickness, qreal (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(colorSamplerPreviewCircleOutlineEnabled, bool (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(colorSamplerPreviewCircleExtraCirclesEnabled, bool (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(setColorSamplerPreviewStyleItems, void (*)(QComboBox *));
    ASSERT_GENERAL_TAB_SIGNATURE(setColorSamplerPreviewStyleIndexByValue,
                                 void (*)(QComboBox *, KisConfig::ColorSamplerPreviewStyle));
    ASSERT_GENERAL_TAB_SIGNATURE(getColorSamplerPreviewStyleValue,
                                 KisConfig::ColorSamplerPreviewStyle (*)(const QComboBox *));
    ASSERT_GENERAL_TAB_SIGNATURE(showOutlineWhilePainting, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(showEraserOutlineWhilePainting, bool (GeneralTab::*)());
}

void KisDlgPreferencesEnumContractTest::generalTabSessionAndDocumentSchemaRemainsStable()
{
    ASSERT_GENERAL_TAB_SIGNATURE(sessionOnStartup, KisConfig::SessionOnStartup (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(iconsInMenu, KisConfig::IconsInMenu (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(saveSessionOnQuit, bool (GeneralTab::*)() const);
    ASSERT_GENERAL_TAB_SIGNATURE(showRootLayer, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(autoSaveInterval, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(undoStackSize, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(compressKra, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(trimKra, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(trimFramesImport, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(useZip64, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(exportMimeType, QString (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(renameMergedLayers, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(renamePastedLayers, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(renameDuplicatedLayers, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(convertToImageColorspaceOnImport, bool (GeneralTab::*)());
}

void KisDlgPreferencesEnumContractTest::generalTabInteractionAndNavigationSchemaRemainsStable()
{
    ASSERT_GENERAL_TAB_SIGNATURE(longPressEnabled, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(kineticScrollingEnabled, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(kineticScrollingGesture, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(kineticScrollingSensitivity, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(kineticScrollingHiddenScrollbars, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(zoomSteps, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(zoomMarginSize, int (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(switchSelectionCtrlAlt, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(toolOptionsInDocker, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(showCanvasMessages, bool (GeneralTab::*)());
}

void KisDlgPreferencesEnumContractTest::generalTabAnimationSchemaRemainsStable()
{
    ASSERT_GENERAL_TAB_SIGNATURE(autopinLayersToTimeline, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(adaptivePlaybackRange, bool (GeneralTab::*)());
    ASSERT_GENERAL_TAB_SIGNATURE(autoZoomTimelineToPlaybackRange, bool (GeneralTab::*)());
}

// clang-format off
void KisDlgPreferencesEnumContractTest::importExportOptionsSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisImportExportPreferenceOptions>);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, exportMimeTypes);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteFormatAsk);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteFormatDownload);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteFormatLocal);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteFormatBitmap);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteAssumeWeb);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteAssumeMonitor);
    ASSERT_PREFERENCE_MEMBER(KisImportExportPreferenceOptions, pasteAsk);
}

void KisDlgPreferencesEnumContractTest::generatedSettingsWidgetSchemaRemainsStable()
{
    static_assert(std::is_class_v<WdgGeneralSettings> && std::is_constructible_v<WdgGeneralSettings, QWidget *, const char *>);
    static_assert(std::is_class_v<WdgShortcutSettings> && std::is_constructible_v<WdgShortcutSettings, QWidget *>);
    static_assert(std::is_class_v<WdgColorSettings> && std::is_constructible_v<WdgColorSettings, QWidget *>);
    static_assert(std::is_class_v<WdgTabletSettings> && std::is_constructible_v<WdgTabletSettings, QWidget *>);
    static_assert(std::is_class_v<WdgPerformanceSettings> && std::is_constructible_v<WdgPerformanceSettings, QWidget *, const char *>);
    static_assert(std::is_class_v<WdgDisplaySettings> && std::is_constructible_v<WdgDisplaySettings, QWidget *, const char *>);
    static_assert(std::is_class_v<WdgFullscreenSettingsBase> && std::is_constructible_v<WdgFullscreenSettingsBase, QWidget *>);
    static_assert(std::is_class_v<WdgPopupPaletteSettingsBase> && std::is_constructible_v<WdgPopupPaletteSettingsBase, QWidget *, const char *>);
}

void KisDlgPreferencesEnumContractTest::colorSettingsStructureSchemaRemainsStable()
{
    static_assert(std::is_class_v<ColorSettingsTab> && std::is_constructible_v<ColorSettingsTab, QWidget *, const KisImportExportPreferenceOptions &, const char *>);
    ASSERT_PREFERENCE_CALLABLE(ColorSettingsTab, setDefault);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_page);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_pasteBehaviourGroup);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_monitorProfileLabels);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_monitorProfileWidgets);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_proofModel);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_screenMigrationTracker);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_colorManagedByOS);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_chkEnableCanvasColorSpaceManagement);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_canvasSurfaceColorSpace);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_canvasSurfaceBitDepth);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_preferredSpaceGraphic);
    ASSERT_PREFERENCE_MEMBER(ColorSettingsTab, m_preferredSpaceGraphicMode);
}

void KisDlgPreferencesEnumContractTest::shortcutAndTabletStructureSchemaRemainsStable()
{
    static_assert(std::is_class_v<ShortcutSettingsTab> && std::is_constructible_v<ShortcutSettingsTab, QWidget *, const char *> && std::has_virtual_destructor_v<ShortcutSettingsTab>);
    ASSERT_PREFERENCE_CALLABLE(ShortcutSettingsTab, setDefault);
    ASSERT_PREFERENCE_CALLABLE(ShortcutSettingsTab, saveChanges);
    ASSERT_PREFERENCE_CALLABLE(ShortcutSettingsTab, cancelChanges);
    ASSERT_PREFERENCE_MEMBER(ShortcutSettingsTab, m_page);
    ASSERT_PREFERENCE_MEMBER(ShortcutSettingsTab, m_snapshot);
    static_assert(std::is_class_v<TabletSettingsTab> && std::is_constructible_v<TabletSettingsTab, QWidget *, const char *>);
    ASSERT_PREFERENCE_CALLABLE(TabletSettingsTab, setDefault);
    ASSERT_PREFERENCE_MEMBER(TabletSettingsTab, m_page);
}

void KisDlgPreferencesEnumContractTest::performanceAndDisplayStructureSchemaRemainsStable()
{
    static_assert(std::is_class_v<PerformanceTab> && std::is_constructible_v<PerformanceTab, QWidget *, const char *> && std::has_virtual_destructor_v<PerformanceTab>);
    ASSERT_PREFERENCE_CALLABLE(PerformanceTab, load);
    ASSERT_PREFERENCE_CALLABLE(PerformanceTab, save);
    static_assert(std::is_class_v<DisplaySettingsTab> && std::is_constructible_v<DisplaySettingsTab, QWidget *, const char *>);
    ASSERT_PREFERENCE_CALLABLE(DisplaySettingsTab, setDefault);
}

void KisDlgPreferencesEnumContractTest::fullscreenPopupAndDialogStructureSchemaRemainsStable()
{
    static_assert(std::is_class_v<FullscreenSettingsTab> && std::is_constructible_v<FullscreenSettingsTab, QWidget *>);
    ASSERT_PREFERENCE_CALLABLE(FullscreenSettingsTab, setDefault);
    static_assert(std::is_class_v<PopupPaletteTab> && std::is_constructible_v<PopupPaletteTab, QWidget *, const char *>);
    ASSERT_PREFERENCE_CALLABLE(PopupPaletteTab, load);
    ASSERT_PREFERENCE_CALLABLE(PopupPaletteTab, save);
    ASSERT_PREFERENCE_CALLABLE(PopupPaletteTab, setDefault);
    static_assert(std::is_class_v<KisDlgPreferences> && std::is_class_v<KisDlgPreferences::PageDesc> && std::is_constructible_v<KisDlgPreferences, QWidget *, const KisImportExportPreferenceOptions &, const char *> && std::has_virtual_destructor_v<KisDlgPreferences>);
    ASSERT_PREFERENCE_MEMBER(KisDlgPreferences::PageDesc, page);
    ASSERT_PREFERENCE_MEMBER(KisDlgPreferences::PageDesc, tab);
    ASSERT_PREFERENCE_CALLABLE(KisDlgPreferences, editPreferences);
    ASSERT_PREFERENCE_CALLABLE(KisDlgPreferences, notifyImageSettingsChanged);
    ASSERT_PREFERENCE_CALLABLE(KisDlgPreferences, showEvent);
}
// clang-format on

#undef ASSERT_PREFERENCE_CALLABLE
#undef ASSERT_PREFERENCE_MEMBER

QTEST_GUILESS_MAIN(KisDlgPreferencesEnumContractTest)

#include "KisDlgPreferencesEnumContractTest.moc"
