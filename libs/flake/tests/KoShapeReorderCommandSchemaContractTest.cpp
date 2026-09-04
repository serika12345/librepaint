/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeGroup.h>
#include <commands/KoPathPointTypeCommand.h>
#include <commands/KoPathSegmentTypeCommand.h>
#include <commands/KoShapeGroupCommand.h>
#include <commands/KoShapeReorderCommand.h>
#include <commands/KoShapeUngroupCommand.h>

#include <QDebug>
#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using IndexedShape = KoShapeReorderCommand::IndexedShape;
using MoveShapeType = KoShapeReorderCommand::MoveShapeType;
using ShapeList = QList<KoShape *>;

#define ASSERT_REORDER_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeReorderCommand::method)), signature>)
#define ASSERT_PATH_POINT_TYPE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathPointTypeCommand::method)), signature>)
#define ASSERT_PATH_SEGMENT_TYPE_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathSegmentTypeCommand::method)), signature>)
#define ASSERT_SHAPE_GROUP_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeGroup::method)), signature>)
#define ASSERT_SHAPE_GROUP_COMMAND_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeGroupCommand::method)), signature>)
#define ASSERT_SHAPE_UNGROUP_COMMAND_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeUngroupCommand::method)), signature>)
} // namespace

class KoShapeReorderCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pathPointTypeCommandTypeAndLifetimeSchemaRemainStable();
    void pathPointTypeValuesRemainStable();
    void pathPointTypeExecutionSignaturesRemainStable();
    void pathSegmentTypeCommandTypeLifetimeAndValuesRemainStable();
    void pathSegmentTypeExecutionSignaturesRemainStable();
    void shapeReorderIdentityAndLifecycleSignaturesRemainStable();
    void indexedShapeValueSchemaRemainsStable();
    void shapeReorderMoveTypeValuesRemainStable();
    void shapeReorderCreationAndMergeSignaturesRemainStable();
    void shapeReorderNormalizationAndExecutionSignaturesRemainStable();
    void shapeGroupTypeLifetimeAndCloneSchemaRemainStable();
    void shapeGroupGeometryAndRenderingSignaturesRemainStable();
    void shapeGroupCommandTypeLifetimeAndConstructionSchemaRemainStable();
    void shapeGroupCommandCreationAndExecutionSignaturesRemainStable();
    void shapeUngroupCommandSchemaRemainStable();
};

void KoShapeReorderCommandSchemaContractTest::pathPointTypeCommandTypeAndLifetimeSchemaRemainStable()
{
    using Command = KoPathPointTypeCommand;
    using PointType = Command::PointType;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_enum_v<PointType>);
    static_assert(std::is_constructible_v<Command, const QList<KoPathPointData> &, PointType>);
    static_assert(std::is_constructible_v<Command, const QList<KoPathPointData> &, PointType, KUndo2Command *>);
}

void KoShapeReorderCommandSchemaContractTest::pathPointTypeValuesRemainStable()
{
    static_assert(int(KoPathPointTypeCommand::Corner) == 0);
    static_assert(int(KoPathPointTypeCommand::Smooth) == 1);
    static_assert(int(KoPathPointTypeCommand::Symmetric) == 2);
    static_assert(int(KoPathPointTypeCommand::Line) == 3);
    static_assert(int(KoPathPointTypeCommand::Curve) == 4);
}

void KoShapeReorderCommandSchemaContractTest::pathPointTypeExecutionSignaturesRemainStable()
{
    ASSERT_PATH_POINT_TYPE_SIGNATURE(redo, void (KoPathPointTypeCommand::*)());
    ASSERT_PATH_POINT_TYPE_SIGNATURE(undo, void (KoPathPointTypeCommand::*)());
    ASSERT_PATH_POINT_TYPE_SIGNATURE(makeCubicPointSmooth, void (*)(KoPathPoint *));
}

void KoShapeReorderCommandSchemaContractTest::pathSegmentTypeCommandTypeLifetimeAndValuesRemainStable()
{
    using Command = KoPathSegmentTypeCommand;
    using SegmentType = Command::SegmentType;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_enum_v<SegmentType>);
    static_assert(int(Command::Curve) == 1);
    static_assert(int(Command::Line) == 2);
    static_assert(std::is_constructible_v<Command, const KoPathPointData &, SegmentType>);
    static_assert(std::is_constructible_v<Command, const KoPathPointData &, SegmentType, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, const QList<KoPathPointData> &, SegmentType>);
    static_assert(std::is_constructible_v<Command, const QList<KoPathPointData> &, SegmentType, KUndo2Command *>);
}

void KoShapeReorderCommandSchemaContractTest::pathSegmentTypeExecutionSignaturesRemainStable()
{
    ASSERT_PATH_SEGMENT_TYPE_SIGNATURE(redo, void (KoPathSegmentTypeCommand::*)());
    ASSERT_PATH_SEGMENT_TYPE_SIGNATURE(undo, void (KoPathSegmentTypeCommand::*)());
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeReorderCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoShapeReorderCommand>);
    static_assert(std::has_virtual_destructor_v<KoShapeReorderCommand>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const ShapeList &, QList<int> &>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const ShapeList &, QList<int> &, KUndo2Command *>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const QList<IndexedShape> &>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const QList<IndexedShape> &, KUndo2Command *>);
}

void KoShapeReorderCommandSchemaContractTest::indexedShapeValueSchemaRemainsStable()
{
    static_assert(std::is_class_v<IndexedShape>);
    static_assert(std::is_same_v<decltype(IndexedShape::zIndex), int>);
    static_assert(std::is_same_v<decltype(IndexedShape::shape), KoShape *>);
    static_assert(std::is_constructible_v<IndexedShape>);
    static_assert(std::is_constructible_v<IndexedShape, KoShape *>);
    static_assert(std::is_same_v<decltype(static_cast<bool (IndexedShape::*)(const IndexedShape &) const>(
                                     &IndexedShape::operator<)),
                                 bool (IndexedShape::*)(const IndexedShape &) const>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const IndexedShape &)>(&operator<<)),
                                 QDebug (*)(QDebug, const IndexedShape &)>);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderMoveTypeValuesRemainStable()
{
    static_assert(std::is_enum_v<MoveShapeType>);
    QCOMPARE(int(KoShapeReorderCommand::RaiseShape), 0);
    QCOMPARE(int(KoShapeReorderCommand::LowerShape), 1);
    QCOMPARE(int(KoShapeReorderCommand::BringToFront), 2);
    QCOMPARE(int(KoShapeReorderCommand::SendToBack), 3);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderCreationAndMergeSignaturesRemainStable()
{
    ASSERT_REORDER_SIGNATURE(createCommand,
                             KoShapeReorderCommand
                                 * (*)(const ShapeList &, KoShapeManager *, MoveShapeType, KUndo2Command *));
    ASSERT_REORDER_SIGNATURE(mergeInShape, KoShapeReorderCommand * (*)(ShapeList, KoShape *, KUndo2Command *));
    ASSERT_REORDER_SIGNATURE(mergeDownShapes, QList<IndexedShape> (*)(ShapeList, ShapeList));

    static_assert(std::is_same_v<decltype(KoShapeReorderCommand::createCommand(std::declval<const ShapeList &>(),
                                                                               std::declval<KoShapeManager *>(),
                                                                               std::declval<MoveShapeType>())),
                                 KoShapeReorderCommand *>);
    static_assert(std::is_same_v<decltype(KoShapeReorderCommand::mergeInShape(std::declval<ShapeList>(),
                                                                              std::declval<KoShape *>())),
                                 KoShapeReorderCommand *>);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderNormalizationAndExecutionSignaturesRemainStable()
{
    ASSERT_REORDER_SIGNATURE(homogenizeZIndexes, QList<IndexedShape> (*)(QList<IndexedShape>));
    ASSERT_REORDER_SIGNATURE(homogenizeZIndexesLazy, QList<IndexedShape> (*)(QList<IndexedShape>));
    ASSERT_REORDER_SIGNATURE(redo, void (KoShapeReorderCommand::*)());
    ASSERT_REORDER_SIGNATURE(undo, void (KoShapeReorderCommand::*)());
}

void KoShapeReorderCommandSchemaContractTest::shapeGroupTypeLifetimeAndCloneSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeGroup>);
    static_assert(std::is_base_of_v<KoShapeContainer, KoShapeGroup>);
    static_assert(std::is_default_constructible_v<KoShapeGroup>);
    static_assert(std::has_virtual_destructor_v<KoShapeGroup>);
    ASSERT_SHAPE_GROUP_SIGNATURE(cloneShape, KoShape * (KoShapeGroup::*)() const);
}

void KoShapeReorderCommandSchemaContractTest::shapeGroupGeometryAndRenderingSignaturesRemainStable()
{
    ASSERT_SHAPE_GROUP_SIGNATURE(paintComponent, void (KoShapeGroup::*)(QPainter &) const);
    ASSERT_SHAPE_GROUP_SIGNATURE(hitTest, bool (KoShapeGroup::*)(const QPointF &) const);
    ASSERT_SHAPE_GROUP_SIGNATURE(size, QSizeF (KoShapeGroup::*)() const);
    ASSERT_SHAPE_GROUP_SIGNATURE(setSize, void (KoShapeGroup::*)(const QSizeF &));
    ASSERT_SHAPE_GROUP_SIGNATURE(outlineRect, QRectF (KoShapeGroup::*)() const);
    ASSERT_SHAPE_GROUP_SIGNATURE(boundingRect, QRectF (KoShapeGroup::*)() const);
}

void KoShapeReorderCommandSchemaContractTest::shapeGroupCommandTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeGroupCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoShapeGroupCommand>);
    static_assert(std::has_virtual_destructor_v<KoShapeGroupCommand>);
    static_assert(std::is_constructible_v<KoShapeGroupCommand, KoShapeContainer *, const ShapeList &, bool>);
    static_assert(
        std::is_constructible_v<KoShapeGroupCommand, KoShapeContainer *, const ShapeList &, bool, KUndo2Command *>);
    static_assert(std::is_constructible_v<KoShapeGroupCommand, KoShapeContainer *, const ShapeList &>);
    static_assert(std::is_constructible_v<KoShapeGroupCommand, KoShapeContainer *, const ShapeList &, KUndo2Command *>);
}

void KoShapeReorderCommandSchemaContractTest::shapeGroupCommandCreationAndExecutionSignaturesRemainStable()
{
    ASSERT_SHAPE_GROUP_COMMAND_SIGNATURE(createCommand,
                                         KoShapeGroupCommand * (*)(KoShapeContainer *, const ShapeList &, bool));
    ASSERT_SHAPE_GROUP_COMMAND_SIGNATURE(redo, void (KoShapeGroupCommand::*)());
    ASSERT_SHAPE_GROUP_COMMAND_SIGNATURE(undo, void (KoShapeGroupCommand::*)());

    static_assert(std::is_same_v<decltype(KoShapeGroupCommand::createCommand(std::declval<KoShapeContainer *>(),
                                                                             std::declval<const ShapeList &>())),
                                 KoShapeGroupCommand *>);
}

void KoShapeReorderCommandSchemaContractTest::shapeUngroupCommandSchemaRemainStable()
{
    static_assert(std::is_class_v<KoShapeUngroupCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoShapeUngroupCommand>);
    static_assert(std::has_virtual_destructor_v<KoShapeUngroupCommand>);
    static_assert(std::is_constructible_v<KoShapeUngroupCommand, KoShapeContainer *, const ShapeList &>);
    static_assert(
        std::is_constructible_v<KoShapeUngroupCommand, KoShapeContainer *, const ShapeList &, const ShapeList &>);
    static_assert(std::is_constructible_v<KoShapeUngroupCommand,
                                          KoShapeContainer *,
                                          const ShapeList &,
                                          const ShapeList &,
                                          KUndo2Command *>);
    ASSERT_SHAPE_UNGROUP_COMMAND_SIGNATURE(redo, void (KoShapeUngroupCommand::*)());
    ASSERT_SHAPE_UNGROUP_COMMAND_SIGNATURE(undo, void (KoShapeUngroupCommand::*)());
}

QTEST_APPLESS_MAIN(KoShapeReorderCommandSchemaContractTest)

#include "KoShapeReorderCommandSchemaContractTest.moc"
