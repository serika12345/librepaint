/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <flake/kis_node_shapes_graph.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeShapesGraph::method)), signature>)
} // namespace

class KisNodeShapesGraphSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void graphTypeAdditionAndShapeLookupSchemaRemainStable();
    void graphRemovalAndCountSchemaRemainStable();
    void graphRootDummyMembershipAndMoveSchemaRemainStable();
};

void KisNodeShapesGraphSchemaContractTest::graphTypeAdditionAndShapeLookupSchemaRemainStable()
{
    static_assert(std::is_class_v<KisNodeShapesGraph>);
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(addNode,
                                       KisNodeShape * (KisNodeShapesGraph::*)(KisNodeSP, KisNodeSP, KisNodeSP));
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(nodeToShape, KisNodeShape * (KisNodeShapesGraph::*)(KisNodeSP));

    QVERIFY(true);
}

void KisNodeShapesGraphSchemaContractTest::graphRemovalAndCountSchemaRemainStable()
{
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(removeNode, void (KisNodeShapesGraph::*)(KisNodeSP));
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(shapesCount, int (KisNodeShapesGraph::*)() const);

    QVERIFY(true);
}

void KisNodeShapesGraphSchemaContractTest::graphRootDummyMembershipAndMoveSchemaRemainStable()
{
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(rootDummy, KisNodeDummy * (KisNodeShapesGraph::*)() const);
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(nodeToDummy, KisNodeDummy * (KisNodeShapesGraph::*)(KisNodeSP));
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(containsNode, bool (KisNodeShapesGraph::*)(KisNodeSP) const);
    ASSERT_NODE_SHAPES_GRAPH_SIGNATURE(moveNode, void (KisNodeShapesGraph::*)(KisNodeSP, KisNodeSP, KisNodeSP));

    QVERIFY(true);
}

#undef ASSERT_NODE_SHAPES_GRAPH_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeShapesGraphSchemaContractTest)

#include "KisNodeShapesGraphSchemaContractTest.moc"
