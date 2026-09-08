/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_convolution_painter.h"
#include "kis_gaussian_kernel.h"

#include <QTest>

#include <type_traits>

#define ASSERT_GAUSSIAN_KERNEL_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGaussianKernel::method)), signature>)
#define ASSERT_CONVOLUTION_PAINTER_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisConvolutionPainter::method)), signature>)

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
    void convolutionBorderOperationEnumSchemaRemainsStable();
    void convolutionPainterTypeAndEngineEnumSchemaRemainStable();
    void convolutionPainterConstructionSignaturesRemainStable();
    void convolutionPainterEngineAndCapabilitySignaturesRemainStable();
    void convolutionPainterApplicationSignaturesRemainStable();
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

void KisGaussianKernelSchemaContractTest::convolutionBorderOperationEnumSchemaRemainsStable()
{
    static_assert(std::is_enum_v<KisConvolutionBorderOp>);
    static_assert(BORDER_IGNORE == 0);
    static_assert(BORDER_REPEAT == 1);
}

void KisGaussianKernelSchemaContractTest::convolutionPainterTypeAndEngineEnumSchemaRemainStable()
{
    using EnginePreference = KisConvolutionPainter::EnginePreference;

    static_assert(std::is_class_v<KisConvolutionPainter>);
    static_assert(std::is_enum_v<EnginePreference>);
    static_assert(KisConvolutionPainter::NONE == 0);
    static_assert(KisConvolutionPainter::SPATIAL == 1);
    static_assert(KisConvolutionPainter::FFTW == 2);
}

void KisGaussianKernelSchemaContractTest::convolutionPainterConstructionSignaturesRemainStable()
{
    using EnginePreference = KisConvolutionPainter::EnginePreference;

    static_assert(std::is_constructible_v<KisConvolutionPainter>);
    static_assert(std::is_constructible_v<KisConvolutionPainter, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<KisConvolutionPainter, KisPaintDeviceSP, KisSelectionSP>);
    static_assert(std::is_constructible_v<KisConvolutionPainter, KisPaintDeviceSP, EnginePreference>);
}

void KisGaussianKernelSchemaContractTest::convolutionPainterEngineAndCapabilitySignaturesRemainStable()
{
    using EnginePreference = KisConvolutionPainter::EnginePreference;

    ASSERT_CONVOLUTION_PAINTER_SIGNATURE(setEnginePreference, void (KisConvolutionPainter::*)(EnginePreference));
    ASSERT_CONVOLUTION_PAINTER_SIGNATURE(supportsFFTW, bool (*)());
}

void KisGaussianKernelSchemaContractTest::convolutionPainterApplicationSignaturesRemainStable()
{
    using ApplyMatrix = void (KisConvolutionPainter::*)(const KisConvolutionKernelSP,
                                                        const KisPaintDeviceSP,
                                                        QPoint,
                                                        QPoint,
                                                        QSize,
                                                        KisConvolutionBorderOp);
    using NeedsTransaction = bool (KisConvolutionPainter::*)(const KisConvolutionKernelSP) const;

    ASSERT_CONVOLUTION_PAINTER_SIGNATURE(applyMatrix, ApplyMatrix);
    ASSERT_CONVOLUTION_PAINTER_SIGNATURE(needsTransaction, NeedsTransaction);
}

QTEST_APPLESS_MAIN(KisGaussianKernelSchemaContractTest)

#include "KisGaussianKernelSchemaContractTest.moc"
