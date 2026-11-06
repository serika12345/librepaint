/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QImage>
#include <QElapsedTimer>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <cmath>
#include <optional>
#include <future>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoResourceLoadResult.h>
#include <KoCanvasResourceProvider.h>
#include <KoCanvasResourcesIds.h>
#include <simpletest.h>

#include <KisGlobalResourcesInterface.h>
#include <KisFigurePaintingOptions.h>
#include <brushengine/kis_paint_information.h>
#include <brushengine/kis_paintop_factory.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_figure_painting_stroke.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_undo_stores.h>
#include <kis_quick_shape.h>
#include "tool/KisQuickShapePreview.h"
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
constexpr qreal changedBrushSpacing = 0.25;
constexpr qreal changedInputSpeed = 0.5;
constexpr qreal inputTilt = 0.0;
constexpr qreal inputRotation = 0.0;
constexpr qreal inputTangentialPressure = 0.0;
constexpr qreal inputPerspective = 1.0;
constexpr qreal inputTime = 0.0;
constexpr qreal inputSpeed = 0.0;
const QRect maintainedStrokeBounds(50, 50, 385, 385);
const QRect maintainedHalfPressureStrokeBounds(126, 126, 234, 234);
#if defined(Q_OS_ANDROID) && defined(Q_PROCESSOR_X86_64)
const QByteArray maintainedHalfPressureDigest("4a73b991ca36c42e199a8194b2cd8e166bfd6e67018d6ba6de226bdcdd302831");
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64) && !defined(Q_OS_ANDROID)
const QByteArray maintainedHalfPressureDigest("4a73b991ca36c42e199a8194b2cd8e166bfd6e67018d6ba6de226bdcdd302831");
#else
const QByteArray maintainedHalfPressureDigest("ffdae59742d86fcfcc3764eeb7d2e82c126cd9cb08fb7c7c97a94e8b46cd5bb9");
#endif
const QRect maintainedPressureGradientBounds(154, 154, 229, 229);
const QByteArray maintainedPressureGradientDigest("e9740f2b00ef8670a37aade2c4f96cec8197dfc96eb3e18adcc20f938b5f87c0");
const QRect maintainedFuzzySeed17Bounds(142, 142, 271, 271);
const QByteArray maintainedFuzzySeed17Digest("34a090d8b904e9950f2bf7868b2c7b1f78c2d5bb3ddb8a531a90f203721c21d3");
const QRect maintainedSpacing025Bounds(50, 50, 353, 353);
#if defined(Q_OS_ANDROID) && defined(Q_PROCESSOR_X86_64)
const QByteArray maintainedSpacing025Digest("2ddab997fbbb9608d884c0a6097df0c8d8496b13cf18c090997b7a3c77af684b");
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64) && !defined(Q_OS_ANDROID)
const QByteArray maintainedSpacing025Digest("24fcf5246719d87bd091c837098ffe4841280b47b507e7a3e2ea2d5cf325f2ba");
#else
const QByteArray maintainedSpacing025Digest("8bdf0e95ea7526b6289bf2393397c7bb005b69da6866891c2cb12bf991d7f210");
#endif
const QRect maintainedSpeed05Bounds(125, 125, 235, 235);
const QByteArray maintainedSpeed05Digest("3c7c2e19b4b91a27b8d1ddb1068db753012e01f98244eb9e6f688026db4f551a");
const QRect rectangularSelectionBounds(225, 225, 100, 100);
const QByteArray maintainedRectangularSelectionDigest("4f5b7f971268c853d893a2c6c25d805cb354eef8c1d6c26fffeaac5dafa4d219");

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
    FreehandStrokeFixture(int width = imageWidth, int height = imageHeight)
        : m_undoStore(new KisSurrogateUndoStore())
        , m_image(new KisImage(m_undoStore,
                               width,
                               height,
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

    KisBrushOpSettings *brushSettings() const
    {
        return dynamic_cast<KisBrushOpSettings *>(m_preset->settings().data());
    }

    void useSizeSensor(const QString &sensorId)
    {
        m_preset->settings()->setProperty(QStringLiteral("PressureSize"), true);
        m_preset->settings()->setProperty(QStringLiteral("SizeSensor"),
                                          QStringLiteral("<!DOCTYPE params><params id=\"%1\"/>").arg(sensorId));
    }

    void select(const QRect &bounds)
    {
        m_selection = new KisSelection();
        m_selection->pixelSelection()->select(bounds);
        m_selection->updateProjection();
    }

    void runStroke(bool cancel,
                   qreal startPressure = inputPressure,
                   qreal endPressure = inputPressure,
                   qreal startSpeed = inputSpeed,
                   qreal endSpeed = inputSpeed,
                   std::optional<int> dabRandomSeed = std::nullopt,
                   const QColor &color = Qt::black,
                   qreal opacity = 1.0)
    {
        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(m_image, m_layer);
        resources->setBrush(m_preset);
        resources->setFGColorOverride(KoColor(color, m_image->colorSpace()));
        resources->setBGColorOverride(KoColor(Qt::white, m_image->colorSpace()));
        resources->setOpacity(opacity);
        resources->setMirroring(false, false);
        resources->setSelectionOverride(m_selection);

        auto *stroke = dabRandomSeed
            ? new FreehandStrokeStrategy(resources,
                                         new KisFreehandStrokeInfo(),
                                         kundo2_noi18n("Freehand Stroke"),
                                         FreehandStrokeStrategy::None,
                                         *dabRandomSeed)
            : new FreehandStrokeStrategy(resources, new KisFreehandStrokeInfo(), kundo2_noi18n("Freehand Stroke"));

        const KisStrokeId strokeId = m_image->startStroke(stroke);
        const KisPaintInformation start = fixedPaintInformation(QPointF(200.0, 200.0), startPressure, startSpeed);
        const KisPaintInformation end = fixedPaintInformation(QPointF(300.0, 300.0), endPressure, endSpeed);

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

    KisNodeSP layer() const
    {
        return m_layer;
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
    KisSelectionSP m_selection;
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
    void brushSpacingProducesMaintainedPixels();
    void speedSensorProducesMaintainedPixels();
    void rectangularSelectionClipsStrokePixels();
    void cancelledStrokeRestoresInitialImage();
    void undoRedoRestoresBothStates();
    void figurePreviewCommitsOnceOnRelease_data();
    void figurePreviewCommitsOnceOnRelease();
    void figureKeepsHeldStrokePressure_data();
    void figureKeepsHeldStrokePressure();
    void curvedFigureKeepsHeldStrokeWidth_data();
    void curvedFigureKeepsHeldStrokeWidth();
    void figureKeepsStrokeColorOpacityAndCompositing();
    void previewSampleKeepsHeldAppearanceWithoutDocumentWork_data();
    void previewSampleKeepsHeldAppearanceWithoutDocumentWork();
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
    fixture.useSizeSensor(QStringLiteral("fuzzy"));
    QCOMPARE(fixture.preset()->settings()->getString(QStringLiteral("SizeSensor")),
             QStringLiteral("<!DOCTYPE params><params id=\"fuzzy\"/>"));
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false, inputPressure, inputPressure, inputSpeed, inputSpeed, 17);
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
    fixture.runStroke(false, inputPressure, inputPressure, inputSpeed, inputSpeed, 17);
    QCOMPARE(imageDigest(fixture.layerImage()), maintainedFuzzySeed17Digest);
    QCOMPARE(fixture.projectionImage(), firstProjection);
    QCOMPARE(fixture.layerExactBounds(), maintainedFuzzySeed17Bounds);

    fixture.undo();
    QCOMPARE(fixture.layerImage(), initialLayer);
    fixture.runStroke(false, inputPressure, inputPressure, inputSpeed, inputSpeed, 18);
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

void FreehandStrokeContractTest::brushSpacingProducesMaintainedPixels()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    KisBrushOpSettings *settings = fixture.brushSettings();
    QVERIFY(settings);
    settings->setSpacing(changedBrushSpacing);
    QCOMPARE(settings->spacing(), changedBrushSpacing);
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false);
    const QImage layer = fixture.layerImage();
    const QImage projection = fixture.projectionImage();
    const QRect bounds = fixture.layerExactBounds();
    const QByteArray digest = imageDigest(layer);
    QVERIFY(layer != initialLayer);
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());

    QPoint mismatch;
    QVERIFY2(
        compareImages(layer,
                      projection,
                      QStringLiteral("freehand-contract-spacing-025-projection-actual.png"),
                      &mismatch),
        qPrintable(
            QStringLiteral("spacing 0.25 layer and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));

    QCOMPARE(bounds, maintainedSpacing025Bounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), maintainedSpacing025Bounds);
    QVERIFY(bounds.width() < maintainedStrokeBounds.width());
    QVERIFY(bounds.height() < maintainedStrokeBounds.height());
    if (digest != maintainedSpacing025Digest) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR) + QStringLiteral("/freehand-contract-spacing-025-actual.png"));
    }
    QCOMPARE(digest, maintainedSpacing025Digest);
}

void FreehandStrokeContractTest::speedSensorProducesMaintainedPixels()
{
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    fixture.useSizeSensor(QStringLiteral("speed"));
    QCOMPARE(fixture.preset()->settings()->getString(QStringLiteral("SizeSensor")),
             QStringLiteral("<!DOCTYPE params><params id=\"speed\"/>"));
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false, inputPressure, inputPressure, changedInputSpeed, changedInputSpeed);
    const QImage layer = fixture.layerImage();
    const QImage projection = fixture.projectionImage();
    const QRect bounds = fixture.layerExactBounds();
    const QByteArray digest = imageDigest(layer);
    QVERIFY(layer != initialLayer);
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());

    QPoint mismatch;
    QVERIFY2(
        compareImages(layer, projection, QStringLiteral("freehand-contract-speed-05-projection-actual.png"), &mismatch),
        qPrintable(
            QStringLiteral("speed 0.5 layer and projection differ at %1,%2").arg(mismatch.x()).arg(mismatch.y())));

    QCOMPARE(bounds, maintainedSpeed05Bounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), maintainedSpeed05Bounds);
    QVERIFY(bounds.width() < maintainedStrokeBounds.width());
    QVERIFY(bounds.height() < maintainedStrokeBounds.height());
    if (digest != maintainedSpeed05Digest) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR) + QStringLiteral("/freehand-contract-speed-05-actual.png"));
    }
    QCOMPARE(digest, maintainedSpeed05Digest);
}

void FreehandStrokeContractTest::rectangularSelectionClipsStrokePixels()
{
    /*
     * Consumer: A painter who draws while a rectangular selection is active.
     * Operation: Runs the maintained freehand stroke through a fixed image-coordinate selection.
     * Observable result: Layer and projection match inside the selection, and every pixel outside it stays unchanged.
     * Failure impact: A brush stroke can alter artwork outside the area selected by the painter.
     */
    FreehandStrokeFixture fixture;
    QVERIFY2(fixture.presetLoaded(), qPrintable(QStringLiteral("failed to load preset: %1").arg(fixture.presetPath())));
    fixture.select(rectangularSelectionBounds);
    const QImage initialLayer = fixture.layerImage();

    fixture.runStroke(false);

    const QImage layer = fixture.layerImage();
    const QImage projection = fixture.projectionImage();
    QVERIFY(layer != initialLayer);
    QVERIFY(!fixture.image()->hasUpdatesRunning());
    QVERIFY(fixture.image()->isIdle());
    QCOMPARE(fixture.layerExactBounds(), rectangularSelectionBounds);
    QCOMPARE(fixture.image()->projection()->exactBounds(), rectangularSelectionBounds);

    QPoint mismatch;
    QVERIFY2(compareImages(layer,
                           projection,
                           QStringLiteral("freehand-contract-rectangular-selection-projection-actual.png"),
                           &mismatch),
             qPrintable(QStringLiteral("selected stroke layer and projection differ at %1,%2")
                            .arg(mismatch.x())
                            .arg(mismatch.y())));

    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            if (!rectangularSelectionBounds.contains(x, y)) {
                QCOMPARE(layer.pixel(x, y), initialLayer.pixel(x, y));
            }
        }
    }

    const QByteArray digest = imageDigest(layer);
    if (maintainedRectangularSelectionDigest.isEmpty()) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR)
                   + QStringLiteral("/freehand-contract-rectangular-selection-actual.png"));
        QFAIL(qPrintable(QStringLiteral("record rectangular selection bounds %1,%2 %3x%4 and RGBA8888 SHA-256 %5")
                             .arg(fixture.layerExactBounds().x())
                             .arg(fixture.layerExactBounds().y())
                             .arg(fixture.layerExactBounds().width())
                             .arg(fixture.layerExactBounds().height())
                             .arg(QString::fromLatin1(digest))));
    }
    QCOMPARE(digest, maintainedRectangularSelectionDigest);
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

void FreehandStrokeContractTest::figurePreviewCommitsOnceOnRelease_data()
{
    QTest::addColumn<bool>("ellipse");
    QTest::addColumn<bool>("circle");
    QTest::newRow("line") << false << false;
    QTest::newRow("ellipse") << true << false;
    QTest::newRow("circle") << true << true;
}

void FreehandStrokeContractTest::figurePreviewCommitsOnceOnRelease()
{
    QFETCH(bool, ellipse);
    QFETCH(bool, circle);
    FreehandStrokeFixture fixture;
    QVERIFY(fixture.presetLoaded());
    fixture.runStroke(false);
    const QImage initial = fixture.projectionImage();
    fixture.runStroke(true);
    QCOMPARE(fixture.projectionImage(), initial);

    QVector<QPointF> points;
    if (ellipse) {
        for (int i = 0; i < 64; ++i) {
            const qreal angle = 2.0 * M_PI * i / 64;
            points.append(QPointF(250 + 180 * std::cos(angle), 250 + 120 * std::sin(angle)));
        }
    } else {
        points = {QPointF(80, 80), QPointF(420, 80)};
    }
    KisQuickShape shape = KisQuickShape::recognize(points);
    QVERIFY(shape.isValid());
    shape.setSnappedToCircle(circle);
    KisQuickShapePreview preview;
    QSignalSpy updates(fixture.image().data(), &KisImage::sigImageUpdated);
    for (int i = 0; i < 400; ++i) {
        if (ellipse) {
            shape.setScale(i % 2 ? 1.01 : 0.99);
            shape.setRotation(i % 2 ? M_PI / 4 : -M_PI / 4);
        } else {
            shape.setLineEnd(QPointF(420, i % 2 ? 84 : 76));
        }
        preview.update(shape);
    }
    QVERIFY(!preview.path().isEmpty());
    QVERIFY(fixture.image()->isIdle());
    QCOMPARE(updates.count(), 0);
    QCOMPARE(fixture.projectionImage(), initial);

    const QPainterPath releasedPath = preview.path();
    preview.clear();
    KoCanvasResourceProvider resourceManager;
    resourceManager.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(fixture.preset()));
    resourceManager.setResource(KoCanvasResource::ForegroundColor, KoColor(Qt::black, fixture.image()->colorSpace()));
    resourceManager.setResource(KoCanvasResource::BackgroundColor, KoColor(Qt::white, fixture.image()->colorSpace()));
    resourceManager.setResource(KoCanvasResource::Opacity, QVariant(1.0));
    resourceManager.setResource(KoCanvasResource::CurrentEffectiveCompositeOp, QStringLiteral("normal"));
    {
        KisFigurePaintingStroke stroke(kundo2_noi18n("Draw Shape"), fixture.image(), fixture.layer(),
                                       &resourceManager, KisFigurePaintingOptions::StrokeStyleForeground,
                                       KisFigurePaintingOptions::FillStyleNone);
        stroke.paintPainterPath(releasedPath);
    }
    fixture.image()->waitForDone();
    const QImage committed = fixture.projectionImage();
    QVERIFY(committed != initial);
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);
    fixture.redo();
    QCOMPARE(fixture.projectionImage(), committed);
    QVERIFY(fixture.image()->isIdle());
}

void FreehandStrokeContractTest::figureKeepsHeldStrokePressure_data()
{
    QTest::addColumn<qreal>("pressure");
    QTest::newRow("light-pressure") << qreal(0.25);
    QTest::newRow("half-pressure") << qreal(0.5);
}

void FreehandStrokeContractTest::figureKeepsHeldStrokePressure()
{
    QFETCH(qreal, pressure);
    FreehandStrokeFixture fixture;
    QVERIFY(fixture.presetLoaded());
    const QImage initial = fixture.projectionImage();
    fixture.runStroke(false, pressure, pressure);
    const QImage expected = fixture.projectionImage();
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);

    QPainterPath path;
    path.moveTo(200, 200);
    path.lineTo(300, 300);
    KoCanvasResourceProvider resources;
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(fixture.preset()));
    resources.setResource(KoCanvasResource::ForegroundColor, KoColor(Qt::black, fixture.image()->colorSpace()));
    resources.setResource(KoCanvasResource::Opacity, QVariant(1.0));
    resources.setResource(KoCanvasResource::CurrentEffectiveCompositeOp, QStringLiteral("normal"));
    const KisPaintInformation heldInput = fixedPaintInformation(QPointF(300, 300), pressure);
    const KisResourcesSnapshot captured(fixture.image(), fixture.layer(), resources.canvasResourcesInterface());
    fixture.brushSettings()->setPaintOpSize(600.0);
    {
        KisFigurePaintingStroke stroke(kundo2_noi18n("Draw Shape"), captured);
        stroke.paintStrokePath(path, heldInput);
    }
    fixture.image()->waitForDone();
    QVERIFY2(compareImages(expected, fixture.projectionImage(),
                           QStringLiteral("held-figure-pressure-actual.png")),
             "The figure must have the same width as the freehand stroke at the held pressure");
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);
    fixture.redo();
    QCOMPARE(fixture.projectionImage(), expected);
}

void FreehandStrokeContractTest::curvedFigureKeepsHeldStrokeWidth_data()
{
    QTest::addColumn<qreal>("radiusY");
    QTest::newRow("ellipse") << qreal(60.0);
    QTest::newRow("circle") << qreal(100.0);
}

void FreehandStrokeContractTest::curvedFigureKeepsHeldStrokeWidth()
{
    QFETCH(qreal, radiusY);
    FreehandStrokeFixture fixture;
    QVERIFY(fixture.presetLoaded());
    fixture.brushSettings()->setPaintOpSize(40.0);
    fixture.useSizeSensor(QStringLiteral("pressure"));
    const QImage initial = fixture.projectionImage();
    QPainterPath path;
    path.addEllipse(QPointF(250, 250), 100, radiusY);
    KoCanvasResourceProvider resources;
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(fixture.preset()));
    resources.setResource(KoCanvasResource::ForegroundColor, KoColor(Qt::black, fixture.image()->colorSpace()));
    resources.setResource(KoCanvasResource::Opacity, QVariant(1.0));
    resources.setResource(KoCanvasResource::CurrentEffectiveCompositeOp, QStringLiteral("normal"));
    {
        KisFigurePaintingStroke stroke(kundo2_noi18n("Draw Shape"), fixture.image(), fixture.layer(),
                                       &resources, KisFigurePaintingOptions::StrokeStyleForeground,
                                       KisFigurePaintingOptions::FillStyleNone);
        stroke.paintStrokePath(path, fixedPaintInformation(QPointF(350, 250), 0.5));
    }
    fixture.image()->waitForDone();
    const QImage committed = fixture.layerImage();
    // Half pressure gives a 20-pixel brush. Every side keeps that thickness;
    // the full-pressure path would also paint the points 12 pixels outside.
    const QVector<QPoint> onFigure {
        QPoint(350, 250), QPoint(150, 250),
        QPoint(250, 250 - radiusY), QPoint(250, 250 + radiusY)
    };
    const QVector<QPoint> outside {
        QPoint(362, 250), QPoint(138, 250),
        QPoint(250, 238 - radiusY), QPoint(250, 262 + radiusY)
    };
    for (const QPoint &point : onFigure) {
        QVERIFY(committed.pixelColor(point).alpha() > 0);
    }
    for (const QPoint &point : outside) {
        QCOMPARE(committed.pixelColor(point).alpha(), 0);
    }
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);
    fixture.redo();
    QCOMPARE(fixture.layerImage(), committed);
}

void FreehandStrokeContractTest::figureKeepsStrokeColorOpacityAndCompositing()
{
    FreehandStrokeFixture fixture;
    QVERIFY(fixture.presetLoaded());
    const QImage initial = fixture.projectionImage();
    const QColor color(200, 40, 90);
    fixture.runStroke(false, 0.5, 0.5, inputSpeed, inputSpeed, std::nullopt, color, 0.35);
    const QImage expected = fixture.projectionImage();
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);

    KoCanvasResourceProvider resources;
    resources.setResource(KoCanvasResource::CurrentPaintOpPreset, QVariant::fromValue(fixture.preset()));
    resources.setResource(KoCanvasResource::ForegroundColor, KoColor(color, fixture.image()->colorSpace()));
    resources.setResource(KoCanvasResource::BackgroundColor, KoColor(Qt::white, fixture.image()->colorSpace()));
    resources.setResource(KoCanvasResource::Opacity, QVariant(0.35));
    resources.setResource(KoCanvasResource::CurrentEffectiveCompositeOp, QStringLiteral("normal"));
    const KisResourcesSnapshot captured(fixture.image(), fixture.layer(), resources.canvasResourcesInterface());
    resources.setResource(KoCanvasResource::ForegroundColor, KoColor(Qt::blue, fixture.image()->colorSpace()));
    resources.setResource(KoCanvasResource::Opacity, QVariant(1.0));
    resources.setResource(KoCanvasResource::CurrentEffectiveCompositeOp, QStringLiteral("erase"));
    QPainterPath path;
    path.moveTo(200, 200);
    path.lineTo(300, 300);
    {
        KisFigurePaintingStroke stroke(kundo2_noi18n("Draw Shape"), captured);
        stroke.paintStrokePath(path, fixedPaintInformation(QPointF(300, 300), 0.5));
    }
    fixture.image()->waitForDone();
    QVERIFY2(compareImages(expected, fixture.projectionImage(),
                           QStringLiteral("held-figure-style-actual.png")),
             "A held figure must retain the stroke's color, opacity and compositing despite current settings changes");
    QCOMPARE(captured.currentFgColor(), KoColor(color, fixture.image()->colorSpace()));
    QCOMPARE(captured.opacity(), 0.35);
    QCOMPARE(captured.compositeOpId(), QStringLiteral("normal"));
    fixture.undo();
    QCOMPARE(fixture.projectionImage(), initial);
    fixture.redo();
    QCOMPARE(fixture.projectionImage(), expected);
}

void FreehandStrokeContractTest::previewSampleKeepsHeldAppearanceWithoutDocumentWork_data()
{
    QTest::addColumn<qreal>("pressure");
    QTest::addColumn<int>("canvasSize");
    QTest::newRow("light-pressure") << qreal(0.2) << 500;
    QTest::newRow("half-pressure") << qreal(0.5) << 500;
    QTest::newRow("large-document") << qreal(0.5) << 3508;
}

void FreehandStrokeContractTest::previewSampleKeepsHeldAppearanceWithoutDocumentWork()
{
    QFETCH(qreal, pressure);
    QFETCH(int, canvasSize);
    FreehandStrokeFixture fixture(canvasSize, canvasSize);
    QVERIFY(fixture.presetLoaded());
    fixture.preset()->settings()->setPaintOpSize(40);
    fixture.runStroke(false, pressure, pressure, inputSpeed, inputSpeed, 17, QColor(200, 40, 90), 0.35);
    const QImage original = fixture.projectionImage();
    KisResourcesSnapshot captured(fixture.image(), fixture.layer());
    captured.setBrush(fixture.preset());
    captured.setFGColorOverride(KoColor(QColor(200, 40, 90), fixture.image()->colorSpace()));
    captured.setOpacity(0.35);
    QSignalSpy updates(fixture.image().data(), &KisImage::sigImageUpdated);
    QElapsedTimer timer;
    timer.start();
    const auto sample = KisFigurePaintingStroke::createPreviewSample(
        captured, fixedPaintInformation(QPointF(300, 300), pressure), 17);
    qInfo() << "one-time preview material ms" << timer.elapsed() << "pixels" << sample.bounds.size();
    QVERIFY(sample.bounds.width() <= 256);
    QVERIFY(sample.bounds.height() <= 128);
    const QImage appearance = sample.device->convertToQImage(nullptr, sample.bounds);
    const QColor center = appearance.pixelColor(qRound(sample.line.center().x()), qRound(sample.line.center().y()));
    QVERIFY(center.alpha() > 0);
    QVERIFY(center.alpha() <= qCeil(255 * 0.35));
    QVERIFY(std::abs(center.red() - 200) <= 4);
    KisQuickShapePreview preview;
    preview.setStrokeSample(appearance, sample.line, sample.imageUnitsPerPixel);
    KisQuickShape shape = KisQuickShape::recognize({QPointF(100, 100), QPointF(300, 100)});
    for (int i = 0; i < 400; ++i) {
        shape.setLineEnd(QPointF(200 + i % 100, 100));
        preview.update(shape);
    }
    QCOMPARE(updates.count(), 0);
    QCOMPARE(fixture.projectionImage(), original);
    fixture.undo();
    QImage empty(original.size(), original.format());
    empty.fill(Qt::transparent);
    QCOMPARE(fixture.projectionImage(), empty);
    fixture.redo();
    QCOMPARE(fixture.projectionImage(), original);
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
