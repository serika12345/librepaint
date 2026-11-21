/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionRenderBenchmark.h"

#include <algorithm>
#include <cstdio>

#include <QBitArray>
#include <QElapsedTimer>
#include <QFileInfo>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>

#include <KisFakeRunnableStrokeJobsExecutor.h>
#include <KisGlobalResourcesInterface.h>
#include <KisRunnableStrokeJobData.h>
#include <brushengine/kis_paint_information.h>
#include <brushengine/kis_paintop.h>
#include <brushengine/kis_paintop_factory.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_settings.h>
#include <brushengine/kis_stroke_random_source.h>
#include <kis_distance_information.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_pointer_utils.h>
#include <kis_undo_stores.h>
#include <strokes/KisStrokePredictionRenderer.h>

#include "KisBrushOpSettings.h"
#include "kis_brushop.h"
#include "testbrush.h"
#include "testutil.h"

namespace
{
constexpr int imageSize = 1000;
constexpr int iterations = 200;
constexpr int dabRandomSeed = 17;

class PixelBrushFactory final : public KisPaintOpFactory
{
public:
#ifdef HAVE_THREADED_TEXT_RENDERING_WORKAROUND
    void preinitializePaintOpIfNeeded(const KisPaintOpSettingsSP settings) override
    {
        KisBrushOp::preinitializeOpStatically(settings);
    }
#endif

    KisPaintOp *
    createOp(const KisPaintOpSettingsSP settings, KisPainter *painter, KisNodeSP node, KisImageSP image) override
    {
        return new KisBrushOp(settings, painter, node, image);
    }

    QString id() const override
    {
        return QStringLiteral("paintbrush");
    }

    QString name() const override
    {
        return QStringLiteral("Pixel");
    }

    QString category() const override
    {
        return KisPaintOpFactory::categoryStable();
    }

    bool lodSizeThresholdSupported() const override
    {
        return true;
    }

    QList<KoResourceLoadResult> prepareLinkedResources(const KisPaintOpSettingsSP settings,
                                                       KisResourcesInterfaceSP resourcesInterface) override
    {
        return KisBrushOp::prepareLinkedResources(settings, resourcesInterface);
    }

    QList<KoResourceLoadResult> prepareEmbeddedResources(const KisPaintOpSettingsSP settings,
                                                         KisResourcesInterfaceSP resourcesInterface) override
    {
        return KisBrushOp::prepareEmbeddedResources(settings, resourcesInterface);
    }

    KisPaintOpSettingsSP createSettings(KisResourcesInterfaceSP resourcesInterface) override
    {
        return new KisBrushOpSettings(resourcesInterface);
    }

    KisPaintOpConfigWidget *createConfigWidget(QWidget *parent,
                                               KisResourcesInterfaceSP resourcesInterface,
                                               KoCanvasResourcesInterfaceSP canvasResourcesInterface) override
    {
        Q_UNUSED(parent);
        Q_UNUSED(resourcesInterface);
        Q_UNUSED(canvasResourcesInterface);
        return nullptr;
    }
};

KisPaintInformation benchmarkPaintInformation(const QPointF &position)
{
    KisPaintInformation info(position, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    info.setCanvasRotation(0.0);
    info.setCanvasMirroredH(false);
    info.setCanvasMirroredV(false);
    info.setTiltDirectionOffset(0.0);
    return info;
}

struct TimeStatistics {
    qreal median = 0.0;
    qreal p95 = 0.0;
    qreal maximum = 0.0;
};

TimeStatistics summarizeTimes(QVector<qreal> times)
{
    TimeStatistics statistics;
    if (times.isEmpty()) return statistics;

    std::sort(times.begin(), times.end());
    statistics.median = times[times.size() / 2];
    const int p95Index = qMin(times.size() - 1, int(times.size() * 0.95));
    statistics.p95 = times[p95Index];
    statistics.maximum = times.last();
    return statistics;
}

void flushPaintOp(KisPainter *painter)
{
    KisFakeRunnableStrokeJobsExecutor executor(KisFakeRunnableStrokeJobsExecutor::AllowBarrierJobs);

    bool needsMoreUpdates = true;
    int guard = 0;

    while (needsMoreUpdates && guard++ < 1000) {
        QVector<KisRunnableStrokeJobData *> jobs;
        needsMoreUpdates = painter->paintOp()->doAsynchronousUpdate(jobs).second;

        if (!jobs.isEmpty()) {
            executor.addRunnableJobs(implicitCastList<KisRunnableStrokeJobDataBase *>(jobs));
        }
    }
}

struct BenchmarkPreset {
    QString fileName;
    int size;
    bool usePresetSize;
};

} // namespace

void KisStrokePredictionRenderBenchmark::initTestCase()
{
    KisPaintOpRegistry *registry = KisPaintOpRegistry::instance();
    if (!registry->get(QStringLiteral("paintbrush"))) {
        registry->add(new PixelBrushFactory());
    }
}

void KisStrokePredictionRenderBenchmark::benchmarkPredictionRender()
{
    KisSurrogateUndoStore *undoStore = new KisSurrogateUndoStore();
    KisImageSP image(new KisImage(undoStore,
                                  imageSize,
                                  imageSize,
                                  KoColorSpaceRegistry::instance()->rgb8(),
                                  QStringLiteral("prediction render benchmark")));
    KisPaintLayerSP layer(new KisPaintLayer(image, QStringLiteral("paint"), OPACITY_OPAQUE_U8));
    image->setWorkingThreadsLimit(1);
    image->addNode(layer, image->rootLayer());
    image->initialRefreshGraph();
    image->waitForDone();

    QVector<BenchmarkPreset> presets{
        {QStringLiteral("Basic_tip_default.kpp"), 10, false},
        {QStringLiteral("Basic_tip_default.kpp"), 40, false},
        {QStringLiteral("Basic_tip_default.kpp"), 150, false},
        {QStringLiteral("prediction_pencil_textured.kpp"), 0, true},
        {QStringLiteral("prediction_pencil_textured.kpp"), 40, false}};

    for (const BenchmarkPreset &entry : presets) {
        const QString path = TestUtil::fetchDataFileLazy(entry.fileName);
        KisPaintOpPresetSP preset(new KisPaintOpPreset(path));
        if (!preset->load(KisGlobalResourcesInterface::instance())) {
            std::printf("prediction_render preset=%s size=%d mode=unavailable skipped=load\n",
                        qPrintable(QFileInfo(entry.fileName).baseName()),
                        entry.size);
            continue;
        }

        const int size = entry.usePresetSize ? qRound(preset->settings()->paintOpSize()) : entry.size;
        preset->settings()->setPaintOpSize(size);

        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(image, layer);
        resources->setBrush(preset);
        resources->setFGColorOverride(KoColor(Qt::black, image->colorSpace()));
        resources->setBGColorOverride(KoColor(Qt::white, image->colorSpace()));
        resources->setOpacity(1.0);
        resources->setMirroring(false, false);

        const QString presetName = QFileInfo(entry.fileName).baseName();

        for (int modeIndex = 0; modeIndex < 2; modeIndex++) {
            const bool indirectPainting = modeIndex == 1;
            const char *modeName = indirectPainting ? "indirect" : "direct";
            const QString compositeOp = resources->indirectPaintingCompositeOp();

            KisPaintDeviceSP sourceDevice = new KisPaintDevice(image->colorSpace());
            sourceDevice->setDefaultBounds(layer->paintDevice()->defaultBounds());

            KisPainter sourcePainter;
            sourcePainter.begin(sourceDevice, nullptr);
            resources->setupPainter(&sourcePainter);
            if (indirectPainting) {
                sourcePainter.setCompositeOpId(compositeOp);
                sourcePainter.setOpacityToUnit();
                sourcePainter.setChannelFlags(QBitArray());
            }

            KisDistanceInformation sourceDistance;
            KisStrokeRandomSource sourceRandom(dabRandomSeed);
            KisRandomSourceSP sourceRnd = sourceRandom.source();
            KisPerStrokeRandomSourceSP sourceStrokeRnd = sourceRandom.perStrokeSource();
            KisPaintInformation sourceStart = benchmarkPaintInformation(QPointF(100.0, 100.0));
            sourceStart.setRandomSource(sourceRnd);
            sourceStart.setPerStrokeRandomSource(sourceStrokeRnd);
            KisPaintInformation sourceEnd = benchmarkPaintInformation(QPointF(150.0, 100.0));
            sourceEnd.setRandomSource(sourceRnd);
            sourceEnd.setPerStrokeRandomSource(sourceStrokeRnd);
            sourcePainter.paintLine(sourceStart, sourceEnd, &sourceDistance);

            KisStrokePredictionRenderer renderer(resources,
                                                 sourceDevice,
                                                 nullptr,
                                                 indirectPainting,
                                                 compositeOp);

            KisDistanceInformation baseDistance;
            KisStrokeRandomSource random(dabRandomSeed);
            KisRandomSourceSP randomSource = random.source();
            KisPerStrokeRandomSourceSP randomPerStroke = random.perStrokeSource();

            QVector<qreal> renderTimes;
            QVector<int> dabCounts;
            QRect lastRect;

            for (int i = 0; i < iterations; i++) {
                const QPointF start(100.0 + i, 100.0);
                KisPaintInformation lastReal = benchmarkPaintInformation(start);
                lastReal.setRandomSource(randomSource);
                lastReal.setPerStrokeRandomSource(randomPerStroke);

                const QVector<QPointF> points{
                    start + QPointF(size, 0.0),
                    start + QPointF(size * 2.0, 1.0),
                    start + QPointF(size * 3.0, 0.0)};
                const QVector<qreal> dtMs{4.0, 4.0, 4.0};

                QElapsedTimer timer;
                timer.start();
                const KisStrokePredictionRenderer::Result result =
                    renderer.render(lastReal, baseDistance, random, points, dtMs);
                renderTimes.append(timer.nsecsElapsed() / 1000.0);

                dabCounts.append(result.dabCount);
                lastRect = result.rect;
            }

            std::sort(dabCounts.begin(), dabCounts.end());
            const int medianDabs = dabCounts[dabCounts.size() / 2];
            const TimeStatistics statistics = summarizeTimes(renderTimes);

            std::printf("prediction_render preset=%s size=%d mode=%s median_us=%.1f p95_us=%.1f max_us=%.1f dabs=%d rect=%dx%d\n",
                        qPrintable(presetName),
                        size,
                        modeName,
                        statistics.median,
                        statistics.p95,
                        statistics.maximum,
                        medianDabs,
                        lastRect.width(),
                        lastRect.height());

            KisPaintDeviceSP realDevice = new KisPaintDevice(image->colorSpace());
            realDevice->setDefaultBounds(layer->paintDevice()->defaultBounds());

            KisPainter realPainter;
            realPainter.begin(realDevice, nullptr);
            resources->setupPainter(&realPainter);
            if (indirectPainting) {
                realPainter.setCompositeOpId(compositeOp);
                realPainter.setOpacityToUnit();
                realPainter.setChannelFlags(QBitArray());
            }

            QVector<qreal> realTimes;

            KisStrokeRandomSource realRandom(dabRandomSeed);
            KisRandomSourceSP realRnd = realRandom.source();
            KisPerStrokeRandomSourceSP realStrokeRnd = realRandom.perStrokeSource();

            for (int i = 0; i < iterations; i++) {
                const QPointF start(100.0 + i, 100.0);
                KisPaintInformation pi0 = benchmarkPaintInformation(start);
                KisPaintInformation pi1 = benchmarkPaintInformation(start + QPointF(size, 0.0));
                KisPaintInformation pi2 = benchmarkPaintInformation(start + QPointF(size * 2.0, 1.0));
                KisPaintInformation pi3 = benchmarkPaintInformation(start + QPointF(size * 3.0, 0.0));

                pi0.setRandomSource(realRnd);
                pi0.setPerStrokeRandomSource(realStrokeRnd);
                pi1.setRandomSource(realRnd);
                pi1.setPerStrokeRandomSource(realStrokeRnd);
                pi2.setRandomSource(realRnd);
                pi2.setPerStrokeRandomSource(realStrokeRnd);
                pi3.setRandomSource(realRnd);
                pi3.setPerStrokeRandomSource(realStrokeRnd);

                KisDistanceInformation distance;

                QElapsedTimer timer;
                timer.start();
                realPainter.paintLine(pi0, pi1, &distance);
                realPainter.paintLine(pi1, pi2, &distance);
                realPainter.paintLine(pi2, pi3, &distance);
                flushPaintOp(&realPainter);
                realTimes.append(timer.nsecsElapsed() / 1000.0);
            }

            const TimeStatistics realStatistics = summarizeTimes(realTimes);

            std::printf("real_paint preset=%s size=%d mode=%s median_us=%.1f p95_us=%.1f max_us=%.1f dabs=%d rect=%dx%d\n",
                        qPrintable(presetName),
                        size,
                        modeName,
                        realStatistics.median,
                        realStatistics.p95,
                        realStatistics.maximum,
                        medianDabs,
                        lastRect.width(),
                        lastRect.height());
        }
    }
}

KISTEST_MAIN(KisStrokePredictionRenderBenchmark)
