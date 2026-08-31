/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBatchNodeUpdate.h"
#include "KisBatchNodeUpdateNodeAccess_p.h"

#include <QDebug>
#include <QTest>
#include <QVector>

#include <algorithm>
#include <initializer_list>

namespace
{

struct NodeToken {
    int references = 0;
};

struct ExpectedUpdate {
    KisNode *node;
    QRect rect;
};

KisNodeList sortedRootsResponse;
QVector<KisNode *> sortInput;
QVector<QPair<KisNode *, KisNode *>> childRelations;

KisNode *nodePointer(NodeToken *token)
{
    return reinterpret_cast<KisNode *>(token);
}

NodeToken *nodeToken(KisNode *node)
{
    return reinterpret_cast<NodeToken *>(node);
}

KisNodeSP sharedNode(NodeToken *token)
{
    return KisNodeSP(nodePointer(token));
}

void compareUpdate(const KisBatchNodeUpdate &update, std::initializer_list<ExpectedUpdate> expected)
{
    QCOMPARE(update.size(), expected.size());

    auto actualIt = update.begin();
    for (const ExpectedUpdate &expectedUpdate : expected) {
        QCOMPARE(actualIt->first.data(), expectedUpdate.node);
        QCOMPARE(actualIt->second, expectedUpdate.rect);
        ++actualIt;
    }
}

QRect rectForNode(const KisBatchNodeUpdate &update, KisNode *node)
{
    const auto it = std::find_if(update.begin(), update.end(), [node](const auto &entry) {
        return entry.first.data() == node;
    });
    return it == update.end() ? QRect() : it->second;
}

void resetNodeAccess()
{
    sortedRootsResponse.clear();
    sortInput.clear();
    childRelations.clear();
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

namespace KisBatchNodeUpdateNodeAccess
{

KisNodeList sortAndFilterMergeableNodes(KisNodeList nodes)
{
    sortInput.clear();
    for (KisNodeSP &node : nodes) {
        sortInput.append(node.data());
    }
    return sortedRootsResponse;
}

bool isChildOf(KisNodeSP node, KisNodeSP parent)
{
    return childRelations.contains(qMakePair(node.data(), parent.data()));
}

} // namespace KisBatchNodeUpdateNodeAccess

class KisBatchNodeUpdateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionCopyMoveAndAssignmentPreserveOrderedValues();
    void addAndUnionMergeRectanglesByNodeIdentity();
    void compressionUnitesDescendantUpdatesWithoutMutatingSource();
    void debugAndSharedAliasesPreserveStructureAndLifetime();
};

void KisBatchNodeUpdateContractTest::constructionCopyMoveAndAssignmentPreserveOrderedValues()
{
    NodeToken first;
    NodeToken second;
    const QRect firstRect(3, 5, 7, 11);
    const QRect secondRect(-13, 17, 19, 23);

    {
        KisBatchNodeUpdate empty;
        QVERIFY(empty.empty());

        std::vector<std::pair<KisNodeSP, QRect>> source{
            {sharedNode(&first), firstRect},
            {sharedNode(&second), secondRect},
        };
        KisBatchNodeUpdate original(source);
        compareUpdate(original, {{nodePointer(&first), firstRect}, {nodePointer(&second), secondRect}});

        source.front().second = QRect(101, 103, 107, 109);
        QCOMPARE(original.front().second, firstRect);

        KisBatchNodeUpdate copied(original);
        original.front().second = QRect(29, 31, 37, 41);
        compareUpdate(copied, {{nodePointer(&first), firstRect}, {nodePointer(&second), secondRect}});

        KisBatchNodeUpdate moved(std::move(original));
        compareUpdate(moved, {{nodePointer(&first), QRect(29, 31, 37, 41)}, {nodePointer(&second), secondRect}});

        KisBatchNodeUpdate assigned;
        assigned = copied;
        compareUpdate(assigned, {{nodePointer(&first), firstRect}, {nodePointer(&second), secondRect}});
    }

    QCOMPARE(first.references, 0);
    QCOMPARE(second.references, 0);
}

void KisBatchNodeUpdateContractTest::addAndUnionMergeRectanglesByNodeIdentity()
{
    NodeToken first;
    NodeToken second;
    NodeToken third;
    const QRect firstRect(1, 2, 5, 7);
    const QRect overlappingFirst(4, 6, 11, 13);
    const QRect secondRect(-10, -8, 3, 4);
    const QRect thirdRect(20, 30, 9, 6);

    {
        KisBatchNodeUpdate left;
        left.addUpdate(sharedNode(&first), firstRect);
        left.addUpdate(sharedNode(&second), secondRect);

        KisBatchNodeUpdate right;
        right.addUpdate(sharedNode(&first), overlappingFirst);
        right.addUpdate(sharedNode(&third), thirdRect);
        const KisBatchNodeUpdate rightBeforeMerge(right);

        left |= right;
        QCOMPARE(left.size(), 3);
        QCOMPARE(rectForNode(left, nodePointer(&first)), firstRect.united(overlappingFirst));
        QCOMPARE(rectForNode(left, nodePointer(&second)), secondRect);
        QCOMPARE(rectForNode(left, nodePointer(&third)), thirdRect);
        compareUpdate(right, {{nodePointer(&first), overlappingFirst}, {nodePointer(&third), thirdRect}});

        const KisBatchNodeUpdate beforeSelfMerge(left);
        left |= left;
        QVERIFY(left == beforeSelfMerge);
        QVERIFY(right == rightBeforeMerge);
    }

    QCOMPARE(first.references, 0);
    QCOMPARE(second.references, 0);
    QCOMPARE(third.references, 0);
}

void KisBatchNodeUpdateContractTest::compressionUnitesDescendantUpdatesWithoutMutatingSource()
{
    NodeToken firstRoot;
    NodeToken firstChild;
    NodeToken secondRoot;
    const QRect rootRect(2, 3, 5, 7);
    const QRect childRect(6, 8, 11, 13);
    const QRect otherRootRect(-17, 19, 23, 29);

    {
        sortedRootsResponse = {sharedNode(&firstRoot), sharedNode(&secondRoot)};
        childRelations = {{nodePointer(&firstChild), nodePointer(&firstRoot)}};

        KisBatchNodeUpdate source;
        source.addUpdate(sharedNode(&firstRoot), rootRect);
        source.addUpdate(sharedNode(&firstChild), childRect);
        source.addUpdate(sharedNode(&secondRoot), otherRootRect);

        const KisBatchNodeUpdate compressed = source.compressed();
        compareUpdate(
            compressed,
            {{nodePointer(&firstRoot), rootRect.united(childRect)}, {nodePointer(&secondRoot), otherRootRect}});
        compareUpdate(source,
                      {{nodePointer(&firstRoot), rootRect},
                       {nodePointer(&firstChild), childRect},
                       {nodePointer(&secondRoot), otherRootRect}});
        QCOMPARE(sortInput,
                 QVector<KisNode *>({nodePointer(&firstRoot), nodePointer(&firstChild), nodePointer(&secondRoot)}));

        KisBatchNodeUpdate inPlace(source);
        inPlace.compress();
        QVERIFY(inPlace == compressed);
    }

    resetNodeAccess();
    QCOMPARE(firstRoot.references, 0);
    QCOMPARE(firstChild.references, 0);
    QCOMPARE(secondRoot.references, 0);
}

void KisBatchNodeUpdateContractTest::debugAndSharedAliasesPreserveStructureAndLifetime()
{
    NodeToken first;
    NodeToken second;

    {
        KisBatchNodeUpdate update;
        update.addUpdate(sharedNode(&first), QRect(1, 2, 3, 4));
        update.addUpdate(sharedNode(&second), QRect(-5, 6, 7, 8));

        QString debugText;
        QDebug debug(&debugText);
        operator<<(debug, update);
        QVERIFY(debugText.startsWith(QStringLiteral("KisBatchNodeUpdate (")));
        QCOMPARE(debugText.count(QStringLiteral("->QRect(")), 2);
        QCOMPARE(debugText.count(QStringLiteral("; ")), 1);
    }

    QCOMPARE(first.references, 0);
    QCOMPARE(second.references, 0);

    KisBatchNodeUpdateSP strong = KisBatchNodeUpdateSP::create();
    strong->addUpdate(KisNodeSP(), QRect(9, 10, 11, 12));
    KisBatchNodeUpdateWSP weak(strong);
    QVERIFY(!weak.isNull());
    QCOMPARE(weak.toStrongRef()->front().second, QRect(9, 10, 11, 12));
    strong.clear();
    QVERIFY(weak.isNull());
}

QTEST_GUILESS_MAIN(KisBatchNodeUpdateContractTest)

#include "KisBatchNodeUpdateContractTest.moc"
