/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <kcolorscheme.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_COLOR_SCHEME_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KColorScheme::method)), signature>)
#define ASSERT_STATEFUL_BRUSH_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KStatefulBrush::method)), signature>)
} // namespace

class KColorSchemeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSetValuesRemainStable();
    void backgroundAndForegroundRoleValuesRemainStable();
    void decorationAndShadeRoleValuesRemainStable();
    void colorSchemeIdentityAndLifecycleSignaturesRemainStable();
    void colorSchemeBrushLookupSignaturesRemainStable();
    void colorSchemeStaticTransformationSignaturesRemainStable();
    void statefulBrushIdentityAndConstructionSignaturesRemainStable();
    void statefulBrushLookupSignaturesRemainStable();
};

void KColorSchemeEnumContractTest::colorSetValuesRemainStable()
{
    const std::array<KColorScheme::ColorSet, 5> colorSets{{
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
    const std::array<KColorScheme::BackgroundRole, 8> backgroundRoles{{
        KColorScheme::NormalBackground,
        KColorScheme::AlternateBackground,
        KColorScheme::ActiveBackground,
        KColorScheme::LinkBackground,
        KColorScheme::VisitedBackground,
        KColorScheme::NegativeBackground,
        KColorScheme::NeutralBackground,
        KColorScheme::PositiveBackground,
    }};
    const std::array<KColorScheme::ForegroundRole, 8> foregroundRoles{{
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
    const std::array<KColorScheme::DecorationRole, 2> decorationRoles{{
        KColorScheme::FocusColor,
        KColorScheme::HoverColor,
    }};
    const std::array<KColorScheme::ShadeRole, 5> shadeRoles{{
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

void KColorSchemeEnumContractTest::colorSchemeIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KColorScheme>);
    static_assert(std::is_copy_constructible_v<KColorScheme>);
    static_assert(std::has_virtual_destructor_v<KColorScheme>);
    static_assert(std::is_same_v<decltype(KColorScheme(std::declval<QPalette::ColorGroup>())), KColorScheme>);

    ASSERT_COLOR_SCHEME_SIGNATURE(operator=, KColorScheme & (KColorScheme::*)(const KColorScheme &));
}

void KColorSchemeEnumContractTest::colorSchemeBrushLookupSignaturesRemainStable()
{
    ASSERT_COLOR_SCHEME_SIGNATURE(background, QBrush (KColorScheme::*)(KColorScheme::BackgroundRole) const);
    ASSERT_COLOR_SCHEME_SIGNATURE(decoration, QBrush (KColorScheme::*)(KColorScheme::DecorationRole) const);
    ASSERT_COLOR_SCHEME_SIGNATURE(foreground, QBrush (KColorScheme::*)(KColorScheme::ForegroundRole) const);
    ASSERT_COLOR_SCHEME_SIGNATURE(shade, QColor (KColorScheme::*)(KColorScheme::ShadeRole) const);

    static_assert(std::is_same_v<decltype(std::declval<const KColorScheme &>().background()), QBrush>);
    static_assert(std::is_same_v<decltype(std::declval<const KColorScheme &>().foreground()), QBrush>);
}

void KColorSchemeEnumContractTest::colorSchemeStaticTransformationSignaturesRemainStable()
{
    ASSERT_COLOR_SCHEME_SIGNATURE(adjustBackground,
                                  void (*)(QPalette &,
                                           KColorScheme::BackgroundRole,
                                           QPalette::ColorRole,
                                           KColorScheme::ColorSet,
                                           KSharedConfigPtr));
    ASSERT_COLOR_SCHEME_SIGNATURE(adjustForeground,
                                  void (*)(QPalette &,
                                           KColorScheme::ForegroundRole,
                                           QPalette::ColorRole,
                                           KColorScheme::ColorSet,
                                           KSharedConfigPtr));
    ASSERT_COLOR_SCHEME_SIGNATURE(contrast, int (*)());
    ASSERT_COLOR_SCHEME_SIGNATURE(contrastF, qreal (*)(const KSharedConfigPtr &));
    ASSERT_COLOR_SCHEME_SIGNATURE(createApplicationPalette, QPalette (*)(const KSharedConfigPtr &));
    ASSERT_COLOR_SCHEME_SIGNATURE(shade, QColor (*)(const QColor &, KColorScheme::ShadeRole));
    ASSERT_COLOR_SCHEME_SIGNATURE(shade, QColor (*)(const QColor &, KColorScheme::ShadeRole, qreal, qreal));

    static_assert(std::is_same_v<decltype(KColorScheme::contrastF()), qreal>);
    static_assert(std::is_same_v<decltype(KColorScheme::adjustBackground(std::declval<QPalette &>())), void>);
    static_assert(std::is_same_v<decltype(KColorScheme::adjustForeground(std::declval<QPalette &>())), void>);
    static_assert(std::is_same_v<decltype(KColorScheme::shade(std::declval<const QColor &>(),
                                                              std::declval<KColorScheme::ShadeRole>(),
                                                              std::declval<qreal>())),
                                 QColor>);
}

void KColorSchemeEnumContractTest::statefulBrushIdentityAndConstructionSignaturesRemainStable()
{
    static_assert(std::is_class_v<KStatefulBrush>);
    static_assert(std::is_default_constructible_v<KStatefulBrush>);
    static_assert(std::is_copy_constructible_v<KStatefulBrush>);
    static_assert(std::is_destructible_v<KStatefulBrush>);
    static_assert(
        std::
            is_constructible_v<KStatefulBrush, KColorScheme::ColorSet, KColorScheme::BackgroundRole, KSharedConfigPtr>);
    static_assert(
        std::
            is_constructible_v<KStatefulBrush, KColorScheme::ColorSet, KColorScheme::DecorationRole, KSharedConfigPtr>);
    static_assert(
        std::
            is_constructible_v<KStatefulBrush, KColorScheme::ColorSet, KColorScheme::ForegroundRole, KSharedConfigPtr>);
    static_assert(std::is_constructible_v<KStatefulBrush, const QBrush &, KSharedConfigPtr>);
    static_assert(std::is_constructible_v<KStatefulBrush, const QBrush &, const QBrush &, KSharedConfigPtr>);

    static_assert(std::is_same_v<decltype(KStatefulBrush(std::declval<KColorScheme::ColorSet>(),
                                                         std::declval<KColorScheme::BackgroundRole>())),
                                 KStatefulBrush>);
    static_assert(std::is_same_v<decltype(KStatefulBrush(std::declval<KColorScheme::ColorSet>(),
                                                         std::declval<KColorScheme::DecorationRole>())),
                                 KStatefulBrush>);
    static_assert(std::is_same_v<decltype(KStatefulBrush(std::declval<KColorScheme::ColorSet>(),
                                                         std::declval<KColorScheme::ForegroundRole>())),
                                 KStatefulBrush>);
    static_assert(std::is_same_v<decltype(KStatefulBrush(std::declval<const QBrush &>())), KStatefulBrush>);
    static_assert(
        std::is_same_v<decltype(KStatefulBrush(std::declval<const QBrush &>(), std::declval<const QBrush &>())),
                       KStatefulBrush>);

    ASSERT_STATEFUL_BRUSH_SIGNATURE(operator=, KStatefulBrush & (KStatefulBrush::*)(const KStatefulBrush &));
}

void KColorSchemeEnumContractTest::statefulBrushLookupSignaturesRemainStable()
{
    ASSERT_STATEFUL_BRUSH_SIGNATURE(brush, QBrush (KStatefulBrush::*)(QPalette::ColorGroup) const);
    ASSERT_STATEFUL_BRUSH_SIGNATURE(brush, QBrush (KStatefulBrush::*)(const QPalette &) const);
    ASSERT_STATEFUL_BRUSH_SIGNATURE(brush, QBrush (KStatefulBrush::*)(const QWidget *) const);
}

QTEST_GUILESS_MAIN(KColorSchemeEnumContractTest)

#include "KColorSchemeEnumContractTest.moc"
