/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/kis_node_commands_adapter.h>
#include <commands_new/KisMergeLabeledLayersCommand.h>

#include <utility>

#include <QTest>

namespace
{


} // namespace

class KisNodeCommandsAdapterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mergeLabeledLayersReferenceNodeInfoValuesRemainStable();
    void mergeLabeledLayersReferenceNodeListAndOwnershipRemainStable();
};

void KisNodeCommandsAdapterSchemaContractTest::mergeLabeledLayersReferenceNodeInfoValuesRemainStable()
{
    using ReferenceNodeInfo = KisMergeLabeledLayersCommand::ReferenceNodeInfo;


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

QTEST_APPLESS_MAIN(KisNodeCommandsAdapterSchemaContractTest)

#include "KisNodeCommandsAdapterSchemaContractTest.moc"
