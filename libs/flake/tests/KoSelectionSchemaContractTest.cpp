/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoSelection.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SELECTION_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoSelection::method)), signature>)
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

QTEST_APPLESS_MAIN(KoSelectionSchemaContractTest)

#include "KoSelectionSchemaContractTest.moc"
