/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorConversionTransformationAbstractFactory.h>

#include <QTest>

#include <array>
#include <memory>
#include <utility>

class FactoryProbe final : public KoColorConversionTransformationAbstractFactory
{
public:
    FactoryProbe() = default;

    ~FactoryProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    KoColorConversionTransformation *
    createColorTransformation(const KoColorSpace *srcColorSpace,
                              const KoColorSpace *dstColorSpace,
                              KoColorConversionTransformation::Intent renderingIntent,
                              KoColorConversionTransformation::ConversionFlags conversionFlags) const override
    {
        ++conversionCallCount;
        conversionSource = srcColorSpace;
        conversionDestination = dstColorSpace;
        conversionRenderingIntent = renderingIntent;
        receivedConversionFlags = conversionFlags;
        return conversionResult;
    }

    KoColorProofingConversionTransformation *createColorProofingTransformation(
        const KoColorSpace *srcColorSpace,
        const KoColorSpace *dstColorSpace,
        const KoColorSpace *proofingSpace,
        KoColorProofingConversionTransformation::Intent renderingIntent,
        KoColorProofingConversionTransformation::Intent proofingIntent,
        bool bpcFirstTransform,
        quint8 *gamutWarning,
        KoColorConversionTransformation::ConversionFlags displayConversionFlags) const override
    {
        ++proofingCallCount;
        proofingSource = srcColorSpace;
        proofingDestination = dstColorSpace;
        receivedProofingSpace = proofingSpace;
        proofingRenderingIntent = renderingIntent;
        receivedProofingIntent = proofingIntent;
        receivedBpcFirstTransform = bpcFirstTransform;
        receivedGamutWarning = gamutWarning;
        receivedDisplayConversionFlags = displayConversionFlags;
        return proofingResult;
    }

    int *destructionCount = nullptr;
    KoColorConversionTransformation *conversionResult = nullptr;
    KoColorProofingConversionTransformation *proofingResult = nullptr;

    mutable int conversionCallCount = 0;
    mutable const KoColorSpace *conversionSource = nullptr;
    mutable const KoColorSpace *conversionDestination = nullptr;
    mutable KoColorConversionTransformation::Intent conversionRenderingIntent =
        KoColorConversionTransformation::IntentPerceptual;
    mutable KoColorConversionTransformation::ConversionFlags receivedConversionFlags;

    mutable int proofingCallCount = 0;
    mutable const KoColorSpace *proofingSource = nullptr;
    mutable const KoColorSpace *proofingDestination = nullptr;
    mutable const KoColorSpace *receivedProofingSpace = nullptr;
    mutable KoColorProofingConversionTransformation::Intent proofingRenderingIntent =
        KoColorProofingConversionTransformation::IntentPerceptual;
    mutable KoColorProofingConversionTransformation::Intent receivedProofingIntent =
        KoColorProofingConversionTransformation::IntentPerceptual;
    mutable bool receivedBpcFirstTransform = false;
    mutable quint8 *receivedGamutWarning = nullptr;
    mutable KoColorConversionTransformation::ConversionFlags receivedDisplayConversionFlags;
};

class KoColorConversionTransformationAbstractFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void conversionCreationPreservesArgumentsAndReturn();
    void proofingCreationPreservesArgumentsAndReturn();
    void defaultConstructionHasVirtualLifetime();
};

void KoColorConversionTransformationAbstractFactoryContractTest::conversionCreationPreservesArgumentsAndReturn()
{
    int sourceStorage = 0;
    int destinationStorage = 0;
    int resultStorage = 0;
    const auto *source = reinterpret_cast<const KoColorSpace *>(&sourceStorage);
    const auto *destination = reinterpret_cast<const KoColorSpace *>(&destinationStorage);
    auto *result = reinterpret_cast<KoColorConversionTransformation *>(&resultStorage);
    const auto flags = KoColorConversionTransformation::ConversionFlags(
        KoColorConversionTransformation::BlackpointCompensation | KoColorConversionTransformation::NoWhiteOnWhiteFixup);
    FactoryProbe factory;
    factory.conversionResult = result;
    const KoColorConversionTransformationAbstractFactory &interface = factory;

    QCOMPARE(interface.createColorTransformation(source,
                                                 destination,
                                                 KoColorConversionTransformation::IntentRelativeColorimetric,
                                                 flags),
             result);
    QCOMPARE(factory.conversionCallCount, 1);
    QCOMPARE(factory.conversionSource, source);
    QCOMPARE(factory.conversionDestination, destination);
    QCOMPARE(factory.conversionRenderingIntent, KoColorConversionTransformation::IntentRelativeColorimetric);
    QCOMPARE(factory.receivedConversionFlags, flags);
}

void KoColorConversionTransformationAbstractFactoryContractTest::proofingCreationPreservesArgumentsAndReturn()
{
    int sourceStorage = 0;
    int destinationStorage = 0;
    int proofingStorage = 0;
    int resultStorage = 0;
    const auto *source = reinterpret_cast<const KoColorSpace *>(&sourceStorage);
    const auto *destination = reinterpret_cast<const KoColorSpace *>(&destinationStorage);
    const auto *proofingSpace = reinterpret_cast<const KoColorSpace *>(&proofingStorage);
    auto *result = reinterpret_cast<KoColorProofingConversionTransformation *>(&resultStorage);
    std::array<quint8, 4> gamutWarning{1, 2, 3, 4};
    const auto flags = KoColorConversionTransformation::ConversionFlags(KoColorConversionTransformation::SoftProofing
                                                                        | KoColorConversionTransformation::GamutCheck);
    FactoryProbe factory;
    factory.proofingResult = result;
    const KoColorConversionTransformationAbstractFactory &interface = factory;

    QCOMPARE(
        interface.createColorProofingTransformation(source,
                                                    destination,
                                                    proofingSpace,
                                                    KoColorProofingConversionTransformation::IntentPerceptual,
                                                    KoColorProofingConversionTransformation::IntentAbsoluteColorimetric,
                                                    true,
                                                    gamutWarning.data(),
                                                    flags),
        result);
    QCOMPARE(factory.proofingCallCount, 1);
    QCOMPARE(factory.proofingSource, source);
    QCOMPARE(factory.proofingDestination, destination);
    QCOMPARE(factory.receivedProofingSpace, proofingSpace);
    QCOMPARE(factory.proofingRenderingIntent, KoColorProofingConversionTransformation::IntentPerceptual);
    QCOMPARE(factory.receivedProofingIntent, KoColorProofingConversionTransformation::IntentAbsoluteColorimetric);
    QVERIFY(factory.receivedBpcFirstTransform);
    QCOMPARE(factory.receivedGamutWarning, gamutWarning.data());
    QCOMPARE(factory.receivedDisplayConversionFlags, flags);
}

void KoColorConversionTransformationAbstractFactoryContractTest::defaultConstructionHasVirtualLifetime()
{
    int destructionCount = 0;

    {
        auto factory = std::make_unique<FactoryProbe>();
        factory->destructionCount = &destructionCount;
        std::unique_ptr<KoColorConversionTransformationAbstractFactory> interface = std::move(factory);

        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoColorConversionTransformationAbstractFactoryContractTest)

#include "KoColorConversionTransformationAbstractFactoryContractTest.moc"
