/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierMesh.h"

#include <QTest>

#include <iterator>
#include <type_traits>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion at %s:%d: %s", file, line, assertion);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion at %s:%d: %s", file, line, assertion);
}

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
    void meshConstructionCreatesAddressableGrid();
    void indexValidityAndFindOverloadsAgree();
    void iteratorFamiliesExposeExactTraversalRanges();
    void meshEqualityIdentityAndDestinationTransformsPreserveSourceRect();
    void sourceAndDestinationTransformUpdatesBothSpaces();
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

void KisBezierMeshValuesContractTest::meshConstructionCreatesAddressableGrid()
{
    static_assert(std::is_same_v<KisBezierMesh, Mesh>);
    static_assert(std::is_same_v<KisBezierMeshBase<Node, KisBezierPatch>, Mesh>);
    static_assert(std::is_same_v<Mesh::Node, Node>);
    static_assert(std::is_same_v<Mesh::Patch, KisBezierPatch>);

    const Mesh defaultMesh;
    QCOMPARE(defaultMesh.size(), QSize(2, 2));
    QCOMPARE(defaultMesh.originalRect(), QRectF(0.0, 0.0, 1.0, 1.0));

    Mesh mesh(QRectF(10.0, 20.0, 30.0, 40.0), QSize(3, 2));
    QCOMPARE(mesh.size(), QSize(3, 2));
    QCOMPARE(mesh.originalRect(), QRectF(10.0, 20.0, 30.0, 40.0));
    QCOMPARE(mesh.node(0, 0).node, QPointF(10.0, 20.0));
    QCOMPARE(mesh.node(2, 1).node, QPointF(40.0, 60.0));
    QCOMPARE(&mesh.node(1, 1), &mesh.node(Mesh::NodeIndex(1, 1)));

    mesh.node(1, 1).node = QPointF(31.0, 57.0);
    const Mesh &constMesh = mesh;
    QCOMPARE(&constMesh.node(1, 1), &constMesh.node(Mesh::NodeIndex(1, 1)));
    QCOMPARE(constMesh.node(1, 1).node, QPointF(31.0, 57.0));
}

void KisBezierMeshValuesContractTest::indexValidityAndFindOverloadsAgree()
{
    static_assert(std::is_same_v<Mesh::SegmentIndex, std::pair<Mesh::NodeIndex, int>>);
    static_assert(std::is_same_v<Mesh::ControlType, ControlPointIndex::ControlType>);

    Mesh mesh(QRectF(10.0, 20.0, 30.0, 40.0), QSize(3, 2));
    const ControlPointIndex control(Mesh::NodeIndex(1, 1), ControlPointIndex::RightControl);
    const Mesh::NodeIndex node(1, 1);
    const Mesh::SegmentIndex segment(Mesh::NodeIndex(0, 0), 1);
    const Mesh::PatchIndex patch(0, 0);
    QVERIFY(mesh.isIndexValid(control));
    QVERIFY(mesh.isIndexValid(node));
    QVERIFY(mesh.isIndexValid(segment));
    QVERIFY(mesh.isIndexValid(patch));

    auto controlIt = mesh.find(control);
    QCOMPARE(controlIt.controlIndex(), control);
    *controlIt = QPointF(33.0, 44.0);
    QCOMPARE(mesh.node(1, 1).rightControl, QPointF(33.0, 44.0));
    QCOMPARE(mesh.find(node).controlIndex(), ControlPointIndex(node, ControlPointIndex::Node));
    QCOMPARE(mesh.find(segment).segmentIndex(), segment);
    QCOMPARE(mesh.find(patch), mesh.beginPatches());

    const Mesh &constMesh = mesh;
    QCOMPARE(constMesh.find(control).controlIndex(), control);
    QCOMPARE(constMesh.constFind(control).controlIndex(), control);
    QCOMPARE(constMesh.find(node).controlIndex(), ControlPointIndex(node, ControlPointIndex::Node));
    QCOMPARE(constMesh.constFind(node).controlIndex(), ControlPointIndex(node, ControlPointIndex::Node));
    QCOMPARE(constMesh.find(segment).segmentIndex(), segment);
    QCOMPARE(constMesh.constFind(segment).segmentIndex(), segment);
    QCOMPARE(constMesh.find(patch), constMesh.beginPatches());
    QCOMPARE(constMesh.constFind(patch), constMesh.constBeginPatches());

    const ControlPointIndex invalidControl(Mesh::NodeIndex(0, 0), ControlPointIndex::LeftControl);
    const Mesh::NodeIndex invalidNode(3, 0);
    const Mesh::SegmentIndex invalidSegment(Mesh::NodeIndex(2, 1), 1);
    const Mesh::PatchIndex invalidPatch(2, 1);
    QVERIFY(!mesh.isIndexValid(invalidControl));
    QVERIFY(!mesh.isIndexValid(invalidNode));
    QVERIFY(!mesh.isIndexValid(invalidSegment));
    QVERIFY(!mesh.isIndexValid(invalidPatch));
    QCOMPARE(mesh.find(invalidControl), mesh.endControlPoints());
    QCOMPARE(mesh.find(invalidNode), mesh.endControlPoints());
    QCOMPARE(mesh.find(invalidSegment), mesh.endSegments());
    QCOMPARE(mesh.find(invalidPatch), mesh.endPatches());
}

void KisBezierMeshValuesContractTest::iteratorFamiliesExposeExactTraversalRanges()
{
    static_assert(std::is_same_v<Mesh::patch_iterator, decltype(std::declval<Mesh &>().beginPatches())>);
    static_assert(std::is_same_v<Mesh::patch_const_iterator, decltype(std::declval<const Mesh &>().beginPatches())>);
    static_assert(std::is_same_v<Mesh::control_point_iterator, decltype(std::declval<Mesh &>().beginControlPoints())>);
    static_assert(std::is_same_v<Mesh::control_point_const_iterator,
                                 decltype(std::declval<const Mesh &>().beginControlPoints())>);
    static_assert(std::is_same_v<Mesh::segment_iterator, decltype(std::declval<Mesh &>().beginSegments())>);
    static_assert(std::is_same_v<Mesh::segment_const_iterator, decltype(std::declval<const Mesh &>().beginSegments())>);

    Mesh mesh(QRectF(10.0, 20.0, 30.0, 40.0), QSize(3, 2));
    QCOMPARE(std::distance(mesh.beginPatches(), mesh.endPatches()), 2);
    QCOMPARE(std::distance(mesh.beginControlPoints(), mesh.endControlPoints()), 20);
    QCOMPARE(std::distance(mesh.beginSegments(), mesh.endSegments()), 7);
    QCOMPARE(mesh.beginControlPoints().controlIndex(),
             ControlPointIndex(Mesh::NodeIndex(0, 0), ControlPointIndex::RightControl));
    QCOMPARE(mesh.beginSegments().segmentIndex(), Mesh::SegmentIndex(Mesh::NodeIndex(0, 0), 0));

    const Mesh &constMesh = mesh;
    QCOMPARE(std::distance(constMesh.beginPatches(), constMesh.endPatches()), 2);
    QCOMPARE(std::distance(constMesh.constBeginPatches(), constMesh.constEndPatches()), 2);
    QCOMPARE(constMesh.beginPatches(), constMesh.constBeginPatches());
    QCOMPARE(constMesh.endPatches(), constMesh.constEndPatches());
    QCOMPARE(std::distance(constMesh.beginControlPoints(), constMesh.endControlPoints()), 20);
    QCOMPARE(std::distance(constMesh.constBeginControlPoints(), constMesh.constEndControlPoints()), 20);
    QCOMPARE(constMesh.beginControlPoints(), constMesh.constBeginControlPoints());
    QCOMPARE(constMesh.endControlPoints(), constMesh.constEndControlPoints());
    QCOMPARE(std::distance(constMesh.beginSegments(), constMesh.endSegments()), 7);
    QCOMPARE(std::distance(constMesh.constBeginSegments(), constMesh.constEndSegments()), 7);
    QCOMPARE(constMesh.beginSegments(), constMesh.constBeginSegments());
    QCOMPARE(constMesh.endSegments(), constMesh.constEndSegments());
}

void KisBezierMeshValuesContractTest::meshEqualityIdentityAndDestinationTransformsPreserveSourceRect()
{
    const QRectF sourceRect(10.0, 20.0, 30.0, 40.0);
    const Mesh original(sourceRect, QSize(3, 2));
    Mesh changed = original;
    QVERIFY(changed == original);
    QVERIFY(changed.isIdentity());
    changed.node(1, 1).node += QPointF(1.0, 0.0);
    QVERIFY(changed != original);
    QVERIFY(!changed.isIdentity());

    Mesh translated = original;
    const QPointF oldNode = translated.node(2, 1).node;
    const QPointF oldLeft = translated.node(2, 1).leftControl;
    translated.translate(QPointF(5.0, -7.0));
    QCOMPARE(translated.node(2, 1).node, oldNode + QPointF(5.0, -7.0));
    QCOMPARE(translated.node(2, 1).leftControl, oldLeft + QPointF(5.0, -7.0));
    QCOMPARE(translated.originalRect(), sourceRect);

    Mesh transformed = original;
    QTransform transform;
    transform.translate(4.0, -3.0);
    transform.scale(2.0, 0.5);
    transformed.transform(transform);
    QCOMPARE(transformed.node(2, 1).node, transform.map(original.node(2, 1).node));
    QCOMPARE(transformed.node(2, 1).topControl, transform.map(original.node(2, 1).topControl));
    QCOMPARE(transformed.originalRect(), sourceRect);
}

void KisBezierMeshValuesContractTest::sourceAndDestinationTransformUpdatesBothSpaces()
{
    const QRectF sourceRect(10.0, 20.0, 30.0, 40.0);
    const Mesh original(sourceRect, QSize(3, 2));
    Mesh transformed = original;
    QTransform transform;
    transform.translate(4.0, -3.0);
    transform.scale(2.0, 0.5);
    const QTransform inputValue = transform;

    transformed.transformSrcAndDst(transform);
    QCOMPARE(transformed.node(0, 0).node, transform.map(original.node(0, 0).node));
    QCOMPARE(transformed.node(2, 1).rightControl, transform.map(original.node(2, 1).rightControl));
    QCOMPARE(transformed.originalRect(), transform.mapRect(sourceRect));
    QCOMPARE(transform, inputValue);
}

QTEST_GUILESS_MAIN(KisBezierMeshValuesContractTest)

#include "KisBezierMeshValuesContractTest.moc"
