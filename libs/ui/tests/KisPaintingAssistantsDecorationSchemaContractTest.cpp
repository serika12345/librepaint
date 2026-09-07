/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "canvas/kis_canvas_widget_base.h"
#include "canvas/kis_painting_assistants_decoration.h"

#define ASSERT_CANVAS_WIDGET_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvasWidgetBase::method)), signature>)
#define ASSERT_CANVAS_DECORATION_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCanvasDecoration::method)), signature>)
#define ASSERT_ASSISTANTS_DECORATION_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintingAssistantsDecoration::method)), signature>)
#define ASSERT_EDITOR_MEMBER_TYPE(member, type)                                                                        \
    static_assert(std::is_same_v<decltype(AssistantEditorData::member), type>)

class CanvasDecorationProbe final : public KisCanvasDecoration
{
public:
    using KisCanvasDecoration::KisCanvasDecoration;

protected:
    void drawDecoration(QPainter &, const QRectF &, const KisCoordinatesConverter *, KisCanvas2 *) override;
};

class CanvasWidgetProbe final : public KisCanvasWidgetBase
{
public:
    using KisCanvasWidgetBase::KisCanvasWidgetBase;
    using KisCanvasWidgetBase::updateCanvasProjection;

    QWidget *widget() override;
    void setDisplayFilter(QSharedPointer<KisDisplayFilter>) override;
    void notifyImageColorSpaceChanged(const KoColorSpace *) override;
    bool wrapAroundViewingMode() const override;
    WrapAroundAxis wrapAroundViewingModeAxis() const override;
    void channelSelectionChanged(const QBitArray &) override;
    void setDisplayConfig(const KisDisplayConfig &) override;
    void finishResizingImage(qint32, qint32) override;
    KisUpdateInfoSP startUpdateCanvasProjection(const QRect &) override;
    QRect updateCanvasProjection(KisUpdateInfoSP) override;
    void updateCanvasImage(const QRect &) override;
    void updateCanvasDecorations(const QRect &) override;
    bool isBusy() const override;
    void setLodResetInProgress(bool) override;

protected:
    bool callFocusNextPrevChild(bool) override;
};

class KisPaintingAssistantsDecorationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasDecorationTypeAndControlSchemaRemainStable();
    void canvasWidgetBaseTypeConstructionAndLifetimeSchemaRemainStable();
    void canvasWidgetBaseCanvasConnectionSignaturesRemainStable();
    void canvasWidgetBaseDecorationCollectionSignaturesRemainStable();
    void canvasWidgetBaseViewingAndProjectionSignaturesRemainStable();
    void canvasWidgetBaseColorAndBitDepthSignaturesRemainStable();
    void assistantEditorDataSchemaRemainStable();
    void assistantDecorationTypeAndCollectionSchemaRemainStable();
    void assistantSelectionVisibilityAndSizeSchemaRemainStable();
    void assistantAdjustmentSnappingColorAndNotificationSchemaRemainStable();
};

void KisPaintingAssistantsDecorationSchemaContractTest::canvasDecorationTypeAndControlSchemaRemainStable()
{
    using Decoration = KisCanvasDecoration;

    static_assert(std::is_same_v<KisCanvasDecorationSP, KisSharedPtr<Decoration>>);
    static_assert(std::is_class_v<Decoration>);
    static_assert(std::is_abstract_v<Decoration>);
    static_assert(std::is_constructible_v<CanvasDecorationProbe, const QString &, QPointer<KisView>>);
    static_assert(std::has_virtual_destructor_v<Decoration>);
    static_assert(
        std::is_same_v<decltype(&Decoration::comparePriority), bool (*)(KisCanvasDecorationSP, KisCanvasDecorationSP)>);
    ASSERT_CANVAS_DECORATION_SIGNATURE(id, const QString &(Decoration::*)() const);
    ASSERT_CANVAS_DECORATION_SIGNATURE(notifyWindowMinimized, void (Decoration::*)(bool));
    ASSERT_CANVAS_DECORATION_SIGNATURE(
        paint,
        void (Decoration::*)(QPainter &, const QRectF &, const KisCoordinatesConverter *, KisCanvas2 *));
    ASSERT_CANVAS_DECORATION_SIGNATURE(priority, int (Decoration::*)() const);
    ASSERT_CANVAS_DECORATION_SIGNATURE(setCanvasWidget, void (Decoration::*)(KisCanvasWidgetBase *));
    ASSERT_CANVAS_DECORATION_SIGNATURE(setView, void (Decoration::*)(QPointer<KisView>));
    ASSERT_CANVAS_DECORATION_SIGNATURE(setVisible, void (Decoration::*)(bool));
    ASSERT_CANVAS_DECORATION_SIGNATURE(toggleVisibility, void (Decoration::*)());
    ASSERT_CANVAS_DECORATION_SIGNATURE(visible, bool (Decoration::*)() const);

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::canvasWidgetBaseTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Widget = KisCanvasWidgetBase;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<KisAbstractCanvasWidget, Widget>);
    static_assert(std::is_abstract_v<Widget>);
    static_assert(std::is_constructible_v<CanvasWidgetProbe, KisCanvas2 *, KisCoordinatesConverter *>);
    static_assert(std::has_virtual_destructor_v<Widget>);

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::canvasWidgetBaseCanvasConnectionSignaturesRemainStable()
{
    using Widget = KisCanvasWidgetBase;

    ASSERT_CANVAS_WIDGET_SIGNATURE(toolProxy, KoToolProxy * (Widget::*)() const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(coordinatesConverter, KisCoordinatesConverter * (Widget::*)() const);

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::canvasWidgetBaseDecorationCollectionSignaturesRemainStable()
{
    using Widget = KisCanvasWidgetBase;
    using DecorationList = QList<KisCanvasDecorationSP>;

    ASSERT_CANVAS_WIDGET_SIGNATURE(addDecoration, void (Widget::*)(KisCanvasDecorationSP));
    ASSERT_CANVAS_WIDGET_SIGNATURE(removeDecoration, void (Widget::*)(const QString &));
    ASSERT_CANVAS_WIDGET_SIGNATURE(decoration, KisCanvasDecorationSP (Widget::*)(const QString &) const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(setDecorations, void (Widget::*)(const DecorationList &));
    ASSERT_CANVAS_WIDGET_SIGNATURE(decorations, DecorationList (Widget::*)() const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(notifyDecorationsWindowMinimized, void (Widget::*)(bool));

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::canvasWidgetBaseViewingAndProjectionSignaturesRemainStable()
{
    using Widget = KisCanvasWidgetBase;

    ASSERT_CANVAS_WIDGET_SIGNATURE(drawDecorations, void (Widget::*)(QPainter &, const QRect &) const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(setWrapAroundViewingMode, void (Widget::*)(bool));
    ASSERT_CANVAS_WIDGET_SIGNATURE(setWrapAroundViewingModeAxis, void (Widget::*)(WrapAroundAxis));
    ASSERT_CANVAS_WIDGET_SIGNATURE(updateCanvasProjection,
                                   QVector<QRect> (Widget::*)(const QVector<KisUpdateInfoSP> &));

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::canvasWidgetBaseColorAndBitDepthSignaturesRemainStable()
{
    using Widget = KisCanvasWidgetBase;

    ASSERT_CANVAS_WIDGET_SIGNATURE(borderColor, QColor (Widget::*)() const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(createCheckersImage, QImage (*)(qint32));
    ASSERT_CANVAS_WIDGET_SIGNATURE(currentBitDepthMode, Widget::BitDepthMode (Widget::*)() const);
    ASSERT_CANVAS_WIDGET_SIGNATURE(currentBitDepthUserReport, QString (Widget::*)() const);

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::assistantEditorDataSchemaRemainStable()
{
    static_assert(std::is_class_v<AssistantEditorData>);
    ASSERT_EDITOR_MEMBER_TYPE(boundingSize, QSize);
    ASSERT_EDITOR_MEMBER_TYPE(buttonPadding, const int);
    ASSERT_EDITOR_MEMBER_TYPE(buttonSize, const int);
    ASSERT_EDITOR_MEMBER_TYPE(buttoncount, unsigned int);
    ASSERT_EDITOR_MEMBER_TYPE(deleteButtonActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(deleteIconPosition, QPointF);
    ASSERT_EDITOR_MEMBER_TYPE(dragDecorationWidth, const int);
    ASSERT_EDITOR_MEMBER_TYPE(duplicateButtonActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(duplicateIconPosition, QPointF);
    ASSERT_EDITOR_MEMBER_TYPE(horizontalButtonLimit, const int);
    ASSERT_EDITOR_MEMBER_TYPE(lockButtonActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(lockedIconPosition, QPointF);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconDelete, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconDuplicate, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconLockOff, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconLockOn, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconMove, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconSnapOff, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(m_iconSnapOn, const QIcon);
    ASSERT_EDITOR_MEMBER_TYPE(moveButtonActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(moveIconPosition, QPointF);
    ASSERT_EDITOR_MEMBER_TYPE(snapButtonActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(snapIconPosition, QPointF);
    ASSERT_EDITOR_MEMBER_TYPE(widgetActivated, bool);
    ASSERT_EDITOR_MEMBER_TYPE(widgetOffset, const int);

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::assistantDecorationTypeAndCollectionSchemaRemainStable()
{
    using Decoration = KisPaintingAssistantsDecoration;
    using AssistantList = QList<KisPaintingAssistantSP>;
    using HandleList = QList<KisPaintingAssistantHandleSP>;

    static_assert(std::is_same_v<KisPaintingAssistantsDecorationSP, KisSharedPtr<Decoration>>);
    static_assert(std::is_class_v<Decoration>);
    static_assert(std::is_base_of_v<KisCanvasDecoration, Decoration>);
    static_assert(std::is_constructible_v<Decoration, QPointer<KisView>>);
    static_assert(std::has_virtual_destructor_v<Decoration>);
    static_assert(std::is_same_v<decltype(Decoration::globalEditorWidgetData), AssistantEditorData>);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(addAssistant, void (Decoration::*)(KisPaintingAssistantSP));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(raiseAssistant, void (Decoration::*)(KisPaintingAssistantSP));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(removeAssistant, void (Decoration::*)(KisPaintingAssistantSP));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(removeAll, void (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setAssistants, void (Decoration::*)(const AssistantList &));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(handles, HandleList (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(assistants, AssistantList (Decoration::*)() const);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(hasPaintableAssistants, bool (Decoration::*)() const);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(uncache, void (Decoration::*)());

    QVERIFY(true);
}

void KisPaintingAssistantsDecorationSchemaContractTest::assistantSelectionVisibilityAndSizeSchemaRemainStable()
{
    using Decoration = KisPaintingAssistantsDecoration;

    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(selectedAssistant, KisPaintingAssistantSP (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setSelectedAssistant, void (Decoration::*)(KisPaintingAssistantSP));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(deselectAssistant, void (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(activateAssistantsEditor, void (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(deactivateAssistantsEditor, void (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(isEditingAssistants, bool (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setAssistantVisible, void (Decoration::*)(bool));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setOutlineVisible, void (Decoration::*)(bool));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setOnlyOneAssistantSnap, void (Decoration::*)(bool));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setEraserSnap, void (Decoration::*)(bool));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(assistantVisibility, bool (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(outlineVisibility, bool (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(handleSize, int (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setHandleSize, void (Decoration::*)(int));

    QVERIFY(true);
}

// clang-format off
void KisPaintingAssistantsDecorationSchemaContractTest::assistantAdjustmentSnappingColorAndNotificationSchemaRemainStable()
{
    using Decoration = KisPaintingAssistantsDecoration;
    using NoArgument = void (Decoration::*)();

    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(globalAssistantsColor, QColor (Decoration::*)());
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setGlobalAssistantsColor, void (Decoration::*)(QColor));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(adjustPosition, QPointF (Decoration::*)(const QPointF &, const QPointF &));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(adjustLine, void (Decoration::*)(QPointF &, QPointF &));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(setAdjustedBrushPosition, void (Decoration::*)(QPointF));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(endStroke, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(snapToGuide,
                                           QPointF (Decoration::*)(KoPointerEvent *, const QPointF &, bool));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(snapToGuide, QPointF (Decoration::*)(const QPointF &, const QPointF &));
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(assistantChanged, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(selectedAssistantChanged, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(toggleAssistantVisible, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(toggleOutlineVisible, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(slotUpdateDecorationVisibility, NoArgument);
    ASSERT_ASSISTANTS_DECORATION_SIGNATURE(slotConfigChanged, NoArgument);

    QVERIFY(true);
}
// clang-format on

QTEST_GUILESS_MAIN(KisPaintingAssistantsDecorationSchemaContractTest)

#include "KisPaintingAssistantsDecorationSchemaContractTest.moc"
