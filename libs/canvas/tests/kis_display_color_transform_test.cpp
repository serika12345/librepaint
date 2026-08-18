/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_display_color_transform_test.h"

#include <QColor>

#include <KoColor.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpaceRegistry.h>

#include <color/KisOcioConfiguration.h>
#include <color/kis_display_color_filter.h>
#include <color/kis_display_color_transform.h>
#include <simpletest.h>

namespace {
class CountingDisplayColorFilter final : public KisDisplayColorFilter
{
public:
    void filter(quint8 *, quint32 numPixels) override
    {
        filteredPixelCount += numPixels;
    }

    void approximateInverseTransformation(quint8 *, quint32) override {}
    void approximateForwardTransformation(quint8 *, quint32) override {}
    bool useInternalColorManagement() const override { return false; }

    quint32 filteredPixelCount {0};
};

const KoColorSpace *standardColorSpace()
{
    return KoColorSpaceRegistry::instance()->rgb8();
}

void configureTransform(KisDisplayColorTransform &transform)
{
    const KoColorSpace *colorSpace = standardColorSpace();
    const KoColorProfile *profile = colorSpace->profile();
    transform.setDisplayConfiguration(
        profile,
        profile,
        KoColorConversionTransformation::internalRenderingIntent(),
        KoColorConversionTransformation::internalConversionFlags());
    transform.setInputColorSpace(colorSpace);
    transform.setPaintingColorSpace(colorSpace);
}
}

void KisDisplayColorTransformTest::testOcioConfigurationIsAValueInput()
{
    KisOcioConfiguration configuration;
    QCOMPARE(configuration.mode, KisOcioConfiguration::INTERNAL);
    QVERIFY(configuration.configurationPath.isEmpty());
    QVERIFY(configuration.lutPath.isEmpty());

    configuration.mode = KisOcioConfiguration::OCIO_CONFIG;
    configuration.configurationPath = QStringLiteral("display.ocio");
    configuration.inputColorSpace = QStringLiteral("Linear Rec.2020");
    configuration.displayView = QStringLiteral("HDR");

    QCOMPARE(configuration.mode, KisOcioConfiguration::OCIO_CONFIG);
    QCOMPARE(configuration.configurationPath, QStringLiteral("display.ocio"));
    QCOMPARE(configuration.inputColorSpace, QStringLiteral("Linear Rec.2020"));
    QCOMPARE(configuration.displayView, QStringLiteral("HDR"));
}

void KisDisplayColorTransformTest::testStandardDisplayConversionWithoutUi()
{
    KisDisplayColorTransform transform;
    configureTransform(transform);
    const KoColorSpace *colorSpace = standardColorSpace();
    const QColor expected(12, 34, 56, 78);
    const KoColor source(expected, colorSpace);

    QCOMPARE(transform.toQColor(source), expected);
    QCOMPARE(transform.approximateFromRenderedQColor(expected).toQColor(), expected);
    QVERIFY(transform.canSkipDisplayConversion(colorSpace));
}

void KisDisplayColorTransformTest::testDisplayFilterParticipatesInConversion()
{
    KisDisplayColorTransform transform;
    configureTransform(transform);
    auto filter = QSharedPointer<CountingDisplayColorFilter>::create();
    transform.setDisplayFilter(filter);

    const KoColorSpace *colorSpace = standardColorSpace();
    const KoColor source(QColor(40, 80, 120, 255), colorSpace);
    const KoColor floatingPointResult =
        transform.applyDisplayFiltering(source, Float32BitsColorDepthID);
    const KoColor integerResult =
        transform.applyDisplayFiltering(source, Integer8BitsColorDepthID);

    QCOMPARE(filter->filteredPixelCount, quint32(2));
    QCOMPARE(floatingPointResult.colorSpace()->colorDepthId(),
             Float32BitsColorDepthID);
    QCOMPARE(integerResult.colorSpace()->colorDepthId(),
             Integer8BitsColorDepthID);
    QVERIFY(!transform.canSkipDisplayConversion(colorSpace));
}

SIMPLE_TEST_MAIN(KisDisplayColorTransformTest)
