/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_abstract_projection_plane.h"

#include <QTest>

#include <array>
#include <cstddef>

bool kisSharedPtrRelease(KisNode *)
{
    qFatal("The projection-plane contract does not own a concrete KisNode");
}

namespace
{

enum class Operation {
    Recalculate,
    Apply,
    NeedRect,
    ChangeRect,
    AccessRect,
    NeedRectForOriginal,
    TightBounds,
    LooseBounds,
    LodDevices,
    Count
};

class ProjectionPlaneProbe final : public KisAbstractProjectionPlane
{
public:
    explicit ProjectionPlaneProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ProjectionPlaneProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QRect recalculate(const QRect &rect, KisNodeSP filthyNode, KisRenderPassFlags flags) override
    {
        record(Operation::Recalculate, rect);
        m_nodeWasNull = filthyNode.isNull();
        m_flags = flags;
        return QRect(101, 102, 103, 104);
    }

    void apply(KisPainter *painter, const QRect &rect) override
    {
        record(Operation::Apply, rect);
        m_painter = painter;
    }

    QRect needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const override
    {
        record(Operation::NeedRect, rect, pos);
        return QRect(201, 202, 203, 204);
    }

    QRect changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const override
    {
        record(Operation::ChangeRect, rect, pos);
        return QRect(301, 302, 303, 304);
    }

    QRect accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const override
    {
        record(Operation::AccessRect, rect, pos);
        return QRect(401, 402, 403, 404);
    }

    QRect needRectForOriginal(const QRect &rect) const override
    {
        record(Operation::NeedRectForOriginal, rect);
        return QRect(501, 502, 503, 504);
    }

    QRect tightUserVisibleBounds() const override
    {
        record(Operation::TightBounds);
        return QRect(601, 602, 603, 604);
    }

    QRect looseUserVisibleBounds() const override
    {
        record(Operation::LooseBounds);
        return QRect(701, 702, 703, 704);
    }

    KisPaintDeviceList getLodCapableDevices() const override
    {
        record(Operation::LodDevices);
        return {};
    }

    int callCount(Operation operation) const
    {
        return m_callCounts[static_cast<std::size_t>(operation)];
    }

    QRect lastRect() const
    {
        return m_lastRect;
    }

    KisLayer::PositionToFilthy lastPosition() const
    {
        return m_lastPosition;
    }

    bool nodeWasNull() const
    {
        return m_nodeWasNull;
    }

    KisRenderPassFlags flags() const
    {
        return m_flags;
    }

    KisPainter *painter() const
    {
        return m_painter;
    }

private:
    void
    record(Operation operation, const QRect &rect = {}, KisLayer::PositionToFilthy position = KisLayer::N_FILTHY) const
    {
        ++m_callCounts[static_cast<std::size_t>(operation)];
        m_lastRect = rect;
        m_lastPosition = position;
    }

private:
    mutable std::array<int, static_cast<std::size_t>(Operation::Count)> m_callCounts{};
    mutable QRect m_lastRect;
    mutable KisLayer::PositionToFilthy m_lastPosition{KisLayer::N_FILTHY};
    bool m_nodeWasNull{false};
    KisRenderPassFlags m_flags;
    KisPainter *m_painter{nullptr};
    int *m_destructionCount{nullptr};
};

struct OpaquePainterStorage {
    alignas(std::max_align_t) std::array<unsigned char, sizeof(std::max_align_t)> bytes{};
};

} // namespace

class KisAbstractProjectionPlaneContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsAndDestroysThroughBase();
    void dispatchesProjectionOperations();
    void dispatchesGeometryAndDeviceQueries();
    void dumbPlanePreservesNoOpResults();
};

void KisAbstractProjectionPlaneContractTest::constructsAndDestroysThroughBase()
{
    int destructionCount = 0;
    KisAbstractProjectionPlane *plane = new ProjectionPlaneProbe(&destructionCount);

    QCOMPARE(destructionCount, 0);
    delete plane;
    QCOMPARE(destructionCount, 1);
}

void KisAbstractProjectionPlaneContractTest::dispatchesProjectionOperations()
{
    ProjectionPlaneProbe probe;
    KisAbstractProjectionPlane &plane = probe;
    const QRect dirtyRect(11, 12, 13, 14);
    const KisRenderPassFlags flags = KisRenderPassFlag::NoTransformMaskUpdates;

    QCOMPARE(plane.recalculate(dirtyRect, {}, flags), QRect(101, 102, 103, 104));
    QCOMPARE(probe.callCount(Operation::Recalculate), 1);
    QCOMPARE(probe.lastRect(), dirtyRect);
    QVERIFY(probe.nodeWasNull());
    QCOMPARE(probe.flags(), flags);

    OpaquePainterStorage painterStorage;
    KisPainter *painter = reinterpret_cast<KisPainter *>(&painterStorage);
    const QRect applyRect(21, 22, 23, 24);
    plane.apply(painter, applyRect);
    QCOMPARE(probe.callCount(Operation::Apply), 1);
    QCOMPARE(probe.lastRect(), applyRect);
    QCOMPARE(probe.painter(), painter);
}

void KisAbstractProjectionPlaneContractTest::dispatchesGeometryAndDeviceQueries()
{
    ProjectionPlaneProbe probe;
    const KisAbstractProjectionPlane &plane = probe;

    const QRect needInput(31, 32, 33, 34);
    QCOMPARE(plane.needRect(needInput, KisLayer::N_ABOVE_FILTHY), QRect(201, 202, 203, 204));
    QCOMPARE(probe.callCount(Operation::NeedRect), 1);
    QCOMPARE(probe.lastRect(), needInput);
    QCOMPARE(probe.lastPosition(), KisLayer::N_ABOVE_FILTHY);

    const QRect changeInput(41, 42, 43, 44);
    QCOMPARE(plane.changeRect(changeInput, KisLayer::N_BELOW_FILTHY), QRect(301, 302, 303, 304));
    QCOMPARE(probe.callCount(Operation::ChangeRect), 1);
    QCOMPARE(probe.lastRect(), changeInput);
    QCOMPARE(probe.lastPosition(), KisLayer::N_BELOW_FILTHY);

    const QRect accessInput(51, 52, 53, 54);
    QCOMPARE(plane.accessRect(accessInput, KisLayer::N_FILTHY_PROJECTION), QRect(401, 402, 403, 404));
    QCOMPARE(probe.callCount(Operation::AccessRect), 1);
    QCOMPARE(probe.lastRect(), accessInput);
    QCOMPARE(probe.lastPosition(), KisLayer::N_FILTHY_PROJECTION);

    const QRect originalInput(61, 62, 63, 64);
    QCOMPARE(plane.needRectForOriginal(originalInput), QRect(501, 502, 503, 504));
    QCOMPARE(probe.callCount(Operation::NeedRectForOriginal), 1);
    QCOMPARE(probe.lastRect(), originalInput);

    QCOMPARE(plane.tightUserVisibleBounds(), QRect(601, 602, 603, 604));
    QCOMPARE(probe.callCount(Operation::TightBounds), 1);
    QCOMPARE(plane.looseUserVisibleBounds(), QRect(701, 702, 703, 704));
    QCOMPARE(probe.callCount(Operation::LooseBounds), 1);
    QVERIFY(plane.getLodCapableDevices().isEmpty());
    QCOMPARE(probe.callCount(Operation::LodDevices), 1);
}

void KisAbstractProjectionPlaneContractTest::dumbPlanePreservesNoOpResults()
{
    KisDumbProjectionPlane plane;
    const QRect rect(71, 72, 73, 74);

    QCOMPARE(plane.recalculate(rect, {}, KisRenderPassFlag::NoTransformMaskUpdates), rect);
    QCOMPARE(plane.needRect(rect, KisLayer::N_ABOVE_FILTHY), rect);
    QCOMPARE(plane.changeRect(rect, KisLayer::N_BELOW_FILTHY), rect);
    QCOMPARE(plane.accessRect(rect, KisLayer::N_FILTHY_PROJECTION), rect);
    QCOMPARE(plane.needRectForOriginal(rect), rect);

    OpaquePainterStorage painterStorage;
    plane.apply(reinterpret_cast<KisPainter *>(&painterStorage), rect);

    QVERIFY(plane.tightUserVisibleBounds().isEmpty());
    QVERIFY(plane.looseUserVisibleBounds().isEmpty());
    QVERIFY(plane.getLodCapableDevices().isEmpty());
}

QTEST_GUILESS_MAIN(KisAbstractProjectionPlaneContractTest)

#include "KisAbstractProjectionPlaneContractTest.moc"
