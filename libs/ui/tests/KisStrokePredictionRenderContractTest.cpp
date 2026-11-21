/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDir>
#include <QImage>
#include <QTemporaryDir>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpIds.h>
#include <simpletest.h>

#include <KisGlobalResourcesInterface.h>
#include <brushengine/kis_paint_information.h>
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
#include <kis_selection.h>
#include <kis_undo_stores.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/KisStrokePredictionRenderer.h>
#include <strokes/freehand_stroke.h>

#include "KisAsynchronousStrokeUpdateHelper.h"
#include "KisBrushOpSettings.h"
#include "kis_brushop.h"
#include "stroke_testing_utils.h"
#include "testbrush.h"
#include "testutil.h"

namespace
{
constexpr int imageWidth = 200;
constexpr int imageHeight = 200;
constexpr qreal inputPressure = 1.0;
constexpr qreal inputTilt = 0.0;
constexpr qreal inputRotation = 0.0;
constexpr qreal inputTangentialPressure = 0.0;
constexpr qreal inputPerspective = 1.0;
constexpr qreal inputTime = 0.0;
constexpr qreal inputSpeed = 0.0;
constexpr int dabRandomSeed = 17;

KisPaintInformation
fixedPaintInformation(const QPointF &position, qreal pressure = inputPressure, qreal speed = inputSpeed)
{
    KisPaintInformation info(position,
                             pressure,
                             inputTilt,
                             inputTilt,
                             inputRotation,
                             inputTangentialPressure,
                             inputPerspective,
                             inputTime,
                             speed);
    info.setCanvasRotation(0.0);
    info.setCanvasMirroredH(false);
    info.setCanvasMirroredV(false);
    info.setTiltDirectionOffset(0.0);
    return info;
}

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

KisPaintOpPresetSP loadPreset(const QString &fileName, bool *loaded)
{
    const QString path = TestUtil::fetchDataFileLazy(fileName);
    KisPaintOpPresetSP preset(new KisPaintOpPreset(path));
    *loaded = preset->load(KisGlobalResourcesInterface::instance());
    return preset;
}

KisResourcesSnapshotSP createResources(KisImageSP image,
                                       KisPaintLayerSP layer,
                                       KisPaintOpPresetSP preset,
                                       qreal opacity = 1.0)
{
    KisResourcesSnapshotSP resources = new KisResourcesSnapshot(image, layer);
    resources->setBrush(preset);
    resources->setFGColorOverride(KoColor(Qt::black, image->colorSpace()));
    resources->setBGColorOverride(KoColor(Qt::white, image->colorSpace()));
    resources->setOpacity(opacity);
    resources->setMirroring(false, false);
    return resources;
}

void setupPredictionPainter(KisPainter *painter,
                            KisResourcesSnapshotSP resources,
                            bool indirectPainting,
                            const QString &indirectPaintingCompositeOp)
{
    resources->setupPainter(painter);

    if (indirectPainting) {
        painter->setCompositeOpId(indirectPaintingCompositeOp);
        painter->setOpacityToUnit();
        painter->setChannelFlags(QBitArray());
    }
}

QImage regionImage(const KisPaintDeviceSP &device, const QRect &rect)
{
    return device->convertToQImage(nullptr, rect.x(), rect.y(), rect.width(), rect.height());
}

bool compareRegion(const QImage &expected,
                   const QImage &actual,
                   const QString &resultName,
                   QPoint *mismatch = nullptr)
{
    QPoint localMismatch;
    const bool equal = TestUtil::compareQImages(localMismatch, expected, actual, 0, 0);
    if (!equal) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        actual.save(QStringLiteral(FILES_OUTPUT_DIR) + QLatin1Char('/') + resultName);
    }

    if (mismatch) {
        *mismatch = localMismatch;
    }
    return equal;
}

class PredictionFixture
{
public:
    PredictionFixture()
        : m_undoStore(new KisSurrogateUndoStore())
        , m_image(new KisImage(m_undoStore,
                               imageWidth,
                               imageHeight,
                               KoColorSpaceRegistry::instance()->rgb8(),
                               QStringLiteral("prediction render contract")))
        , m_layer(new KisPaintLayer(m_image, QStringLiteral("paint"), OPACITY_OPAQUE_U8))
    {
        m_image->setWorkingThreadsLimit(1);
        m_image->addNode(m_layer, m_image->rootLayer());
        m_image->initialRefreshGraph();
        m_image->waitForDone();
    }

    void setPreset(KisPaintOpPresetSP preset)
    {
        m_preset = preset;
    }

    KisResourcesSnapshotSP resources(qreal opacity = 1.0) const
    {
        return createResources(m_image, m_layer, m_preset, opacity);
    }

    void runStroke(bool cancel, bool withPrediction)
    {
        KisResourcesSnapshotSP snapshot = resources();
        auto *stroke = new FreehandStrokeStrategy(snapshot,
                                                  new KisFreehandStrokeInfo(),
                                                  kundo2_noi18n("Prediction Stroke"),
                                                  FreehandStrokeStrategy::SupportsPrediction);

        const KisStrokeId strokeId = m_image->startStroke(stroke);
        const KisPaintInformation start = fixedPaintInformation(QPointF(30.0, 100.0));
        const KisPaintInformation end = fixedPaintInformation(QPointF(120.0, 100.0));

        m_image->addJob(strokeId, new FreehandStrokeStrategy::Data(0, start, end));

        if (withPrediction) {
            for (int i = 0; i < 10; i++) {
                const QVector<QPointF> points{
                    QPointF(130.0 + i, 100.0),
                    QPointF(140.0 + i, 101.0),
                    QPointF(150.0 + i, 100.0)};
                const QVector<qreal> dtMs{2.0, 2.0, 2.0};
                m_image->addJob(strokeId, new FreehandStrokeStrategy::Data(0, points, dtMs));
            }
        }

        m_image->addJob(strokeId, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));

        if (cancel) {
            m_image->cancelStroke(strokeId);
        } else {
            m_image->endStroke(strokeId);
        }
        m_image->waitForDone();
    }

    KisImageSP image() const
    {
        return m_image;
    }

    KisPaintLayerSP layer() const
    {
        return m_layer;
    }

private:
    KisSurrogateUndoStore *m_undoStore;
    KisImageSP m_image;
    KisPaintLayerSP m_layer;
    KisPaintOpPresetSP m_preset;
};

class PredictionStrokeTester : public utils::StrokeTester
{
public:
    PredictionStrokeTester(bool withPredictions)
        : StrokeTester(QStringLiteral("prediction_history"),
                       QSize(imageWidth, imageHeight),
                       QStringLiteral("Basic_tip_default.kpp"))
        , m_withPredictions(withPredictions)
    {
    }

    QImage resultImage;
    int undoStepCount = -1;
    KisPaintLayerSP resultLayer;

    int countUndoSteps()
    {
        if (!m_image) return -1;

        KisSurrogateUndoStore *store = dynamic_cast<KisSurrogateUndoStore *>(m_image->undoStore());
        if (!store) return -1;

        int count = 0;
        while (store->presentCommand() && count < 1000) {
            store->undo();
            count++;
        }
        m_image->waitForDone();
        return count;
    }

protected:
    KisStrokeStrategy *createStroke(KisResourcesSnapshotSP resources, KisImageWSP image) override
    {
        Q_UNUSED(image);

        return new FreehandStrokeStrategy(resources,
                                         new KisFreehandStrokeInfo(),
                                         kundo2_noi18n("Prediction History Stroke"),
                                         FreehandStrokeStrategy::SupportsPrediction);
    }

    void addPaintingJobs(KisImageWSP image, KisResourcesSnapshotSP resources) override
    {
        Q_UNUSED(resources);

        const QVector<QPointF> points{
            QPointF(30.0, 100.0),
            QPointF(50.0, 100.0),
            QPointF(70.0, 100.0),
            QPointF(90.0, 100.0),
            QPointF(110.0, 100.0),
            QPointF(130.0, 100.0)};

        for (int i = 0; i < points.size() - 1; i++) {
            const KisPaintInformation pi1 = fixedPaintInformation(points[i]);
            const KisPaintInformation pi2 = fixedPaintInformation(points[i + 1]);
            image->addJob(strokeId(), new FreehandStrokeStrategy::Data(0, pi1, pi2));

            if (m_withPredictions && i < 5) {
                for (int repeat = 0; repeat < 2; repeat++) {
                    const QVector<QPointF> predicted{
                        points[i + 1] + QPointF(10.0, 0.0),
                        points[i + 1] + QPointF(20.0, 1.0),
                        points[i + 1] + QPointF(30.0, 0.0)};
                    const QVector<qreal> dtMs{2.0, 2.0, 2.0};
                    image->addJob(strokeId(), new FreehandStrokeStrategy::Data(0, predicted, dtMs));
                }
            }
        }

        image->addJob(strokeId(), new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
    }

    void beforeCheckingResult(KisImageWSP image, KisNodeSP activeNode) override
    {
        m_image = image;
        resultLayer = qobject_cast<KisPaintLayer *>(activeNode.data());
        resultImage = activeNode->paintDevice()->convertToQImage(nullptr, 0, 0, imageWidth, imageHeight);
    }

private:
    bool m_withPredictions;
    KisImageSP m_image;
};

class RecordingPredictionStrokeStrategy : public FreehandStrokeStrategy
{
public:
    RecordingPredictionStrokeStrategy(KisResourcesSnapshotSP resources,
                                      KisFreehandStrokeInfo *strokeInfo,
                                      const KUndo2MagicString &name,
                                      Flags flags,
                                      bool *rendererSeenDuringStroke)
        : FreehandStrokeStrategy(resources, strokeInfo, name, flags)
        , m_rendererSeenDuringStroke(rendererSeenDuringStroke)
    {
    }

protected:
    void paintersInitialized() override
    {
        FreehandStrokeStrategy::paintersInitialized();
        *m_rendererSeenDuringStroke = hasPredictionRenderer();
    }

private:
    bool *m_rendererSeenDuringStroke;
};

class PredictionMidStrokeTester : public utils::StrokeTester
{
public:
    PredictionMidStrokeTester()
        : StrokeTester(QStringLiteral("prediction_mid_stroke"),
                       QSize(imageWidth, imageHeight),
                       QStringLiteral("Basic_tip_default.kpp"))
    {
    }

    bool predictionRendererSeenDuringStroke = false;

protected:
    KisStrokeStrategy *createStroke(KisResourcesSnapshotSP resources, KisImageWSP image) override
    {
        Q_UNUSED(image);

        return new RecordingPredictionStrokeStrategy(
            resources,
            new KisFreehandStrokeInfo(),
            kundo2_noi18n("Prediction Mid Stroke Stroke"),
            FreehandStrokeStrategy::SupportsPrediction,
            &predictionRendererSeenDuringStroke);
    }

    void addPaintingJobs(KisImageWSP image, KisResourcesSnapshotSP resources) override
    {
        Q_UNUSED(resources);

        const KisPaintInformation start = fixedPaintInformation(QPointF(30.0, 100.0));
        const KisPaintInformation end = fixedPaintInformation(QPointF(110.0, 100.0));
        image->addJob(strokeId(), new FreehandStrokeStrategy::Data(0, start, end));

        const QVector<QPointF> predicted{
            QPointF(130.0, 100.0),
            QPointF(150.0, 100.0),
            QPointF(170.0, 100.0)};
        const QVector<qreal> dtMs{2.0, 2.0, 2.0};
        image->addJob(strokeId(), new FreehandStrokeStrategy::Data(0, predicted, dtMs));

        image->addJob(strokeId(), new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
    }
};

} // namespace

class KisStrokePredictionRenderContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void predictionMatchesLaterRealPainting_data();
    void predictionMatchesLaterRealPainting();
    void predictionJobsDoNotChangeLayerOrUndoHistory();
    void predictionRendererExistsDuringStroke();
    void predictionIsClearedAfterStrokeAndCancel();
    void predictionComposesWithLayerOpacityAndCompositeOp();
};

void KisStrokePredictionRenderContractTest::initTestCase()
{
    KisPaintOpRegistry *registry = KisPaintOpRegistry::instance();
    if (!registry->get(QStringLiteral("paintbrush"))) {
        registry->add(new PixelBrushFactory());
    }
}

void KisStrokePredictionRenderContractTest::predictionMatchesLaterRealPainting_data()
{
    QTest::addColumn<QString>("presetFile");
    QTest::addColumn<bool>("indirectPainting");
    QTest::addColumn<QString>("resultName");

    QTest::newRow("basic-direct")
        << QStringLiteral("Basic_tip_default.kpp") << false << QStringLiteral("basic-direct");
    QTest::newRow("basic-indirect")
        << QStringLiteral("Basic_tip_default.kpp") << true << QStringLiteral("basic-indirect");
    QTest::newRow("pencil-direct")
        << QStringLiteral("prediction_pencil_textured.kpp") << false << QStringLiteral("pencil-direct");
    QTest::newRow("pencil-indirect")
        << QStringLiteral("prediction_pencil_textured.kpp") << true << QStringLiteral("pencil-indirect");
}

void KisStrokePredictionRenderContractTest::predictionMatchesLaterRealPainting()
{
    QFETCH(QString, presetFile);
    QFETCH(bool, indirectPainting);
    QFETCH(QString, resultName);

    bool loaded = false;
    KisPaintOpPresetSP preset = loadPreset(presetFile, &loaded);
    if (!loaded) {
        QWARN(qPrintable(QStringLiteral("skipping %1: preset did not load").arg(presetFile)));
        return;
    }

    PredictionFixture fixture;
    fixture.setPreset(preset);

    KisImageSP image = fixture.image();
    KisPaintLayerSP layer = fixture.layer();
    KisResourcesSnapshotSP resources = fixture.resources();

    KisPaintDeviceSP actualDevice = new KisPaintDevice(image->colorSpace());
    actualDevice->setDefaultBounds(layer->paintDevice()->defaultBounds());

    const QString compositeOp = resources->indirectPaintingCompositeOp();

    KisPainter actualPainter;
    actualPainter.begin(actualDevice, nullptr);
    setupPredictionPainter(&actualPainter, resources, indirectPainting, compositeOp);

    KisStrokeRandomSource random(dabRandomSeed);
    KisRandomSourceSP rnd = random.source();
    KisPerStrokeRandomSourceSP strokeRnd = random.perStrokeSource();

    KisPaintInformation pi0 = fixedPaintInformation(QPointF(40.0, 100.0));
    KisPaintInformation pi1 = fixedPaintInformation(QPointF(70.0, 100.0));
    KisPaintInformation pi2 = fixedPaintInformation(QPointF(100.0, 104.0));
    KisPaintInformation pi3 = fixedPaintInformation(QPointF(130.0, 100.0));

    pi0.setRandomSource(rnd);
    pi0.setPerStrokeRandomSource(strokeRnd);
    pi1.setRandomSource(rnd);
    pi1.setPerStrokeRandomSource(strokeRnd);
    pi2.setRandomSource(rnd);
    pi2.setPerStrokeRandomSource(strokeRnd);
    pi3.setRandomSource(rnd);
    pi3.setPerStrokeRandomSource(strokeRnd);

    KisDistanceInformation distance;
    actualPainter.paintLine(pi0, pi1, &distance);

    KisStrokePredictionRenderer renderer(resources,
                                         actualDevice,
                                         nullptr,
                                         indirectPainting,
                                         compositeOp);

    const QVector<QPointF> predicted{pi2.pos(), pi3.pos()};
    const QVector<qreal> dtMs{3.0, 3.0};
    const KisStrokePredictionRenderer::Result result =
        renderer.render(pi1, distance, random, predicted, dtMs);

    QVERIFY(!result.rect.isEmpty());
    QVERIFY(result.device);
    QVERIFY(result.dabCount > 0);

    actualPainter.paintLine(pi1, pi2, &distance);
    actualPainter.paintLine(pi2, pi3, &distance);

    const QImage predictedImage = regionImage(result.device, result.rect);
    const QImage actualImage = regionImage(actualDevice, result.rect);

    QPoint mismatch;
    QVERIFY2(compareRegion(predictedImage,
                           actualImage,
                           QStringLiteral("prediction-render-%1-actual.png").arg(resultName),
                           &mismatch),
             qPrintable(QStringLiteral("%1 prediction and real painting differ at %2,%3")
                            .arg(resultName)
                            .arg(mismatch.x())
                            .arg(mismatch.y())));
}

void KisStrokePredictionRenderContractTest::predictionJobsDoNotChangeLayerOrUndoHistory()
{
    PredictionStrokeTester withPredictions(true);
    withPredictions.benchmark();

    PredictionStrokeTester withoutPredictions(false);
    withoutPredictions.benchmark();

    QVERIFY(!withPredictions.resultImage.isNull());
    QVERIFY(!withoutPredictions.resultImage.isNull());

    QPoint mismatch;
    QVERIFY2(compareRegion(withoutPredictions.resultImage,
                           withPredictions.resultImage,
                           QStringLiteral("prediction-history-layer-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("prediction changed the layer at %1,%2")
                            .arg(mismatch.x())
                            .arg(mismatch.y())));

    const int stepsWithPredictions = withPredictions.countUndoSteps();
    const int stepsWithoutPredictions = withoutPredictions.countUndoSteps();
    QVERIFY(stepsWithPredictions >= 0);
    QCOMPARE(stepsWithPredictions, stepsWithoutPredictions);

    QVERIFY(withPredictions.resultLayer);
    QVERIFY(!withPredictions.resultLayer->hasPrediction());
}

void KisStrokePredictionRenderContractTest::predictionRendererExistsDuringStroke()
{
    PredictionMidStrokeTester tester;
    tester.benchmark();

    QVERIFY(tester.predictionRendererSeenDuringStroke);
}

void KisStrokePredictionRenderContractTest::predictionIsClearedAfterStrokeAndCancel()
{
    bool loaded = false;
    KisPaintOpPresetSP preset = loadPreset(QStringLiteral("Basic_tip_default.kpp"), &loaded);
    QVERIFY2(loaded, "Basic_tip_default.kpp did not load");

    PredictionFixture finishedFixture;
    finishedFixture.setPreset(preset);
    finishedFixture.runStroke(false, true);
    QVERIFY(!finishedFixture.layer()->hasPrediction());

    PredictionFixture cancelledFixture;
    cancelledFixture.setPreset(preset);
    cancelledFixture.runStroke(true, true);
    QVERIFY(!cancelledFixture.layer()->hasPrediction());

    const QImage blank =
        cancelledFixture.image()->projection()->convertToQImage(nullptr, 0, 0, imageWidth, imageHeight);
    for (int y = 0; y < imageHeight; y++) {
        for (int x = 0; x < imageWidth; x++) {
            QCOMPARE(blank.pixel(x, y), qRgba(0, 0, 0, 0));
        }
    }
}

void KisStrokePredictionRenderContractTest::predictionComposesWithLayerOpacityAndCompositeOp()
{
    bool loaded = false;
    KisPaintOpPresetSP preset = loadPreset(QStringLiteral("Basic_tip_default.kpp"), &loaded);
    QVERIFY2(loaded, "Basic_tip_default.kpp did not load");

    PredictionFixture fixture;
    fixture.setPreset(preset);

    KisImageSP image = fixture.image();
    KisPaintLayerSP layer = fixture.layer();
    KisResourcesSnapshotSP resources = fixture.resources(0.5);

    layer->setOpacity(128);
    layer->setCompositeOpId(COMPOSITE_MULT);

    KisPaintDeviceSP device = layer->paintDevice();

    KisPainter actualPainter;
    actualPainter.begin(device, nullptr);
    setupPredictionPainter(&actualPainter, resources, false, resources->indirectPaintingCompositeOp());

    KisStrokeRandomSource random(dabRandomSeed);
    KisRandomSourceSP rnd = random.source();
    KisPerStrokeRandomSourceSP strokeRnd = random.perStrokeSource();

    KisPaintInformation pi0 = fixedPaintInformation(QPointF(40.0, 100.0));
    KisPaintInformation pi1 = fixedPaintInformation(QPointF(70.0, 100.0));
    KisPaintInformation pi2 = fixedPaintInformation(QPointF(100.0, 104.0));
    KisPaintInformation pi3 = fixedPaintInformation(QPointF(130.0, 100.0));

    pi0.setRandomSource(rnd);
    pi0.setPerStrokeRandomSource(strokeRnd);
    pi1.setRandomSource(rnd);
    pi1.setPerStrokeRandomSource(strokeRnd);
    pi2.setRandomSource(rnd);
    pi2.setPerStrokeRandomSource(strokeRnd);
    pi3.setRandomSource(rnd);
    pi3.setPerStrokeRandomSource(strokeRnd);

    KisDistanceInformation distance;
    actualPainter.paintLine(pi0, pi1, &distance);

    KisStrokePredictionRenderer renderer(resources, device, nullptr, false, QString());
    const QVector<QPointF> predicted{pi2.pos(), pi3.pos()};
    const QVector<qreal> dtMs{3.0, 3.0};
    const KisStrokePredictionRenderer::Result result =
        renderer.render(pi1, distance, random, predicted, dtMs);
    QVERIFY(!result.rect.isEmpty());

    layer->setPrediction(result.device,
                         result.rect,
                         KisIndirectPaintingSupport::PredictionMode::ReplaceOriginal);
    layer->setDirty(result.rect);
    image->waitForDone();

    const QImage predictedProjection =
        image->projection()->convertToQImage(nullptr, 0, 0, imageWidth, imageHeight);

    layer->clearPrediction();
    KisPainter::copyAreaOptimized(result.rect.topLeft(), result.device, device, result.rect);
    layer->setDirty(result.rect);
    image->waitForDone();

    const QImage realProjection =
        image->projection()->convertToQImage(nullptr, 0, 0, imageWidth, imageHeight);

    QPoint mismatch;
    QVERIFY2(compareRegion(realProjection,
                           predictedProjection,
                           QStringLiteral("prediction-composition-projection-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("prediction composition differs at %1,%2")
                            .arg(mismatch.x())
                            .arg(mismatch.y())));
}

int main(int argc, char *argv[])
{
    qputenv("LANGUAGE", "en");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    qputenv("QT_LOGGING_RULES", "");
    QStandardPaths::setTestModeEnabled(true);
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));

    QTemporaryDir pluginDirectory;
    if (!pluginDirectory.isValid()) {
        return 1;
    }
    qputenv("KRITA_PLUGIN_PATH", pluginDirectory.path().toUtf8());

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    registerResources();

    KisStrokePredictionRenderContractTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "KisStrokePredictionRenderContractTest.moc"
