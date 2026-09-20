/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commands/KisNodeRenameCommand.h"
#include "commands/KisNodeRenameCommandNodeAccess_p.h"

#include <kis_command_ids.h>

#include <QTest>

namespace
{

struct NodeToken {
    int references = 0;
};

struct NameApplication {
    KisNode *node;
    QString name;
};

QVector<NameApplication> nameApplications;
QStringList safeAssertions;

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

void resetObservations()
{
    nameApplications.clear();
    safeAssertions.clear();
}

void compareApplication(int index, KisNode *node, const QString &name)
{
    QCOMPARE(nameApplications.at(index).node, node);
    QCOMPARE(nameApplications.at(index).name, name);
}

} // namespace

void kisSharedPtrAddReference(KisNode *node)
{
    nodeToken(node)->references++;
}

bool kisSharedPtrRelease(KisNode *node)
{
    nodeToken(node)->references--;
    return true;
}

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    safeAssertions.append(QString::fromLatin1(assertion));
}

void KisNodeRenameCommandNodeAccess::setName(KisNode *node, const QString &name)
{
    nameApplications.append({node, name});
}

class KisNodeRenameCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void redoUndoDispatchNamesAndKeepNodeAlive();
    void idUsesStableRenameIdentifier();
    void sameNodeCommandsMergeFromOriginalToFinalName();
    void differentNodesAndOtherCommandsCannotMerge();
    void discontinuousNamesAssertButStillMerge();
};

void KisNodeRenameCommandContractTest::redoUndoDispatchNamesAndKeepNodeAlive()
{
    resetObservations();
    NodeToken token;
    KisNode *const nodeAddress = nodePointer(&token);

    {
        KisNodeSP node(nodeAddress);
        KisNodeRenameCommand command(node, QStringLiteral("Before"), QStringLiteral("After"));
        node.clear();

        QCOMPARE(token.references, 1);
        command.redo();
        command.undo();

        QCOMPARE(nameApplications.size(), 2);
        compareApplication(0, nodeAddress, QStringLiteral("After"));
        compareApplication(1, nodeAddress, QStringLiteral("Before"));
        QVERIFY(safeAssertions.isEmpty());
    }

    QCOMPARE(token.references, 0);
}

void KisNodeRenameCommandContractTest::idUsesStableRenameIdentifier()
{
    resetObservations();
    KisNodeRenameCommand command(KisNodeSP(), QStringLiteral("Before"), QStringLiteral("After"));

    QCOMPARE(command.id(), static_cast<int>(KisCommandUtils::ChangeNodeNameId));
    QVERIFY(nameApplications.isEmpty());
    QVERIFY(safeAssertions.isEmpty());
}

void KisNodeRenameCommandContractTest::sameNodeCommandsMergeFromOriginalToFinalName()
{
    resetObservations();
    NodeToken token;
    KisNode *const nodeAddress = nodePointer(&token);

    {
        KisNodeSP node(nodeAddress);
        KisNodeRenameCommand first(node, QStringLiteral("First"), QStringLiteral("Second"));
        KisNodeRenameCommand second(node, QStringLiteral("Second"), QStringLiteral("Third"));
        node.clear();

        QCOMPARE(token.references, 2);
        QVERIFY(first.canMergeWith(&second));
        QVERIFY(first.mergeWith(&second));

        first.redo();
        first.undo();
        QCOMPARE(nameApplications.size(), 2);
        compareApplication(0, nodeAddress, QStringLiteral("Third"));
        compareApplication(1, nodeAddress, QStringLiteral("First"));
        QVERIFY(safeAssertions.isEmpty());
    }

    QCOMPARE(token.references, 0);
}

void KisNodeRenameCommandContractTest::differentNodesAndOtherCommandsCannotMerge()
{
    resetObservations();
    NodeToken firstToken;
    NodeToken secondToken;
    KisNodeSP firstNode(nodePointer(&firstToken));
    KisNodeSP secondNode(nodePointer(&secondToken));
    KisNodeRenameCommand first(firstNode, QStringLiteral("A"), QStringLiteral("B"));
    KisNodeRenameCommand second(secondNode, QStringLiteral("B"), QStringLiteral("C"));
    KUndo2Command otherCommand;

    QVERIFY(!first.canMergeWith(&second));
    QVERIFY(!first.mergeWith(&second));
    QVERIFY(!first.canMergeWith(&otherCommand));
    QVERIFY(!first.mergeWith(&otherCommand));
    QVERIFY(nameApplications.isEmpty());
    QVERIFY(safeAssertions.isEmpty());
}

void KisNodeRenameCommandContractTest::discontinuousNamesAssertButStillMerge()
{
    resetObservations();
    NodeToken token;
    KisNode *const nodeAddress = nodePointer(&token);
    KisNodeSP node(nodeAddress);
    KisNodeRenameCommand first(node, QStringLiteral("Original"), QStringLiteral("Expected"));
    KisNodeRenameCommand second(node, QStringLiteral("Unexpected"), QStringLiteral("Final"));

    QVERIFY(first.canMergeWith(&second));
    QVERIFY(first.mergeWith(&second));
    QCOMPARE(safeAssertions.size(), 1);

    first.redo();
    first.undo();
    QCOMPARE(nameApplications.size(), 2);
    compareApplication(0, nodeAddress, QStringLiteral("Final"));
    compareApplication(1, nodeAddress, QStringLiteral("Original"));
}

QTEST_GUILESS_MAIN(KisNodeRenameCommandContractTest)

#include "KisNodeRenameCommandContractTest.moc"
