/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_convolution_kernel.h"

#include "KisConvolutionKernelMaskAccess_p.h"

#include <QColor>
#include <QDebug>
#include <QImage>
#include <QRegularExpression>
#include <QTest>

#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace
{

using Matrix = Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic>;

class TrackedKernel final : public KisConvolutionKernel
{
public:
    explicit TrackedKernel(int *destructionCount)
        : KisConvolutionKernel(1, 1, 0.0, 1.0)
        , m_destructionCount(destructionCount)
    {
    }

    ~TrackedKernel() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

struct MaskObservations {
    const KisMaskGenerator *generator{nullptr};
    qreal width{3.2};
    qreal height{1.6};
    int widthRequests{0};
    int heightRequests{0};
    QVector<QPointF> samplePoints;
    std::array<quint8, 6> sampleValues{10, 20, 30, 40, 50, 60};
};

MaskObservations maskObservations;

} // namespace

namespace KisConvolutionKernelMaskAccess
{

qreal width(const KisMaskGenerator *generator)
{
    if (generator != maskObservations.generator) {
        qFatal("unexpected mask generator passed to width access");
    }
    ++maskObservations.widthRequests;
    return maskObservations.width;
}

qreal height(const KisMaskGenerator *generator)
{
    if (generator != maskObservations.generator) {
        qFatal("unexpected mask generator passed to height access");
    }
    ++maskObservations.heightRequests;
    return maskObservations.height;
}

quint8 valueAt(const KisMaskGenerator *generator, qreal x, qreal y)
{
    if (generator != maskObservations.generator) {
        qFatal("unexpected mask generator passed to value access");
    }
    const std::size_t sampleIndex = std::size_t(maskObservations.samplePoints.size());
    if (sampleIndex >= maskObservations.sampleValues.size()) {
        qFatal("too many mask samples requested");
    }
    maskObservations.samplePoints.append(QPointF(x, y));
    return maskObservations.sampleValues.at(sampleIndex);
}

} // namespace KisConvolutionKernelMaskAccess

class KisConvolutionKernelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionMutationAndReshapePreserveKernelValues();
    void matrixFactoryOwnsInputAndDeletesVirtually();
    void imageFactoryConvertsFixedRgbaPixels();
    void maskFactorySamplesCoordinatesAndAccumulatesFactor();
    void debugStreamReportsDimensionsValuesFactorAndOffset();
};

void KisConvolutionKernelContractTest::constructionMutationAndReshapePreserveKernelValues()
{
    KisConvolutionKernel kernel(3, 2, -2.5, 7.25);

    QCOMPARE(kernel.width(), 3U);
    QCOMPARE(kernel.height(), 2U);
    QCOMPARE(kernel.offset(), -2.5);
    QCOMPARE(kernel.factor(), 7.25);

    Matrix &matrix = kernel.data();
    matrix << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    const qreal *const originalStorage = matrix.data();
    const std::array<qreal, 6> originalValues{originalStorage[0],
                                              originalStorage[1],
                                              originalStorage[2],
                                              originalStorage[3],
                                              originalStorage[4],
                                              originalStorage[5]};

    kernel.setSize(2, 3);
    kernel.setFactor(11.5);

    QCOMPARE(kernel.width(), 2U);
    QCOMPARE(kernel.height(), 3U);
    QCOMPARE(kernel.data().data(), originalStorage);
    for (Eigen::Index i = 0; i < kernel.data().size(); ++i) {
        QCOMPARE(kernel.data().data()[i], originalValues.at(std::size_t(i)));
    }
    QCOMPARE(kernel.offset(), -2.5);
    QCOMPARE(kernel.factor(), 11.5);

    const KisConvolutionKernel &constKernel = kernel;
    QCOMPARE(constKernel.data(), static_cast<const Matrix *>(&kernel.data()));
}

void KisConvolutionKernelContractTest::matrixFactoryOwnsInputAndDeletesVirtually()
{
    Matrix input(2, 2);
    input << 1.25, -2.5, 3.75, 4.5;
    const Matrix expected = input;

    KisConvolutionKernelSP kernel = KisConvolutionKernel::fromMatrix(input, -3.5, 9.25);
    input.setZero();

    QCOMPARE(kernel->width(), 2U);
    QCOMPARE(kernel->height(), 2U);
    QCOMPARE(kernel->offset(), -3.5);
    QCOMPARE(kernel->factor(), 9.25);
    QVERIFY(kernel->data().isApprox(expected, 0.0));

    static_assert(std::has_virtual_destructor_v<KisConvolutionKernel>);
    int destructionCount = 0;
    KisConvolutionKernelSP first(new TrackedKernel(&destructionCount));
    KisConvolutionKernelSP second = first;
    first.clear();
    QCOMPARE(destructionCount, 0);
    second.clear();
    QCOMPARE(destructionCount, 1);
}

void KisConvolutionKernelContractTest::imageFactoryConvertsFixedRgbaPixels()
{
    QImage image(2, 2, QImage::Format_RGBA8888);
    image.setPixelColor(0, 0, QColor(10, 20, 30, 7));
    image.setPixelColor(1, 0, QColor(90, 60, 30, 31));
    image.setPixelColor(0, 1, QColor(0, 0, 0, 127));
    image.setPixelColor(1, 1, QColor(255, 255, 255, 3));

    KisConvolutionKernelSP kernel = KisConvolutionKernel::fromQImage(image);

    QCOMPARE(kernel->width(), 2U);
    QCOMPARE(kernel->height(), 2U);
    QCOMPARE(kernel->offset(), 0.0);
    QCOMPARE(kernel->factor(), 685.0);
    QCOMPARE(kernel->data()(0, 0), 235.0);
    QCOMPARE(kernel->data()(0, 1), 195.0);
    QCOMPARE(kernel->data()(1, 0), 255.0);
    QCOMPARE(kernel->data()(1, 1), 0.0);
}

void KisConvolutionKernelContractTest::maskFactorySamplesCoordinatesAndAccumulatesFactor()
{
    alignas(void *) std::byte generatorStorage;
    auto *generator = reinterpret_cast<KisMaskGenerator *>(&generatorStorage);
    maskObservations.generator = generator;
    maskObservations.widthRequests = 0;
    maskObservations.heightRequests = 0;
    maskObservations.samplePoints.clear();

    const qreal angle = std::acos(qreal(-1.0)) / 2.0;
    KisConvolutionKernelSP kernel = KisConvolutionKernel::fromMaskGenerator(generator, angle);

    QCOMPARE(kernel->width(), 3U);
    QCOMPARE(kernel->height(), 2U);
    QCOMPARE(kernel->factor(), 1320.0);
    QCOMPARE(maskObservations.widthRequests, 1);
    QCOMPARE(maskObservations.heightRequests, 1);
    QCOMPARE(maskObservations.samplePoints.size(), 6);

    const std::array<QPointF, 6> expectedPoints{QPointF(0.5, -1.0),
                                                QPointF(0.5, 0.0),
                                                QPointF(0.5, 1.0),
                                                QPointF(-0.5, -1.0),
                                                QPointF(-0.5, 0.0),
                                                QPointF(-0.5, 1.0)};
    for (qsizetype i = 0; i < maskObservations.samplePoints.size(); ++i) {
        QVERIFY(qAbs(maskObservations.samplePoints.at(i).x() - expectedPoints.at(std::size_t(i)).x()) < 1e-12);
        QVERIFY(qAbs(maskObservations.samplePoints.at(i).y() - expectedPoints.at(std::size_t(i)).y()) < 1e-12);
        const int row = int(i) / 3;
        const int column = int(i) % 3;
        QCOMPARE(kernel->data()(row, column), qreal(255 - maskObservations.sampleValues.at(std::size_t(i))));
    }
}

void KisConvolutionKernelContractTest::debugStreamReportsDimensionsValuesFactorAndOffset()
{
    KisConvolutionKernel kernel(2, 1, -1.5, 7.0);
    kernel.data() << 3.25, 4.5;

    QString output;
    {
        QDebug debug(&output);
        debug << kernel;
    }

    output.remove(QRegularExpression(QStringLiteral("\\s+")));
    QCOMPARE(output, QStringLiteral("[2,1]{{3.25}{4.5}7-1.5}"));
}

QTEST_GUILESS_MAIN(KisConvolutionKernelContractTest)

#include "KisConvolutionKernelContractTest.moc"
