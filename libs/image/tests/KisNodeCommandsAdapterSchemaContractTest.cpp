/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_node_commands_adapter.h>
#include <commands_new/KisMergeLabeledLayersCommand.h>

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
    void mergeLabeledLayersTypeConstructionAndLifetimeSchemaRemainStable();
    void mergeLabeledLayersGroupSelectionPolicyValuesRemainStable();
    void mergeLabeledLayersReferenceNodeInfoValuesRemainStable();
    void mergeLabeledLayersReferenceNodeListAndOwnershipRemainStable();
    void mergeLabeledLayersExecutionSignaturesRemainStable();
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

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Command = KisMergeLabeledLayersCommand;
    using Policy = Command::GroupSelectionPolicy;
    using ReferenceListSP = Command::ReferenceNodeInfoListSP;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageSP, KisPaintDeviceSP, QList<int>>);
    static_assert(std::is_constructible_v<Command, KisImageSP, KisPaintDeviceSP, QList<int>, Policy>);
    static_assert(std::is_constructible_v<Command,
                                          KisImageSP,
                                          ReferenceListSP,
                                          ReferenceListSP,
                                          KisPaintDeviceSP,
                                          KisPaintDeviceSP,
                                          QList<int>>);
    static_assert(std::is_constructible_v<Command,
                                          KisImageSP,
                                          ReferenceListSP,
                                          ReferenceListSP,
                                          KisPaintDeviceSP,
                                          KisPaintDeviceSP,
                                          QList<int>,
                                          Policy,
                                          bool,
                                          KisNodeSP>);
    static_assert(std::is_destructible_v<Command>);

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersGroupSelectionPolicyValuesRemainStable()
{
    using Command = KisMergeLabeledLayersCommand;
    using Policy = Command::GroupSelectionPolicy;

    static_assert(std::is_enum_v<Policy>);
    static_assert(Command::GroupSelectionPolicy_SelectAlways == 0);
    static_assert(Command::GroupSelectionPolicy_SelectIfColorLabeled == 1);
    static_assert(Command::GroupSelectionPolicy_NeverSelect == 2);

    QVERIFY(true);
}

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersReferenceNodeInfoValuesRemainStable()
{
    using ReferenceNodeInfo = KisMergeLabeledLayersCommand::ReferenceNodeInfo;
    using EqualitySignature = bool (ReferenceNodeInfo::*)(const ReferenceNodeInfo &) const;

    static_assert(std::is_class_v<ReferenceNodeInfo>);
    static_assert(std::is_same_v<decltype(ReferenceNodeInfo::nodeId), QUuid>);
    static_assert(std::is_same_v<decltype(ReferenceNodeInfo::sequenceNumber), int>);
    static_assert(std::is_same_v<decltype(ReferenceNodeInfo::opacity), int>);
    static_assert(std::is_same_v<decltype(&ReferenceNodeInfo::operator==), EqualitySignature>);

    const ReferenceNodeInfo reference{QUuid(QStringLiteral("{82ac94b1-ca78-42b5-b542-6b730f676b1e}")), 7, 173};
    const ReferenceNodeInfo same = reference;
    QVERIFY(reference == same);

    ReferenceNodeInfo changed = same;
    changed.nodeId = QUuid(QStringLiteral("{bf784033-baf8-4de2-9e59-5bd8a6ba5096}"));
    QVERIFY(!(reference == changed));
    changed = same;
    changed.sequenceNumber = 8;
    QVERIFY(!(reference == changed));
    changed = same;
    changed.opacity = 174;
    QVERIFY(!(reference == changed));
}

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersReferenceNodeListAndOwnershipRemainStable()
{
    using Command = KisMergeLabeledLayersCommand;
    using ReferenceNodeInfo = Command::ReferenceNodeInfo;
    using ReferenceList = Command::ReferenceNodeInfoList;
    using ReferenceListSP = Command::ReferenceNodeInfoListSP;

    static_assert(std::is_same_v<ReferenceList, QList<ReferenceNodeInfo>>);
    static_assert(std::is_same_v<ReferenceListSP, QSharedPointer<ReferenceList>>);

    const ReferenceNodeInfo first{QUuid(QStringLiteral("{bd667c0a-994d-4f83-9884-c2aa8ae8aa97}")), 11, 128};
    const ReferenceNodeInfo second{QUuid(QStringLiteral("{76bb1178-4c79-4574-9f4a-47089f13c721}")), 12, 255};
    const ReferenceList values{first, second};
    QCOMPARE(values.size(), 2);
    QVERIFY(values.at(0) == first);
    QVERIFY(values.at(1) == second);

    ReferenceListSP owner = ReferenceListSP::create(values);
    const ReferenceListSP shared = owner;
    owner.reset();
    QCOMPARE(shared->size(), 2);
    QVERIFY(shared->at(0) == first);
    QVERIFY(shared->at(1) == second);
}

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersExecutionSignaturesRemainStable()
{
    using Command = KisMergeLabeledLayersCommand;
    using ExecutionSignature = void (Command::*)();
    using CreateReferenceDeviceSignature = KisPaintDeviceSP (*)(KisImageSP, QString);

    static_assert(std::is_same_v<decltype(&Command::undo), ExecutionSignature>);
    static_assert(std::is_same_v<decltype(&Command::redo), ExecutionSignature>);
    static_assert(std::is_same_v<decltype(&Command::createRefPaintDevice), CreateReferenceDeviceSignature>);
    static_assert(
        std::is_same_v<decltype(Command::createRefPaintDevice(std::declval<KisImageSP>())), KisPaintDeviceSP>);

    QVERIFY(true);
}

#undef ASSERT_NODE_COMMANDS_ADAPTER_MEMBER

QTEST_APPLESS_MAIN(KisNodeCommandsAdapterSchemaContractTest)

#include "KisNodeCommandsAdapterSchemaContractTest.moc"
