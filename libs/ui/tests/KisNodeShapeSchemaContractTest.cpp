/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <flake/kis_node_shape.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_NODE_SHAPE_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeShape::method)), signature>)
} // namespace

class KisNodeShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeShapeTypeConstructionAndLifetimeSchemaRemainStable();
    void nodeShapeAndGeometrySchemaRemainStable();
    void nodeShapePositionAndPaintingSchemaRemainStable();
};

void KisNodeShapeSchemaContractTest::nodeShapeTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisNodeShape>);
    static_assert(std::is_base_of_v<QObject, KisNodeShape>);
    static_assert(std::is_base_of_v<KoShapeLayer, KisNodeShape>);
    static_assert(std::is_constructible_v<KisNodeShape, KisNodeSP>);
    static_assert(std::has_virtual_destructor_v<KisNodeShape>);

    QVERIFY(true);
}

void KisNodeShapeSchemaContractTest::nodeShapeAndGeometrySchemaRemainStable()
{
    ASSERT_NODE_SHAPE_SIGNATURE(node, KisNodeSP (KisNodeShape::*)());
    ASSERT_NODE_SHAPE_SIGNATURE(size, QSizeF (KisNodeShape::*)() const);
    ASSERT_NODE_SHAPE_SIGNATURE(boundingRect, QRectF (KisNodeShape::*)() const);

    QVERIFY(true);
}

void KisNodeShapeSchemaContractTest::nodeShapePositionAndPaintingSchemaRemainStable()
{
    ASSERT_NODE_SHAPE_SIGNATURE(setPosition, void (KisNodeShape::*)(const QPointF &));
    ASSERT_NODE_SHAPE_SIGNATURE(paint, void (KisNodeShape::*)(QPainter &) const);

    QVERIFY(true);
}

#undef ASSERT_NODE_SHAPE_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeShapeSchemaContractTest)

#include "KisNodeShapeSchemaContractTest.moc"
