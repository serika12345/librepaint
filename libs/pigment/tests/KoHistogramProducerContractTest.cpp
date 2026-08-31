/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoHistogramProducer.h"

#include <QTest>

namespace
{

class ProducerProbe : public KoHistogramProducer
{
public:
    ProducerProbe(const KoID &identifier, int *destructionCount = nullptr)
        : identifier(identifier)
        , destructionCount(destructionCount)
    {
    }

    ~ProducerProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    void clear() override
    {
        ++clearCalls;
    }

    void addRegionToBin(const quint8 *pixels,
                        const quint8 *selectionMask,
                        quint32 nPixels,
                        const KoColorSpace *colorSpace) override
    {
        addedPixels = pixels;
        addedSelectionMask = selectionMask;
        addedPixelCount = nPixels;
        addedColorSpace = colorSpace;
        ++addCalls;
    }

    void setView(qreal from, qreal width) override
    {
        currentViewFrom = from;
        currentViewWidth = width;
        ++setViewCalls;
    }

    const KoID &id() const override
    {
        return identifier;
    }

    QList<KoChannelInfo *> channels() override
    {
        ++channelsCalls;
        return channelValues;
    }

    qint32 numberOfBins() override
    {
        ++numberOfBinsCalls;
        return bins;
    }

    QString positionToString(qreal pos) const override
    {
        lastPosition = pos;
        ++positionToStringCalls;
        return positionText;
    }

    qreal viewFrom() const override
    {
        ++viewFromCalls;
        return currentViewFrom;
    }

    qreal viewWidth() const override
    {
        ++viewWidthCalls;
        return currentViewWidth;
    }

    qreal maximalZoom() const override
    {
        ++maximalZoomCalls;
        return maximumZoom;
    }

    qint32 count() override
    {
        ++countCalls;
        return sampleCount;
    }

    qint32 getBinAt(qint32 channel, qint32 position) override
    {
        lastBinChannel = channel;
        lastBinPosition = position;
        ++getBinCalls;
        return binValue;
    }

    qint32 outOfViewLeft(qint32 channel) override
    {
        lastLeftChannel = channel;
        ++outOfViewLeftCalls;
        return leftValue;
    }

    qint32 outOfViewRight(qint32 channel) override
    {
        lastRightChannel = channel;
        ++outOfViewRightCalls;
        return rightValue;
    }

    bool skipsTransparent() const
    {
        return m_skipTransparent;
    }

    bool skipsUnselected() const
    {
        return m_skipUnselected;
    }

    KoID identifier;
    int *destructionCount = nullptr;
    int clearCalls = 0;
    int addCalls = 0;
    const quint8 *addedPixels = nullptr;
    const quint8 *addedSelectionMask = nullptr;
    quint32 addedPixelCount = 0;
    const KoColorSpace *addedColorSpace = nullptr;
    int setViewCalls = 0;
    QList<KoChannelInfo *> channelValues;
    int channelsCalls = 0;
    qint32 bins = 257;
    int numberOfBinsCalls = 0;
    QString positionText = QStringLiteral("位置: 37.5%");
    mutable qreal lastPosition = 0.0;
    mutable int positionToStringCalls = 0;
    qreal currentViewFrom = -0.25;
    qreal currentViewWidth = 1.75;
    mutable int viewFromCalls = 0;
    mutable int viewWidthCalls = 0;
    qreal maximumZoom = 8.5;
    mutable int maximalZoomCalls = 0;
    qint32 sampleCount = 271;
    int countCalls = 0;
    qint32 lastBinChannel = -1;
    qint32 lastBinPosition = -1;
    int getBinCalls = 0;
    qint32 binValue = 821;
    qint32 lastLeftChannel = -1;
    int outOfViewLeftCalls = 0;
    qint32 leftValue = 31;
    qint32 lastRightChannel = -1;
    int outOfViewRightCalls = 0;
    qint32 rightValue = 47;
};

class FactoryProbe : public KoHistogramProducerFactory
{
public:
    FactoryProbe(const KoID &identifier,
                 KoHistogramProducer *generatedProducer,
                 bool compatible,
                 float preferredness,
                 int *destructionCount = nullptr)
        : KoHistogramProducerFactory(identifier)
        , generatedProducer(generatedProducer)
        , compatible(compatible)
        , preferredness(preferredness)
        , destructionCount(destructionCount)
    {
    }

    ~FactoryProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    KoHistogramProducer *generate() override
    {
        ++generateCalls;
        return generatedProducer;
    }

    bool isCompatibleWith(const KoColorSpace *colorSpace, bool strict = false) const override
    {
        lastCompatibilityColorSpace = colorSpace;
        lastStrict = strict;
        ++compatibilityCalls;
        return compatible;
    }

    float preferrednessLevelWith(const KoColorSpace *colorSpace) const override
    {
        lastPreferrednessColorSpace = colorSpace;
        ++preferrednessCalls;
        return preferredness;
    }

    KoHistogramProducer *generatedProducer = nullptr;
    bool compatible = false;
    float preferredness = 0.0f;
    int *destructionCount = nullptr;
    int generateCalls = 0;
    mutable const KoColorSpace *lastCompatibilityColorSpace = nullptr;
    mutable bool lastStrict = false;
    mutable int compatibilityCalls = 0;
    mutable const KoColorSpace *lastPreferrednessColorSpace = nullptr;
    mutable int preferrednessCalls = 0;
};

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class KoHistogramProducerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void producerDefaultsAndLifetimePreserveExclusionState();
    void producerVirtualDispatchPreservesArgumentsAndResults();
    void factoryValuesAndVirtualDispatchPreserveTokens();
    void registryFiltersSortsAndOwnsFactories();
};

void KoHistogramProducerContractTest::producerDefaultsAndLifetimePreserveExclusionState()
{
    int destructionCount = 0;
    KoHistogramProducer *producer =
        new ProducerProbe(KoID(QStringLiteral("既定生成器"), QStringLiteral("既定ヒストグラム")), &destructionCount);
    auto *probe = static_cast<ProducerProbe *>(producer);

    QVERIFY(probe->skipsTransparent());
    QVERIFY(probe->skipsUnselected());

    producer->setSkipTransparent(false);
    QVERIFY(!probe->skipsTransparent());
    QVERIFY(probe->skipsUnselected());

    producer->setSkipUnselected(false);
    QVERIFY(!probe->skipsTransparent());
    QVERIFY(!probe->skipsUnselected());

    producer->setSkipTransparent(true);
    producer->setSkipUnselected(true);
    QVERIFY(probe->skipsTransparent());
    QVERIFY(probe->skipsUnselected());

    delete producer;
    QCOMPARE(destructionCount, 1);
}

void KoHistogramProducerContractTest::producerVirtualDispatchPreservesArgumentsAndResults()
{
    ProducerProbe producer(KoID(QStringLiteral("生成器-色域"), QStringLiteral("生成器 色域")));
    const quint8 pixels[] = {3, 5, 8, 13};
    const quint8 selection[] = {21, 34, 55, 89};
    char colorSpaceStorage = 0;
    const auto *colorSpace = reinterpret_cast<const KoColorSpace *>(&colorSpaceStorage);
    char firstChannelStorage = 0;
    char secondChannelStorage = 0;
    auto *firstChannel = reinterpret_cast<KoChannelInfo *>(&firstChannelStorage);
    auto *secondChannel = reinterpret_cast<KoChannelInfo *>(&secondChannelStorage);
    producer.channelValues = {firstChannel, secondChannel};

    producer.clear();
    QCOMPARE(producer.clearCalls, 1);

    producer.addRegionToBin(pixels, selection, 4, colorSpace);
    QCOMPARE(producer.addCalls, 1);
    QCOMPARE(producer.addedPixels, pixels);
    QCOMPARE(producer.addedSelectionMask, selection);
    QCOMPARE(producer.addedPixelCount, quint32(4));
    QCOMPARE(producer.addedColorSpace, colorSpace);

    producer.setView(0.125, 0.625);
    QCOMPARE(producer.setViewCalls, 1);
    QCOMPARE(producer.currentViewFrom, 0.125);
    QCOMPARE(producer.currentViewWidth, 0.625);

    QCOMPARE(producer.id().id(), QStringLiteral("生成器-色域"));
    QCOMPARE(producer.id().name(), QStringLiteral("生成器 色域"));
    QCOMPARE(&producer.id(), &producer.identifier);
    QCOMPARE(producer.channels(), QList<KoChannelInfo *>({firstChannel, secondChannel}));
    QCOMPARE(producer.channelsCalls, 1);
    QCOMPARE(producer.numberOfBins(), 257);
    QCOMPARE(producer.numberOfBinsCalls, 1);
    QCOMPARE(producer.positionToString(0.375), QStringLiteral("位置: 37.5%"));
    QCOMPARE(producer.lastPosition, 0.375);
    QCOMPARE(producer.positionToStringCalls, 1);
    QCOMPARE(producer.viewFrom(), 0.125);
    QCOMPARE(producer.viewFromCalls, 1);
    QCOMPARE(producer.viewWidth(), 0.625);
    QCOMPARE(producer.viewWidthCalls, 1);
    QCOMPARE(producer.maximalZoom(), 8.5);
    QCOMPARE(producer.maximalZoomCalls, 1);
    QCOMPARE(producer.count(), 271);
    QCOMPARE(producer.countCalls, 1);
    QCOMPARE(producer.getBinAt(2, 19), 821);
    QCOMPARE(producer.lastBinChannel, 2);
    QCOMPARE(producer.lastBinPosition, 19);
    QCOMPARE(producer.getBinCalls, 1);
    QCOMPARE(producer.outOfViewLeft(3), 31);
    QCOMPARE(producer.lastLeftChannel, 3);
    QCOMPARE(producer.outOfViewLeftCalls, 1);
    QCOMPARE(producer.outOfViewRight(4), 47);
    QCOMPARE(producer.lastRightChannel, 4);
    QCOMPARE(producer.outOfViewRightCalls, 1);
}

void KoHistogramProducerContractTest::factoryValuesAndVirtualDispatchPreserveTokens()
{
    ProducerProbe producer(KoID(QStringLiteral("生成結果"), QStringLiteral("生成結果")));
    int destructionCount = 0;
    KoHistogramProducerFactory *factory =
        new FactoryProbe(KoID(QStringLiteral("工場-α"), QStringLiteral("工場 表示名")),
                         &producer,
                         true,
                         0.625f,
                         &destructionCount);
    auto *probe = static_cast<FactoryProbe *>(factory);
    char colorSpaceStorage = 0;
    const auto *colorSpace = reinterpret_cast<const KoColorSpace *>(&colorSpaceStorage);

    QCOMPARE(factory->id(), QStringLiteral("工場-α"));
    QCOMPARE(factory->name(), QStringLiteral("工場 表示名"));
    QCOMPARE(factory->generate(), &producer);
    QCOMPARE(probe->generateCalls, 1);

    QVERIFY(factory->isCompatibleWith(colorSpace));
    QCOMPARE(probe->lastCompatibilityColorSpace, colorSpace);
    QVERIFY(!probe->lastStrict);
    QVERIFY(factory->isCompatibleWith(colorSpace, true));
    QVERIFY(probe->lastStrict);
    QCOMPARE(probe->compatibilityCalls, 2);

    QCOMPARE(factory->preferrednessLevelWith(colorSpace), 0.625f);
    QCOMPARE(probe->lastPreferrednessColorSpace, colorSpace);
    QCOMPARE(probe->preferrednessCalls, 1);

    delete factory;
    QCOMPARE(destructionCount, 1);
}

void KoHistogramProducerContractTest::registryFiltersSortsAndOwnsFactories()
{
    KoHistogramProducerFactoryRegistry *globalRegistry = KoHistogramProducerFactoryRegistry::instance();
    QVERIFY(globalRegistry);
    QCOMPARE(KoHistogramProducerFactoryRegistry::instance(), globalRegistry);

    int lowDestructionCount = 0;
    int highDestructionCount = 0;
    int excludedDestructionCount = 0;
    char colorSpaceStorage = 0;
    const auto *colorSpace = reinterpret_cast<const KoColorSpace *>(&colorSpaceStorage);

    {
        KoHistogramProducerFactoryRegistry registry;
        QCOMPARE(registry.count(), 0);

        auto *low = new FactoryProbe(KoID(QStringLiteral("低"), QStringLiteral("低優先")),
                                     nullptr,
                                     true,
                                     0.25f,
                                     &lowDestructionCount);
        auto *high = new FactoryProbe(KoID(QStringLiteral("高"), QStringLiteral("高優先")),
                                      nullptr,
                                      true,
                                      0.9f,
                                      &highDestructionCount);
        auto *excluded = new FactoryProbe(KoID(QStringLiteral("除外"), QStringLiteral("非互換")),
                                          nullptr,
                                          false,
                                          0.7f,
                                          &excludedDestructionCount);
        registry.add(low);
        registry.add(high);
        registry.add(excluded);

        QCOMPARE(registry.keysCompatibleWith(colorSpace, true),
                 QList<QString>({QStringLiteral("高"), QStringLiteral("低")}));
        QCOMPARE(low->lastCompatibilityColorSpace, colorSpace);
        QCOMPARE(high->lastCompatibilityColorSpace, colorSpace);
        QCOMPARE(excluded->lastCompatibilityColorSpace, colorSpace);
        QVERIFY(low->lastStrict);
        QVERIFY(high->lastStrict);
        QVERIFY(excluded->lastStrict);
        QCOMPARE(low->compatibilityCalls, 1);
        QCOMPARE(high->compatibilityCalls, 1);
        QCOMPARE(excluded->compatibilityCalls, 1);
        QCOMPARE(low->lastPreferrednessColorSpace, colorSpace);
        QCOMPARE(high->lastPreferrednessColorSpace, colorSpace);
        QCOMPARE(low->preferrednessCalls, 1);
        QCOMPARE(high->preferrednessCalls, 1);
        QCOMPARE(excluded->preferrednessCalls, 0);

        QCOMPARE(lowDestructionCount, 0);
        QCOMPARE(highDestructionCount, 0);
        QCOMPARE(excludedDestructionCount, 0);
    }

    QCOMPARE(lowDestructionCount, 1);
    QCOMPARE(highDestructionCount, 1);
    QCOMPARE(excludedDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KoHistogramProducerContractTest)

#include "KoHistogramProducerContractTest.moc"
