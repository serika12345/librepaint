/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../tool_transform_args.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TRANSFORM_ARGS_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&ToolTransformArgs::method)), signature>)
} // namespace

class ToolTransformArgsGeometrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void freeTransformCenterSignaturesRemainStable();
    void freeTransformRotationSignaturesRemainStable();
    void freeTransformScaleSignaturesRemainStable();
    void freeTransformShearAndBoundsSignaturesRemainStable();
    void freeTransformPerspectiveSignaturesRemainStable();
};

void ToolTransformArgsGeometrySchemaContractTest::freeTransformCenterSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(transformedCenter, QPointF (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setTransformedCenter, void (ToolTransformArgs::*)(QPointF));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(originalCenter, QPointF (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setOriginalCenter, void (ToolTransformArgs::*)(QPointF));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(rotationCenterOffset, QPointF (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setRotationCenterOffset, void (ToolTransformArgs::*)(QPointF));
}

void ToolTransformArgsGeometrySchemaContractTest::freeTransformRotationSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(transformAroundRotationCenter, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setTransformAroundRotationCenter, void (ToolTransformArgs::*)(bool));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(aX, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setAX, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(aY, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setAY, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(aZ, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setAZ, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(cameraPos, QVector3D (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setCameraPos, void (ToolTransformArgs::*)(const QVector3D &));
}

void ToolTransformArgsGeometrySchemaContractTest::freeTransformScaleSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(scaleX, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setScaleX, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(scaleY, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setScaleY, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(keepAspectRatio, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setKeepAspectRatio, void (ToolTransformArgs::*)(bool));
}

void ToolTransformArgsGeometrySchemaContractTest::freeTransformShearAndBoundsSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(shearX, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setShearX, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(shearY, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setShearY, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(boundsRotation, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setBoundsRotation, void (ToolTransformArgs::*)(double));
}

void ToolTransformArgsGeometrySchemaContractTest::freeTransformPerspectiveSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(flattenedPerspectiveTransform, QTransform (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setFlattenedPerspectiveTransform, void (ToolTransformArgs::*)(const QTransform &));
}

QTEST_GUILESS_MAIN(ToolTransformArgsGeometrySchemaContractTest)

#include "ToolTransformArgsGeometrySchemaContractTest.moc"
