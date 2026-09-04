/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoMultiPathPointJoinCommand.h>
#include <commands/KoMultiPathPointMergeCommand.h>
#include <commands/KoPathCombineCommand.h>
#include <commands/KoPathControlPointMoveCommand.h>
#include <commands/KoPathPointInsertCommand.h>
#include <commands/KoPathPointMergeCommand.h>
#include <commands/KoPathPointRemoveCommand.h>
#include <commands/KoShapeUnclipCommand.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using PointDataList = QList<KoPathPointData>;

#define ASSERT_INSERT_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathPointInsertCommand::method)), signature>)
#define ASSERT_REMOVE_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathPointRemoveCommand::method)), signature>)
#define ASSERT_MERGE_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathPointMergeCommand::method)), signature>)
#define ASSERT_MULTI_MERGE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoMultiPathPointMergeCommand::method)), signature>)
#define ASSERT_PATH_COMBINE_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathCombineCommand::method)), signature>)
#define ASSERT_CONTROL_POINT_MOVE_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathControlPointMoveCommand::method)), signature>)
#define ASSERT_SHAPE_UNCLIP_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeUnclipCommand::method)), signature>)
} // namespace

class KoPathPointTopologyCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pathPointTopologyCommandTypeAndLifetimeSchemaRemainStable();
    void pathPointTopologyExecutionSignaturesRemainStable();
    void pathPointInsertionResultSignatureRemainsStable();
    void pathPointRemovalFactorySignatureRemainsStable();
    void pathPointMergeResultSignaturesRemainStable();
    void pathCombineCommandTypeAndConstructionSchemaRemainStable();
    void pathCombineExecutionAndCorrespondenceSignaturesRemainStable();
    void pathControlPointMoveCommandSchemaRemainStable();
    void multiPathPointJoinCommandSchemaRemainStable();
    void shapeUnclipCommandSchemaRemainStable();
};

void KoPathPointTopologyCommandSchemaContractTest::pathPointTopologyCommandTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPathPointInsertCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoPathPointInsertCommand>);
    static_assert(std::has_virtual_destructor_v<KoPathPointInsertCommand>);
    static_assert(std::is_constructible_v<KoPathPointInsertCommand, const PointDataList &, qreal>);
    static_assert(std::is_constructible_v<KoPathPointInsertCommand, const PointDataList &, qreal, KUndo2Command *>);
    static_assert(
        std::is_same_v<decltype(KoPathPointInsertCommand(std::declval<const PointDataList &>(), std::declval<qreal>())),
                       KoPathPointInsertCommand>);

    static_assert(std::is_class_v<KoPathPointRemoveCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoPathPointRemoveCommand>);
    static_assert(std::has_virtual_destructor_v<KoPathPointRemoveCommand>);
    static_assert(std::is_constructible_v<KoPathPointRemoveCommand, const PointDataList &>);
    static_assert(std::is_constructible_v<KoPathPointRemoveCommand, const PointDataList &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoPathPointRemoveCommand(std::declval<const PointDataList &>())),
                                 KoPathPointRemoveCommand>);

    static_assert(std::is_class_v<KoPathPointMergeCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoPathPointMergeCommand>);
    static_assert(std::has_virtual_destructor_v<KoPathPointMergeCommand>);
    static_assert(std::is_constructible_v<KoPathPointMergeCommand, const KoPathPointData &, const KoPathPointData &>);
    static_assert(std::is_constructible_v<KoPathPointMergeCommand,
                                          const KoPathPointData &,
                                          const KoPathPointData &,
                                          KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoPathPointMergeCommand(std::declval<const KoPathPointData &>(),
                                                                  std::declval<const KoPathPointData &>())),
                                 KoPathPointMergeCommand>);

    static_assert(std::is_class_v<KoMultiPathPointMergeCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoMultiPathPointMergeCommand>);
    static_assert(std::has_virtual_destructor_v<KoMultiPathPointMergeCommand>);
    static_assert(std::is_constructible_v<KoMultiPathPointMergeCommand,
                                          const KoPathPointData &,
                                          const KoPathPointData &,
                                          KoShapeControllerBase *,
                                          KoSelection *>);
    static_assert(std::is_constructible_v<KoMultiPathPointMergeCommand,
                                          const KoPathPointData &,
                                          const KoPathPointData &,
                                          KoShapeControllerBase *,
                                          KoSelection *,
                                          KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoMultiPathPointMergeCommand(std::declval<const KoPathPointData &>(),
                                                                       std::declval<const KoPathPointData &>(),
                                                                       std::declval<KoShapeControllerBase *>(),
                                                                       std::declval<KoSelection *>())),
                                 KoMultiPathPointMergeCommand>);
}

void KoPathPointTopologyCommandSchemaContractTest::pathPointTopologyExecutionSignaturesRemainStable()
{
    ASSERT_INSERT_SIGNATURE(redo, void (KoPathPointInsertCommand::*)());
    ASSERT_INSERT_SIGNATURE(undo, void (KoPathPointInsertCommand::*)());
    ASSERT_REMOVE_SIGNATURE(redo, void (KoPathPointRemoveCommand::*)());
    ASSERT_REMOVE_SIGNATURE(undo, void (KoPathPointRemoveCommand::*)());
    ASSERT_MERGE_SIGNATURE(redo, void (KoPathPointMergeCommand::*)());
    ASSERT_MERGE_SIGNATURE(undo, void (KoPathPointMergeCommand::*)());
    ASSERT_MULTI_MERGE_SIGNATURE(redo, void (KoMultiPathPointMergeCommand::*)());
    ASSERT_MULTI_MERGE_SIGNATURE(undo, void (KoMultiPathPointMergeCommand::*)());
}

void KoPathPointTopologyCommandSchemaContractTest::pathPointInsertionResultSignatureRemainsStable()
{
    ASSERT_INSERT_SIGNATURE(insertedPoints, QList<KoPathPoint *> (KoPathPointInsertCommand::*)() const);
}

void KoPathPointTopologyCommandSchemaContractTest::pathPointRemovalFactorySignatureRemainsStable()
{
    ASSERT_REMOVE_SIGNATURE(createCommand,
                            KUndo2Command * (*)(const PointDataList &, KoShapeController *, KUndo2Command *));
    static_assert(std::is_same_v<decltype(KoPathPointRemoveCommand::createCommand(std::declval<const PointDataList &>(),
                                                                                  std::declval<KoShapeController *>())),
                                 KUndo2Command *>);
}

void KoPathPointTopologyCommandSchemaContractTest::pathPointMergeResultSignaturesRemainStable()
{
    ASSERT_MERGE_SIGNATURE(mergedPointData, KoPathPointData (KoPathPointMergeCommand::*)() const);
    ASSERT_MULTI_MERGE_SIGNATURE(testingCombinedPath, KoPathShape * (KoMultiPathPointMergeCommand::*)() const);
}

void KoPathPointTopologyCommandSchemaContractTest::pathCombineCommandTypeAndConstructionSchemaRemainStable()
{
    using PathList = QList<KoPathShape *>;

    static_assert(std::is_class_v<KoPathCombineCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoPathCombineCommand>);
    static_assert(std::is_constructible_v<KoPathCombineCommand, KoShapeControllerBase *, const PathList &>);
    static_assert(
        std::is_constructible_v<KoPathCombineCommand, KoShapeControllerBase *, const PathList &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoPathCombineCommand(std::declval<KoShapeControllerBase *>(),
                                                               std::declval<const PathList &>())),
                                 KoPathCombineCommand>);
    static_assert(std::has_virtual_destructor_v<KoPathCombineCommand>);
}

void KoPathPointTopologyCommandSchemaContractTest::pathCombineExecutionAndCorrespondenceSignaturesRemainStable()
{
    ASSERT_PATH_COMBINE_SIGNATURE(combinedPath, KoPathShape * (KoPathCombineCommand::*)() const);
    ASSERT_PATH_COMBINE_SIGNATURE(originalToCombined, KoPathPointData (KoPathCombineCommand::*)(KoPathPointData) const);
    ASSERT_PATH_COMBINE_SIGNATURE(redo, void (KoPathCombineCommand::*)());
    ASSERT_PATH_COMBINE_SIGNATURE(undo, void (KoPathCombineCommand::*)());
}

void KoPathPointTopologyCommandSchemaContractTest::pathControlPointMoveCommandSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPathControlPointMoveCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoPathControlPointMoveCommand>);
    static_assert(std::is_constructible_v<KoPathControlPointMoveCommand,
                                          const KoPathPointData &,
                                          const QPointF &,
                                          KoPathPoint::PointType>);
    static_assert(std::is_constructible_v<KoPathControlPointMoveCommand,
                                          const KoPathPointData &,
                                          const QPointF &,
                                          KoPathPoint::PointType,
                                          KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoPathControlPointMoveCommand(std::declval<const KoPathPointData &>(),
                                                                        std::declval<const QPointF &>(),
                                                                        std::declval<KoPathPoint::PointType>())),
                                 KoPathControlPointMoveCommand>);
    ASSERT_CONTROL_POINT_MOVE_SIGNATURE(id, int (KoPathControlPointMoveCommand::*)() const);
    ASSERT_CONTROL_POINT_MOVE_SIGNATURE(mergeWith, bool (KoPathControlPointMoveCommand::*)(const KUndo2Command *));
    ASSERT_CONTROL_POINT_MOVE_SIGNATURE(redo, void (KoPathControlPointMoveCommand::*)());
    ASSERT_CONTROL_POINT_MOVE_SIGNATURE(undo, void (KoPathControlPointMoveCommand::*)());
}

void KoPathPointTopologyCommandSchemaContractTest::multiPathPointJoinCommandSchemaRemainStable()
{
    static_assert(std::is_class_v<KoMultiPathPointJoinCommand>);
    static_assert(std::is_base_of_v<KoMultiPathPointMergeCommand, KoMultiPathPointJoinCommand>);
    static_assert(std::is_constructible_v<KoMultiPathPointJoinCommand,
                                          const KoPathPointData &,
                                          const KoPathPointData &,
                                          KoShapeControllerBase *,
                                          KoSelection *>);
    static_assert(std::is_constructible_v<KoMultiPathPointJoinCommand,
                                          const KoPathPointData &,
                                          const KoPathPointData &,
                                          KoShapeControllerBase *,
                                          KoSelection *,
                                          KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoMultiPathPointJoinCommand(std::declval<const KoPathPointData &>(),
                                                                      std::declval<const KoPathPointData &>(),
                                                                      std::declval<KoShapeControllerBase *>(),
                                                                      std::declval<KoSelection *>())),
                                 KoMultiPathPointJoinCommand>);
}

void KoPathPointTopologyCommandSchemaContractTest::shapeUnclipCommandSchemaRemainStable()
{
    using ShapeList = QList<KoShape *>;

    static_assert(std::is_class_v<KoShapeUnclipCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoShapeUnclipCommand>);
    static_assert(std::is_constructible_v<KoShapeUnclipCommand, KoShapeControllerBase *, KoShape *>);
    static_assert(std::is_constructible_v<KoShapeUnclipCommand, KoShapeControllerBase *, KoShape *, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoShapeUnclipCommand(std::declval<KoShapeControllerBase *>(),
                                                               std::declval<KoShape *>())),
                                 KoShapeUnclipCommand>);
    static_assert(std::is_constructible_v<KoShapeUnclipCommand, KoShapeControllerBase *, const ShapeList &>);
    static_assert(
        std::is_constructible_v<KoShapeUnclipCommand, KoShapeControllerBase *, const ShapeList &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoShapeUnclipCommand(std::declval<KoShapeControllerBase *>(),
                                                               std::declval<const ShapeList &>())),
                                 KoShapeUnclipCommand>);
    static_assert(std::has_virtual_destructor_v<KoShapeUnclipCommand>);
    ASSERT_SHAPE_UNCLIP_SIGNATURE(redo, void (KoShapeUnclipCommand::*)());
    ASSERT_SHAPE_UNCLIP_SIGNATURE(undo, void (KoShapeUnclipCommand::*)());
}

QTEST_APPLESS_MAIN(KoPathPointTopologyCommandSchemaContractTest)

#include "KoPathPointTopologyCommandSchemaContractTest.moc"
