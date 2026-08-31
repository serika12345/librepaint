/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierMesh.h"

#include <QTest>

#include <type_traits>

namespace
{

using Node = KisBezierMeshDetails::BaseMeshNode;
using Mesh = KisBezierMeshDetails::Mesh<>;
using ControlPointIndex = Mesh::ControlPointIndex;

} // namespace

class KisBezierMeshValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeConstructionInitializesEveryPoint();
    void relativeControlsConvertToAndFromAbsolutePoints();
    void equalityTranslationAndTransformPreserveIndependentValues();
    void controlPointIndicesClassifyAndSelectExactMembers();
    void nodeAndPatchIndicesApplyPointOffsets();
};

void KisBezierMeshValuesContractTest::nodeConstructionInitializesEveryPoint()
{
    const Node empty;
    QCOMPARE(empty.leftControl, QPointF());
    QCOMPARE(empty.topControl, QPointF());
    QCOMPARE(empty.node, QPointF());
    QCOMPARE(empty.rightControl, QPointF());
    QCOMPARE(empty.bottomControl, QPointF());

    const QPointF position(7.5, -3.25);
    const Node initialized(position);
    QCOMPARE(initialized.leftControl, position);
    QCOMPARE(initialized.topControl, position);
    QCOMPARE(initialized.node, position);
    QCOMPARE(initialized.rightControl, position);
    QCOMPARE(initialized.bottomControl, position);
}

void KisBezierMeshValuesContractTest::relativeControlsConvertToAndFromAbsolutePoints()
{
    Node node(QPointF(10.0, 20.0));
    node.setLeftControlRelative(QPointF(-3.0, 1.0));
    node.setTopControlRelative(QPointF(2.0, -4.0));
    node.setRightControlRelative(QPointF(5.0, 6.0));
    node.setBottomControlRelative(QPointF(-7.0, 8.0));

    QCOMPARE(node.leftControl, QPointF(7.0, 21.0));
    QCOMPARE(node.topControl, QPointF(12.0, 16.0));
    QCOMPARE(node.rightControl, QPointF(15.0, 26.0));
    QCOMPARE(node.bottomControl, QPointF(3.0, 28.0));
    QCOMPARE(node.leftControlRelative(), QPointF(-3.0, 1.0));
    QCOMPARE(node.topControlRelative(), QPointF(2.0, -4.0));
    QCOMPARE(node.rightControlRelative(), QPointF(5.0, 6.0));
    QCOMPARE(node.bottomControlRelative(), QPointF(-7.0, 8.0));
}

void KisBezierMeshValuesContractTest::equalityTranslationAndTransformPreserveIndependentValues()
{
    Node original(QPointF(2.0, 3.0));
    original.leftControl = QPointF(1.0, 3.0);
    original.topControl = QPointF(2.0, 1.0);
    original.rightControl = QPointF(5.0, 3.0);
    original.bottomControl = QPointF(2.0, 7.0);
    Node changed = original;
    QVERIFY(changed == original);
    changed.rightControl.rx() += 1.0;
    QVERIFY(changed != original);

    Node translated = original;
    translated.translate(QPointF(10.0, -5.0));
    QCOMPARE(translated.leftControl, QPointF(11.0, -2.0));
    QCOMPARE(translated.topControl, QPointF(12.0, -4.0));
    QCOMPARE(translated.node, QPointF(12.0, -2.0));
    QCOMPARE(translated.rightControl, QPointF(15.0, -2.0));
    QCOMPARE(translated.bottomControl, QPointF(12.0, 2.0));
    QCOMPARE(original.node, QPointF(2.0, 3.0));

    Node transformed = original;
    QTransform transform;
    transform.translate(4.0, -2.0);
    transform.scale(2.0, 3.0);
    transformed.transform(transform);
    QCOMPARE(transformed.leftControl, transform.map(original.leftControl));
    QCOMPARE(transformed.topControl, transform.map(original.topControl));
    QCOMPARE(transformed.node, transform.map(original.node));
    QCOMPARE(transformed.rightControl, transform.map(original.rightControl));
    QCOMPARE(transformed.bottomControl, transform.map(original.bottomControl));
}

void KisBezierMeshValuesContractTest::controlPointIndicesClassifyAndSelectExactMembers()
{
    static_assert(std::is_default_constructible_v<ControlPointIndex>);
    QCOMPARE(int(ControlPointIndex::LeftControl), 0);
    QCOMPARE(int(ControlPointIndex::TopControl), 1);
    QCOMPARE(int(ControlPointIndex::RightControl), 2);
    QCOMPARE(int(ControlPointIndex::BottomControl), 3);
    QCOMPARE(int(ControlPointIndex::Node), 4);

    const Mesh::NodeIndex nodeIndex(3, 5);
    const ControlPointIndex left(nodeIndex, ControlPointIndex::LeftControl);
    const ControlPointIndex sameLeft(nodeIndex, ControlPointIndex::LeftControl);
    const ControlPointIndex center(nodeIndex, ControlPointIndex::Node);
    QCOMPARE(left.nodeIndex, nodeIndex);
    QCOMPARE(left.controlType, ControlPointIndex::LeftControl);
    QVERIFY(left == sameLeft);
    QVERIFY(left != center);
    QVERIFY(left.isControlPoint());
    QVERIFY(!left.isNode());
    QVERIFY(center.isNode());
    QVERIFY(!center.isControlPoint());

    Node node(QPointF(10.0, 20.0));
    node.leftControl = QPointF(1.0, 2.0);
    node.topControl = QPointF(3.0, 4.0);
    node.rightControl = QPointF(5.0, 6.0);
    node.bottomControl = QPointF(7.0, 8.0);
    QCOMPARE(&ControlPointIndex::controlPoint(node, ControlPointIndex::LeftControl), &node.leftControl);
    QCOMPARE(&ControlPointIndex::controlPoint(node, ControlPointIndex::TopControl), &node.topControl);
    QCOMPARE(&ControlPointIndex::controlPoint(node, ControlPointIndex::RightControl), &node.rightControl);
    QCOMPARE(&ControlPointIndex::controlPoint(node, ControlPointIndex::BottomControl), &node.bottomControl);
    QCOMPARE(&ControlPointIndex::controlPoint(node, ControlPointIndex::Node), &node.node);

    ControlPointIndex right(nodeIndex, ControlPointIndex::RightControl);
    QCOMPARE(&right.controlPoint(node), &node.rightControl);
    right.controlPoint(node) = QPointF(-11.0, 13.0);
    QCOMPARE(node.rightControl, QPointF(-11.0, 13.0));
}

void KisBezierMeshValuesContractTest::nodeAndPatchIndicesApplyPointOffsets()
{
    Mesh::NodeIndex nodeIndex(4, 7);
    Mesh::NodeIndex &addedNode = (nodeIndex += QPoint(-2, 5));
    QCOMPARE(&addedNode, &nodeIndex);
    QCOMPARE(nodeIndex, QPoint(2, 12));
    Mesh::NodeIndex &subtractedNode = (nodeIndex -= QPoint(3, 4));
    QCOMPARE(&subtractedNode, &nodeIndex);
    QCOMPARE(nodeIndex, QPoint(-1, 8));

    Mesh::PatchIndex patchIndex(-3, 9);
    Mesh::PatchIndex &addedPatch = (patchIndex += QPoint(8, -2));
    QCOMPARE(&addedPatch, &patchIndex);
    QCOMPARE(patchIndex, QPoint(5, 7));
    Mesh::PatchIndex &subtractedPatch = (patchIndex -= QPoint(1, 10));
    QCOMPARE(&subtractedPatch, &patchIndex);
    QCOMPARE(patchIndex, QPoint(4, -3));
}

QTEST_GUILESS_MAIN(KisBezierMeshValuesContractTest)

#include "KisBezierMeshValuesContractTest.moc"
