/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_image_config.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImageConfig::method)), signature>)
} // namespace

class KisImageConfigResourceBudgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void memoryBudgetSignaturesRemainStable();
    void derivedTilePoolLimitSignaturesRemainStable();
    void swapStorageBudgetSignaturesRemainStable();
    void workerSchedulingSignaturesRemainStable();
    void updatePatchPartitionSignaturesRemainStable();
};

void KisImageConfigResourceBudgetSchemaContractTest::memoryBudgetSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(defaultMemoryHardLimitPercent, qreal (*)());
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maximumMemoryHardLimitPercent, qreal (*)());
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(memoryHardLimitPercent, qreal (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(memoryPoolLimitPercent, qreal (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(memorySoftLimitPercent, qreal (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMemoryHardLimitPercent, void (KisImageConfig::*)(qreal));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMemoryPoolLimitPercent, void (KisImageConfig::*)(qreal));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMemorySoftLimitPercent, void (KisImageConfig::*)(qreal));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(totalRAM, int (*)());

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().memoryHardLimitPercent()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().memoryPoolLimitPercent()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().memorySoftLimitPercent()), qreal>);

    QVERIFY(true);
}

void KisImageConfigResourceBudgetSchemaContractTest::derivedTilePoolLimitSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(poolLimit, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(tilesHardLimit, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(tilesSoftLimit, int (KisImageConfig::*)() const);

    QVERIFY(true);
}

void KisImageConfigResourceBudgetSchemaContractTest::swapStorageBudgetSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxSwapSize, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMaxSwapSize, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSwapDir, void (KisImageConfig::*)(const QString &));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSwapSlabSize, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSwapWindowSize, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(swapDir, QString (KisImageConfig::*)(bool));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(swapSlabSize, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(swapWindowSize, int (KisImageConfig::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().maxSwapSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<KisImageConfig &>().swapDir()), QString>);

    QVERIFY(true);
}

void KisImageConfigResourceBudgetSchemaContractTest::workerSchedulingSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(maxNumberOfThreads, int (KisImageConfig::*)(bool) const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(schedulerBalancingRatio, qreal (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setMaxNumberOfThreads, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setSchedulerBalancingRatio, void (KisImageConfig::*)(qreal));

    static_assert(std::is_same_v<decltype(std::declval<const KisImageConfig &>().maxNumberOfThreads()), int>);

    QVERIFY(true);
}

void KisImageConfigResourceBudgetSchemaContractTest::updatePatchPartitionSignaturesRemainStable()
{
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUpdatePatchHeight, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(setUpdatePatchWidth, void (KisImageConfig::*)(int));
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(updatePatchHeight, int (KisImageConfig::*)() const);
    ASSERT_KIS_IMAGE_CONFIG_SIGNATURE(updatePatchWidth, int (KisImageConfig::*)() const);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisImageConfigResourceBudgetSchemaContractTest)

#include "KisImageConfigResourceBudgetSchemaContractTest.moc"
