/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <testui.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <document/KisDocument.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <document/kis_filter_manager.h>
#include <generator/kis_generator.h>
#include <generator/kis_generator_registry.h>
#include <generator/kis_generator_layer.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <input/ui/kis_input_profile_manager.h>
#include <nodes/kis_node_manager.h>
#include <opengl/kis_opengl.h>
#include <tool/kis_tool_freehand_helper.h>
#include <kis_painting_information_builder.h>
#include <kis_smoothing_options.h>
#include <kis_config_widget.h>
#include <kis_animation_importer.h>
#include <kis_image_animation_interface.h>
#include <kis_keyframe_channel.h>
#include <kis_paint_information.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_transparency_mask.h>
#include <kis_paintop_registry.h>
#include <kis_paintop_preset.h>
#include <kis_paintop_settings.h>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceStorage.h>
#include <KisResourceModel.h>
#include <KisResourceLoaderRegistry.h>
#include <KoCanvasResourcesIds.h>
#include <KoColorProfile.h>
#include <KoColorSet.h>
#include <KoStopGradient.h>
#include <KisSwatch.h>
#include <KoResourcePaths.h>
#include <KoJsonTrader.h>
#include <Node.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDomDocument>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include <QListWidget>
#include <functional>
#include <exception>
#include <memory>
#include <algorithm>
#include <cmath>

namespace Gpu = LibrePaint::Image::Gpu;

namespace {
struct Outcome {
    Gpu::ExecutionLocation location = Gpu::ExecutionLocation::Gpu;
    QString error;
};

class SmokeDocument
{
public:
    SmokeDocument()
    {
        document.reset(KisPart::instance()->createDocument());
        document->setFileBatchMode(true);
        document->setAutoSaveActive(false);
        const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
        if (!document->newImage(QStringLiteral("GPU operation smoke"), 64, 48, cs,
                KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1)) {
            error = document->errorMessage();
            if (error.isEmpty()) error = QStringLiteral("Document creation failed");
            return;
        }
        image = document->image();
        owner = image->gpuImage();
        node = image->root()->firstChild();
        if (!owner || !node || !node->paintDevice()) {
            error = QStringLiteral("Default RGBA document did not use the GPU engine");
            return;
        }
        // All probes start with visible, nonuniform native pixels. Smudge and
        // destructive filters must change actual pixels to count as working.
        QScopedPointer<Node> api(Node::createNode(image, node));
        QByteArray seed(64 * 48 * 4, Qt::Uninitialized);
        for (int y = 0; y < 48; ++y) for (int x = 0; x < 64; ++x) {
            auto *pixel = seed.data() + (y * 64 + x) * 4;
            pixel[0] = char((x * 7 + y * 3) % 256);
            pixel[1] = char((y * 11 + 41) % 256);
            pixel[2] = char((x * 13 + 17) % 256);
            pixel[3] = char(255);
        }
        if (!api->setPixelData(seed, 0, 0, 64, 48))
            error = QStringLiteral("Could not initialize GPU layer pixels");
    }

    ~SmokeDocument()
    {
        if (!window) return;
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    bool openView()
    {
        window.reset(KisPart::instance()->createMainWindow());
        view = window->newView(document.get());
        if (!view) return false;
        lifetime = document.release();
        window->viewManager()->setCurrentView(view);
        window->viewManager()->nodeManager()->slotUiActivatedNode(node);
        return true;
    }

    QByteArray pixels() const
    {
        QScopedPointer<Node> api(Node::createNode(image, node));
        return api->pixelData(0, 0, 64, 48);
    }

    std::unique_ptr<KisDocument> document;
    std::unique_ptr<KisMainWindow> window;
    QPointer<KisDocument> lifetime;
    KisView *view = nullptr;
    KisImageSP image;
    KisNodeSP node;
    Gpu::PaintImageSP owner;
    QString error;
};

class BrushProbe : public KisToolFreehandHelper
{
public:
    using KisToolFreehandHelper::KisToolFreehandHelper;
    using KisToolFreehandHelper::initPaintImpl;
    using KisToolFreehandHelper::paintAt;
    using KisToolFreehandHelper::paintLine;
};

Outcome brush(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::brushExecutionLocation(id);
    if (!KisPaintOpRegistry::instance()->value(id))
        return {location, QStringLiteral("Brush engine was not registered")};
    const bool mypaint = id == QLatin1String("mypaintbrush");
    const auto extension = mypaint ? QStringLiteral(".myb") : QStringLiteral(".kpp");
    const auto path = QStringLiteral(KRITA_SOURCE_DIR "/plugins/paintops/defaultpresets/") + id + extension;
    KisPaintOpPresetSP preset;
    if (mypaint) {
        const auto loader = KisResourceLoaderRegistry::instance()->loader(
            ResourceType::PaintOpPresets, QStringLiteral("application/x-mypaint-brush"));
        if (loader) preset = loader->create(path).dynamicCast<KisPaintOpPreset>();
    } else {
        preset.reset(new KisPaintOpPreset(path));
    }
    if (!preset) return {location, QStringLiteral("Brush preset loader was not registered")};
    if (!preset->load(KisGlobalResourcesInterface::instance()) || !preset->valid())
        return {location, QStringLiteral("Default brush preset could not be loaded")};
    if (preset->paintOp().id() != id)
        return {location, QStringLiteral("Default preset selected a different brush engine")};
    preset->setName(QStringLiteral("Operation smoke ") + id);
    preset->setFilename(QStringLiteral("operation-smoke-") + id + QStringLiteral(".kpp"));
    if (id == QLatin1String("filter")) {
        const auto implementation = KisFilterRegistry::instance()->value(QStringLiteral("invert"));
        if (!implementation) return {location, QStringLiteral("Brush filter was not registered")};
        preset->settings()->setProperty("Filter/id", QStringLiteral("invert"));
        preset->settings()->setProperty("Filter/configuration",
            implementation->defaultConfiguration(KisGlobalResourcesInterface::instance())->toXML());
    }
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    if (!presets.addResource(preset)) return {location, QStringLiteral("Could not register brush preset")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *resources = doc.window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
    if (!resources->currentPreset() || resources->currentPreset()->paintOp().id() != id)
        return {location, QStringLiteral("Document view selected a different brush engine")};
    resources->setSize(12);
    resources->setOpacity(1);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setFGColor(KoColor(QColor(221, 19, 73), doc.image->colorSpace()));
    doc.view->canvasBase()->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    KisPaintingInformationBuilder builder;
    BrushProbe helper(&builder, doc.view->canvasBase()->resourceManager(), kundo2_i18n("Paint stroke"));
    helper.smoothingOptions()->setSmoothingType(KisSmoothingOptions::NO_SMOOTHING);
    QSignalSpy errors(&helper, &KisToolFreehandHelper::requestErrorMessage);
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    KisPaintInformation first(QPointF(16.5, 24.5), 1.0, 20.0, 15.0, 0.0);
    if (id == QLatin1String("duplicate")) {
        auto settings = resources->currentPreset()->settings();
        if (settings->mousePressEvent(KisPaintInformation(QPointF(4.5, 6.5)), Qt::ControlModifier, doc.node) ||
            !settings->mousePressEvent(first, Qt::NoModifier, doc.node))
            return {location, QStringLiteral("Clone brush source could not be selected")};
    }
    helper.initPaintImpl(0, first, doc.view->canvasBase()->resourceManager(), doc.image, doc.node, doc.image.data());
    if (!helper.isRunning()) return {location, QStringLiteral("Brush stroke did not start")};
    helper.paintAt(first);
    // Curves and shape brushes need a series of changing directions; MyPaint
    // also consumes elapsed input time. These are ordinary tablet samples.
    auto previous = first;
    for (int i = 1; i <= 24; ++i) {
        KisPaintInformation next(QPointF(16.5 + i, 24.5 + 10.0 * std::sin(i * 0.2)),
            1.0, 20.0, 15.0, 0.0);
        next.setCurrentTime(i * 20.0);
        helper.paintLine(previous, next);
        previous = next;
    }
    helper.endPaint();
    doc.image->waitForDone();
    if (id == QLatin1String("duplicate")) resources->currentPreset()->settings()->mouseReleaseEvent();
    if (!errors.isEmpty()) return {location, errors.first().first().toString()};
    if (doc.owner->operations().undoCount() != history + 1 || doc.pixels() == before)
        return {location, QStringLiteral("Brush did not publish changed pixels and one history entry")};
    return {location, {}};
}

Outcome filter(SmokeDocument &doc, const QString &id, bool brightnessContrast = false)
{
    const auto location = Gpu::filterExecutionLocation(id);
    const auto implementation = KisFilterRegistry::instance()->value(id);
    if (!implementation) return {location, QStringLiteral("Filter plugin was not registered")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto config = implementation->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (!config) return {location, QStringLiteral("Filter did not provide its default configuration")};
    QTemporaryDir reference;
    if (id == QLatin1String("levels")) config->setProperty("gammavalue", 1.4);
    else if (id == QLatin1String("perchannel")) {
        // The current curves filter owns brightness/contrast as a lightness
        // curve, including the legacy brightnesscontrast KRA conversion.
        config->setProperty("nTransfers", 8);
        config->setProperty(brightnessContrast ? "curve7" : "curve0",
            QStringLiteral("0,0;0.5,0.7;1,1;"));
    } else if (id == QLatin1String("hsvadjustment")) {
        config->setProperty("h", 32);
        config->setProperty("s", 15);
        config->setProperty("v", 10);
    } else if (id == QLatin1String("colorbalance")) config->setProperty("cyan_red_midtones", 35);
    else if (id == QLatin1String("gaussian blur")) {
        config->setProperty("horizRadius", 2.0);
        config->setProperty("vertRadius", 2.0);
    } else if (id == QLatin1String("posterize")) config->setProperty("steps", 4);
    else if (id == QLatin1String("gaussiannoisereducer")) config->setProperty("threshold", 0);
    else if (id == QLatin1String("raindrops")) {
        // Fit several drops inside the small smoke document.
        config->setProperty("dropSize", 12);
        config->setProperty("number", 5);
        config->setProperty("seed", 41);
    } else if (id == QLatin1String("noise") || id == QLatin1String("randompick")) {
        for (const auto *key : {"seedThreshold", "seedRed", "seedGreen", "seedBlue", "seedH", "seedV"})
            config->setProperty(key, 41);
    } else if (id == QLatin1String("crosschannel")) {
        config->setProperty("nTransfers", 8);
        config->setProperty("driver6", 7);
        config->setProperty("curve6", QStringLiteral("0,0.5;1,0.85;"));
    } else if (id == QLatin1String("asc-cdl")) {
        config->setProperty("slope", QVariant::fromValue(KoColor(QColor(180, 220, 200), doc.image->colorSpace())));
    } else if (id == QLatin1String("phongbumpmap")) {
        config->setProperty("heightChannel", doc.image->colorSpace()->channels().first()->name());
    } else if (id == QLatin1String("gradientmap")) {
        KoStopGradient gradient;
        gradient.setName(QStringLiteral("Operation smoke gradient"));
        gradient.setStops({KoGradientStop(0.0, KoColor(QColor(19, 41, 173), doc.image->colorSpace())),
                           KoGradientStop(1.0, KoColor(QColor(239, 181, 31), doc.image->colorSpace()))});
        QDomDocument xml;
        auto element = xml.createElement(QStringLiteral("gradient"));
        gradient.toXML(xml, element);
        xml.appendChild(element);
        config->setProperty("gradientXML", xml.toString());
    } else if (id == QLatin1String("palettize")) {
        KoColorSetSP palette(new KoColorSet);
        palette->setName(QStringLiteral("Operation smoke palette"));
        palette->setFilename(QStringLiteral("operation-smoke.kpl"));
        for (const auto &color : {QColor(Qt::black), QColor(Qt::white), QColor(221, 19, 73), QColor(31, 173, 91)})
            palette->addSwatch(KisSwatch(KoColor(color, doc.image->colorSpace())));
        palette->setValid(true);
        KisResourceModel palettes(ResourceType::Palettes);
        if (!palettes.addResource(palette)) return {location, QStringLiteral("Could not register filter palette")};
        config->setProperty("md5sum", palette->md5Sum());
        config->setProperty("palette", palette->name());
    } else if (id == QLatin1String("colortransfer")) {
        // The ordinary configuration widget imports the reference image and
        // records the Lab statistics consumed by the filter.
        if (!reference.isValid()) return {location, QStringLiteral("Could not create reference image directory")};
        const auto path = reference.filePath(QStringLiteral("reference.png"));
        QImage image(32, 24, QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y) for (int x = 0; x < image.width(); ++x)
            image.setPixelColor(x, y, QColor(80 + x * 4, 30 + y * 6, 40 + (x + y) * 3));
        if (!image.save(path)) return {location, QStringLiteral("Could not save reference image")};
        std::unique_ptr<KisConfigWidget> widget(implementation->createConfigurationWidget(nullptr, doc.node->paintDevice(), false));
        if (!widget) return {location, QStringLiteral("Could not create color transfer configuration widget")};
        config->setProperty("filename", path);
        widget->setConfiguration(config);
        config = dynamic_cast<KisFilterConfiguration *>(widget->configuration().data());
        if (!config || !config->hasProperty("meanL") || !config->hasProperty("sigmaL"))
            return {location, QStringLiteral("Reference image statistics were not loaded")};
    } else if (id == QLatin1String("resettransparent") || id == QLatin1String("propagatecolors")) {
        QScopedPointer<Node> api(Node::createNode(doc.image, doc.node));
        QByteArray hole(8 * 8 * 4, char(71));
        for (int i = 3; i < hole.size(); i += 4) hole[i] = 0;
        if (!api->setPixelData(hole, 24, 16, 8, 8))
            return {location, QStringLiteral("Could not initialize transparent filter pixels")};
    }
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    doc.window->viewManager()->filterManager()->apply(config);
    doc.image->waitForDone();
    const auto after = doc.pixels();
    if (doc.image->gpuImage() != doc.owner || before.size() != 64 * 48 * 4 ||
        after.size() != before.size() || doc.owner->operations().undoCount() != history + 1 || after == before)
        return {location, QStringLiteral("Filter did not publish changed pixels and one history entry")};
    return {location, {}};
}

Outcome generator(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::layerExecutionLocation(Gpu::LayerFeature::Generator);
    const auto implementation = KisGeneratorRegistry::instance()->value(id);
    if (!implementation) return {location, QStringLiteral("Generator plugin was not registered")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto config = implementation->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (!config) return {location, QStringLiteral("Generator did not provide its default configuration")};
    if (id == QLatin1String("color"))
        config->setProperty("color", QVariant::fromValue(KoColor(QColor(39, 131, 217), doc.image->colorSpace())));
    else if (id == QLatin1String("simplex_noise")) config->setProperty("seed", 41);
    else if (id == QLatin1String("seexpr")) config->setProperty("script", QStringLiteral("[$u, $v, 0.35]"));
    else if (id == QLatin1String("gradient")) {
        KoStopGradient gradient;
        gradient.setName(QStringLiteral("Operation smoke generator gradient"));
        gradient.setStops({KoGradientStop(0.0, KoColor(QColor(19, 41, 173), doc.image->colorSpace())),
                           KoGradientStop(1.0, KoColor(QColor(239, 181, 31), doc.image->colorSpace()))});
        QDomDocument xml;
        auto element = xml.createElement(QStringLiteral("gradient"));
        gradient.toXML(xml, element);
        xml.appendChild(element);
        config->setProperty("gradient", xml.toString());
    }
    // Select the generator in the ordinary Add Fill Layer dialog. Keeping the
    // timer scoped also covers an operation that returns before opening it.
    QTimer dialogDriver;
    dialogDriver.setInterval(10);
    bool configured = false;
    QString error;
    QObject::connect(&dialogDriver, &QTimer::timeout, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog || !dialog->inherits("KisDlgGeneratorLayer")) return;
        dialogDriver.stop();
        try {
            auto ids = KisGeneratorRegistry::instance()->keys();
            ids.sort();
            auto *gallery = dialog->findChild<QListWidget *>(QStringLiteral("lstGenerators"));
            if (gallery) gallery->setCurrentRow(ids.indexOf(id));
            auto *widget = dialog->findChild<KisConfigWidget *>();
            if (widget) widget->setConfiguration(config);
            const KisFilterConfigurationSP selected = widget ?
                dynamic_cast<KisFilterConfiguration *>(widget->configuration().data()) : nullptr;
            configured = selected && selected->name() == id &&
                QMetaObject::invokeMethod(dialog, "slotDelayedPreviewGenerator", Qt::DirectConnection);
            if (configured) dialog->accept();
            else dialog->reject();
        } catch (const std::exception &exception) {
            error = QString::fromUtf8(exception.what());
            dialog->reject();
        }
    });
    const auto history = doc.owner->operations().undoCount();
    const auto count = doc.image->root()->childCount();
    dialogDriver.start();
    const auto added = doc.window->viewManager()->nodeManager()->createNode(QStringLiteral("KisGeneratorLayer"));
    doc.image->waitForDone();
    if (!error.isEmpty()) return {location, error};
    const auto layer = qobject_cast<const KisGeneratorLayer *>(added.data());
    if (!configured || !layer || !layer->parent() || !layer->filter() || layer->filter()->name() != id ||
        doc.image->gpuImage() != doc.owner || doc.image->root()->childCount() != count + 1 ||
        doc.owner->operations().undoCount() != history + 1)
        return {location, QStringLiteral("Generator did not publish its configured layer and one history entry (A9)")};
    QScopedPointer<Node> api(Node::createNode(doc.image, added));
    const auto pixels = api->pixelData(0, 0, 64, 48);
    bool visible = false;
    for (qsizetype i = 3; i < pixels.size(); i += 4) visible |= quint8(pixels[i]) != 0;
    if (pixels.size() != 64 * 48 * 4 || !visible)
        return {location, QStringLiteral("Generator did not publish visible native pixels (A9)")};
    return {location, {}};
}

Outcome gmic(SmokeDocument &doc)
{
    const auto location = Gpu::filterExecutionLocation(QStringLiteral("gmic"));
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    if (KoJsonTrader::instance()->query(QStringLiteral("Krita/GMic"), {}).isEmpty())
        return {location, QStringLiteral("External G'MIC-Qt plugin is unavailable in the application plugin directory (A10)")};
    const auto action = doc.window->viewManager()->actionManager()->actionByName(QStringLiteral("QMicAgain"));
    if (!action) return {location, QStringLiteral("G'MIC action plugin was not registered")};
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return {location, QStringLiteral("Could not create G'MIC settings directory")};
    const auto previousFormat = QSettings::defaultFormat();
    const auto restore = qScopeGuard([&] { QSettings::setDefaultFormat(previousFormat); });
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    // Repeat Last Filter is the ordinary noninteractive G'MIC entry point.
    // A custom command avoids downloads and version-specific filter paths.
#ifdef Q_OS_MACOS
    QSettings settings(QStringLiteral("greyc.fr"), QStringLiteral("gmic_qt"));
#else
    QSettings settings(QStringLiteral("GREYC"), QStringLiteral("gmic_qt"));
#endif
    settings.setValue(QStringLiteral("LastExecution/host_krita/Command"), QStringLiteral("mul"));
    settings.setValue(QStringLiteral("LastExecution/host_krita/Arguments"), QStringLiteral("0.5"));
    settings.setValue(QStringLiteral("LastExecution/host_krita/FilterPath"), QString());
    settings.setValue(QStringLiteral("LastExecution/host_krita/GmicStatusString"), QString());
    settings.setValue(QStringLiteral("LastExecution/host_krita/InputMode"), 1);
    settings.setValue(QStringLiteral("LastExecution/host_krita/OutputMode"), 0);
    settings.sync();
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    action->setEnabled(true);
    action->trigger();
    doc.image->waitForDone();
    const auto after = doc.pixels();
    if (doc.image->gpuImage() != doc.owner || after.size() != before.size() || after == before ||
        doc.owner->operations().undoCount() != history + 1)
        return {location, QStringLiteral("G'MIC did not publish changed pixels and one history entry (A10)")};
    return {location, {}};
}
}

class KisGpuOperationSmokeTest : public QObject
{
    Q_OBJECT
    enum class Group { Normal, Brushes, Filters, SpatialFilters, ReliefFilters, ArtisticFilters,
                       ColorFilters, ResourceFilters, Generators };
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        KoResourcePaths::addAssetDir(QStringLiteral("kis_actions"), QStringLiteral(KRITA_SOURCE_DIR "/krita"));
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        const auto path = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        const auto bundle = KisResourceStorageSP::create(path);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(path, bundle));
        KisResourceModel brushes(ResourceType::Brushes);
        if (brushes.rowCount() == 0)
            QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
        KisResourceModel patterns(ResourceType::Patterns);
        if (patterns.rowCount() == 0)
            QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    }

    void normalOperations()
    {
        runOperations(Group::Normal);
    }

    void brushes()
    {
        runOperations(Group::Brushes);
    }

    void filters()
    {
        runOperations(Group::Filters);
    }

    void spatialFilters()
    {
        runOperations(Group::SpatialFilters);
    }

    void reliefFilters()
    {
        runOperations(Group::ReliefFilters);
    }

    void artisticFilters()
    {
        runOperations(Group::ArtisticFilters);
    }

    void colorFilters()
    {
        runOperations(Group::ColorFilters);
    }

    void resourceFilters()
    {
        runOperations(Group::ResourceFilters);
    }

    void generators()
    {
        runOperations(Group::Generators);
    }

private:
    void runOperations(Group group)
    {
        const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
        const auto restore = qScopeGuard([&] {
            if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
            else qputenv("LIBREPAINT_GPU_ENGINE", previous);
        });
        qunsetenv("LIBREPAINT_GPU_ENGINE");
        struct Probe {
            QString id;
            bool resident;
            std::function<Outcome(SmokeDocument &)> execute;
            Group group = Group::Normal;
        };
        QList<Probe> probes = {
            {QStringLiteral("document/new-rgba-u8"), true, [](SmokeDocument &) { return Outcome{}; }},
            {QStringLiteral("selection/rectangle"), true, [](SmokeDocument &doc) {
                const QRect selected(11, 9, 23, 17);
                if (!doc.image->setGpuSelectionRect(selected) || !doc.image->globalSelection() ||
                    doc.image->globalSelection()->selectedExactRect() != selected)
                    return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Selection coverage was not adopted")};
                return Outcome{};
            }},
            {QStringLiteral("layer/paint"), true, [](SmokeDocument &doc) {
                if (!doc.openView()) return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Could not open the document view")};
                const auto count = doc.image->root()->childCount();
                const auto added = doc.window->viewManager()->nodeManager()->createNode(QStringLiteral("KisPaintLayer"));
                if (!added || !added->paintDevice() ||
                    doc.image->root()->childCount() != count + 1)
                    return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Paint layer was not added to the GPU document")};
                for (const auto &layer : doc.owner->operations().snapshot()->layers)
                    if (layer.uuid == added->uuid()) return Outcome{};
                return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Added paint layer has no GPU version")};
            }},
            {QStringLiteral("mask/transparency"), true, [](SmokeDocument &doc) {
                if (!doc.openView()) return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Could not open the document view")};
                const auto added = doc.window->viewManager()->nodeManager()->createNode(QStringLiteral("KisTransparencyMask"));
                const auto mask = qobject_cast<const KisTransparencyMask *>(added.data());
                if (!mask || !mask->selection() || !doc.image->isGpuTransparencyMaskView(added) ||
                    added->parent() != doc.node)
                    return Outcome{Gpu::ExecutionLocation::Gpu, QStringLiteral("Transparency mask was not added to the GPU document")};
                return Outcome{};
            }},
            {QStringLiteral("api/native-pixels"), false, [](SmokeDocument &doc) {
                QScopedPointer<Node> api(Node::createNode(doc.image, doc.node));
                const auto value = QByteArray::fromHex("112233ad");
                if (!api->setPixelData(value, 17, 13, 1, 1) || api->pixelData(17, 13, 1, 1) != value)
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Public native pixel transfer failed")};
                return Outcome{Gpu::pixelAccessExecutionLocation(Gpu::PixelAccess::PublicApi), {}};
            }},
            {QStringLiteral("animation/start"), false, [](SmokeDocument &doc) {
                doc.node->enableAnimation();
                const auto channel = doc.node->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true);
                if (doc.image->gpuImage())
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Animation start did not migrate the document to CPU (A1c-4b2c)")};
                if (!channel || !channel->keyframeAt(0))
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Animation did not create a native raster frame")};
                return Outcome{Gpu::ExecutionLocation::Cpu, {}};
            }},
            {QStringLiteral("animation/import-frames"), false, [](SmokeDocument &doc) {
                QTemporaryDir directory;
                if (!directory.isValid()) return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Could not create frame fixture")};
                const auto path = directory.filePath(QStringLiteral("frame.png"));
                QImage frame(64, 48, QImage::Format_ARGB32);
                frame.fill(QColor(47, 139, 211));
                if (!frame.save(path)) return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Could not save frame fixture")};
                KisAnimationImporter importer(doc.document.get());
                const auto result = importer.import({path}, 7, 1);
                if (!result.isOk() || doc.image->gpuImage())
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Frame import did not migrate the document to CPU (A1c-4b2c)")};
                const auto imported = doc.image->root()->lastChild();
                const auto channel = imported->getKeyframeChannel(KisKeyframeChannel::Raster.id());
                if (!channel || !channel->keyframeAt(7))
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Imported raster frame was not retained")};
                return Outcome{Gpu::ExecutionLocation::Cpu, {}};
            }}
        };
        // #107 requires three resident engines and thirteen CPU engines.
        const QStringList brushIds = {QStringLiteral("paintbrush"), QStringLiteral("roundmarker"),
            QStringLiteral("colorsmudge"), QStringLiteral("duplicate"), QStringLiteral("hairybrush"),
            QStringLiteral("deformbrush"), QStringLiteral("curvebrush"), QStringLiteral("spraybrush"),
            QStringLiteral("filter"), QStringLiteral("experimentbrush"), QStringLiteral("particlebrush"),
            QStringLiteral("gridbrush"), QStringLiteral("hatchingbrush"), QStringLiteral("sketchbrush"),
            QStringLiteral("tangentnormal"), QStringLiteral("mypaintbrush")};
        for (const auto &id : brushIds)
            probes.append({QStringLiteral("brush/") + id,
                id == QLatin1String("paintbrush") || id == QLatin1String("roundmarker") || id == QLatin1String("colorsmudge"),
                [id](SmokeDocument &doc) { return brush(doc, id); }, Group::Brushes});
        // Keep #107's required placement independent of today's capabilities,
        // so CPU execution of a resident filter stays visible until A10.
        const QStringList residentFilters = {QStringLiteral("levels"), QStringLiteral("perchannel"),
            QStringLiteral("hsvadjustment"), QStringLiteral("colorbalance"), QStringLiteral("invert"),
            QStringLiteral("desaturate"), QStringLiteral("gaussian blur")};
        for (const auto &id : residentFilters)
            probes.append({QStringLiteral("filter/") + id, true,
                [id](SmokeDocument &doc) { return filter(doc, id); }, Group::Filters});
        probes.append({QStringLiteral("filter/brightness-contrast"), true,
            [](SmokeDocument &doc) { return filter(doc, QStringLiteral("perchannel"), true); }, Group::Filters});
        probes.append({QStringLiteral("filter/posterize"), false,
            [](SmokeDocument &doc) { return filter(doc, QStringLiteral("posterize")); }, Group::Filters});
        const auto addFilters = [&](Group filterGroup, const QStringList &ids) {
            for (const auto &id : ids)
                probes.append({QStringLiteral("filter/") + id, false,
                    [id](SmokeDocument &doc) { return filter(doc, id); }, filterGroup});
        };
        addFilters(Group::SpatialFilters, {QStringLiteral("blur"), QStringLiteral("motion blur"),
            QStringLiteral("lens blur"), QStringLiteral("gaussianhighpass"), QStringLiteral("unsharp"),
            QStringLiteral("gaussiannoisereducer"), QStringLiteral("waveletnoisereducer"),
            QStringLiteral("edge detection"), QStringLiteral("height to normal")});
        addFilters(Group::ReliefFilters, {QStringLiteral("sharpen"), QStringLiteral("mean removal"),
            QStringLiteral("emboss laplascian"), QStringLiteral("emboss all directions"),
            QStringLiteral("emboss horizontal and vertical"), QStringLiteral("emboss vertical only"),
            QStringLiteral("emboss horizontal only"), QStringLiteral("emboss")});
        addFilters(Group::ArtisticFilters, {QStringLiteral("noise"), QStringLiteral("oilpaint"),
            QStringLiteral("pixelize"), QStringLiteral("raindrops"), QStringLiteral("randompick"),
            QStringLiteral("roundcorners"), QStringLiteral("smalltiles"), QStringLiteral("wave")});
        addFilters(Group::ColorFilters, {QStringLiteral("autocontrast"), QStringLiteral("crosschannel"),
            QStringLiteral("asc-cdl"), QStringLiteral("dodge"), QStringLiteral("burn"),
            QStringLiteral("threshold"), QStringLiteral("normalize"), QStringLiteral("maximize"),
            QStringLiteral("minimize"), QStringLiteral("colortoalpha")});
        addFilters(Group::ResourceFilters, {QStringLiteral("fastcoloroverlay"), QStringLiteral("resettransparent"),
            QStringLiteral("propagatecolors"), QStringLiteral("gradientmap"), QStringLiteral("palettize"),
            QStringLiteral("indexcolors"), QStringLiteral("colortransfer"), QStringLiteral("halftone"),
            QStringLiteral("phongbumpmap")});
        const QStringList generatorIds = {QStringLiteral("color"), QStringLiteral("pattern"),
            QStringLiteral("gradient"), QStringLiteral("simplex_noise"), QStringLiteral("screentone"),
            QStringLiteral("multigrid"), QStringLiteral("seexpr")};
        for (const auto &id : generatorIds)
            probes.append({QStringLiteral("generator/") + id, false,
                [id](SmokeDocument &doc) { return generator(doc, id); }, Group::Generators});
        probes.append({QStringLiteral("filter/gmic"), false, gmic, Group::Generators});
        probes.erase(std::remove_if(probes.begin(), probes.end(), [group](const Probe &probe) {
            return probe.group != group;
        }), probes.end());
        static constexpr const char *functions[] = {"normalOperations", "brushes", "filters",
            "spatialFilters", "reliefFilters", "artisticFilters", "colorFilters", "resourceFilters", "generators"};
        const auto function = QString::fromLatin1(functions[int(group)]);
        const auto reportSuffix = group == Group::Normal ? QString() : QLatin1Char('.') + function;
        QJsonArray rows;
        for (const auto &probe : probes)
            rows.append(QJsonObject{{"id", probe.id}, {"required", probe.resident ? "gpu" : "cpu"},
                {"result", "failure"}, {"detail", "Operation did not finish"}});
        const auto publish = [&] {
            auto path = qEnvironmentVariable("LIBREPAINT_GPU_COVERAGE_FILE");
            if (path.isEmpty()) return true;
            path += reportSuffix;
            QSaveFile file(path);
            const auto bytes = QJsonDocument(QJsonObject{{"schema", 1}, {"scope", "A2b2b3"}, {"rows", rows}}).toJson();
            return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
        };
        QVERIFY(publish());
        const auto selected = qEnvironmentVariable("LIBREPAINT_GPU_COVERAGE_OPERATION");
        QTemporaryDir children;
        QVERIFY(children.isValid());
        int gpu = 0, cpu = 0, failed = 0;
        for (int i = 0; i < probes.size(); ++i) {
            if (!selected.isEmpty() && probes[i].id != selected) continue;
            Outcome outcome;
            if (selected.isEmpty()) {
                // A crashing or stuck operation contributes one failure while
                // the remaining ordinary operations still produce results.
                const auto root = children.filePath(QString::number(i));
                QVERIFY(QDir().mkpath(root));
                const auto report = root + QStringLiteral("/report.json");
                auto environment = QProcessEnvironment::systemEnvironment();
                environment.insert(QStringLiteral("LIBREPAINT_GPU_COVERAGE_OPERATION"), probes[i].id);
                environment.insert(QStringLiteral("LIBREPAINT_GPU_COVERAGE_FILE"), report);
                QProcess process;
                process.setProcessEnvironment(environment);
                process.setProcessChannelMode(QProcess::MergedChannels);
#if defined(Q_OS_UNIX) && QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                process.setUnixProcessParameters(QProcess::UnixProcessFlag::DisableCoreDumps);
#endif
                process.start(QCoreApplication::applicationFilePath(),
                    {function, QStringLiteral("-nocrashhandler")});
                const bool finished = process.waitForFinished(20000);
                if (!finished) {
                    process.kill();
                    process.waitForFinished(5000);
                }
                QFile file(report + reportSuffix);
                if (!finished || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || !file.open(QIODevice::ReadOnly)) {
                    outcome.error = !finished ? QStringLiteral("Operation process exceeded its time limit") :
                        QStringLiteral("Operation process failed (exit %1)").arg(process.exitCode());
                } else {
                    const auto childRows = QJsonDocument::fromJson(file.readAll()).object().value("rows").toArray();
                    const auto row = childRows.size() == probes.size() ? childRows[i].toObject() : QJsonObject{};
                    const auto result = row.value("result").toString();
                    if (row.value("id").toString() != probes[i].id ||
                        (result != QLatin1String("gpu") && result != QLatin1String("cpu") && result != QLatin1String("failure")))
                        outcome.error = QStringLiteral("Operation process produced an invalid report");
                    else if (result == QLatin1String("failure")) {
                        outcome.error = row.value("detail").toString();
                        if (outcome.error.isEmpty()) outcome.error = QStringLiteral("Operation process reported failure");
                    }
                    else outcome.location = result == QLatin1String("gpu") ? Gpu::ExecutionLocation::Gpu : Gpu::ExecutionLocation::Cpu;
                }
                if (!outcome.error.isEmpty())
                    qInfo().noquote() << probes[i].id << QString::fromUtf8(process.readAll().right(4000));
            } else {
                try {
                    SmokeDocument doc;
                    outcome = doc.error.isEmpty() ? probes[i].execute(doc) : Outcome{Gpu::ExecutionLocation::Gpu, doc.error};
                } catch (const std::exception &error) {
                    outcome.error = QString::fromUtf8(error.what());
                }
            }
            const auto result = !outcome.error.isEmpty() ? QStringLiteral("failure") :
                outcome.location == Gpu::ExecutionLocation::Gpu ? QStringLiteral("gpu") : QStringLiteral("cpu");
            auto row = rows[i].toObject();
            row["result"] = result;
            row["detail"] = outcome.error;
            rows[i] = row;
            QVERIFY(publish());
            qInfo().noquote() << "GPU_COVERAGE" << QJsonDocument(row).toJson(QJsonDocument::Compact);
            if (result == QLatin1String("gpu")) ++gpu;
            else if (result == QLatin1String("cpu")) ++cpu;
            else ++failed;
        }
        // Feature failures are the migration backlog. The report remains usable
        // before A14; coverage --require-complete enforces the final criterion.
        qInfo().noquote() << QStringLiteral("GPU=%1 CPU=%2 FAILURE=%3").arg(gpu).arg(cpu).arg(failed);
        QCOMPARE(gpu + cpu + failed, selected.isEmpty() ? probes.size() : 1);
    }
};

int main(int argc, char *argv[])
{
    qputenv("LANGUAGE", "en");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    qputenv("QT_LOGGING_RULES", "");
    QStandardPaths::setTestModeEnabled(true);
    TestUtil::initializeResourcePathsForTests();
    QApplication app(argc, argv);
    // Qt's test-mode resource database belongs to the application name. Give
    // each child its own database while the parent keeps its registry open.
    auto operation = qEnvironmentVariable("LIBREPAINT_GPU_COVERAGE_OPERATION");
    if (!operation.isEmpty())
        app.setApplicationName(QStringLiteral("KisGpuOperationSmokeTest-") + operation.replace('/', '-'));
    app.setAttribute(Qt::AA_Use96Dpi, true);
    QTEST_DISABLE_KEYPAD_NAVIGATION
    registerResources();
    KisGpuOperationSmokeTest tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
#include "KisGpuOperationSmokeTest.moc"
