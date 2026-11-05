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
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <input/ui/kis_input_profile_manager.h>
#include <nodes/kis_node_manager.h>
#include <opengl/kis_opengl.h>
#include <tool/kis_tool_freehand_helper.h>
#include <kis_painting_information_builder.h>
#include <kis_smoothing_options.h>
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
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceStorage.h>
#include <KisResourceModel.h>
#include <KoCanvasResourcesIds.h>
#include <KoColorProfile.h>
#include <KoResourcePaths.h>
#include <Node.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <functional>
#include <exception>
#include <memory>

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
    KisPaintOpPresetSP preset(new KisPaintOpPreset(QStringLiteral(KRITA_SOURCE_DIR "/plugins/paintops/defaultpresets/") + id + QStringLiteral(".kpp")));
    if (!preset->load(KisGlobalResourcesInterface::instance()) || !preset->valid())
        return {location, QStringLiteral("Default brush preset could not be loaded")};
    preset->setName(QStringLiteral("Operation smoke ") + id);
    preset->setFilename(QStringLiteral("operation-smoke-") + id + QStringLiteral(".kpp"));
    QImage preview(16, 16, QImage::Format_ARGB32);
    preview.fill(Qt::black);
    preset->setImage(preview);
    KisResourceModel presets(ResourceType::PaintOpPresets);
    if (!presets.addResource(preset)) return {location, QStringLiteral("Could not register brush preset")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto *resources = doc.window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(preset);
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
    KisPaintInformation first(QPointF(16.5, 16.5)), last(QPointF(43.5, 28.5));
    helper.initPaintImpl(0, first, doc.view->canvasBase()->resourceManager(), doc.image, doc.node, doc.image.data());
    if (!helper.isRunning()) return {location, QStringLiteral("Brush stroke did not start")};
    helper.paintAt(first);
    helper.paintLine(first, last);
    helper.endPaint();
    doc.image->waitForDone();
    if (!errors.isEmpty()) return {location, errors.first().first().toString()};
    if (doc.owner->operations().undoCount() != history + 1 || doc.pixels() == before)
        return {location, QStringLiteral("Brush did not publish changed pixels and one history entry")};
    return {location, {}};
}

Outcome filter(SmokeDocument &doc, const QString &id)
{
    const auto location = Gpu::filterExecutionLocation(id);
    const auto implementation = KisFilterRegistry::instance()->value(id);
    if (!implementation) return {location, QStringLiteral("Filter plugin was not registered")};
    if (!doc.openView()) return {location, QStringLiteral("Could not open the document view")};
    auto config = implementation->defaultConfiguration(KisGlobalResourcesInterface::instance());
    if (id == QLatin1String("posterize")) config->setProperty("steps", 4);
    const auto before = doc.pixels();
    const auto history = doc.owner->operations().undoCount();
    doc.window->viewManager()->filterManager()->apply(config);
    doc.image->waitForDone();
    if (doc.owner->operations().undoCount() != history + 1 || doc.pixels() == before)
        return {location, QStringLiteral("Filter did not publish changed pixels and one history entry")};
    return {location, {}};
}
}

class KisGpuOperationSmokeTest : public QObject
{
    Q_OBJECT
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
        };
        const QList<Probe> probes = {
            {QStringLiteral("document/new-rgba-u8"), true, [](SmokeDocument &) { return Outcome{}; }},
            {QStringLiteral("brush/paintbrush"), true, [](SmokeDocument &doc) { return brush(doc, QStringLiteral("paintbrush")); }},
            {QStringLiteral("brush/roundmarker"), true, [](SmokeDocument &doc) { return brush(doc, QStringLiteral("roundmarker")); }},
            {QStringLiteral("brush/colorsmudge"), true, [](SmokeDocument &doc) { return brush(doc, QStringLiteral("colorsmudge")); }},
            {QStringLiteral("brush/gridbrush"), false, [](SmokeDocument &doc) { return brush(doc, QStringLiteral("gridbrush")); }},
            {QStringLiteral("filter/invert"), true, [](SmokeDocument &doc) { return filter(doc, QStringLiteral("invert")); }},
            {QStringLiteral("filter/posterize"), false, [](SmokeDocument &doc) { return filter(doc, QStringLiteral("posterize")); }},
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
        QJsonArray rows;
        for (const auto &probe : probes)
            rows.append(QJsonObject{{"id", probe.id}, {"required", probe.resident ? "gpu" : "cpu"},
                {"result", "failure"}, {"detail", "Operation did not finish"}});
        const auto publish = [&] {
            const auto path = qEnvironmentVariable("LIBREPAINT_GPU_COVERAGE_FILE");
            if (path.isEmpty()) return true;
            QSaveFile file(path);
            const auto bytes = QJsonDocument(QJsonObject{{"schema", 1}, {"scope", "A2a"}, {"rows", rows}}).toJson();
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
                process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("normalOperations"), QStringLiteral("-nocrashhandler")});
                const bool finished = process.waitForFinished(20000);
                if (!finished) {
                    process.kill();
                    process.waitForFinished(5000);
                }
                QFile file(report);
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
