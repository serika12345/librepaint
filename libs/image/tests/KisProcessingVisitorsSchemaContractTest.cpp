/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "processing/KisSelectionBasedProcessingHelper.h"
#include "processing/kis_assign_profile_processing_visitor.h"
#include "processing/kis_convert_color_space_processing_visitor.h"
#include "processing/kis_crop_processing_visitor.h"
#include "processing/kis_mirror_processing_visitor.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

} // namespace

class KisProcessingVisitorsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void helperTypeAliasAndConstructionSchemaRemainStable();
    void helperSelectionSignatureRemainsStable();
    void helperInitializationCommandSignaturesRemainStable();
    void helperTransformationSignaturesRemainStable();
    void colorConversionTypeAndConstructionSchemaRemainStable();
    void colorConversionVisitSignaturesRemainStable();
    void mirrorVisitorSchemaRemainsStable();
    void cropVisitorSchemaRemainsStable();
    void assignProfileVisitorSchemaRemainsStable();
};

void KisProcessingVisitorsSchemaContractTest::helperTypeAliasAndConstructionSchemaRemainStable()
{
    using Helper = KisSelectionBasedProcessingHelper;

    static_assert(std::is_class_v<Helper>);
    static_assert(std::is_same_v<Helper::Functor, std::function<void(KisPaintDeviceSP)>>);
    static_assert(std::is_constructible_v<Helper, KisSelectionSP, Helper::Functor>);

    QVERIFY(true);
}

void KisProcessingVisitorsSchemaContractTest::helperSelectionSignatureRemainsStable()
{
    ASSERT_SIGNATURE(KisSelectionBasedProcessingHelper,
                     setSelection,
                     void (KisSelectionBasedProcessingHelper::*)(KisSelectionSP));
}

void KisProcessingVisitorsSchemaContractTest::helperInitializationCommandSignaturesRemainStable()
{
    using Helper = KisSelectionBasedProcessingHelper;

    ASSERT_SIGNATURE(Helper, createInitCommand, KUndo2Command * (Helper::*)());
    ASSERT_SIGNATURE(Helper, createInitCommand, KUndo2Command * (Helper::*)(Helper::Functor));
}

void KisProcessingVisitorsSchemaContractTest::helperTransformationSignaturesRemainStable()
{
    using Helper = KisSelectionBasedProcessingHelper;

    ASSERT_SIGNATURE(Helper, transformPaintDevice, void (Helper::*)(KisPaintDeviceSP, KisUndoAdapter *));
    ASSERT_SIGNATURE(Helper,
                     transformPaintDevice,
                     void (Helper::*)(KisPaintDeviceSP, KisUndoAdapter *, Helper::Functor));
}

void KisProcessingVisitorsSchemaContractTest::colorConversionTypeAndConstructionSchemaRemainStable()
{
    using Visitor = KisConvertColorSpaceProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisSimpleProcessingVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor,
                                          const KoColorSpace *,
                                          const KoColorSpace *,
                                          KoColorConversionTransformation::Intent,
                                          KoColorConversionTransformation::ConversionFlags>);

    QVERIFY(true);
}

void KisProcessingVisitorsSchemaContractTest::colorConversionVisitSignaturesRemainStable()
{
    using Visitor = KisConvertColorSpaceProcessingVisitor;

    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisGeneratorLayer *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisAdjustmentLayer *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisFilterMask *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisGroupLayer *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visitColorizeMask, void (Visitor::*)(KisColorizeMask *, KisUndoAdapter *));
}

void KisProcessingVisitorsSchemaContractTest::mirrorVisitorSchemaRemainsStable()
{
    using Visitor = KisMirrorProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisSimpleProcessingVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor, const QRect &, Qt::Orientation>);
    static_assert(std::is_constructible_v<Visitor, KisSelectionSP, Qt::Orientation>);
    ASSERT_SIGNATURE(
        Visitor,
        applyToNodes,
        void (*)(KisImageSP, const KisNodeList &, Qt::Orientation, KisSelectionSP, const KUndo2MagicString &));
}

void KisProcessingVisitorsSchemaContractTest::cropVisitorSchemaRemainsStable()
{
    using Visitor = KisCropProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisSimpleProcessingVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor, const QRect &, bool, bool>);
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visitColorizeMask, void (Visitor::*)(KisColorizeMask *, KisUndoAdapter *));
}

void KisProcessingVisitorsSchemaContractTest::assignProfileVisitorSchemaRemainsStable()
{
    using Visitor = KisAssignProfileProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisSimpleProcessingVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor, const KoColorSpace *, const KoColorSpace *>);
    ASSERT_SIGNATURE(Visitor, visit, void (Visitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_SIGNATURE(Visitor, visitColorizeMask, void (Visitor::*)(KisColorizeMask *, KisUndoAdapter *));
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisProcessingVisitorsSchemaContractTest)

#include "KisProcessingVisitorsSchemaContractTest.moc"
