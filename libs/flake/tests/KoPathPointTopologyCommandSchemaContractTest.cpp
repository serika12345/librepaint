/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoMultiPathPointMergeCommand.h>
#include <commands/KoPathPointInsertCommand.h>
#include <commands/KoPathPointMergeCommand.h>
#include <commands/KoPathPointRemoveCommand.h>

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

QTEST_APPLESS_MAIN(KoPathPointTopologyCommandSchemaContractTest)

#include "KoPathPointTopologyCommandSchemaContractTest.moc"
