/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <kcolorscheme.h>

#include <QTest>

#include <array>

class KColorSchemeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSetValuesRemainStable();
    void backgroundAndForegroundRoleValuesRemainStable();
    void decorationAndShadeRoleValuesRemainStable();
};

void KColorSchemeEnumContractTest::colorSetValuesRemainStable()
{
    const std::array<KColorScheme::ColorSet, 5> colorSets {{
        KColorScheme::View,
        KColorScheme::Window,
        KColorScheme::Button,
        KColorScheme::Selection,
        KColorScheme::Tooltip,
    }};

    for (std::size_t index = 0; index < colorSets.size(); ++index) {
        QCOMPARE(int(colorSets[index]), int(index));
    }
}

void KColorSchemeEnumContractTest::backgroundAndForegroundRoleValuesRemainStable()
{
    const std::array<KColorScheme::BackgroundRole, 8> backgroundRoles {{
        KColorScheme::NormalBackground,
        KColorScheme::AlternateBackground,
        KColorScheme::ActiveBackground,
        KColorScheme::LinkBackground,
        KColorScheme::VisitedBackground,
        KColorScheme::NegativeBackground,
        KColorScheme::NeutralBackground,
        KColorScheme::PositiveBackground,
    }};
    const std::array<KColorScheme::ForegroundRole, 8> foregroundRoles {{
        KColorScheme::NormalText,
        KColorScheme::InactiveText,
        KColorScheme::ActiveText,
        KColorScheme::LinkText,
        KColorScheme::VisitedText,
        KColorScheme::NegativeText,
        KColorScheme::NeutralText,
        KColorScheme::PositiveText,
    }};

    for (std::size_t index = 0; index < backgroundRoles.size(); ++index) {
        QCOMPARE(int(backgroundRoles[index]), int(index));
        QCOMPARE(int(foregroundRoles[index]), int(index));
    }
}

void KColorSchemeEnumContractTest::decorationAndShadeRoleValuesRemainStable()
{
    const std::array<KColorScheme::DecorationRole, 2> decorationRoles {{
        KColorScheme::FocusColor,
        KColorScheme::HoverColor,
    }};
    const std::array<KColorScheme::ShadeRole, 5> shadeRoles {{
        KColorScheme::LightShade,
        KColorScheme::MidlightShade,
        KColorScheme::MidShade,
        KColorScheme::DarkShade,
        KColorScheme::ShadowShade,
    }};

    for (std::size_t index = 0; index < decorationRoles.size(); ++index) {
        QCOMPARE(int(decorationRoles[index]), int(index));
    }
    for (std::size_t index = 0; index < shadeRoles.size(); ++index) {
        QCOMPARE(int(shadeRoles[index]), int(index));
    }
}

QTEST_GUILESS_MAIN(KColorSchemeEnumContractTest)

#include "KColorSchemeEnumContractTest.moc"
