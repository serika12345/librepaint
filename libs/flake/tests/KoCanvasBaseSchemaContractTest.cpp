/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasBase.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_CANVAS_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCanvasBase::method)), signature>)

class CanvasConstructorProbe : public KoCanvasBase
{
protected:
    explicit CanvasConstructorProbe(KoShapeControllerBase *shapeController)
        : KoCanvasBase(shapeController)
    {
    }
};
} // namespace

class KoCanvasBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasIdentityAndLifecycleSignaturesRemainStable();
    void canvasControllerAndResourceSignaturesRemainStable();
    void canvasShapeCommandAndObserverSignaturesRemainStable();
    void canvasWidgetAndViewSignaturesRemainStable();
    void canvasGridAndPresentationSignaturesRemainStable();
};

void KoCanvasBaseSchemaContractTest::canvasIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoCanvasBase>);
    static_assert(std::is_abstract_v<KoCanvasBase>);
    static_assert(std::is_base_of_v<QObject, KoCanvasBase>);
    static_assert(std::has_virtual_destructor_v<KoCanvasBase>);
    static_assert(std::is_abstract_v<CanvasConstructorProbe>);
}

void KoCanvasBaseSchemaContractTest::canvasControllerAndResourceSignaturesRemainStable()
{
    ASSERT_CANVAS_SIGNATURE(canvasController, KoCanvasController * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(displayRendererInterface, KoColorDisplayRendererInterface * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(resourceManager, KoCanvasResourceProvider * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(setCanvasController, void (KoCanvasBase::*)(KoCanvasController *));
    ASSERT_CANVAS_SIGNATURE(shapeController, KoShapeController * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(snapGuide, KoSnapGuide * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(toolProxy, KoToolProxy * (KoCanvasBase::*)() const);
}

void KoCanvasBaseSchemaContractTest::canvasShapeCommandAndObserverSignaturesRemainStable()
{
    ASSERT_CANVAS_SIGNATURE(addCommand, void (KoCanvasBase::*)(KUndo2Command *));
    ASSERT_CANVAS_SIGNATURE(currentShapeManagerOwnerShape, KoShape * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(disconnectCanvasObserver, void (KoCanvasBase::*)(QObject *));
    ASSERT_CANVAS_SIGNATURE(selectedShapesProxy, KoSelectedShapesProxy * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(setCurrentShapeManagerOwnerShape, void (KoCanvasBase::*)(KoShape *));
    ASSERT_CANVAS_SIGNATURE(shapeManager, KoShapeManager * (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(updateCanvas, void (KoCanvasBase::*)(const QRectF &));
}

void KoCanvasBaseSchemaContractTest::canvasWidgetAndViewSignaturesRemainStable()
{
    ASSERT_CANVAS_SIGNATURE(canvasIsOpenGL, bool (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(canvasWidget, QWidget * (KoCanvasBase::*)());
    ASSERT_CANVAS_SIGNATURE(canvasWidget, const QWidget *(KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(viewConverter, KoViewConverter * (KoCanvasBase::*)());
    ASSERT_CANVAS_SIGNATURE(viewConverter, const KoViewConverter *(KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(viewToDocument, QPointF (KoCanvasBase::*)(const QPointF &) const);
}

void KoCanvasBaseSchemaContractTest::canvasGridAndPresentationSignaturesRemainStable()
{
    ASSERT_CANVAS_SIGNATURE(clipToDocument, void (KoCanvasBase::*)(const KoShape *, QPointF &) const);
    ASSERT_CANVAS_SIGNATURE(documentOrigin, QPoint (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(gridSize, void (KoCanvasBase::*)(QPointF *, QSizeF *) const);
    ASSERT_CANVAS_SIGNATURE(setCursor, void (KoCanvasBase::*)(const QCursor &));
    ASSERT_CANVAS_SIGNATURE(snapToGrid, bool (KoCanvasBase::*)() const);
    ASSERT_CANVAS_SIGNATURE(unit, KoUnit (KoCanvasBase::*)() const);
}

QTEST_APPLESS_MAIN(KoCanvasBaseSchemaContractTest)

#include "KoCanvasBaseSchemaContractTest.moc"
