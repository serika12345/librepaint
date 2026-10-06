/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_display_color_transform_test.h"

#include <QColor>
#include <QElapsedTimer>
#include <QImage>

#include <KoColor.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpaceRegistry.h>

#include <color/kis_display_color_filter.h>
#include <color/kis_display_color_transform.h>
#include <kis_paint_device.h>
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

void KisDisplayColorTransformTest::testDisplayImagePreservesPixelsWithinRequestedPatch_data()
{
    QTest::addColumn<QString>("depth");
    QTest::addColumn<bool>("filter");
    QTest::addColumn<bool>("linearProfile");
    QTest::newRow("display-rgb8") << Integer8BitsColorDepthID.id() << false << false;
    QTest::newRow("float-conversion") << Float32BitsColorDepthID.id() << false << false;
    QTest::newRow("profile-conversion") << Integer8BitsColorDepthID.id() << false << true;
    QTest::newRow("rgb8-display-filter") << Integer8BitsColorDepthID.id() << true << false;
    QTest::newRow("display-filter") << Float32BitsColorDepthID.id() << true << false;
}

void KisDisplayColorTransformTest::testDisplayImagePreservesPixelsWithinRequestedPatch()
{
    QFETCH(QString, depth);
    QFETCH(bool, filter);
    QFETCH(bool, linearProfile);
    KisDisplayColorTransform transform;
    configureTransform(transform);
    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(
        RGBAColorModelID.id(), depth, linearProfile ? KoColorSpaceRegistry::instance()->p709G10Profile()
                                                   : standardColorSpace()->profile());
    QVERIFY(colorSpace);
    QVERIFY(!linearProfile || *colorSpace != *standardColorSpace());
    auto displayFilter = QSharedPointer<CountingDisplayColorFilter>::create();
    if (filter) transform.setDisplayFilter(displayFilter);
    KisPaintDeviceSP device = new KisPaintDevice(colorSpace);
    const KoColor background(QColor(240, 245, 250), colorSpace);
    device->fill(0, 0, 3508, 2480, background.data());
    const KoColor stroke(QColor(12, 34, 56, 78), colorSpace);
    device->fill(800, 590, 1908, 1308, stroke.data());
    const QRect patch(800, 590, 1908, 1308);
    QImage expected(patch.size(), QImage::Format_ARGB32);
    KoColor displayedStroke(stroke);
    displayedStroke.convertTo(standardColorSpace());
    expected.fill(displayedStroke.toQColor());
    QElapsedTimer timer;
    timer.start();
    const QImage actual = transform.convertImageToDisplayColorSpace(device, patch, filter);
    qInfo() << "display patch conversion ms" << timer.elapsed() << "pixels" << actual.size();
    QCOMPARE(actual, expected);
    QCOMPARE(displayFilter->filteredPixelCount, filter ? quint32(patch.width() * patch.height()) : quint32(0));
    QCOMPARE(device->exactBounds(), QRect(0, 0, 3508, 2480));
    KoColor original(colorSpace);
    device->pixel(0, 0, &original);
    QCOMPARE(original, background);
}

SIMPLE_TEST_MAIN(KisDisplayColorTransformTest)
