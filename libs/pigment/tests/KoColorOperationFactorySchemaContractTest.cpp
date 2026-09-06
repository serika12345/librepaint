/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoAlphaMaskApplicatorFactory.h>
#include <KoAlphaMaskApplicatorFactoryImpl.h>
#include <KoColorTransformationFactoryRegistry.h>
#include <KoOptimizedPixelDataScalerU8ToU16FactoryImpl.h>

#include <QTest>

#include <type_traits>

namespace
{
struct DummyArchitecture;
using AlphaMaskFactoryImpl = KoAlphaMaskApplicatorFactoryImpl<quint8, 4, 3>;
} // namespace

class KoColorOperationFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void alphaMaskApplicatorFactorySchemaRemainsStable();
    void optimizedPixelScalerFactorySchemaRemainsStable();
    void colorTransformationFactoryRegistrySchemaRemainsStable();
};

void KoColorOperationFactorySchemaContractTest::alphaMaskApplicatorFactorySchemaRemainsStable()
{
    using FactoryCreateSignature = KoAlphaMaskApplicatorBase *(*)(KoID, int, int);
    using ImplementationCreateSignature = KoAlphaMaskApplicatorBase *(*)();

    static_assert(std::is_class_v<KoAlphaMaskApplicatorFactory>);
    static_assert(std::is_same_v<decltype(&KoAlphaMaskApplicatorFactory::create), FactoryCreateSignature>);
    static_assert(std::is_class_v<AlphaMaskFactoryImpl>);
    static_assert(
        std::is_same_v<decltype(&AlphaMaskFactoryImpl::create<DummyArchitecture>), ImplementationCreateSignature>);
}

void KoColorOperationFactorySchemaContractTest::optimizedPixelScalerFactorySchemaRemainsStable()
{
    using CreateSignature = KoOptimizedPixelDataScalerU8ToU16Base *(*)(int);

    static_assert(std::is_class_v<KoOptimizedPixelDataScalerU8ToU16FactoryImpl>);
    static_assert(std::is_same_v<decltype(&KoOptimizedPixelDataScalerU8ToU16FactoryImpl::create<DummyArchitecture>),
                                 CreateSignature>);
}

void KoColorOperationFactorySchemaContractTest::colorTransformationFactoryRegistrySchemaRemainsStable()
{
    using FactoryRegistrationSignature = void (*)(KoColorTransformationFactory *);

    static_assert(std::is_class_v<KoColorTransformationFactoryRegistry>);
    static_assert(std::has_virtual_destructor_v<KoColorTransformationFactoryRegistry>);
    static_assert(std::is_same_v<decltype(&KoColorTransformationFactoryRegistry::addColorTransformationFactory),
                                 FactoryRegistrationSignature>);
    static_assert(std::is_same_v<decltype(&KoColorTransformationFactoryRegistry::removeColorTransformationFactory),
                                 FactoryRegistrationSignature>);
}

QTEST_GUILESS_MAIN(KoColorOperationFactorySchemaContractTest)

#include "KoColorOperationFactorySchemaContractTest.moc"
