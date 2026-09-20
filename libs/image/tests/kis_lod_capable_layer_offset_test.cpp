/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_lod_capable_layer_offset_test.h"

#include <type_traits>
#include <utility>

#include "kis_lod_capable_layer_offset.h"

KisDefaultBoundsBase::~KisDefaultBoundsBase() = default;

QRect KisDefaultBoundsBase::imageBorderRect() const
{
    return bounds();
}

namespace
{
class BoundsProbe final : public KisDefaultBoundsBase
{
public:
    explicit BoundsProbe(int lod = 0, int *destructionCount = nullptr)
        : m_lod(lod)
        , m_destructionCount(destructionCount)
    {
    }

    ~BoundsProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QRect bounds() const override
    {
        return QRect(0, 0, 100, 100);
    }

    bool wrapAroundMode() const override
    {
        return false;
    }

    WrapAroundAxis wrapAroundModeAxis() const override
    {
        return WRAPAROUND_BOTH;
    }

    int currentLevelOfDetail() const override
    {
        return m_lod;
    }

    int currentTime() const override
    {
        return 0;
    }

    bool externalFrameActive() const override
    {
        return false;
    }

    void *sourceCookie() const override
    {
        return nullptr;
    }

    void setLod(int lod)
    {
        m_lod = lod;
    }

private:
    int m_lod;
    int *m_destructionCount;
};

struct SyncValue {
    int value = 0;
    int synchronizedLod = -1;

    SyncValue syncLodNValue(int lod) const
    {
        return {value + 10 * lod, lod};
    }
};

KisDefaultBoundsBaseSP makeBounds(int lod = 0, int *destructionCount = nullptr)
{
    return new BoundsProbe(lod, destructionCount);
}

BoundsProbe *boundsProbe(KisDefaultBoundsBaseSP &bounds)
{
    return static_cast<BoundsProbe *>(bounds.data());
}
} // namespace

void KisLodCapableLayerOffsetTest::testSynchronizationRules()
{
    static_assert(std::is_same_v<KisLodCapableLayerOffset, KisLodSwitchingWrapper<QPoint>>);
    static_assert(std::is_same_v<KisLodCapableLayerOffset::LodState, std::pair<int, QPoint>>);

    QCOMPARE(KisLodSwitchingWrapperDetail::syncLodNValue(QPoint(12, 16), 0), QPoint(12, 16));
    QCOMPARE(KisLodSwitchingWrapperDetail::syncLodNValue(QPoint(12, 16), 2), QPoint(3, 4));
    QCOMPARE(KisLodSwitchingWrapperDetail::syncLodNValue(QPoint(-9, 11), 2), QPoint(-2, 2));

    const SyncValue value{7, -1};
    const SyncValue synchronized = KisLodSwitchingWrapperDetail::syncLodNValue(value, 3);
    QCOMPARE(synchronized.value, 37);
    QCOMPARE(synchronized.synchronizedLod, 3);
}

void KisLodCapableLayerOffsetTest::testDefaultConstructionAndAccess()
{
    KisDefaultBoundsBaseSP bounds = makeBounds();
    KisLodCapableLayerOffset offset(bounds);

    QCOMPARE(*offset, QPoint());
    offset->setX(12);
    offset->setY(16);
    QCOMPARE(*offset, QPoint(12, 16));

    const KisLodCapableLayerOffset &constOffset = offset;
    QCOMPARE(*constOffset, QPoint(12, 16));
    QCOMPARE(constOffset->x(), 12);
    QCOMPARE(constOffset->y(), 16);

    boundsProbe(bounds)->setLod(1);
    QCOMPARE(*constOffset, QPoint());

    offset.syncLodCache();
    QCOMPARE(*constOffset, QPoint(6, 8));

    offset->setX(2);
    offset->setY(6);
    QCOMPARE(*offset, QPoint(2, 6));

    boundsProbe(bounds)->setLod(0);
    QCOMPARE(*offset, QPoint(12, 16));
}

void KisLodCapableLayerOffsetTest::testValueConstructionAndBounds()
{
    int firstDestructionCount = 0;
    int secondDestructionCount = 0;

    {
        KisDefaultBoundsBaseSP firstBounds = makeBounds(2, &firstDestructionCount);
        KisDefaultBoundsBaseSP secondBounds = makeBounds(0, &secondDestructionCount);
        KisDefaultBoundsBase *firstPointer = firstBounds.data();
        KisDefaultBoundsBase *secondPointer = secondBounds.data();

        KisLodCapableLayerOffset offset(QPoint(20, -12), firstBounds);
        QCOMPARE(offset.defaultBounds().data(), firstPointer);
        QCOMPARE(*offset, QPoint(5, -3));

        firstBounds.clear();
        QCOMPARE(firstDestructionCount, 0);

        offset.setDefaultBounds(secondBounds);
        QCOMPARE(firstDestructionCount, 1);
        QCOMPARE(offset.defaultBounds().data(), secondPointer);
        QCOMPARE(*offset, QPoint(20, -12));

        secondBounds.clear();
        QCOMPARE(secondDestructionCount, 0);
    }

    QCOMPARE(firstDestructionCount, 1);
    QCOMPARE(secondDestructionCount, 1);

    KisDefaultBoundsBaseSP bounds = makeBounds(2);
    KisLodSwitchingWrapper<SyncValue> value(SyncValue{7, -1}, bounds);
    QCOMPARE(value->value, 27);
    QCOMPARE(value->synchronizedLod, 2);

    boundsProbe(bounds)->setLod(0);
    QCOMPARE(value->value, 7);
    QCOMPARE(value->synchronizedLod, -1);
    value->value = 9;

    boundsProbe(bounds)->setLod(2);
    value.syncLodCache();
    QCOMPARE(value->value, 29);
    QCOMPARE(value->synchronizedLod, 2);
}

void KisLodCapableLayerOffsetTest::testCopyAndAssignment()
{
    KisDefaultBoundsBaseSP bounds = makeBounds();
    KisLodCapableLayerOffset source(bounds);
    *source = QPoint(12, 16);
    boundsProbe(bounds)->setLod(1);
    source.syncLodCache();

    KisLodCapableLayerOffset copy(source);
    QCOMPARE(*copy, QPoint(6, 8));

    boundsProbe(bounds)->setLod(0);
    *copy = QPoint(30, 40);
    QCOMPARE(*copy, QPoint(30, 40));
    QCOMPARE(*source, QPoint(12, 16));

    const KisDefaultBoundsBaseSP otherBounds = makeBounds(2);
    KisLodCapableLayerOffset assigned(otherBounds);
    assigned = source;
    QCOMPARE(assigned.defaultBounds().data(), bounds.data());
    QCOMPARE(*assigned, QPoint(12, 16));

    boundsProbe(bounds)->setLod(1);
    QCOMPARE(*assigned, QPoint(6, 8));

    KisLodCapableLayerOffset *identity = &assigned;
    assigned = *identity;
    QCOMPARE(&assigned, identity);
    QCOMPARE(*assigned, QPoint(6, 8));
}

void KisLodCapableLayerOffsetTest::testLodState()
{
    KisDefaultBoundsBaseSP bounds = makeBounds();
    KisLodCapableLayerOffset offset(bounds);

    offset.setLodState({0, QPoint(3, 4)});
    QCOMPARE(*offset, QPoint(3, 4));

    offset.setLodState({2, QPoint(30, 40)});
    QCOMPARE(*offset, QPoint(3, 4));

    boundsProbe(bounds)->setLod(1);
    QCOMPARE(*offset, QPoint(30, 40));

    const KisLodCapableLayerOffset::LodState state = offset.lodState();
    QCOMPARE(state.first, 1);
    QCOMPARE(state.second, QPoint(30, 40));

    const auto converted = static_cast<KisLodCapableLayerOffset::LodState>(offset);
    QCOMPARE(converted, state);

    offset = KisLodCapableLayerOffset::LodState{0, QPoint(5, 6)};
    QCOMPARE(*offset, QPoint(30, 40));
    boundsProbe(bounds)->setLod(0);
    QCOMPARE(*offset, QPoint(5, 6));

    offset = KisLodCapableLayerOffset::LodState{5, QPoint(50, 60)};
    boundsProbe(bounds)->setLod(2);
    QCOMPARE(*offset, QPoint(50, 60));
}

QTEST_GUILESS_MAIN(KisLodCapableLayerOffsetTest)
