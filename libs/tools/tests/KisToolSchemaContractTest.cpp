/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool.h>

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
};

#define ASSERT_KIS_TOOL_SIGNATURE(Method, Signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&KisTool::Method)), Signature>)

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

#undef ASSERT_KIS_TOOL_SIGNATURE

QTEST_GUILESS_MAIN(KisToolSchemaContractTest)

#include "KisToolSchemaContractTest.moc"
