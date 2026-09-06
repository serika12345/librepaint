/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorProofingConversionTransformation.h>
#include <KoFallBackColorTransformation.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_FALLBACK_TRANSFORMATION_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoFallBackColorTransformation::method)), signature>)
#define ASSERT_PROOFING_TRANSFORMATION_SIGNATURE(method, signature)                                                    \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KoColorProofingConversionTransformation::method)), signature>)

class ProofingTransformationConstructorProbe : public KoColorProofingConversionTransformation
{
protected:
    ProofingTransformationConstructorProbe(const KoColorSpace *source,
                                           const KoColorSpace *destination,
                                           const KoColorSpace *proofing,
                                           Intent intent,
                                           ConversionFlags flags)
        : KoColorProofingConversionTransformation(source, destination, proofing, intent, flags)
    {
    }
};
} // namespace

class KoColorTransformationVariantsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fallbackTransformationTypeConstructionAndLifetimeSchemaRemainStable();
    void fallbackTransformationOperationSignaturesRemainStable();
    void proofingTransformationTypeConstructionAndLifetimeSchemaRemainStable();
    void proofingTransformationSpaceSignatureRemainsStable();
};

void KoColorTransformationVariantsSchemaContractTest::
    fallbackTransformationTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Transformation = KoFallBackColorTransformation;

    static_assert(std::is_class_v<Transformation>);
    static_assert(std::is_base_of_v<KoColorTransformation, Transformation>);
    static_assert(
        std::is_constructible_v<Transformation, const KoColorSpace *, const KoColorSpace *, KoColorTransformation *>);
    static_assert(std::is_constructible_v<Transformation,
                                          KoColorConversionTransformation *,
                                          KoColorConversionTransformation *,
                                          KoColorTransformation *>);
    static_assert(std::has_virtual_destructor_v<Transformation>);
}

void KoColorTransformationVariantsSchemaContractTest::fallbackTransformationOperationSignaturesRemainStable()
{
    ASSERT_FALLBACK_TRANSFORMATION_SIGNATURE(transform,
                                             void (KoFallBackColorTransformation::*)(const quint8 *, quint8 *, qint32)
                                                 const);
    ASSERT_FALLBACK_TRANSFORMATION_SIGNATURE(parameters, QList<QString> (KoFallBackColorTransformation::*)() const);
    ASSERT_FALLBACK_TRANSFORMATION_SIGNATURE(parameterId,
                                             int (KoFallBackColorTransformation::*)(const QString &) const);
    ASSERT_FALLBACK_TRANSFORMATION_SIGNATURE(setParameter,
                                             void (KoFallBackColorTransformation::*)(int, const QVariant &));
}

void KoColorTransformationVariantsSchemaContractTest::
    proofingTransformationTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Transformation = KoColorProofingConversionTransformation;

    static_assert(std::is_class_v<Transformation>);
    static_assert(std::is_base_of_v<KoColorConversionTransformation, Transformation>);
    static_assert(std::is_abstract_v<Transformation>);
    static_assert(std::is_abstract_v<ProofingTransformationConstructorProbe>);
    static_assert(std::has_virtual_destructor_v<Transformation>);
}

void KoColorTransformationVariantsSchemaContractTest::proofingTransformationSpaceSignatureRemainsStable()
{
    ASSERT_PROOFING_TRANSFORMATION_SIGNATURE(proofingSpace,
                                             const KoColorSpace *(KoColorProofingConversionTransformation::*)() const);
}

QTEST_GUILESS_MAIN(KoColorTransformationVariantsSchemaContractTest)

#include "KoColorTransformationVariantsSchemaContractTest.moc"
