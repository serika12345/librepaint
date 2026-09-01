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
    void transformModeAndPrecisionSchemaRemainsStable();
    void warpPointAndCalculationSignaturesRemainStable();
    void meshTransformStateSignaturesRemainStable();
    void liquifyAndEditingStateSignaturesRemainStable();
    void transformStateContinuationComparisonAndMappingSignaturesRemainStable();
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

void ToolTransformArgsGeometrySchemaContractTest::transformModeAndPrecisionSchemaRemainsStable()
{
    static_assert(std::is_class_v<ToolTransformArgs>);
    static_assert(std::is_base_of_v<KisToolChangesTrackerData, ToolTransformArgs>);
    static_assert(std::is_enum_v<ToolTransformArgs::TransformMode>);
    static_assert(static_cast<int>(ToolTransformArgs::FREE_TRANSFORM) == 0);
    static_assert(static_cast<int>(ToolTransformArgs::WARP) == 1);
    static_assert(static_cast<int>(ToolTransformArgs::CAGE) == 2);
    static_assert(static_cast<int>(ToolTransformArgs::LIQUIFY) == 3);
    static_assert(static_cast<int>(ToolTransformArgs::PERSPECTIVE_4POINT) == 4);
    static_assert(static_cast<int>(ToolTransformArgs::MESH) == 5);
    static_assert(static_cast<int>(ToolTransformArgs::N_MODES) == 6);

    ASSERT_TRANSFORM_ARGS_SIGNATURE(mode, ToolTransformArgs::TransformMode (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setMode, void (ToolTransformArgs::*)(ToolTransformArgs::TransformMode));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(pixelPrecision, int (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setPixelPrecision, void (ToolTransformArgs::*)(int));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(previewPixelPrecision, int (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setPreviewPixelPrecision, void (ToolTransformArgs::*)(int));
}

void ToolTransformArgsGeometrySchemaContractTest::warpPointAndCalculationSignaturesRemainStable()
{
    using SetPointsSignature = void (ToolTransformArgs::*)(QVector<QPointF>, QVector<QPointF>);

    ASSERT_TRANSFORM_ARGS_SIGNATURE(alpha, double (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setAlpha, void (ToolTransformArgs::*)(double));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(defaultPoints, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setDefaultPoints, void (ToolTransformArgs::*)(bool));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(numPoints, int (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(origPoint, QPointF & (ToolTransformArgs::*)(int));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(origPoints, const QVector<QPointF> &(ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(refOriginalPoints, QVector<QPointF> & (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(refTransformedPoints, QVector<QPointF> & (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setPoints, SetPointsSignature);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(transfPoint, QPointF & (ToolTransformArgs::*)(int));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(transfPoints, const QVector<QPointF> &(ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(warpCalculation, KisWarpTransformWorker::WarpCalculation (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setWarpCalculation,
                                    void (ToolTransformArgs::*)(KisWarpTransformWorker::WarpCalculation));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(warpType, KisWarpTransformWorker::WarpType (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setWarpType, void (ToolTransformArgs::*)(KisWarpTransformWorker::WarpType));
}

void ToolTransformArgsGeometrySchemaContractTest::meshTransformStateSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(meshTransform, KisBezierTransformMesh * (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(meshTransform, const KisBezierTransformMesh *(ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(meshScaleHandles, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setMeshScaleHandles, void (ToolTransformArgs::*)(bool));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(meshShowHandles, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setMeshShowHandles, void (ToolTransformArgs::*)(bool));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(meshSymmetricalHandles, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setMeshSymmetricalHandles, void (ToolTransformArgs::*)(bool));
}

void ToolTransformArgsGeometrySchemaContractTest::liquifyAndEditingStateSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(initLiquifyTransformMode, void (ToolTransformArgs::*)(const QRect &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(isEditingTransformPoints, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(setEditingTransformPoints, void (ToolTransformArgs::*)(bool));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(liquifyProperties, KisLiquifyProperties * (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(liquifyProperties, const KisLiquifyProperties *(ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(liquifyWorker, KisLiquifyTransformWorker * (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(saveLiquifyTransformMode, void (ToolTransformArgs::*)() const);
}

void ToolTransformArgsGeometrySchemaContractTest::transformStateContinuationComparisonAndMappingSignaturesRemainStable()
{
    ASSERT_TRANSFORM_ARGS_SIGNATURE(continuedTransform, const ToolTransformArgs *(ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(restoreContinuedState, void (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(saveContinuedState, void (ToolTransformArgs::*)());
    ASSERT_TRANSFORM_ARGS_SIGNATURE(isIdentity, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(isSameMode, bool (ToolTransformArgs::*)(const ToolTransformArgs &) const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(isUnchanging, bool (ToolTransformArgs::*)() const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(operator==, bool (ToolTransformArgs::*)(const ToolTransformArgs &) const);
    ASSERT_TRANSFORM_ARGS_SIGNATURE(scale3dSrcAndDst, void (ToolTransformArgs::*)(qreal));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(transformSrcAndDst, void (ToolTransformArgs::*)(const QTransform &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(translateDstSpace, void (ToolTransformArgs::*)(const QPointF &));
    ASSERT_TRANSFORM_ARGS_SIGNATURE(translateSrcAndDst, void (ToolTransformArgs::*)(const QPointF &));
}

QTEST_GUILESS_MAIN(ToolTransformArgsGeometrySchemaContractTest)

#include "ToolTransformArgsGeometrySchemaContractTest.moc"
