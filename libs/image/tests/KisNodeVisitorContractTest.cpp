/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTranslateLayerNamesVisitor.h"

#include "kis_node_visitor.h"

#include <QTest>

#include <array>
#include <cstddef>
#include <type_traits>

namespace
{

enum class VisitKind {
    Node,
    PaintLayer,
    GroupLayer,
    AdjustmentLayer,
    ExternalLayer,
    GeneratorLayer,
    CloneLayer,
    FilterMask,
    TransformMask,
    TransparencyMask,
    SelectionMask,
    ColorizeMask,
    Count
};

class NodeVisitorProbe final : public KisNodeVisitor
{
public:
    explicit NodeVisitorProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~NodeVisitorProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    bool visit(KisNode *node) override
    {
        return record(VisitKind::Node, node);
    }

    bool visit(KisPaintLayer *layer) override
    {
        return record(VisitKind::PaintLayer, layer);
    }

    bool visit(KisGroupLayer *layer) override
    {
        return record(VisitKind::GroupLayer, layer);
    }

    bool visit(KisAdjustmentLayer *layer) override
    {
        return record(VisitKind::AdjustmentLayer, layer);
    }

    bool visit(KisExternalLayer *layer) override
    {
        return record(VisitKind::ExternalLayer, layer);
    }

    bool visit(KisGeneratorLayer *layer) override
    {
        return record(VisitKind::GeneratorLayer, layer);
    }

    bool visit(KisCloneLayer *layer) override
    {
        return record(VisitKind::CloneLayer, layer);
    }

    bool visit(KisFilterMask *mask) override
    {
        return record(VisitKind::FilterMask, mask);
    }

    bool visit(KisTransformMask *mask) override
    {
        return record(VisitKind::TransformMask, mask);
    }

    bool visit(KisTransparencyMask *mask) override
    {
        return record(VisitKind::TransparencyMask, mask);
    }

    bool visit(KisSelectionMask *mask) override
    {
        return record(VisitKind::SelectionMask, mask);
    }

    bool visit(KisColorizeMask *mask) override
    {
        return record(VisitKind::ColorizeMask, mask);
    }

    void setNextResult(bool result)
    {
        m_nextResult = result;
    }

    VisitKind lastKind() const
    {
        return m_lastKind;
    }

    const void *lastPointer() const
    {
        return m_lastPointer;
    }

    int callCount(VisitKind kind) const
    {
        return m_callCounts[static_cast<std::size_t>(kind)];
    }

    int totalCallCount() const
    {
        return m_totalCallCount;
    }

private:
    bool record(VisitKind kind, const void *pointer)
    {
        m_lastKind = kind;
        m_lastPointer = pointer;
        ++m_callCounts[static_cast<std::size_t>(kind)];
        ++m_totalCallCount;
        return m_nextResult;
    }

private:
    std::array<int, static_cast<std::size_t>(VisitKind::Count)> m_callCounts{};
    VisitKind m_lastKind{VisitKind::Node};
    const void *m_lastPointer{nullptr};
    int m_totalCallCount{0};
    int *m_destructionCount{nullptr};
    bool m_nextResult{true};
};

template<typename NodeType>
void verifyVisit(KisNodeVisitor &visitor,
                 NodeVisitorProbe &probe,
                 VisitKind expectedKind,
                 NodeType *expectedPointer,
                 bool expectedResult)
{
    const int previousCallCount = probe.callCount(expectedKind);
    probe.setNextResult(expectedResult);

    QCOMPARE(visitor.visit(expectedPointer), expectedResult);
    QCOMPARE(static_cast<int>(probe.lastKind()), static_cast<int>(expectedKind));
    QCOMPARE(probe.lastPointer(), static_cast<const void *>(expectedPointer));
    QCOMPARE(probe.callCount(expectedKind), previousCallCount + 1);
}

struct OpaquePointerStorage {
    alignas(std::max_align_t) std::array<unsigned char, sizeof(std::max_align_t)> bytes{};
};

} // namespace

class KisNodeVisitorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsAndDestroysThroughBase();
    void dispatchesNodeAndLayerVisits();
    void dispatchesMaskVisits();
    void translateLayerNamesVisitorTypeConstructionAndDictionarySchemaRemainStable();
    void translateLayerNamesVisitorBaseAndPaintLayerSignaturesRemainStable();
    void translateLayerNamesVisitorGeneratedAndExternalLayerSignaturesRemainStable();
    void translateLayerNamesVisitorCloneAndFilterMaskSignaturesRemainStable();
    void translateLayerNamesVisitorRemainingMaskSignaturesRemainStable();
};

void KisNodeVisitorContractTest::constructsAndDestroysThroughBase()
{
    int destructionCount = 0;
    KisNodeVisitor *visitor = new NodeVisitorProbe(&destructionCount);

    QCOMPARE(destructionCount, 0);
    delete visitor;
    QCOMPARE(destructionCount, 1);
}

void KisNodeVisitorContractTest::dispatchesNodeAndLayerVisits()
{
    std::array<OpaquePointerStorage, 7> storage;
    NodeVisitorProbe probe;
    KisNodeVisitor &visitor = probe;

    verifyVisit(visitor, probe, VisitKind::Node, reinterpret_cast<KisNode *>(&storage[0]), true);
    verifyVisit(visitor, probe, VisitKind::PaintLayer, reinterpret_cast<KisPaintLayer *>(&storage[1]), false);
    verifyVisit(visitor, probe, VisitKind::GroupLayer, reinterpret_cast<KisGroupLayer *>(&storage[2]), true);
    verifyVisit(visitor, probe, VisitKind::AdjustmentLayer, reinterpret_cast<KisAdjustmentLayer *>(&storage[3]), false);
    verifyVisit(visitor, probe, VisitKind::ExternalLayer, reinterpret_cast<KisExternalLayer *>(&storage[4]), true);
    verifyVisit(visitor, probe, VisitKind::GeneratorLayer, reinterpret_cast<KisGeneratorLayer *>(&storage[5]), false);
    verifyVisit(visitor, probe, VisitKind::CloneLayer, reinterpret_cast<KisCloneLayer *>(&storage[6]), true);

    QCOMPARE(probe.totalCallCount(), 7);
}

void KisNodeVisitorContractTest::dispatchesMaskVisits()
{
    std::array<OpaquePointerStorage, 5> storage;
    NodeVisitorProbe probe;
    KisNodeVisitor &visitor = probe;

    verifyVisit(visitor, probe, VisitKind::FilterMask, reinterpret_cast<KisFilterMask *>(&storage[0]), false);
    verifyVisit(visitor, probe, VisitKind::TransformMask, reinterpret_cast<KisTransformMask *>(&storage[1]), true);
    verifyVisit(visitor,
                probe,
                VisitKind::TransparencyMask,
                reinterpret_cast<KisTransparencyMask *>(&storage[2]),
                false);
    verifyVisit(visitor, probe, VisitKind::SelectionMask, reinterpret_cast<KisSelectionMask *>(&storage[3]), true);
    verifyVisit(visitor, probe, VisitKind::ColorizeMask, reinterpret_cast<KisColorizeMask *>(&storage[4]), false);

    QCOMPARE(probe.totalCallCount(), 5);
}

void KisNodeVisitorContractTest::translateLayerNamesVisitorTypeConstructionAndDictionarySchemaRemainStable()
{
    using Visitor = KisTranslateLayerNamesVisitor;
    using Dictionary = QMap<QString, QString>;
    using DictionaryQuery = Dictionary (Visitor::*)();

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor, Dictionary>);
    static_assert(std::is_same_v<decltype(static_cast<DictionaryQuery>(&Visitor::defaultDictionary)), DictionaryQuery>);

    QVERIFY(true);
}

void KisNodeVisitorContractTest::translateLayerNamesVisitorBaseAndPaintLayerSignaturesRemainStable()
{
    using Visitor = KisTranslateLayerNamesVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisNode *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisNode *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisPaintLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisPaintLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGroupLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGroupLayer *)>);

    QVERIFY(true);
}

void KisNodeVisitorContractTest::translateLayerNamesVisitorGeneratedAndExternalLayerSignaturesRemainStable()
{
    using Visitor = KisTranslateLayerNamesVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisAdjustmentLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisAdjustmentLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisExternalLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisExternalLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGeneratorLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGeneratorLayer *)>);

    QVERIFY(true);
}

void KisNodeVisitorContractTest::translateLayerNamesVisitorCloneAndFilterMaskSignaturesRemainStable()
{
    using Visitor = KisTranslateLayerNamesVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisCloneLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisCloneLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisFilterMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisFilterMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransformMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransformMask *)>);

    QVERIFY(true);
}

void KisNodeVisitorContractTest::translateLayerNamesVisitorRemainingMaskSignaturesRemainStable()
{
    using Visitor = KisTranslateLayerNamesVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransparencyMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransparencyMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisSelectionMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisSelectionMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisColorizeMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisColorizeMask *)>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisNodeVisitorContractTest)

#include "KisNodeVisitorContractTest.moc"
