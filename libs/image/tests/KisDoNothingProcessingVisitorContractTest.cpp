/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "processing/kis_do_nothing_processing_visitor.h"

#include <QTest>

KisProcessingVisitor::~KisProcessingVisitor() = default;

KUndo2Command *KisProcessingVisitor::createInitCommand()
{
    return nullptr;
}

namespace
{

class LifetimeProbe : public KisDoNothingProcessingVisitor
{
public:
    explicit LifetimeProbe(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~LifetimeProbe() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

class KisDoNothingProcessingVisitorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeAndPaintContainersAcceptNullInputs();
    void generatedAndExternalLayersAcceptNullInputs();
    void maskKindsAcceptNullInputs();
    void virtualLifetimeDeletesThroughProcessingVisitor();
};

void KisDoNothingProcessingVisitorContractTest::nodeAndPaintContainersAcceptNullInputs()
{
    KisDoNothingProcessingVisitor visitor;

    visitor.visit(static_cast<KisNode *>(nullptr), nullptr);
    visitor.visit(static_cast<KisPaintLayer *>(nullptr), nullptr);
    visitor.visit(static_cast<KisGroupLayer *>(nullptr), nullptr);

}

void KisDoNothingProcessingVisitorContractTest::generatedAndExternalLayersAcceptNullInputs()
{
    KisDoNothingProcessingVisitor visitor;

    visitor.visit(static_cast<KisAdjustmentLayer *>(nullptr), nullptr);
    visitor.visit(static_cast<KisExternalLayer *>(nullptr), nullptr);
    visitor.visit(static_cast<KisGeneratorLayer *>(nullptr), nullptr);
    visitor.visit(static_cast<KisCloneLayer *>(nullptr), nullptr);

}

void KisDoNothingProcessingVisitorContractTest::maskKindsAcceptNullInputs()
{
    KisDoNothingProcessingVisitor visitor;

    visitor.visit(static_cast<KisFilterMask *>(nullptr), nullptr);
    visitor.visit(static_cast<KisTransformMask *>(nullptr), nullptr);
    visitor.visit(static_cast<KisTransparencyMask *>(nullptr), nullptr);
    visitor.visit(static_cast<KisSelectionMask *>(nullptr), nullptr);
    visitor.visit(static_cast<KisColorizeMask *>(nullptr), nullptr);

}

void KisDoNothingProcessingVisitorContractTest::virtualLifetimeDeletesThroughProcessingVisitor()
{
    int destructionCount = 0;
    KisProcessingVisitor *visitor = new LifetimeProbe(&destructionCount);

    delete visitor;

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisDoNothingProcessingVisitorContractTest)

#include "KisDoNothingProcessingVisitorContractTest.moc"
