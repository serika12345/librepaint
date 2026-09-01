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
};

void KisDlgPreferencesEnumContractTest::preferredSpaceValuesRemainStable()
{
    QCOMPARE(int(ColorSettingsTab::PreferredSpace), 0);
    QCOMPARE(int(ColorSettingsTab::MasteringSpace), 1);
}

void KisDlgPreferencesEnumContractTest::canvasSurfaceAliasesRemainStable()
{
    QVERIFY((std::is_same_v<ColorSettingsTab::CanvasSurfaceMode,
                            KisConfig::CanvasSurfaceMode>));
    QVERIFY((std::is_same_v<ColorSettingsTab::CanvasSurfaceBitDepthMode,
                            KisConfig::CanvasSurfaceBitDepthMode>));
}

void KisDlgPreferencesEnumContractTest::pageValuesRemainStable()
{
    const std::array<KisDlgPreferences::Page, 9> pages {{
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
    const std::array<KisDlgPreferences::GeneralTabs, 8> tabs {{
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
    const std::array<KisDlgPreferences::ColorTabs, 3> tabs {{
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
    const std::array<KisDlgPreferences::DisplayTabs, 4> tabs {{
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
    const std::array<KisDlgPreferences::PerformaceTabs, 4> tabs {{
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

QTEST_GUILESS_MAIN(KisDlgPreferencesEnumContractTest)

#include "KisDlgPreferencesEnumContractTest.moc"
