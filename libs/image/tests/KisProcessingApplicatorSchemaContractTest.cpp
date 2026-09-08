/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_processing_applicator.h"
#include "processing/kis_simple_processing_visitor.h"
#include "processing/kis_transform_processing_visitor.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PROCESSING_APPLICATOR_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisProcessingApplicator::method)), signature>)
#define ASSERT_TRANSFORM_VISITOR_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTransformProcessingVisitor::method)), signature>)
#define ASSERT_SIMPLE_VISITOR_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSimpleProcessingVisitor::method)), signature>)
} // namespace

class KisProcessingApplicatorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void processingApplicatorTypeAndFlagSchemaRemainStable();
    void processingApplicatorConstructionAndLifetimeSchemaRemainStable();
    void processingApplicatorVisitorAndCommandSignaturesRemainStable();
    void processingApplicatorCompletionAndControlSignaturesRemainStable();
    void processingApplicatorSingleCommandSignatureRemainsStable();
    void transformVisitorTypeAndConstructionSchemaRemainStable();
    void transformVisitorSelectionAndInitializationSignaturesRemainStable();
    void transformVisitorNodeAndLayerSignaturesRemainStable();
    void transformVisitorExternalAndFilterSignaturesRemainStable();
    void transformVisitorMaskSignaturesRemainStable();
    void simpleVisitorTypeAndLifetimeSchemaRemainStable();
    void simpleVisitorNodeAndLayerSignaturesRemainStable();
    void simpleVisitorExternalAndFilterSignaturesRemainStable();
    void simpleVisitorMaskSignaturesRemainStable();
};

void KisProcessingApplicatorSchemaContractTest::processingApplicatorTypeAndFlagSchemaRemainStable()
{
    using Flag = KisProcessingApplicator::ProcessingFlag;
    using Flags = KisProcessingApplicator::ProcessingFlags;

    static_assert(std::is_class_v<KisProcessingApplicator>);
    static_assert(std::is_enum_v<Flag>);
    static_assert(std::is_same_v<Flags, QFlags<Flag>>);
    static_assert(KisProcessingApplicator::NONE == 0x0);
    static_assert(KisProcessingApplicator::RECURSIVE == 0x1);
    static_assert(KisProcessingApplicator::NO_UI_UPDATES == 0x2);
    static_assert(KisProcessingApplicator::SUPPORTS_WRAPAROUND_MODE == 0x4);
    static_assert(KisProcessingApplicator::NO_IMAGE_UPDATES == 0x8);
}

void KisProcessingApplicatorSchemaContractTest::processingApplicatorConstructionAndLifetimeSchemaRemainStable()
{
    using Flags = KisProcessingApplicator::ProcessingFlags;

    static_assert(std::is_constructible_v<KisProcessingApplicator,
                                          KisImageWSP,
                                          KisNodeSP,
                                          Flags,
                                          KisImageSignalVector,
                                          const KUndo2MagicString &,
                                          KUndo2CommandExtraData *,
                                          int>);
    static_assert(std::is_constructible_v<KisProcessingApplicator,
                                          KisImageWSP,
                                          KisNodeList,
                                          Flags,
                                          KisImageSignalVector,
                                          const KUndo2MagicString &,
                                          KUndo2CommandExtraData *,
                                          int>);
    static_assert(
        std::is_same_v<decltype(KisProcessingApplicator(std::declval<KisImageWSP>(), std::declval<KisNodeSP>())),
                       KisProcessingApplicator>);
    static_assert(
        std::is_same_v<decltype(KisProcessingApplicator(std::declval<KisImageWSP>())), KisProcessingApplicator>);
    static_assert(std::is_destructible_v<KisProcessingApplicator>);
}

void KisProcessingApplicatorSchemaContractTest::processingApplicatorVisitorAndCommandSignaturesRemainStable()
{
    using Sequentiality = KisStrokeJobData::Sequentiality;
    using Exclusivity = KisStrokeJobData::Exclusivity;

    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(
        applyCommand,
        void (KisProcessingApplicator::*)(KUndo2Command *, Sequentiality, Exclusivity));
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(
        applyVisitor,
        void (KisProcessingApplicator::*)(KisProcessingVisitorSP, Sequentiality, Exclusivity));
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(
        applyVisitorAllFrames,
        void (KisProcessingApplicator::*)(KisProcessingVisitorSP, Sequentiality, Exclusivity));

    static_assert(std::is_same_v<decltype(std::declval<KisProcessingApplicator &>().applyCommand(
                                     std::declval<KUndo2Command *>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<KisProcessingApplicator &>().applyVisitor(
                                     std::declval<KisProcessingVisitorSP>())),
                                 void>);
    static_assert(std::is_same_v<decltype(std::declval<KisProcessingApplicator &>().applyVisitorAllFrames(
                                     std::declval<KisProcessingVisitorSP>())),
                                 void>);
}

void KisProcessingApplicatorSchemaContractTest::processingApplicatorCompletionAndControlSignaturesRemainStable()
{
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(cancel, void (KisProcessingApplicator::*)());
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(end, void (KisProcessingApplicator::*)());
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(explicitlyEmitFinalSignals, void (KisProcessingApplicator::*)());
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(getStroke, const KisStrokeId (KisProcessingApplicator::*)() const);
    ASSERT_PROCESSING_APPLICATOR_SIGNATURE(successfullyCompletedFuture,
                                           std::future<bool> && (KisProcessingApplicator::*)());
}

void KisProcessingApplicatorSchemaContractTest::processingApplicatorSingleCommandSignatureRemainsStable()
{
    using Sequentiality = KisStrokeJobData::Sequentiality;
    using Exclusivity = KisStrokeJobData::Exclusivity;
    using RunSingleCommandStroke = void (*)(KisImageSP, KUndo2Command *, Sequentiality, Exclusivity);

    static_assert(
        std::is_same_v<decltype(static_cast<RunSingleCommandStroke>(&KisProcessingApplicator::runSingleCommandStroke)),
                       RunSingleCommandStroke>);
    static_assert(
        std::is_same_v<decltype(KisProcessingApplicator::runSingleCommandStroke(std::declval<KisImageSP>(),
                                                                                std::declval<KUndo2Command *>())),
                       void>);
}

void KisProcessingApplicatorSchemaContractTest::transformVisitorTypeAndConstructionSchemaRemainStable()
{
    using Visitor = KisTransformProcessingVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisProcessingVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          KisFilterStrategy *,
                                          const QTransform &>);
    static_assert(std::is_same_v<decltype(Visitor(std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<qreal>(),
                                                  std::declval<KisFilterStrategy *>())),
                                 Visitor>);
}

void KisProcessingApplicatorSchemaContractTest::transformVisitorSelectionAndInitializationSignaturesRemainStable()
{
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(setSelection, void (KisTransformProcessingVisitor::*)(KisSelectionSP));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(createInitCommand, KUndo2Command * (KisTransformProcessingVisitor::*)());
}

void KisProcessingApplicatorSchemaContractTest::transformVisitorNodeAndLayerSignaturesRemainStable()
{
    using Visitor = KisTransformProcessingVisitor;

    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisNode *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisPaintLayer *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisGroupLayer *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisAdjustmentLayer *, KisUndoAdapter *));
}

void KisProcessingApplicatorSchemaContractTest::transformVisitorExternalAndFilterSignaturesRemainStable()
{
    using Visitor = KisTransformProcessingVisitor;

    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisExternalLayer *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisGeneratorLayer *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisCloneLayer *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisFilterMask *, KisUndoAdapter *));
}

void KisProcessingApplicatorSchemaContractTest::transformVisitorMaskSignaturesRemainStable()
{
    using Visitor = KisTransformProcessingVisitor;

    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisTransparencyMask *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisSelectionMask *, KisUndoAdapter *));
    ASSERT_TRANSFORM_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisColorizeMask *, KisUndoAdapter *));
}

void KisProcessingApplicatorSchemaContractTest::simpleVisitorTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSimpleProcessingVisitor>);
    static_assert(std::is_abstract_v<KisSimpleProcessingVisitor>);
    static_assert(std::is_base_of_v<KisProcessingVisitor, KisSimpleProcessingVisitor>);
    static_assert(std::has_virtual_destructor_v<KisSimpleProcessingVisitor>);
}

void KisProcessingApplicatorSchemaContractTest::simpleVisitorNodeAndLayerSignaturesRemainStable()
{
    using Visitor = KisSimpleProcessingVisitor;

    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisNode *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisPaintLayer *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisGroupLayer *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisAdjustmentLayer *, KisUndoAdapter *));
}

void KisProcessingApplicatorSchemaContractTest::simpleVisitorExternalAndFilterSignaturesRemainStable()
{
    using Visitor = KisSimpleProcessingVisitor;

    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisExternalLayer *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisGeneratorLayer *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisCloneLayer *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisFilterMask *, KisUndoAdapter *));
}

void KisProcessingApplicatorSchemaContractTest::simpleVisitorMaskSignaturesRemainStable()
{
    using Visitor = KisSimpleProcessingVisitor;

    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisTransparencyMask *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisSelectionMask *, KisUndoAdapter *));
    ASSERT_SIMPLE_VISITOR_SIGNATURE(visit, void (Visitor::*)(KisColorizeMask *, KisUndoAdapter *));
}

#undef ASSERT_SIMPLE_VISITOR_SIGNATURE
#undef ASSERT_TRANSFORM_VISITOR_SIGNATURE

QTEST_APPLESS_MAIN(KisProcessingApplicatorSchemaContractTest)

#include "KisProcessingApplicatorSchemaContractTest.moc"
