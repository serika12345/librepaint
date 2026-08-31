/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierMesh.h"

#include <QDomDocument>
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

bool closePoint(const QPointF &actual, const QPointF &expected, qreal epsilon = 1e-6)
{
    return qAbs(actual.x() - expected.x()) <= epsilon && qAbs(actual.y() - expected.y()) <= epsilon;
}

template<typename T>
QString debugText(const T &value)
{
    QString text;
    QDebug debug(&text);
    debug.nospace() << value;
    return text;
}

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
    void patchProjectionBoundsAndDiagnosticsRemainStable();
    void subdivisionRemovalAndReshapePreserveGridContinuity();
    void hitTestsSeparateNodesControlsAndSegments();
    void smartMoveModesPreserveHandleConstraints();
    void nodeDomRoundTripRejectsWrongOuterType();
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

void KisBezierMeshValuesContractTest::patchProjectionBoundsAndDiagnosticsRemainStable()
{
    Mesh mesh(QRectF(10.0, 20.0, 100.0, 60.0), QSize(3, 2));

    const KisBezierPatch byCoordinates = mesh.makePatch(1, 0);
    const KisBezierPatch byIndex = mesh.makePatch(Mesh::PatchIndex(1, 0));
    QCOMPARE(byCoordinates.originalRect, QRectF(60.0, 20.0, 50.0, 60.0));
    QVERIFY(byCoordinates.points == byIndex.points);
    QCOMPARE(byCoordinates.points[KisBezierPatch::TL], mesh.node(1, 0).node);
    QCOMPARE(byCoordinates.points[KisBezierPatch::TL_HC], mesh.node(1, 0).rightControl);
    QCOMPARE(byCoordinates.points[KisBezierPatch::BR], mesh.node(2, 1).node);
    QCOMPARE(byCoordinates.points[KisBezierPatch::BR_HC], mesh.node(2, 1).leftControl);

    KisBezierPatch assigned{};
    const QRectF assignedSource(-4.0, 7.0, 13.0, 19.0);
    const Node ignoredTopLeft(QPointF(-100.0, -200.0));
    const Node ignoredTopRight(QPointF(300.0, -400.0));
    const Node ignoredBottomLeft(QPointF(-500.0, 600.0));
    const Node ignoredBottomRight(QPointF(700.0, 800.0));
    KisBezierMeshDetails::assignPatchData(&assigned,
                                          assignedSource,
                                          ignoredTopLeft,
                                          ignoredTopRight,
                                          ignoredBottomLeft,
                                          ignoredBottomRight);
    QCOMPARE(assigned.originalRect, assignedSource);
    for (const QPointF &point : assigned.points) {
        QCOMPARE(point, QPointF());
    }

    QCOMPARE(mesh.dstBoundingRect(), QRectF(10.0, 20.0, 100.0, 60.0));
    mesh.node(0, 0).rightControl = QPointF(-5.0, -7.0);
    mesh.node(2, 1).leftControl = QPointF(130.0, 95.0);
    QCOMPARE(mesh.dstBoundingRect(), QRectF(-5.0, -7.0, 135.0, 102.0));

    Node diagnosticNode(QPointF(2.0, 3.0));
    diagnosticNode.leftControl = QPointF(1.0, 3.0);
    diagnosticNode.topControl = QPointF(2.0, 1.0);
    diagnosticNode.rightControl = QPointF(5.0, 3.0);
    diagnosticNode.bottomControl = QPointF(2.0, 7.0);
    const QString nodeText = debugText(diagnosticNode);
    QVERIFY(nodeText.contains(QStringLiteral("Node QPointF(2,3)")));
    QVERIFY(nodeText.contains(QStringLiteral("lC: QPointF(1,3)")));
    QVERIFY(nodeText.contains(QStringLiteral("bC: QPointF(2,7)")));

    const QString meshText = debugText(Mesh(QRectF(0.0, 0.0, 10.0, 20.0), QSize(2, 2)));
    QVERIFY(meshText.contains(QStringLiteral("Mesh QSize(2, 2)")));
    QVERIFY(meshText.contains(QStringLiteral("node(0, 0) Node QPointF(0,0)")));
    QVERIFY(meshText.contains(QStringLiteral("node(1, 1) Node QPointF(10,20)")));

    const ControlPointIndex diagnosticIndex(Mesh::NodeIndex(3, 5), ControlPointIndex::RightControl);
    const QString indexText = debugText(diagnosticIndex);
    // known_defect: the diagnostic currently repeats x instead of printing y.
    QVERIFY(indexText.contains(QStringLiteral("ControlPointIndex (3, 3, RightControl)")));
}

void KisBezierMeshValuesContractTest::subdivisionRemovalAndReshapePreserveGridContinuity()
{
    Node destination(QPointF(5.0, 7.0));
    destination.leftControl = QPointF(1.0, 2.0);
    destination.rightControl = QPointF(9.0, 11.0);
    const Node unchangedDestination = destination;
    KisBezierMeshDetails::lerpNodeData(Node(QPointF(-10.0, 20.0)),
                                       Node(QPointF(30.0, -40.0)),
                                       0.25,
                                       destination);
    QCOMPARE(destination, unchangedDestination);

    Mesh columns(QRectF(0.0, 0.0, 100.0, 80.0), QSize(2, 2));
    QCOMPARE(columns.subdivideColumn(0.25), 1);
    QCOMPARE(columns.size(), QSize(3, 2));
    QVERIFY(closePoint(columns.node(1, 0).node, QPointF(25.0, 0.0), 1e-4));
    QCOMPARE(columns.subdivideColumn(0.25), 1);
    QCOMPARE(columns.size(), QSize(3, 2));
    QCOMPARE(columns.subdivideColumn(1, 0.5), 2);
    QCOMPARE(columns.size(), QSize(4, 2));
    QVERIFY(closePoint(columns.node(2, 1).node, QPointF(62.5, 80.0), 1e-3));

    Mesh rows(QRectF(0.0, 0.0, 100.0, 80.0), QSize(2, 2));
    QCOMPARE(rows.subdivideRow(0.5), 1);
    QCOMPARE(rows.size(), QSize(2, 3));
    QVERIFY(closePoint(rows.node(0, 1).node, QPointF(0.0, 40.0), 1e-4));
    QCOMPARE(rows.subdivideRow(1, 0.5), 2);
    QCOMPARE(rows.size(), QSize(2, 4));
    QVERIFY(closePoint(rows.node(1, 2).node, QPointF(100.0, 60.0), 1e-2));

    Mesh horizontalSegment(QRectF(0.0, 0.0, 100.0, 80.0), QSize(2, 2));
    horizontalSegment.subdivideSegment(Mesh::SegmentIndex(Mesh::NodeIndex(0, 0), 1), 0.5);
    QCOMPARE(horizontalSegment.size(), QSize(3, 2));
    Mesh verticalSegment(QRectF(0.0, 0.0, 100.0, 80.0), QSize(2, 2));
    verticalSegment.subdivideSegment(Mesh::SegmentIndex(Mesh::NodeIndex(0, 0), 0), 0.5);
    QCOMPARE(verticalSegment.size(), QSize(2, 3));

    Mesh removed(QRectF(0.0, 0.0, 100.0, 80.0), QSize(3, 3));
    removed.removeColumn(1);
    QCOMPARE(removed.size(), QSize(2, 3));
    removed.removeRow(1);
    QCOMPARE(removed.size(), QSize(2, 2));

    Mesh dispatchedRemoval(QRectF(0.0, 0.0, 100.0, 80.0), QSize(3, 3));
    dispatchedRemoval.removeColumnOrRow(Mesh::NodeIndex(1, 0), false);
    QCOMPARE(dispatchedRemoval.size(), QSize(2, 3));
    dispatchedRemoval.removeColumnOrRow(Mesh::NodeIndex(0, 1), true);
    QCOMPARE(dispatchedRemoval.size(), QSize(2, 2));

    Mesh reshaped(QRectF(0.0, 0.0, 100.0, 80.0), QSize(2, 2));
    const QPointF topLeft = reshaped.node(0, 0).node;
    const QPointF bottomRight = reshaped.node(1, 1).node;
    reshaped.reshapeMeshHorizontally(4);
    reshaped.reshapeMeshVertically(3);
    QCOMPARE(reshaped.size(), QSize(4, 3));
    QCOMPARE(reshaped.node(0, 0).node, topLeft);
    QCOMPARE(reshaped.node(3, 2).node, bottomRight);
    reshaped.reshapeMeshHorizontally(3);
    reshaped.reshapeMeshVertically(2);
    QCOMPARE(reshaped.size(), QSize(3, 2));
    QCOMPARE(reshaped.originalRect(), QRectF(0.0, 0.0, 100.0, 80.0));
}

void KisBezierMeshValuesContractTest::hitTestsSeparateNodesControlsAndSegments()
{
    const Mesh mesh(QRectF(0.0, 0.0, 100.0, 100.0), QSize(2, 2));

    QCOMPARE(mesh.hitTestNode(QPointF(0.5, 0.5), 2.0),
             ControlPointIndex(Mesh::NodeIndex(0, 0), ControlPointIndex::Node));
    QCOMPARE(mesh.hitTestControlPoint(QPointF(10.5, 0.0), 2.0),
             ControlPointIndex(Mesh::NodeIndex(0, 0), ControlPointIndex::RightControl));
    QCOMPARE(mesh.hitTestNode(QPointF(10.0, 0.0), 1.0),
             ControlPointIndex(Mesh::NodeIndex(0, 2), ControlPointIndex::LeftControl));
    QCOMPARE(mesh.hitTestControlPoint(QPointF(12.0, 0.0), 2.0),
             ControlPointIndex(Mesh::NodeIndex(0, 2), ControlPointIndex::LeftControl));

    qreal segmentPosition = -1.0;
    const Mesh::SegmentIndex topSegment = mesh.hitTestSegment(QPointF(50.0, 3.0), 5.0, &segmentPosition);
    QVERIFY(topSegment == Mesh::SegmentIndex(Mesh::NodeIndex(0, 0), 1));
    QVERIFY(qAbs(segmentPosition - 0.5) < 1e-3);

    const Mesh::SegmentIndex missingSegment = mesh.hitTestSegment(QPointF(50.0, 50.0), 1.0);
    QVERIFY(missingSegment == Mesh::SegmentIndex(Mesh::NodeIndex(0, 2), 0));
}

void KisBezierMeshValuesContractTest::smartMoveModesPreserveHandleConstraints()
{
    static_assert(std::is_same_v<KisSmartMoveMeshControlMode,
                                 KisBezierMeshDetails::SmartMoveMeshControlMode>);
    QCOMPARE(int(KisBezierMeshDetails::MoveFree), 0);
    QCOMPARE(int(KisBezierMeshDetails::MoveSymmetricLock), 1);
    QCOMPARE(int(KisBezierMeshDetails::MoveRotationLock), 2);

    Mesh nodeMove(QRectF(0.0, 0.0, 100.0, 100.0), QSize(3, 3));
    const Mesh::NodeIndex centerIndex(1, 1);
    const Node centerBefore = nodeMove.node(centerIndex);
    const QPointF nodeOffset(5.0, -7.0);
    KisBezierMeshDetails::smartMoveControl(
        nodeMove,
        ControlPointIndex(centerIndex, ControlPointIndex::Node),
        nodeOffset,
        KisBezierMeshDetails::MoveFree,
        false);
    QCOMPARE(nodeMove.node(centerIndex).node, centerBefore.node + nodeOffset);
    QCOMPARE(nodeMove.node(centerIndex).leftControl, centerBefore.leftControl + nodeOffset);
    QCOMPARE(nodeMove.node(centerIndex).bottomControl, centerBefore.bottomControl + nodeOffset);

    Mesh freeMove(QRectF(0.0, 0.0, 100.0, 100.0), QSize(3, 3));
    const Node freeBefore = freeMove.node(centerIndex);
    KisBezierMeshDetails::smartMoveControl(
        freeMove,
        ControlPointIndex(centerIndex, ControlPointIndex::RightControl),
        QPointF(0.0, 10.0),
        KisBezierMeshDetails::MoveFree,
        false);
    QCOMPARE(freeMove.node(centerIndex).rightControl, freeBefore.rightControl + QPointF(0.0, 10.0));
    QCOMPARE(freeMove.node(centerIndex).leftControl, freeBefore.leftControl);
    QCOMPARE(freeMove.node(centerIndex).topControl, freeBefore.topControl);
    QCOMPARE(freeMove.node(centerIndex).bottomControl, freeBefore.bottomControl);

    Mesh symmetricMove(QRectF(0.0, 0.0, 100.0, 100.0), QSize(3, 3));
    const Node symmetricBefore = symmetricMove.node(centerIndex);
    const QPointF handleMove(0.0, 10.0);
    const QPointF newRight = symmetricBefore.rightControl + handleMove;
    KisBezierMeshDetails::smartMoveControl(
        symmetricMove,
        ControlPointIndex(centerIndex, ControlPointIndex::RightControl),
        handleMove,
        KisBezierMeshDetails::MoveSymmetricLock,
        false);
    QCOMPARE(symmetricMove.node(centerIndex).rightControl, newRight);
    QVERIFY(closePoint(symmetricMove.node(centerIndex).leftControl,
                       QPointF(46.3019986918, 44.4529980377)));
    QCOMPARE(symmetricMove.node(centerIndex).topControl, symmetricBefore.topControl);

    Mesh rotationMove(QRectF(0.0, 0.0, 100.0, 100.0), QSize(3, 3));
    const Node rotationBefore = rotationMove.node(centerIndex);
    KisBezierMeshDetails::smartMoveControl(
        rotationMove,
        ControlPointIndex(centerIndex, ControlPointIndex::RightControl),
        handleMove,
        KisBezierMeshDetails::MoveRotationLock,
        false);
    QCOMPARE(rotationMove.node(centerIndex).rightControl, rotationBefore.rightControl + handleMove);
    QVERIFY(closePoint(rotationMove.node(centerIndex).leftControl,
                       QPointF(46.3019986918, 44.4529980377)));
    QVERIFY(closePoint(rotationMove.node(centerIndex).topControl,
                       QPointF(55.5470019623, 46.3019986918)));
    QVERIFY(closePoint(rotationMove.node(centerIndex).bottomControl,
                       QPointF(44.4529980377, 53.6980013082)));

    Mesh unscaled(QRectF(0.0, 0.0, 100.0, 100.0), QSize(3, 3));
    Mesh scaled = unscaled;
    KisBezierMeshDetails::smartMoveControl(
        unscaled,
        ControlPointIndex(centerIndex, ControlPointIndex::Node),
        QPointF(5.0, 0.0),
        KisBezierMeshDetails::MoveFree,
        false);
    KisBezierMeshDetails::smartMoveControl(
        scaled,
        ControlPointIndex(centerIndex, ControlPointIndex::Node),
        QPointF(5.0, 0.0),
        KisBezierMeshDetails::MoveFree,
        true);
    QCOMPARE(scaled.node(centerIndex).node, unscaled.node(centerIndex).node);
    QVERIFY(scaled.node(0, 1).rightControl != unscaled.node(0, 1).rightControl);
    QVERIFY(closePoint(scaled.node(0, 1).rightControl, QPointF(7.3333333333, 50.0), 1e-5));
}

void KisBezierMeshValuesContractTest::nodeDomRoundTripRejectsWrongOuterType()
{
    Node source(QPointF(3.25, -4.5));
    source.leftControl = QPointF(-1.5, 2.75);
    source.topControl = QPointF(6.0, -8.25);
    source.rightControl = QPointF(11.5, 12.75);
    source.bottomControl = QPointF(-13.0, 14.25);

    QDomDocument document(QStringLiteral("mesh-node-contract"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);
    KisBezierMeshDetails::saveValue(&root, QStringLiteral("mesh"), source);

    const QDomElement saved = root.firstChildElement(QStringLiteral("mesh"));
    QCOMPARE(saved.attribute(QStringLiteral("type")), QStringLiteral("mesh-node"));
    QVERIFY(!saved.firstChildElement(QStringLiteral("node")).isNull());
    QVERIFY(!saved.firstChildElement(QStringLiteral("left-control")).isNull());
    QVERIFY(!saved.firstChildElement(QStringLiteral("right-control")).isNull());
    QVERIFY(!saved.firstChildElement(QStringLiteral("top-control")).isNull());
    QVERIFY(!saved.firstChildElement(QStringLiteral("bottom-control")).isNull());

    Node restored;
    QVERIFY(KisBezierMeshDetails::loadValue(saved, &restored));
    QCOMPARE(restored, source);

    QDomElement wrongType = saved.cloneNode(true).toElement();
    wrongType.setAttribute(QStringLiteral("type"), QStringLiteral("pointf"));
    Node unchanged(QPointF(99.0, -101.0));
    const Node expectedUnchanged = unchanged;
    QVERIFY(!KisBezierMeshDetails::loadValue(wrongType, &unchanged));
    QCOMPARE(unchanged, expectedUnchanged);
}

QTEST_GUILESS_MAIN(KisBezierMeshValuesContractTest)

#include "KisBezierMeshValuesContractTest.moc"
