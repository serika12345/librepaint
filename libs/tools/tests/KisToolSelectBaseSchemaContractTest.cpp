/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_select_base.h>
#include <tool/KisToolOutlineBase.h>
#include <tool/kis_tool_polyline_base.h>
#include <tool/kis_tool_rectangle_base.h>

#include <QTest>

#include <type_traits>

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

class OutlineProbe final : public KisToolOutlineBase
{
public:
    using KisToolOutlineBase::KisToolOutlineBase;

protected:
    void finishOutline(const QVector<QPointF> &) override;
};

class RectangleProbe final : public KisToolRectangleBase
{
public:
    using KisToolRectangleBase::KisToolRectangleBase;

protected:
    void finishRect(const QRectF &, qreal, qreal) override;
};

class PolylineProbe final : public KisToolPolylineBase
{
public:
    using KisToolPolylineBase::KisToolPolylineBase;

protected:
    void finishPolyline(const QVector<QPointF> &) override;
};

using Subject = KisToolSelectBase<BaseProbe>;
using OutlineSubject = KisToolOutlineBase;
using PolylineSubject = KisToolPolylineBase;
using RectangleSubject = KisToolRectangleBase;
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
    void outlineTypeAndConstructionSchemaRemainStable();
    void outlinePrimaryInputSignaturesRemainStable();
    void outlineAuxiliaryInputSignaturesRemainStable();
    void outlinePresentationAndLifecycleSignaturesRemainStable();
    void rectangleTypeAndConstructionSchemaRemainStable();
    void rectanglePrimaryInputSignaturesRemainStable();
    void rectangleConfigurationAndNotificationSignaturesRemainStable();
    void rectanglePresentationAndLifecycleSignaturesRemainStable();
    void polylineTypeAndPrimaryInputSignaturesRemainStable();
    void polylineAuxiliaryAndLifecycleSignaturesRemainStable();
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

#define ASSERT_MEMBER_SIGNATURE(Type, Method, Signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Type::Method)), Signature>)
#define ACTIVATE_SIGNATURE(Type) void (Type::*)(const QSet<KoShape *> &)
#define EVENT_FILTER_SIGNATURE(Type) bool (Type::*)(QObject *, QEvent *)
#define KEY_EVENT_SIGNATURE(Type) void (Type::*)(QKeyEvent *)
#define PAINT_SIGNATURE(Type) void (Type::*)(QPainter &, const KoViewConverter &)
#define POINTER_EVENT_SIGNATURE(Type) void (Type::*)(KoPointerEvent *)
#define POPUP_SIGNATURE(Type) KisPopupWidgetInterface *(Type::*)()
#define VOID_SIGNATURE(Type) void (Type::*)()

void KisToolSelectBaseSchemaContractTest::outlineTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<OutlineSubject>);
    static_assert(std::is_enum_v<OutlineSubject::ToolType>);
    static_assert(OutlineSubject::PAINT == 0);
    static_assert(OutlineSubject::SELECT == 1);
    static_assert(std::is_constructible_v<OutlineProbe, KoCanvasBase *, OutlineSubject::ToolType, const QCursor &>);
    static_assert(std::has_virtual_destructor_v<OutlineSubject>);
}

void KisToolSelectBaseSchemaContractTest::outlinePrimaryInputSignaturesRemainStable()
{
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, beginPrimaryAction, POINTER_EVENT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, continuePrimaryAction, POINTER_EVENT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, endPrimaryAction, POINTER_EVENT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, mouseMoveEvent, POINTER_EVENT_SIGNATURE(OutlineSubject));
}

void KisToolSelectBaseSchemaContractTest::outlineAuxiliaryInputSignaturesRemainStable()
{
    using InteractionState = bool (OutlineSubject::*)() const;
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, keyPressEvent, KEY_EVENT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, keyReleaseEvent, KEY_EVENT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, eventFilter, EVENT_FILTER_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, hasUserInteractionRunning, InteractionState);
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, undoLastPoint, VOID_SIGNATURE(OutlineSubject));
}

void KisToolSelectBaseSchemaContractTest::outlinePresentationAndLifecycleSignaturesRemainStable()
{
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, paint, PAINT_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, popupWidget, POPUP_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, activate, ACTIVATE_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, deactivate, VOID_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, requestStrokeEnd, VOID_SIGNATURE(OutlineSubject));
    ASSERT_MEMBER_SIGNATURE(OutlineSubject, requestStrokeCancellation, VOID_SIGNATURE(OutlineSubject));
}

void KisToolSelectBaseSchemaContractTest::rectangleTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<RectangleSubject>);
    static_assert(std::is_enum_v<RectangleSubject::ToolType>);
    static_assert(RectangleSubject::PAINT == 0);
    static_assert(RectangleSubject::SELECT == 1);
    static_assert(std::is_constructible_v<RectangleProbe, KoCanvasBase *, RectangleSubject::ToolType, const QCursor &>);
}

void KisToolSelectBaseSchemaContractTest::rectanglePrimaryInputSignaturesRemainStable()
{
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, beginPrimaryAction, POINTER_EVENT_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, continuePrimaryAction, POINTER_EVENT_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, endPrimaryAction, POINTER_EVENT_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, keyPressEvent, KEY_EVENT_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, keyReleaseEvent, KEY_EVENT_SIGNATURE(RectangleSubject));
}

void KisToolSelectBaseSchemaContractTest::rectangleConfigurationAndNotificationSignaturesRemainStable()
{
    using Constraints = void (RectangleSubject::*)(bool, bool, bool, float, float, float);
    using RectangleChanged = void (RectangleSubject::*)(const QRectF &);
    using RoundCorners = void (RectangleSubject::*)(int, int);

    ASSERT_MEMBER_SIGNATURE(RectangleSubject, constraintsChanged, Constraints);
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, roundCornersChanged, RoundCorners);
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, rectangleChanged, RectangleChanged);
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, sigRequestReloadConfig, VOID_SIGNATURE(RectangleSubject));
}

void KisToolSelectBaseSchemaContractTest::rectanglePresentationAndLifecycleSignaturesRemainStable()
{
    using OptionWidgets = QList<QPointer<QWidget>> (RectangleSubject::*)();
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, paint, PAINT_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, activate, ACTIVATE_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, deactivate, VOID_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, createOptionWidgets, OptionWidgets);
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, showSize, VOID_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, requestStrokeEnd, VOID_SIGNATURE(RectangleSubject));
    ASSERT_MEMBER_SIGNATURE(RectangleSubject, requestStrokeCancellation, VOID_SIGNATURE(RectangleSubject));
}

void KisToolSelectBaseSchemaContractTest::polylineTypeAndPrimaryInputSignaturesRemainStable()
{
    static_assert(std::is_class_v<PolylineSubject>);
    static_assert(std::is_enum_v<PolylineSubject::ToolType>);
    static_assert(PolylineSubject::PAINT == 0);
    static_assert(PolylineSubject::SELECT == 1);
    static_assert(std::is_constructible_v<PolylineProbe, KoCanvasBase *, PolylineSubject::ToolType, const QCursor &>);
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, beginPrimaryAction, POINTER_EVENT_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, endPrimaryAction, POINTER_EVENT_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, beginPrimaryDoubleClickAction, POINTER_EVENT_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, mouseMoveEvent, POINTER_EVENT_SIGNATURE(PolylineSubject));
}

void KisToolSelectBaseSchemaContractTest::polylineAuxiliaryAndLifecycleSignaturesRemainStable()
{
    using Alternate = void (PolylineSubject::*)(KoPointerEvent *, KisTool::AlternateAction);
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, beginAlternateAction, Alternate);
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, eventFilter, EVENT_FILTER_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, paint, PAINT_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, activate, ACTIVATE_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, deactivate, VOID_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, requestStrokeEnd, VOID_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, requestStrokeCancellation, VOID_SIGNATURE(PolylineSubject));
    ASSERT_MEMBER_SIGNATURE(PolylineSubject, popupWidget, POPUP_SIGNATURE(PolylineSubject));
}

#undef ACTIVATE_SIGNATURE
#undef ASSERT_MEMBER_SIGNATURE
#undef EVENT_FILTER_SIGNATURE
#undef KEY_EVENT_SIGNATURE
#undef PAINT_SIGNATURE
#undef POINTER_EVENT_SIGNATURE
#undef POPUP_SIGNATURE
#undef VOID_SIGNATURE

QTEST_APPLESS_MAIN(KisToolSelectBaseSchemaContractTest)

#include "KisToolSelectBaseSchemaContractTest.moc"
