/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commands_new/KisLayerCollapseCommand.h"
#include "commands_new/KisLayerCollapseCommandNodeAccess_p.h"

#include <QTest>

namespace
{

struct NodeToken {
    int references = 0;
    QString name;
    bool collapsed = false;
    int nameReads = 0;
    int collapsedReads = 0;
    QVector<bool> writes;
};

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
    safeAssertions.clear();
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

QString KisLayerCollapseCommandNodeAccess::name(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    token->nameReads++;
    return token->name;
}

bool KisLayerCollapseCommandNodeAccess::collapsed(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    token->collapsedReads++;
    return token->collapsed;
}

void KisLayerCollapseCommandNodeAccess::setCollapsed(KisNode *node, bool value)
{
    NodeToken *const token = nodeToken(node);
    token->collapsed = value;
    token->writes.append(value);
}

class KisLayerCollapseCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void explicitValuesDispatchRedoUndoAndRetainParentedNode();
    void convenienceConstructorReadsOldValueOnce();
    void sameNodeConsecutiveCommandsMergeFirstOldWithLastNew();
    void differentTypesNodesAndDiscontinuousValuesAreRejected();
};

void KisLayerCollapseCommandContractTest::explicitValuesDispatchRedoUndoAndRetainParentedNode()
{
    resetObservations();
    NodeToken token{0, QStringLiteral("Layer"), false, 0, 0, {}};
    KisNode *const nodeAddress = nodePointer(&token);

    {
        KisNodeSP node(nodeAddress);
        KUndo2Command parent;
        auto *command = new KisLayerCollapseCommand(node, false, true, &parent);
        node.clear();

        QCOMPARE(parent.childCount(), 1);
        QCOMPARE(parent.child(0), command);
        QCOMPARE(token.references, 1);
        QCOMPARE(token.nameReads, 1);
        QCOMPARE(token.collapsedReads, 0);

        command->redo();
        QVERIFY(token.collapsed);
        command->undo();
        QVERIFY(!token.collapsed);
        QCOMPARE(token.writes, QVector<bool>({true, false}));
        QVERIFY(safeAssertions.isEmpty());
    }

    QCOMPARE(token.references, 0);
}

void KisLayerCollapseCommandContractTest::convenienceConstructorReadsOldValueOnce()
{
    resetObservations();
    NodeToken token{0, QStringLiteral("Layer"), true, 0, 0, {}};

    {
        KisNodeSP node(nodePointer(&token));
        KisLayerCollapseCommand command(node, false);

        QCOMPARE(token.nameReads, 1);
        QCOMPARE(token.collapsedReads, 1);
        token.collapsed = false;

        command.redo();
        command.undo();
        QVERIFY(token.collapsed);
        QCOMPARE(token.collapsedReads, 1);
        QCOMPARE(token.writes, QVector<bool>({false, true}));
        QVERIFY(safeAssertions.isEmpty());
    }

    QCOMPARE(token.references, 0);
}

void KisLayerCollapseCommandContractTest::sameNodeConsecutiveCommandsMergeFirstOldWithLastNew()
{
    resetObservations();
    NodeToken token{0, QStringLiteral("Layer"), false, 0, 0, {}};

    {
        KisNodeSP node(nodePointer(&token));
        KisLayerCollapseCommand first(node, false, false);
        KisLayerCollapseCommand second(node, false, true);
        node.clear();

        QCOMPARE(token.references, 2);
        QVERIFY(first.mergeWith(&second));

        first.redo();
        first.undo();
        QCOMPARE(token.writes, QVector<bool>({true, false}));
        QVERIFY(safeAssertions.isEmpty());
    }

    QCOMPARE(token.references, 0);
}

void KisLayerCollapseCommandContractTest::differentTypesNodesAndDiscontinuousValuesAreRejected()
{
    resetObservations();
    NodeToken firstToken{0, QStringLiteral("First"), false, 0, 0, {}};
    NodeToken secondToken{0, QStringLiteral("Second"), false, 0, 0, {}};
    KisNodeSP firstNode(nodePointer(&firstToken));
    KisNodeSP secondNode(nodePointer(&secondToken));
    KisLayerCollapseCommand first(firstNode, false, true);
    KisLayerCollapseCommand differentNode(secondNode, true, false);
    KisLayerCollapseCommand discontinuous(firstNode, false, true);
    KUndo2Command otherType;

    QVERIFY(!first.mergeWith(&differentNode));
    QVERIFY(!first.mergeWith(&otherType));
    QVERIFY(!first.mergeWith(&discontinuous));
    QCOMPARE(safeAssertions, QStringList({QStringLiteral("m_newValue == cmd->m_oldValue")}));
    QVERIFY(firstToken.writes.isEmpty());
    QVERIFY(secondToken.writes.isEmpty());
}

QTEST_GUILESS_MAIN(KisLayerCollapseCommandContractTest)

#include "KisLayerCollapseCommandContractTest.moc"
