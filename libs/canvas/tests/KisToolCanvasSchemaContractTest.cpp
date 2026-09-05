/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisToolCanvas.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_TOOL_CANVAS_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisToolCanvas::method)), signature>)

} // namespace

class KisToolCanvasSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolCanvasTypeLifetimeAndContextSchemaRemainStable();
    void toolCanvasSelectionStateAndCursorSignaturesRemainStable();
    void toolCanvasInputBindingSignaturesRemainStable();
    void toolCanvasStrokeControlSignaturesRemainStable();
    void toolCanvasPresentationSignaturesRemainStable();
};

void KisToolCanvasSchemaContractTest::toolCanvasTypeLifetimeAndContextSchemaRemainStable()
{
    using Canvas = KisToolCanvas;

    static_assert(std::is_class_v<Canvas>);
    static_assert(std::is_abstract_v<Canvas>);
    static_assert(std::has_virtual_destructor_v<Canvas>);
    ASSERT_TOOL_CANVAS_SIGNATURE(coordinatesConverter, const KisCoordinatesConverter *(Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(currentImage, KisImageWSP (Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(toolConfigNotifier, QObject * (Canvas::*)() const);

    QVERIFY(true);
}

void KisToolCanvasSchemaContractTest::toolCanvasSelectionStateAndCursorSignaturesRemainStable()
{
    using Canvas = KisToolCanvas;

    ASSERT_TOOL_CANVAS_SIGNATURE(currentSelectionForTool, KisSelectionSP (Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(selectedNodesForTool, KisNodeList (Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(selectionEditableForTool, bool (Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(selectionModifierMappingSwapsCtrlAndAltForTool, bool (Canvas::*)() const);
    ASSERT_TOOL_CANVAS_SIGNATURE(moveSelectionCursorForTool, QCursor (Canvas::*)() const);

    QVERIFY(true);
}

void KisToolCanvasSchemaContractTest::toolCanvasInputBindingSignaturesRemainStable()
{
    using Canvas = KisToolCanvas;
    using EventFilterConnection = std::function<void(QObject *, bool, int)>;
    using CanvasWidgetChangedCallback = std::function<void()>;

    ASSERT_TOOL_CANVAS_SIGNATURE(attachPriorityEventFilterForTool, void (Canvas::*)(QObject *, int));
    ASSERT_TOOL_CANVAS_SIGNATURE(detachPriorityEventFilterForTool, void (Canvas::*)(QObject *));
    ASSERT_TOOL_CANVAS_SIGNATURE(setInputEventFilterConnection, void (Canvas::*)(EventFilterConnection));
    ASSERT_TOOL_CANVAS_SIGNATURE(setInputCanvasWidgetChangedCallback, void (Canvas::*)(CanvasWidgetChangedCallback));
    ASSERT_TOOL_CANVAS_SIGNATURE(inputActionGroupsMaskInterface,
                                 KisInputActionGroupsMaskInterface::SharedInterface (Canvas::*)());
    static_assert(
        std::is_same_v<decltype(std::declval<Canvas &>().attachPriorityEventFilterForTool(std::declval<QObject *>())),
                       void>);

    QVERIFY(true);
}

void KisToolCanvasSchemaContractTest::toolCanvasStrokeControlSignaturesRemainStable()
{
    using Canvas = KisToolCanvas;

    ASSERT_TOOL_CANVAS_SIGNATURE(requestStrokeEndForTool, void (Canvas::*)());
    ASSERT_TOOL_CANVAS_SIGNATURE(requestStrokeCancellationForTool, void (Canvas::*)());
    ASSERT_TOOL_CANVAS_SIGNATURE(blockUntilOperationsFinishedForTool, bool (Canvas::*)(KisImageSP));
    ASSERT_TOOL_CANVAS_SIGNATURE(blockUntilOperationsFinishedForToolForced, void (Canvas::*)(KisImageSP));

    QVERIFY(true);
}

void KisToolCanvasSchemaContractTest::toolCanvasPresentationSignaturesRemainStable()
{
    using Canvas = KisToolCanvas;

    ASSERT_TOOL_CANVAS_SIGNATURE(showToolMessage, void (Canvas::*)(const QString &, const QString &));
    ASSERT_TOOL_CANVAS_SIGNATURE(drawToolOutline, void (Canvas::*)(QPainter *, const KisOptimizedBrushOutline &, int));

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisToolCanvasSchemaContractTest)

#include "KisToolCanvasSchemaContractTest.moc"
