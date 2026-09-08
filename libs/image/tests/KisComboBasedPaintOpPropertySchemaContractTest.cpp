/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_combo_based_paintop_property.h"

#include <QTest>

#include <functional>
#include <type_traits>

namespace
{

#define ASSERT_COMBO_PROPERTY_SIGNATURE(method, ...)                                                                   \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisComboBasedPaintOpProperty::method)), __VA_ARGS__>)
#define ASSERT_CALLBACK_PROPERTY_SIGNATURE(method, ...)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&CallbackProperty::method)), __VA_ARGS__>)

} // namespace

class KisComboBasedPaintOpPropertySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void comboTypeConstructionAndLifetimeSchemaRemainStable();
    void comboItemSignaturesRemainStable();
    void comboIconSignaturesRemainStable();
    void callbackTypeAndAliasSchemaRemainStable();
    void callbackConstructionSchemaRemainsStable();
    void callbackSetterSignaturesRemainStable();
};

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::comboTypeConstructionAndLifetimeSchemaRemainStable()
// clang-format on
{
    using ComboProperty = KisComboBasedPaintOpProperty;

    static_assert(std::is_class_v<ComboProperty>);
    static_assert(std::is_base_of_v<KisUniformPaintOpProperty, ComboProperty>);
    static_assert(std::is_same_v<KisComboBasedPaintOpPropertyCallback, KisCallbackBasedPaintopProperty<ComboProperty>>);
    static_assert(std::is_constructible_v<ComboProperty, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<ComboProperty,
                                          ComboProperty::Type,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(std::is_constructible_v<ComboProperty,
                                          ComboProperty::Type,
                                          ComboProperty::SubType,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(std::has_virtual_destructor_v<ComboProperty>);

    QVERIFY(true);
}

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::comboItemSignaturesRemainStable()
// clang-format on
{
    ASSERT_COMBO_PROPERTY_SIGNATURE(items, QList<QString> (KisComboBasedPaintOpProperty::*)() const);
    ASSERT_COMBO_PROPERTY_SIGNATURE(setItems, void (KisComboBasedPaintOpProperty::*)(const QList<QString> &));
}

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::comboIconSignaturesRemainStable()
// clang-format on
{
    ASSERT_COMBO_PROPERTY_SIGNATURE(icons, QList<QIcon> (KisComboBasedPaintOpProperty::*)() const);
    ASSERT_COMBO_PROPERTY_SIGNATURE(setIcons, void (KisComboBasedPaintOpProperty::*)(const QList<QIcon> &));
}

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::callbackTypeAndAliasSchemaRemainStable()
// clang-format on
{
    using CallbackProperty = KisComboBasedPaintOpPropertyCallback;
    using ReadCallback = CallbackProperty::Callback;
    using VisibleCallback = CallbackProperty::VisibleCallback;

    static_assert(std::is_class_v<KisCallbackBasedPaintopProperty<KisComboBasedPaintOpProperty>>);
    static_assert(std::is_base_of_v<KisComboBasedPaintOpProperty, CallbackProperty>);
    static_assert(std::is_same_v<ReadCallback, std::function<void(KisUniformPaintOpProperty *)>>);
    static_assert(std::is_same_v<VisibleCallback, std::function<bool(const KisUniformPaintOpProperty *)>>);

    QVERIFY(true);
}

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::callbackConstructionSchemaRemainsStable()
// clang-format on
{
    using CallbackProperty = KisComboBasedPaintOpPropertyCallback;

    static_assert(std::is_constructible_v<CallbackProperty, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<CallbackProperty,
                                          CallbackProperty::Type,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(std::is_constructible_v<CallbackProperty,
                                          CallbackProperty::Type,
                                          CallbackProperty::SubType,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);

    QVERIFY(true);
}

// clang-format off
void KisComboBasedPaintOpPropertySchemaContractTest::callbackSetterSignaturesRemainStable()
// clang-format on
{
    using CallbackProperty = KisComboBasedPaintOpPropertyCallback;

    ASSERT_CALLBACK_PROPERTY_SIGNATURE(setReadCallback, void (CallbackProperty::*)(CallbackProperty::Callback));
    ASSERT_CALLBACK_PROPERTY_SIGNATURE(setWriteCallback, void (CallbackProperty::*)(CallbackProperty::Callback));
    ASSERT_CALLBACK_PROPERTY_SIGNATURE(setIsVisibleCallback,
                                       void (CallbackProperty::*)(CallbackProperty::VisibleCallback));
}

#undef ASSERT_CALLBACK_PROPERTY_SIGNATURE
#undef ASSERT_COMBO_PROPERTY_SIGNATURE

QTEST_GUILESS_MAIN(KisComboBasedPaintOpPropertySchemaContractTest)

#include "KisComboBasedPaintOpPropertySchemaContractTest.moc"
