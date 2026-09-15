/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFilterOptionWidget.h"
#include "KisMirrorOptionWidget.h"
#include "KisPaintingModeOptionWidget.h"
#include "KisScatterOptionWidget.h"
#include "KisTextureOptionWidget.h"

#include <QTest>

#include <type_traits>

namespace
{

template<typename Widget, typename Data>
void verifyPaintOpOptionWidgetSchema()
{
    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisPaintOpOption, Widget>);
    static_assert(std::is_same_v<typename Widget::data_type, Data>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<Data>>);
    static_assert(std::is_destructible_v<Widget>);
    static_assert(
        std::is_same_v<decltype(&Widget::readOptionSetting), void (Widget::*)(const KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&Widget::writeOptionSetting), void (Widget::*)(KisPropertiesConfigurationSP) const>);
}

template<typename Widget, typename Data>
void verifyCurveOptionWidgetSchema()
{
    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisCurveOptionWidget, Widget>);
    static_assert(std::is_same_v<typename Widget::data_type, Data>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<Data>>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<Data>, KisPaintOpOption::PaintopCategory>);
    static_assert(std::is_destructible_v<Widget>);
    static_assert(
        std::is_same_v<decltype(&Widget::readOptionSetting), void (Widget::*)(const KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&Widget::writeOptionSetting), void (Widget::*)(KisPropertiesConfigurationSP) const>);
}

} // namespace

class KisPaintOptionWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterOptionWidgetSchemaRemainStable();
    void mirrorOptionWidgetSchemaRemainStable();
    void paintingModeOptionWidgetSchemaRemainStable();
    void scatterOptionWidgetSchemaRemainStable();
    void textureOptionWidgetSchemaRemainStable();
};

void KisPaintOptionWidgetSchemaContractTest::filterOptionWidgetSchemaRemainStable()
{
    using Widget = KisFilterOptionWidget;

    verifyPaintOpOptionWidgetSchema<Widget, KisFilterOptionData>();
    static_assert(std::is_same_v<decltype(&Widget::setImage), void (Widget::*)(KisImageWSP)>);
    static_assert(std::is_same_v<decltype(&Widget::setNode), void (Widget::*)(KisNodeWSP)>);
}

void KisPaintOptionWidgetSchemaContractTest::mirrorOptionWidgetSchemaRemainStable()
{
    verifyCurveOptionWidgetSchema<KisMirrorOptionWidget, KisMirrorOptionData>();
}

void KisPaintOptionWidgetSchemaContractTest::paintingModeOptionWidgetSchemaRemainStable()
{
    using Widget = KisPaintingModeOptionWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisPaintOpOption, Widget>);
    static_assert(std::is_same_v<Widget::data_type, KisPaintingModeOptionData>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<KisPaintingModeOptionData>>);
    static_assert(std::is_constructible_v<Widget, lager::cursor<KisPaintingModeOptionData>, lager::reader<bool>>);
    static_assert(std::is_destructible_v<Widget>);
    static_assert(
        std::is_same_v<decltype(&Widget::readOptionSetting), void (Widget::*)(const KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&Widget::writeOptionSetting), void (Widget::*)(KisPropertiesConfigurationSP) const>);
}

void KisPaintOptionWidgetSchemaContractTest::scatterOptionWidgetSchemaRemainStable()
{
    verifyCurveOptionWidgetSchema<KisScatterOptionWidget, KisScatterOptionData>();
}

void KisPaintOptionWidgetSchemaContractTest::textureOptionWidgetSchemaRemainStable()
{
    using Widget = KisTextureOptionWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisPaintOpOption, Widget>);
    static_assert(std::is_same_v<Widget::data_type, KisTextureOptionData>);
    static_assert(std::is_constructible_v<Widget,
                                          lager::cursor<KisTextureOptionData>,
                                          KisResourcesInterfaceSP,
                                          KisBrushTextureFlags>);
    static_assert(std::is_destructible_v<Widget>);
    static_assert(
        std::is_same_v<decltype(&Widget::readOptionSetting), void (Widget::*)(const KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&Widget::writeOptionSetting), void (Widget::*)(KisPropertiesConfigurationSP) const>);
}

QTEST_GUILESS_MAIN(KisPaintOptionWidgetSchemaContractTest)

#include "KisPaintOptionWidgetSchemaContractTest.moc"
