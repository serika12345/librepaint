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

class RecordingColorSamplerFacade : public KisStrokesFacade
{
public:
    KisStrokeId startStroke(KisStrokeStrategy *strokeStrategy) override
    {
        m_events.append(QStringLiteral("start"));
        m_stroke.reset(new KisStroke(strokeStrategy));
        return m_stroke;
    }

    void addJob(KisStrokeId id, KisStrokeJobData *data) override
    {
        Q_ASSERT(id.toStrongRef() == m_stroke);
        m_events.append(QStringLiteral("job"));
        m_stroke->addJob(data);
        QScopedPointer<KisStrokeJob> job(m_stroke->popOneJob());
        Q_ASSERT(job);
        job->run();
    }

    void endStroke(KisStrokeId id) override
    {
        Q_ASSERT(id.toStrongRef() == m_stroke);
        m_events.append(QStringLiteral("end"));
        m_stroke->endStroke();
    }

    bool cancelStroke(KisStrokeId id) override
    {
        Q_ASSERT(id.toStrongRef() == m_stroke);
        m_events.append(QStringLiteral("cancel"));
        m_stroke->cancelStroke();
        return true;
    }

    const QStringList &events() const
    {
        return m_events;
    }

private:
    KisStrokeSP m_stroke;
    QStringList m_events;
};

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
    KisPaintDeviceSP device = new KisPaintDevice(colorSpace);
    const KoColor red(Qt::red, colorSpace);
    const KoColor blue(Qt::blue, colorSpace);
    device->fill(QRect(4, 5, 1, 1), red);
    device->fill(QRect(8, 9, 1, 1), blue);

    RecordingColorSamplerFacade facade;
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

    stroke.start(&facade, 1, 100);
    QVERIFY(stroke.isActive());
    stroke.addSample(device, QPoint(4, 5), KoColor(Qt::black, colorSpace));
    stroke.addSample(device, QPoint(8, 9), KoColor(Qt::black, colorSpace));

    QCOMPARE(facade.events(), QStringList({QStringLiteral("start"),
                                           QStringLiteral("job"),
                                           QStringLiteral("job")}));
    QCOMPARE(finalColors.size(), 0);

    stroke.finish();

    QVERIFY(!stroke.isActive());
    QCOMPARE(facade.events(), QStringList({QStringLiteral("start"),
                                           QStringLiteral("job"),
                                           QStringLiteral("job"),
                                           QStringLiteral("job"),
                                           QStringLiteral("end")}));
    QCOMPARE(updatedColors.size(), 2);
    QCOMPARE(updatedColors.at(0).toQColor(), QColor(Qt::red));
    QCOMPARE(updatedColors.at(1).toQColor(), QColor(Qt::blue));
    QCOMPARE(finalColors.size(), 1);
    QCOMPARE(finalColors.constFirst().toQColor(), QColor(Qt::blue));
}

QTEST_MAIN(TestPaintingBoundary)

#include "TestPaintingBoundary.moc"
