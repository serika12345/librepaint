/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasBase.h>
#include <KoShapeManager.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CANVAS_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCanvasBase::method)), signature>)
#define ASSERT_SHAPE_MANAGER_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeManager::method)), signature>)

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
    void shapeManagerTypeLifetimeAndRepaintSchemaRemainStable();
    void shapeManagerPaintJobValueSchemaRemainStable();
    void shapeManagerPaintOrderingAndRenderingSignaturesRemainStable();
    void shapeManagerShapeAndSelectionSignaturesRemainStable();
    void shapeManagerQueryUpdateAndInterfaceSignaturesRemainStable();
    void canvasIdentityAndLifecycleSignaturesRemainStable();
    void canvasControllerAndResourceSignaturesRemainStable();
    void canvasShapeCommandAndObserverSignaturesRemainStable();
    void canvasWidgetAndViewSignaturesRemainStable();
    void canvasGridAndPresentationSignaturesRemainStable();
};

void KoCanvasBaseSchemaContractTest::shapeManagerTypeLifetimeAndRepaintSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeManager>);
    static_assert(std::is_base_of_v<QObject, KoShapeManager>);
    static_assert(std::has_virtual_destructor_v<KoShapeManager>);
    static_assert(std::is_constructible_v<KoShapeManager, KoCanvasBase *>);
    static_assert(std::is_constructible_v<KoShapeManager, KoCanvasBase *, const QList<KoShape *> &>);
    static_assert(std::is_destructible_v<KoShapeManager>);

    static_assert(std::is_enum_v<KoShapeManager::Repaint>);
    static_assert(KoShapeManager::PaintShapeOnAdd == 0);
    static_assert(KoShapeManager::AddWithoutRepaint == 1);
}

void KoCanvasBaseSchemaContractTest::shapeManagerPaintJobValueSchemaRemainStable()
{
    using PaintJob = KoShapeManager::PaintJob;

    static_assert(std::is_class_v<PaintJob>);
    static_assert(std::is_same_v<PaintJob::ShapesStorage, std::vector<std::unique_ptr<KoShape>>>);
    static_assert(std::is_same_v<PaintJob::SharedSafeStorage, std::shared_ptr<PaintJob::ShapesStorage>>);
    static_assert(std::is_default_constructible_v<PaintJob>);
    static_assert(std::is_constructible_v<PaintJob, QRectF, QRect>);
    static_assert(std::is_same_v<decltype(static_cast<bool (PaintJob::*)() const>(&PaintJob::isEmpty)),
                                 bool (PaintJob::*)() const>);
    static_assert(std::is_same_v<decltype(PaintJob::docUpdateRect), QRectF>);
    static_assert(std::is_same_v<decltype(PaintJob::viewUpdateRect), QRect>);
    static_assert(std::is_same_v<decltype(PaintJob::shapes), QList<KoShape *>>);
    static_assert(std::is_same_v<decltype(PaintJob::allClonedShapes), PaintJob::SharedSafeStorage>);
}

void KoCanvasBaseSchemaContractTest::shapeManagerPaintOrderingAndRenderingSignaturesRemainStable()
{
    using PaintJob = KoShapeManager::PaintJob;
    using PaintJobsOrder = KoShapeManager::PaintJobsOrder;

    static_assert(std::is_class_v<PaintJobsOrder>);
    static_assert(std::is_same_v<decltype(PaintJobsOrder::uncroppedViewUpdateRect), QRect>);
    static_assert(std::is_same_v<decltype(PaintJobsOrder::jobs), QList<PaintJob>>);
    static_assert(std::is_same_v<decltype(static_cast<void (PaintJobsOrder::*)()>(&PaintJobsOrder::clear)),
                                 void (PaintJobsOrder::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<bool (PaintJobsOrder::*)() const>(&PaintJobsOrder::isEmpty)),
                                 bool (PaintJobsOrder::*)() const>);
    ASSERT_SHAPE_MANAGER_SIGNATURE(preparePaintJobs, void (KoShapeManager::*)(PaintJobsOrder &, KoShape *));
    ASSERT_SHAPE_MANAGER_SIGNATURE(paintJob, void (KoShapeManager::*)(QPainter &, const PaintJob &));
    ASSERT_SHAPE_MANAGER_SIGNATURE(paint, void (KoShapeManager::*)(QPainter &));
    ASSERT_SHAPE_MANAGER_SIGNATURE(renderSingleShape, void (*)(KoShape *, QPainter &));
}

void KoCanvasBaseSchemaContractTest::shapeManagerShapeAndSelectionSignaturesRemainStable()
{
    ASSERT_SHAPE_MANAGER_SIGNATURE(setShapes,
                                   void (KoShapeManager::*)(const QList<KoShape *> &, KoShapeManager::Repaint));
    ASSERT_SHAPE_MANAGER_SIGNATURE(shapes, QList<KoShape *> (KoShapeManager::*)() const);
    ASSERT_SHAPE_MANAGER_SIGNATURE(topLevelShapes, QList<KoShape *> (KoShapeManager::*)() const);
    ASSERT_SHAPE_MANAGER_SIGNATURE(addShape, void (KoShapeManager::*)(KoShape *, KoShapeManager::Repaint));
    ASSERT_SHAPE_MANAGER_SIGNATURE(remove, void (KoShapeManager::*)(KoShape *));
    ASSERT_SHAPE_MANAGER_SIGNATURE(selection, KoSelection * (KoShapeManager::*)() const);
    ASSERT_SHAPE_MANAGER_SIGNATURE(selectionChanged, void (KoShapeManager::*)());
    ASSERT_SHAPE_MANAGER_SIGNATURE(selectionContentChanged, void (KoShapeManager::*)());
    ASSERT_SHAPE_MANAGER_SIGNATURE(contentChanged, void (KoShapeManager::*)());

    static_assert(
        std::is_same_v<decltype(std::declval<KoShapeManager &>().setShapes(std::declval<const QList<KoShape *> &>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoShapeManager &>().addShape(static_cast<KoShape *>(nullptr))), void>);
}

void KoCanvasBaseSchemaContractTest::shapeManagerQueryUpdateAndInterfaceSignaturesRemainStable()
{
    using ShapeInterface = KoShapeManager::ShapeInterface;

    ASSERT_SHAPE_MANAGER_SIGNATURE(explicitlyIssueShapeChangedSignals, void (KoShapeManager::*)());
    ASSERT_SHAPE_MANAGER_SIGNATURE(shapeAt,
                                   KoShape * (KoShapeManager::*)(const QPointF &, KoFlake::ShapeSelection, bool));
    ASSERT_SHAPE_MANAGER_SIGNATURE(shapesAt, QList<KoShape *> (KoShapeManager::*)(const QRectF &, bool, bool));
    ASSERT_SHAPE_MANAGER_SIGNATURE(update, void (KoShapeManager::*)(const QRectF &, const KoShape *, bool));
    ASSERT_SHAPE_MANAGER_SIGNATURE(setUpdatesBlocked, void (KoShapeManager::*)(bool));
    ASSERT_SHAPE_MANAGER_SIGNATURE(updatesBlocked, bool (KoShapeManager::*)() const);
    ASSERT_SHAPE_MANAGER_SIGNATURE(notifyShapeChanged, void (KoShapeManager::*)(KoShape *));
    static_assert(std::is_class_v<ShapeInterface>);
    static_assert(std::is_constructible_v<ShapeInterface, KoShapeManager *>);
    static_assert(std::is_same_v<decltype(static_cast<void (ShapeInterface::*)(KoShape *)>(
                                     &ShapeInterface::notifyShapeDestructed)),
                                 void (ShapeInterface::*)(KoShape *)>);
    ASSERT_SHAPE_MANAGER_SIGNATURE(shapeInterface, ShapeInterface * (KoShapeManager::*)());
    ASSERT_SHAPE_MANAGER_SIGNATURE(forwardUpdate, void (KoShapeManager::*)());

    static_assert(
        std::is_same_v<decltype(std::declval<KoShapeManager &>().shapeAt(std::declval<const QPointF &>())), KoShape *>);
    static_assert(std::is_same_v<decltype(std::declval<KoShapeManager &>().shapesAt(std::declval<const QRectF &>())),
                                 QList<KoShape *>>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoShapeManager &>().update(std::declval<const QRectF &>())), void>);
}

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
