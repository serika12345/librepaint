/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNodeUuidInfoNodeAccess_p.h"
#include "kis_node_uuid_info.h"

#include <QTest>
#include <QVector>

namespace
{

struct NodeToken {
    int references = 0;
    QUuid uuid;
    QString name;
    NodeToken *firstChild = nullptr;
    NodeToken *nextSibling = nullptr;
    int uuidReads = 0;
    int nameReads = 0;
};

QVector<KisNode *> uuidReadOrder;

QUuid fixedUuid(const char *value)
{
    return QUuid(QString::fromLatin1(value));
}

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

void resetReads(std::initializer_list<NodeToken *> tokens)
{
    uuidReadOrder.clear();
    for (NodeToken *token : tokens) {
        token->uuidReads = 0;
        token->nameReads = 0;
    }
}

} // namespace

void kisSharedPtrAddReference(KisNode *node)
{
    ++nodeToken(node)->references;
}

bool kisSharedPtrRelease(KisNode *node)
{
    --nodeToken(node)->references;
    return true;
}

QUuid KisNodeUuidInfoNodeAccess::uuid(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    ++token->uuidReads;
    uuidReadOrder.append(node);
    return token->uuid;
}

QString KisNodeUuidInfoNodeAccess::name(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    ++token->nameReads;
    return token->name;
}

KisNodeSP KisNodeUuidInfoNodeAccess::firstChild(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    return token->firstChild ? KisNodeSP(nodePointer(token->firstChild)) : KisNodeSP();
}

KisNodeSP KisNodeUuidInfoNodeAccess::nextSibling(KisNode *node)
{
    NodeToken *const token = nodeToken(node);
    return token->nextSibling ? KisNodeSP(nodePointer(token->nextSibling)) : KisNodeSP();
}

class KisNodeUuidInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorsPreserveIdentifiersAtConstruction();
    void uuidSearchUsesDepthFirstTraversalAndRetainsResult();
    void nameSearchFallsBackAndMissingReturnsNull();
};

void KisNodeUuidInfoContractTest::constructorsPreserveIdentifiersAtConstruction()
{
    KisNodeUuidInfo defaults;
    QVERIFY(defaults.uuid().isNull());
    QVERIFY(defaults.name().isEmpty());

    const QUuid expectedUuid(QStringLiteral("{9bb61be7-944e-4906-94e8-40558e445fe2}"));
    KisNodeUuidInfo byUuid(expectedUuid);
    QCOMPARE(byUuid.uuid(), expectedUuid);
    QVERIFY(byUuid.name().isEmpty());

    KisNodeUuidInfo byName(QStringLiteral("Layer by name"));
    QVERIFY(byName.uuid().isNull());
    QCOMPARE(byName.name(), QStringLiteral("Layer by name"));

    NodeToken token{0, expectedUuid, QStringLiteral("Captured layer")};
    {
        KisNodeSP node(nodePointer(&token));
        KisNodeUuidInfo byNode(node);

        QCOMPARE(token.references, 1);
        QCOMPARE(token.uuidReads, 1);
        QCOMPARE(token.nameReads, 1);
        token.uuid = QUuid(QStringLiteral("{24bdbb2a-b14c-4b20-a1eb-edc857f7701f}"));
        token.name = QStringLiteral("Changed later");
        QCOMPARE(byNode.uuid(), expectedUuid);
        QCOMPARE(byNode.name(), QStringLiteral("Captured layer"));
    }
    QCOMPARE(token.references, 0);
}

void KisNodeUuidInfoContractTest::uuidSearchUsesDepthFirstTraversalAndRetainsResult()
{
    const QUuid targetUuid(QStringLiteral("{ae54b667-a513-4e98-b45d-b6b397774204}"));
    NodeToken root{0, fixedUuid("{11111111-1111-4111-8111-111111111111}"), QStringLiteral("Root")};
    NodeToken branch{0, fixedUuid("{22222222-2222-4222-8222-222222222222}"), QStringLiteral("Branch")};
    NodeToken deepMatch{0, targetUuid, QStringLiteral("Deep match")};
    NodeToken laterSibling{0, fixedUuid("{44444444-4444-4444-8444-444444444444}"), QStringLiteral("Later sibling")};
    root.firstChild = &branch;
    branch.firstChild = &deepMatch;
    branch.nextSibling = &laterSibling;
    resetReads({&root, &branch, &deepMatch, &laterSibling});

    KisNodeUuidInfo info(targetUuid);
    KisNodeSP rootNode(nodePointer(&root));
    KisNodeSP found = info.findNode(rootNode);

    QCOMPARE(found.data(), nodePointer(&deepMatch));
    QCOMPARE(uuidReadOrder, QVector<KisNode *>({nodePointer(&root), nodePointer(&branch), nodePointer(&deepMatch)}));
    QCOMPARE(root.nameReads, 0);
    QCOMPARE(branch.nameReads, 0);
    QCOMPARE(deepMatch.nameReads, 0);
    QCOMPARE(laterSibling.uuidReads, 0);
    QCOMPARE(root.references, 1);
    QCOMPARE(deepMatch.references, 1);

    rootNode.clear();
    QCOMPARE(root.references, 0);
    QCOMPARE(deepMatch.references, 1);
    found.clear();
    QCOMPARE(deepMatch.references, 0);
    QCOMPARE(branch.references, 0);
    QCOMPARE(laterSibling.references, 0);
}

void KisNodeUuidInfoContractTest::nameSearchFallsBackAndMissingReturnsNull()
{
    NodeToken root{0, fixedUuid("{55555555-5555-4555-8555-555555555555}"), QStringLiteral("Root")};
    NodeToken child{0, fixedUuid("{66666666-6666-4666-8666-666666666666}"), QStringLiteral("Child")};
    NodeToken namedMatch{0, fixedUuid("{77777777-7777-4777-8777-777777777777}"), QStringLiteral("Requested")};
    root.firstChild = &child;
    child.firstChild = &namedMatch;
    resetReads({&root, &child, &namedMatch});

    KisNodeSP rootNode(nodePointer(&root));
    KisNodeUuidInfo byName(QStringLiteral("Requested"));
    KisNodeSP found = byName.findNode(rootNode);

    QCOMPARE(found.data(), nodePointer(&namedMatch));
    QCOMPARE(root.uuidReads, 1);
    QCOMPARE(root.nameReads, 1);
    QCOMPARE(child.uuidReads, 1);
    QCOMPARE(child.nameReads, 1);
    QCOMPARE(namedMatch.uuidReads, 1);
    QCOMPARE(namedMatch.nameReads, 1);

    found.clear();
    resetReads({&root, &child, &namedMatch});
    KisNodeUuidInfo missing(QStringLiteral("Missing"));
    QVERIFY(missing.findNode(rootNode).isNull());
    QCOMPARE(uuidReadOrder, QVector<KisNode *>({nodePointer(&root), nodePointer(&child), nodePointer(&namedMatch)}));

    rootNode.clear();
    QCOMPARE(root.references, 0);
    QCOMPARE(child.references, 0);
    QCOMPARE(namedMatch.references, 0);
}

QTEST_GUILESS_MAIN(KisNodeUuidInfoContractTest)

#include "KisNodeUuidInfoContractTest.moc"
