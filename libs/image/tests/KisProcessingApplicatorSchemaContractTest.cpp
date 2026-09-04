/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_processing_applicator.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PROCESSING_APPLICATOR_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisProcessingApplicator::method)), signature>)
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

QTEST_APPLESS_MAIN(KisProcessingApplicatorSchemaContractTest)

#include "KisProcessingApplicatorSchemaContractTest.moc"
