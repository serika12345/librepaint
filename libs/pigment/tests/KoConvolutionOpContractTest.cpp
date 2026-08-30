/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoConvolutionOp.h>

#include <QBitArray>
#include <QTest>

#include <array>
#include <memory>

class RecordingConvolutionOp final : public KoConvolutionOp
{
public:
    explicit RecordingConvolutionOp(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingConvolutionOp() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void convolveColors(const quint8 *const *colors,
                        const qreal *kernelValues,
                        quint8 *dst,
                        qreal factor,
                        qreal offset,
                        qint32 nColors,
                        const QBitArray &channelFlags) const override
    {
        ++callCount;
        receivedColors = colors;
        receivedKernelValues = kernelValues;
        receivedDestination = dst;
        receivedFactor = factor;
        receivedOffset = offset;
        receivedColorCount = nColors;
        receivedChannelFlags = &channelFlags;

        dst[0] = quint8(colors[1][0] + kernelValues[2]);
    }

    mutable int callCount = 0;
    mutable const quint8 *const *receivedColors = nullptr;
    mutable const qreal *receivedKernelValues = nullptr;
    mutable quint8 *receivedDestination = nullptr;
    mutable qreal receivedFactor = 0.0;
    mutable qreal receivedOffset = 0.0;
    mutable qint32 receivedColorCount = 0;
    mutable const QBitArray *receivedChannelFlags = nullptr;

private:
    int *m_destructionCount;
};

class KoConvolutionOpContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void convolveColorsPreservesArgumentsAndResult();
    void baseOwnershipDestroysDerivedOnce();
};

void KoConvolutionOpContractTest::convolveColorsPreservesArgumentsAndResult()
{
    RecordingConvolutionOp operation;
    const KoConvolutionOp &interface = operation;
    const std::array<quint8, 2> firstColor{11, 13};
    const std::array<quint8, 2> secondColor{17, 19};
    const std::array<quint8, 2> thirdColor{23, 29};
    const std::array<const quint8 *, 3> colors{firstColor.data(), secondColor.data(), thirdColor.data()};
    const std::array<qreal, 3> kernelValues{1.0, 2.0, 3.0};
    std::array<quint8, 2> destination{0xaa, 0xbb};
    QBitArray channelFlags(2);
    channelFlags.setBit(0);

    interface.convolveColors(colors.data(), kernelValues.data(), destination.data(), 6.0, 4.5, 3, channelFlags);

    QCOMPARE(operation.callCount, 1);
    QCOMPARE(operation.receivedColors, colors.data());
    QCOMPARE(operation.receivedKernelValues, kernelValues.data());
    QCOMPARE(operation.receivedDestination, destination.data());
    QCOMPARE(operation.receivedFactor, 6.0);
    QCOMPARE(operation.receivedOffset, 4.5);
    QCOMPARE(operation.receivedColorCount, 3);
    QCOMPARE(operation.receivedChannelFlags, &channelFlags);
    QCOMPARE(destination[0], quint8(20));
    QCOMPARE(destination[1], quint8(0xbb));
}

void KoConvolutionOpContractTest::baseOwnershipDestroysDerivedOnce()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KoConvolutionOp> operation = std::make_unique<RecordingConvolutionOp>(&destructionCount);

        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoConvolutionOpContractTest)

#include "KoConvolutionOpContractTest.moc"
