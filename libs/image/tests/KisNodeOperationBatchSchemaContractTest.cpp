/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_node_operation_batch.h>

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_NODE_OPERATION_BATCH_MEMBER(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeOperationBatch::method)), signature>)

} // namespace

class KisNodeOperationBatchSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeOperationBatchTypeLifetimeAndStateSchemaRemainStable();
    void nodeOperationBatchHierarchyOrderingSignaturesRemainStable();
    void nodeOperationBatchDestinationMutationSignaturesRemainStable();
    void nodeOperationBatchGroupingAndMergeSignaturesRemainStable();
    void nodeOperationBatchCompletionAndNotificationSignaturesRemainStable();
};

void KisNodeOperationBatchSchemaContractTest::nodeOperationBatchTypeLifetimeAndStateSchemaRemainStable()
{
    using Batch = KisNodeOperationBatch;

    static_assert(std::is_class_v<Batch>);
    static_assert(std::is_base_of_v<QObject, Batch>);
    static_assert(std::has_virtual_destructor_v<Batch>);
    static_assert(
        std::is_same_v<
            decltype(Batch(std::declval<const KUndo2MagicString &>(), std::declval<KisImageSP>(), std::declval<int>())),
            Batch>);
    ASSERT_NODE_OPERATION_BATCH_MEMBER(setAutoDelete, void (Batch::*)(bool));
    ASSERT_NODE_OPERATION_BATCH_MEMBER(isEnded, bool (Batch::*)() const);

    QVERIFY(true);
}

void KisNodeOperationBatchSchemaContractTest::nodeOperationBatchHierarchyOrderingSignaturesRemainStable()
{
    using Batch = KisNodeOperationBatch;
    using HierarchyOperation = void (Batch::*)(const KisNodeList &, KisNodeSP);

    ASSERT_NODE_OPERATION_BATCH_MEMBER(addNode, void (Batch::*)(const KisNodeList &, KisNodeSP, KisNodeSP, KisNodeSP));
    ASSERT_NODE_OPERATION_BATCH_MEMBER(removeNode, HierarchyOperation);
    ASSERT_NODE_OPERATION_BATCH_MEMBER(duplicateNode, HierarchyOperation);
    ASSERT_NODE_OPERATION_BATCH_MEMBER(raiseNode, HierarchyOperation);
    ASSERT_NODE_OPERATION_BATCH_MEMBER(lowerNode, HierarchyOperation);

    QVERIFY(true);
}

void KisNodeOperationBatchSchemaContractTest::nodeOperationBatchDestinationMutationSignaturesRemainStable()
{
    using Batch = KisNodeOperationBatch;
    using MultiNodeOperation = void (Batch::*)(const KisNodeList &, KisNodeSP, KisNodeSP, KisNodeSP);

    ASSERT_NODE_OPERATION_BATCH_MEMBER(moveNode, void (Batch::*)(KisNodeSP, KisNodeSP, KisNodeSP));
    ASSERT_NODE_OPERATION_BATCH_MEMBER(moveNode, MultiNodeOperation);
    ASSERT_NODE_OPERATION_BATCH_MEMBER(copyNode, MultiNodeOperation);

    QVERIFY(true);
}

void KisNodeOperationBatchSchemaContractTest::nodeOperationBatchGroupingAndMergeSignaturesRemainStable()
{
    using Batch = KisNodeOperationBatch;

    ASSERT_NODE_OPERATION_BATCH_MEMBER(
        createGroup,
        bool (Batch::*)(const KisNodeList &, KisNodeSP, const QString &, KisNodeSP *, KisNodeSP *));
    ASSERT_NODE_OPERATION_BATCH_MEMBER(ungroupNodes,
                                       bool (Batch::*)(const KisNodeList &, KisNodeSP, KisNodeSP *, KisNodeSP *));
    ASSERT_NODE_OPERATION_BATCH_MEMBER(canMergeAction, bool (Batch::*)(const KUndo2MagicString &) const);

    QVERIFY(true);
}

void KisNodeOperationBatchSchemaContractTest::nodeOperationBatchCompletionAndNotificationSignaturesRemainStable()
{
    using Batch = KisNodeOperationBatch;

    ASSERT_NODE_OPERATION_BATCH_MEMBER(end, void (Batch::*)());
    ASSERT_NODE_OPERATION_BATCH_MEMBER(requestUpdateAsyncFromCommand, void (Batch::*)());

    QVERIFY(true);
}

#undef ASSERT_NODE_OPERATION_BATCH_MEMBER

QTEST_APPLESS_MAIN(KisNodeOperationBatchSchemaContractTest)

#include "KisNodeOperationBatchSchemaContractTest.moc"
