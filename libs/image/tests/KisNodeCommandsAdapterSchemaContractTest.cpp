/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_node_commands_adapter.h>

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeCommandsAdapter::method)), signature>)

} // namespace

class KisNodeCommandsAdapterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeCommandsAdapterTypeLifetimeAndImageBindingSchemaRemainStable();
    void nodeCommandsAdapterCommandTransactionSignaturesRemainStable();
    void nodeCommandsAdapterSynchronousHierarchyMutationSignaturesRemainStable();
    void nodeCommandsAdapterAsynchronousApplicationSignaturesRemainStable();
    void nodeCommandsAdapterNodePropertyMutationSignaturesRemainStable();
};

void KisNodeCommandsAdapterSchemaContractTest::nodeCommandsAdapterTypeLifetimeAndImageBindingSchemaRemainStable()
{
    using Adapter = KisNodeCommandsAdapter;

    static_assert(std::is_class_v<Adapter>);
    static_assert(std::is_base_of_v<QObject, Adapter>);
    static_assert(std::is_default_constructible_v<Adapter>);
    static_assert(std::is_constructible_v<Adapter, KisImageWSP, QObject *>);
    static_assert(std::has_virtual_destructor_v<Adapter>);
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(setImage, void (Adapter::*)(KisImageWSP));

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::nodeCommandsAdapterCommandTransactionSignaturesRemainStable()
{
    using Adapter = KisNodeCommandsAdapter;

    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(addExtraCommand, void (Adapter::*)(KUndo2Command *));
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(beginMacro, void (Adapter::*)(const KUndo2MagicString &));
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(endMacro, void (Adapter::*)());
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(undoLastCommand, void (Adapter::*)());

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::nodeCommandsAdapterSynchronousHierarchyMutationSignaturesRemainStable()
{
    using Adapter = KisNodeCommandsAdapter;
    using Flags = KisImageLayerAddCommand::Flags;

    static_assert(std::is_same_v<decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP, Flags)>(
                                     &Adapter::addNode)),
                                 void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP, Flags)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, quint32, Flags)>(
                                     &Adapter::addNode)),
                                 void (Adapter::*)(KisNodeSP, KisNodeSP, quint32, Flags)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP)>(&Adapter::moveNode)),
                       void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, quint32)>(&Adapter::moveNode)),
                       void (Adapter::*)(KisNodeSP, KisNodeSP, quint32)>);
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(removeNode, void (Adapter::*)(KisNodeSP));
    static_assert(std::is_same_v<decltype(std::declval<Adapter &>().addNode(std::declval<KisNodeSP>(),
                                                                            std::declval<KisNodeSP>(),
                                                                            std::declval<KisNodeSP>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<Adapter &>().addNode(std::declval<KisNodeSP>(),
                                                                            std::declval<KisNodeSP>(),
                                                                            std::declval<quint32>())),
                                 void>);

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::nodeCommandsAdapterAsynchronousApplicationSignaturesRemainStable()
{
    using Adapter = KisNodeCommandsAdapter;
    using Applicator = KisProcessingApplicator;

    static_assert(std::is_same_v<
                  decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP, bool, bool, Applicator *)>(
                      &Adapter::addNodeAsync)),
                  void (Adapter::*)(KisNodeSP, KisNodeSP, KisNodeSP, bool, bool, Applicator *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Adapter::*)(KisNodeSP, KisNodeSP, quint32, bool, bool, Applicator *)>(
                           &Adapter::addNodeAsync)),
                       void (Adapter::*)(KisNodeSP, KisNodeSP, quint32, bool, bool, Applicator *)>);
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(applyOneCommandAsync, void (Adapter::*)(KUndo2Command *, Applicator *));
    static_assert(std::is_same_v<decltype(std::declval<Adapter &>().addNodeAsync(std::declval<KisNodeSP>(),
                                                                                 std::declval<KisNodeSP>(),
                                                                                 std::declval<KisNodeSP>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<Adapter &>().addNodeAsync(std::declval<KisNodeSP>(),
                                                                                 std::declval<KisNodeSP>(),
                                                                                 std::declval<quint32>())),
                                 void>);
    static_assert(
        std::is_same_v<decltype(std::declval<Adapter &>().applyOneCommandAsync(std::declval<KUndo2Command *>())),
                       void>);

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::nodeCommandsAdapterNodePropertyMutationSignaturesRemainStable()
{
    using Adapter = KisNodeCommandsAdapter;

    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(setCompositeOp, void (Adapter::*)(KisNodeSP, const KoCompositeOp *));
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(setNodeName, void (Adapter::*)(KisNodeSP, const QString &));
    ASSERT_NODE_COMMANDS_ADAPTER_MEMBER(setOpacity, void (Adapter::*)(KisNodeSP, qint32));

    QVERIFY(true);
}

#undef ASSERT_NODE_COMMANDS_ADAPTER_MEMBER

QTEST_APPLESS_MAIN(KisNodeCommandsAdapterSchemaContractTest)

#include "KisNodeCommandsAdapterSchemaContractTest.moc"
