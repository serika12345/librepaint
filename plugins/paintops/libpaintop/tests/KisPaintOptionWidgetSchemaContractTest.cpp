/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisFilterOptionWidget.h"
#include "KisMirrorOptionWidget.h"
#include "KisPaintingModeOptionWidget.h"
#include "KisScatterOptionWidget.h"
#include "KisTextureOptionWidget.h"
#include "kis_auto_brush_widget.h"
#include "kis_brush_selection_widget.h"
#include "kis_predefined_brush_chooser.h"

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
    void autoBrushWidgetSchemaRemainsStable();
    void predefinedBrushChooserSchemaRemainsStable();
    void brushSelectionWidgetSchemaRemainsStable();
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

void KisPaintOptionWidgetSchemaContractTest::autoBrushWidgetSchemaRemainsStable()
{
    using BaseWidget = KisWdgAutoBrush;
    using Widget = KisAutoBrushWidget;

    static_assert(std::is_base_of_v<QWidget, BaseWidget>);
    static_assert(std::is_constructible_v<BaseWidget, QWidget *, const char *>);
    static_assert(std::is_base_of_v<BaseWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, int, KisAutoBrushModel *, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::brush), KisBrushSP (Widget::*)()>);
}

void KisPaintOptionWidgetSchemaContractTest::predefinedBrushChooserSchemaRemainsStable()
{
    using Chooser = KisPredefinedBrushChooser;

    static_assert(std::is_base_of_v<QWidget, Chooser>);
    static_assert(std::is_constructible_v<Chooser, int, KisPredefinedBrushModel *, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Chooser>);
    static_assert(std::is_same_v<decltype(&Chooser::setBrush), void (Chooser::*)(KisBrushSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::setImage), void (Chooser::*)(KisImageWSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::lightnessModeEnabled), lager::reader<bool> (Chooser::*)() const>);
}

void KisPaintOptionWidgetSchemaContractTest::brushSelectionWidgetSchemaRemainsStable()
{
    using Widget = KisBrushSelectionWidget;

    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget,
                                          int,
                                          KisAutoBrushModel *,
                                          KisPredefinedBrushModel *,
                                          KisTextBrushModel *,
                                          lager::cursor<KisBrushModel::BrushType>,
                                          lager::cursor<KisBrushModel::PrecisionData>,
                                          KisBrushOptionWidgetFlags,
                                          QWidget *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::brush), KisBrushSP (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::setImage), void (Widget::*)(KisImageWSP)>);
    static_assert(std::is_same_v<decltype(&Widget::hideOptions), void (Widget::*)(const QStringList &)>);
    static_assert(std::is_same_v<decltype(&Widget::lightnessModeEnabled), lager::reader<bool> (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::sigBrushChanged), void (Widget::*)()>);
    static_assert(std::is_same_v<decltype(&Widget::sigPrecisionChanged), void (Widget::*)()>);
}

QTEST_GUILESS_MAIN(KisPaintOptionWidgetSchemaContractTest)

#include "KisPaintOptionWidgetSchemaContractTest.moc"
