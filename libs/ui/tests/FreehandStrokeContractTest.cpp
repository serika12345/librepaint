/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <optional>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoResourceLoadResult.h>
#include <simpletest.h>

#include <KisGlobalResourcesInterface.h>
#include <brushengine/kis_paint_information.h>
#include <brushengine/kis_paintop_factory.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_undo_stores.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/freehand_stroke.h>

#include "KisAsynchronousStrokeUpdateHelper.h"
#include "KisBrushOpSettings.h"
#include "kis_brushop.h"
#include "testbrush.h"
#include "testutil.h"

namespace
{
constexpr int imageWidth = 500;
constexpr int imageHeight = 500;
constexpr int referenceAlphaTolerance = 3;
constexpr qreal inputPressure = 1.0;
constexpr qreal halfInputPressure = 0.5;
constexpr qreal gradientStartPressure = 0.25;
constexpr qreal inputTilt = 0.0;
constexpr qreal inputRotation = 0.0;
constexpr qreal inputTangentialPressure = 0.0;
constexpr qreal inputPerspective = 1.0;
constexpr qreal inputTime = 0.0;
constexpr qreal inputSpeed = 0.0;
const QRect maintainedStrokeBounds(50, 50, 385, 385);
const QRect maintainedHalfPressureStrokeBounds(126, 126, 234, 234);
const QByteArray maintainedHalfPressureDigest("ffdae59742d86fcfcc3764eeb7d2e82c126cd9cb08fb7c7c97a94e8b46cd5bb9");
const QRect maintainedPressureGradientBounds(154, 154, 229, 229);
const QByteArray maintainedPressureGradientDigest("e9740f2b00ef8670a37aade2c4f96cec8197dfc96eb3e18adcc20f938b5f87c0");
const QRect maintainedFuzzySeed17Bounds(142, 142, 271, 271);
const QByteArray maintainedFuzzySeed17Digest("34a090d8b904e9950f2bf7868b2c7b1f78c2d5bb3ddb8a531a90f203721c21d3");

KisPaintInformation fixedPaintInformation(const QPointF &position, qreal pressure = inputPressure)
{
    KisPaintInformation info(position,
                             pressure,
                             inputTilt,
                             inputTilt,
                             inputRotation,
                             inputTangentialPressure,
                             inputPerspective,
                             inputTime,
                             inputSpeed);
    info.setCanvasRotation(0.0);
    info.setCanvasMirroredH(false);
    info.setCanvasMirroredV(false);
    info.setTiltDirectionOffset(0.0);
    info.setLevelOfDetail(0);
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

QImage deviceImage(const KisPaintDeviceSP &device)
{
    return device->convertToQImage(0, 0, 0, imageWidth, imageHeight);
}

bool compareImages(const QImage &expected,
                   const QImage &actual,
                   const QString &resultName,
                   QPoint *mismatch = nullptr,
                   int colorTolerance = 0,
                   int alphaTolerance = 0)
{
    QPoint localMismatch;
    const bool equal = TestUtil::compareQImages(localMismatch, expected, actual, colorTolerance, alphaTolerance);
    if (!equal) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        actual.save(QStringLiteral(FILES_OUTPUT_DIR) + QLatin1Char('/') + resultName);
    }

    if (mismatch) {
        *mismatch = localMismatch;
    }
    return equal;
}

QByteArray imageDigest(const QImage &image)
{
    const QImage normalized = image.convertToFormat(QImage::Format_RGBA8888);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (int row = 0; row < normalized.height(); ++row) {
        hash.addData(QByteArray::fromRawData(reinterpret_cast<const char *>(normalized.constScanLine(row)),
                                             normalized.width() * 4));
    }
    return hash.result().toHex();
}

class FreehandStrokeFixture
{
public:
    FreehandStrokeFixture()
        : m_undoStore(new KisSurrogateUndoStore())
        , m_image(new KisImage(m_undoStore,
                               imageWidth,
                               imageHeight,
                               KoColorSpaceRegistry::instance()->rgb8(),
                               QStringLiteral("freehand stroke contract")))
        , m_layer(new KisPaintLayer(m_image, QStringLiteral("paint"), OPACITY_OPAQUE_U8))
    {
        m_image->setWorkingThreadsLimit(1);
        m_image->addNode(m_layer, m_image->rootLayer());
        m_image->initialRefreshGraph();
        m_image->waitForDone();

        m_presetPath = TestUtil::fetchDataFileLazy(QStringLiteral("autobrush_300px.kpp"));
        m_preset.reset(new KisPaintOpPreset(m_presetPath));
        m_presetLoaded = m_preset->load(KisGlobalResourcesInterface::instance());
    }

    bool presetLoaded() const
    {
        return m_presetLoaded;
    }

    QString presetPath() const
    {
        return m_presetPath;
    }

    void useFuzzyDabSizeSensor()
    {
        m_preset->settings()->setProperty(QStringLiteral("PressureSize"), true);
        m_preset->settings()->setProperty(QStringLiteral("SizeSensor"),
                                          QStringLiteral("<!DOCTYPE params><params id=\"fuzzy\"/>"));
    }

    void runStroke(bool cancel,
                   qreal startPressure = inputPressure,
                   qreal endPressure = inputPressure,
                   std::optional<int> dabRandomSeed = std::nullopt)
    {
        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(m_image, m_layer);
        resources->setBrush(m_preset);
        resources->setFGColorOverride(KoColor(Qt::black, m_image->colorSpace()));
        resources->setBGColorOverride(KoColor(Qt::white, m_image->colorSpace()));
        resources->setOpacity(1.0);
        resources->setMirroring(false, false);
        resources->setSelectionOverride(nullptr);

        auto *stroke = dabRandomSeed
            ? new FreehandStrokeStrategy(resources,
                                         new KisFreehandStrokeInfo(),
                                         kundo2_noi18n("Freehand Stroke"),
                                         FreehandStrokeStrategy::None,
                                         *dabRandomSeed)
            : new FreehandStrokeStrategy(resources, new KisFreehandStrokeInfo(), kundo2_noi18n("Freehand Stroke"));

        const KisStrokeId strokeId = m_image->startStroke(stroke);
        const KisPaintInformation start = fixedPaintInformation(QPointF(200.0, 200.0), startPressure);
        const KisPaintInformation end = fixedPaintInformation(QPointF(300.0, 300.0), endPressure);

        m_image->addJob(strokeId, new FreehandStrokeStrategy::Data(0, start, end));
        m_image->addJob(strokeId, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));

        if (cancel) {
            m_image->cancelStroke(strokeId);
        } else {
            m_image->endStroke(strokeId);
        }
        m_image->waitForDone();
    }

    void undo()
    {
        m_undoStore->undo();
        m_image->waitForDone();
    }

    void redo()
    {
        m_undoStore->redo();
        m_image->waitForDone();
    }

    KisImageSP image() const
    {
        return m_image;
    }

    KisPaintOpPresetSP preset() const
    {
        return m_preset;
    }

    QImage layerImage() const
    {
        return deviceImage(m_layer->paintDevice());
    }

    QRect layerExactBounds() const
    {
        return m_layer->paintDevice()->exactBounds();
    }

    QImage projectionImage() const
    {
        return deviceImage(m_image->projection());
    }

private:
    KisSurrogateUndoStore *m_undoStore;
    KisImageSP m_image;
    KisPaintLayerSP m_layer;
    KisPaintOpPresetSP m_preset;
    QString m_presetPath;
    bool m_presetLoaded{false};
};
} // namespace

class FreehandStrokeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void presetAndInputValuesAreFixed();
    void finishedStrokeMatchesMaintainedProjection();
    void pressureResponseProducesMaintainedPixels_data();
    void pressureResponseProducesMaintainedPixels();
    void fuzzyDabRandomSeedIsDeterministic();
    void cancelledStrokeRestoresInitialImage();
    void undoRedoRestoresBothStates();
};

void FreehandStrokeContractTest::initTestCase()
{
    KisPaintOpRegistry *registry = KisPaintOpRegistry::instance();
    QVERIFY(!registry->get(QStringLiteral("paintbrush")));
    registry->add(new PixelBrushFactory());
}

void FreehandStrokeContractTest::presetAndInputValuesAreFixed()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));

    KisPaintOpPresetSP preset = fixture.preset();
    QCOMPARE(fixture.image()->bounds(), QRect(0, 0, imageWidth, imageHeight));
    QCOMPARE(fixture.image()->colorSpace(), KoColorSpaceRegistry::instance()->rgb8());
    QCOMPARE(fixture.image()->rootLayer()->childCount(), 1);
    QCOMPARE(preset->paintOp().id(), QStringLiteral("paintbrush"));
    KisPaintOpSettingsSP settings = preset->settings();
    QVERIFY(settings);
    QCOMPARE(settings->getBool(QStringLiteral("PressureOpacity")), true);
    QCOMPARE(settings->getBool(QStringLiteral("PressureSize")), true);
    QCOMPARE(settings->getBool(QStringLiteral("PressureRotation")), false);
    QCOMPARE(settings->getBool(QStringLiteral("PressureScatter")), false);
    QCOMPARE(settings->getBool(QStringLiteral("Texture/Pattern/Enabled")), false);
    QVERIFY(settings->getString(QStringLiteral("OpacitySensor")).contains(QStringLiteral("id=\"pressure\"")));
    QVERIFY(settings->getString(QStringLiteral("SizeSensor")).contains(QStringLiteral("id=\"pressure\"")));

    QDomDocument brushDocument;
    QVERIFY(brushDocument.setContent(settings->getString(QStringLiteral("brush_definition"))));
    const QDomElement brush = brushDocument.documentElement();
    QCOMPARE(brush.tagName(), QStringLiteral("Brush"));
    QCOMPARE(brush.attribute(QStringLiteral("type")), QStringLiteral("auto_brush"));
    QCOMPARE(brush.attribute(QStringLiteral("spacing")).toDouble(), 0.1);
    QCOMPARE(brush.attribute(QStringLiteral("angle")).toDouble(), 0.0);
    QCOMPARE(brush.attribute(QStringLiteral("randomness"), QStringLiteral("0.0")).toDouble(), 0.0);
    QCOMPARE(brush.attribute(QStringLiteral("density"), QStringLiteral("1.0")).toDouble(), 1.0);

    const QDomElement mask = brush.firstChildElement(QStringLiteral("MaskGenerator"));
    QVERIFY(!mask.isNull());
    QCOMPARE(mask.attribute(QStringLiteral("type")), QStringLiteral("circle"));
    QCOMPARE(mask.attribute(QStringLiteral("radius")).toDouble(), 300.0);
    QCOMPARE(mask.attribute(QStringLiteral("ratio")).toDouble(), 1.0);
    QCOMPARE(mask.attribute(QStringLiteral("hfade")).toDouble(), 0.25);
    QCOMPARE(mask.attribute(QStringLiteral("vfade")).toDouble(), 0.25);
    QCOMPARE(mask.attribute(QStringLiteral("spikes")).toInt(), 2);

    const KisPaintInformation start = fixedPaintInformation(QPointF(200.0, 200.0));
    const KisPaintInformation end = fixedPaintInformation(QPointF(300.0, 300.0));
    QCOMPARE(start.pos(), QPointF(200.0, 200.0));
    QCOMPARE(end.pos(), QPointF(300.0, 300.0));
    QCOMPARE(start.pressure(), inputPressure);
    QCOMPARE(end.pressure(), inputPressure);
    QCOMPARE(start.xTilt(), inputTilt);
    QCOMPARE(start.yTilt(), inputTilt);
    QCOMPARE(start.rotation(), inputRotation);
    QCOMPARE(start.tangentialPressure(), inputTangentialPressure);
    QCOMPARE(start.perspective(), inputPerspective);
    QCOMPARE(start.currentTime(), inputTime);
    QCOMPARE(start.drawingSpeed(), inputSpeed);
    QCOMPARE(start.canvasRotation(), 0.0);
    QCOMPARE(start.canvasMirroredH(), false);
    QCOMPARE(start.canvasMirroredV(), false);
    QCOMPARE(start.tiltDirectionOffset(), 0.0);
}

void FreehandStrokeContractTest::finishedStrokeMatchesMaintainedProjection()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    const QImage initialLayer = fixture.layerImage();
    QSignalSpy updateSpy(fixture.image().data(), &KisImage::sigImageUpdated);

    fixture.runStroke(false);

    const QImage layer = fixture.layerImage();
    const QImage projection = fixture.projectionImage();
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());
    QVERIFY(!updateSpy.isEmpty());
    QVERIFY(layer != initialLayer);
    QCOMPARE(fixture.layerExactBounds(), maintainedStrokeBounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), maintainedStrokeBounds);

    QPoint mismatch;
    QVERIFY2(compareImages(layer, projection, QStringLiteral("freehand-contract-projection-actual.png"), &mismatch),
             qPrintable(QStringLiteral("layer and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));

    const QString referencePath =
        QStringLiteral(FILES_DATA_DIR) + QStringLiteral("/freehand-contract/autobrush-finished-projection.png");
    const QImage reference(referencePath);
    QVERIFY2(!reference.isNull(), qPrintable(QStringLiteral("missing reference image: %1").arg(referencePath)));
    QVERIFY2(
        compareImages(reference,
                      projection,
                      QStringLiteral("freehand-contract-reference-actual.png"),
                      &mismatch,
                      0,
                      referenceAlphaTolerance),
        qPrintable(QStringLiteral("reference and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
}

void FreehandStrokeContractTest::pressureResponseProducesMaintainedPixels_data()
{
    QTest::addColumn<qreal>("startPressure");
    QTest::addColumn<qreal>("endPressure");
    QTest::addColumn<QRect>("expectedBounds");
    QTest::addColumn<QByteArray>("expectedDigest");
    QTest::addColumn<QString>("resultName");

    QTest::newRow("half-pressure") << halfInputPressure << halfInputPressure << maintainedHalfPressureStrokeBounds
                                   << maintainedHalfPressureDigest << QStringLiteral("half-pressure");
    QTest::newRow("pressure-gradient") << gradientStartPressure << inputPressure << maintainedPressureGradientBounds
                                       << maintainedPressureGradientDigest << QStringLiteral("pressure-gradient");
}

void FreehandStrokeContractTest::pressureResponseProducesMaintainedPixels()
{
    QFETCH(qreal, startPressure);
    QFETCH(qreal, endPressure);
    QFETCH(QRect, expectedBounds);
    QFETCH(QByteArray, expectedDigest);
    QFETCH(QString, resultName);

    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false, startPressure, endPressure);

    const QImage layer = fixture.layerImage();
    const QImage projection = fixture.projectionImage();
    QVERIFY(layer != initialLayer);
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());

    QPoint mismatch;
    QVERIFY2(compareImages(layer,
                           projection,
                           QStringLiteral("freehand-contract-%1-projection-actual.png").arg(resultName),
                           &mismatch),
             qPrintable(QStringLiteral("%1 layer and projection differ at %2,%3")
                            .arg(resultName)
                            .arg(mismatch.x())
                            .arg(mismatch.y())));

    const QRect actualBounds = fixture.layerExactBounds();
    const QByteArray actualDigest = imageDigest(layer);
    if (expectedDigest.isEmpty()) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR)
                   + QStringLiteral("/freehand-contract-%1-actual.png").arg(resultName));
        QFAIL(qPrintable(QStringLiteral("record %1 bounds %2,%3 %4x%5 and RGBA8888 SHA-256 %6")
                             .arg(resultName)
                             .arg(actualBounds.x())
                             .arg(actualBounds.y())
                             .arg(actualBounds.width())
                             .arg(actualBounds.height())
                             .arg(QString::fromLatin1(actualDigest))));
    }

    QCOMPARE(actualBounds, expectedBounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), expectedBounds);
    QVERIFY(expectedBounds.width() < maintainedStrokeBounds.width());
    QVERIFY(expectedBounds.height() < maintainedStrokeBounds.height());

    if (actualDigest != expectedDigest) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR)
                   + QStringLiteral("/freehand-contract-%1-actual.png").arg(resultName));
    }
    QCOMPARE(actualDigest, expectedDigest);
}

void FreehandStrokeContractTest::fuzzyDabRandomSeedIsDeterministic()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    fixture.useFuzzyDabSizeSensor();
    QCOMPARE(fixture.preset()->settings()->getString(QStringLiteral("SizeSensor")),
             QStringLiteral("<!DOCTYPE params><params id=\"fuzzy\"/>"));
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false, inputPressure, inputPressure, 17);
    const QImage firstLayer = fixture.layerImage();
    const QImage firstProjection = fixture.projectionImage();
    const QRect firstBounds = fixture.layerExactBounds();
    const QByteArray firstDigest = imageDigest(firstLayer);
    QVERIFY(firstLayer != initialLayer);
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());

    QPoint mismatch;
    QVERIFY2(
        compareImages(firstLayer,
                      firstProjection,
                      QStringLiteral("freehand-contract-fuzzy-seed-17-projection-actual.png"),
                      &mismatch),
        qPrintable(
            QStringLiteral("fuzzy seed 17 layer and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QCOMPARE(firstBounds, maintainedFuzzySeed17Bounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), maintainedFuzzySeed17Bounds);
    if (firstDigest != maintainedFuzzySeed17Digest) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        firstLayer.save(QStringLiteral(FILES_OUTPUT_DIR)
                        + QStringLiteral("/freehand-contract-fuzzy-seed-17-actual.png"));
    }
    QCOMPARE(firstDigest, maintainedFuzzySeed17Digest);

    fixture.undo();
    QCOMPARE(fixture.layerImage(), initialLayer);
    fixture.runStroke(false, inputPressure, inputPressure, 17);
    QCOMPARE(imageDigest(fixture.layerImage()), maintainedFuzzySeed17Digest);
    QCOMPARE(fixture.projectionImage(), firstProjection);
    QCOMPARE(fixture.layerExactBounds(), maintainedFuzzySeed17Bounds);

    fixture.undo();
    QCOMPARE(fixture.layerImage(), initialLayer);
    fixture.runStroke(false, inputPressure, inputPressure, 18);
    const QImage secondSeedLayer = fixture.layerImage();
    const QByteArray secondSeedDigest = imageDigest(secondSeedLayer);
    QVERIFY2(
        compareImages(secondSeedLayer,
                      fixture.projectionImage(),
                      QStringLiteral("freehand-contract-fuzzy-seed-18-projection-actual.png"),
                      &mismatch),
        qPrintable(
            QStringLiteral("fuzzy seed 18 layer and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());
    QVERIFY(secondSeedDigest != firstDigest);
}

void FreehandStrokeContractTest::cancelledStrokeRestoresInitialImage()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    const QImage initialLayer = fixture.layerImage();
    const QImage initialProjection = fixture.projectionImage();

    fixture.runStroke(true);

    QPoint mismatch;
    QVERIFY2(compareImages(initialLayer,
                           fixture.layerImage(),
                           QStringLiteral("freehand-contract-cancelled-layer-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("cancelled layer differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY2(compareImages(initialProjection,
                           fixture.projectionImage(),
                           QStringLiteral("freehand-contract-cancelled-projection-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("cancelled projection differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());
}

void FreehandStrokeContractTest::undoRedoRestoresBothStates()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    const QImage initialLayer = fixture.layerImage();
    const QImage initialProjection = fixture.projectionImage();

    fixture.runStroke(false);
    const QImage finishedLayer = fixture.layerImage();
    const QImage finishedProjection = fixture.projectionImage();

    fixture.undo();

    QPoint mismatch;
    QVERIFY2(compareImages(initialLayer,
                           fixture.layerImage(),
                           QStringLiteral("freehand-contract-undo-layer-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("undo layer differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY2(compareImages(initialProjection,
                           fixture.projectionImage(),
                           QStringLiteral("freehand-contract-undo-projection-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("undo projection differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));

    fixture.redo();

    QVERIFY2(compareImages(finishedLayer,
                           fixture.layerImage(),
                           QStringLiteral("freehand-contract-redo-layer-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("redo layer differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY2(compareImages(finishedProjection,
                           fixture.projectionImage(),
                           QStringLiteral("freehand-contract-redo-projection-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("redo projection differs at %1,%2").arg(mismatch.x()).arg(mismatch.y())));
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());
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

    FreehandStrokeContractTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "FreehandStrokeContractTest.moc"
