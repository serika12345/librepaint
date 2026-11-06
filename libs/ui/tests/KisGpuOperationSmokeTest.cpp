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
#include <canvas/kis_coordinates_converter.h>
#include <document/KisDocument.h>
#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <document/kis_filter_manager.h>
#include <generator/kis_generator.h>
#include <generator/kis_generator_registry.h>
#include <generator/kis_generator_layer.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <gpu/KisGpuOperationDocument.h>
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
#include <kis_shape_layer.h>
#include <KoSvgTextShape.h>
#include <KoSvgTextShapeMarkupConverter.h>
#include <kis_pixel_selection.h>
#include <kis_selection.h>
#include <kis_selection_options.h>
#include <kis_slider_spin_box.h>
#include <operations/kis_operation_configuration.h>
#include <kis_transparency_mask.h>
#include <kis_paintop_registry.h>
#include <kis_paintop_preset.h>
#include <kis_paintop_settings.h>
#include <kis_tool.h>
#include <KisFigurePaintingOptions.h>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceStorage.h>
#include <KisResourceModel.h>
#include <KisResourceLoaderRegistry.h>
#include <KoCanvasResourcesIds.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSet.h>
#include <KoStopGradient.h>
#include <KisSwatch.h>
#include <KoResourcePaths.h>
#include <KoJsonTrader.h>
#include <KoPointerEvent.h>
#include <KoSnapGuide.h>
#include <KoToolManager.h>
#include <Node.h>
#include <Document.h>
#include <Filter.h>
#include <FilterLayer.h>
#include <Selection.h>
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
#include <QComboBox>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QMouseEvent>
#include <KConfigGroup>
#include <KSharedConfig>
#include <functional>
#include <exception>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

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

Outcome documentFormat(const QString &id)
{
    const auto registry = KoColorSpaceRegistry::instance();
    const auto integer8 = Integer8BitsColorDepthID.id();
    const KoColorSpace *cs = nullptr;
    bool resident = true;
    if (id == QLatin1String("rgba-u8")) cs = registry->rgb8();
    else if (id == QLatin1String("rgba-u16")) cs = registry->rgb16();
    else if (id == QLatin1String("rgba-f16"))
        cs = registry->colorSpace(RGBAColorModelID.id(), Float16BitsColorDepthID.id(), QString());
    else if (id == QLatin1String("rgba-f32"))
        cs = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), QString());
    else {
        resident = false;
        const auto model = id == QLatin1String("cmyka-u8") ? CMYKAColorModelID :
            id == QLatin1String("laba-u8") ? LABAColorModelID :
            id == QLatin1String("ycbcr-u8") ? YCbCrAColorModelID :
            id == QLatin1String("xyza-u8") ? XYZAColorModelID :
            id == QLatin1String("graya-u8") ? GrayAColorModelID : GrayColorModelID;
        cs = registry->colorSpace(model.id(), integer8, QString());
    }
    const auto location = resident ? Gpu::ExecutionLocation::Gpu : Gpu::ExecutionLocation::Cpu;
    if (!cs) return {location, QStringLiteral("Document colour space was not registered")};
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    const bool created = document->newImage(QStringLiteral("Operation smoke ") + id, 32, 16, cs,
        KoColor(QColor(19, 41, 73), cs), KisConfig::CANVAS_COLOR, 1, {}, 1);
    if (!resident) {
        // #107 opens these documents with the previous engine as a whole.
        if (!created) return {location, document->errorMessage()};
        if (document->image()->gpuImage())
            return {location, QStringLiteral("Excluded colour model was opened by the GPU engine (A1-1)")};
        return {location, {}};
    }
    if (!created) {
        const auto hint = id.startsWith(QLatin1String("rgba-f"))
            ? QStringLiteral(" (F16/F32 RGBA documents are A4 scope)") : QString();
        return {location, document->errorMessage() + hint};
    }
    const auto owner = document->image()->gpuImage();
    if (!owner) return {location, QStringLiteral("RGBA document did not use the GPU engine")};
    const auto expected = id == QLatin1String("rgba-u16") ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    if (owner->operations().snapshot()->format != expected)
        return {location, QStringLiteral("RGBA document used a different GPU pixel format")};
    return {location, {}};
}

Outcome layerType(SmokeDocument &doc, const QString &id)
{
    const bool text = id == QLatin1String("text");
    const bool resident = id == QLatin1String("group") || id == QLatin1String("selection-mask");
    const auto location = resident ? Gpu::ExecutionLocation::Gpu : Gpu::ExecutionLocation::Cpu;
    const auto nodeType = id == QLatin1String("group") ? QStringLiteral("KisGroupLayer") :
        id == QLatin1String("clone") ? QStringLiteral("KisCloneLayer") :
        id == QLatin1String("selection-mask") ? QStringLiteral("KisSelectionMask") :
        id == QLatin1String("filter-mask") ? QStringLiteral("KisFilterMask") :
        id == QLatin1String("colorize-mask") ? QStringLiteral("KisColorizeMask") :
        id == QLatin1String("transform-mask") ? QStringLiteral("KisTransformMask") :
        QStringLiteral("KisShapeLayer");
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *manager = doc.window->viewManager()->nodeManager();
    // Filter masks choose a filter in a dialog unless creation is quiet.
    auto added = id == QLatin1String("filter-mask") ? manager->createNode(nodeType, true) :
        manager->createNode(nodeType);
    doc.image->waitForDone();
    if (!added || !added->parent() || !added->inherits(nodeType.toLatin1().constData()) ||
        doc.image->gpuImage() != doc.owner)
        return {location, text ? QStringLiteral("Text layer (KisShapeLayer with a text shape) was not added to the GPU document") :
                                 QStringLiteral("%1 was not added to the GPU document").arg(nodeType)};
    if (text) {
        // #107 draws text on the CPU when it changes and sends the result to the GPU.
        auto *shapeLayer = qobject_cast<KisShapeLayer *>(added.data());
        std::unique_ptr<KoSvgTextShape> shape(new KoSvgTextShape());
        shape->setPosition(QPointF(4, 4));
        KoSvgTextShapeMarkupConverter converter(shape.get());
        if (!shapeLayer ||
            !converter.convertFromSvg(QStringLiteral("<text>GPU smoke</text>"), QString(),
                QRectF(0, 0, doc.image->width(), doc.image->height()), 72.0) ||
            shape->plainText().isEmpty())
            return {location, QStringLiteral("Text shape could not be created")};
        shapeLayer->addShape(shape.release());
        doc.image->waitForDone();
        if (shapeLayer->shapeCount() != 1)
            return {location, QStringLiteral("Text shape was not retained by the layer")};
    }
    return {location, {}};
}

Outcome adjustmentLayer(SmokeDocument &doc)
{
    const auto location = Gpu::layerExecutionLocation(Gpu::LayerFeature::Adjustment);
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    if (!KisFilterRegistry::instance()->value(QStringLiteral("invert")))
        return {location, QStringLiteral("Adjustment filter was not registered")};
    Filter filter;
    filter.setName(QStringLiteral("invert"));
    Selection selection;
    Document api(doc.document ? doc.document.get() : doc.lifetime.data(), false);
    QScopedPointer<FilterLayer> layer(api.createFilterLayer(
        QStringLiteral("Operation smoke adjustment"), filter, selection));
    if (!layer) return {location, QStringLiteral("Adjustment layer could not be created")};
    const auto before = doc.image->root()->childCount();
    QScopedPointer<Node> root(Node::createNode(doc.image, doc.image->root()));
    if (!root || !root->addChildNode(layer.data(), nullptr))
        return {location, QStringLiteral("Adjustment layer was not added to the GPU document")};
    doc.image->waitForDone();
    if (doc.image->root()->childCount() != before + 1 || doc.image->gpuImage() != doc.owner)
        return {location, QStringLiteral("Adjustment layer was not retained by the GPU document")};
    for (const auto &entry : doc.owner->operations().snapshot()->layers)
        if (entry.name == layer->name())
            return entry.kind == Gpu::LayerKind::Adjustment ? Outcome{location, {}} :
                Outcome{location, QStringLiteral("Adjustment layer has a different GPU layer kind")};
    return {location, QStringLiteral("Added adjustment layer has no GPU version")};
}

Outcome fileLayer(SmokeDocument &doc)
{
    const auto location = Gpu::layerExecutionLocation(Gpu::LayerFeature::File);
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    QTemporaryDir directory;
    if (!directory.isValid()) return {location, QStringLiteral("Could not create file layer fixture")};
    const auto path = directory.filePath(QStringLiteral("operation-smoke-file.png"));
    QImage source(24, 18, QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y)
        for (int x = 0; x < source.width(); ++x)
            source.setPixelColor(x, y, QColor(x * 10, y * 12, 96, 255));
    if (!source.save(path)) return {location, QStringLiteral("Could not save file layer fixture")};
    Document api(doc.document ? doc.document.get() : doc.lifetime.data(), false);
    QScopedPointer<Node> layer(api.createFileLayer(QStringLiteral("Operation smoke file"), path,
        QStringLiteral("None"), QStringLiteral("Bicubic")));
    if (!layer) return {location, QStringLiteral("File layer could not be created")};
    const auto before = doc.image->root()->childCount();
    QScopedPointer<Node> root(Node::createNode(doc.image, doc.image->root()));
    if (!root || !root->addChildNode(layer.data(), nullptr))
        return {location, QStringLiteral("File layer was not added to the GPU document (A9)")};
    doc.image->waitForDone();
    if (doc.image->root()->childCount() != before + 1 || doc.image->gpuImage() != doc.owner)
        return {location, QStringLiteral("File layer was not retained by the GPU document")};
    return {location, {}};
}

Outcome publicApiPixels(bool u16)
{
    const auto location = Gpu::pixelAccessExecutionLocation(Gpu::PixelAccess::PublicApi);
    const auto *colorSpace = u16 ? KoColorSpaceRegistry::instance()->rgb16() :
                                   KoColorSpaceRegistry::instance()->rgb8();
    if (!colorSpace) return {location, QStringLiteral("Public API colour space was not registered")};
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setFileBatchMode(true);
    document->setAutoSaveActive(false);
    if (!document->newImage(QStringLiteral("Operation smoke public API"), 32, 16, colorSpace,
            KoColor(QColor(7, 9, 11), colorSpace), KisConfig::CANVAS_COLOR, 1, {}, 1))
        return {location, document->errorMessage()};
    const auto image = document->image();
    const auto owner = image->gpuImage();
    const auto node = image->root()->firstChild();
    const auto expected = u16 ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    if (!owner || !node) return {location, QStringLiteral("Public API document did not use the GPU engine")};
    if (owner->operations().snapshot()->format != expected)
        return {location, QStringLiteral("Public API document used a different GPU pixel format")};
    QScopedPointer<Node> api(Node::createNode(image, node));
    const int pixelSize = colorSpace->pixelSize();
    QByteArray value(8 * 6 * pixelSize, Qt::Uninitialized);
    for (int i = 0; i < 8 * 6; ++i) {
        auto *bytes = value.data() + i * pixelSize;
        if (u16) {
            const quint16 components[4] = {quint16(1000 + i * 37), quint16(2000 + i * 53),
                quint16(3000 + i * 71), quint16(65535 - i * 17)};
            std::memcpy(bytes, components, sizeof(components));
        } else {
            const quint8 components[4] = {quint8(40 + i * 7), quint8(90 + i * 11),
                quint8(160 + i * 13), 235};
            std::memcpy(bytes, components, sizeof(components));
        }
    }
    if (!api->setPixelData(value, 3, 2, 8, 6) || api->pixelData(3, 2, 8, 6) != value)
        return {location, QStringLiteral("Public API pixel transfer did not preserve the native bytes")};
    // The byte count must describe the whole requested rectangle.
    if (api->setPixelData(value.left(value.size() - pixelSize), 0, 0, 8, 6))
        return {location, QStringLiteral("Public API accepted a pixel buffer smaller than its rectangle")};
    return {location, {}};
}

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

Outcome figure(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::brushExecutionLocation(QStringLiteral("roundmarker"));
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    if (!preset) return {location, QStringLiteral("Figure brush preset was not registered")};
    preset->setName(QStringLiteral("Operation smoke figure"));
    preset->setFilename(QStringLiteral("operation-smoke-figure.kpp"));
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    if (!presets.addResource(preset)) return {location, QStringLiteral("Could not register figure brush preset")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *resources = doc.window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
    resources->setSize(6);
    resources->setOpacity(1);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setFGColor(KoColor(QColor(221, 19, 73), doc.image->colorSpace()));
    auto *canvas = doc.view->canvasBase();
    canvas->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    canvas->snapGuide()->enableSnapping(false);
    const bool line = id == QLatin1String("line");
    const bool rectangle = id == QLatin1String("rectangle") || id == QLatin1String("rounded-rectangle");
    const bool path = id == QLatin1String("bezier");
    const bool vertices = path || id == QLatin1String("polygon") || id == QLatin1String("polyline");
    const auto toolId = line ? QStringLiteral("KritaShape/KisToolLine") :
        rectangle ? QStringLiteral("KritaShape/KisToolRectangle") :
        path ? QStringLiteral("KisToolPath") :
        id == QLatin1String("polygon") ? QStringLiteral("KisToolPolygon") :
        id == QLatin1String("polyline") ? QStringLiteral("KisToolPolyline") : QStringLiteral("KritaShape/KisToolEllipse");
    KoToolManager::instance()->switchToolRequested(toolId);
    auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(canvas, toolId));
    if (!tool || KoToolManager::instance()->activeToolId() != toolId)
        return {location, QStringLiteral("Figure tool could not be activated")};
    const auto cancel = qScopeGuard([&] { tool->requestStrokeCancellation(); });
    if (!line) {
        QComboBox *outline = nullptr, *fill = nullptr;
        for (QWidget *panel : tool->optionWidgets()) {
            if (auto *box = panel->findChild<QComboBox *>(QStringLiteral("cmbOutline"))) outline = box;
            if (auto *box = panel->findChild<QComboBox *>(QStringLiteral("cmbFill"))) fill = box;
        }
        if (!outline || !fill) return {location, QStringLiteral("Figure painting options were not available")};
        outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleForeground);
        fill->setCurrentIndex(KisFigurePaintingOptions::FillStyleNone);
    }
    if (rectangle) {
        const int radius = id == QLatin1String("rounded-rectangle") ? 8 : 0;
        if (!QMetaObject::invokeMethod(tool, "roundCornersChanged", Qt::DirectConnection,
                                      Q_ARG(int, radius), Q_ARG(int, radius)))
            return {location, QStringLiteral("Rectangle corner options were not available")};
    }
    // Drive the primary pointer actions and ordinary finish action, letting
    // tool dispatch and the selected preset choose the drawing implementation.
    const auto send = [&](QEvent::Type type, QPointF point, bool held = false) {
        const auto button = type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton;
        const auto buttons = type == QEvent::MouseButtonPress || held ? Qt::LeftButton : Qt::NoButton;
        QMouseEvent event(type, QPointF(), QPointF(), button, buttons, Qt::NoModifier);
        KoPointerEvent input(&event, canvas->coordinatesConverter()->imageToDocument(point));
        if (type == QEvent::MouseButtonPress) tool->beginPrimaryAction(&input);
        else if (type == QEvent::MouseMove) tool->continuePrimaryAction(&input);
        else tool->endPrimaryAction(&input);
    };
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
    if (vertices) {
        const QList<QPointF> points{{12, 12}, {32, 34}, {52, 12}};
        for (const auto &point : points) {
            send(QEvent::MouseMove, point);
            send(QEvent::MouseButtonPress, point);
            // Dragging a Bezier anchor creates handles, exercising an actual
            // curve rather than the polygon's straight segments.
            if (path) send(QEvent::MouseMove, point + QPointF(6, 0), true);
            send(QEvent::MouseButtonRelease, path ? point + QPointF(6, 0) : point);
        }
        tool->requestStrokeEnd();
    } else {
        send(QEvent::MouseButtonPress, {12, 10});
        send(QEvent::MouseMove, {50, 36}, true);
        if (line) send(QEvent::MouseButtonRelease, {50, 36});
        // Rectangle-based tools cancel synthetic releases, which also occur
        // on focus loss. Their finish action commits this synthetic gesture.
        else tool->requestStrokeEnd();
    }
    doc.image->waitForDone();
    const auto after = doc.pixels();
    if (doc.image->gpuImage() != doc.owner || before.size() != 64 * 48 * 4 ||
        after.size() != before.size() || after == before || doc.owner->operations().undoCount() != history + 1) {
        QString detail = QStringLiteral("Figure tool did not publish changed pixels and one history entry");
        for (const auto &error : errors)
            if (!error.first().toString().isEmpty()) detail += QStringLiteral("; ") + error.first().toString();
        return {location, detail};
    }
    return {location, {}};
}

void toolPointer(KisTool *tool, KisCanvas2 *canvas, QEvent::Type type, QPointF point, bool held = false)
{
    const auto button = type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton;
    const auto buttons = type == QEvent::MouseButtonPress || held ? Qt::LeftButton : Qt::NoButton;
    const auto widgetPoint = canvas->coordinatesConverter()->imageToWidget(point);
    QMouseEvent event(type, widgetPoint, widgetPoint, button, buttons, Qt::NoModifier);
    KoPointerEvent input(&event, canvas->coordinatesConverter()->imageToDocument(point));
    if (type == QEvent::MouseButtonPress) tool->beginPrimaryAction(&input);
    else if (type == QEvent::MouseButtonRelease) tool->endPrimaryAction(&input);
    else if (held) tool->continuePrimaryAction(&input);
    else tool->mouseMoveEvent(&input);
}

Outcome moveOrTransform(SmokeDocument &doc, bool transform)
{
    const auto location = Gpu::ExecutionLocation::Gpu;
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *canvas = doc.view->canvasBase();
    canvas->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    canvas->snapGuide()->enableSnapping(false);
    const auto toolId = transform ? QStringLiteral("KisToolTransform") : QStringLiteral("KritaTransform/KisToolMove");
    const auto source = doc.owner->operations().snapshot();
    const auto layer = std::find_if(source->layers.cbegin(), source->layers.cend(),
        [&](const auto &entry) { return entry.uuid == doc.node->uuid(); });
    if (layer == source->layers.cend()) return {location, QStringLiteral("Tool target has no GPU layer")};
    const auto read = [&](const Gpu::Snapshot &version) {
        const auto result = doc.owner->execution().readRgbaPixels(doc.owner->handle(), version,
            doc.image->bounds(), layer->id, 64 * 48 * 4);
        return result.code == Gpu::ExecutionCode::Success ? result.bytes : QByteArray{};
    };
    const auto before = read(source);
    if (before.size() != 64 * 48 * 4) return {location, QStringLiteral("Could not read the initial tool pixels")};
    const auto history = doc.owner->operations().undoCount();
    // A cancelled preview is the boundary case for the ordinary gesture. A
    // fresh activation then exercises the same gesture through its finish action.
    for (const bool cancel : {true, false}) {
        KoToolManager::instance()->switchToolRequested(toolId);
        auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(canvas, toolId));
        if (!tool || KoToolManager::instance()->activeToolId() != toolId)
            return {location, QStringLiteral("Move/transform tool could not be activated")};
        const auto cleanup = qScopeGuard([&] { tool->requestStrokeCancellation(); });
        const auto options = tool->optionWidgets();
        if (transform) {
            QSpinBox *scaleX = nullptr, *scaleY = nullptr;
            for (QWidget *panel : options) {
                if (auto *spin = panel->findChild<QSpinBox *>(QStringLiteral("scaleXBox"))) scaleX = spin;
                if (auto *spin = panel->findChild<QSpinBox *>(QStringLiteral("scaleYBox"))) scaleY = spin;
            }
            if (!scaleX || !scaleY) return {location, QStringLiteral("Transform scale options were not available")};
            scaleX->setValue(125);
            scaleY->setValue(75);
        } else {
            QRadioButton *selected = nullptr;
            for (QWidget *panel : options)
                if (auto *button = panel->findChild<QRadioButton *>(QStringLiteral("radioSelectedLayer"))) selected = button;
            if (!selected) return {location, QStringLiteral("Move layer options were not available")};
            selected->setChecked(true);
            toolPointer(tool, canvas, QEvent::MouseMove, {20, 16});
            toolPointer(tool, canvas, QEvent::MouseButtonPress, {20, 16});
            toolPointer(tool, canvas, QEvent::MouseMove, {27, 21}, true);
            toolPointer(tool, canvas, QEvent::MouseButtonRelease, {27, 21});
        }
        // waitForDone() requests stroke completion; render the preview directly
        // while the ordinary move/transform gesture is still open.
        const auto preview = doc.owner->operations().preview();
        const auto pixels = read(preview);
        if (!doc.owner->operations().hasActiveStroke() || preview == source ||
            doc.owner->operations().snapshot() != source || doc.owner->operations().undoCount() != history ||
            pixels.size() != before.size() || pixels == before)
            return {location, QStringLiteral("Move/transform did not publish a changed preview before history commit (A7): active=%1 previewChanged=%2 sourceChanged=%3 history=%4/%5 bytes=%6 changed=%7")
                .arg(doc.owner->operations().hasActiveStroke()).arg(preview != source)
                .arg(doc.owner->operations().snapshot() != source).arg(doc.owner->operations().undoCount()).arg(history)
                .arg(pixels.size()).arg(pixels != before)};
        if (cancel) {
            tool->requestStrokeCancellation();
            // Transform cancellation resets its handles for another edit.
            // Leaving the tool then closes that unchanged editing session.
            KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
            if (doc.owner->operations().hasActiveStroke() || doc.owner->operations().snapshot() != source ||
                read(doc.owner->operations().snapshot()) != before || doc.owner->operations().undoCount() != history)
                return {location, QStringLiteral("Cancelled move/transform changed pixels or history (A7)")};
        } else {
            tool->requestStrokeEnd();
            doc.image->waitForDone();
            if (doc.image->gpuImage() != doc.owner || doc.owner->operations().hasActiveStroke() ||
                doc.owner->operations().undoCount() != history + 1 || read(doc.owner->operations().snapshot()) != pixels)
                return {location, QStringLiteral("Move/transform did not commit its preview as one history entry (A7)")};
        }
    }
    return {location, {}};
}

Outcome cpuTool(SmokeDocument &doc, Gpu::ToolOperation operation)
{
    const auto location = Gpu::toolExecutionLocation(operation);
    const bool fill = operation == Gpu::ToolOperation::Fill;
    const bool gradient = operation == Gpu::ToolOperation::Gradient;
    const bool patch = operation == Gpu::ToolOperation::SmartPatch;
    const auto toolId = fill ? QStringLiteral("KritaFill/KisToolFill") :
        gradient ? QStringLiteral("KritaFill/KisToolGradient") :
        patch ? QStringLiteral("KritaShape/KisToolSmartPatch") : QStringLiteral("KisToolEncloseAndFill");
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("whatToFill", "fillContiguousRegion");
    configuration.writeEntry("contiguousFillMode", "floodFill");
    configuration.writeEntry("fillWith", "foregroundColor");
    configuration.writeEntry("sampleLayersMode", "currentLayer");
    configuration.writeEntry("thresholdAmount", 8);
    configuration.writeEntry("opacitySpread", 100);
    configuration.writeEntry("closeGapAmount", 0);
    configuration.writeEntry("antiAlias", false);
    configuration.writeEntry("growSelection", 0);
    configuration.writeEntry("featherAmount", 0);
    configuration.writeEntry("useCustomBlendingOptions", false);
    configuration.writeEntry("shape", 0);
    configuration.writeEntry("repeat", 0);
    configuration.writeEntry("reverse", false);
    configuration.writeEntry("dither", false);
    configuration.writeEntry("enclosingMethod", "rectangle");
    configuration.writeEntry("regionSelectionMethod", "allRegions");
    configuration.writeEntry("regionSelectionIncludeContourRegions", true);
    configuration.writeEntry("regionSelectionInvert", false);
    configuration.writeEntry("reference", "currentLayer");
    configuration.writeEntry("fillThreshold", 8);
    configuration.writeEntry("fillOpacitySpread", 100);
    configuration.writeEntry("expand", 0);
    configuration.writeEntry("feather", 0);
    auto preset = KisPaintOpRegistry::instance()->defaultPreset(KoID("roundmarker"), KisGlobalResourcesInterface::instance());
    if (!preset) return {location, QStringLiteral("Tool brush preset was not registered")};
    preset->setName(QStringLiteral("Operation smoke CPU tool"));
    preset->setFilename(QStringLiteral("operation-smoke-cpu-tool.kpp"));
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    if (!presets.addResource(preset)) return {location, QStringLiteral("Could not register tool brush preset")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *resources = doc.window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
    resources->setSize(6);
    resources->setOpacity(1);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setFGColor(KoColor(QColor(221, 19, 73), doc.image->colorSpace()));
    resources->setBGColor(KoColor(Qt::white, doc.image->colorSpace()));
    if (gradient) {
        auto resource = QSharedPointer<KoStopGradient>::create();
        resource->setName(QStringLiteral("Operation smoke tool gradient"));
        resource->setStops({{0, KoColor(Qt::red, doc.image->colorSpace())},
                            {1, KoColor(Qt::blue, doc.image->colorSpace())}});
        resource->setValid(true);
        resources->slotGradientActivated(resource);
    }
    auto *canvas = doc.view->canvasBase();
    canvas->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    canvas->snapGuide()->enableSnapping(false);
    KoToolManager::instance()->switchToolRequested(toolId);
    auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(canvas, toolId));
    if (!tool || KoToolManager::instance()->activeToolId() != toolId)
        return {location, QStringLiteral("CPU tool could not be activated")};
    const auto cleanup = qScopeGuard([&] { tool->requestStrokeCancellation(); });
    const auto options = tool->optionWidgets();
    if (patch) {
        QSlider *accuracy = nullptr;
        QSpinBox *radius = nullptr;
        for (QWidget *panel : options) {
            if (auto *slider = panel->findChild<QSlider *>(QStringLiteral("accuracySlider"))) accuracy = slider;
            if (auto *spin = panel->findChild<QSpinBox *>(QStringLiteral("patchRadius"))) radius = spin;
        }
        if (!accuracy || !radius) return {location, QStringLiteral("Smart patch options were not available")};
        accuracy->setValue(0);
        radius->setValue(2);
        std::srand(197);
    }
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    toolPointer(tool, canvas, QEvent::MouseMove, {16, 14});
    toolPointer(tool, canvas, QEvent::MouseButtonPress, {16, 14});
    if (!fill) toolPointer(tool, canvas, QEvent::MouseMove, patch ? QPointF(20, 18) : QPointF(48, 34), true);
    // Rectangle delegates treat synthetic releases as cancellation; the finish
    // action commits the enclosing shape before sending its pointer release.
    if (!fill && !gradient && !patch) tool->requestStrokeEnd();
    toolPointer(tool, canvas, QEvent::MouseButtonRelease, fill ? QPointF(16, 14) : patch ? QPointF(20, 18) : QPointF(48, 34));
    doc.image->waitForDone();
    const auto after = doc.pixels();
    if (doc.image->gpuImage() != doc.owner || doc.owner->operations().hasActiveStroke() ||
        before.size() != 64 * 48 * 4 || after.size() != before.size() || after == before ||
        doc.owner->operations().undoCount() != history + 1)
        return {location, QStringLiteral("CPU tool did not publish changed pixels and one history entry in the GPU document (A7)")};
    return {location, {}};
}

QByteArray selectionCoverage(const SmokeDocument &doc)
{
    QByteArray bytes(64 * 48, '\0');
    if (const auto selection = doc.image->globalSelection())
        selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), doc.image->bounds());
    return bytes;
}

Outcome selectionTool(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::ExecutionLocation::Gpu;
    const auto combination = id == QLatin1String("add") ? SELECTION_ADD :
        id == QLatin1String("subtract") ? SELECTION_SUBTRACT :
        id == QLatin1String("intersect") ? SELECTION_INTERSECT :
        id == QLatin1String("symmetric-difference") ? SELECTION_SYMMETRICDIFFERENCE : SELECTION_REPLACE;
    const bool rectangle = id == QLatin1String("rectangle") || id == QLatin1String("vector-rectangle") ||
        combination != SELECTION_REPLACE;
    const bool ellipse = id == QLatin1String("ellipse");
    const bool outline = id == QLatin1String("freehand");
    const bool bezier = id == QLatin1String("bezier");
    const bool magnetic = id == QLatin1String("magnetic");
    const bool color = id == QLatin1String("contiguous") || id == QLatin1String("similar");
    const auto toolId = rectangle ? QStringLiteral("KisToolSelectRectangular") :
        ellipse ? QStringLiteral("KisToolSelectElliptical") :
        outline ? QStringLiteral("KisToolSelectOutline") :
        bezier ? QStringLiteral("KisToolSelectPath") :
        magnetic ? QStringLiteral("KisToolSelectMagnetic") :
        id == QLatin1String("contiguous") ? QStringLiteral("KisToolSelectContiguous") :
        id == QLatin1String("similar") ? QStringLiteral("KisToolSelectSimilar") : QStringLiteral("KisToolSelectPolygonal");
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("roundCornersX", 0);
    configuration.writeEntry("roundCornersY", 0);
    configuration.writeEntry("closeGapAmount", 0);
    configuration.writeEntry("useSelectionAsBoundary", false);
    configuration.writeEntry("contiguousSelectionMode", "floodFill");
    configuration.writeEntry("filterradius", 3.0);
    configuration.writeEntry("searchradius", 20);
    configuration.writeEntry("anchorgap", 20);
    configuration.writeEntry("threshold", magnetic ? 100 : 8);
    configuration.writeEntry("opacitySpread", 100);
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *canvas = doc.view->canvasBase();
    canvas->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    canvas->snapGuide()->enableSnapping(false);
    KoToolManager::instance()->switchToolRequested(toolId);
    auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(canvas, toolId));
    if (!tool || KoToolManager::instance()->activeToolId() != toolId)
        return {location, QStringLiteral("Selection tool could not be activated")};
    const auto cancel = qScopeGuard([&] { tool->requestStrokeCancellation(); });
    KisSelectionOptions *options = nullptr;
    for (QWidget *panel : tool->optionWidgets())
        if (auto *candidate = qobject_cast<KisSelectionOptions *>(panel)) options = candidate;
    if (!options) return {location, QStringLiteral("Selection tool options were not available")};
    options->setMode(id == QLatin1String("vector-rectangle") ? SHAPE_PROTECTION : PIXEL_SELECTION);
    options->setAction(combination);
    options->setAntiAliasSelection(false);
    options->setGrowSelection(0);
    options->setFeatherSelection(0);
    options->setStopGrowingAtDarkestPixel(false);
    options->setReferenceLayers(KisSelectionOptions::CurrentLayer);
    if (color) {
        auto *threshold = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderThreshold"));
        auto *spread = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderSpread"));
        if (!threshold || !spread) return {location, QStringLiteral("Colour selection options were not available")};
        threshold->setValue(8);
        spread->setValue(100);
    }
    if (combination != SELECTION_REPLACE && !doc.image->setGpuSelectionRect({24, 10, 26, 24}))
        return {location, QStringLiteral("Could not initialize the selection combination")};
    const auto before = selectionCoverage(doc);
    const auto pixels = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    const auto send = [&](QEvent::Type type, QPointF point, bool held = false) {
        const auto button = type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton;
        const auto buttons = type == QEvent::MouseButtonPress || held ? Qt::LeftButton : Qt::NoButton;
        QMouseEvent event(type, QPointF(), QPointF(), button, buttons, Qt::NoModifier);
        KoPointerEvent input(&event, canvas->coordinatesConverter()->imageToDocument(point));
        if (type == QEvent::MouseButtonPress) tool->beginPrimaryAction(&input);
        else if (type == QEvent::MouseMove) tool->continuePrimaryAction(&input);
        else tool->endPrimaryAction(&input);
    };
    if (rectangle || ellipse) {
        send(QEvent::MouseButtonPress, {10, 18});
        send(QEvent::MouseMove, {36, 38}, true);
        tool->requestStrokeEnd();
    } else if (color) {
        send(QEvent::MouseButtonPress, {22, 22});
        send(QEvent::MouseButtonRelease, {22, 22});
    } else {
        const QList<QPointF> points{{12, 10}, {50, 14}, {32, 38}};
        if (outline) {
            send(QEvent::MouseButtonPress, points.first());
            for (qsizetype i = 1; i < points.size(); ++i) send(QEvent::MouseMove, points[i], true);
            // Like rectangle tools, freehand outlines cancel synthetic
            // releases. The normal finish action commits the held gesture.
            tool->requestStrokeEnd();
        } else {
            for (const auto &point : points) {
                send(QEvent::MouseMove, point);
                send(QEvent::MouseButtonPress, point);
                if (bezier) send(QEvent::MouseMove, point + QPointF(6, 0), true);
                send(QEvent::MouseButtonRelease, bezier ? point + QPointF(6, 0) : point);
            }
            tool->requestStrokeEnd();
        }
    }
    doc.image->waitForDone();
    const auto after = selectionCoverage(doc);
    if (doc.image->gpuImage() != doc.owner || before == after ||
        !doc.image->globalSelection() || doc.image->globalSelection()->selectedExactRect().isEmpty() ||
        doc.pixels() != pixels || doc.owner->operations().undoCount() != history + 1)
        return {location, QStringLiteral("Selection tool did not publish changed coverage and one history entry (A7)")};
    if (id == QLatin1String("vector-rectangle") && !doc.image->globalSelection()->hasShapeSelection())
        return {location, QStringLiteral("Vector selection mode published raster coverage without editable shapes (A7)")};
    if (combination != SELECTION_REPLACE) {
        for (int y = 0; y < 48; ++y) for (int x = 0; x < 64; ++x) {
            const bool old = QRect(24, 10, 26, 24).contains(x, y);
            const bool added = QRect(10, 18, 26, 20).contains(x, y);
            const bool expected = combination == SELECTION_ADD ? old || added :
                combination == SELECTION_SUBTRACT ? old && !added :
                combination == SELECTION_INTERSECT ? old && added : old != added;
            if (quint8(after[y * 64 + x]) != (expected ? 255 : 0))
                return {location, QStringLiteral("Selection combination did not preserve the requested coverage")};
        }
    }
    return {location, {}};
}

Outcome selectionEdit(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::ExecutionLocation::Gpu;
    const bool filtered = id == QLatin1String("growselection") || id == QLatin1String("shrinkselection") ||
        id == QLatin1String("borderselection") || id == QLatin1String("featherselection") ||
        id == QLatin1String("smoothselection") || id == QLatin1String("shrink-empty");
    const bool freehand = id == QLatin1String("borderselection") || id == QLatin1String("featherselection") ||
        id == QLatin1String("smoothselection");
    const auto setup = selectionTool(doc, freehand ? QStringLiteral("polygon") : QStringLiteral("rectangle"));
    if (!setup.error.isEmpty()) return setup;
    const auto initial = selectionCoverage(doc);
    auto *manager = doc.window->viewManager()->actionManager();
    manager->updateGUI();
    if (id == QLatin1String("reselect")) {
        auto *deselect = manager->actionByName(QStringLiteral("deselect"));
        if (!deselect) return {location, QStringLiteral("Deselect action was not registered")};
        deselect->trigger();
        doc.image->waitForDone();
        if (doc.image->globalSelection()) return {location, QStringLiteral("Deselect did not remove the active selection")};
        manager->updateGUI();
    }
    const auto before = selectionCoverage(doc);
    const auto pixels = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    if (filtered) {
        const auto operation = id == QLatin1String("shrink-empty") ? QStringLiteral("shrinkselection") : id;
        if (!manager->actionByName(operation)) return {location, QStringLiteral("Selection filter was not registered")};
        KisOperationConfigurationSP config = new KisOperationConfiguration(operation);
        const int radius = id == QLatin1String("shrink-empty") ? 64 : 3;
        config->setProperty("x-radius", radius);
        config->setProperty("y-radius", radius);
        config->setProperty("radius", radius);
        config->setProperty("edgeLock", false);
        config->setProperty("antialiasing", false);
        manager->runOperationFromConfiguration(config);
    } else {
        auto *action = manager->actionByName(id);
        if (!action) return {location, QStringLiteral("Selection edit action was not registered")};
        action->trigger();
    }
    doc.image->waitForDone();
    const auto after = selectionCoverage(doc);
    if (doc.image->gpuImage() != doc.owner || after == before || doc.pixels() != pixels ||
        doc.owner->operations().undoCount() != history + 1)
        return {location, QStringLiteral("Selection edit did not publish changed coverage and one history entry (A7)")};
    if ((id == QLatin1String("deselect") || id == QLatin1String("shrink-empty")) && doc.image->globalSelection())
        return {location, QStringLiteral("Selection edit did not remove the empty selection")};
    if (id == QLatin1String("select_all") && after != QByteArray(64 * 48, char(255)))
        return {location, QStringLiteral("Select all did not cover the canvas")};
    if (id == QLatin1String("reselect") && after != initial)
        return {location, QStringLiteral("Reselect did not restore the previous coverage")};
    if (id == QLatin1String("invert_selection")) {
        for (qsizetype i = 0; i < before.size(); ++i)
            if (quint8(after[i]) != 255 - quint8(before[i]))
                return {location, QStringLiteral("Invert selection did not complement coverage")};
    }
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
                       ColorFilters, ResourceFilters, Generators, Figures, Selections, SelectionEdits, Tools };
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

    void figures()
    {
        runOperations(Group::Figures);
    }

    void selections()
    {
        runOperations(Group::Selections);
    }

    void selectionEdits()
    {
        runOperations(Group::SelectionEdits);
    }

    void tools()
    {
        runOperations(Group::Tools);
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
            {QStringLiteral("document/new-rgba-u8"), true,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("rgba-u8")); }},
            {QStringLiteral("document/new-rgba-u16"), true,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("rgba-u16")); }},
            {QStringLiteral("document/new-rgba-f16"), true,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("rgba-f16")); }},
            {QStringLiteral("document/new-rgba-f32"), true,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("rgba-f32")); }},
            {QStringLiteral("document/new-cmyka-u8"), false,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("cmyka-u8")); }},
            {QStringLiteral("document/new-laba-u8"), false,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("laba-u8")); }},
            {QStringLiteral("document/new-ycbcr-u8"), false,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("ycbcr-u8")); }},
            {QStringLiteral("document/new-xyza-u8"), false,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("xyza-u8")); }},
            {QStringLiteral("document/new-graya-u8"), false,
                [](SmokeDocument &) { return documentFormat(QStringLiteral("graya-u8")); }},
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
            {QStringLiteral("layer/group"), true,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("group")); }},
            {QStringLiteral("layer/clone"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("clone")); }},
            {QStringLiteral("layer/vector"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("vector")); }},
            {QStringLiteral("layer/text"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("text")); }},
            {QStringLiteral("layer/adjustment"), false,
                [](SmokeDocument &doc) { return adjustmentLayer(doc); }},
            {QStringLiteral("layer/file"), false,
                [](SmokeDocument &doc) { return fileLayer(doc); }},
            {QStringLiteral("mask/selection"), true,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("selection-mask")); }},
            {QStringLiteral("mask/filter"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("filter-mask")); }},
            {QStringLiteral("mask/colorize"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("colorize-mask")); }},
            {QStringLiteral("mask/transform"), false,
                [](SmokeDocument &doc) { return layerType(doc, QStringLiteral("transform-mask")); }},
            {QStringLiteral("api/native-pixels"), false, [](SmokeDocument &doc) {
                QScopedPointer<Node> api(Node::createNode(doc.image, doc.node));
                const auto value = QByteArray::fromHex("112233ad");
                if (!api->setPixelData(value, 17, 13, 1, 1) || api->pixelData(17, 13, 1, 1) != value)
                    return Outcome{Gpu::ExecutionLocation::Cpu, QStringLiteral("Public native pixel transfer failed")};
                return Outcome{Gpu::pixelAccessExecutionLocation(Gpu::PixelAccess::PublicApi), {}};
            }},
            {QStringLiteral("api/native-pixels-u16"), false, [](SmokeDocument &) {
                return publicApiPixels(true);
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
        for (const auto &id : {QStringLiteral("line"), QStringLiteral("ellipse"), QStringLiteral("rectangle"),
                              QStringLiteral("rounded-rectangle"), QStringLiteral("polygon"),
                              QStringLiteral("polyline"), QStringLiteral("bezier")})
            probes.append({QStringLiteral("tool/") + id, true,
                [id](SmokeDocument &doc) { return figure(doc, id); }, Group::Figures});
        for (const auto &id : {QStringLiteral("rectangle"), QStringLiteral("ellipse"), QStringLiteral("freehand"),
                              QStringLiteral("polygon"), QStringLiteral("bezier"), QStringLiteral("contiguous"),
                              QStringLiteral("similar"), QStringLiteral("magnetic"), QStringLiteral("vector-rectangle"),
                              QStringLiteral("add"), QStringLiteral("subtract"), QStringLiteral("intersect"),
                              QStringLiteral("symmetric-difference")})
            probes.append({QStringLiteral("selection/") + id, true,
                [id](SmokeDocument &doc) { return selectionTool(doc, id); }, Group::Selections});
        for (const auto &id : {QStringLiteral("select_all"), QStringLiteral("deselect"), QStringLiteral("reselect"),
                              QStringLiteral("invert_selection"), QStringLiteral("growselection"), QStringLiteral("shrinkselection"),
                              QStringLiteral("borderselection"), QStringLiteral("featherselection"),
                              QStringLiteral("smoothselection"), QStringLiteral("shrink-empty")})
            probes.append({QStringLiteral("selection/") + id, true,
                [id](SmokeDocument &doc) { return selectionEdit(doc, id); }, Group::SelectionEdits});
        probes.append({QStringLiteral("tool/transform"), true,
            [](SmokeDocument &doc) { return moveOrTransform(doc, true); }, Group::Tools});
        probes.append({QStringLiteral("tool/move"), true,
            [](SmokeDocument &doc) { return moveOrTransform(doc, false); }, Group::Tools});
        for (const auto &[id, operation] : {
                std::pair{QStringLiteral("fill"), Gpu::ToolOperation::Fill},
                std::pair{QStringLiteral("gradient"), Gpu::ToolOperation::Gradient},
                std::pair{QStringLiteral("smart-patch"), Gpu::ToolOperation::SmartPatch},
                std::pair{QStringLiteral("enclose-and-fill"), Gpu::ToolOperation::EncloseAndFill}})
            probes.append({QStringLiteral("tool/") + id, false,
                [operation](SmokeDocument &doc) { return cpuTool(doc, operation); }, Group::Tools});
        probes.erase(std::remove_if(probes.begin(), probes.end(), [group](const Probe &probe) {
            return probe.group != group;
        }), probes.end());
        static constexpr const char *functions[] = {"normalOperations", "brushes", "filters",
            "spatialFilters", "reliefFilters", "artisticFilters", "colorFilters", "resourceFilters", "generators", "figures",
            "selections", "selectionEdits", "tools"};
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
            const auto bytes = QJsonDocument(QJsonObject{{"schema", 1}, {"scope", "A2b4b"}, {"rows", rows}}).toJson();
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
