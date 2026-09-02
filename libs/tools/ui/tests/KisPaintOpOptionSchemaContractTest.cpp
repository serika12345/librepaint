/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later */
#include "../kis_paintop_option.h"
#include <QTest>
#include <type_traits>

class KisPaintOpOptionSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void paintOpOptionTypeAndCategorySchemaRemainsStable();
    void paintOpOptionConstructionAndLifetimeSchemaRemainsStable();
    void paintOpOptionStateAndIdentitySchemaRemainsStable();
    void paintOpOptionContextAndConfigurationSchemaRemainsStable();
    void paintOpOptionLodAndNotificationSchemaRemainsStable();
};
void KisPaintOpOptionSchemaContractTest::paintOpOptionTypeAndCategorySchemaRemainsStable()
{
    using O = KisPaintOpOption;
    static_assert(std::is_class_v<O>);
    static_assert(
        std::is_same_v<O::OptionalLodLimitationsReader, std::optional<lager::reader<KisPaintopLodLimitations>>>);
    static_assert(std::is_enum_v<O::PaintopCategory>);
    static_assert(O::GENERAL == 0 && O::COLOR == 1 && O::TEXTURE == 2 && O::FILTER == 3 && O::MASKING_BRUSH == 4);
}
void KisPaintOpOptionSchemaContractTest::paintOpOptionConstructionAndLifetimeSchemaRemainsStable()
{
    using O = KisPaintOpOption;
    static_assert(std::is_constructible_v<O, const QString &, O::PaintopCategory, bool>);
    static_assert(std::is_constructible_v<O, const QString &, O::PaintopCategory, lager::cursor<bool>>);
    static_assert(
        std::is_constructible_v<O, const QString &, O::PaintopCategory, lager::cursor<bool>, lager::reader<bool>>);
    static_assert(std::is_destructible_v<O>);
}
void KisPaintOpOptionSchemaContractTest::paintOpOptionStateAndIdentitySchemaRemainsStable()
{
    using O = KisPaintOpOption;
    static_assert(std::is_same_v<decltype(&O::category), O::PaintopCategory (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::isCheckable), bool (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::isChecked), bool (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::setChecked), void (O::*)(bool)>);
    static_assert(std::is_same_v<decltype(&O::isEnabled), bool (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::setLocked), void (O::*)(bool)>);
    static_assert(std::is_same_v<decltype(&O::isLocked), bool (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::label), QString (O::*)() const>);
}
void KisPaintOpOptionSchemaContractTest::paintOpOptionContextAndConfigurationSchemaRemainsStable()
{
    using O = KisPaintOpOption;
    static_assert(std::is_same_v<decltype(&O::setImage), void (O::*)(KisImageWSP)>);
    static_assert(std::is_same_v<decltype(&O::setNode), void (O::*)(KisNodeWSP)>);
    static_assert(std::is_same_v<decltype(&O::setResourcesInterface), void (O::*)(KisResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&O::setCanvasResourcesInterface), void (O::*)(KoCanvasResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&O::configurationPage), QWidget *(O::*)() const>);
    static_assert(
        std::is_same_v<decltype(&O::startReadOptionSetting), void (O::*)(const KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&O::startWriteOptionSetting), void (O::*)(KisPropertiesConfigurationSP) const>);
}
void KisPaintOpOptionSchemaContractTest::paintOpOptionLodAndNotificationSchemaRemainsStable()
{
    using O = KisPaintOpOption;
    static_assert(std::is_same_v<decltype(&O::lodLimitations), void (O::*)(KisPaintopLodLimitations *) const>);
    static_assert(
        std::is_same_v<decltype(&O::effectiveLodLimitations), O::OptionalLodLimitationsReader (O::*)() const>);
    static_assert(std::is_same_v<decltype(&O::sigCheckedChanged), void (O::*)(bool)>);
    static_assert(std::is_same_v<decltype(&O::sigEnabledChanged), void (O::*)(bool)>);
    static_assert(std::is_same_v<decltype(&O::sigSettingChanged), void (O::*)()>);
}
QTEST_GUILESS_MAIN(KisPaintOpOptionSchemaContractTest)
#include "KisPaintOpOptionSchemaContractTest.moc"
