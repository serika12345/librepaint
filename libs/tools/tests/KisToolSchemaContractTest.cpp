/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_painting_information_builder.h>
#include <kis_tool.h>
#include <kis_tool_paint_interaction.h>
#include <operations/kis_operation_ui_widget.h>
#include <tool/kis_painting_information_builder_adapters.h>
#include <tools/ui/KisPaintResourceServerProvider.h>
#include <tools/ui/kis_paintop_list_widget.h>
#include <tools/ui/kis_rectangle_constraint_widget.h>
#include <tools/ui/kis_tool_options_popup.h>

#include <QTest>

#include <type_traits>

namespace
{
class ConstructorProbe final : public KisTool
{
public:
    using KisTool::KisTool;

    void paint(QPainter &, const KoViewConverter &) override;
};

class PaintInteractionConstructorProbe final : public KisToolPaintInteraction
{
public:
    using KisToolPaintInteraction::KisToolPaintInteraction;

private:
    void paint(QPainter &, const KoViewConverter &) override;
    void requestUpdateOutline(const QPointF &, const KoPointerEvent *) override;
};

class OperationUiWidgetProbe final : public KisOperationUIWidget
{
public:
    using KisOperationUIWidget::KisOperationUIWidget;

    void getConfiguration(KisOperationConfigurationSP) override;
};
} // namespace

class KisToolSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolIdentityLifetimeAndActivationStateSchemaRemainsStable();
    void toolActivationResourceAndCapabilitySignaturesRemainStable();
    void toolPrimaryActionSignaturesRemainStable();
    void toolAlternateActionSignaturesRemainStable();
    void toolPointerEventDispatchSignaturesRemainStable();
    void paintInteractionTypeConstructionAndLifetimeSchemaRemainStable();
    void paintInteractionActivationFlagsAndPointerEventSchemaRemainStable();
    void paintInteractionNotificationSchemaRemainStable();
    void paintOpListWidgetSchemaRemainStable();
    void paintingInformationBuilderAdaptersSchemaRemainStable();
    void paintingInformationBuilderRemainderSchemaRemainStable();
    void paintResourceServerProviderSchemaRemainStable();
    void operationUiWidgetSchemaRemainStable();
    void rectangleConstraintWidgetSchemaRemainStable();
    void toolOptionsPopupSchemaRemainStable();
};

#define ASSERT_KIS_TOOL_SIGNATURE(Method, Signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&KisTool::Method)), Signature>)

#define ASSERT_PAINT_INTERACTION_SIGNATURE(Method, Signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&KisToolPaintInteraction::Method)), Signature>)

void KisToolSchemaContractTest::toolIdentityLifetimeAndActivationStateSchemaRemainsStable()
{
    using FlagsSignature = int (KisTool::*)() const;
    using ActiveSignature = bool (KisTool::*)() const;
    using ActiveChangedSignature = void (KisTool::*)(bool);

    static_assert(std::is_class_v<KisTool>);
    static_assert(std::is_base_of_v<KoToolBase, KisTool>);
    static_assert(std::is_abstract_v<KisTool>);
    static_assert(std::has_virtual_destructor_v<KisTool>);
    static_assert(std::is_constructible_v<ConstructorProbe, KoCanvasBase *, const QCursor &>);
    ASSERT_KIS_TOOL_SIGNATURE(flags, FlagsSignature);
    ASSERT_KIS_TOOL_SIGNATURE(isActive, ActiveSignature);
    ASSERT_KIS_TOOL_SIGNATURE(isActiveChanged, ActiveChangedSignature);
}

void KisToolSchemaContractTest::toolActivationResourceAndCapabilitySignaturesRemainStable()
{
    using ActivateSignature = void (KisTool::*)(const QSet<KoShape *> &);
    using VoidSignature = void (KisTool::*)();
    using ResourceChangedSignature = void (KisTool::*)(int, const QVariant &);
    using ExternalSourceSignature = void (KisTool::*)(KisPaintDeviceSP);
    using AbilitySignature = KisTool::NodePaintAbility (KisTool::*)();
    using ConstBooleanSignature = bool (KisTool::*)() const;

    ASSERT_KIS_TOOL_SIGNATURE(activate, ActivateSignature);
    ASSERT_KIS_TOOL_SIGNATURE(deactivate, VoidSignature);
    ASSERT_KIS_TOOL_SIGNATURE(canvasResourceChanged, ResourceChangedSignature);
    ASSERT_KIS_TOOL_SIGNATURE(deleteSelection, VoidSignature);
    ASSERT_KIS_TOOL_SIGNATURE(newActivationWithExternalSource, ExternalSourceSignature);
    ASSERT_KIS_TOOL_SIGNATURE(nodePaintAbility, AbilitySignature);
    ASSERT_KIS_TOOL_SIGNATURE(supportsPaintingAssistants, ConstBooleanSignature);
    ASSERT_KIS_TOOL_SIGNATURE(updateSettingsViews, VoidSignature);
}

void KisToolSchemaContractTest::toolPrimaryActionSignaturesRemainStable()
{
    using VoidSignature = void (KisTool::*)();
    using EventSignature = void (KisTool::*)(KoPointerEvent *);
    using ConstBooleanSignature = bool (KisTool::*)() const;

    ASSERT_KIS_TOOL_SIGNATURE(activatePrimaryAction, VoidSignature);
    ASSERT_KIS_TOOL_SIGNATURE(deactivatePrimaryAction, VoidSignature);
    ASSERT_KIS_TOOL_SIGNATURE(beginPrimaryAction, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(continuePrimaryAction, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(endPrimaryAction, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(beginPrimaryDoubleClickAction, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(primaryActionSupportsHiResEvents, ConstBooleanSignature);
}

void KisToolSchemaContractTest::toolAlternateActionSignaturesRemainStable()
{
    using ActionSignature = void (KisTool::*)(KisTool::AlternateAction);
    using EventActionSignature = void (KisTool::*)(KoPointerEvent *, KisTool::AlternateAction);
    using HiResSignature = bool (KisTool::*)(KisTool::AlternateAction) const;

    ASSERT_KIS_TOOL_SIGNATURE(activateAlternateAction, ActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(deactivateAlternateAction, ActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(beginAlternateAction, EventActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(continueAlternateAction, EventActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(endAlternateAction, EventActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(beginAlternateDoubleClickAction, EventActionSignature);
    ASSERT_KIS_TOOL_SIGNATURE(alternateActionSupportsHiResEvents, HiResSignature);
}

void KisToolSchemaContractTest::toolPointerEventDispatchSignaturesRemainStable()
{
    using EventSignature = void (KisTool::*)(KoPointerEvent *);

    ASSERT_KIS_TOOL_SIGNATURE(mousePressEvent, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(mouseDoubleClickEvent, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(mouseTripleClickEvent, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(mouseReleaseEvent, EventSignature);
    ASSERT_KIS_TOOL_SIGNATURE(mouseMoveEvent, EventSignature);
}

void KisToolSchemaContractTest::paintInteractionTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisToolPaintInteraction>);
    static_assert(std::is_base_of_v<KisTool, KisToolPaintInteraction>);
    static_assert(std::is_abstract_v<KisToolPaintInteraction>);
    static_assert(std::is_constructible_v<PaintInteractionConstructorProbe, KoCanvasBase *, const QCursor &>);
    static_assert(std::has_virtual_destructor_v<KisToolPaintInteraction>);

    QVERIFY(true);
}

void KisToolSchemaContractTest::paintInteractionActivationFlagsAndPointerEventSchemaRemainStable()
{
    ASSERT_PAINT_INTERACTION_SIGNATURE(flags, int (KisToolPaintInteraction::*)() const);
    ASSERT_PAINT_INTERACTION_SIGNATURE(activate, void (KisToolPaintInteraction::*)(const QSet<KoShape *> &));
    ASSERT_PAINT_INTERACTION_SIGNATURE(deactivate, void (KisToolPaintInteraction::*)());
    ASSERT_PAINT_INTERACTION_SIGNATURE(mousePressEvent, void (KisToolPaintInteraction::*)(KoPointerEvent *));
    ASSERT_PAINT_INTERACTION_SIGNATURE(mouseReleaseEvent, void (KisToolPaintInteraction::*)(KoPointerEvent *));
    ASSERT_PAINT_INTERACTION_SIGNATURE(mouseMoveEvent, void (KisToolPaintInteraction::*)(KoPointerEvent *));

    QVERIFY(true);
}

void KisToolSchemaContractTest::paintInteractionNotificationSchemaRemainStable()
{
    ASSERT_PAINT_INTERACTION_SIGNATURE(sigPaintingFinished, void (KisToolPaintInteraction::*)());

    QVERIFY(true);
}

void KisToolSchemaContractTest::paintOpListWidgetSchemaRemainStable()
{
    using Widget = KisPaintOpListWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisCategorizedListView, Widget>);
    static_assert(std::is_default_constructible_v<Widget>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::currentItem), QString (Widget::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Widget::setPaintOpList), void (Widget::*)(const QList<KisPaintOpFactory *> &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Widget::*)(const KisPaintOpFactory *)>(&Widget::setCurrent)),
                       void (Widget::*)(const KisPaintOpFactory *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Widget::*)(const QString &)>(&Widget::setCurrent)),
                                 void (Widget::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Widget::activated), void (Widget::*)(const QString &)>);
}

void KisToolSchemaContractTest::paintingInformationBuilderAdaptersSchemaRemainStable()
{
    static_assert(std::is_class_v<KisConverterPaintingInformationBuilder>);
    static_assert(std::is_base_of_v<KisPaintingInformationBuilder, KisConverterPaintingInformationBuilder>);
    static_assert(std::is_constructible_v<KisConverterPaintingInformationBuilder, const KisCoordinatesConverter *>);
    static_assert(std::is_class_v<KisToolFreehandPaintingInformationBuilder>);
    static_assert(std::is_base_of_v<KisPaintingInformationBuilder, KisToolFreehandPaintingInformationBuilder>);
    static_assert(std::is_constructible_v<KisToolFreehandPaintingInformationBuilder, KisToolFreehand *>);
}

void KisToolSchemaContractTest::paintingInformationBuilderRemainderSchemaRemainStable()
{
    using Builder = KisPaintingInformationBuilder;

    static_assert(std::has_virtual_destructor_v<Builder>);
    static_assert(
        std::is_same_v<decltype(&Builder::continueStroke), KisPaintInformation (Builder::*)(KoPointerEvent *, int)>);
    static_assert(std::is_same_v<decltype(&Builder::reset), void (Builder::*)()>);
}

void KisToolSchemaContractTest::paintResourceServerProviderSchemaRemainStable()
{
    using Provider = KisPaintResourceServerProvider;

    static_assert(std::is_same_v<KisPaintOpPresetResourceServer, KoResourceServer<KisPaintOpPreset>>);
    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_base_of_v<QObject, Provider>);
    static_assert(std::is_default_constructible_v<Provider>);
    static_assert(std::has_virtual_destructor_v<Provider>);
    static_assert(std::is_same_v<decltype(&Provider::instance), Provider *(*)()>);
    static_assert(
        std::is_same_v<decltype(&Provider::paintOpPresetServer), KisPaintOpPresetResourceServer *(Provider::*)()>);
    static_assert(
        std::is_same_v<decltype(&Provider::layerStyleServer), KoResourceServer<KisPSDLayerStyle> *(Provider::*)()>);
}

void KisToolSchemaContractTest::operationUiWidgetSchemaRemainStable()
{
    using Widget = KisOperationUIWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_abstract_v<Widget>);
    static_assert(std::is_constructible_v<OperationUiWidgetProbe, const QString &>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::caption), QString (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::getConfiguration), void (Widget::*)(KisOperationConfigurationSP)>);
}

void KisToolSchemaContractTest::rectangleConstraintWidgetSchemaRemainStable()
{
    using Widget = KisRectangleConstraintWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, const QString &, bool>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_same_v<decltype(&Widget::setRectangle), void (Widget::*)(const QRectF &)>);
    static_assert(std::is_same_v<decltype(&Widget::reloadConfig), void (Widget::*)()>);
    static_assert(
        std::is_same_v<decltype(&Widget::constraintsChanged), void (Widget::*)(bool, bool, bool, float, float, float)>);
    static_assert(std::is_same_v<decltype(&Widget::roundCornersChanged), void (Widget::*)(int, int)>);
}

void KisToolSchemaContractTest::toolOptionsPopupSchemaRemainStable()
{
    using Popup = KisToolOptionsPopup;

    static_assert(std::is_class_v<Popup>);
    static_assert(std::is_base_of_v<QWidget, Popup>);
    static_assert(std::is_constructible_v<Popup, const QFont &>);
    static_assert(std::has_virtual_destructor_v<Popup>);
    static_assert(
        std::is_same_v<decltype(&Popup::newOptionWidgets), void (Popup::*)(const QList<QPointer<QWidget>> &)>);
}

#undef ASSERT_KIS_TOOL_SIGNATURE
#undef ASSERT_PAINT_INTERACTION_SIGNATURE

QTEST_GUILESS_MAIN(KisToolSchemaContractTest)

#include "KisToolSchemaContractTest.moc"
