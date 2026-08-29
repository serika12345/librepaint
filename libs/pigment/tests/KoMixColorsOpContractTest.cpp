/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoMixColorsOp.h>

#include <QTest>

#include <array>
#include <memory>

class RecordingMixer : public KoMixColorsOp::Mixer
{
public:
    explicit RecordingMixer(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingMixer() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void accumulate(const quint8 *data, const qint16 *weights, int weightSum, int nPixels) override
    {
        ++accumulateCallCount;
        accumulatedData = data;
        accumulatedWeights = weights;
        accumulatedWeightSum = weightSum;
        accumulatedPixelCount = nPixels;
        currentWeightSum += weightSum;
    }

    void accumulateAverage(const quint8 *data, int nPixels) override
    {
        ++accumulateAverageCallCount;
        averageData = data;
        averagePixelCount = nPixels;
        currentWeightSum += nPixels;
    }

    void computeMixedColor(quint8 *data) override
    {
        ++computeCallCount;
        computedData = data;
        data[0] = static_cast<quint8>(currentWeightSum);
    }

    qint64 currentWeightsSum() const override
    {
        ++currentWeightsSumCallCount;
        return currentWeightSum;
    }

    int accumulateCallCount = 0;
    const quint8 *accumulatedData = nullptr;
    const qint16 *accumulatedWeights = nullptr;
    int accumulatedWeightSum = -1;
    int accumulatedPixelCount = -1;

    int accumulateAverageCallCount = 0;
    const quint8 *averageData = nullptr;
    int averagePixelCount = -1;

    int computeCallCount = 0;
    quint8 *computedData = nullptr;

    mutable int currentWeightsSumCallCount = 0;
    qint64 currentWeightSum = 0;

private:
    int *m_destructionCount;
};

class RecordingMixColorsOp : public KoMixColorsOp
{
public:
    enum class Call {
        None,
        PointerWeighted,
        ContiguousWeighted,
        PointerAverage,
        ContiguousAverage,
        TwoArrays,
        ArrayWithColor,
    };

    explicit RecordingMixColorsOp(int *destructionCount = nullptr, int *mixerDestructionCount = nullptr)
        : m_destructionCount(destructionCount)
        , m_mixerDestructionCount(mixerDestructionCount)
    {
    }

    ~RecordingMixColorsOp() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    Mixer *createMixer() const override
    {
        ++createMixerCallCount;
        lastCreatedMixer = new RecordingMixer(m_mixerDestructionCount);
        return lastCreatedMixer;
    }

    void mixColors(const quint8 *const *colors,
                   const qint16 *weights,
                   int nColors,
                   quint8 *dst,
                   int weightSum) const override
    {
        lastCall = Call::PointerWeighted;
        pointerColors = colors;
        recordedWeights = weights;
        colorCount = nColors;
        destination = dst;
        recordedWeightSum = weightSum;
        dst[0] = 0x11;
    }

    void mixColors(const quint8 *colors, const qint16 *weights, int nColors, quint8 *dst, int weightSum) const override
    {
        lastCall = Call::ContiguousWeighted;
        contiguousColors = colors;
        recordedWeights = weights;
        colorCount = nColors;
        destination = dst;
        recordedWeightSum = weightSum;
        dst[0] = 0x22;
    }

    void mixColors(const quint8 *const *colors, int nColors, quint8 *dst) const override
    {
        lastCall = Call::PointerAverage;
        pointerColors = colors;
        colorCount = nColors;
        destination = dst;
        dst[0] = 0x33;
    }

    void mixColors(const quint8 *colors, int nColors, quint8 *dst) const override
    {
        lastCall = Call::ContiguousAverage;
        contiguousColors = colors;
        colorCount = nColors;
        destination = dst;
        dst[0] = 0x44;
    }

    void mixTwoColorArrays(const quint8 *colorsA,
                           const quint8 *colorsB,
                           int nColors,
                           qreal weight,
                           quint8 *dst) const override
    {
        lastCall = Call::TwoArrays;
        firstColorArray = colorsA;
        secondColorArray = colorsB;
        colorCount = nColors;
        recordedWeight = weight;
        destination = dst;
        dst[0] = 0x55;
    }

    void mixArrayWithColor(const quint8 *colorArray,
                           const quint8 *color,
                           int nColors,
                           qreal weight,
                           quint8 *dst) const override
    {
        lastCall = Call::ArrayWithColor;
        firstColorArray = colorArray;
        singleColor = color;
        colorCount = nColors;
        recordedWeight = weight;
        destination = dst;
        dst[0] = 0x66;
    }

    mutable int createMixerCallCount = 0;
    mutable RecordingMixer *lastCreatedMixer = nullptr;

    mutable Call lastCall = Call::None;
    mutable const quint8 *const *pointerColors = nullptr;
    mutable const quint8 *contiguousColors = nullptr;
    mutable const qint16 *recordedWeights = nullptr;
    mutable const quint8 *firstColorArray = nullptr;
    mutable const quint8 *secondColorArray = nullptr;
    mutable const quint8 *singleColor = nullptr;
    mutable int colorCount = -1;
    mutable quint8 *destination = nullptr;
    mutable int recordedWeightSum = -1;
    mutable qreal recordedWeight = -1.0;

private:
    int *m_destructionCount;
    int *m_mixerDestructionCount;
};

class KoMixColorsOpContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mixOverloadsPreserveArgumentsAndWrites();
    void arrayMixingPreservesArgumentsAndWrites();
    void mixerPreservesArgumentsStateAndWrites();
    void baseOwnershipDestroysDerivedObjectsExactlyOnce();
};

void KoMixColorsOpContractTest::mixOverloadsPreserveArgumentsAndWrites()
{
    RecordingMixColorsOp operation;
    const KoMixColorsOp &interface = operation;
    const std::array<quint8, 3> firstColor{{1, 2, 3}};
    const std::array<quint8, 3> secondColor{{4, 5, 6}};
    const std::array<const quint8 *, 2> colorPointers{{firstColor.data(), secondColor.data()}};
    const std::array<quint8, 6> contiguousColors{{1, 2, 3, 4, 5, 6}};
    const std::array<qint16, 2> weights{{113, 142}};
    std::array<quint8, 3> destination{{0, 0, 0}};

    interface.mixColors(colorPointers.data(), weights.data(), 2, destination.data(), 511);
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::PointerWeighted);
    QCOMPARE(operation.pointerColors, colorPointers.data());
    QCOMPARE(operation.recordedWeights, weights.data());
    QCOMPARE(operation.colorCount, 2);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(operation.recordedWeightSum, 511);
    QCOMPARE(destination[0], quint8(0x11));

    destination.fill(0);
    interface.mixColors(colorPointers.data(), weights.data(), 2, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::PointerWeighted);
    QCOMPARE(operation.recordedWeightSum, 255);
    QCOMPARE(destination[0], quint8(0x11));

    destination.fill(0);
    interface.mixColors(contiguousColors.data(), weights.data(), 2, destination.data(), 383);
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::ContiguousWeighted);
    QCOMPARE(operation.contiguousColors, contiguousColors.data());
    QCOMPARE(operation.recordedWeights, weights.data());
    QCOMPARE(operation.colorCount, 2);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(operation.recordedWeightSum, 383);
    QCOMPARE(destination[0], quint8(0x22));

    destination.fill(0);
    interface.mixColors(contiguousColors.data(), weights.data(), 2, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::ContiguousWeighted);
    QCOMPARE(operation.recordedWeightSum, 255);
    QCOMPARE(destination[0], quint8(0x22));

    destination.fill(0);
    interface.mixColors(colorPointers.data(), 2, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::PointerAverage);
    QCOMPARE(operation.pointerColors, colorPointers.data());
    QCOMPARE(operation.colorCount, 2);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(destination[0], quint8(0x33));

    destination.fill(0);
    interface.mixColors(contiguousColors.data(), 2, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::ContiguousAverage);
    QCOMPARE(operation.contiguousColors, contiguousColors.data());
    QCOMPARE(operation.colorCount, 2);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(destination[0], quint8(0x44));
}

void KoMixColorsOpContractTest::arrayMixingPreservesArgumentsAndWrites()
{
    RecordingMixColorsOp operation;
    const KoMixColorsOp &interface = operation;
    const std::array<quint8, 4> firstArray{{10, 20, 30, 40}};
    const std::array<quint8, 4> secondArray{{50, 60, 70, 80}};
    const std::array<quint8, 2> singleColor{{90, 100}};
    std::array<quint8, 4> destination{{0, 0, 0, 0}};

    interface.mixTwoColorArrays(firstArray.data(), secondArray.data(), 4, 0.625, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::TwoArrays);
    QCOMPARE(operation.firstColorArray, firstArray.data());
    QCOMPARE(operation.secondColorArray, secondArray.data());
    QCOMPARE(operation.colorCount, 4);
    QCOMPARE(operation.recordedWeight, 0.625);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(destination[0], quint8(0x55));

    destination.fill(0);
    interface.mixArrayWithColor(firstArray.data(), singleColor.data(), 4, 0.375, destination.data());
    QCOMPARE(operation.lastCall, RecordingMixColorsOp::Call::ArrayWithColor);
    QCOMPARE(operation.firstColorArray, firstArray.data());
    QCOMPARE(operation.singleColor, singleColor.data());
    QCOMPARE(operation.colorCount, 4);
    QCOMPARE(operation.recordedWeight, 0.375);
    QCOMPARE(operation.destination, destination.data());
    QCOMPARE(destination[0], quint8(0x66));
}

void KoMixColorsOpContractTest::mixerPreservesArgumentsStateAndWrites()
{
    RecordingMixColorsOp operation;
    const KoMixColorsOp &interface = operation;
    std::unique_ptr<KoMixColorsOp::Mixer> mixer(interface.createMixer());

    QCOMPARE(operation.createMixerCallCount, 1);
    QCOMPARE(mixer.get(), operation.lastCreatedMixer);

    const std::array<quint8, 6> weightedPixels{{1, 2, 3, 4, 5, 6}};
    const std::array<qint16, 3> weights{{17, 19, 23}};
    mixer->accumulate(weightedPixels.data(), weights.data(), 59, 3);
    QCOMPARE(operation.lastCreatedMixer->accumulateCallCount, 1);
    QCOMPARE(operation.lastCreatedMixer->accumulatedData, weightedPixels.data());
    QCOMPARE(operation.lastCreatedMixer->accumulatedWeights, weights.data());
    QCOMPARE(operation.lastCreatedMixer->accumulatedWeightSum, 59);
    QCOMPARE(operation.lastCreatedMixer->accumulatedPixelCount, 3);

    const std::array<quint8, 4> averagePixels{{7, 8, 9, 10}};
    mixer->accumulateAverage(averagePixels.data(), 4);
    QCOMPARE(operation.lastCreatedMixer->accumulateAverageCallCount, 1);
    QCOMPARE(operation.lastCreatedMixer->averageData, averagePixels.data());
    QCOMPARE(operation.lastCreatedMixer->averagePixelCount, 4);

    QCOMPARE(mixer->currentWeightsSum(), qint64(63));
    QCOMPARE(operation.lastCreatedMixer->currentWeightsSumCallCount, 1);

    std::array<quint8, 2> mixedColor{{0, 0}};
    mixer->computeMixedColor(mixedColor.data());
    QCOMPARE(operation.lastCreatedMixer->computeCallCount, 1);
    QCOMPARE(operation.lastCreatedMixer->computedData, mixedColor.data());
    QCOMPARE(mixedColor[0], quint8(63));
}

void KoMixColorsOpContractTest::baseOwnershipDestroysDerivedObjectsExactlyOnce()
{
    int operationDestructionCount = 0;
    int mixerDestructionCount = 0;

    std::unique_ptr<KoMixColorsOp> operation(
        new RecordingMixColorsOp(&operationDestructionCount, &mixerDestructionCount));
    std::unique_ptr<KoMixColorsOp::Mixer> mixer(operation->createMixer());

    mixer.reset();
    QCOMPARE(mixerDestructionCount, 1);
    QCOMPARE(operationDestructionCount, 0);

    operation.reset();
    QCOMPARE(mixerDestructionCount, 1);
    QCOMPARE(operationDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KoMixColorsOpContractTest)

#include "KoMixColorsOpContractTest.moc"
