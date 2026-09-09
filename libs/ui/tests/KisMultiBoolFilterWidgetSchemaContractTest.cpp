/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_multi_bool_filter_widget.h"

#include <QTest>

#include <type_traits>

class KisMultiBoolFilterWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parameterTypeConstructionAndValuesSchemaRemainStable();
    void widgetTypeAndConstructionSchemaRemainStable();
    void configurationSignaturesRemainStable();
    void valueQuerySignaturesRemainStable();
};

void KisMultiBoolFilterWidgetSchemaContractTest::parameterTypeConstructionAndValuesSchemaRemainStable()
{
    using Parameter = KisBoolWidgetParam;

    static_assert(std::is_class_v<Parameter>);
    static_assert(std::is_same_v<vKisBoolWidgetParam, std::vector<Parameter>>);
    static_assert(std::is_constructible_v<Parameter, bool, const QString &, const QString &>);
    static_assert(std::is_same_v<decltype(&Parameter::initvalue), bool Parameter::*>);
    static_assert(std::is_same_v<decltype(&Parameter::label), QString Parameter::*>);
    static_assert(std::is_same_v<decltype(&Parameter::name), QString Parameter::*>);
}

void KisMultiBoolFilterWidgetSchemaContractTest::widgetTypeAndConstructionSchemaRemainStable()
{
    using Widget = KisMultiBoolFilterWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisConfigWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, const QString &, QWidget *, const QString &, vKisBoolWidgetParam>);
}

void KisMultiBoolFilterWidgetSchemaContractTest::configurationSignaturesRemainStable()
{
    using Widget = KisMultiBoolFilterWidget;

    static_assert(std::is_same_v<decltype(&Widget::setConfiguration), void (Widget::*)(KisPropertiesConfigurationSP)>);
    static_assert(std::is_same_v<decltype(&Widget::configuration), KisPropertiesConfigurationSP (Widget::*)() const>);
}

void KisMultiBoolFilterWidgetSchemaContractTest::valueQuerySignaturesRemainStable()
{
    using Widget = KisMultiBoolFilterWidget;

    static_assert(std::is_same_v<decltype(&Widget::nbValues), qint32 (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::valueAt), bool (Widget::*)(qint32) const>);
}

QTEST_APPLESS_MAIN(KisMultiBoolFilterWidgetSchemaContractTest)

#include "KisMultiBoolFilterWidgetSchemaContractTest.moc"
