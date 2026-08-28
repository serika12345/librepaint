/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QBitArray>
#include <QDomDocument>
#include <QImage>
#include <QTest>

#include <array>
#include <cmath>

#include <KoColorConversionTransformationFactory.h>
#include <colorspaces/KoAlphaColorSpace.h>

namespace
{
class ExposedAlphaColorSpaceFactory : public KoAlphaColorSpaceFactory
{
public:
    using KoAlphaColorSpaceFactory::createColorSpace;
};

class SimpleFactoryProbe : public KoSimpleColorSpaceFactory
{
public:
    using KoSimpleColorSpaceFactory::KoSimpleColorSpaceFactory;

protected:
    KoColorSpace *createColorSpace(const KoColorProfile *) const override
    {
        return nullptr;
    }
};
} // namespace

class KoAlphaColorSpaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void channelImplementationsExposeStableIdentifiers();
    void constructionProvidesAlphaMetadata();
    void qColorAndDisplayConversionsPreserveAlpha();
    void sixteenBitInterchangeUsesAlphaAsGray();
    void convolutionHonorsAlphaChannelSelection();
    void unsupportedColorOperationsReturnNeutralResults();
    void factoryCreatesMatchingColorSpaceAndConversionLinks();
    void simpleFactoryExposesConfiguredMetadata();
};

void KoAlphaColorSpaceContractTest::channelImplementationsExposeStableIdentifiers()
{
    QCOMPARE(KoAlphaColorSpace::colorSpaceId(), QStringLiteral("ALPHA"));
    QCOMPARE(KoAlphaU16ColorSpace::colorSpaceId(), QStringLiteral("ALPHAU16"));
    QCOMPARE(KoAlphaF32ColorSpace::colorSpaceId(), QStringLiteral("ALPHAF32"));
#ifdef HAVE_OPENEXR
    QCOMPARE(KoAlphaF16ColorSpace::colorSpaceId(), QStringLiteral("ALPHAF16"));
#endif
}

void KoAlphaColorSpaceContractTest::constructionProvidesAlphaMetadata()
{
    KoAlphaColorSpace colorSpace;

    QCOMPARE(colorSpace.colorSpaceId(), QStringLiteral("ALPHA"));
    QCOMPARE(colorSpace.colorModelId(), AlphaColorModelID);
    QCOMPARE(colorSpace.colorDepthId(), Integer8BitsColorDepthID);
    QCOMPARE(colorSpace.colorChannelCount(), 0U);
    QCOMPARE(colorSpace.colorSpaceType(), 0U);
    QVERIFY(!colorSpace.hasHighDynamicRange());
    QVERIFY(!colorSpace.willDegrade(FULLY_INDEPENDENT));
    QVERIFY(!colorSpace.profileIsCompatible(colorSpace.profile()));
    QVERIFY(colorSpace.profile());

    KoColorSpace *clone = colorSpace.clone();
    QVERIFY(clone);
    QCOMPARE(clone->colorModelId(), AlphaColorModelID);
    QCOMPARE(clone->colorDepthId(), Integer8BitsColorDepthID);
    delete static_cast<KoAlphaColorSpace *>(clone);
}

void KoAlphaColorSpaceContractTest::qColorAndDisplayConversionsPreserveAlpha()
{
    KoAlphaColorSpace colorSpace;
    quint8 pixel = 0;

    colorSpace.fromQColor(QColor(12, 34, 56, 64), &pixel);
    QCOMPARE(pixel, quint8(64));

    QColor color;
    colorSpace.toQColor(&pixel, &color);
    QCOMPARE(color, QColor(255, 255, 255, 64));

    const quint8 low = 32;
    const quint8 high = 96;
    QCOMPARE(colorSpace.difference(&low, &high), quint8(64));
    QCOMPARE(colorSpace.difference(&high, &low), quint8(64));
    QCOMPARE(colorSpace.differenceA(&low, &high), quint8(64));
    QCOMPARE(colorSpace.channelValueText(&pixel, 0), QStringLiteral("64"));

    bool converted = false;
    const qreal normalized = colorSpace.normalisedChannelValueText(&pixel, 0).toDouble(&converted);
    QVERIFY(converted);
    QVERIFY(std::abs(normalized - (64.0 / 255.0)) < 0.00001);

    const std::array<quint8, 4> pixels{0, 127, 255, 64};
    const QImage image = colorSpace.convertToQImage(pixels.data(),
                                                    2,
                                                    2,
                                                    nullptr,
                                                    KoColorConversionTransformation::IntentPerceptual,
                                                    KoColorConversionTransformation::Empty);
    QCOMPARE(image.format(), QImage::Format_Indexed8);
    QCOMPARE(image.size(), QSize(2, 2));
    QCOMPARE(image.pixelIndex(0, 0), 0);
    QCOMPARE(image.pixelIndex(1, 0), 127);
    QCOMPARE(image.pixelIndex(0, 1), 255);
    QCOMPARE(image.pixelIndex(1, 1), 64);
    QCOMPARE(image.color(127), qRgb(127, 127, 127));
}

void KoAlphaColorSpaceContractTest::sixteenBitInterchangeUsesAlphaAsGray()
{
    KoAlphaColorSpace colorSpace;
    const quint8 pixel = 128;
    std::array<quint16, 4> channels{};

    colorSpace.toLabA16(&pixel, reinterpret_cast<quint8 *>(channels.data()), 1);
    QCOMPARE(channels[0], quint16(128 * 257));
    QCOMPARE(channels[1], quint16(UINT16_MAX / 2));
    QCOMPARE(channels[2], quint16(UINT16_MAX / 2));
    QCOMPARE(channels[3], quint16(UINT16_MAX));

    quint8 roundTrip = 0;
    colorSpace.fromLabA16(reinterpret_cast<const quint8 *>(channels.data()), &roundTrip, 1);
    QCOMPARE(roundTrip, pixel);

    channels.fill(0);
    colorSpace.toRgbA16(&pixel, reinterpret_cast<quint8 *>(channels.data()), 1);
    QCOMPARE(channels[0], quint16(128 * 257));
    QCOMPARE(channels[1], channels[0]);
    QCOMPARE(channels[2], channels[0]);
    QCOMPARE(channels[3], quint16(UINT16_MAX));

    roundTrip = 0;
    colorSpace.fromRgbA16(reinterpret_cast<const quint8 *>(channels.data()), &roundTrip, 1);
    QCOMPARE(roundTrip, pixel);
}

void KoAlphaColorSpaceContractTest::convolutionHonorsAlphaChannelSelection()
{
    KoAlphaColorSpace colorSpace;
    quint8 first = 10;
    quint8 second = 30;
    quint8 *colors[] = {&first, &second};
    qreal weights[] = {1.0, 3.0};

    quint8 destination = 0;
    colorSpace.convolveColors(colors, weights, &destination, 4.0, 5.0, 2, QBitArray());
    QCOMPARE(destination, quint8(30));

    QBitArray selected(1, true);
    destination = 0;
    colorSpace.convolveColors(colors, weights, &destination, 4.0, 5.0, 2, selected);
    QCOMPARE(destination, quint8(30));

    QBitArray excluded(1, false);
    destination = 77;
    colorSpace.convolveColors(colors, weights, &destination, 4.0, 5.0, 2, excluded);
    QCOMPARE(destination, quint8(77));
}

void KoAlphaColorSpaceContractTest::unsupportedColorOperationsReturnNeutralResults()
{
    KoAlphaColorSpace colorSpace;

    QVERIFY(!colorSpace.createBrightnessContrastAdjustment(nullptr));
    QVERIFY(!colorSpace.createPerChannelAdjustment(nullptr));
    QVERIFY(!colorSpace.createDarkenAdjustment(5, true, 0.5));

    quint8 pixel = 91;
    colorSpace.invertColor(&pixel, 1);
    QCOMPARE(pixel, quint8(91));

    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("color"));
    document.appendChild(root);
    colorSpace.colorToXML(&pixel, document, root);
    QVERIFY(!root.hasChildNodes());

    colorSpace.colorFromXML(&pixel, root);
    QCOMPARE(pixel, quint8(91));

    qreal first = 1.0;
    qreal second = 2.0;
    qreal third = 3.0;
    colorSpace.toHSY(QVector<double>{0.25}, &first, &second, &third);
    QCOMPARE(first, 1.0);
    QCOMPARE(second, 2.0);
    QCOMPARE(third, 3.0);

    const QVector<double> hsy = colorSpace.fromHSY(&first, &second, &third);
    QCOMPARE(hsy, QVector<double>{0.0});

    colorSpace.toYUV(QVector<double>{0.75}, &first, &second, &third);
    QCOMPARE(first, 1.0);
    QCOMPARE(second, 2.0);
    QCOMPARE(third, 3.0);

    const QVector<double> yuv = colorSpace.fromYUV(&first, &second, &third);
    QCOMPARE(yuv, QVector<double>{0.0});
}

void KoAlphaColorSpaceContractTest::factoryCreatesMatchingColorSpaceAndConversionLinks()
{
    ExposedAlphaColorSpaceFactory factory;
    KoColorSpace *colorSpace = factory.createColorSpace(nullptr);
    QVERIFY(colorSpace);
    QCOMPARE(colorSpace->colorModelId(), AlphaColorModelID);
    QCOMPARE(colorSpace->colorDepthId(), Integer8BitsColorDepthID);

    const QList<KoColorConversionTransformationFactory *> links = factory.colorConversionLinks();
    QCOMPARE(links.size(), 8);

    const QStringList grayDepths{Integer8BitsColorDepthID.id(),
                                 Integer16BitsColorDepthID.id(),
                                 Float16BitsColorDepthID.id(),
                                 Float32BitsColorDepthID.id()};
    for (const QString &depth : grayDepths) {
        int fromAlphaCount = 0;
        int toAlphaCount = 0;
        for (const KoColorConversionTransformationFactory *link : links) {
            if (link->srcColorModelId() == AlphaColorModelID.id()
                && link->srcColorDepthId() == Integer8BitsColorDepthID.id()
                && link->dstColorModelId() == GrayAColorModelID.id() && link->dstColorDepthId() == depth) {
                ++fromAlphaCount;
            }
            if (link->srcColorModelId() == GrayAColorModelID.id() && link->srcColorDepthId() == depth
                && link->dstColorModelId() == AlphaColorModelID.id()
                && link->dstColorDepthId() == Integer8BitsColorDepthID.id()) {
                ++toAlphaCount;
            }
        }
        QCOMPARE(fromAlphaCount, 1);
        QCOMPARE(toAlphaCount, 1);
    }

    for (KoColorConversionTransformationFactory *link : links) {
        delete link;
    }
    delete static_cast<KoAlphaColorSpace *>(colorSpace);
}

void KoAlphaColorSpaceContractTest::simpleFactoryExposesConfiguredMetadata()
{
    SimpleFactoryProbe factory(QStringLiteral("probe-id"),
                               QStringLiteral("Probe name"),
                               true,
                               AlphaColorModelID,
                               Integer16BitsColorDepthID,
                               24,
                               37);
    KoAlphaColorSpace alphaColorSpace;

    QCOMPARE(factory.id(), QStringLiteral("probe-id"));
    QCOMPARE(factory.name(), QStringLiteral("Probe name"));
    QVERIFY(factory.userVisible());
    QCOMPARE(factory.colorModelId(), AlphaColorModelID);
    QCOMPARE(factory.colorDepthId(), Integer16BitsColorDepthID);
    QVERIFY(factory.profileIsCompatible(alphaColorSpace.profile()));
    QCOMPARE(factory.colorSpaceEngine(), QStringLiteral("simple"));
    QVERIFY(!factory.isHdr());
    QCOMPARE(factory.referenceDepth(), 24);
    QCOMPARE(factory.crossingCost(), 37);
    QVERIFY(factory.colorConversionLinks().isEmpty());
    QCOMPARE(factory.defaultProfile(), QStringLiteral("default"));
}

QTEST_GUILESS_MAIN(KoAlphaColorSpaceContractTest)

#include "KoAlphaColorSpaceContractTest.moc"
