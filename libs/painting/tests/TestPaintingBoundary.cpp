/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KoCanvasResourcesIds.h>
#include <KoColorSpaceRegistry.h>
#include <KoLocalStrokeCanvasResources.h>
#include <resources/KoPattern.h>
#include <resources/KoStopGradient.h>
#include <KisColorSamplerStroke.h>
#include <KisFigurePaintingOptions.h>
#include <KisStrokeSpeedMonitor.h>
#include <kis_image.h>
#include <kis_image_interfaces.h>
#include <kis_figure_painting_stroke.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_resources_snapshot.h>
#include <kis_stroke.h>
#include <kis_stroke_job.h>
#include <strokes/FreehandStrokeRunnableJobDataWithUpdate.h>

#include <type_traits>

class TestPaintingBoundary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapshotOwnsStrokeState();
    void snapshotOwnsResourceSignatures();
    void measurementStateIsSetByTheCaller();
    void figurePaintingContracts();
    void colorSamplingStrokeFinalizesAfterQueuedSamples();
};

void TestPaintingBoundary::snapshotOwnsStrokeState()
{
    KisImageSP image = new KisImage(nullptr,
                                    64,
                                    48,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("painting-boundary"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("paint"), OPACITY_OPAQUE_U8);
    image->addNode(layer);

    KisResourcesSnapshot snapshot(image, layer);

    QCOMPARE(snapshot.image().data(), image.data());
    QCOMPARE(snapshot.currentNode().data(), layer.data());
    QCOMPARE(snapshot.selectedNodes(), KisNodeList());
    QCOMPARE(snapshot.opacity(), OPACITY_OPAQUE_F);

    snapshot.setOpacity(0.25);
    QCOMPARE(snapshot.opacity(), 0.25);
}

void TestPaintingBoundary::snapshotOwnsResourceSignatures()
{
    // Consumer: Painters that change their pattern or gradient after beginning a stroke.
    // Operation: A stroke snapshots canvas resources, then the selected resources are replaced.
    // Observable result: The active snapshot keeps its original signatures and the next snapshot uses the replacements.
    // Failure impact: A running stroke can switch resources unexpectedly or a later stroke can ignore the new selection.
    KisImageSP image = new KisImage(nullptr,
                                    64,
                                    48,
                                    KoColorSpaceRegistry::instance()->rgb8(),
                                    QStringLiteral("painting-boundary-resources"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("paint"), OPACITY_OPAQUE_U8);
    image->addNode(layer);

    KisResourcesSnapshot emptySnapshot(image, layer);
    QCOMPARE(emptySnapshot.currentPatternSignature(), KoResourceSignature());
    QCOMPARE(emptySnapshot.currentGradientSignature(), KoResourceSignature());
    QCOMPARE(emptySnapshot.currentPaintOpPresetSignature(), KoResourceSignature());

    KoPatternSP pattern(new KoPattern(QImage(2, 2, QImage::Format_ARGB32),
                                      QStringLiteral("pattern"),
                                      QStringLiteral("pattern.pat")));
    pattern->setMD5Sum(QStringLiteral("pattern-md5"));

    KoStopGradientSP gradient(new KoStopGradient(QStringLiteral("gradient.svg")));
    gradient->setName(QStringLiteral("gradient"));
    gradient->setMD5Sum(QStringLiteral("gradient-md5"));

    KoLocalStrokeCanvasResourcesSP resources(new KoLocalStrokeCanvasResources());
    resources->storeResource(KoCanvasResource::CurrentPattern, QVariant::fromValue(pattern));
    resources->storeResource(KoCanvasResource::CurrentGradient, QVariant::fromValue(KoAbstractGradientSP(gradient)));

    KisResourcesSnapshot populatedSnapshot(image, layer, resources);
    QCOMPARE(populatedSnapshot.currentPatternSignature(), pattern->signature());
    QCOMPARE(populatedSnapshot.currentGradientSignature(), gradient->signature());

    KoPatternSP replacementPattern(new KoPattern(QImage(2, 2, QImage::Format_ARGB32),
                                                 QStringLiteral("replacement-pattern"),
                                                 QStringLiteral("replacement-pattern.pat")));
    replacementPattern->setMD5Sum(QStringLiteral("replacement-pattern-md5"));

    KoStopGradientSP replacementGradient(new KoStopGradient(QStringLiteral("replacement-gradient.svg")));
    replacementGradient->setName(QStringLiteral("replacement-gradient"));
    replacementGradient->setMD5Sum(QStringLiteral("replacement-gradient-md5"));

    resources->storeResource(KoCanvasResource::CurrentPattern, QVariant::fromValue(replacementPattern));
    resources->storeResource(KoCanvasResource::CurrentGradient,
                             QVariant::fromValue(KoAbstractGradientSP(replacementGradient)));

    QCOMPARE(populatedSnapshot.currentPatternSignature(), pattern->signature());
    QCOMPARE(populatedSnapshot.currentGradientSignature(), gradient->signature());

    KisResourcesSnapshot replacementSnapshot(image, layer, resources);
    QCOMPARE(replacementSnapshot.currentPatternSignature(), replacementPattern->signature());
    QCOMPARE(replacementSnapshot.currentGradientSignature(), replacementGradient->signature());
}

void TestPaintingBoundary::measurementStateIsSetByTheCaller()
{
    KisStrokeSpeedMonitor *monitor = KisStrokeSpeedMonitor::instance();

    monitor->setHaveStrokeSpeedMeasurement(false);
    QVERIFY(!monitor->haveStrokeSpeedMeasurement());

    monitor->setHaveStrokeSpeedMeasurement(true);
    QVERIFY(monitor->haveStrokeSpeedMeasurement());

    monitor->setHaveStrokeSpeedMeasurement(false);
}

void TestPaintingBoundary::figurePaintingContracts()
{
    using namespace KisFigurePaintingOptions;

    QCOMPARE(int(StrokeStyleNone), 0);
    QCOMPARE(int(StrokeStyleForeground), 1);
    QCOMPARE(int(StrokeStyleBackground), 2);
    QCOMPARE(int(FillStyleNone), 0);
    QCOMPARE(int(FillStyleForegroundColor), 1);
    QCOMPARE(int(FillStyleBackgroundColor), 2);
    QCOMPARE(int(FillStylePattern), 3);

    QVERIFY(!std::is_copy_constructible_v<KisFigurePaintingStroke>);
    QVERIFY(!std::is_copy_assignable_v<KisFigurePaintingStroke>);
}

void TestPaintingBoundary::colorSamplingStrokeFinalizesAfterQueuedSamples()
{
    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(nullptr, 32, 32, colorSpace, QStringLiteral("Sampling"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Colors"), 255);
    QVERIFY(image->addNode(layer));
    auto device = layer->paintDevice();
    const KoColor red(Qt::red, colorSpace);
    const KoColor blue(Qt::blue, colorSpace);
    device->fill(QRect(4, 5, 1, 1), red);
    device->fill(QRect(8, 9, 1, 1), blue);

    KisColorSamplerStroke stroke;
    QVector<KoColor> updatedColors;
    QVector<KoColor> finalColors;
    connect(&stroke, &KisColorSamplerStroke::sigColorUpdated,
            this, [&updatedColors](const KoColor &color) {
                updatedColors.append(color);
            });
    connect(&stroke, &KisColorSamplerStroke::sigFinalColorSelected,
            this, [&finalColors](const KoColor &color) {
                finalColors.append(color);
            });

    QVERIFY(stroke.start(image, 1, 100));
    QVERIFY(stroke.isActive());
    stroke.addSample(layer, QPoint(4, 5), KoColor(Qt::black, colorSpace));
    stroke.addSample(layer, QPoint(8, 9), KoColor(Qt::black, colorSpace));

    QCOMPARE(finalColors.size(), 0);

    stroke.finish();

    QVERIFY(!stroke.isActive());
    image->waitForDone();
    QTRY_COMPARE(updatedColors.size(), 2);
    QCOMPARE(updatedColors.at(0).toQColor(), QColor(Qt::red));
    QCOMPARE(updatedColors.at(1).toQColor(), QColor(Qt::blue));
    QTRY_COMPARE(finalColors.size(), 1);
    QCOMPARE(finalColors.constFirst().toQColor(), QColor(Qt::blue));

    QVERIFY(stroke.start(image, 1, 100));
    stroke.addSample(layer, QPoint(4, 5), KoColor(Qt::black, colorSpace));
    stroke.finish();
    stroke.cancel();
    image->waitForDone();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(updatedColors.size(), 2);
    QCOMPARE(finalColors.size(), 1);
    QVERIFY(stroke.start(image, 1, 100));
    stroke.addSample(layer, QPoint(4, 5), KoColor(Qt::black, colorSpace));
    stroke.finish();
    image->waitForDone();
    QTRY_COMPARE(finalColors.size(), 2);
    QCOMPARE(finalColors.last().toQColor(), QColor(Qt::red));

    for (const bool transparentSample : {false, true}) {
        KisImageSP empty = new KisImage(nullptr, 4, 4, colorSpace, QStringLiteral("Empty sampling"));
        KisImageWSP lifetime(empty);
        QVERIFY(stroke.start(empty, 1, 100));
        if (transparentSample) stroke.addSample({}, QPoint(), KoColor(Qt::black, colorSpace));
        stroke.finish();
        empty->waitForDone();
        empty.clear();
        QTRY_VERIFY(!lifetime.isValid());
        QCOMPARE(finalColors.size(), 2);
    }
}

QTEST_MAIN(TestPaintingBoundary)

#include "TestPaintingBoundary.moc"
