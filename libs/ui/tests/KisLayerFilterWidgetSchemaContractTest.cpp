/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_layer_filter_widget.h"
#include "widgets/kis_multi_double_filter_widget.h"
#include "widgets/kis_multi_integer_filter_widget.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerFilterWidget::method)), signature>)

#define ASSERT_LAYER_FILTER_TOOL_BUTTON_SIGNATURE(method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerFilterWidgetToolButton::method)), signature>)

#define ASSERT_MOUSE_CLICK_IGNORE_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&MouseClickIgnore::method)), signature>)

#define ASSERT_NUMERIC_FILTER_SIGNATURE(type, method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
} // namespace

class KisLayerFilterWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layerFilterWidgetTypeAndConstructionSchemaRemainStable();
    void layerFilterWidgetQueryAndGeometrySignaturesRemainStable();
    void layerFilterWidgetMutationPresentationAndNotificationSignaturesRemainStable();
    void layerFilterWidgetToolButtonTypeLifetimeAndControlSchemaRemainStable();
    void mouseClickIgnoreTypeConstructionAndEventFilterSchemaRemainStable();
    void doubleFilterParameterValueSchemaRemainStable();
    void integerFilterParameterValueSchemaRemainStable();
    void delayedNumericInputSchemaRemainStable();
    void multiDoubleFilterWidgetSchemaRemainStable();
    void multiIntegerFilterWidgetSchemaRemainStable();
};

void KisLayerFilterWidgetSchemaContractTest::layerFilterWidgetTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisLayerFilterWidget>);
    static_assert(std::is_base_of_v<QWidget, KisLayerFilterWidget>);
    static_assert(std::is_constructible_v<KisLayerFilterWidget, QWidget *>);
    static_assert(std::is_same_v<decltype(KisLayerFilterWidget()), KisLayerFilterWidget>);
}

void KisLayerFilterWidgetSchemaContractTest::layerFilterWidgetQueryAndGeometrySignaturesRemainStable()
{
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(getActiveColors, QSet<int> (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(getDesiredMinimumHeight, int (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(getDesiredMinimumWidth, int (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(getTextFilter, QString (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(hasTextFilter, bool (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(isCurrentlyFiltering, bool (KisLayerFilterWidget::*)() const);
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(scanUsedColorLabels, void (*)(KisNodeSP, QSet<int> &));
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(sizeHint, QSize (KisLayerFilterWidget::*)() const);
}

// clang-format off
void KisLayerFilterWidgetSchemaContractTest::layerFilterWidgetMutationPresentationAndNotificationSignaturesRemainStable()
{
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(filteringOptionsChanged, void (KisLayerFilterWidget::*)());
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(reset, void (KisLayerFilterWidget::*)());
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(showEvent, void (KisLayerFilterWidget::*)(QShowEvent *));
    ASSERT_LAYER_FILTER_WIDGET_SIGNATURE(updateColorLabels, void (KisLayerFilterWidget::*)(KisNodeSP));
}
// clang-format on

void KisLayerFilterWidgetSchemaContractTest::layerFilterWidgetToolButtonTypeLifetimeAndControlSchemaRemainStable()
{
    using ToolButton = KisLayerFilterWidgetToolButton;

    static_assert(std::is_class_v<ToolButton>);
    static_assert(std::is_base_of_v<QToolButton, ToolButton>);
    static_assert(std::is_constructible_v<ToolButton, QWidget *>);
    static_assert(std::is_same_v<decltype(ToolButton()), ToolButton>);
    static_assert(std::is_constructible_v<ToolButton, const ToolButton &>);
    static_assert(std::is_same_v<decltype(ToolButton(std::declval<const ToolButton &>())), ToolButton>);
    static_assert(std::is_destructible_v<ToolButton>);
    ASSERT_LAYER_FILTER_TOOL_BUTTON_SIGNATURE(setSelectedColors, void (ToolButton::*)(QList<int>));
    ASSERT_LAYER_FILTER_TOOL_BUTTON_SIGNATURE(setTextFilter, void (ToolButton::*)(bool));
}

void KisLayerFilterWidgetSchemaContractTest::mouseClickIgnoreTypeConstructionAndEventFilterSchemaRemainStable()
{
    static_assert(std::is_class_v<MouseClickIgnore>);
    static_assert(std::is_base_of_v<QObject, MouseClickIgnore>);
    static_assert(std::is_constructible_v<MouseClickIgnore, QObject *>);
    static_assert(std::is_same_v<decltype(MouseClickIgnore()), MouseClickIgnore>);
    ASSERT_MOUSE_CLICK_IGNORE_SIGNATURE(eventFilter, bool (MouseClickIgnore::*)(QObject *, QEvent *));
}

void KisLayerFilterWidgetSchemaContractTest::doubleFilterParameterValueSchemaRemainStable()
{
    using Param = KisDoubleWidgetParam;

    static_assert(std::is_class_v<Param>);
    static_assert(std::is_same_v<vKisDoubleWidgetParam, std::vector<Param>>);
    static_assert(std::is_constructible_v<Param, double, double, double, const QString &, const QString &>);
    static_assert(std::is_same_v<decltype(&Param::min), double Param::*>);
    static_assert(std::is_same_v<decltype(&Param::max), double Param::*>);
    static_assert(std::is_same_v<decltype(&Param::initvalue), double Param::*>);
    static_assert(std::is_same_v<decltype(&Param::label), QString Param::*>);
    static_assert(std::is_same_v<decltype(&Param::name), QString Param::*>);
}

void KisLayerFilterWidgetSchemaContractTest::integerFilterParameterValueSchemaRemainStable()
{
    using Param = KisIntegerWidgetParam;

    static_assert(std::is_class_v<Param>);
    static_assert(std::is_same_v<vKisIntegerWidgetParam, std::vector<Param>>);
    static_assert(
        std::is_constructible_v<Param, qint32, qint32, qint32, const QString &, const QString &, const QString &>);
    static_assert(std::is_same_v<decltype(&Param::min), qint32 Param::*>);
    static_assert(std::is_same_v<decltype(&Param::max), qint32 Param::*>);
    static_assert(std::is_same_v<decltype(&Param::initvalue), qint32 Param::*>);
    static_assert(std::is_same_v<decltype(&Param::label), QString Param::*>);
    static_assert(std::is_same_v<decltype(&Param::name), QString Param::*>);
    static_assert(std::is_same_v<decltype(&Param::lockerName), QString Param::*>);
}

void KisLayerFilterWidgetSchemaContractTest::delayedNumericInputSchemaRemainStable()
{
    using DoubleInput = KisDelayedActionDoubleInput;
    using IntegerInput = KisDelayedActionIntegerInput;

    static_assert(std::is_class_v<DoubleInput>);
    static_assert(std::is_base_of_v<KisDoubleSliderSpinBox, DoubleInput>);
    static_assert(std::is_constructible_v<DoubleInput, QWidget *, const QString &>);
    ASSERT_NUMERIC_FILTER_SIGNATURE(DoubleInput, cancelDelayedSignal, void (DoubleInput::*)());
    ASSERT_NUMERIC_FILTER_SIGNATURE(DoubleInput, valueChangedDelayed, void (DoubleInput::*)(double));

    static_assert(std::is_class_v<IntegerInput>);
    static_assert(std::is_base_of_v<KisSliderSpinBox, IntegerInput>);
    static_assert(std::is_constructible_v<IntegerInput, QWidget *, const QString &>);
    ASSERT_NUMERIC_FILTER_SIGNATURE(IntegerInput, cancelDelayedSignal, void (IntegerInput::*)());
    ASSERT_NUMERIC_FILTER_SIGNATURE(IntegerInput, valueChangedDelayed, void (IntegerInput::*)(int));
}

void KisLayerFilterWidgetSchemaContractTest::multiDoubleFilterWidgetSchemaRemainStable()
{
    using Widget = KisMultiDoubleFilterWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisConfigWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, const QString &, QWidget *, const QString &, vKisDoubleWidgetParam>);
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, setConfiguration, void (Widget::*)(const KisPropertiesConfigurationSP));
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, configuration, KisPropertiesConfigurationSP (Widget::*)() const);
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, nbValues, qint32 (Widget::*)() const);
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, valueAt, double (Widget::*)(qint32));
}

void KisLayerFilterWidgetSchemaContractTest::multiIntegerFilterWidgetSchemaRemainStable()
{
    using Widget = KisMultiIntegerFilterWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisConfigWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, const QString &, QWidget *, const QString &, vKisIntegerWidgetParam>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, setConfiguration, void (Widget::*)(KisPropertiesConfigurationSP));
    ASSERT_NUMERIC_FILTER_SIGNATURE(Widget, configuration, KisPropertiesConfigurationSP (Widget::*)() const);
}

#undef ASSERT_NUMERIC_FILTER_SIGNATURE

QTEST_APPLESS_MAIN(KisLayerFilterWidgetSchemaContractTest)

#include "KisLayerFilterWidgetSchemaContractTest.moc"
