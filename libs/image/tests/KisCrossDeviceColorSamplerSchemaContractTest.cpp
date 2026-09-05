/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cross_device_color_sampler.h"

#include <QTest>

#include <type_traits>

class KisCrossDeviceColorSamplerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void realTraitSchemaRemainStable();
    void integerTraitSchemaRemainStable();
    void samplerTypeAliasesRemainStable();
    void samplerConstructionAndLifetimeSchemaRemainStable();
    void samplerSamplingSignaturesRemainStable();
};

void KisCrossDeviceColorSamplerSchemaContractTest::realTraitSchemaRemainStable()
{
    using CreateAccessor = SamplerTraitReal::accessor_type (*)(KisPaintDeviceSP);
    using SampleData = void (*)(SamplerTraitReal::accessor_type, quint8 *, const KoColorSpace *);

    static_assert(std::is_class_v<SamplerTraitReal>);
    static_assert(std::is_same_v<SamplerTraitReal::coord_type, qreal>);
    static_assert(std::is_same_v<SamplerTraitReal::accessor_type, KisRandomSubAccessorSP>);
    static_assert(
        std::is_same_v<decltype(static_cast<CreateAccessor>(&SamplerTraitReal::createAccessor)), CreateAccessor>);
    static_assert(std::is_same_v<decltype(static_cast<SampleData>(&SamplerTraitReal::sampleData<false>)), SampleData>);
    static_assert(std::is_same_v<decltype(static_cast<SampleData>(&SamplerTraitReal::sampleData<true>)), SampleData>);
    QVERIFY(true);
}

void KisCrossDeviceColorSamplerSchemaContractTest::integerTraitSchemaRemainStable()
{
    using CreateAccessor = SamplerTraitInt::accessor_type (*)(KisPaintDeviceSP);
    using SampleData = void (*)(SamplerTraitInt::accessor_type, quint8 *, const KoColorSpace *);

    static_assert(std::is_class_v<SamplerTraitInt>);
    static_assert(std::is_same_v<SamplerTraitInt::coord_type, int>);
    static_assert(std::is_same_v<SamplerTraitInt::accessor_type, KisRandomConstAccessorSP>);
    static_assert(
        std::is_same_v<decltype(static_cast<CreateAccessor>(&SamplerTraitInt::createAccessor)), CreateAccessor>);
    static_assert(std::is_same_v<decltype(static_cast<SampleData>(&SamplerTraitInt::sampleData<false>)), SampleData>);
    static_assert(std::is_same_v<decltype(static_cast<SampleData>(&SamplerTraitInt::sampleData<true>)), SampleData>);
    QVERIFY(true);
}

void KisCrossDeviceColorSamplerSchemaContractTest::samplerTypeAliasesRemainStable()
{
    using RealSampler = KisCrossDeviceColorSamplerImpl<SamplerTraitReal>;
    using IntegerSampler = KisCrossDeviceColorSamplerImpl<SamplerTraitInt>;

    static_assert(std::is_class_v<RealSampler>);
    static_assert(std::is_same_v<KisCrossDeviceColorSampler, RealSampler>);
    static_assert(std::is_same_v<KisCrossDeviceColorSamplerInt, IntegerSampler>);
    QVERIFY(true);
}

void KisCrossDeviceColorSamplerSchemaContractTest::samplerConstructionAndLifetimeSchemaRemainStable()
{
    using Sampler = KisCrossDeviceColorSamplerImpl<SamplerTraitReal>;

    static_assert(std::is_constructible_v<Sampler, KisPaintDeviceSP, KisFixedPaintDeviceSP>);
    static_assert(std::is_constructible_v<Sampler, KisPaintDeviceSP, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<Sampler, KisPaintDeviceSP, const KoColor &>);
    static_assert(std::is_destructible_v<Sampler>);
    QVERIFY(true);
}

void KisCrossDeviceColorSamplerSchemaContractTest::samplerSamplingSignaturesRemainStable()
{
    using RealSampler = KisCrossDeviceColorSamplerImpl<SamplerTraitReal>;
    using IntegerSampler = KisCrossDeviceColorSamplerImpl<SamplerTraitInt>;
    using RealSample = void (RealSampler::*)(qreal, qreal, quint8 *);
    using IntegerSample = void (IntegerSampler::*)(int, int, quint8 *);

    static_assert(std::is_same_v<decltype(static_cast<RealSample>(&RealSampler::sampleColor)), RealSample>);
    static_assert(std::is_same_v<decltype(static_cast<RealSample>(&RealSampler::sampleOldColor)), RealSample>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerSample>(&IntegerSampler::sampleColor)), IntegerSample>);
    static_assert(std::is_same_v<decltype(static_cast<IntegerSample>(&IntegerSampler::sampleOldColor)), IntegerSample>);
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisCrossDeviceColorSamplerSchemaContractTest)

#include "KisCrossDeviceColorSamplerSchemaContractTest.moc"
