/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_layer_filter_widget.h"

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

QTEST_APPLESS_MAIN(KisLayerFilterWidgetSchemaContractTest)

#include "KisLayerFilterWidgetSchemaContractTest.moc"
