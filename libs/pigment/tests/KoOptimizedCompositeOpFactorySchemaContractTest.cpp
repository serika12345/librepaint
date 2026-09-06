/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <compositeops/KoOptimizedCompositeOpFactory.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(method)                                                           \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(&KoOptimizedCompositeOpFactory::method), KoCompositeOp *(*)(const KoColorSpace *)>)
} // namespace

class KoOptimizedCompositeOpFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void factoryTypeSchemaRemainsStable();
    void factory32BitSignaturesRemainStable();
    void factory128BitSignaturesRemainStable();
    void factoryU64SignaturesRemainStable();
};

void KoOptimizedCompositeOpFactorySchemaContractTest::factoryTypeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoOptimizedCompositeOpFactory>);
}

void KoOptimizedCompositeOpFactorySchemaContractTest::factory32BitSignaturesRemainStable()
{
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpHard32);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpCreamy32);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createOverOp32);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createCopyOp32);
}

void KoOptimizedCompositeOpFactorySchemaContractTest::factory128BitSignaturesRemainStable()
{
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpHard128);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpCreamy128);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createOverOp128);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createCopyOp128);
}

void KoOptimizedCompositeOpFactorySchemaContractTest::factoryU64SignaturesRemainStable()
{
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpHardU64);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createAlphaDarkenOpCreamyU64);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createOverOpU64);
    ASSERT_OPTIMIZED_COMPOSITE_FACTORY_SIGNATURE(createCopyOpU64);
}

QTEST_GUILESS_MAIN(KoOptimizedCompositeOpFactorySchemaContractTest)

#include "KoOptimizedCompositeOpFactorySchemaContractTest.moc"
