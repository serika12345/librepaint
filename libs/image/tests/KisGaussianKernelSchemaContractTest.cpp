/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_gaussian_kernel.h"

#include <QTest>

#include <type_traits>

#define ASSERT_GAUSSIAN_KERNEL_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGaussianKernel::method)), signature>)

namespace
{

using Matrix = Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic>;

} // namespace

class KisGaussianKernelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndRadiusConversionSchemaRemainStable();
    void horizontalAndVerticalMatrixSignaturesRemainStable();
    void convolutionKernelFactorySignaturesRemainStable();
    void logAndDilateMatrixApplicationSignaturesRemainStable();
    void gaussianDilateAndErodeApplicationSignaturesRemainStable();
};

void KisGaussianKernelSchemaContractTest::typeAndRadiusConversionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGaussianKernel>);
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(sigmaFromRadius, qreal (*)(qreal));
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(kernelSizeFromRadius, int (*)(qreal));
}

void KisGaussianKernelSchemaContractTest::horizontalAndVerticalMatrixSignaturesRemainStable()
{
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createHorizontalMatrix, Matrix (*)(qreal));
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createVerticalMatrix, Matrix (*)(qreal));
}

void KisGaussianKernelSchemaContractTest::convolutionKernelFactorySignaturesRemainStable()
{
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createHorizontalKernel, KisConvolutionKernelSP (*)(qreal));
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createVerticalKernel, KisConvolutionKernelSP (*)(qreal));
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createUniform2DKernel, KisConvolutionKernelSP (*)(qreal, qreal));
}

void KisGaussianKernelSchemaContractTest::logAndDilateMatrixApplicationSignaturesRemainStable()
{
    using ApplyLoG = void (*)(KisPaintDeviceSP, const QRect &, qreal, qreal, const QBitArray &, KoUpdater *);

    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createLoGMatrix, Matrix (*)(qreal, qreal, bool, bool));
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(applyLoG, ApplyLoG);
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(applyTightLoG, ApplyLoG);
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(createDilateMatrix, Matrix (*)(qreal));
}

void KisGaussianKernelSchemaContractTest::gaussianDilateAndErodeApplicationSignaturesRemainStable()
{
    using ApplyGaussian = void (*)(KisPaintDeviceSP,
                                   const QRect &,
                                   qreal,
                                   qreal,
                                   const QBitArray &,
                                   KoUpdater *,
                                   bool,
                                   KisConvolutionBorderOp);
    using ApplyMorphology = void (*)(KisPaintDeviceSP, const QRect &, qreal, const QBitArray &, KoUpdater *, bool);

    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(applyGaussian, ApplyGaussian);
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(applyDilate, ApplyMorphology);
    ASSERT_GAUSSIAN_KERNEL_SIGNATURE(applyErodeU8, ApplyMorphology);
}

QTEST_APPLESS_MAIN(KisGaussianKernelSchemaContractTest)

#include "KisGaussianKernelSchemaContractTest.moc"
