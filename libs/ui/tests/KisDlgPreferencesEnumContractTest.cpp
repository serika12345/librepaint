/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <dialogs/kis_dlg_preferences.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

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

QTEST_GUILESS_MAIN(KisDlgPreferencesEnumContractTest)

#include "KisDlgPreferencesEnumContractTest.moc"
