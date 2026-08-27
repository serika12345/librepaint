/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDir>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoResourceLoadResult.h>
#include <simpletest.h>

#include <KisGlobalResourcesInterface.h>
#include <brushengine/kis_paint_information.h>
#include <brushengine/kis_paintop_factory.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_registry.h>
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

    void runStroke(bool cancel)
    {
        KisResourcesSnapshotSP resources = new KisResourcesSnapshot(m_image, m_layer);
        resources->setBrush(m_preset);
        resources->setFGColorOverride(KoColor(Qt::black, m_image->colorSpace()));
        resources->setBGColorOverride(KoColor(Qt::white, m_image->colorSpace()));

        auto *stroke =
            new FreehandStrokeStrategy(resources, new KisFreehandStrokeInfo(), kundo2_noi18n("Freehand Stroke"));

        const KisStrokeId strokeId = m_image->startStroke(stroke);
        const KisPaintInformation start(QPointF(200.0, 200.0));
        const KisPaintInformation end(QPointF(300.0, 300.0));

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

    QImage layerImage() const
    {
        return deviceImage(m_layer->paintDevice());
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
    void finishedStrokeMatchesMaintainedProjection();
    void cancelledStrokeRestoresInitialImage();
    void undoRedoRestoresBothStates();
};

void FreehandStrokeContractTest::initTestCase()
{
    KisPaintOpRegistry *registry = KisPaintOpRegistry::instance();
    QVERIFY(!registry->get(QStringLiteral("paintbrush")));
    registry->add(new PixelBrushFactory());
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
