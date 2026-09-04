/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPathToolSelection.h>
#include <KoSelection.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SELECTION_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoSelection::method)), signature>)
#define ASSERT_PATH_TOOL_SELECTION_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathToolSelection::method)), signature>)
} // namespace

class KoSelectionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void selectionIdentityAndLifecycleSignaturesRemainStable();
    void selectionGeometryAndActiveLayerSignaturesRemainStable();
    void selectionMembershipQuerySignaturesRemainStable();
    void selectionFilteredShapeViewSignaturesRemainStable();
    void selectionMutationObservationAndPaintingSignaturesRemainStable();
    void pathToolSelectionTypeAndLifetimeSchemaRemainStable();
    void pathToolSelectionMutationSignaturesRemainStable();
    void pathToolSelectionQuerySignaturesRemainStable();
    void pathToolSelectionShapeSynchronizationSignaturesRemainStable();
    void pathToolSelectionPaintingAndNotificationSignaturesRemainStable();
};

void KoSelectionSchemaContractTest::selectionIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoSelection>);
    static_assert(std::is_base_of_v<QObject, KoSelection>);
    static_assert(std::is_base_of_v<KoShape, KoSelection>);
    static_assert(std::is_base_of_v<KoShape::ShapeChangeListener, KoSelection>);
    static_assert(std::is_constructible_v<KoSelection>);
    static_assert(std::is_constructible_v<KoSelection, QObject *>);
    static_assert(std::has_virtual_destructor_v<KoSelection>);
}

void KoSelectionSchemaContractTest::selectionGeometryAndActiveLayerSignaturesRemainStable()
{
    ASSERT_SELECTION_SIGNATURE(activeLayer, KoShapeLayer * (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(setActiveLayer, void (KoSelection::*)(KoShapeLayer *));
    ASSERT_SELECTION_SIGNATURE(currentLayerChanged, void (KoSelection::*)(const KoShapeLayer *));
    ASSERT_SELECTION_SIGNATURE(boundingRect, QRectF (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(outlineRect, QRectF (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(size, QSizeF (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(setSize, void (KoSelection::*)(const QSizeF &));
}

void KoSelectionSchemaContractTest::selectionMembershipQuerySignaturesRemainStable()
{
    ASSERT_SELECTION_SIGNATURE(count, int (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(firstSelectedShape, KoShape * (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(isSelected, bool (KoSelection::*)(const KoShape *) const);
    ASSERT_SELECTION_SIGNATURE(hitTest, bool (KoSelection::*)(const QPointF &) const);
}

void KoSelectionSchemaContractTest::selectionFilteredShapeViewSignaturesRemainStable()
{
    ASSERT_SELECTION_SIGNATURE(selectedShapes, const QList<KoShape *> (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(selectedVisibleShapes, const QList<KoShape *> (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(selectedEditableShapes, const QList<KoShape *> (KoSelection::*)() const);
    ASSERT_SELECTION_SIGNATURE(selectedEditableShapesAndDelegates, const QList<KoShape *> (KoSelection::*)() const);
}

void KoSelectionSchemaContractTest::selectionMutationObservationAndPaintingSignaturesRemainStable()
{
    ASSERT_SELECTION_SIGNATURE(select, void (KoSelection::*)(KoShape *));
    ASSERT_SELECTION_SIGNATURE(deselect, void (KoSelection::*)(KoShape *));
    ASSERT_SELECTION_SIGNATURE(deselectAll, void (KoSelection::*)());
    ASSERT_SELECTION_SIGNATURE(notifyShapeChanged, void (KoSelection::*)(KoShape::ChangeType, KoShape *));
    ASSERT_SELECTION_SIGNATURE(paint, void (KoSelection::*)(QPainter &) const);
    ASSERT_SELECTION_SIGNATURE(selectionChanged, void (KoSelection::*)());
}

void KoSelectionSchemaContractTest::pathToolSelectionTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPathToolSelection>);
    static_assert(std::is_base_of_v<KoToolSelection, KoPathToolSelection>);
    static_assert(std::is_base_of_v<KoPathShape::PointSelectionChangeListener, KoPathToolSelection>);
    static_assert(std::is_constructible_v<KoPathToolSelection, KoPathTool *>);
    static_assert(std::has_virtual_destructor_v<KoPathToolSelection>);
}

void KoSelectionSchemaContractTest::pathToolSelectionMutationSignaturesRemainStable()
{
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(add, void (KoPathToolSelection::*)(KoPathPoint *, bool));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(remove, void (KoPathToolSelection::*)(KoPathPoint *));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(clear, void (KoPathToolSelection::*)());
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectPoints, void (KoPathToolSelection::*)(const QRectF &, bool));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectAll, void (KoPathToolSelection::*)());
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(update, void (KoPathToolSelection::*)());
}

void KoSelectionSchemaContractTest::pathToolSelectionQuerySignaturesRemainStable()
{
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(contains, bool (KoPathToolSelection::*)(KoPathPoint *));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(hasSelection, bool (KoPathToolSelection::*)());
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(objectCount, int (KoPathToolSelection::*)() const);
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(size, int (KoPathToolSelection::*)() const);
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectedPoints, const QSet<KoPathPoint *> &(KoPathToolSelection::*)() const);
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectedPointsData, QList<KoPathPointData> (KoPathToolSelection::*)() const);
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectedSegmentsData, QList<KoPathPointData> (KoPathToolSelection::*)() const);
}

void KoSelectionSchemaContractTest::pathToolSelectionShapeSynchronizationSignaturesRemainStable()
{
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectedShapes, QList<KoPathShape *> (KoPathToolSelection::*)() const);
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(setSelectedShapes, void (KoPathToolSelection::*)(QList<KoPathShape *>));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(recommendPointSelectionChange,
                                         void (KoPathToolSelection::*)(KoPathShape *, const QList<KoPathPointIndex> &));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(notifyPathPointsChanged, void (KoPathToolSelection::*)(KoPathShape *));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(notifyShapeChanged,
                                         void (KoPathToolSelection::*)(KoShape::ChangeType, KoShape *));
}

void KoSelectionSchemaContractTest::pathToolSelectionPaintingAndNotificationSignaturesRemainStable()
{
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(
        paint,
        void (KoPathToolSelection::*)(QPainter &, const KoViewConverter &, qreal, KoColorDisplayRendererInterface *));
    ASSERT_PATH_TOOL_SELECTION_SIGNATURE(selectionChanged, void (KoPathToolSelection::*)());
}

QTEST_APPLESS_MAIN(KoSelectionSchemaContractTest)

#include "KoSelectionSchemaContractTest.moc"
