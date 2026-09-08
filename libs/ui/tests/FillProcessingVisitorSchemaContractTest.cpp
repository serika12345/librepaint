/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <processing/fill_processing_visitor.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_FILL_VISITOR_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&FillProcessingVisitor::method)), signature>)
} // namespace

class FillProcessingVisitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeEnumerationAndConstructionSchemaRemainStable();
    void seedSelectionAndInputPolicySignaturesRemainStable();
    void fillGeometryAndThresholdSignaturesRemainStable();
    void regionAndContinuousFillSignaturesRemainStable();
    void colorCompositionAndOutputSignaturesRemainStable();
};

void FillProcessingVisitorSchemaContractTest::typeEnumerationAndConstructionSchemaRemainStable()
{
    using Visitor = FillProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisSimpleProcessingVisitor, Visitor>);
    static_assert(std::has_virtual_destructor_v<Visitor>);
    static_assert(std::is_constructible_v<Visitor, KisPaintDeviceSP, KisSelectionSP, KisResourcesSnapshotSP>);
    static_assert(Visitor::ContinuousFillMode_DoNotUse == 0);
    static_assert(Visitor::ContinuousFillMode_FillAnyRegion == 1);
    static_assert(Visitor::ContinuousFillMode_FillSimilarRegions == 2);
}

void FillProcessingVisitorSchemaContractTest::seedSelectionAndInputPolicySignaturesRemainStable()
{
    using Visitor = FillProcessingVisitor;

    ASSERT_FILL_VISITOR_SIGNATURE(setSeedPoint, void (Visitor::*)(const QPoint &));
    ASSERT_FILL_VISITOR_SIGNATURE(setSeedPoints, void (Visitor::*)(const QVector<QPoint> &));
    ASSERT_FILL_VISITOR_SIGNATURE(setSelectionOnly, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setUnmerged, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setUsePattern, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setUseSelectionAsBoundary, void (Visitor::*)(bool));
}

void FillProcessingVisitorSchemaContractTest::fillGeometryAndThresholdSignaturesRemainStable()
{
    using Visitor = FillProcessingVisitor;

    ASSERT_FILL_VISITOR_SIGNATURE(setAntiAlias, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setCloseGap, void (Visitor::*)(int));
    ASSERT_FILL_VISITOR_SIGNATURE(setFeather, void (Visitor::*)(int));
    ASSERT_FILL_VISITOR_SIGNATURE(setFillThreshold, void (Visitor::*)(int));
    ASSERT_FILL_VISITOR_SIGNATURE(setOpacitySpread, void (Visitor::*)(int));
    ASSERT_FILL_VISITOR_SIGNATURE(setSizeMod, void (Visitor::*)(int));
    ASSERT_FILL_VISITOR_SIGNATURE(setStopGrowingAtDarkestPixel, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setUseFastMode, void (Visitor::*)(bool));
}

void FillProcessingVisitorSchemaContractTest::regionAndContinuousFillSignaturesRemainStable()
{
    using Visitor = FillProcessingVisitor;

    ASSERT_FILL_VISITOR_SIGNATURE(setContinuousFillMask, void (Visitor::*)(KisSelectionSP));
    ASSERT_FILL_VISITOR_SIGNATURE(setContinuousFillMode, void (Visitor::*)(Visitor::ContinuousFillMode));
    ASSERT_FILL_VISITOR_SIGNATURE(setContinuousFillReferenceColor, void (Visitor::*)(QSharedPointer<KoColor>));
    ASSERT_FILL_VISITOR_SIGNATURE(setRegionFillingBoundaryColor, void (Visitor::*)(const KoColor &));
    ASSERT_FILL_VISITOR_SIGNATURE(setRegionFillingMode, void (Visitor::*)(KisFillPainter::RegionFillingMode));
}

void FillProcessingVisitorSchemaContractTest::colorCompositionAndOutputSignaturesRemainStable()
{
    using Visitor = FillProcessingVisitor;

    ASSERT_FILL_VISITOR_SIGNATURE(setCustomCompositeOp, void (Visitor::*)(const QString &));
    ASSERT_FILL_VISITOR_SIGNATURE(setCustomOpacity, void (Visitor::*)(qreal));
    ASSERT_FILL_VISITOR_SIGNATURE(setOutDirtyRect, void (Visitor::*)(QSharedPointer<QRect>));
    ASSERT_FILL_VISITOR_SIGNATURE(setProgressHelper, void (Visitor::*)(QSharedPointer<Visitor::ProgressHelper>));
    ASSERT_FILL_VISITOR_SIGNATURE(setUseBgColor, void (Visitor::*)(bool));
    ASSERT_FILL_VISITOR_SIGNATURE(setUseCustomBlendingOptions, void (Visitor::*)(bool));
}

#undef ASSERT_FILL_VISITOR_SIGNATURE

QTEST_GUILESS_MAIN(FillProcessingVisitorSchemaContractTest)

#include "FillProcessingVisitorSchemaContractTest.moc"
