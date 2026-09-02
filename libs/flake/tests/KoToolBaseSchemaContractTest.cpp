/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoToolBase.h>
#include <KoToolFactoryBase.h>
#include <KoToolRegistry.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TOOL_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolBase::method)), signature>)
#define ASSERT_TOOL_FACTORY_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolFactoryBase::method)), signature>)

class ToolConstructorProbe : public KoToolBase
{
public:
    explicit ToolConstructorProbe(KoCanvasBase *canvas)
        : KoToolBase(canvas)
    {
    }
};

class ToolFactoryConstructorProbe final : public KoToolFactoryBase
{
public:
    using KoToolFactoryBase::KoToolFactoryBase;

    KoToolBase *createTool(KoCanvasBase *canvas) override;
};
} // namespace

class KoToolBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolIdentityContextAndPresentationSignaturesRemainStable();
    void toolInputFocusDragAndPaintingSignaturesRemainStable();
    void toolSelectionClipboardAndPopupSignaturesRemainStable();
    void toolLifecycleResourceAndStrokeSignaturesRemainStable();
    void toolActivationCursorAndStatusSignalSignaturesRemainStable();
    void toolFactoryTypeAndSectionValuesRemainStable();
    void toolFactoryLifetimeAndCreationSignaturesRemainStable();
    void toolFactoryIdentityAndPresentationSignaturesRemainStable();
    void toolRegistryTypeAndLifetimeSchemaRemainsStable();
    void toolRegistryAccessSignatureRemainsStable();
};

void KoToolBaseSchemaContractTest::toolIdentityContextAndPresentationSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoToolBase>);
    static_assert(std::is_abstract_v<KoToolBase>);
    static_assert(std::is_base_of_v<QObject, KoToolBase>);
    static_assert(std::has_virtual_destructor_v<KoToolBase>);
    static_assert(std::is_abstract_v<ToolConstructorProbe>);
    ASSERT_TOOL_SIGNATURE(factory, KoToolFactoryBase * (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(canvas, KoCanvasBase * (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(toolId, QString (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(cursor, QCursor (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(decorationsRect, QRectF (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(decorationThickness, int (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(wantsAutoScroll, bool (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(optionWidgets, QList<QPointer<QWidget>> (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(action, QAction * (KoToolBase::*)(const QString &) const);
}

void KoToolBaseSchemaContractTest::toolInputFocusDragAndPaintingSignaturesRemainStable()
{
    ASSERT_TOOL_SIGNATURE(paint, void (KoToolBase::*)(QPainter &, const KoViewConverter &));
    ASSERT_TOOL_SIGNATURE(mousePressEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(mouseDoubleClickEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(mouseTripleClickEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(mouseMoveEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(mouseReleaseEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(customPressEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(customReleaseEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(customMoveEvent, void (KoToolBase::*)(KoPointerEvent *));
    ASSERT_TOOL_SIGNATURE(keyPressEvent, void (KoToolBase::*)(QKeyEvent *));
    ASSERT_TOOL_SIGNATURE(keyReleaseEvent, void (KoToolBase::*)(QKeyEvent *));
    ASSERT_TOOL_SIGNATURE(inputMethodQuery, QVariant (KoToolBase::*)(Qt::InputMethodQuery) const);
    ASSERT_TOOL_SIGNATURE(inputMethodEvent, void (KoToolBase::*)(QInputMethodEvent *));
    ASSERT_TOOL_SIGNATURE(focusInEvent, void (KoToolBase::*)(QFocusEvent *));
    ASSERT_TOOL_SIGNATURE(focusOutEvent, void (KoToolBase::*)(QFocusEvent *));
    ASSERT_TOOL_SIGNATURE(dragMoveEvent, void (KoToolBase::*)(QDragMoveEvent *, const QPointF &));
    ASSERT_TOOL_SIGNATURE(dragLeaveEvent, void (KoToolBase::*)(QDragLeaveEvent *));
    ASSERT_TOOL_SIGNATURE(dropEvent, void (KoToolBase::*)(QDropEvent *, const QPointF &));
}

void KoToolBaseSchemaContractTest::toolSelectionClipboardAndPopupSignaturesRemainStable()
{
    ASSERT_TOOL_SIGNATURE(selection, KoToolSelection * (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(hasSelection, bool (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(copy, void (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(deleteSelection, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(cut, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(paste, bool (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(selectAll, bool (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(deselect, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(popupActionsMenu, QMenu * (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(popupWidget, KisPopupWidgetInterface * (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(isInTextMode, bool (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(isOpacityPresetMode, bool (KoToolBase::*)() const);
}

void KoToolBaseSchemaContractTest::toolLifecycleResourceAndStrokeSignaturesRemainStable()
{
    ASSERT_TOOL_SIGNATURE(activate, void (KoToolBase::*)(const QSet<KoShape *> &));
    ASSERT_TOOL_SIGNATURE(deactivate, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(canvasResourceChanged, void (KoToolBase::*)(int, const QVariant &));
    ASSERT_TOOL_SIGNATURE(documentResourceChanged, void (KoToolBase::*)(int, const QVariant &));
    ASSERT_TOOL_SIGNATURE(explicitUserStrokeEndRequest, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(maskSyntheticEvents, bool (KoToolBase::*)() const);
    ASSERT_TOOL_SIGNATURE(requestUndoDuringStroke, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(requestRedoDuringStroke, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(requestStrokeCancellation, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(requestStrokeEnd, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(repaintDecorations, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(updateOptionsWidgetIcons, void (KoToolBase::*)());
    ASSERT_TOOL_SIGNATURE(setStatusText, void (KoToolBase::*)(const QString &));
}

void KoToolBaseSchemaContractTest::toolActivationCursorAndStatusSignalSignaturesRemainStable()
{
    ASSERT_TOOL_SIGNATURE(activateTool, void (KoToolBase::*)(const QString &));
    ASSERT_TOOL_SIGNATURE(cursorChanged, void (KoToolBase::*)(const QCursor &));
    ASSERT_TOOL_SIGNATURE(selectionChanged, void (KoToolBase::*)(bool));
    ASSERT_TOOL_SIGNATURE(statusTextChanged, void (KoToolBase::*)(const QString &));
    ASSERT_TOOL_SIGNATURE(textModeChanged, void (KoToolBase::*)(bool));
}

void KoToolBaseSchemaContractTest::toolFactoryTypeAndSectionValuesRemainStable()
{
    static_assert(std::is_class_v<KoToolFactoryBase>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Main), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Shape), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Transform), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Fill), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::View), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Select), const QString>);
    static_assert(std::is_same_v<decltype(ToolBoxSection::Navigation), const QString>);

    QCOMPARE(ToolBoxSection::Main, QStringLiteral("main"));
    QCOMPARE(ToolBoxSection::Shape, QStringLiteral("0 Krita/Shape"));
    QCOMPARE(ToolBoxSection::Transform, QStringLiteral("2 Krita/Transform"));
    QCOMPARE(ToolBoxSection::Fill, QStringLiteral("3 Krita/Fill"));
    QCOMPARE(ToolBoxSection::View, QStringLiteral("4 Krita/View"));
    QCOMPARE(ToolBoxSection::Select, QStringLiteral("5 Krita/Select"));
    QCOMPARE(ToolBoxSection::Navigation, QStringLiteral("navigation"));
}

void KoToolBaseSchemaContractTest::toolFactoryLifetimeAndCreationSignaturesRemainStable()
{
    using Factory = KoToolFactoryBase;

    static_assert(std::is_abstract_v<Factory>);
    static_assert(std::is_base_of_v<QObject, Factory>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_constructible_v<ToolFactoryConstructorProbe, const QString &>);
    ASSERT_TOOL_FACTORY_SIGNATURE(createActions, QList<QAction *> (Factory::*)(KisKActionCollection *));
    ASSERT_TOOL_FACTORY_SIGNATURE(createTool, KoToolBase * (Factory::*)(KoCanvasBase *));
}

void KoToolBaseSchemaContractTest::toolFactoryIdentityAndPresentationSignaturesRemainStable()
{
    using Factory = KoToolFactoryBase;

    ASSERT_TOOL_FACTORY_SIGNATURE(id, QString (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(priority, int (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(section, QString (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(toolTip, QString (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(iconName, QString (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(activationShapeId, QString (Factory::*)() const);
    ASSERT_TOOL_FACTORY_SIGNATURE(shortcut, QKeySequence (Factory::*)() const);
}

void KoToolBaseSchemaContractTest::toolRegistryTypeAndLifetimeSchemaRemainsStable()
{
    using Registry = KoToolRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<KoGenericRegistry<KoToolFactoryBase *>, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
}

void KoToolBaseSchemaContractTest::toolRegistryAccessSignatureRemainsStable()
{
    using Registry = KoToolRegistry;
    using InstanceSignature = Registry *(*)();

    static_assert(std::is_same_v<decltype(static_cast<InstanceSignature>(&Registry::instance)), InstanceSignature>);
}

QTEST_APPLESS_MAIN(KoToolBaseSchemaContractTest)

#include "KoToolBaseSchemaContractTest.moc"
