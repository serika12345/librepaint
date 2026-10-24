/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QFile>
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
#include <brushengine/kis_paintop.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_registry.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_undo_stores.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/KisMaskedFreehandStrokePainter.h>
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
// A constant-pressure isotropic circle on x == y preserves transposed pixels.
const QByteArray maintainedHalfPressureDigest("ffdae59742d86fcfcc3764eeb7d2e82c126cd9cb08fb7c7c97a94e8b46cd5bb9");
const QRect maintainedPressureGradientBounds(154, 154, 229, 229);
const QByteArray maintainedPressureGradientDigest("e9740f2b00ef8670a37aade2c4f96cec8197dfc96eb3e18adcc20f938b5f87c0");
const QRect maintainedFuzzySeed17Bounds(142, 142, 271, 271);
const QByteArray maintainedFuzzySeed17Digest("34a090d8b904e9950f2bf7868b2c7b1f78c2d5bb3ddb8a531a90f203721c21d3");
const QRect maintainedSpacing025Bounds(50, 50, 353, 353);
const QByteArray maintainedSpacing025Digest("9342d0c1e5bbb15abe0c20a19be7b651d6b87e9061ad1572d86012f3e01800c1");
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

class PixelBrushFactory : public KisPaintOpFactory
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

// A line implementation may update pixels without registering a spaced dab.
class DirectLinePaintOp final : public KisPaintOp
{
public:
    DirectLinePaintOp(KisPainter *painter, bool deferred)
        : KisPaintOp(painter), m_deferred(deferred)
    {}

    void paintLine(const KisPaintInformation &pi1,
                   const KisPaintInformation &pi2,
                   KisDistanceInformation *distance) override
    {
        Q_UNUSED(pi1);
        Q_UNUSED(distance);
        m_position = pi2.pos().toPoint();
        m_pending = true;
        if (!m_deferred) {
            paintPendingLine();
        }
    }

    std::pair<int, bool> doAsynchronousUpdate(QVector<KisRunnableStrokeJobData *> &jobs) override
    {
        Q_UNUSED(jobs);
        paintPendingLine();
        return {40, false};
    }

protected:
    KisSpacingInformation paintAt(const KisPaintInformation &info) override
    {
        paintPixel(info.pos().toPoint() + QPoint(m_pointCount++, 0), Qt::blue);
        return KisSpacingInformation(1.0);
    }

    KisSpacingInformation updateSpacingImpl(const KisPaintInformation &info) const override
    {
        Q_UNUSED(info);
        return KisSpacingInformation(1.0);
    }

private:
    void paintPixel(const QPoint &position, Qt::GlobalColor color)
    {
        const KoColor pixel(color, painter()->device()->colorSpace());
        painter()->device()->fill(position.x(), position.y(), 1, 1, pixel.data());
        painter()->addDirtyRect(QRect(position, QSize(1, 1)));
    }

    void paintPendingLine()
    {
        if (m_pending) {
            paintPixel(m_position, Qt::red);
            m_pending = false;
        }
    }

    bool m_deferred;
    bool m_pending{false};
    int m_pointCount{0};
    QPoint m_position;
};

class DirectLineBrushFactory final : public PixelBrushFactory
{
public:
    explicit DirectLineBrushFactory(bool deferred) : m_deferred(deferred) {}

    QString id() const override
    {
        return m_deferred ? QStringLiteral("contract-deferred-line") : QStringLiteral("contract-direct-line");
    }

    KisPaintOp *createOp(const KisPaintOpSettingsSP settings, KisPainter *painter,
                        KisNodeSP node, KisImageSP image) override
    {
        Q_UNUSED(settings);
        Q_UNUSED(node);
        Q_UNUSED(image);
        return new DirectLinePaintOp(painter, m_deferred);
    }

private:
    bool m_deferred;
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

bool diagonalPixelsMatch(const QImage &image, QPoint &mismatch)
{
    if (image.width() != image.height()) {
        mismatch = QPoint(-1, -1);
        return false;
    }
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < y; ++x) {
            if (image.pixel(x, y) != image.pixel(y, x)) {
                mismatch = QPoint(x, y);
                return false;
            }
        }
    }
    return true;
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
                   std::optional<int> dabRandomSeed = std::nullopt)
    {
        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(m_image, m_layer);
        resources->setBrush(m_preset);
        resources->setFGColorOverride(KoColor(Qt::black, m_image->colorSpace()));
        resources->setBGColorOverride(KoColor(Qt::white, m_image->colorSpace()));
        resources->setOpacity(1.0);
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

    void runStationaryStroke(int lineCount, bool initialPoint, bool conditionalPoint, bool clonePoint = false)
    {
        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(m_image, m_layer);
        resources->setBrush(m_preset);
        resources->setFGColorOverride(KoColor(Qt::black, m_image->colorSpace()));
        resources->setOpacity(0.5);
        resources->setMirroring(false, false);
        auto *stroke = new FreehandStrokeStrategy(resources, new KisFreehandStrokeInfo(),
                                                kundo2_noi18n("Stationary stroke"));
        const KisStrokeId id = m_image->startStroke(stroke);
        const auto info = fixedPaintInformation(QPointF(250.0, 250.0));
        if (initialPoint) {
            m_image->addJob(id, new FreehandStrokeStrategy::Data(0, info));
        }
        for (int i = 0; i < lineCount; ++i) {
            m_image->addJob(id, new FreehandStrokeStrategy::Data(0, info, info));
            m_image->addJob(id, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
        }
        if (conditionalPoint) {
            auto *point = new FreehandStrokeStrategy::Data(0, info);
            point->onlyIfEmpty = true;
            if (clonePoint) {
                m_image->addJob(id, point->createLodClone(1));
                delete point;
            } else {
                m_image->addJob(id, point);
            }
        }
        m_image->addJob(id, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
        m_image->endStroke(id);
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
    void stationaryCompletionProducesExactlyOneTap_data();
    void stationaryCompletionProducesExactlyOneTap();
    void directLineCompletionPreservesEachPainter_data();
    void directLineCompletionPreservesEachPainter();
};

void FreehandStrokeContractTest::initTestCase()
{
    KisPaintOpRegistry *registry = KisPaintOpRegistry::instance();
    QVERIFY(!registry->get(QStringLiteral("paintbrush")));
    registry->add(new PixelBrushFactory());
    registry->add(new DirectLineBrushFactory(false));
    registry->add(new DirectLineBrushFactory(true));
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

    QPoint symmetryMismatch;
    const bool symmetric = startPressure != endPressure || diagonalPixelsMatch(layer, symmetryMismatch);
    if (actualDigest != expectedDigest || !symmetric) {
        QDir().mkpath(QStringLiteral(FILES_OUTPUT_DIR));
        layer.save(QStringLiteral(FILES_OUTPUT_DIR)
                   + QStringLiteral("/freehand-contract-%1-actual.png").arg(resultName));
    }
    QVERIFY2(symmetric,
             qPrintable(QStringLiteral("constant-pressure circular stroke is asymmetric at %1,%2")
                            .arg(symmetryMismatch.x()).arg(symmetryMismatch.y())));
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
    // An isotropic circle on the fixed diagonal path preserves all channels
    // when x and y are exchanged, including the feathered edge.
    QPoint symmetryMismatch;
    QVERIFY2(diagonalPixelsMatch(layer, symmetryMismatch),
             qPrintable(QStringLiteral("circular stroke is asymmetric at %1,%2")
                            .arg(symmetryMismatch.x()).arg(symmetryMismatch.y())));
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

void FreehandStrokeContractTest::stationaryCompletionProducesExactlyOneTap_data()
{
    QTest::addColumn<int>("lineCount");
    QTest::addColumn<bool>("initialPoint");
    QTest::addColumn<bool>("clonePoint");
    for (int count : {0, 1, 2, 7}) {
        QTest::newRow(qPrintable(QStringLiteral("empty-%1-lines").arg(count))) << count << false << false;
        QTest::newRow(qPrintable(QStringLiteral("initial-dab-%1-lines").arg(count))) << count << true << false;
    }
    QTest::newRow("initial-dab-lod-clone") << 0 << true << true;
}

void FreehandStrokeContractTest::stationaryCompletionProducesExactlyOneTap()
{
    QFETCH(int, lineCount);
    QFETCH(bool, initialPoint);
    QFETCH(bool, clonePoint);
    FreehandStrokeFixture tap;
    FreehandStrokeFixture repeated;
    QVERIFY(tap.presetLoaded());
    QVERIFY(repeated.presetLoaded());
    tap.runStationaryStroke(0, true, false);
    repeated.runStationaryStroke(lineCount, initialPoint, true, clonePoint);
    QVERIFY(!tap.layerExactBounds().isEmpty());
    const QString tag = QString::fromLatin1(QTest::currentDataTag());
    QPoint mismatch;
    const bool equal = compareImages(tap.layerImage(), repeated.layerImage(),
                                    QStringLiteral("freehand-contract-stationary-%1-actual.png").arg(tag), &mismatch);
    if (!equal) {
        tap.layerImage().save(QStringLiteral(FILES_OUTPUT_DIR) +
                              QStringLiteral("/freehand-contract-stationary-%1-expected.png").arg(tag));
    }
    QVERIFY2(equal, qPrintable(QStringLiteral("stationary completion differs from tap at %1,%2")
                                  .arg(mismatch.x()).arg(mismatch.y())));
    QCOMPARE(repeated.projectionImage(), tap.projectionImage());
}

void FreehandStrokeContractTest::directLineCompletionPreservesEachPainter_data()
{
    QTest::addColumn<bool>("deferred");
    QTest::addColumn<bool>("lineIsMask");
    for (bool deferred : {false, true}) {
        for (bool mask : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("%1-line-%2")
                                        .arg(deferred ? QStringLiteral("deferred") : QStringLiteral("immediate"),
                                             mask ? QStringLiteral("mask") : QStringLiteral("main"))))
                << deferred << mask;
        }
    }
}

void FreehandStrokeContractTest::directLineCompletionPreservesEachPainter()
{
    QFETCH(bool, deferred);
    QFETCH(bool, lineIsMask);
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP mainDevice = new KisPaintDevice(cs);
    KisPaintDeviceSP maskDevice = new KisPaintDevice(cs);
    KisFreehandStrokeInfo main;
    KisFreehandStrokeInfo mask;
    main.painter->begin(mainDevice);
    mask.painter->begin(maskDevice);
    auto preset = [](const QString &id) {
        KisPaintOpPresetSP result(new KisPaintOpPreset());
        KisPaintOpSettingsSP settings = new KisBrushOpSettings(KisGlobalResourcesInterface::instance());
        settings->setProperty(QStringLiteral("paintop"), id);
        result->setSettings(settings);
        return result;
    };
    const auto direct = preset(deferred ? QStringLiteral("contract-deferred-line")
                                        : QStringLiteral("contract-direct-line"));
    const auto other = preset(QStringLiteral("contract-direct-line"));
    main.painter->setPaintOpPreset(lineIsMask ? other : direct, {}, {});
    mask.painter->setPaintOpPreset(lineIsMask ? direct : other, {}, {});
    KisMaskedFreehandStrokePainter painter(&main, &mask);
    const auto info = fixedPaintInformation(QPointF(250.0, 250.0));
    KisFreehandStrokeInfo &line = lineIsMask ? mask : main;
    line.painter->paintLine(info, info, line.dragDistance);
    if (deferred) {
        QVector<KisRunnableStrokeJobData *> jobs;
        painter.doAsynchronousUpdate(jobs);
        QVERIFY(jobs.isEmpty());
    }
    QVERIFY(!line.dragDistance->isStarted());
    QVERIFY(painter.hasDirtyRegion());
    QVERIFY(!painter.takeDirtyRegion().isEmpty());
    QVERIFY(!painter.hasDirtyRegion());
    painter.paintAtIfEmpty(info);
    const QImage mainImage = deviceImage(mainDevice);
    const QImage maskImage = deviceImage(maskDevice);
    QCOMPARE(mainImage.pixelColor(250, 250), QColor(lineIsMask ? Qt::blue : Qt::red));
    QCOMPARE(maskImage.pixelColor(250, 250), QColor(lineIsMask ? Qt::red : Qt::blue));
    painter.takeDirtyRegion();
    painter.paintAtIfEmpty(info);
    QCOMPARE(deviceImage(mainDevice), mainImage);
    QCOMPARE(deviceImage(maskDevice), maskImage);
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

    QTemporaryDir fontDirectory;
    if (!fontDirectory.isValid()) return 1;
    QDir fontRoot(fontDirectory.path());
    if (!fontRoot.mkdir(QStringLiteral("fonts")) || !fontRoot.mkdir(QStringLiteral("cache"))) return 1;
    Q_INIT_RESOURCE(FreehandStrokeFonts);
    if (!QFile::copy(QStringLiteral(":/freehand-contract-fonts/DejaVuSans.ttf"),
                     fontRoot.filePath(QStringLiteral("fonts/DejaVuSans.ttf")))) return 1;
    QDomDocument fontConfiguration;
    auto configuration = fontConfiguration.createElement(QStringLiteral("fontconfig"));
    fontConfiguration.appendChild(configuration);
    for (const auto &entry : {qMakePair(QStringLiteral("dir"), QStringLiteral("fonts")),
                              qMakePair(QStringLiteral("cachedir"), QStringLiteral("cache"))}) {
        auto element = fontConfiguration.createElement(entry.first);
        element.appendChild(fontConfiguration.createTextNode(fontRoot.filePath(entry.second)));
        configuration.appendChild(element);
    }
    QFile fontFile(fontRoot.filePath(QStringLiteral("fonts.conf")));
    if (!fontFile.open(QIODevice::WriteOnly)) return 1;
    const auto configurationBytes = fontConfiguration.toByteArray();
    if (fontFile.write(configurationBytes) != configurationBytes.size()) return 1;
    fontFile.close();
    qputenv("FONTCONFIG_FILE", fontFile.fileName().toUtf8());

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    registerResources();

    FreehandStrokeContractTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "FreehandStrokeContractTest.moc"
