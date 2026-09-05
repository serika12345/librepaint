/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierGradientMesh.h"

#include <QTest>

#include <array>
#include <type_traits>

namespace
{

namespace GradientMeshDetail = KisBezierGradientMeshDetail;
using GradientMesh = GradientMeshDetail::KisBezierGradientMesh;
using GradientMeshNode = GradientMeshDetail::GradientMeshNode;
using GradientMeshPatch = GradientMeshDetail::GradientMeshPatch;

bool closeComponent(qreal actual, qreal expected)
{
    constexpr qreal epsilon = 2.0 / 65535.0;
    return qAbs(actual - expected) <= epsilon;
}

void compareColor(const QColor &actual, qreal red, qreal green, qreal blue, qreal alpha)
{
    QVERIFY(closeComponent(actual.redF(), red));
    QVERIFY(closeComponent(actual.greenF(), green));
    QVERIFY(closeComponent(actual.blueF(), blue));
    QVERIFY(closeComponent(actual.alphaF(), alpha));
}

} // namespace

class KisBezierGradientMeshContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientMeshTypesAndAliasSchemaRemainStable();
    void gradientMeshPublicColorDataSchemaRemainStable();
    void gradientMeshInlineColorOperationsRemainStable();
    void gradientMeshHitTestAndRenderingSignaturesRemainStable();
    void gradientMeshSerializationSignaturesRemainStable();
};

void KisBezierGradientMeshContractTest::gradientMeshTypesAndAliasSchemaRemainStable()
{
    static_assert(std::is_same_v<KisBezierGradientMesh, GradientMesh>);
    static_assert(std::is_class_v<GradientMesh>);
    static_assert(std::is_class_v<GradientMeshNode>);
    static_assert(std::is_class_v<GradientMeshPatch>);
}

void KisBezierGradientMeshContractTest::gradientMeshPublicColorDataSchemaRemainStable()
{
    using NodeColorMember = QColor GradientMeshNode::*;
    using PatchColorsMember = std::array<QColor, 4> GradientMeshPatch::*;

    static_assert(std::is_same_v<decltype(&GradientMeshNode::color), NodeColorMember>);
    static_assert(std::is_same_v<decltype(&GradientMeshPatch::colors), PatchColorsMember>);
}

void KisBezierGradientMeshContractTest::gradientMeshInlineColorOperationsRemainStable()
{
    using LerpSignature = QColor (*)(const QColor &, const QColor &, qreal);
    using LerpNodeDataSignature =
        void (*)(const GradientMeshNode &, const GradientMeshNode &, qreal, GradientMeshNode &);
    using AssignPatchDataSignature = void (*)(GradientMeshPatch *,
                                              const QRectF &,
                                              const GradientMeshNode &,
                                              const GradientMeshNode &,
                                              const GradientMeshNode &,
                                              const GradientMeshNode &);
    using EqualitySignature = bool (GradientMeshNode::*)(const GradientMeshNode &) const;

    static_assert(std::is_same_v<decltype(static_cast<LerpSignature>(&GradientMeshDetail::lerp)), LerpSignature>);
    static_assert(std::is_same_v<decltype(static_cast<LerpNodeDataSignature>(&GradientMeshDetail::lerpNodeData)),
                                 LerpNodeDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<AssignPatchDataSignature>(&GradientMeshDetail::assignPatchData)),
                                 AssignPatchDataSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<EqualitySignature>(&GradientMeshNode::operator==)), EqualitySignature>);

    const QColor leftColor = QColor::fromRgbF(0.2, 0.4, 0.6, 0.8);
    const QColor rightColor = QColor::fromRgbF(0.8, 0.2, 0.4, 0.6);
    const QColor interpolated = GradientMeshDetail::lerp(leftColor, rightColor, 0.25);
    compareColor(interpolated, 0.35, 0.35, 0.55, 0.75);

    GradientMeshNode left(QPointF(1.0, 2.0));
    left.color = leftColor;
    GradientMeshNode right(QPointF(7.0, 11.0));
    right.color = rightColor;
    GradientMeshNode destination(QPointF(100.0, 200.0));
    destination.leftControl = QPointF(90.0, 200.0);
    destination.topControl = QPointF(100.0, 180.0);
    destination.rightControl = QPointF(130.0, 200.0);
    destination.bottomControl = QPointF(100.0, 240.0);
    const KisBezierMeshDetails::BaseMeshNode originalGeometry = destination;

    GradientMeshDetail::lerpNodeData(left, right, 0.25, destination);
    compareColor(destination.color, 0.35, 0.35, 0.55, 0.75);
    QCOMPARE(destination.leftControl, originalGeometry.leftControl);
    QCOMPARE(destination.topControl, originalGeometry.topControl);
    QCOMPARE(destination.node, originalGeometry.node);
    QCOMPARE(destination.rightControl, originalGeometry.rightControl);
    QCOMPARE(destination.bottomControl, originalGeometry.bottomControl);

    GradientMeshNode topLeft;
    topLeft.color = QColor(Qt::red);
    GradientMeshNode topRight;
    topRight.color = QColor(Qt::green);
    GradientMeshNode bottomLeft;
    bottomLeft.color = QColor(Qt::blue);
    GradientMeshNode bottomRight;
    bottomRight.color = QColor(Qt::yellow);
    GradientMeshPatch patch;
    GradientMeshDetail::assignPatchData(&patch,
                                        QRectF(10.0, 20.0, 30.0, 40.0),
                                        topLeft,
                                        topRight,
                                        bottomLeft,
                                        bottomRight);
    QCOMPARE(patch.originalRect, QRectF(0.0, 0.0, 1.0, 1.0));
    QCOMPARE(patch.colors[0], topLeft.color);
    QCOMPARE(patch.colors[1], topRight.color);
    QCOMPARE(patch.colors[2], bottomLeft.color);
    QCOMPARE(patch.colors[3], bottomRight.color);

    GradientMeshNode equalNode = destination;
    QVERIFY(equalNode == destination);
    equalNode.rightControl += QPointF(1.0, 0.0);
    QVERIFY(equalNode != destination);
    equalNode = destination;
    equalNode.color = QColor(Qt::magenta);
    QVERIFY(equalNode != destination);
}

void KisBezierGradientMeshContractTest::gradientMeshHitTestAndRenderingSignaturesRemainStable()
{
    using HitTestSignature = GradientMesh::PatchIndex (GradientMesh::*)(const QPointF &, QPointF *) const;
    using RenderPatchSignature = void (*)(const GradientMeshPatch &, const QPoint &, QImage *);
    using RenderMeshSignature = void (GradientMesh::*)(const QPoint &, QImage *) const;

    static_assert(
        std::is_same_v<decltype(static_cast<HitTestSignature>(&GradientMesh::hitTestPatch)), HitTestSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<RenderPatchSignature>(&GradientMesh::renderPatch)), RenderPatchSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<RenderMeshSignature>(&GradientMesh::renderMesh)), RenderMeshSignature>);
}

void KisBezierGradientMeshContractTest::gradientMeshSerializationSignaturesRemainStable()
{
    using SaveNodeSignature = void (*)(QDomElement *, const QString &, const GradientMeshNode &);
    using LoadNodeSignature = bool (*)(const QDomElement &, GradientMeshNode *);
    using SaveMeshSignature = void (*)(QDomElement *, const QString &, const GradientMesh &);
    using LoadMeshSignature = bool (*)(const QDomElement &, const QString &, GradientMesh *);

    static_assert(
        std::is_same_v<decltype(static_cast<SaveNodeSignature>(&GradientMeshDetail::saveValue)), SaveNodeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<LoadNodeSignature>(&GradientMeshDetail::loadValue)), LoadNodeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SaveMeshSignature>(&GradientMeshDetail::saveValue)), SaveMeshSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<LoadMeshSignature>(&GradientMeshDetail::loadValue)), LoadMeshSignature>);
}

QTEST_APPLESS_MAIN(KisBezierGradientMeshContractTest)

#include "KisBezierGradientMeshContractTest.moc"
