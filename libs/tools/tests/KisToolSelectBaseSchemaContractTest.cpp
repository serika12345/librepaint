/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_select_base.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
class BaseProbe
{
public:
    explicit BaseProbe(KoCanvasBase *);
    BaseProbe(KoCanvasBase *, const QCursor &);
    BaseProbe(KoCanvasBase *, const QCursor &, KoToolBase *);
    virtual ~BaseProbe();

    virtual void activateAlternateAction(KisTool::AlternateAction);
    virtual void deactivateAlternateAction(KisTool::AlternateAction);
    virtual void beginAlternateAction(KoPointerEvent *, KisTool::AlternateAction);
    virtual void continueAlternateAction(KoPointerEvent *, KisTool::AlternateAction);
    virtual void endAlternateAction(KoPointerEvent *, KisTool::AlternateAction);
    virtual void beginPrimaryAction(KoPointerEvent *);
    virtual void continuePrimaryAction(KoPointerEvent *);
    virtual void endPrimaryAction(KoPointerEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void mouseMoveEvent(KoPointerEvent *);
    virtual KisPopupWidgetInterface *popupWidget();

protected:
    KoCanvasBase *canvas() const;
};

class ToolSelectProbe final : public KisToolSelectBase<BaseProbe>
{
public:
    using KisToolSelectBase<BaseProbe>::KisToolSelectBase;

protected:
    SelectionAction configuredSelectionAction() const override;
};

using Subject = KisToolSelectBase<BaseProbe>;
} // namespace

class KisToolSelectBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolSelectBaseTypeAndConstructionSchemaRemainStable();
    void toolSelectBaseSelectionActionAndStateSignaturesRemainStable();
    void toolSelectBasePrimaryInputSignaturesRemainStable();
    void toolSelectBaseAlternateActionSignaturesRemainStable();
    void toolSelectBaseInteractionLookupAndPopupSignaturesRemainStable();
};

#define ASSERT_TOOL_SELECT_SIGNATURE(Method, Signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Subject::Method)), Signature>)

void KisToolSelectBaseSchemaContractTest::toolSelectBaseTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Subject>);
    static_assert(std::is_constructible_v<ToolSelectProbe, KoCanvasBase *>);
    static_assert(std::is_constructible_v<ToolSelectProbe, KoCanvasBase *, const QCursor &>);
    static_assert(std::is_constructible_v<ToolSelectProbe, KoCanvasBase *, const QCursor &, KoToolBase *>);
}

void KisToolSelectBaseSchemaContractTest::toolSelectBaseSelectionActionAndStateSignaturesRemainStable()
{
    using ActionGetter = SelectionAction (Subject::*)() const;
    using ActionSetter = void (Subject::*)(SelectionAction);
    using BooleanGetter = bool (Subject::*)() const;

    ASSERT_TOOL_SELECT_SIGNATURE(selectionAction, ActionGetter);
    ASSERT_TOOL_SELECT_SIGNATURE(alternateSelectionAction, ActionGetter);
    ASSERT_TOOL_SELECT_SIGNATURE(setAlternateSelectionAction, ActionSetter);
    ASSERT_TOOL_SELECT_SIGNATURE(isMovingSelection, BooleanGetter);
    ASSERT_TOOL_SELECT_SIGNATURE(isSelecting, BooleanGetter);
    ASSERT_TOOL_SELECT_SIGNATURE(selectionDidMove, BooleanGetter);
}

void KisToolSelectBaseSchemaContractTest::toolSelectBasePrimaryInputSignaturesRemainStable()
{
    using KeyEvent = void (Subject::*)(QKeyEvent *);
    using PointerEvent = void (Subject::*)(KoPointerEvent *);

    ASSERT_TOOL_SELECT_SIGNATURE(keyPressEvent, KeyEvent);
    ASSERT_TOOL_SELECT_SIGNATURE(keyReleaseEvent, KeyEvent);
    ASSERT_TOOL_SELECT_SIGNATURE(mouseMoveEvent, PointerEvent);
    ASSERT_TOOL_SELECT_SIGNATURE(beginPrimaryAction, PointerEvent);
    ASSERT_TOOL_SELECT_SIGNATURE(continuePrimaryAction, PointerEvent);
    ASSERT_TOOL_SELECT_SIGNATURE(endPrimaryAction, PointerEvent);
}

void KisToolSelectBaseSchemaContractTest::toolSelectBaseAlternateActionSignaturesRemainStable()
{
    using Action = void (Subject::*)(KisTool::AlternateAction);
    using PointerAction = void (Subject::*)(KoPointerEvent *, KisTool::AlternateAction);

    ASSERT_TOOL_SELECT_SIGNATURE(activateAlternateAction, Action);
    ASSERT_TOOL_SELECT_SIGNATURE(deactivateAlternateAction, Action);
    ASSERT_TOOL_SELECT_SIGNATURE(beginAlternateAction, PointerAction);
    ASSERT_TOOL_SELECT_SIGNATURE(continueAlternateAction, PointerAction);
    ASSERT_TOOL_SELECT_SIGNATURE(endAlternateAction, PointerAction);
}

void KisToolSelectBaseSchemaContractTest::toolSelectBaseInteractionLookupAndPopupSignaturesRemainStable()
{
    using Interaction = bool (Subject::*)();
    using Locate = KisNodeSP (Subject::*)(const QPointF &, Qt::KeyboardModifiers) const;
    using Popup = KisPopupWidgetInterface *(Subject::*)();

    ASSERT_TOOL_SELECT_SIGNATURE(beginMoveSelectionInteraction, Interaction);
    ASSERT_TOOL_SELECT_SIGNATURE(endMoveSelectionInteraction, Interaction);
    ASSERT_TOOL_SELECT_SIGNATURE(beginSelectInteraction, Interaction);
    ASSERT_TOOL_SELECT_SIGNATURE(endSelectInteraction, Interaction);
    ASSERT_TOOL_SELECT_SIGNATURE(locateSelectionMaskUnderCursor, Locate);
    ASSERT_TOOL_SELECT_SIGNATURE(popupWidget, Popup);
}

#undef ASSERT_TOOL_SELECT_SIGNATURE

QTEST_APPLESS_MAIN(KisToolSelectBaseSchemaContractTest)

#include "KisToolSelectBaseSchemaContractTest.moc"
