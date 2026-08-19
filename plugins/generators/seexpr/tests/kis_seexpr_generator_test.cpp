/*
 * This file is part of Krita
 *
 * SPDX-FileCopyrightText: 2020 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisGlobalResourcesInterface.h>
#include <KisImageResolutionProxy.h>
#include <QSet>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoProgressUpdater.h>
#include <KoUpdater.h>
#include <generator/kis_generator_registry.h>
#include <kis_default_bounds.h>
#include <kis_fill_painter.h>
#include <kis_filter_configuration.h>
#include <kis_processing_information.h>
#include <kis_selection.h>
#include <resources/KisSeExprScript.h>
#include <simpletest.h>
#include <testimage.h>
#include <testutil.h>


#include "kis_seexpr_generator_test.h"

#define BASE_SCRIPT                                                                                                                                                                                                                            \
    "$val=voronoi(5*[$u,$v,.5],4,.6,.2); \n \
$color=ccurve($val,\n\
    0.000, [0.141, 0.059, 0.051], 4,\n\
    0.185, [0.302, 0.176, 0.122], 4,\n\
    0.301, [0.651, 0.447, 0.165], 4,\n\
    0.462, [0.976, 0.976, 0.976], 4);\n\
$color\n\
"

namespace
{
QImage generateImage(const QString &script)
{
    KisGeneratorSP generator = KisGeneratorRegistry::instance()->get("seexpr");
    if (!generator) {
        return QImage();
    }

    KisFilterConfigurationSP config =
        generator->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (!config) {
        return QImage();
    }

    config->setProperty("script", script);

    const QPoint point(0, 0);
    const QSize testSize(256, 256);
    KisDefaultBoundsBaseSP bounds(new KisWrapAroundBoundsWrapper(
        new KisDefaultBounds(), QRect(point, testSize)));
    KisPaintDeviceSP device = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    device->setDefaultBounds(bounds);
    device->setSupportsWraparoundMode(true);

    KisFillPainter fillPainter(device);
    fillPainter.fillRect(point.x(),
                         point.y(),
                         testSize.width(),
                         testSize.height(),
                         config);

    return device->convertToQImage(nullptr, QRect(point, testSize));
}

void verifyGeneratedImageProperties(const QImage &image)
{
    QCOMPARE(image.size(), QSize(256, 256));

    QSet<QRgb> colors;
    int minimumLuminance = 255;
    int maximumLuminance = 0;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            QCOMPARE(qAlpha(pixel), 255);
            colors.insert(pixel);
            minimumLuminance = qMin(minimumLuminance, qGray(pixel));
            maximumLuminance = qMax(maximumLuminance, qGray(pixel));
        }
    }

    QVERIFY(colors.size() >= 64);
    QVERIFY(minimumLuminance < 32);
    QVERIFY(maximumLuminance > 224);
}
}

void KisSeExprGeneratorTest::initTestCase()
{
    KisGeneratorRegistry::instance();
}

void KisSeExprGeneratorTest::testGenerationFromScript()
{
    const QImage firstResult = generateImage(BASE_SCRIPT);
    const QImage secondResult = generateImage(BASE_SCRIPT);

    QVERIFY(!firstResult.isNull());
    verifyGeneratedImageProperties(firstResult);

    QPoint mismatch;
    QVERIFY2(TestUtil::compareQImages(mismatch, firstResult, secondResult),
             qPrintable(QString("Repeated generation differs at (%1, %2)")
                            .arg(mismatch.x())
                            .arg(mismatch.y())));
}

void KisSeExprGeneratorTest::testGenerationFromKoResource()
{
    KisSeExprScript resource(TestUtil::fetchDataFileLazy("Disney_noisecolor2.kse"));
    QVERIFY(resource.load(KisGlobalResourcesInterface::instance()));
    QVERIFY(resource.valid());

    const QImage resourceResult = generateImage(resource.script());
    const QImage scriptResult = generateImage(BASE_SCRIPT);

    QVERIFY(!resourceResult.isNull());
    QPoint mismatch;
    QVERIFY2(TestUtil::compareQImages(mismatch, resourceResult, scriptResult),
             qPrintable(QString("Resource generation differs from the source script at (%1, %2)")
                            .arg(mismatch.x())
                            .arg(mismatch.y())));
}

KISTEST_MAIN(KisSeExprGeneratorTest)
