/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoBasicHistogramProducers.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_HISTOGRAM_MEMBER(type, method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class BasicHistogramProducerConstructionProbe final : public KoBasicHistogramProducer
{
public:
    using KoBasicHistogramProducer::KoBasicHistogramProducer;

    void addRegionToBin(const quint8 *, const quint8 *, quint32, const KoColorSpace *) override;
    QString positionToString(qreal) const override;
    qreal maximalZoom() const override;
};
} // namespace

class KoBasicHistogramProducerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basicHistogramConstructionAndLifetimeSchemaRemainStable();
    void fixedDepthHistogramConstructionAndLifetimeSchemaRemainStable();
    void genericHistogramConstructionAndLifetimeSchemaRemainStable();
    void basicHistogramFactoryConstructionAndLifetimeSchemaRemainStable();
    void genericHistogramFactoryConstructionAndLifetimeSchemaRemainStable();
    void basicHistogramStateAndViewSignaturesRemainStable();
    void fixedDepthHistogramSignaturesRemainStable();
    void genericHistogramSignaturesRemainStable();
    void basicHistogramFactorySignaturesRemainStable();
    void genericHistogramFactorySignaturesRemainStable();
};

void KoBasicHistogramProducerSchemaContractTest::basicHistogramConstructionAndLifetimeSchemaRemainStable()
{
    using Producer = KoBasicHistogramProducer;
    using Probe = BasicHistogramProducerConstructionProbe;

    static_assert(!std::is_abstract_v<Probe>);
    static_assert(std::is_constructible_v<Probe, const KoID &, int, int>);
    static_assert(std::is_constructible_v<Probe, const KoID &, int, const KoColorSpace *>);
    static_assert(std::has_virtual_destructor_v<Producer>);
    static_assert(std::is_destructible_v<Producer>);

    QVERIFY(true);
}

void KoBasicHistogramProducerSchemaContractTest::fixedDepthHistogramConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_constructible_v<KoBasicU8HistogramProducer, const KoID &, const KoColorSpace *>);
    static_assert(std::is_destructible_v<KoBasicU8HistogramProducer>);
    static_assert(std::is_constructible_v<KoBasicU16HistogramProducer, const KoID &, const KoColorSpace *>);
    static_assert(std::is_destructible_v<KoBasicU16HistogramProducer>);
    static_assert(std::is_constructible_v<KoBasicF32HistogramProducer, const KoID &, const KoColorSpace *>);
    static_assert(std::is_destructible_v<KoBasicF32HistogramProducer>);
    static_assert(std::is_constructible_v<KoBasicF16HalfHistogramProducer, const KoID &, const KoColorSpace *>);
    static_assert(std::is_destructible_v<KoBasicF16HalfHistogramProducer>);

    QVERIFY(true);
}

void KoBasicHistogramProducerSchemaContractTest::genericHistogramConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_default_constructible_v<KoGenericRGBHistogramProducer>);
    static_assert(std::is_destructible_v<KoGenericRGBHistogramProducer>);
    static_assert(std::is_default_constructible_v<KoGenericLabHistogramProducer>);
    static_assert(std::is_destructible_v<KoGenericLabHistogramProducer>);

    QVERIFY(true);
}

void KoBasicHistogramProducerSchemaContractTest::basicHistogramFactoryConstructionAndLifetimeSchemaRemainStable()
{
    using Factory = KoBasicHistogramProducerFactory<KoBasicU8HistogramProducer>;

    static_assert(std::is_constructible_v<Factory, const KoID &, const QString &, const QString &>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_destructible_v<Factory>);

    QVERIFY(true);
}

void KoBasicHistogramProducerSchemaContractTest::genericHistogramFactoryConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_default_constructible_v<KoGenericRGBHistogramProducerFactory>);
    static_assert(std::is_destructible_v<KoGenericRGBHistogramProducerFactory>);
    static_assert(std::is_default_constructible_v<KoGenericLabHistogramProducerFactory>);
    static_assert(std::is_destructible_v<KoGenericLabHistogramProducerFactory>);

    QVERIFY(true);
}

void KoBasicHistogramProducerSchemaContractTest::basicHistogramStateAndViewSignaturesRemainStable()
{
    using Producer = KoBasicHistogramProducer;

    static_assert(std::is_class_v<Producer>);
    static_assert(std::is_base_of_v<KoHistogramProducer, Producer>);
    static_assert(std::is_abstract_v<Producer>);

    ASSERT_HISTOGRAM_MEMBER(Producer, channels, QList<KoChannelInfo *> (Producer::*)());
    ASSERT_HISTOGRAM_MEMBER(Producer, clear, void (Producer::*)());
    ASSERT_HISTOGRAM_MEMBER(Producer, count, qint32 (Producer::*)());
    ASSERT_HISTOGRAM_MEMBER(Producer, getBinAt, qint32 (Producer::*)(int, int));
    ASSERT_HISTOGRAM_MEMBER(Producer, id, const KoID &(Producer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(Producer, numberOfBins, qint32 (Producer::*)());
    ASSERT_HISTOGRAM_MEMBER(Producer, outOfViewLeft, qint32 (Producer::*)(int));
    ASSERT_HISTOGRAM_MEMBER(Producer, outOfViewRight, qint32 (Producer::*)(int));
    ASSERT_HISTOGRAM_MEMBER(Producer, setView, void (Producer::*)(qreal, qreal));
    ASSERT_HISTOGRAM_MEMBER(Producer, viewFrom, qreal (Producer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(Producer, viewWidth, qreal (Producer::*)() const);
}

void KoBasicHistogramProducerSchemaContractTest::fixedDepthHistogramSignaturesRemainStable()
{
    using AddRegionU8 =
        void (KoBasicU8HistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);
    using AddRegionU16 =
        void (KoBasicU16HistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);
    using AddRegionF32 =
        void (KoBasicF32HistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);
    using AddRegionF16 =
        void (KoBasicF16HalfHistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);

    static_assert(std::is_class_v<KoBasicU8HistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoBasicU8HistogramProducer>);
    static_assert(!std::is_abstract_v<KoBasicU8HistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU8HistogramProducer, addRegionToBin, AddRegionU8);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU8HistogramProducer, maximalZoom, qreal (KoBasicU8HistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU8HistogramProducer,
                            positionToString,
                            QString (KoBasicU8HistogramProducer::*)(qreal) const);

    static_assert(std::is_class_v<KoBasicU16HistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoBasicU16HistogramProducer>);
    static_assert(!std::is_abstract_v<KoBasicU16HistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU16HistogramProducer, addRegionToBin, AddRegionU16);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU16HistogramProducer, maximalZoom, qreal (KoBasicU16HistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoBasicU16HistogramProducer,
                            positionToString,
                            QString (KoBasicU16HistogramProducer::*)(qreal) const);

    static_assert(std::is_class_v<KoBasicF32HistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoBasicF32HistogramProducer>);
    static_assert(!std::is_abstract_v<KoBasicF32HistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF32HistogramProducer, addRegionToBin, AddRegionF32);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF32HistogramProducer, maximalZoom, qreal (KoBasicF32HistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF32HistogramProducer,
                            positionToString,
                            QString (KoBasicF32HistogramProducer::*)(qreal) const);

    static_assert(std::is_class_v<KoBasicF16HalfHistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoBasicF16HalfHistogramProducer>);
    static_assert(!std::is_abstract_v<KoBasicF16HalfHistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF16HalfHistogramProducer, addRegionToBin, AddRegionF16);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF16HalfHistogramProducer,
                            maximalZoom,
                            qreal (KoBasicF16HalfHistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoBasicF16HalfHistogramProducer,
                            positionToString,
                            QString (KoBasicF16HalfHistogramProducer::*)(qreal) const);
}

void KoBasicHistogramProducerSchemaContractTest::genericHistogramSignaturesRemainStable()
{
    using AddRegionRgb =
        void (KoGenericRGBHistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);
    using AddRegionLab =
        void (KoGenericLabHistogramProducer::*)(const quint8 *, const quint8 *, quint32, const KoColorSpace *);

    static_assert(std::is_class_v<KoGenericRGBHistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoGenericRGBHistogramProducer>);
    static_assert(!std::is_abstract_v<KoGenericRGBHistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoGenericRGBHistogramProducer, addRegionToBin, AddRegionRgb);
    ASSERT_HISTOGRAM_MEMBER(KoGenericRGBHistogramProducer,
                            channels,
                            QList<KoChannelInfo *> (KoGenericRGBHistogramProducer::*)());
    ASSERT_HISTOGRAM_MEMBER(KoGenericRGBHistogramProducer,
                            maximalZoom,
                            qreal (KoGenericRGBHistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoGenericRGBHistogramProducer,
                            positionToString,
                            QString (KoGenericRGBHistogramProducer::*)(qreal) const);

    static_assert(std::is_class_v<KoGenericLabHistogramProducer>);
    static_assert(std::is_base_of_v<KoBasicHistogramProducer, KoGenericLabHistogramProducer>);
    static_assert(!std::is_abstract_v<KoGenericLabHistogramProducer>);
    ASSERT_HISTOGRAM_MEMBER(KoGenericLabHistogramProducer, addRegionToBin, AddRegionLab);
    ASSERT_HISTOGRAM_MEMBER(KoGenericLabHistogramProducer,
                            channels,
                            QList<KoChannelInfo *> (KoGenericLabHistogramProducer::*)());
    ASSERT_HISTOGRAM_MEMBER(KoGenericLabHistogramProducer,
                            maximalZoom,
                            qreal (KoGenericLabHistogramProducer::*)() const);
    ASSERT_HISTOGRAM_MEMBER(KoGenericLabHistogramProducer,
                            positionToString,
                            QString (KoGenericLabHistogramProducer::*)(qreal) const);
}

void KoBasicHistogramProducerSchemaContractTest::basicHistogramFactorySignaturesRemainStable()
{
    using Factory = KoBasicHistogramProducerFactory<KoBasicU8HistogramProducer>;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KoHistogramProducerFactory, Factory>);
    static_assert(!std::is_abstract_v<Factory>);
    ASSERT_HISTOGRAM_MEMBER(Factory, generate, KoHistogramProducer * (Factory::*)());
    ASSERT_HISTOGRAM_MEMBER(Factory, isCompatibleWith, bool (Factory::*)(const KoColorSpace *, bool) const);
    ASSERT_HISTOGRAM_MEMBER(Factory, preferrednessLevelWith, float (Factory::*)(const KoColorSpace *) const);

    static_assert(std::is_same_v<decltype(std::declval<const Factory &>().isCompatibleWith(
                                     static_cast<const KoColorSpace *>(nullptr))),
                                 bool>);
}

void KoBasicHistogramProducerSchemaContractTest::genericHistogramFactorySignaturesRemainStable()
{
    using RgbFactory = KoGenericRGBHistogramProducerFactory;
    using LabFactory = KoGenericLabHistogramProducerFactory;

    static_assert(std::is_class_v<RgbFactory>);
    static_assert(std::is_base_of_v<KoHistogramProducerFactory, RgbFactory>);
    static_assert(!std::is_abstract_v<RgbFactory>);
    ASSERT_HISTOGRAM_MEMBER(RgbFactory, generate, KoHistogramProducer * (RgbFactory::*)());
    ASSERT_HISTOGRAM_MEMBER(RgbFactory, isCompatibleWith, bool (RgbFactory::*)(const KoColorSpace *, bool) const);
    ASSERT_HISTOGRAM_MEMBER(RgbFactory, preferrednessLevelWith, float (RgbFactory::*)(const KoColorSpace *) const);
    static_assert(std::is_same_v<decltype(std::declval<const RgbFactory &>().isCompatibleWith(
                                     static_cast<const KoColorSpace *>(nullptr))),
                                 bool>);

    static_assert(std::is_class_v<LabFactory>);
    static_assert(std::is_base_of_v<KoHistogramProducerFactory, LabFactory>);
    static_assert(!std::is_abstract_v<LabFactory>);
    ASSERT_HISTOGRAM_MEMBER(LabFactory, generate, KoHistogramProducer * (LabFactory::*)());
    ASSERT_HISTOGRAM_MEMBER(LabFactory, isCompatibleWith, bool (LabFactory::*)(const KoColorSpace *, bool) const);
    ASSERT_HISTOGRAM_MEMBER(LabFactory, preferrednessLevelWith, float (LabFactory::*)(const KoColorSpace *) const);
    static_assert(std::is_same_v<decltype(std::declval<const LabFactory &>().isCompatibleWith(
                                     static_cast<const KoColorSpace *>(nullptr))),
                                 bool>);
}

QTEST_APPLESS_MAIN(KoBasicHistogramProducerSchemaContractTest)

#include "KoBasicHistogramProducerSchemaContractTest.moc"
