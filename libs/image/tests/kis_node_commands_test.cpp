/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_commands_test.h"

#include <QTest>

#include <commands/kis_node_command.h>

namespace
{

struct NodeToken {
    int references = 0;
};

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

class InspectableNodeCommand : public KisNodeCommand
{
public:
    using KisNodeCommand::KisNodeCommand;

    bool hasNode() const
    {
        return !m_node.isNull();
    }
};

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

void kisSharedPtrAddReference(KisNode *node)
{
    nodeToken(node)->references++;
}

bool kisSharedPtrRelease(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    Q_ASSERT(token->references > 0);
    token->references--;

    return token->references > 0;
}

void KisNodeCommandsTest::constructorPreservesTextAndNullNode()
{
    InspectableNodeCommand command(kundo2_noi18n(QStringLiteral("Node Contract")), KisNodeSP());

    QCOMPARE(command.text().toString(), QStringLiteral("Node Contract"));
    QVERIFY(!command.hasNode());
}

void KisNodeCommandsTest::destructorReleasesHeldNodeReference()
{
    NodeToken token;
    KisNodeSP externalNode(nodePointer(&token));
    QCOMPARE(token.references, 1);

    KUndo2Command *command =
        new InspectableNodeCommand(kundo2_noi18n(QStringLiteral("Node Lifetime Contract")), externalNode);
    QCOMPARE(token.references, 2);

    delete command;
    QCOMPARE(token.references, 1);

    externalNode.clear();
    QCOMPARE(token.references, 0);
}

QTEST_GUILESS_MAIN(KisNodeCommandsTest)
