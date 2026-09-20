/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <kcolorscheme.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QTemporaryDir>
#include <QTest>

// Compatibility requirement: Packaged and user-supplied .colors theme files depend on the color group and entry names interpreted by KColorScheme.

class KColorSchemeThemeCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void themeColorGroupsProvideConfiguredColors();
    void themeFeedbackRolesProvideConfiguredColors();
    void shadeRolesProvideVisibleBorderContrast();
};

void KColorSchemeThemeCompatibilityTest::themeColorGroupsProvideConfiguredColors()
{
    /*
     * Consumer: The theme manager and widgets that render view, window, button, selection, and tooltip surfaces.
     * Operation: Loads the named color groups from a selected .colors theme file.
     * Observable result: Each surface exposes its configured background and foreground color.
     * Failure impact: A selected theme can display the wrong colors or unreadable text in the application interface.
     */
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const KSharedConfigPtr config = KSharedConfig::openConfig(directory.filePath(QStringLiteral("theme.colors")),
                                                                KConfig::SimpleConfig);
    struct ThemeColorSet {
        KColorScheme::ColorSet set;
        const char *group;
        QColor background;
        QColor foreground;
    };
    const ThemeColorSet colors[] = {
        {KColorScheme::View, "Colors:View", QColor(21, 22, 23), QColor(24, 25, 26)},
        {KColorScheme::Window, "Colors:Window", QColor(31, 32, 33), QColor(34, 35, 36)},
        {KColorScheme::Button, "Colors:Button", QColor(41, 42, 43), QColor(44, 45, 46)},
        {KColorScheme::Selection, "Colors:Selection", QColor(51, 52, 53), QColor(54, 55, 56)},
        {KColorScheme::Tooltip, "Colors:Tooltip", QColor(61, 62, 63), QColor(64, 65, 66)},
    };

    for (const ThemeColorSet &entry : colors) {
        KConfigGroup group(config, QLatin1String(entry.group));
        group.writeEntry("BackgroundNormal", entry.background);
        group.writeEntry("ForegroundNormal", entry.foreground);
    }
    config->sync();

    for (const ThemeColorSet &entry : colors) {
        const KColorScheme scheme(QPalette::Active, entry.set, config);
        QCOMPARE(scheme.background().color(), entry.background);
        QCOMPARE(scheme.foreground().color(), entry.foreground);
    }
}

void KColorSchemeThemeCompatibilityTest::themeFeedbackRolesProvideConfiguredColors()
{
    /*
     * Consumer: Plugin and widget feedback that marks errors, warnings, success, focus, and hover states.
     * Operation: Loads named feedback entries from the view color group.
     * Observable result: Feedback and decoration role lookups return the colors chosen by the theme author.
     * Failure impact: Users lose visual error, warning, success, focus, or hover cues needed to operate the interface.
     */
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const KSharedConfigPtr config = KSharedConfig::openConfig(directory.filePath(QStringLiteral("theme.colors")),
                                                                KConfig::SimpleConfig);
    KConfigGroup view(config, "Colors:View");
    const QColor negative(151, 1, 2);
    const QColor neutral(151, 152, 3);
    const QColor positive(4, 153, 5);
    const QColor focus(6, 7, 154);
    const QColor hover(155, 8, 9);
    view.writeEntry("ForegroundNegative", negative);
    view.writeEntry("ForegroundNeutral", neutral);
    view.writeEntry("ForegroundPositive", positive);
    view.writeEntry("DecorationFocus", focus);
    view.writeEntry("DecorationHover", hover);
    config->sync();

    const KColorScheme scheme(QPalette::Active, KColorScheme::View, config);
    QCOMPARE(scheme.foreground(KColorScheme::NegativeText).color(), negative);
    QCOMPARE(scheme.foreground(KColorScheme::NeutralText).color(), neutral);
    QCOMPARE(scheme.foreground(KColorScheme::PositiveText).color(), positive);
    QCOMPARE(scheme.decoration(KColorScheme::FocusColor).color(), focus);
    QCOMPARE(scheme.decoration(KColorScheme::HoverColor).color(), hover);
}

void KColorSchemeThemeCompatibilityTest::shadeRolesProvideVisibleBorderContrast()
{
    /*
     * Consumer: Widgets that draw frames, bevels, and other borders from a theme surface color.
     * Operation: Requests light, midlight, mid, dark, and shadow shades for a medium base color.
     * Observable result: The light shades are brighter and the dark shades are darker than the base surface.
     * Failure impact: Interface borders lose the contrast that separates controls and their surrounding surfaces.
     */
    const QColor base(128, 96, 64);
    const QColor light = KColorScheme::shade(base, KColorScheme::LightShade, 0.7);
    const QColor midlight = KColorScheme::shade(base, KColorScheme::MidlightShade, 0.7);
    const QColor mid = KColorScheme::shade(base, KColorScheme::MidShade, 0.7);
    const QColor dark = KColorScheme::shade(base, KColorScheme::DarkShade, 0.7);
    const QColor shadow = KColorScheme::shade(base, KColorScheme::ShadowShade, 0.7);

    QVERIFY(light.lightness() > base.lightness());
    QVERIFY(midlight.lightness() > base.lightness());
    QVERIFY(mid.lightness() < base.lightness());
    QVERIFY(dark.lightness() < base.lightness());
    QVERIFY(shadow.lightness() < dark.lightness());
}

QTEST_GUILESS_MAIN(KColorSchemeThemeCompatibilityTest)

#include "KColorSchemeThemeCompatibilityTest.moc"
