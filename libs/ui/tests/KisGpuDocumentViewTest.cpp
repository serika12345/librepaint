/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QApplication>
#include <QClipboard>
#include <kis_clipboard.h>
#include <selection/kis_selection_manager.h>
#include <QBitArray>
#include <QCheckBox>
#include <QColorSpace>
#include <QMouseEvent>
#include <QDockWidget>
#include <QEnterEvent>
#include <QListView>
#include <QItemSelectionModel>
#include <QIcon>
#include <QPointer>
#include <QPainter>
#include <QSignalSpy>
#include <QStatusBar>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTreeView>
#include <QTimer>
#include <QSettings>
#include <QScreen>
#include <application/kis_config.h>
#include <kis_config_notifier.h>
#include <QFile>
#include <kra/KisKraRasterCodec.h>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KisSynchronizedConnection.h>
#include <KisGlobalResourcesInterface.h>
#include <KisResourceLocator.h>
#include <KisResourceModel.h>
#include <KisResourceStorage.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <actions/input/KisApplicationInputActions.h>
#include <operations/kis_operation_configuration.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/kis_zoom_manager.h>
#include <input/ui/kis_input_profile_manager.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <KoToolManager.h>
#include <KoSnapGuide.h>
#include <KoCanvasResourcesIds.h>
#include <KoToolBase.h>
#include <KoColorSpaceRegistry.h>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_image_config.h>
#include <kis_node.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_properties_configuration.h>
#include <kis_selection.h>
#include <kis_tool_utils.h>
#include <nodes/kis_node_manager.h>
#include <nodes/kis_node_filter_proxy_model.h>
#include <nodes/kis_node_model.h>
#include <kis_canvas2.h>
#include <kundo2stack.h>
#include <kundo2magicstring.h>
#include <brushengine/kis_paintop_registry.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <thread>

class KisGpuDocumentViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setFallbackSearchPaths({QStringLiteral(":/icons")});
        KLocalizedString::setApplicationDomain("krita");
        KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
        KisInputProfileManager::instance()->setActions(createApplicationInputActions());
        const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/libs/resources/tests/data/bundles/test1.bundle");
        auto bundle = KisResourceStorageSP::create(bundlePath);
        QVERIFY(bundle->valid());
        QVERIFY(KisResourceLocator::instance()->addStorage(bundlePath, bundle));
        auto preset = KisPaintOpRegistry::instance()->defaultPreset(
            KoID("roundmarker"), KisGlobalResourcesInterface::instance());
        QVERIFY(preset);
        preset->setName(QStringLiteral("GPU document view fixture"));
        preset->setFilename(QStringLiteral("gpu-document-view-fixture.kpp"));
        QImage preview(16, 16, QImage::Format_ARGB32);
        preview.fill(Qt::black);
        preset->setImage(preview);
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
        m_preset = preset;
    }
    void presentsEditsAndReleasesTheClosedTab();
    void brushOutlineFollowsTheOrdinaryPointerWithoutEditing();
    void cropPreviewPreservesPixelsHistoryAndCancellation();
    void canvasBackgroundFollowsPreferencesWithoutEditing_data();
    void canvasBackgroundFollowsPreferencesWithoutEditing();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled();
    void paintingBackendChangesRebindTheCanvas_data();
    void paintingBackendChangesRebindTheCanvas();
    void replacementUpdatesPresentationInputHistoryAndSaving_data();
    void replacementUpdatesPresentationInputHistoryAndSaving();
    void layerPanelEditsUseOneDocumentHistory_data();
    void layerPanelEditsUseOneDocumentHistory();
    void layerPanelThumbnailsReadOnlyReducedGpuPixels();
    void rectangularSelectionLimitsPainting();
    void selectionFillUsesGpuSelection();
    void selectionMergedCopyUsesGpuSelection();
    void selectionCopyAndCutUseGpuSelection();
    void selectionGrowAndShrinkUseGpuSelection();
    void invertedSelectionLimitsPaintingAndKra();
    void ellipticalSelectionLimitsPaintingAndKra();
    void ellipticalToolCreatesDocumentSelection();
    void freehandToolCreatesArbitraryMaskSelection();
    void pasteClipboardImageCreatesGpuLayer();
    void reselectRestoresGpuSelection();
    void arrowKeysNudgeGpuSelection();
    void copySelectionToNewLayerUsesGpuPixels();
    void selectionMovePreviewsAndCommitsOneHistory();
    void ordinaryLayerChangesDuringInput();
    void lineToolPreservesPreviewCancellationAndHistory_data();
    void lineToolPreservesPreviewCancellationAndHistory();
    void layerPanelReordersSelectionWithOneHistoryEntry_data();
    void layerPanelReordersSelectionWithOneHistoryEntry();
    void layerPanelSelectionSurvivesSaving();
    void layerPanelSelectionPreservesAnUnselectedActiveLayer_data();
    void layerPanelSelectionPreservesAnUnselectedActiveLayer();
    void layerSelectionCanCloseTheWindowDuringNotification_data();
    void layerSelectionCanCloseTheWindowDuringNotification();
    void layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry_data();
    void layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry();
    void layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry_data();
    void layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry();
    void layerPanelAddsAtSelectionWithOneHistoryEntry_data();
    void layerPanelAddsAtSelectionWithOneHistoryEntry();
    void clearActionPreservesSelectedLayersAndHistory_data();
    void clearActionPreservesSelectedLayersAndHistory();
    void colorFillActionsPreservePixelsHistoryAndSaving_data();
    void colorFillActionsPreservePixelsHistoryAndSaving();
private:
    KisPaintOpPresetSP m_preset;
};

void KisGpuDocumentViewTest::layerPanelThumbnailsReadOnlyReducedGpuPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device, 8 * Gpu::TileRenderer::TileBytes, 1, 4096);
    QByteArray pixels(Gpu::RgbaTile::ByteCount, '\0');
    for (qsizetype i = 0; i < pixels.size(); i += 4) pixels.replace(i, 4, QByteArray::fromHex("316ac2c8"));
    Gpu::RasterImage raster;
    raster.size = {256, 256};
    raster.layers = {{QStringLiteral("Thumbnail"), true, {}, {{{0, 0}, pixels}}}};
    const auto owner = Gpu::PaintImageSP::create(execution, raster);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer thumbnails")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    QTRY_VERIFY(!windowReady.isEmpty());
    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    model->setPreferredThumnalSize(32);
    const auto node = lifetime->image()->root()->firstChild();
    QTRY_VERIFY(proxy->indexFromNode(node).isValid());
    const auto thumbnail = [&] { return proxy->data(proxy->indexFromNode(node), int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>(); };
    QSignalSpy changes(owner.data(), &Gpu::PaintImage::changed);
    const auto before = thumbnail();
    QCOMPARE(before.size(), QSize(32, 32));
    QCOMPARE(before.pixelColor(16, 16), QColor(49, 106, 194, 200));
    QCOMPARE(changes.size(), 0);
    QCOMPARE(owner->operations().snapshot(), saved);
    QCOMPARE(lifetime->undoStack()->count(), 0);
    QCOMPARE(execution->beginReadbackFrame().code, Gpu::ExecutionCode::Success);
    QCOMPARE(thumbnail(), before);
    QCOMPARE(execution->cancelFrame().code, Gpu::ExecutionCode::Success);
    Gpu::Brush brush;
    brush.diameter = 9;
    brush.rgba = {211, 39, 17, 255};
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), brush, 17);
    QCOMPARE(owner->appendSamples(stroke, {{{128.5, 128.5}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    QTRY_COMPARE(thumbnail().pixelColor(16, 16), QColor(211, 39, 17, 255));
    QCOMPARE(thumbnail().pixelColor(31, 31), QColor(49, 106, 194, 200));
    QCOMPARE(lifetime->undoStack()->count(), 1);
    auto *historyDock = window->findChild<QDockWidget*>(QStringLiteral("History"));
    QVERIFY(historyDock);
    auto *historyView = qobject_cast<QListView*>(historyDock->widget());
    QVERIFY(historyView);
    QImage historyImage;
    QTRY_VERIFY(!(historyImage = historyView->model()->data(historyView->model()->index(1, 0), Qt::DecorationRole).value<QImage>()).isNull());
    QCOMPARE(historyImage.pixelColor(historyImage.width() / 2, historyImage.height() / 2), QColor(211, 39, 17, 255));
    const auto after = thumbnail();
    lifetime->undoStack()->undo();
    QTRY_COMPARE(thumbnail(), before);
    QCOMPARE(lifetime->undoStack()->count(), 1);
    QCOMPARE(historyView->model()->data(historyView->model()->index(1, 0), Qt::DecorationRole).value<QImage>(), historyImage);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        QVERIFY(QDir().mkpath(results));
        const QList<QPair<QString, QImage>> images{{QStringLiteral("layer-thumbnail-before"), before},
            {QStringLiteral("layer-thumbnail-after"), after}, {QStringLiteral("history-thumbnail-fixed"), historyImage}};
        for (const auto &entry : images) {
            const auto rgba = entry.second.convertToFormat(QImage::Format_RGBA8888);
            QFile file(QDir(results).filePath(entry.first + QStringLiteral(".rgba")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(reinterpret_cast<const char*>(rgba.constBits()), rgba.sizeInBytes()), rgba.sizeInBytes());
            QVERIFY(entry.second.save(QDir(results).filePath(entry.first + QStringLiteral(".png"))));
        }
    }
}

void KisGpuDocumentViewTest::colorFillActionsPreservePixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<QByteArray>("blank");
    QTest::addColumn<QByteArray>("covered");
    struct Case { const char *name; const char *blank; const char *covered; };
    const Case cases[] = {
        {"foreground", "f18111ff", "f18111ff"},
        {"background", "2c11ddff", "2c11ddff"},
        {"foreground-opacity", "f1811180", "815222ff"},
        {"background-opacity", "2c11dd80", "1f1988ff"},
        {"half-alpha-opacity", "f1811180", "815222ff"},
        {"alpha-lock", "f1811100", "f18111ff"},
        {"layer-alpha-lock", "f1811100", "f18111ff"},
        {"channel-lock", "f18100ff", "f18133ff"},
        {"erase", "00000000", "11223300"},
        {"zero-opacity", "00000000", "112233ff"}
    };
    for (const bool gpu : {false, true}) for (const auto &c : cases) {
        const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") + c.name;
        QTest::newRow(name.constData()) << gpu << QString::fromLatin1(c.name)
            << QByteArray::fromHex(c.blank) << QByteArray::fromHex(c.covered);
    }
    for (const auto &scenario : {"reject-pattern", "reject-blend", "close-during-fill"}) {
        const QByteArray name = QByteArray("gpu-") + scenario;
        QTest::newRow(name.constData()) << true << QString::fromLatin1(scenario)
            << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f18111ff");
    }
}

void KisGpuDocumentViewTest::colorFillActionsPreservePixelsHistoryAndSaving()
{
    QFETCH(bool, gpu);
    QFETCH(QString, scenario);
    QFETCH(QByteArray, blank);
    QFETCH(QByteArray, covered);
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto oldRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), oldRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    namespace Gpu = LibrePaint::Image::Gpu;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Other")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Color fill")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Color fill"), 32, 16, cs,
                                  KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    auto target = image->root()->at(1);
    auto other = image->root()->at(0);
    other->setOpacity(0);
    const auto seed = QByteArray::fromHex("112233ff");
    const auto read = [](KisNodeSP node, QPoint point) {
        QByteArray bytes(4, '\0');
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), point.x(), point.y(), 1, 1);
        return bytes;
    };
    for (auto node : {target, other}) for (const auto &point : {QPoint(3, 5), QPoint(-5, 4), QPoint(40, 20)})
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(seed.constData()), point.x(), point.y(), 1, 1);
    if (scenario == QLatin1String("layer-alpha-lock")) qobject_cast<KisPaintLayer *>(target.data())->setAlphaLocked(true);
    if (scenario == QLatin1String("channel-lock")) {
        QBitArray channels(4, true);
        channels.setBit(2, false); // Public BGRA8 paint-device channels protect red.
        qobject_cast<KisPaintLayer *>(target.data())->setChannelLockFlags(channels);
    }
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(target);
    manager->slotSetSelectedNodes({other});
    auto *resources = view->resourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setFGColor(KoColor(QColor(17, 129, 241, scenario == QLatin1String("half-alpha-opacity") ? 128 : 255), cs));
    resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
    const bool withOpacity = scenario.endsWith(QLatin1String("opacity"));
    resources->setOpacity(scenario == QLatin1String("zero-opacity") ? 0 : qreal(withOpacity ? 128 : 64) / 255);
    resources->setEraserMode(scenario == QLatin1String("erase"));
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->resourceManager()->setResource(KoCanvasResource::GlobalAlphaLock,
        scenario == QLatin1String("alpha-lock") || scenario == QLatin1String("erase"));
    if (scenario == QLatin1String("reject-blend")) resources->setCurrentCompositeOp(QStringLiteral("multiply"));
    const auto count = lifetime->undoStack()->count();
    const auto index = lifetime->undoStack()->index();
    if (owner) QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    else lifetime->undoStack()->setClean();
    const auto actionName = scenario == QLatin1String("reject-pattern") ? QStringLiteral("fill_selection_pattern") : QStringLiteral("fill_selection_") +
        (scenario.startsWith(QLatin1String("background")) ? QStringLiteral("background_color") : QStringLiteral("foreground_color")) +
        (withOpacity ? QStringLiteral("_opacity") : QString());
    auto *action = window->viewManager()->actionManager()->actionByName(actionName);
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto before = owner ? owner->operations().snapshot() : Gpu::Snapshot{};
    QMetaObject::Connection close;
    if (scenario == QLatin1String("close-during-fill")) {
        close = connect(owner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            QObject::disconnect(close);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            window.reset();
        });
    }
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    if (scenario == QLatin1String("close-during-fill")) {
        QVERIFY(!window);
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QCOMPARE(owner->operations().undoCount(), count + 1);
        QCOMPARE(read(target, {8, 8}), blank);
        return;
    }
    if (scenario.startsWith(QLatin1String("reject-"))) {
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(lifetime->undoStack()->count(), count);
        QVERIFY(lifetime->undoStack()->isClean());
        QCOMPARE(read(target, {8, 8}), QByteArray(4, '\0'));
        QCOMPARE(read(target, {3, 5}), seed);
        return;
    }
    resources->setFGColor(KoColor(Qt::green, cs));
    resources->setBGColor(KoColor(Qt::yellow, cs));
    resources->setOpacity(1);
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("Flood Fill Layer").toString());
    QCOMPARE(read(target, {8, 8}), blank);
    QCOMPARE(read(target, {3, 5}), covered);
    QCOMPARE(read(target, {-5, 4}), seed);
    QCOMPARE(read(target, {40, 20}), seed);
    QCOMPARE(read(other, {3, 5}), seed);
    QCOMPARE(manager->activeNode(), target);
    QCOMPARE(manager->selectedNodes(), KisNodeList{other});
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), index);
    QCOMPARE(read(target, {8, 8}), QByteArray(4, '\0'));
    QCOMPARE(read(target, {3, 5}), seed);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(read(target, {8, 8}), blank);
    QCOMPARE(read(target, {3, 5}), covered);
    const QRect region(-8, -8, 64, 40);
    QHash<QUuid, QByteArray> expectedLayers;
    for (const auto &node : {target, other}) {
        QByteArray bytes(region.width() * region.height() * 4, '\0');
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region.x(), region.y(), region.width(), region.height());
        for (qsizetype i = 0; i < bytes.size(); i += 4) std::swap(bytes[i], bytes[i + 2]);
        expectedLayers.insert(node->uuid(), bytes);
    }
    const auto projected = image->projection()->convertToQImage(nullptr, 0, 0, 32, 16).convertToFormat(QImage::Format_RGBA8888);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("filled.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QVERIFY(lifetime->undoStack()->isClean());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::PaintImage reopened(execution, loaded.document.image);
    QCOMPARE(reopened.operations().snapshot()->layers.size(), 2);
    QByteArray canonical;
    for (const auto &layer : reopened.operations().snapshot()->layers) {
        QVERIFY(expectedLayers.contains(layer.uuid));
        const auto pixels = reopened.readRgbaPixels(layer.id, region, region.width() * region.height() * 4);
        QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
        QCOMPARE(pixels.bytes, expectedLayers.value(layer.uuid));
        canonical.append(pixels.bytes);
    }
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = directory.filePath(QStringLiteral("filled.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(png.size(), projected.size());
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()),
             QByteArray::fromRawData(reinterpret_cast<const char *>(projected.constBits()), projected.sizeInBytes()));
    QByteArray expectedProjection(32 * 16 * 4, '\0');
    const auto projectionPixel = [](QByteArray pixel) {
        if (pixel[3] == 0) return QByteArray(4, '\0');
        std::swap(pixel[0], pixel[2]);
        return pixel;
    };
    const auto backgroundPixel = projectionPixel(blank);
    for (qsizetype i = 0; i < expectedProjection.size(); i += 4)
        std::copy(backgroundPixel.cbegin(), backgroundPixel.cend(), expectedProjection.begin() + i);
    const auto coveredPixel = projectionPixel(covered);
    std::copy(coveredPixel.cbegin(), coveredPixel.cend(), expectedProjection.begin() + (5 * 32 + 3) * 4);
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), expectedProjection);
    const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!resultDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(resultDirectory));
        const auto stem = QDir(resultDirectory).filePath(QStringLiteral("fill-") + QString::fromLatin1(QTest::currentDataTag()));
        QFile pixels(stem + QStringLiteral(".rgba"));
        QVERIFY(pixels.open(QIODevice::WriteOnly));
        QCOMPARE(pixels.write(canonical), canonical.size());
        QFile exported(stem + QStringLiteral(".png.rgba"));
        QVERIFY(exported.open(QIODevice::WriteOnly));
        QCOMPARE(exported.write(reinterpret_cast<const char *>(png.constBits()), png.sizeInBytes()), png.sizeInBytes());
        QVERIFY(QFile::copy(path, stem + QStringLiteral(".kra")));
        QVERIFY(QFile::copy(pngPath, stem + QStringLiteral(".png")));
    }
}

void KisGpuDocumentViewTest::clearActionPreservesSelectedLayersAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<bool>("locked");
    QTest::addColumn<bool>("closeDuringClear");
    for (const bool gpu : {false, true}) {
        const auto prefix = gpu ? QByteArray("gpu-") : QByteArray("ordinary-");
        QTest::newRow(QByteArray(prefix + "single").constData()) << gpu << false << false << false;
        QTest::newRow(QByteArray(prefix + "multiple").constData()) << gpu << true << false << false;
        QTest::newRow(QByteArray(prefix + "locked").constData()) << gpu << true << true << false;
    }
    QTest::newRow("gpu-close-during-clear") << true << true << false << true;
}

void KisGpuDocumentViewTest::clearActionPreservesSelectedLayersAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, multiple);
    QFETCH(bool, locked);
    QFETCH(bool, closeDuringClear);
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    namespace Gpu = LibrePaint::Image::Gpu;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    Gpu::PaintImageSP owner;
    if (gpu) {
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        QVERIFY2(execution, qPrintable(diagnostic));
        owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Clear layers")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Clear layers"), 32, 16, cs,
                                  KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    KisNodeList layers{image->root()->at(0), image->root()->at(1), image->root()->at(2)};
    const QByteArray pixel = QByteArray::fromHex("112233ff");
    const QList<QPoint> points{{3, 5}, {-4, 8}, {40, 20}};
    for (auto layer : layers) {
        for (const auto &point : points)
            layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), point.x(), point.y(), 1, 1);
    }
    if (locked) layers[1]->setUserLocked(true);
    layers[0]->setOpacity(0);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(layers[0]);
    const KisNodeList selected = multiple ? KisNodeList{layers[1], layers[2]} : KisNodeList{layers[2]};
    manager->slotSetSelectedNodes(selected);
    view->resourceProvider()->setPaintOpPreset(m_preset);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QApplication::processEvents();
    const auto count = lifetime->undoStack()->count();
    const auto index = lifetime->undoStack()->index();
    if (owner) QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    else lifetime->undoStack()->setClean();
    auto *clear = window->viewManager()->actionManager()->actionByName(QStringLiteral("clear"));
    QVERIFY(clear);
    QVERIFY(clear->isEnabled());
    if (owner) {
        const auto before = owner->operations().snapshot();
        QVERIFY(!KisToolUtils::clearImage(image, selected, KisSelectionSP(new KisSelection)));
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(lifetime->undoStack()->count(), count);
    }
    QMetaObject::Connection close;
    if (closeDuringClear) {
        close = connect(owner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            QObject::disconnect(close);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            window.reset();
        });
    }
    try {
        clear->trigger();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (closeDuringClear) {
        QVERIFY(!window);
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QCOMPARE(owner->operations().undoCount(), count + 1);
        for (auto layer : selected) {
            QByteArray actual(4, '\0');
            layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 3, 5, 1, 1);
            QCOMPARE(actual, QByteArray(4, '\0'));
        }
        return;
    }
    image->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(lifetime->undoStack()->index(), index + 1);
    const auto checkPixels = [&](bool cleared) {
        for (const auto &layer : layers) {
            QCOMPARE(image->root()->at(layers.indexOf(layer)), layer);
            for (const auto &point : points) {
                QByteArray actual(4, '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), point.x(), point.y(), 1, 1);
                QCOMPARE(actual, cleared && selected.contains(layer) && !layer->userLocked() ? QByteArray(4, '\0') : pixel);
            }
        }
        QCOMPARE(manager->activeNode(), layers[0]);
        QCOMPARE(manager->selectedNodes().size(), selected.size());
        for (const auto &layer : selected) QVERIFY(manager->selectedNodes().contains(layer));
    };
    checkPixels(true);
    QTRY_VERIFY(lifetime->isModified());
    lifetime->undoStack()->undo();
    image->waitForDone();
    checkPixels(false);
    QVERIFY(lifetime->undoStack()->isClean());
    lifetime->undoStack()->redo();
    image->waitForDone();
    checkPixels(true);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("cleared.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.size(), 3);
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = directory.filePath(QStringLiteral("cleared.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration),
             qPrintable(lifetime->errorMessage()));
    const QImage png(pngPath);
    QCOMPARE(png.size(), QSize(32, 16));
    QCOMPARE(png.pixelColor(3, 5), multiple && !locked ? QColor(0, 0, 0, 0) : QColor(0x33, 0x22, 0x11));
    QVERIFY(lifetime->undoStack()->isClean());
    if (owner) {
        Gpu::PaintImage reopened(owner->execution(), loaded.document.image);
        QByteArray canonical;
        for (const auto &layer : reopened.operations().snapshot()->layers) {
            const auto found = std::find_if(layers.cbegin(), layers.cend(), [&](const auto &node) { return node->uuid() == layer.uuid; });
            QVERIFY(found != layers.cend());
            QCOMPARE(layer.opacity, (*found)->opacity());
            QCOMPARE(layer.userLocked, (*found)->userLocked());
            for (const auto &point : points) {
                const auto pixels = reopened.readRgbaPixels(layer.id, QRect(point, QSize(1, 1)), 4);
                QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
                QCOMPARE(pixels.bytes, selected.contains(*found) && !(*found)->userLocked() ? QByteArray(4, '\0') : QByteArray::fromHex("332211ff"));
            }
            const auto pixels = reopened.readRgbaPixels(layer.id, QRect(-8, -8, 64, 40), 64 * 40 * 4);
            QVERIFY2(pixels.code == Gpu::ExecutionCode::Success, qPrintable(pixels.diagnostic));
            canonical.append(pixels.bytes);
        }
        const auto resultDirectory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!resultDirectory.isEmpty()) {
            QVERIFY(QDir().mkpath(resultDirectory));
            const auto stem = QDir(resultDirectory).filePath(QStringLiteral("clear-") + QString::fromLatin1(QTest::currentDataTag()));
            QFile pixels(stem + QStringLiteral(".rgba"));
            QVERIFY(pixels.open(QIODevice::WriteOnly));
            QCOMPARE(pixels.write(canonical), canonical.size());
            QVERIFY(QFile::copy(path, stem + QStringLiteral(".kra")));
            QVERIFY(QFile::copy(pngPath, stem + QStringLiteral(".png")));
            const auto rgba = png.convertToFormat(QImage::Format_RGBA8888);
            QFile exported(stem + QStringLiteral(".png.rgba"));
            QVERIFY(exported.open(QIODevice::WriteOnly));
            QCOMPARE(exported.write(reinterpret_cast<const char *>(rgba.constBits()), rgba.sizeInBytes()), rgba.sizeInBytes());
        }
        const auto clearText = lifetime->undoStack()->undoText();
        const auto direct = QByteArray::fromHex("445566ff");
        layers[2]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(direct.constData()), 9, 9, 1, 1);
        QCOMPARE(owner->restoreRecovery(owner->operations().recoveryData()), Gpu::Result::Success);
        QTRY_COMPARE(lifetime->undoStack()->undoText(), clearText);
        QCOMPARE(lifetime->undoStack()->count(), count + 1);
    }
}

void KisGpuDocumentViewTest::lineToolPreservesPreviewCancellationAndHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("reject");
    QTest::addColumn<bool>("closeAfterError");
    QTest::addColumn<bool>("rejectOnRelease");
    QTest::newRow("ordinary") << false << false << false << false;
    QTest::newRow("gpu") << true << false << false << false;
    QTest::newRow("gpu-rejected-preview") << true << true << false << false;
    QTest::newRow("gpu-close-on-error") << true << true << true << false;
    QTest::newRow("gpu-close-on-release") << true << true << true << true;
}

void KisGpuDocumentViewTest::lineToolPreservesPreviewCancellationAndHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, reject);
    QFETCH(bool, closeAfterError);
    QFETCH(bool, rejectOnRelease);
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Line input")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Line input"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<KoToolBase> tool;
    QPointer<QWindow> native;
    QPointer<QWidget> input(view->canvasBase()->canvasWidget());
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (tool) tool->requestStrokeCancellation();
        if (pressed) {
            if (native) QTest::mouseRelease(native.data(), Qt::LeftButton);
            else if (input) QTest::mouseRelease(input.data(), Qt::LeftButton);
        }
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolLine"));
    tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolLine"));
    QVERIFY(tool);
    QPointer<QCheckBox> previewOption;
    for (QWidget *panel : tool->optionWidgets()) {
        for (auto *option : panel->findChildren<QCheckBox*>())
            if (option->text() == i18n("Show Preview")) previewOption = option;
    }
    QVERIFY(previewOption);
    const bool previousPreview = previewOption->isChecked();
    const auto restorePreview = qScopeGuard([&] {
        if (previewOption) previewOption->setChecked(previousPreview);
    });
    previewOption->setChecked(!rejectOnRelease);
    QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
    if (closeAfterError) {
        connect(tool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message.isEmpty()) return;
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    input->setFocus();
    QTRY_VERIFY(input->width() >= 64 && input->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto size = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({size.width() / 2, size.height() / 2});
    if (gpu) {
        QTRY_COMPARE(execution->published(owner->handle()), owner->operations().snapshot());
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = candidate;
        QVERIFY(native);
    }
    const auto point = [&](QPointF position) {
        return view->canvasBase()->coordinatesConverter()->imageToWidget(position).toPoint();
    };
    const auto move = [&](QPointF position) {
        if (native) QTest::mouseMove(native.data(), point(position));
        else {
            const auto p = point(position);
            QMouseEvent event(QEvent::MouseMove, p, input->mapToGlobal(p), Qt::NoButton,
                              pressed ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(input.data(), &event);
        }
        QTest::qWait(10);
    };
    const auto press = [&](QPointF position) {
        move(position);
        const auto p = point(position);
        QEnterEvent entered(p, p, native ? native->mapToGlobal(p) : input->mapToGlobal(p));
        if (native) QApplication::sendEvent(native.data(), &entered);
        else QApplication::sendEvent(input.data(), &entered);
        pressed = true;
        if (native) QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, p);
        else QTest::mousePress(input.data(), Qt::LeftButton, Qt::NoModifier, p);
    };
    const auto release = [&](QPointF position) {
        if (native) QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(position));
        else QTest::mouseRelease(input.data(), Qt::LeftButton, Qt::NoModifier, point(position));
        pressed = false;
    };
    KisImageWSP image = lifetime->image();
    const auto layer = image->root()->firstChild();
    const auto pixel = [&](QPoint position) {
        QByteArray bytes(4, '\0');
        layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), position.x(), position.y(), 1, 1);
        return bytes;
    };
    try {
        if (reject) {
            const auto before = owner->operations().snapshot();
            resources->setCurrentCompositeOp(QStringLiteral("multiply"));
            press({16, 16});
            move({40, 16});
            if (rejectOnRelease) release({40, 16});
            else if (tool) QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
            QVERIFY(!errors.isEmpty());
            if (closeAfterError) {
                if (pressed) QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
                pressed = false;
                QTRY_VERIFY(!lifetime);
                QTRY_VERIFY(!native);
                QCOMPARE(owner->operations().snapshot(), before);
                return;
            }
            release({40, 16});
            QVERIFY(!owner->operations().hasActiveStroke());
            QCOMPARE(owner->operations().snapshot(), before);
            QCOMPARE(lifetime->undoStack()->count(), 0);
            QVERIFY(!lifetime->isModified());
            resources->setCurrentCompositeOp(QStringLiteral("normal"));
        }
        press({16, 16});
        move({40, 16});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        if (gpu) {
            QVERIFY2(owner->operations().hasActiveStroke(),
                     errors.isEmpty() ? "The line preview did not start a stroke" : qPrintable(errors.last().first().toString()));
            QCOMPARE(owner->operations().undoCount(), 0);
            QTRY_COMPARE(execution->published(owner->handle()), owner->operations().preview());
        }
        move({40, 40});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        release({40, 40});
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), 1);
        if (gpu) QCOMPARE(owner->operations().historyAction(1), Gpu::HistoryAction::DrawLine);
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        QCOMPARE(pixel({36, 16}), QByteArray(4, '\0'));
        lifetime->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray(4, '\0'));
        lifetime->undoStack()->redo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        press({16, 48});
        move({40, 48});
        QVERIFY(QMetaObject::invokeMethod(tool, "updateStroke", Qt::DirectConnection));
        tool->requestStrokeCancellation();
        release({40, 48});
        image->waitForDone();
        QCOMPARE(lifetime->undoStack()->count(), 1);
        QCOMPARE(pixel({28, 48}), QByteArray(4, '\0'));
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        lifetime->setFileBatchMode(true);
        const auto path = directory.filePath(QStringLiteral("line.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QTRY_VERIFY(!lifetime->isModified());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.size(), 1);
        const auto &tiles = reopened.document.image.layers.first().tiles;
        QCOMPARE(tiles.size(), 1);
        QCOMPARE(tiles.first().rgba.mid((28 * Gpu::RgbaTile::Size + 28) * 4, 4), QByteArray::fromHex("1181f1ff"));
        resources->setEraserMode(true);
        QTRY_VERIFY(resources->eraserMode());
        press({16, 16});
        move({40, 40});
        release({40, 40});
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), 2);
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f1811100"));
        lifetime->undoStack()->undo();
        image->waitForDone();
        QCOMPARE(pixel({28, 28}), QByteArray::fromHex("f18111ff"));
        QTRY_VERIFY(!lifetime->isModified());
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
}

void KisGpuDocumentViewTest::layerPanelReordersSelectionWithOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("raise");
    QTest::addColumn<int>("selection");
    for (const bool gpu : {false, true}) {
        for (const bool raise : {false, true}) {
            for (const int selection : {0, 1, 2}) {
                const QByteArray name = QByteArray(gpu ? "gpu-" : "ordinary-") +
                    (raise ? "raise-" : "lower-") +
                    (selection == 0 ? "single" : selection == 1 ? "contiguous" : "disjoint");
                QTest::newRow(name.constData()) << gpu << raise << selection;
            }
        }
    }
}

void KisGpuDocumentViewTest::layerPanelReordersSelectionWithOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(bool, raise);
    QFETCH(int, selection);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        for (const auto &name : {QStringLiteral("Second"), QStringLiteral("Third"), QStringLiteral("Top")})
            QVERIFY(owner->addLayer(name));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer ordering")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer ordering"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 4, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    QVERIFY(window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox")));
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    const auto root = image->root();
    const KisNodeList original{root->at(0), root->at(1), root->at(2), root->at(3)};
    const QList<QByteArray> pixels{QByteArray::fromHex("332211ff"), QByteArray::fromHex("665544ff"),
                                  QByteArray::fromHex("998877ff"), QByteArray::fromHex("ccbbaaff")};
    for (qsizetype i = 0; i < original.size(); ++i)
        original[i]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels[i].constData()), 5, 5, 1, 1);
    const KisNodeList selected = selection == 0 ? KisNodeList{original[1]} : selection == 1 ?
        KisNodeList{original[1], original[2]} : raise ?
        KisNodeList{original[0], original[2]} : KisNodeList{original[1], original[3]};
    auto *manager = window->viewManager()->nodeManager();
    const auto active = selected.last();
    manager->slotUiActivatedNode(active);
    manager->slotSetSelectedNodes(selected);
    const auto before = lifetime->undoStack()->count();
    try {
        if (raise) manager->raiseNode();
        else manager->lowerNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    const KisNodeList expected = selection == 2 ? KisNodeList{original[1], original[3], original[0], original[2]} : raise ?
        (selection == 1 ? KisNodeList{original[0], original[3], original[1], original[2]} :
                    KisNodeList{original[0], original[2], original[1], original[3]}) :
        (selection == 1 ? KisNodeList{original[1], original[2], original[0], original[3]} :
                    KisNodeList{original[1], original[0], original[2], original[3]});
    const auto order = [&] {
        KisNodeList result;
        for (quint32 i = 0; i < root->childCount(); ++i) result.append(root->at(i));
        return result;
    };
    QTRY_COMPARE(order(), expected);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QByteArray projection(4, '\0');
    image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), 5, 5, 1, 1);
    QCOMPARE(projection, pixels[original.indexOf(expected.last())]);
    QCOMPARE(view->currentNode(), active);
    const auto actualSelection = [&] {
        auto nodes = manager->selectedNodes();
        std::sort(nodes.begin(), nodes.end());
        return nodes;
    };
    auto sortedSelection = selected;
    std::sort(sortedSelection.begin(), sortedSelection.end());
    QTRY_COMPARE(actualSelection(), sortedSelection);
    if (gpu) {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("ordered.kra"));
        lifetime->setFileBatchMode(true);
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QTRY_VERIFY(!lifetime->isModified());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.size(), expected.size());
        for (qsizetype i = 0; i < expected.size(); ++i)
            QCOMPARE(reopened.document.image.layers[i].uuid, expected[i]->uuid());
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(order(), original);
    QCOMPARE(view->currentNode(), active);
    QTRY_COMPARE(actualSelection(), sortedSelection);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(order(), expected);
    QCOMPARE(view->currentNode(), active);
    QTRY_COMPARE(actualSelection(), sortedSelection);
    for (qsizetype i = 0; i < original.size(); ++i) {
        QByteArray actual(4, '\0');
        original[i]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
        QCOMPARE(actual, pixels[i]);
    }
}

void KisGpuDocumentViewTest::layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("rename");
    QTest::addColumn<bool>("closeAfterChange");
    QTest::newRow("ordinary") << false << true << false;
    QTest::newRow("ordinary-keep-names") << false << false << false;
    QTest::newRow("gpu") << true << true << false;
    QTest::newRow("gpu-keep-names") << true << false << false;
    QTest::newRow("gpu-close-on-change") << true << true << true;
}

void KisGpuDocumentViewTest::layerPanelDuplicationPreservesPixelsSelectionAndOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(bool, rename);
    QFETCH(bool, closeAfterChange);
    const bool previousRename = KisImageConfig(true).renameDuplicatedLayers();
    KisImageConfig(false).setRenameDuplicatedLayers(rename);
    const auto restoreNames = qScopeGuard([&] { KisImageConfig(false).setRenameDuplicatedLayers(previousRename); });
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer duplication")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer duplication"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    auto root = image->root();
    KisNodeList originals{root->at(0), root->at(1), root->at(2)};
    const auto pixel = QByteArray::fromHex("332211ff");
    for (int index = 0; index < originals.size(); ++index) {
        originals[index]->setName(QStringLiteral("Source %1").arg(index));
        originals[index]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    }
    originals[2]->setUserLocked(true);
    originals[2]->setVisible(false);
    originals[0]->setOpacity(128);
    originals[0]->setPinnedToTimeline(true);
    qobject_cast<KisPaintLayer*>(originals[0].data())->setAlphaLocked(true);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(originals[0]);
    manager->slotSetSelectedNodes({originals[2], originals[0]});
    const auto before = lifetime->undoStack()->count();
    const KisImageWSP imageLifetime = image;
    const auto retainedOwner = image->gpuImage();
    QMetaObject::Connection closeOnChange;
    if (closeAfterChange) {
        closeOnChange = connect(retainedOwner.data(), &Gpu::PaintImage::changed, window.get(), [&] {
            if (!lifetime || retainedOwner->operations().snapshot()->layers.size() != 5) return;
            disconnect(closeOnChange);
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        });
        image.clear();
    }
    const auto disconnectClose = qScopeGuard([&] { disconnect(closeOnChange); });
    try {
        manager->duplicateActiveNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    if (closeAfterChange) {
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!lifetime);
        QTRY_VERIFY(!imageLifetime.isValid());
        QCOMPARE(root->childCount(), 5U);
        return;
    }
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 5U);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("Duplicate Nodes").toString());
    QTRY_VERIFY(lifetime->isModified());
    for (int index = 0; index < originals.size(); ++index) QCOMPARE(root->at(index), originals[index]);
    KisNodeList copies{root->at(3), root->at(4)};
    for (int index = 0; index < copies.size(); ++index) {
        auto original = originals[index * 2];
        QVERIFY(copies[index]->uuid() != original->uuid());
        QVERIFY(copies[index]->name().contains(original->name()));
        if (rename) QVERIFY(copies[index]->name() != original->name());
        else QCOMPARE(copies[index]->name(), original->name());
        QCOMPARE(copies[index]->userLocked(), original->userLocked());
        QCOMPARE(copies[index]->visible(), original->visible());
        QCOMPARE(copies[index]->opacity(), original->opacity());
        QCOMPARE(copies[index]->isPinnedToTimeline(), original->isPinnedToTimeline());
        QCOMPARE(qobject_cast<KisPaintLayer*>(copies[index].data())->alphaLocked(),
                 qobject_cast<KisPaintLayer*>(original.data())->alphaLocked());
        QByteArray actual(4, '\0');
        copies[index]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
        QCOMPARE(actual, pixel);
    }
    QTRY_COMPARE(view->currentNode(), copies[0]);
    QTRY_COMPARE(manager->selectedNodes().size(), 2);
    const auto changed = QByteArray::fromHex("665544aa");
    copies[0]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(changed.constData()), 6, 6, 1, 1);
    QByteArray actual(4, '\0');
    originals[0]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
    QCOMPARE(actual, QByteArray(4, '\0'));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 3U);
    QTRY_COMPARE(view->currentNode(), originals[0]);
    QTRY_COMPARE(manager->selectedNodes().size(), 2);
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 5U);
    QCOMPARE(root->at(3), copies[0]);
    QCOMPARE(root->at(4), copies[1]);
    copies[0]->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
    QCOMPARE(actual, changed);
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<int>("selection");
    for (const bool gpu : {false, true}) {
        const auto prefix = gpu ? QByteArrayLiteral("gpu-") : QByteArrayLiteral("ordinary-");
        QTest::newRow(QByteArray(prefix + "single").constData()) << gpu << 0;
        QTest::newRow(QByteArray(prefix + "multiple").constData()) << gpu << 1;
        QTest::newRow(QByteArray(prefix + "all").constData()) << gpu << 2;
    }
}

void KisGpuDocumentViewTest::layerPanelRemovalPreservesLayersSelectionAndOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(int, selection);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer removal")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer removal"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    QVERIFY(window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox")));
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    const auto root = image->root();
    KisNodeList originals{root->at(0), root->at(1), root->at(2)};
    const auto pixel = QByteArray::fromHex("332211ff");
    for (const auto &node : originals)
        node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    auto *manager = window->viewManager()->nodeManager();
    const KisNodeList selected = selection == 0 ? KisNodeList{originals[1]} :
        selection == 1 ? KisNodeList{originals[0], originals[2]} : originals;
    const auto active = selected.last();
    manager->slotUiActivatedNode(active);
    manager->slotSetSelectedNodes(selected);
    const auto before = lifetime->undoStack()->count();
    try {
        manager->removeNode();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), selection == 0 ? 2U : 1U);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QTRY_VERIFY(lifetime->isModified());
    for (const auto &node : selected) QVERIFY(!node->parent());
    const auto detachedPixel = QByteArray::fromHex("665544aa");
    try {
        for (const auto &node : selected) {
            QByteArray retained(4, '\0');
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(retained.data()), 5, 5, 1, 1);
            QCOMPARE(retained, pixel);
            node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(detachedPixel.constData()), 6, 6, 1, 1);
            node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(retained.data()), 6, 6, 1, 1);
            QCOMPARE(retained, detachedPixel);
            QVERIFY(node->paintDevice()->exactBounds().contains(QPoint(6, 6)));
            QVERIFY(!node->paintDevice()->createThumbnail(8, 8).isNull());
        }
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QByteArray remaining(4, '\0');
    root->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8 *>(remaining.data()), 6, 6, 1, 1);
    QCOMPARE(remaining, QByteArray(4, '\0'));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QTRY_VERIFY(view->currentNode() && view->currentNode()->parent() == root);
    const auto after = view->currentNode();
    lifetime->undoStack()->undo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), 3U);
    for (int i = 0; i < originals.size(); ++i) QCOMPARE(root->at(i), originals[i]);
    QTRY_COMPARE(view->currentNode(), active);
    QTRY_COMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    QByteArray actual(4, '\0');
    active->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
    QCOMPARE(actual, pixel);
    for (const auto &node : selected) {
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
        QCOMPARE(actual, detachedPixel);
    }
    lifetime->undoStack()->redo();
    image->waitForDone();
    QTRY_COMPARE(root->childCount(), selection == 0 ? 2U : 1U);
    QTRY_COMPARE(view->currentNode(), after);
    for (const auto &node : selected) {
        node->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 6, 6, 1, 1);
        QCOMPARE(actual, detachedPixel);
    }
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("pinned");
    QTest::newRow("ordinary") << false << false;
    QTest::newRow("gpu") << true << false;
    QTest::newRow("ordinary-pinned") << false << true;
    QTest::newRow("gpu-pinned") << true << true;
}

void KisGpuDocumentViewTest::layerPanelAddsAtSelectionWithOneHistoryEntry()
{
    QFETCH(bool, gpu);
    QFETCH(bool, pinned);
    const bool previousPin = KisConfig(true).autoPinLayersToTimeline();
    const auto restorePin = qScopeGuard([&] { KisConfig(false).setAutoPinLayersToTimeline(previousPin); });
    KisConfig(false).setAutoPinLayersToTimeline(pinned);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer creation")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer creation"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    const auto root = image->root();
    const auto middle = root->at(1);
    const auto top = root->at(2);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(middle);
    QCOMPARE(view->currentNode(), middle);
    const auto before = lifetime->undoStack()->count();
    KisNodeSP added;
    try {
        added = manager->createNode(QStringLiteral("KisPaintLayer"));
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    QVERIFY(added);
    image->waitForDone();
    QTRY_COMPARE(view->currentNode(), added);
    QCOMPARE(root->childCount(), 4U);
    QCOMPARE(root->at(1), middle);
    QCOMPARE(root->at(2), added);
    QCOMPARE(root->at(3), top);
    QCOMPARE(added->isPinnedToTimeline(), pinned);
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    QVERIFY(lifetime->isModified());
    const auto pixel = QByteArray::fromHex("332211ff");
    added->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.constData()), 5, 5, 1, 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(root->childCount(), 3U);
    QVERIFY(!added->parent());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(root->at(2), added);
    QTRY_COMPARE(view->currentNode(), added);
    QByteArray actual(4, '\0');
    added->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actual.data()), 5, 5, 1, 1);
    QCOMPARE(actual, pixel);
}

void KisGpuDocumentViewTest::layerPanelSelectionPreservesAnUnselectedActiveLayer_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("multiple");
    QTest::newRow("ordinary-single") << false << false;
    QTest::newRow("ordinary-multiple") << false << true;
    QTest::newRow("gpu-single") << true << false;
    QTest::newRow("gpu-multiple") << true << true;
}

void KisGpuDocumentViewTest::layerPanelSelectionPreservesAnUnselectedActiveLayer()
{
    QFETCH(bool, gpu);
    QFETCH(bool, multiple);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Middle")));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer selection")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Layer selection"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 3, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    const auto active = image->root()->at(0);
    const auto middle = image->root()->at(1);
    const auto top = image->root()->at(2);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(active);
    const KisNodeList selected = multiple ? KisNodeList{middle, top} : KisNodeList{top};
    const auto before = lifetime->undoStack()->count();
    manager->slotSetSelectedNodes(selected);
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(view->currentNode(), active);
    QCOMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    QCOMPARE(lifetime->undoStack()->count(), before);
    manager->duplicateActiveNode();
    image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), 3U + quint32(selected.size()));
    QCOMPARE(lifetime->undoStack()->count(), before + 1);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(image->root()->childCount(), 3U);
    QTRY_COMPARE(view->currentNode(), active);
    QCOMPARE(manager->selectedNodes().size(), selected.size());
    for (const auto &node : selected) QVERIFY(manager->selectedNodes().contains(node));
    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    tree->selectionModel()->clearSelection();
    QTRY_COMPARE(manager->selectedNodes(), KisNodeList{active});
    QCOMPARE(view->currentNode(), active);
}

void KisGpuDocumentViewTest::layerSelectionCanCloseTheWindowDuringNotification_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::newRow("ordinary-selection-notification") << false;
    QTest::newRow("gpu-image-notification") << true;
}

void KisGpuDocumentViewTest::layerSelectionCanCloseTheWindowDuringNotification()
{
    QFETCH(bool, gpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    std::shared_ptr<Gpu::Execution> execution;
    Gpu::PaintImageSP owner;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (gpu) {
        QString diagnostic;
        const auto device = Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<Gpu::Execution>(device);
        owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
        QVERIFY(owner->addLayer(QStringLiteral("Top")));
        QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Closing during selection")));
    } else {
        const auto cs = KoColorSpaceRegistry::instance()->rgb8();
        QVERIFY(document->newImage(QStringLiteral("Closing during selection"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (window && lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    KisImageWSP imageLifetime = lifetime->image();
    auto selected = lifetime->image()->root()->firstChild();
    QPointer<KisNodeManager> manager = window->viewManager()->nodeManager();
    QMetaObject::Connection close;
    const auto closeDuringSelection = [&] {
        QObject::disconnect(close);
        lifetime->setModified(false);
        window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        window.reset();
    };
    close = gpu ? connect(owner.data(), &Gpu::PaintImage::changed, window.get(), closeDuringSelection)
                : connect(manager.data(), &KisNodeManager::sigUiNeedChangeSelectedNodes, window.get(), closeDuringSelection);
    manager->slotSetSelectedNodes({selected});
    QVERIFY(!manager);
    QVERIFY(!window);
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!lifetime);
    QTRY_VERIFY(!imageLifetime.isValid());
    const auto handle = owner ? owner->handle() : 0;
    selected.clear();
    owner.clear();
    if (execution) QVERIFY(!execution->requested(handle));
}

void KisGpuDocumentViewTest::layerPanelSelectionSurvivesSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(32, 16));
    QVERIFY(owner->addLayer(QStringLiteral("Middle")));
    QVERIFY(owner->addLayer(QStringLiteral("Top")));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    const auto undoCount = owner->operations().undoCount();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selected layer")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    QTRY_VERIFY(view->currentNode());
    QPointer<QDockWidget> layerPanel;
    for (auto *dock : window->findChildren<QDockWidget*>())
        if (dock->metaObject()->indexOfSlot("slotNodeCollapsedChanged()") >= 0) layerPanel = dock;
    QVERIFY(layerPanel);
    QSignalSpy panelImageChanged(layerPanel, SIGNAL(imageChanged()));
    QVERIFY(panelImageChanged.isValid());
    QCOMPARE(view->currentNode()->uuid(), saved->layers.last().uuid);
    const auto middle = lifetime->image()->root()->at(1);
    QVERIFY(middle);
    window->viewManager()->nodeManager()->slotUiActivatedNode(middle);
    QCOMPARE(view->currentNode(), middle);
    QCOMPARE(lifetime->preActivatedNode(), middle);
    QCOMPARE(owner->operations().activeLayer(), saved->layers[1].id);
    QCOMPARE(owner->operations().undoCount(), undoCount);
    QVERIFY(!owner->operations().isModified());
    QVERIFY(!lifetime->isModified());
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("selected.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.activeLayer, 1);
    QCOMPARE(loaded.document.image.layers[1].uuid, middle->uuid());
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened selection")));
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), loaded.document.image.layers[1].uuid);
    QCOMPARE(reopened->operations().activeLayer(), reopened->operations().snapshot()->layers[1].id);
    QCOMPARE(reopened->operations().undoCount(), 0);
    QVERIFY(!lifetime->isModified());
    QVERIFY(QMetaObject::invokeMethod(layerPanel, "slotNodeCollapsedChanged", Qt::DirectConnection));
    QTRY_COMPARE(panelImageChanged.size(), 1);
    QVERIFY(QMetaObject::invokeMethod(layerPanel, "slotNodeCollapsedChanged", Qt::DirectConnection));
    QCOMPARE(panelImageChanged.size(), 1);
    auto *list = layerPanel->findChild<QAbstractItemView*>(QStringLiteral("listLayers"));
    QVERIFY(list);
    auto *model = qobject_cast<KisNodeFilterProxyModel*>(list->model());
    QVERIFY(model);
    const auto newMiddle = lifetime->image()->root()->at(1);
    QVERIFY(newMiddle != middle);
    const auto index = model->indexFromNode(newMiddle);
    QVERIFY(index.isValid());
    QCOMPARE(model->nodeFromIndex(index), newMiddle);
    QVERIFY(!model->indexFromNode(middle).isValid());
    QVERIFY(model->setData(index, QStringLiteral("Reopened middle"), Qt::EditRole));
    QCOMPARE(newMiddle->name(), QStringLiteral("Reopened middle"));
    QCOMPARE(reopened->operations().snapshot()->layers[1].name, QStringLiteral("Reopened middle"));
    QCOMPARE(owner->operations().snapshot()->layers[1].name, QStringLiteral("Middle"));
    QCOMPARE(reopened->operations().undoCount(), 1);

}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("locked");
    QTest::addColumn<bool>("alphaLocked");
    QTest::newRow("ordinary") << false << false << false;
    QTest::newRow("gpu") << true << false << false;
    QTest::newRow("ordinary-lock") << false << true << false;
    QTest::newRow("gpu-lock") << true << true << false;
    QTest::newRow("ordinary-alpha-lock") << false << false << true;
    QTest::newRow("gpu-alpha-lock") << true << false << true;
}

void KisGpuDocumentViewTest::layerPanelEditsUseOneDocumentHistory()
{
    QFETCH(bool, gpu);
    QFETCH(bool, locked);
    QFETCH(bool, alphaLocked);
    std::shared_ptr<LibrePaint::Image::Gpu::Execution> execution;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    if (gpu) {
        QString diagnostic;
        auto device = LibrePaint::Image::Gpu::Device::createNative(diagnostic);
        QVERIFY2(device, qPrintable(diagnostic));
        execution = std::make_shared<LibrePaint::Image::Gpu::Execution>(device);
        const auto owner = LibrePaint::Image::Gpu::PaintImageSP::create(execution, QSize(32, 16));
        document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Layer panel editing")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Layer panel editing"), 32, 16, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    const auto originalName = layer->name();
    const auto red = QByteArray::fromHex("0000ffff");
    layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(red.constData()), 5, 5, 1, 1);
    auto *manager = window->viewManager()->nodeManager();
    try {
        manager->setNodeName(layer, QStringLiteral("Adjusting"));
        image->waitForDone();
        manager->setNodeName(layer, QStringLiteral("Renamed"));
        image->waitForDone();
        manager->setNodeOpacity(layer, 192);
        image->waitForDone();
        manager->setNodeOpacity(layer, 128);
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    QCOMPARE(layer->opacity(), quint8(128));
    QCOMPARE(lifetime->undoStack()->count(), 2);
    if (gpu) {
        QCOMPARE(lifetime->undoStack()->command(0)->text().toString(), i18n("Rename layer"));
        QCOMPARE(lifetime->undoStack()->command(1)->text().toString(), i18n("Change layer opacity"));
    }
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(layer->opacity(), quint8(255));
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(layer->name(), originalName);
    lifetime->undoStack()->redo();
    image->waitForDone();
    lifetime->undoStack()->redo();
    image->waitForDone();
    QCOMPARE(layer->name(), QStringLiteral("Renamed"));
    QCOMPARE(layer->opacity(), quint8(128));
    image->refreshGraphAsync();
    image->waitForDone();
    QByteArray pixel(4, '\0');
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 5, 5, 1, 1);
    QCOMPARE(pixel, QByteArray::fromHex("0000ff80"));
    QCOMPARE(manager->activeNode(), layer);
    const auto toggle = [&] {
        if (alphaLocked) manager->toggleAlphaLock();
        else if (locked) manager->toggleLock();
        else manager->toggleVisibility();
    };
    const auto propertySet = [&] {
        return alphaLocked ? qobject_cast<const KisPaintLayer*>(layer.data())->alphaLocked() :
                             locked ? layer->userLocked() : !layer->visible();
    };
    try {
        toggle();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QVERIFY(propertySet());
    QCOMPARE(lifetime->undoStack()->count(), 3);
    image->projection()->readBytes(reinterpret_cast<quint8 *>(pixel.data()), 5, 5, 1, 1);
    QCOMPARE(pixel, locked || alphaLocked ? QByteArray::fromHex("0000ff80") : QByteArray(4, '\0'));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(!propertySet());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QVERIFY(propertySet());
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(!propertySet());
    lifetime->undoStack()->redo();
    image->waitForDone();
    try {
        toggle();
    } catch (const std::exception &error) {
        QFAIL(error.what());
    }
    image->waitForDone();
    QVERIFY(!propertySet());
    QCOMPARE(lifetime->undoStack()->count(), 2);
    if (gpu) {
        manager->setNodeName(layer, QStringLiteral("Temporary"));
        manager->setNodeName(layer, QStringLiteral("Renamed"));
        QCOMPARE(lifetime->undoStack()->count(), 3);
        QCOMPARE(lifetime->undoStack()->command(2)->text().toString(), i18n("Rename layer"));
    }
}

void KisGpuDocumentViewTest::rectangularSelectionLimitsPainting()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QTRY_COMPARE(execution->published(owner->handle()), saved);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect selected(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point({64, 64}));
    const auto start = point({64, 64});
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);
    QTRY_COMPARE(owner->selectionRect(), selected);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({128, 128}));
    QTest::mouseMove(native.data(), point({32, 128}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 128}));
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(256, 256));
    const auto painted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 256 + x) * 4, 4); };
    QCOMPARE(pixel(painted.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(painted.bytes, 32, 128), QByteArray(4, char(0)));
    QCOMPARE(pixel(painted.bytes, 224, 128), QByteArray(4, char(0)));

    auto *deselect = window->viewManager()->actionManager()->actionByName(QStringLiteral("deselect"));
    QVERIFY(deselect);
    deselect->trigger();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QVERIFY(!lifetime->image()->globalSelection() ||
            lifetime->image()->globalSelection()->selectedExactRect().isEmpty());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 32}));
    QTest::mouseMove(native.data(), point({224, 32}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({224, 32}));
    QTRY_COMPARE(owner->operations().undoCount(), 4);
    const auto unrestricted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(unrestricted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(unrestricted.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(unrestricted.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
}

void KisGpuDocumentViewTest::selectionFillUsesGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection fill")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    QCOMPARE(owner->operations().undoCount(), 1);
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 8, 8), QByteArray(4, char(0)));
    QCOMPARE(pixel(filled.bytes, 56, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 24, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    const auto redone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(redone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(redone.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(redone.bytes, 8, 8), QByteArray(4, char(0)));
}

void KisGpuDocumentViewTest::selectionMergedCopyUsesGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255, false, false) == Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection copy")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    action->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *data = QGuiApplication::clipboard()->mimeData();
    QVERIFY(data);
    const QImage copied = KisClipboard::instance()->getImageWithFallback(data);
    QVERIFY(!copied.isNull());
    QCOMPARE(copied.size(), QSize(32, 32));
    QCOMPARE(copied.pixelColor(16, 16), QColor(17, 129, 241));
}

void KisGpuDocumentViewTest::selectionCopyAndCutUseGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255, false, false) == Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection copy cut")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(lifetime->image()->setGpuSelectionRect(QRect(16, 16, 32, 32)));
    QTRY_COMPARE(owner->selectionRect(), QRect(16, 16, 32, 32));
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *data = QGuiApplication::clipboard()->mimeData();
    QVERIFY(data);
    const QImage copied = KisClipboard::instance()->getImageWithFallback(data);
    QVERIFY(!copied.isNull());
    QCOMPARE(copied.size(), QSize(32, 32));
    QCOMPARE(copied.pixelColor(16, 16), QColor(17, 129, 241));
    QCOMPARE(owner->operations().undoCount(), 2);

    manager->cut();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(64, 64));
    const auto cut = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cut.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(quint8(pixel(cut.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(cut.bytes, 8, 8), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 24, 24), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    const auto redone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(redone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(redone.bytes, 24, 24).at(3)), 0);
    QCOMPARE(pixel(redone.bytes, 8, 8), QByteArray::fromHex("1181f1ff"));
}

void KisGpuDocumentViewTest::selectionMovePreviewsAndCommitsOneHistory()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection move")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect selected(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point(selected.topLeft()));
    const auto start = point(selected.topLeft());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);

    QVERIFY(lifetime->image()->setGpuSelectionRect(selected, true));
    QTRY_VERIFY(owner->selectionInverted());
    const QPoint delta(16, 16);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({32, 32}));
    QTest::mouseMove(native.data(), point({48, 48}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({48, 48}));
    QTRY_COMPARE(owner->selectionRect(), selected.translated(delta));
    QVERIFY(owner->selectionInverted());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(!owner->selectionInverted());
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->operations().undoCount(), 1);

    const QRect moved = selected.translated(delta);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(selected.center()));
    QTest::mouseMove(native.data(), point(selected.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);
    QCOMPARE(owner->selectionRect(), selected);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(selected.center() + QPointF(delta)));
    QTRY_COMPARE(owner->selectionRect(), moved);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == selected);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), moved);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);

    const int history = owner->operations().undoCount();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(moved.center()));
    QTest::mouseMove(native.data(), point(moved.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved.translated(delta));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Escape);
    QTRY_COMPARE(owner->selectionRect(), moved);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == moved);
    QCOMPARE(owner->operations().undoCount(), history);


}

void KisGpuDocumentViewTest::selectionGrowAndShrinkUseGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Selection shrink")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    const QRect selected(64, 64, 128, 128);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    const auto runFilter = [&](const QString &id, int x, int y) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", false);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("shrinkselection"), 8, 4);
    const QRect shrunk(72, 68, 112, 120);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == shrunk);
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    runFilter(QStringLiteral("growselection"), 8, 4);
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->operations().undoCount(), 3);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), shrunk);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
    runFilter(QStringLiteral("shrinkselection"), 100, 100);
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QTRY_VERIFY(!lifetime->image()->globalSelection() ||
                lifetime->image()->globalSelection()->selectedExactRect().isEmpty());
    QCOMPARE(owner->operations().undoCount(), 4);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), selected);
}

void KisGpuDocumentViewTest::invertedSelectionLimitsPaintingAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Inverted selection")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    const QRect hole(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionRect(hole));
    QTRY_COMPARE(owner->selectionRect(), hole);
    KisOperationConfigurationSP invert = new KisOperationConfiguration("invertselection");
    window->viewManager()->actionManager()->runOperationFromConfiguration(invert);
    QTRY_VERIFY(owner->selectionInverted());
    QTRY_COMPARE(owner->selectionRect(), hole);
    QCOMPARE(owner->operations().undoCount(), 2);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == QRect(QPoint(), QSize(64, 64)));
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 24, 24), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 4, 4), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    {
        KisOperationConfigurationSP growInverted = new KisOperationConfiguration("growselection");
        growInverted->setProperty("x-radius", 2);
        growInverted->setProperty("y-radius", 2);
        growInverted->setProperty("edgeLock", true);
        window->viewManager()->actionManager()->runOperationFromConfiguration(growInverted);
        lifetime->image()->waitForDone();
        QTRY_COMPARE(owner->selectionRect(), hole.adjusted(2, 2, -2, -2));
        QVERIFY(owner->selectionInverted());
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->selectionRect(), hole);
        QVERIFY(owner->selectionInverted());
    }
    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *clipboardData = QGuiApplication::clipboard()->mimeData();
    QVERIFY(clipboardData);
    const QImage copiedMerged = KisClipboard::instance()->getImageWithFallback(clipboardData);
    QVERIFY(!copiedMerged.isNull());
    QCOMPARE(copiedMerged.size(), QSize(64, 64));
    QCOMPARE(copiedMerged.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedMerged.pixelColor(24, 24).alpha(), 0);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *copyData = QGuiApplication::clipboard()->mimeData();
    QVERIFY(copyData);
    const QImage copiedLayer = KisClipboard::instance()->getImageWithFallback(copyData);
    QVERIFY(!copiedLayer.isNull());
    QCOMPARE(copiedLayer.size(), QSize(64, 64));
    QCOMPARE(copiedLayer.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedLayer.pixelColor(24, 24).alpha(), 0);
    manager->cut();
    QTRY_COMPARE(owner->operations().undoCount(), 4);
    const auto cut = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cut.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cut.bytes, 4, 4).at(3)), 0);
    QCOMPARE(quint8(pixel(cut.bytes, 24, 24).at(3)), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto restored = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(restored.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("inverted-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selection, hole);
    QVERIFY(loaded.document.image.selectionInverted);
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    QVERIFY(reopened->selectionInverted());
    QCOMPARE(reopened->selectionRect(), hole);

    const auto runFilter = [&](const QString &id, int x, int y, bool edgeLock = false) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", edgeLock);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(20, 20, 24, 24));
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), hole);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::ellipticalSelectionLimitsPaintingAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Elliptical selection")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    const QRect ellipseRect(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect));
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QTRY_VERIFY(lifetime->image()->globalSelection());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 16, 16), QByteArray(4, char(0)));
    QCOMPARE(pixel(filled.bytes, 47, 47), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto undone = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(undone.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(undone.bytes, 32, 32), QByteArray(4, char(0)));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("elliptical-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selection, ellipseRect);
    QCOMPARE(loaded.document.image.selectionKind, Gpu::SelectionKind::Ellipse);
    const auto reopened = Gpu::PaintImageSP::create(execution, loaded.document.image);
    QCOMPARE(reopened->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(reopened->selectionRect(), ellipseRect);
    const auto reopenedPixels = reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(reopenedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(reopenedPixels.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(reopenedPixels.bytes, 16, 16), QByteArray(4, char(0)));

    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *ellipseClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(ellipseClipboard);
    const QImage copiedEllipse = KisClipboard::instance()->getImageWithFallback(ellipseClipboard);
    QVERIFY(!copiedEllipse.isNull());
    QCOMPARE(copiedEllipse.size(), QSize(32, 32));
    QCOMPARE(copiedEllipse.pixelColor(16, 16), QColor(17, 129, 241));
    QCOMPARE(copiedEllipse.pixelColor(0, 0).alpha(), 0);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *layerClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(layerClipboard);
    const QImage copiedEllipseLayer = KisClipboard::instance()->getImageWithFallback(layerClipboard);
    QVERIFY(!copiedEllipseLayer.isNull());
    QCOMPARE(copiedEllipseLayer.size(), QSize(32, 32));
    QCOMPARE(copiedEllipseLayer.pixelColor(0, 0).alpha(), 0);
    manager->cut();
    const auto cutEllipse = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cutEllipse.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cutEllipse.bytes, 32, 32).at(3)), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto runFilter = [&](const QString &id, int x, int y, bool edgeLock = false) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", edgeLock);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(12, 12, 40, 40));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    runFilter(QStringLiteral("invertselection"), 0, 0);
    QTRY_VERIFY(owner->selectionInverted());
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    const QRect invertedHole = owner->selectionRect();
    runFilter(QStringLiteral("growselection"), 2, 2, true);
    QTRY_COMPARE(owner->selectionRect(), invertedHole.adjusted(2, 2, -2, -2));
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 2, 2, true);
    QTRY_COMPARE(owner->selectionRect(), invertedHole);
    QVERIFY(owner->selectionInverted());
    const auto invertedPath = directory.filePath(QStringLiteral("inverted-ellipse.kra"));
    QVERIFY2(lifetime->saveAs(invertedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile invertedFile(invertedPath);
    QVERIFY(invertedFile.open(QIODevice::ReadOnly));
    const auto invertedLoaded = LibrePaint::ImportExport::Kra::RasterCodec::read(invertedFile);
    QVERIFY2(invertedLoaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(invertedLoaded.diagnostic));
    QCOMPARE(invertedLoaded.document.image.selection, ellipseRect);
    QCOMPARE(invertedLoaded.document.image.selectionKind, Gpu::SelectionKind::Ellipse);
    QVERIFY(invertedLoaded.document.image.selectionInverted);

    fill->trigger();
    QTRY_VERIFY(owner->selectionInverted());
    copyMerged->trigger();
    const auto *invertedClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(invertedClipboard);
    const QImage copiedInverted = KisClipboard::instance()->getImageWithFallback(invertedClipboard);
    QVERIFY(!copiedInverted.isNull());
    QCOMPARE(copiedInverted.size(), QSize(64, 64));
    QCOMPARE(copiedInverted.pixelColor(4, 4), QColor(17, 129, 241));
    QCOMPARE(copiedInverted.pixelColor(32, 32).alpha(), 0);
    manager->cut();
    const auto cutInverted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cutInverted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(pixel(cutInverted.bytes, 4, 4).at(3)), 0);
    QCOMPARE(pixel(cutInverted.bytes, 32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto restoredInverted = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(restoredInverted.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(restoredInverted.bytes, 4, 4), QByteArray::fromHex("1181f1ff"));

    runFilter(QStringLiteral("growselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), QRect(20, 20, 24, 24));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
    runFilter(QStringLiteral("shrinkselection"), 4, 4);
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::ellipticalToolCreatesDocumentSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Ellipse tool")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectElliptical"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectElliptical"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QRect ellipseRect(64, 64, 128, 128);
    QTest::mouseMove(native.data(), point(ellipseRect.topLeft()));
    const auto start = point(ellipseRect.topLeft());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(native.data(), point({192, 192}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
    QTRY_COMPARE(owner->selectionRect(), ellipseRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == ellipseRect);

    QVERIFY(lifetime->image()->clearGpuSelection());
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    const QRect secondRect(32, 48, 128, 152);
    QTest::mouseMove(native.data(), point(secondRect.topLeft()));
    const auto secondStart = point(secondRect.topLeft());
    QEnterEvent secondEntered(secondStart, secondStart, native->mapToGlobal(secondStart));
    QApplication::sendEvent(native.data(), &secondEntered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, secondStart);
    QTest::mouseMove(native.data(), point({160, 200}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({160, 200}));
    QTRY_COMPARE(owner->selectionRect(), secondRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);

    const QPoint delta(16, 16);
    const QRect movedSecond = secondRect.translated(delta);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(secondRect.center()));
    QTest::mouseMove(native.data(), point(secondRect.center() + QPointF(delta)), 30);
    QTRY_VERIFY(lifetime->image()->globalSelection() &&
                lifetime->image()->globalSelection()->selectedExactRect() == movedSecond);
    QCOMPARE(owner->selectionRect(), secondRect);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(secondRect.center() + QPointF(delta)));
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), secondRect);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);

    QVERIFY(lifetime->image()->setGpuSelectionEllipse(movedSecond, true));
    QTRY_VERIFY(owner->selectionInverted());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({200, 200}));
    QTest::mouseMove(native.data(), point({216, 216}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({216, 216}));
    const QRect invertedEllipse = movedSecond.translated(delta);
    QTRY_COMPARE(owner->selectionRect(), invertedEllipse);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), movedSecond);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::freehandToolCreatesArbitraryMaskSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Freehand tool")));
    QSignalSpy windowReady(KisPart::instance(), &KisPart::sigMainWindowCreated);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QTRY_VERIFY(!windowReady.isEmpty());
    const auto *converter = view->canvasBase()->coordinatesConverter();
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    view->zoomManager()->zoomTo100();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    const auto imageSize = converter->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectOutline"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectOutline"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QList<QPointF> polygon{{64, 64}, {192, 64}, {192, 128}, {128, 192}, {64, 192}};
    QTest::mouseMove(native.data(), point(polygon.first()));
    const auto start = point(polygon.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    for (qsizetype i = 1; i < polygon.size(); ++i)
        QTest::mouseMove(native.data(), point(polygon[i]), 20);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(polygon.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QVERIFY(mask->contains(80, 80));
    QVERIFY(!mask->contains(176, 176));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QTRY_VERIFY(lifetime->image()->globalSelection());
    QTRY_COMPARE(lifetime->image()->globalSelection()->selectedExactRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 2);
    const QRect region(QPoint(), QSize(256, 256));
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 256 + x) * 4, 4); };
    QCOMPARE(pixel(filled.bytes, 80, 80), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(pixel(filled.bytes, 176, 176), QByteArray(4, char(0)));

    QVERIFY(lifetime->image()->clearGpuSelection());
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QVERIFY(lifetime->image()->reselectGpuSelection());
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask());
    QVERIFY(owner->selectionMask()->contains(80, 80));
    QVERIFY(!owner->selectionMask()->contains(176, 176));
    QCOMPARE(owner->selectionRect(), mask->bounds());
}

void KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Paste image")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QImage pasted(16, 16, QImage::Format_RGBA8888);
    pasted.fill(QColor(17, 129, 241));
    QGuiApplication::clipboard()->setImage(pasted);
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->paste();
    QTRY_COMPARE(owner->operations().undoCount(), 1);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    const auto pixels = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
    int matches = 0;
    for (qsizetype offset = 0; offset + 3 < pixels.bytes.size(); offset += 4)
        if (pixels.bytes.mid(offset, 4) == QByteArray::fromHex("1181f1ff")) ++matches;
    QCOMPARE(matches, 16 * 16);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QImage large(128, 128, QImage::Format_RGBA8888);
    large.fill(QColor(241, 129, 17));
    QGuiApplication::clipboard()->setImage(large);
    manager->paste();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto cropped = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cropped.code, Gpu::ExecutionCode::Success);
    int croppedMatches = 0;
    for (qsizetype offset = 0; offset + 3 < cropped.bytes.size(); offset += 4)
        if (cropped.bytes.mid(offset, 4) == QByteArray::fromHex("f18111ff")) ++croppedMatches;
    QCOMPARE(croppedMatches, 64 * 64);
}

void KisGpuDocumentViewTest::reselectRestoresGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(128, 128));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Reselect")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    const QRect selected(16, 24, 64, 48);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    manager->deselect();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    manager->reselect();
    QTRY_COMPARE(owner->selectionRect(), selected);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Rectangle);
    const QRect ellipse(32, 40, 48, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipse, true));
    QTRY_VERIFY(owner->selectionInverted());
    manager->deselect();
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    manager->reselect();
    QTRY_COMPARE(owner->selectionRect(), ellipse);
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::arrowKeysNudgeGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(128, 128));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Nudge")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
    view->canvasBase()->canvasWidget()->setFocus();
    const QRect selected(16, 24, 48, 40);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1, 0));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Down, Qt::ShiftModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1, 10));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right, Qt::ControlModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(101, 10));
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Right,
                    Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_COMPARE(owner->selectionRect(), selected.translated(1101, 10));
    const QRect ellipse(24, 32, 40, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipse, true));
    QTRY_VERIFY(owner->selectionInverted());
    QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Left);
    QTRY_COMPARE(owner->selectionRect(), ellipse.translated(-1, 0));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    QVERIFY(owner->selectionInverted());
}

void KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255, false, false) == Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    const auto baseLayer = saved->activeLayer;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Copy selection layer")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    const QRect selected(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_selection_to_new_layer"));
    QVERIFY(action);
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const QRect region(QPoint(), QSize(64, 64));
    LibrePaint::Image::Gpu::LayerId copiedLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) copiedLayer = layer.id;
    QVERIFY(copiedLayer);
    QCOMPARE(owner->operations().activeLayer(), copiedLayer);
    const auto copiedPixels = owner->readRgbaPixels(copiedLayer, region, 8 * 1024 * 1024);
    QCOMPARE(copiedPixels.code, Gpu::ExecutionCode::Success);
    int copiedMatches = 0;
    for (qsizetype offset = 0; offset + 3 < copiedPixels.bytes.size(); offset += 4)
        if (copiedPixels.bytes.mid(offset, 4) == QByteArray::fromHex("1181f1ff")) ++copiedMatches;
    QCOMPARE(copiedMatches, 32 * 32);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QVERIFY(lifetime->image()->setGpuSelectionRect(selected, true));
    QTRY_VERIFY(owner->selectionInverted());
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId invertedLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) invertedLayer = layer.id;
    QVERIFY(invertedLayer);
    QCOMPARE(owner->operations().activeLayer(), invertedLayer);
    const auto invertedPixels = owner->readRgbaPixels(invertedLayer, region, 8 * 1024 * 1024);
    QCOMPARE(invertedPixels.code, Gpu::ExecutionCode::Success);
    int invertedMatches = 0;
    for (qsizetype offset = 0; offset + 3 < invertedPixels.bytes.size(); offset += 4)
        if (invertedPixels.bytes.mid(offset, 4) == QByteArray::fromHex("1181f1ff")) ++invertedMatches;
    QCOMPARE(invertedMatches, 64 * 64 - 32 * 32);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    const QRect ellipseRect(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Ellipse);
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId ellipseLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) ellipseLayer = layer.id;
    QVERIFY(ellipseLayer);
    QCOMPARE(owner->operations().activeLayer(), ellipseLayer);
    const auto ellipsePixels = owner->readRgbaPixels(ellipseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(ellipsePixels.code, Gpu::ExecutionCode::Success);
    const auto ellipsePixel = [&](int x, int y) { return ellipsePixels.bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(ellipsePixel(32, 32), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(ellipsePixel(16, 16), QByteArray(4, char(0)));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QVERIFY(lifetime->image()->setGpuSelectionEllipse(ellipseRect, true));
    QTRY_VERIFY(owner->selectionInverted());
    action->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    LibrePaint::Image::Gpu::LayerId invertedEllipseLayer = 0;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.name == QStringLiteral("Copied Selection")) invertedEllipseLayer = layer.id;
    QVERIFY(invertedEllipseLayer);
    QCOMPARE(owner->operations().activeLayer(), invertedEllipseLayer);
    const auto invertedEllipsePixels = owner->readRgbaPixels(invertedEllipseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(invertedEllipsePixels.code, Gpu::ExecutionCode::Success);
    const auto invertedEllipsePixel = [&](int x, int y) {
        return invertedEllipsePixels.bytes.mid((y * 64 + x) * 4, 4);
    };
    QCOMPARE(invertedEllipsePixel(32, 32), QByteArray(4, char(0)));
    QCOMPARE(invertedEllipsePixel(16, 16), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));

    auto *cutAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("cut_selection_to_new_layer"));
    QVERIFY(cutAction);
    cutAction->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto basePixels = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(basePixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(basePixels.bytes.mid((24 * 64 + 24) * 4 + 3, 1).front()), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    const auto restored = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
    QCOMPARE(restored.bytes.mid((24 * 64 + 24) * 4, 4), QByteArray::fromHex("1181f1ff"));
}

void KisGpuDocumentViewTest::ordinaryLayerChangesDuringInput()
{
    QSettings displaySettings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                              QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = displaySettings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = displaySettings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreCanvas = qScopeGuard([&] {
        if (hadRenderer) displaySettings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else displaySettings.remove(QStringLiteral("OpenGLRenderer"));
        displaySettings.sync();
    });
    KisConfig(true).disableOpenGL();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(document->newImage(QStringLiteral("Layer change during input"), 64, 64, cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<QWidget> input = view->canvasBase()->canvasWidget();
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (pressed && input) QTest::mouseRelease(input, Qt::LeftButton);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    window->viewManager()->setCurrentView(view);
    image->setWorkingThreadsLimit(1);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", 0));
    input->setFocus();
    QTRY_VERIFY(input->width() >= 64 && input->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    const auto first = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(16, 16)).toPoint();
    const auto last = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(40, 40)).toPoint();
    QEnterEvent entered(first, first, input->mapToGlobal(first));
    QApplication::sendEvent(input, &entered);
    QTest::mouseMove(input, first);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, first);
    QTest::mouseMove(input, last);
    QTest::qWait(10);
    QTimer release;
    release.setSingleShot(true);
    connect(&release, &QTimer::timeout, this, [&] {
        QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
    });
    release.start(0);
    window->viewManager()->nodeManager()->toggleVisibility();
    QVERIFY(!pressed);
    QCOMPARE(lifetime->undoStack()->count(), 2);
    release.stop();
    if (pressed) QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    image->waitForDone();
    QByteArray stroke(64 * 64 * 4, '\0');
    QVERIFY(!layer->visible());
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(stroke.data()), image->bounds());
    // The interior is opaque; endpoint coverage depends on display-coordinate rounding.
    QCOMPARE(stroke.mid((28 * 64 + 28) * 4, 4), QByteArray::fromHex("f18111ff"));
    lifetime->undoStack()->undo();
    image->waitForDone();
    QVERIFY(layer->visible());
    QByteArray pixels(stroke.size(), '\0');
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
    QCOMPARE(pixels, stroke);
    lifetime->undoStack()->undo();
    image->waitForDone();
    QCOMPARE(lifetime->undoStack()->index(), 0);
    layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
    QCOMPARE(pixels, QByteArray(stroke.size(), '\0'));
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data()
{
    QTest::addColumn<int>("smoothingType");
    QTest::addColumn<bool>("idlePause");
    QTest::addColumn<bool>("cancelAtIdle");
    QTest::addColumn<bool>("exhaustInput");
    QTest::addColumn<bool>("closeAfterError");
    QTest::addColumn<bool>("hideDuringStroke");
    QTest::addColumn<bool>("selectDuringStroke");
    QTest::addColumn<bool>("alphaLock");
    QTest::addColumn<bool>("layerAlphaLock");
    QTest::addColumn<quint8>("mirrorMode");
    QTest::newRow("paint-and-cancel") << 0 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("input-limit") << 0 << false << false << true << false << false << false << false << false << quint8(0);
    QTest::newRow("close-on-input-limit") << 0 << false << false << true << true << false << false << false << false << quint8(0);
    QTest::newRow("hide-during-stroke") << 0 << false << false << false << false << true << false << false << false << quint8(0);
    QTest::newRow("select-during-stroke") << 0 << false << false << false << false << false << true << false << false << quint8(0);
    QTest::newRow("alpha-lock") << 0 << false << false << false << false << false << false << true << false << quint8(0);
    QTest::newRow("layer-alpha-lock") << 0 << false << false << false << false << false << false << true << true << quint8(0);
    QTest::newRow("horizontal-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(1);
    QTest::newRow("vertical-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(2);
    QTest::newRow("both-axes-symmetry") << 0 << false << false << false << false << false << false << false << false << quint8(3);
    QTest::newRow("pixel-perfect") << 4 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing") << 1 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing-idle") << 1 << true << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("basic-smoothing-pending-cancel") << 1 << false << true << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing") << 2 << false << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-idle") << 2 << true << false << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-pending-cancel") << 2 << false << true << false << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-input-limit") << 2 << false << false << true << false << false << false << false << false << quint8(0);
    QTest::newRow("weighted-smoothing-close-on-limit") << 2 << false << false << true << true << false << false << false << false << quint8(0);
}

void KisGpuDocumentViewTest::freehandInputCommitsOneHistoryEntryAndCanBeCancelled()
{
    QFETCH(int, smoothingType);
    QFETCH(bool, idlePause);
    QFETCH(bool, cancelAtIdle);
    QFETCH(bool, exhaustInput);
    QFETCH(bool, closeAfterError);
    QFETCH(bool, hideDuringStroke);
    QFETCH(bool, selectDuringStroke);
    QFETCH(bool, alphaLock);
    QFETCH(bool, layerAlphaLock);
    QFETCH(quint8, mirrorMode);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Limits limits;
    if (exhaustInput) limits.samplesPerStroke = 4;
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64), limits);
    if (selectDuringStroke) QVERIFY(owner->addLayer(QStringLiteral("Paint target")));
    const auto paintTarget = owner->operations().activeLayer();
    if (alphaLock) QCOMPARE(owner->writeRgbaPixels(paintTarget, QRect(40, 40, 1, 1),
                                                QByteArray::fromHex("dd112c80")), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU freehand input")));
    if (layerAlphaLock) qobject_cast<KisPaintLayer*>(document->image()->root()->firstChild().data())->setAlphaLocked(true);
    if (mirrorMode) {
        KisMirrorAxisConfig axes;
        axes.setAxisPosition(QPointF(32, 32));
        axes.setMirrorHorizontal(mirrorMode & 1);
        axes.setMirrorVertical(mirrorMode & 2);
        document->setMirrorAxisConfig(axes);
    }
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (documentLifetime) documentLifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (documentLifetime) {
            documentLifetime->setModified(false);
            auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
            if (close) close->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->statusBar()->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    QVERIFY(profiles->currentProfile());
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setGlobalAlphaLock(alphaLock && !layerAlphaLock);
    resources->setMirrorHorizontal(mirrorMode & 1);
    resources->setMirrorVertical(mirrorMode & 2);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QVERIFY(brushTool->setProperty("smoothingType", smoothingType));
    if (smoothingType == 2) {
        QVERIFY(brushTool->setProperty("smoothnessQualityMin", 12));
        QVERIFY(brushTool->setProperty("smoothnessQualityMax", 24));
        QVERIFY(brushTool->setProperty("smoothnessFactor", 0.25));
        QVERIFY(brushTool->setProperty("smoothPressure", true));
        QVERIFY(brushTool->setProperty("useScalableDistance", false));
    }
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
    if (closeAfterError) {
        connect(brushTool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
            if (message.isEmpty()) return;
            documentLifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
    }
    view->canvasBase()->canvasWidget()->setFocus();
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64 && view->canvasBase()->canvasWidget()->height() >= 64);
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    const auto *converter = view->canvasBase()->coordinatesConverter();
    const auto enterAt = [&](QPointF imagePoint) {
        QTest::mouseMove(native.data(), converter->imageToWidget(imagePoint).toPoint());
        QTest::qWait(10);
        const auto point = converter->imageToWidget(imagePoint).toPoint();
        QEnterEvent entered(point, point, native->mapToGlobal(point));
        QApplication::sendEvent(native.data(), &entered);
        return point;
    };
    const auto first = enterAt({16, 16});
    auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));
    const auto beforeStroke = owner->operations().snapshot();
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a stroke" : qPrintable(errors.first().first().toString()));
    QTRY_VERIFY(!documentLifetime || owner->execution().published(owner->handle()) == owner->operations().preview());
    if (!documentLifetime) {
        QVERIFY(closeAfterError);
        QVERIFY(!native);
        QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        return;
    }
    if (smoothingType == 2) {
        QVERIFY(brushTool->setProperty("smoothnessQualityMin", 80));
        QVERIFY(brushTool->setProperty("smoothnessQualityMax", 160));
        QVERIFY(brushTool->setProperty("smoothnessFactor", 0.75));
        QVERIFY(brushTool->setProperty("smoothPressure", false));
        QVERIFY(brushTool->setProperty("useScalableDistance", true));
    }
    if (smoothingType != 0) QVERIFY(brushTool->setProperty("smoothingType", 3));
    if (!exhaustInput) {
        if (mirrorMode) {
            auto axes = documentLifetime->mirrorAxisConfig();
            axes.setAxisPosition(QPointF(8, 8));
            axes.setMirrorHorizontal(false);
            axes.setMirrorVertical(false);
            documentLifetime->setMirrorAxisConfig(axes);
            resources->setMirrorHorizontal(false);
            resources->setMirrorVertical(false);
        }
        if (alphaLock) resources->setGlobalAlphaLock(false);
        resources->setSize(16);
        resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    }
    QTest::qWait(10);
    last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    if (exhaustInput) {
        for (int step = 1; step <= 8 && owner->operations().hasActiveStroke(); ++step)
            QTest::mouseMove(native.data(), first + (last - first) * step / 8);
    } else {
        if (smoothingType == 2) {
            for (const auto point : {QPointF(20, 20), QPointF(24, 24), QPointF(28, 28), QPointF(32, 32)})
                QTest::mouseMove(native.data(), converter->imageToWidget(point).toPoint());
        }
        QTest::mouseMove(native.data(), last);
    }
    QTest::qWait(10);
    if (cancelAtIdle) {
        view->image()->requestStrokeCancellation();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
        QTest::qWait(150);
        QCOMPARE(owner->operations().snapshot(), beforeStroke);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QVERIFY(!documentLifetime->isModified());
        QTRY_COMPARE(owner->execution().published(owner->handle()), beforeStroke);
        return;
    }
    if (idlePause) {
        QTRY_VERIFY(!std::get<Gpu::PaintSnapshot>(owner->operations().preview()->layers.first().operations.last())->smoothingFlushes.isEmpty());
        last = converter->imageToWidget(QPointF(48, 16)).toPoint();
        QTest::mouseMove(native.data(), last);
    }
    if (selectDuringStroke) {
        const auto other = documentLifetime->image()->root()->firstChild();
        window->viewManager()->nodeManager()->slotUiActivatedNode(other);
        QCOMPARE(view->currentNode(), other);
        QCOMPARE(owner->operations().activeLayer(), owner->operations().snapshot()->layers.first().id);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
        pressed = false;
        QVERIFY(!owner->operations().hasActiveStroke());
        QCOMPARE(owner->operations().undoCount(), 2);
        QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                 alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        documentLifetime->undoStack()->undo();
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(view->currentNode(), other);
        QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        return;
    }
    if (hideDuringStroke) {
        auto *manager = window->viewManager()->nodeManager();
        QTimer release;
        release.setSingleShot(true);
        connect(&release, &QTimer::timeout, this, [&] {
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
            pressed = false;
        });
        release.start(0);
        try {
            manager->toggleVisibility();
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
        QVERIFY(!owner->operations().hasActiveStroke());
        QVERIFY(!pressed);
        QCOMPARE(owner->operations().undoCount(), 2);
        QVERIFY(!manager->activeNode()->visible());
        QVERIFY(documentLifetime->isModified());
        documentLifetime->undoStack()->undo();
        QVERIFY(manager->activeNode()->visible());
        QCOMPARE(owner->operations().undoCount(), 1);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
        documentLifetime->undoStack()->undo();
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!documentLifetime->isModified());
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray(4, '\0'));
        return;
    }
    if (closeAfterError) {
        QVERIFY(!documentLifetime);
        QVERIFY(!native);
        QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!owner->operations().hasActiveStroke());
        QStringList reasons;
        for (const auto &notification : errors)
            if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
        QCOMPARE(reasons.size(), 1);
        return;
    }
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QVERIFY(!owner->operations().hasActiveStroke());
    if (exhaustInput) {
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(!documentLifetime->isModified());
        QCOMPARE(errors.size(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(window->statusBar()->currentMessage(), errors.first().first().toString());
        QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
        return;
    }
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto painted = owner->operations().snapshot();
    const auto operation = std::get<Gpu::PaintSnapshot>(painted->layers.first().operations.last());
    QCOMPARE(operation->brush.pixelPerfect, smoothingType == 4);
    QCOMPARE(operation->brush.basicSmoothing, smoothingType == 1 || smoothingType == 2);
    QCOMPARE(bool(operation->brush.weightedSmoothing), smoothingType == 2);
    if (smoothingType == 2) {
        QVERIFY(operation->inputSamples().size() >= 6);
        const auto &weighted = *operation->brush.weightedSmoothing;
        QCOMPARE(weighted.distanceMin, 12.0);
        QCOMPARE(weighted.distanceMax, 24.0);
        QCOMPARE(weighted.tailAggressiveness, 0.25);
        QVERIFY(weighted.smoothPressure);
    }
    if (smoothingType == 4) qInfo() << "Pixel-corrected pointer input:" << operation->inputSamples().last().position;
    if (mirrorMode) {
        for (const auto &sample : QList<QPair<QPoint, bool>>{{{48, 16}, bool(mirrorMode & 1)},
             {{16, 48}, bool(mirrorMode & 2)}, {{48, 48}, mirrorMode == 3}}) {
            QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(sample.first, QSize(1, 1)), 4).bytes,
                     sample.second ? QByteArray::fromHex("1181f1ff") : QByteArray(4, '\0'));
        }
    }
    QTRY_COMPARE(owner->execution().published(owner->handle()), painted);
    if (alphaLock) QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("1181f100"));
    // Weighted smoothing moves the endpoint according to the measured input speed.
    // The earlier interior sample is opaque for every accepted input timing.
    const QPoint paintedProbe = smoothingType == 2 ? QPoint(24, 24) : QPoint(40, 40);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(paintedProbe, QSize(1, 1)), 4).bytes,
             alphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f1ff"));
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(mirrorMode ? 56 : smoothingType != 0 ? 48 : 45, 40, 1, 1), 4).bytes,
             QByteArray::fromHex("00000000"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().undoCount(), 0);
    if (alphaLock) QCOMPARE(owner->readRgbaPixels(paintTarget, QRect(40, 40, 1, 1), 4).bytes,
                            QByteArray::fromHex("dd112c80"));
    documentLifetime->undoStack()->redo();
    QCOMPARE(owner->operations().snapshot(), painted);

    QVERIFY(brushTool->setProperty("smoothingType", smoothingType));
    resources->setSize(8);
    resources->setGlobalAlphaLock(alphaLock && !layerAlphaLock);
    resources->setEraserMode(true);
    last = enterAt(paintedProbe);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The erase press did not start a stroke" : qPrintable(errors.last().first().toString()));
    resources->setEraserMode(false);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QCOMPARE(owner->operations().undoCount(), 2);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(paintedProbe, QSize(1, 1)), 4).bytes,
             layerAlphaLock ? QByteArray::fromHex("1181f180") : QByteArray::fromHex("1181f100"));
    documentLifetime->undoStack()->undo();
    QCOMPARE(owner->operations().snapshot(), painted);

    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    const auto cancelled = enterAt({48, 16});
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    QVERIFY(owner->operations().hasActiveStroke());
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().preview());
    view->image()->requestStrokeCancellation();
    QVERIFY(!owner->operations().hasActiveStroke());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, cancelled);
    pressed = false;
    QCOMPARE(owner->operations().snapshot(), painted);
    QCOMPARE(owner->operations().undoCount(), 1);
    QTRY_COMPARE(owner->execution().published(owner->handle()), painted);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("ordinary-document.kra"));
    documentLifetime->setFileBatchMode(true);
    QVERIFY2(documentLifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(documentLifetime->errorMessage()));
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 8, 1, 1), QByteArray::fromHex("112233ff")), Gpu::Result::Success);
    auto *save = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_save"));
    QVERIFY(save);
    QTRY_VERIFY(save->isEnabled());
    QSignalSpy saved(documentLifetime.data(), &KisDocument::sigSavingFinished);
    save->trigger();
    QTRY_COMPARE(saved.size(), 1);
    QVERIFY(!documentLifetime->isModified());
    QCOMPARE(documentLifetime->path(), path);
    QCOMPARE(owner->operations().undoCount(), 2);
    documentLifetime->undoStack()->undo();
    QVERIFY(documentLifetime->isModified());
    documentLifetime->undoStack()->redo();
    QVERIFY(!documentLifetime->isModified());
    if (layerAlphaLock) {
        QFile savedFile(path);
        QVERIFY(savedFile.open(QIODevice::ReadOnly));
        const auto reopened = LibrePaint::ImportExport::Kra::RasterCodec::read(savedFile);
        QVERIFY2(reopened.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(reopened.diagnostic));
        QCOMPARE(reopened.document.image.layers.first().channelWriteMask, quint8(7));
        const auto imported = Gpu::PaintImageSP::create(execution, reopened.document.image);
        const auto image = KisImage::fromGpuImage(imported, QStringLiteral("Protected layer reopened"));
        QVERIFY(qobject_cast<KisPaintLayer*>(image->root()->firstChild().data())->alphaLocked());
        Gpu::Brush additional;
        additional.rgba = {10, 20, 30, 255};
        additional.diameter = 8;
        const auto stroke = imported->beginStroke(imported->operations().activeLayer(), additional, 0);
        QCOMPARE(imported->appendSamples(stroke, {{{40, 40}, 1}}), Gpu::Result::Success);
        QCOMPARE(imported->commitStroke(stroke), Gpu::Result::Success);
        QCOMPARE(imported->readRgbaPixels(imported->operations().activeLayer(), QRect(40, 40, 1, 1), 4).bytes,
                 QByteArray::fromHex("0a141e80"));
    }
}

void KisGpuDocumentViewTest::paintingBackendChangesRebindTheCanvas_data()
{
    QTest::addColumn<bool>("initialGpu");
    QTest::addColumn<bool>("replacementGpu");
    QTest::newRow("cpu-to-gpu") << false << true;
    QTest::newRow("gpu-to-cpu") << true << false;
    QTest::newRow("cpu-to-cpu") << false << false;
}

void KisGpuDocumentViewTest::paintingBackendChangesRebindTheCanvas()
{
    QFETCH(bool, initialGpu);
    QFETCH(bool, replacementGpu);
    namespace Gpu = LibrePaint::Image::Gpu;
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
                       QStringLiteral("/kritadisplayrc"), QSettings::IniFormat);
    const bool hadRenderer = settings.contains(QStringLiteral("OpenGLRenderer"));
    const auto previousRenderer = settings.value(QStringLiteral("OpenGLRenderer"));
    const auto restoreRenderer = qScopeGuard([&] {
        if (hadRenderer) settings.setValue(QStringLiteral("OpenGLRenderer"), previousRenderer);
        else settings.remove(QStringLiteral("OpenGLRenderer"));
        settings.sync();
    });
    KisConfig(true).disableOpenGL();
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    const auto cs = KoColorSpaceRegistry::instance()->rgb8();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    if (initialGpu) {
        document->setCurrentImage(KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, QSize(64, 64)), QStringLiteral("Initial image")));
    } else {
        QVERIFY(document->newImage(QStringLiteral("Initial image"), 64, 64, cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
    }
    const KisImageWSP oldIdentity = document->image();
    connect(document.get(), &KisDocument::sigCurrentImageAboutToChange, this, [oldIdentity] {
        std::thread notification([oldIdentity] {
            auto image = oldIdentity.toStrongRef();
            image->sigSizeChanged({12, 12}, {16, 16});
        });
        notification.join();
    });
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<QWindow> native;
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (lifetime) lifetime->image()->requestStrokeCancellation();
        if (pressed) QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    auto oldImage = lifetime->image().toStrongRef();
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
    QPointer<QWindow> oldNative;
    if (initialGpu) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) oldNative = candidate;
        QVERIFY(oldNative);
    }
    window->viewManager()->nodeManager()->setNodeName(oldImage->root()->firstChild(), QStringLiteral("Edited old image"));
    oldImage->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), 1);
    KisImageSP replacement;
    if (replacementGpu) {
        replacement = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, QSize(96, 80)), QStringLiteral("Replacement image"));
    } else {
        std::unique_ptr<KisDocument> fixture(KisPart::instance()->createDocument());
        QVERIFY(fixture->newImage(QStringLiteral("Replacement image"), 96, 80, cs,
                                  KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        replacement = fixture->image()->clone();
    }
    const auto seed = QByteArray::fromHex("cc8844ff");
    replacement->root()->firstChild()->paintDevice()->writeBytes(reinterpret_cast<const quint8*>(seed.constData()), 8, 8, 1, 1);
    QSignalSpy sizes(view->canvasBase(), &KisCanvas2::sigViewImageSizeChanged);
    lifetime->setCurrentImage(replacement);
    replacement.clear();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QCOMPARE(sizes.size(), 0);
    QCOMPARE(lifetime->undoStack()->count(), 0);
    QCOMPARE(view->canvasBase()->coordinatesConverter()->imageRectInImagePixels(), QRect(0, 0, 96, 80));
    QTRY_COMPARE(view->currentNode(), lifetime->image()->root()->firstChild());
    if (initialGpu) QTRY_VERIFY(!oldNative);
    if (replacementGpu) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = candidate;
        QVERIFY(native);
        const auto owner = lifetime->image()->gpuImage();
        QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
    }
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brush && brush->setProperty("smoothingType", 0));
    QPointer<QWidget> input(view->canvasBase()->canvasWidget());
    input->setFocus();
    QTRY_VERIFY(!view->canvasBase()->coordinatesConverter()->getCanvasWidgetSize().isEmpty());
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto size = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({size.width() / 2, size.height() / 2});
    const auto first = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(16, 16)).toPoint();
    const auto last = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(40, 40)).toPoint();
    QEnterEvent entered(first, first, native ? native->mapToGlobal(first) : input->mapToGlobal(first));
    QApplication::sendEvent(native ? static_cast<QObject*>(native.data()) : input.data(), &entered);
    const auto before = lifetime->undoStack()->count();
    pressed = true;
    if (native) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
        QTest::mouseMove(native.data(), last);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    } else {
        QTest::mousePress(input.data(), Qt::LeftButton, Qt::NoModifier, first);
        QMouseEvent moved(QEvent::MouseMove, last, input->mapToGlobal(last), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(input.data(), &moved);
        QTest::mouseRelease(input.data(), Qt::LeftButton, Qt::NoModifier, last);
    }
    pressed = false;
    lifetime->image()->waitForDone();
    QTRY_COMPARE(lifetime->undoStack()->count(), before + 1);
    const auto pixel = [](KisImageWSP image, QPoint position) {
        QByteArray result(4, '\0');
        image->root()->firstChild()->paintDevice()->readBytes(reinterpret_cast<quint8*>(result.data()), position.x(), position.y(), 1, 1);
        return result;
    };
    QCOMPARE(pixel(lifetime->image(), {28, 28}), QByteArray::fromHex("f18111ff"));
    QCOMPARE(pixel(oldImage, {28, 28}), QByteArray(4, '\0'));
    lifetime->undoStack()->undo();
    lifetime->image()->waitForDone();
    QCOMPARE(pixel(lifetime->image(), {28, 28}), QByteArray(4, '\0'));
    QCOMPARE(pixel(lifetime->image(), {8, 8}), seed);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("replacement.kra"));
    QSignalSpy savingFinished(lifetime, &KisDocument::sigSavingFinished);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QTRY_COMPARE(savingFinished.size(), 1);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto saved = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(saved.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(saved.diagnostic));
    QCOMPARE(saved.document.image.size, QSize(96, 80));
    QCOMPARE(saved.document.image.layers.first().tiles.first().rgba.mid((8 * 256 + 8) * 4, 4), QByteArray::fromHex("4488ccff"));
}

void KisGpuDocumentViewTest::replacementUpdatesPresentationInputHistoryAndSaving_data()
{
    QTest::addColumn<bool>("replaceIdentity");
    QTest::addColumn<bool>("differentExecution");
    QTest::addColumn<bool>("replaceDuringInput");
    QTest::addColumn<int>("closePhase");
    QTest::newRow("copy-keeps-image-identity") << false << false << false << 0;
    QTest::newRow("set-current-image-replaces-identity") << true << false << false << 0;
    QTest::newRow("set-current-image-changes-execution") << true << true << false << 0;
    QTest::newRow("set-current-image-cancels-old-input") << true << false << true << 0;
    QTest::newRow("close-before-adoption") << true << false << false << 1;
    QTest::newRow("close-after-adoption") << true << false << false << 2;
}

void KisGpuDocumentViewTest::replacementUpdatesPresentationInputHistoryAndSaving()
{
    QFETCH(bool, replaceIdentity);
    QFETCH(bool, differentExecution);
    QFETCH(bool, replaceDuringInput);
    QFETCH(int, closePhase);
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    Gpu::Execution otherExecution(device);
    const auto previous = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto sourceOwner = Gpu::PaintImageSP::create(differentExecution ? otherExecution : execution, replaceIdentity ? QSize(96, 80) : QSize(64, 64));
    const auto layer = sourceOwner->operations().activeLayer();
    QCOMPARE(sourceOwner->replaceRgbaPixels(layer, QRect(8, 8, 1, 1), QByteArray::fromHex("4488ccff")), Gpu::Result::Success);
    QScopedPointer<KisDocument> source(KisPart::instance()->createDocument());
    source->setCurrentImage(KisImage::fromGpuImage(sourceOwner, QStringLiteral("Replacement source")));
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(previous, QStringLiteral("Replacement target")));
    const KisImageWSP previousIdentity = document->image();
    connect(document.get(), &KisDocument::sigCurrentImageAboutToChange, this, [previousIdentity, replaceIdentity, closePhase] {
        if (replaceIdentity && closePhase == 0) {
            auto image = previousIdentity.toStrongRef();
            image->sigRequestNodeReselection(image->root()->firstChild(), {image->root()->firstChild()});
        }
    });
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed) {
            if (documentLifetime) documentLifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native ? native.data() : window->windowHandle(), Qt::LeftButton);
        }
        if (documentLifetime) {
            documentLifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    window->activateWindow();
    QTRY_COMPARE(execution.published(previous->handle()), previous->operations().snapshot());
    if (replaceDuringInput) {
        auto *profiles = KisInputProfileManager::instance();
        profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                     QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
        profiles->loadProfiles();
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(m_preset);
        resources->setSize(8);
        resources->setOpacity(1);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
        QVERIFY(brush && brush->setProperty("smoothingType", 0));
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
        QVERIFY(native);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
        QEnterEvent entered(point, point, native->mapToGlobal(point));
        QApplication::sendEvent(native.data(), &entered);
        QTest::mouseMove(native.data(), point);
        QTest::qWait(10);
        pressed = true;
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(previous->operations().hasActiveStroke());
    }
    auto *identity = view->image().data();
    KisImageSP retainedPrevious = replaceIdentity && closePhase == 0 ? view->image().toStrongRef() : KisImageSP();
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    if (replaceIdentity) {
        auto replacement = KisImage::fromGpuImage(sourceOwner->clone(), QStringLiteral("Replacement image"));
        const auto adopted = replacement->gpuImage();
        if (closePhase) {
            const auto signal = closePhase == 1 ? &KisDocument::sigCurrentImageAboutToChange : &KisDocument::sigCurrentImageChanged;
            connect(documentLifetime, signal, window.get(), [&, view] {
                documentLifetime->setModified(false);
                view->closeView();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
        }
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
        documentLifetime->setCurrentImage(replacement);
        replacement.clear();
        if (closePhase) {
            QTRY_VERIFY(!documentLifetime);
            QCOMPARE(adopted->operations().undoCount(), 0);
            QVERIFY(!adopted->operations().hasActiveStroke());
            QCOMPARE(adopted->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("4488ccff"));
            QCOMPARE(sourceOwner->operations().undoCount(), 1);
            return;
        }
        QVERIFY(view->image().data() != identity);
    } else {
        documentLifetime->copyFromDocument(*source);
        QCOMPARE(view->image().data(), identity);
    }
    const auto copied = view->image()->gpuImage();
    QVERIFY(copied != previous);
    QVERIFY(copied != sourceOwner);
    QTRY_COMPARE(copied->execution().published(copied->handle()), copied->operations().snapshot());
    QCOMPARE(view->canvasBase()->coordinatesConverter()->imageRectInImagePixels(), QRect(QPoint(), sourceOwner->operations().snapshot()->size));
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *brush = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brush);
    QVERIFY(brush->setProperty("smoothingType", 0));
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate)) native = canvas;
    QVERIFY(native);
    if (replaceDuringInput) {
        QVERIFY(!previous->operations().hasActiveStroke());
        QCOMPARE(previous->operations().undoCount(), 0);
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point);
        pressed = false;
        QCOMPARE(copied->operations().undoCount(), 0);
    }
    QTRY_VERIFY(view->currentNode());
    QTRY_COMPARE(view->currentNode()->uuid(), copied->operations().snapshot()->layers.first().uuid);
    if (retainedPrevious) {
        const auto active = view->currentNode();
        QSignalSpy sizes(view->canvasBase(), &KisCanvas2::sigViewImageSizeChanged);
        QSignalSpy activation(window->viewManager()->nodeManager(), &KisNodeManager::sigNodeActivated);
        retainedPrevious->sigSizeChanged({}, {});
        retainedPrevious->sigRequestNodeReselection(retainedPrevious->root()->firstChild(), {retainedPrevious->root()->firstChild()});
        QApplication::sendPostedEvents();
        QApplication::processEvents();
        QCOMPARE(sizes.size(), 0);
        QCOMPARE(activation.size(), 0);
        QCOMPARE(view->currentNode(), active);
    }
    const auto point = view->canvasBase()->coordinatesConverter()->imageToWidget(QPointF(32, 32)).toPoint();
    QEnterEvent entered(point, point, native->mapToGlobal(point));
    QApplication::sendEvent(native.data(), &entered);
    QTest::mouseMove(native.data(), point);
    QTest::qWait(10);
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    QVERIFY(copied->operations().hasActiveStroke());
    QVERIFY(!previous->operations().hasActiveStroke());
    if (retainedPrevious) {
        retainedPrevious->requestStrokeCancellation();
        retainedPrevious->requestStrokeEnd();
        QApplication::sendPostedEvents();
        QApplication::processEvents();
        QVERIFY(copied->operations().hasActiveStroke());
    }
    auto *undo = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    QVERIFY(undo);
    QTRY_VERIFY(undo->isEnabled());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    pressed = false;
    QCOMPARE(copied->operations().undoCount(), 1);
    documentLifetime->undoStack()->undo();
    QCOMPARE(copied->operations().undoCount(), 0);
    QCOMPARE(copied->readRgbaPixels(layer, QRect(32, 32, 1, 1), 4).bytes, QByteArray::fromHex("00000000"));
    QCOMPARE(sourceOwner->operations().undoCount(), 1);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("replacement.kra"));
    documentLifetime->setFileBatchMode(true);
    QVERIFY(documentLifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.layers.first().tiles.first().rgba.mid((8 * 256 + 8) * 4, 4), QByteArray::fromHex("4488ccff"));
}


void KisGpuDocumentViewTest::canvasBackgroundFollowsPreferencesWithoutEditing_data()
{
    QTest::addColumn<bool>("scrollCheckers");
    QTest::addColumn<int>("secondAlpha");
    QTest::addColumn<bool>("imageLimitPan");
    QTest::newRow("fixed-opaque") << false << 255 << false;
    QTest::newRow("scrolling-opaque") << true << 255 << false;
    QTest::newRow("fixed-translucent") << false << 128 << false;
    QTest::newRow("scrolling-translucent") << true << 128 << false;
    QTest::newRow("scrolling-image-limit") << true << 128 << true;
}

void KisGpuDocumentViewTest::canvasBackgroundFollowsPreferencesWithoutEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, scrollCheckers);
    QFETCH(int, secondAlpha);
    QFETCH(bool, imageLimitPan);
    KisConfig config(false);
    const auto oldFirst = config.checkersColor1();
    const auto oldSecond = config.checkersColor2();
    const auto oldBorder = config.canvasBorderColor();
    const auto oldSize = config.checkSize();
    const auto oldScrolling = config.scrollCheckers();
    const auto oldVastScrolling = config.vastScrolling();
    const auto oldHideScrollbars = config.hideScrollbars();
    const auto restoreConfig = qScopeGuard([&] {
        config.setCheckersColor1(oldFirst);
        config.setCheckersColor2(oldSecond);
        config.setCanvasBorderColor(oldBorder);
        config.setCheckSize(oldSize);
        config.setScrollingCheckers(oldScrolling);
        config.setVastScrolling(oldVastScrolling);
        config.setHideScrollbars(oldHideScrollbars);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    // Scrollbar overlays are not part of the canvas background contract.
    config.setHideScrollbars(true);
    QColor first(24, 68, 116, 128);
    QColor second(173, 209, 67, secondAlpha);
    QColor border(9, 33, 57);
    int checkSize = imageLimitPan ? 1 : 8;
    if (imageLimitPan) config.setVastScrolling(1);
    const auto configure = [&] {
        config.setCheckersColor1(first);
        config.setCheckersColor2(second);
        config.setCanvasBorderColor(border);
        config.setCheckSize(checkSize);
        config.setScrollingCheckers(scrollCheckers);
    };
    configure();
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(imageLimitPan ? Gpu::RgbaTile::CoordinateLimit : 128, 128));
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    const auto recovery = owner->operations().recoveryData();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU canvas background")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    if (imageLimitPan) view->canvasBase()->canvasWidget()->setFixedSize(128, 128);
    window->show();
    window->activateWindow();
    KoToolManager::instance()->switchToolRequested(QStringLiteral("PanTool"));
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto *converter = view->canvasBase()->coordinatesConverter();
    view->canvasController()->setPreferredCenter(converter->imageRectInWidgetPixels().center());
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    const auto findNative = [&] {
        native.clear();
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
                if (canvas->isVisible()) native = canvas;
        if (native) native->setCursor(Qt::BlankCursor);
        return bool(native);
    };
    QTRY_VERIFY(findNative());
    const auto reference = [&] {
        const auto ratio = native->devicePixelRatio();
        QImage result(QSizeF(native->size() * ratio).toSize(), QImage::Format_ARGB32_Premultiplied);
        result.setDevicePixelRatio(ratio);
        result.fill(border);
        QImage checks(2 * checkSize, 2 * checkSize, QImage::Format_ARGB32_Premultiplied);
        // The configured border supplies a defined base for translucent checks.
        checks.fill(border);
        QPainter pattern(&checks);
        pattern.fillRect(checks.rect(), second);
        pattern.fillRect(0, 0, checkSize, checkSize, first);
        pattern.fillRect(checkSize, checkSize, checkSize, checkSize, first);
        pattern.end();
        QTransform transform;
        QPointF origin;
        QPolygonF unusedPolygon;
        converter->getQPainterCheckersInfo(&transform, &origin, &unusedPolygon, scrollCheckers);
        const auto physicalToLogical = QTransform::fromScale(1.0 / ratio, 1.0 / ratio);
        const auto toDocument = physicalToLogical * converter->imageToWidgetTransform().inverted();
        const auto toPattern = physicalToLogical * transform.inverted();
        const QRgb colors[] = {checks.pixel(0, 0), checks.pixel(checkSize, 0)};
        const auto outside = qPremultiply(border.rgba());
        // Native document and pattern bounds sample physical pixel centers.
        for (int y = 0; y < result.height(); ++y) {
            auto *row = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                const QPointF center(x + 0.5, y + 0.5);
                const auto point = toDocument.map(center);
                row[x] = outside;
                if (point.x() >= 0 && point.y() >= 0 && point.x() < snapshot->size.width() && point.y() < snapshot->size.height()) {
                    const auto patternPoint = toPattern.map(center) - origin;
                    const auto column = qint64(std::floor(patternPoint.x() / checkSize));
                    const auto patternRow = qint64(std::floor(patternPoint.y() / checkSize));
                    row[x] = colors[(column ^ patternRow) & 1];
                }
            }
        }
        return result.convertToFormat(QImage::Format_RGBA8888);
    };
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path());
    QString stage = QStringLiteral("initial");
    QImage actual, expected;
    const auto saveFrames = [&] {
        if (!QDir().mkpath(directory)) return false;
        const auto stem = QDir(directory).filePath(QStringLiteral("ordinary-canvas-background-") +
            QString::fromLatin1(QTest::currentDataTag()) + QLatin1Char('-') + stage);
        return actual.save(stem + QStringLiteral("-actual.png")) && expected.save(stem + QStringLiteral("-expected.png"));
    };
    const auto preserveFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            temporary.setAutoRemove(false);
            saveFrames();
            qInfo() << "Canvas background actual/expected artifacts at" << directory;
        }
    });
    const auto matches = [&] {
        if (!findNative()) return false;
        const auto frame = native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888);
        if (frame.isNull()) return false;
        // The native child's outer two physical pixels can contain the window
        // shadow. Compare the displayed canvas content and its border.
        const QRect interior = frame.rect().adjusted(2, 2, -2, -2);
        actual = frame.copy(interior);
        expected = reference().copy(interior);
        expected.setDevicePixelRatio(actual.devicePixelRatio());
        expected.setColorSpace(actual.colorSpace());
        return actual == expected;
    };
    if (imageLimitPan) {
        const auto center = converter->widgetCenterPoint();
        view->canvasController()->setPreferredCenter(center + QPointF(converter->imageRectInWidgetPixels().width() - 32, 0));
        QTRY_COMPARE(converter->imageRectInWidgetPixels().right(), qreal(32));
        QTRY_COMPARE(native->size(), QSize(128, 128));
    }
    QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
    QVERIFY(saveFrames());
    stage = QStringLiteral("pan");
    view->canvasController()->pan(QPoint(13, 7));
    QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
    QVERIFY(saveFrames());
    if (!imageLimitPan) {
        stage = QStringLiteral("zoom");
        view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
        stage = QStringLiteral("rotation");
        view->canvasController()->rotateCanvas(23);
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
        stage = QStringLiteral("preferences");
        first = QColor(200, 50, 70, 128);
        second = QColor(70, 120, 190, secondAlpha);
        border = QColor(31, 19, 73);
        checkSize = 16;
        configure();
        KisConfigNotifier::instance()->notifyConfigChanged();
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 30000);
        QVERIFY(saveFrames());
    }
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().recoveryData(), recovery);
    QCOMPARE(owner->operations().undoCount(), 0);
    const auto pixels = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), 65536);
    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixels.bytes, QByteArray(65536, char(0)));
    QVERIFY(!owner->operations().isModified());
    QVERIFY(!lifetime->isModified());
}

void KisGpuDocumentViewTest::brushOutlineFollowsTheOrdinaryPointerWithoutEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig config(false);
    const auto oldStyle = config.newOutlineStyle();
    const auto oldColor = config.getCursorMainColor();
    const auto oldSeparate = config.separateEraserCursor();
    const auto oldFullSize = config.forceAlwaysFullSizedOutline();
    const auto oldMinimum = config.outlineSizeMinimum();
    const auto oldBorder = config.canvasBorderColor();
    const auto restoreConfig = qScopeGuard([&] {
        config.setNewOutlineStyle(oldStyle);
        config.setCursorMainColor(oldColor);
        config.setSeparateEraserCursor(oldSeparate);
        config.setForceAlwaysFullSizedOutline(oldFullSize);
        config.setOutlineSizeMinimum(oldMinimum);
        config.setCanvasBorderColor(oldBorder);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    config.setNewOutlineStyle(OUTLINE_NONE);
    config.setCursorMainColor(Qt::white);
    config.setSeparateEraserCursor(false);
    config.setForceAlwaysFullSizedOutline(true);
    config.setOutlineSizeMinimum(1);
    config.setCanvasBorderColor(Qt::white);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(128, 128));
    QByteArray pixels;
    for (int i = 0; i < 128 * 128; ++i) pixels += QByteArray::fromHex("506478ff");
    QCOMPARE(owner->writeRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels), Gpu::Result::Success);
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU brush outline")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(24);
    resources->setEraserMode(false);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    view->canvasBase()->canvasWidget()->setFocus();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto *converter = view->canvasBase()->coordinatesConverter();
    view->canvasController()->setPreferredCenter(converter->imageRectInWidgetPixels().center());
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
            if (canvas->isVisible()) native = canvas;
    QVERIFY(native);
    native->setCursor(Qt::BlankCursor);
    QSignalSpy frames(native, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
    const auto capture = [&] { return native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888); };
    const auto save = [&](const QImage &image, const QString &stage) {
        const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (directory.isEmpty()) return true;
        return QDir().mkpath(directory) && image.save(QDir(directory).filePath(QStringLiteral("ordinary-brush-outline-") + stage + QStringLiteral(".png")));
    };
    const auto preserveFailureFrame = qScopeGuard([&] {
        if (QTest::currentTestFailed() && native) save(capture(), QStringLiteral("failure"));
    });
    const auto first = converter->imageToWidget(QPointF(40, 64)).toPoint();
    const auto second = converter->imageToWidget(QPointF(88, 64)).toPoint();
    QTest::mouseMove(native.data(), first);
    QEnterEvent entered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &entered);
    native->refresh();
    QTRY_VERIFY(!frames.isEmpty());
    QImage baseline;
    QTRY_VERIFY(!(baseline = capture()).isNull());
    const auto ratio = native->devicePixelRatio();
    const auto physical = [&](QPoint point) { return (QPointF(point) * ratio).toPoint(); };
    const auto currentBackground = [&] {
        const auto screenshot = capture();
        QImage result(screenshot.size(), QImage::Format_RGBA8888);
        result.setDevicePixelRatio(screenshot.devicePixelRatio());
        result.setColorSpace(screenshot.colorSpace());
        result.fill(Qt::white);
        const auto bounds = QTransform::fromScale(ratio, ratio).mapRect(converter->imageRectInWidgetPixels()).toAlignedRect().intersected(result.rect());
        for (int y = bounds.top(); y <= bounds.bottom(); ++y)
            for (int x = bounds.left(); x <= bounds.right(); ++x)
                result.setPixelColor(x, y, QColor(80, 100, 120));
        return result;
    };
    const auto expectedBaseline = currentBackground();
    // Startup notifications can overlap the native child. Accept its clean frame.
    QTRY_VERIFY_WITH_TIMEOUT((baseline = capture()) == expectedBaseline, 10000);
    QCOMPARE(baseline.pixelColor(physical(first)), QColor(80, 100, 120));
    QCOMPARE(baseline.pixelColor(physical(second)), QColor(80, 100, 120));
    QVERIFY(save(baseline, QStringLiteral("baseline")));
    const auto changedPixels = [&](const QImage &image, QPoint center) {
        const auto region = QRect(physical(center) - QPoint(16 * ratio, 16 * ratio), QSize(32 * ratio, 32 * ratio)).intersected(image.rect());
        int count = 0;
        for (int y = region.top(); y <= region.bottom(); ++y)
            for (int x = region.left(); x <= region.right(); ++x)
                if (image.pixel(x, y) != baseline.pixel(x, y)) ++count;
        return count;
    };
    const auto differenceBounds = [&](const QImage &image) {
        QRect bounds;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                if (image.pixel(x, y) != baseline.pixel(x, y)) bounds |= QRect(x, y, 1, 1);
        return bounds;
    };
    config.setNewOutlineStyle(OUTLINE_CIRCLE);
    KisConfigNotifier::instance()->notifyConfigChanged();
    // Configuration can recreate the native canvas. Follow its current window.
    QCoreApplication::processEvents();
    if (!native) {
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
                if (canvas->isVisible()) native = canvas;
    }
    QTRY_VERIFY(native && native->isVisible());
    native->setCursor(Qt::BlankCursor);
    QEnterEvent reentered(first, first, native->mapToGlobal(first));
    QApplication::sendEvent(native.data(), &reentered);
    QTest::mouseMove(native.data(), first + QPoint(1, 0));
    QTest::mouseMove(native.data(), first);
    QImage visible;
    const auto preserveVisible = qScopeGuard([&] { if (!visible.isNull()) save(visible, QStringLiteral("visible")); });
    QTRY_VERIFY_WITH_TIMEOUT((visible = capture()).size() == baseline.size() && changedPixels(visible, first) >= 40 && changedPixels(visible, second) == 0, 5000);
    QVERIFY(save(visible, QStringLiteral("visible")));
    QCOMPARE(changedPixels(visible, second), 0);
    const auto fullBounds = differenceBounds(visible);
    const auto fullSize = converter->imageToWidget(QRectF(0, 0, 24, 24)).size() * ratio;
    QVERIFY(qAbs(fullBounds.width() - fullSize.width()) <= 2 * ratio);
    QVERIFY(qAbs(fullBounds.height() - fullSize.height()) <= 2 * ratio);
    QVERIFY((fullBounds.center() - physical(first)).manhattanLength() <= 2);

    QTest::mouseMove(native.data(), second);
    QImage moved;
    QTRY_VERIFY((moved = capture()).size() == baseline.size() && changedPixels(moved, second) >= 40 && changedPixels(moved, first) == 0);
    QVERIFY(save(moved, QStringLiteral("moved")));
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(native.data(), &leave);
    QImage cleared;
    QTRY_VERIFY((cleared = capture()) == baseline);
    QVERIFY(save(cleared, QStringLiteral("cleared")));
    view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
    const auto halfBackground = currentBackground();
    QTRY_VERIFY(capture() == halfBackground);
    baseline = halfBackground;
    const auto halfPoint = converter->imageToWidget(QPointF(64, 64)).toPoint();
    QEnterEvent halfEntered(halfPoint, halfPoint, native->mapToGlobal(halfPoint));
    QApplication::sendEvent(native.data(), &halfEntered);
    QTest::mouseMove(native.data(), halfPoint);
    QImage halfVisible;
    const auto preserveHalfVisible = qScopeGuard([&] {
        if (QTest::currentTestFailed() && !halfVisible.isNull()) save(halfVisible, QStringLiteral("half-zoom"));
    });
    const auto halfOutlineMatches = [&] {
        halfVisible = capture();
        if (halfVisible.size() != baseline.size() || changedPixels(halfVisible, halfPoint) < 20) return false;
        const auto bounds = differenceBounds(halfVisible);
        const auto size = converter->imageToWidget(QRectF(0, 0, 24, 24)).size() * ratio;
        return qAbs(bounds.width() - size.width()) <= 2 * ratio &&
               qAbs(bounds.height() - size.height()) <= 2 * ratio &&
               (bounds.center() - physical(halfPoint)).manhattanLength() <= 2;
    };
    // A delayed zoom notification can cover an otherwise complete native frame.
    QTRY_VERIFY_WITH_TIMEOUT(halfOutlineMatches(), 10000);
    QVERIFY(save(halfVisible, QStringLiteral("half-zoom")));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("PanTool"));
    // Switching tools can resize the canvas viewport through the tool options bar.
    const auto matchesCurrentBackground = [&] {
        const auto background = currentBackground();
        const auto frame = capture();
        if (frame.size() != background.size()) return false;
        baseline = background;
        return frame == background;
    };
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    const auto currentHalfPoint = converter->imageToWidget(QPointF(64, 64)).toPoint();
    QEnterEvent currentHalfEntered(currentHalfPoint, currentHalfPoint, native->mapToGlobal(currentHalfPoint));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QApplication::sendEvent(native.data(), &currentHalfEntered);
    QTest::mouseMove(native.data(), currentHalfPoint + QPoint(1, 0));
    QTest::mouseMove(native.data(), currentHalfPoint);
    QTRY_VERIFY(changedPixels(capture(), currentHalfPoint) >= 20);
    window->viewManager()->setCurrentView(nullptr);
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    window->viewManager()->setCurrentView(view);
    QApplication::sendEvent(native.data(), &leave);
    QTRY_VERIFY_WITH_TIMEOUT(matchesCurrentBackground(), 10000);
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().undoCount(), 0);
    const auto after = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size());
    QCOMPARE(after.code, Gpu::ExecutionCode::Success);
    QCOMPARE(after.bytes, pixels);
    QVERIFY(!lifetime->isModified());
}

void KisGpuDocumentViewTest::presentsEditsAndReleasesTheClosedTab()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(64, 64));
    const auto handle = owner->handle();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU document view")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    KisView *view = nullptr;
    try {
        view = window->newView(document.get());
    } catch (const std::exception &exception) {
        qWarning("GPU view initialization failed: %s", exception.what());
        throw;
    }
    QVERIFY(view);
    QPointer<KisDocument> documentLifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (documentLifetime) {
            documentLifetime->setModified(false);
            if (window) {
                auto *close = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
                if (close) close->trigger();
            }
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    QVERIFY(view->canvasBase()->canvasWidget());
    QTRY_COMPARE(execution.published(handle), owner->operations().preview());
    auto *overview = window->findChild<QDockWidget*>(QStringLiteral("OverviewDocker"));
    QVERIFY(overview);
    QWidget *preview = nullptr;
    for (auto *child : overview->findChildren<QWidget*>())
        if (QLatin1String(child->metaObject()->className()) == QLatin1String("OverviewWidget")) preview = child;
    QVERIFY(preview);
    QVERIFY(!preview->isEnabled());
    QVERIFY(!preview->toolTip().isEmpty());
    auto *undoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_undo"));
    auto *redoAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("edit_redo"));
    QVERIFY(undoAction);
    QVERIFY(redoAction);
    auto *history = documentLifetime->undoStack();
    auto *historyDock = window->findChild<QDockWidget*>(QStringLiteral("History"));
    QVERIFY(historyDock);
    auto *historyView = qobject_cast<QListView*>(historyDock->widget());
    QVERIFY(historyView);
    QCOMPARE(historyView->model()->rowCount(), 1);
    QSignalSpy historyChanges(history, &KUndo2Stack::indexChanged);
    QCOMPARE(history->count(), 0);
    const auto initial = owner->operations().snapshot();
    const auto firstStroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QCOMPARE(owner->appendSamples(firstStroke, {{{24, 24}, 1}}), Gpu::Result::Success);
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), initial);
    QCOMPARE(history->count(), 0);
    window->resize(1000, 700);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("1181f1ff")), Gpu::Result::Success);
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QCOMPARE(history->count(), 1);
    QCOMPARE(history->index(), 1);
    QCOMPARE(historyView->model()->rowCount(), 2);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 1);
    QVERIFY(history->canUndo());
    QVERIFY(!history->canRedo());
    QVERIFY(!history->isClean());
    QVERIFY(history->command(0));
    QVERIFY(!history->command(0)->text().toString().isEmpty());
    QTRY_VERIFY(undoAction->isEnabled());
    undoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(history->index(), 0);
    QCOMPARE(historyView->selectionModel()->currentIndex().row(), 0);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    QTRY_COMPARE(execution.published(handle), owner->operations().snapshot());
    QTRY_VERIFY(redoAction->isEnabled());
    redoAction->trigger();
    QCOMPARE(owner->operations().undoCount(), 1);
    QVERIFY(!history->isClean());
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1), 4).bytes,
             QByteArray::fromHex("1181f1ff"));
    QCOMPARE(historyChanges.size(), 3);
    historyChanges.clear();
    historyView->selectionModel()->setCurrentIndex(historyView->model()->index(0, 0), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(historyChanges.size(), 1);
    history->setIndex(1);
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    QVERIFY(history->isClean());
    QVERIFY(!documentLifetime->isModified());
    const auto stroke = owner->beginStroke(owner->operations().activeLayer(), {}, 17);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{16, 16}, 1}}), Gpu::Result::Success);
    undoAction->trigger();
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), saved);
    QCOMPARE(history->count(), 1);
    QTRY_COMPARE(execution.published(handle), saved);
    QPointer<KisView> retainedView(view);
    QPointer<QWidget> retainedCanvas(view->canvasBase()->canvasWidget());
    documentLifetime->setModified(false);
    auto *closeAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
    QVERIFY(closeAction);
    closeAction->trigger();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(!retainedView);
    QTRY_VERIFY(!retainedCanvas);
    QTRY_VERIFY(!documentLifetime);
    const auto lastPresented = execution.published(handle);
    QCOMPARE(owner->replaceRgbaPixels(owner->operations().activeLayer(), QRect(8, 12, 1, 1),
                                      QByteArray::fromHex("010203ff")), Gpu::Result::Success);
    QApplication::processEvents();
    QCOMPARE(execution.published(handle), lastPresented);
    window->hide();
    window.reset();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void KisGpuDocumentViewTest::cropPreviewPreservesPixelsHistoryAndCancellation()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    KisConfig config(false);
    const auto oldStyle = config.newOutlineStyle();
    const auto oldColor = config.getCursorMainColor();
    const auto oldSeparate = config.separateEraserCursor();
    const auto oldFullSize = config.forceAlwaysFullSizedOutline();
    const auto oldMinimum = config.outlineSizeMinimum();
    const auto oldBorder = config.canvasBorderColor();
    const auto oldVastScrolling = config.vastScrolling();
    const auto restoreConfig = qScopeGuard([&] {
        config.setNewOutlineStyle(oldStyle);
        config.setCursorMainColor(oldColor);
        config.setSeparateEraserCursor(oldSeparate);
        config.setForceAlwaysFullSizedOutline(oldFullSize);
        config.setOutlineSizeMinimum(oldMinimum);
        config.setCanvasBorderColor(oldBorder);
        config.setVastScrolling(oldVastScrolling);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
    config.setNewOutlineStyle(OUTLINE_NONE);
    config.setCursorMainColor(Qt::white);
    config.setSeparateEraserCursor(false);
    config.setForceAlwaysFullSizedOutline(true);
    config.setOutlineSizeMinimum(1);
    config.setCanvasBorderColor(Qt::white);
    config.setVastScrolling(1);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = Gpu::PaintImageSP::create(execution, QSize(128, 128));
    QByteArray pixels;
    for (int i = 0; i < 128 * 128; ++i) pixels += QByteArray::fromHex("506478ff");
    QCOMPARE(owner->writeRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels), Gpu::Result::Success);
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("GPU crop preview")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto releaseWindow = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
    window->show();
    window->activateWindow();
    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(24);
    resources->setEraserMode(false);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    view->canvasBase()->canvasWidget()->setFocus();
    view->canvasController()->resetCanvasRotation();
    view->canvasController()->mirrorCanvas(false);
    view->zoomManager()->zoomTo100();
    const auto *converter = view->canvasBase()->coordinatesConverter();
    // This case runs as its own CTest entry. Wait for the new view's layout so
    // centering uses the real canvas size and the native window can present.
    QTRY_VERIFY(!converter->getCanvasWidgetSize().isEmpty());
    const auto centerImage = [&] {
        const auto size = converter->imageRectInWidgetPixels().size();
        view->canvasController()->setPreferredCenter(QPointF(size.width() / 2, size.height() / 2));
    };
    centerImage();
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    const auto findNative = [&] {
        native.clear();
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow*>(candidate))
                if (canvas->isVisible()) native = canvas;
        if (native) native->setCursor(Qt::BlankCursor);
        return bool(native);
    };
    QTRY_VERIFY(findNative());
    // The offscreen platform does not deliver an expose event for the new
    // window, so request the frame that the ordinary exposed window receives.
    native->refresh();
    QTRY_COMPARE(execution.published(owner->handle()), snapshot);
    const auto capture = [&] { return native->screen()->grabWindow(native->winId()).toImage().convertToFormat(QImage::Format_RGBA8888); };

    const auto ratio = native->devicePixelRatio();
    const auto expectedSize = capture();
    QImage expected(expectedSize.size(), QImage::Format_RGBA8888);
    expected.setDevicePixelRatio(expectedSize.devicePixelRatio());
    expected.setColorSpace(expectedSize.colorSpace());
    expected.fill(Qt::white);
    const auto rect = QTransform::fromScale(ratio, ratio).mapRect(converter->imageRectInWidgetPixels()).toAlignedRect().intersected(expected.rect());
    for (int y = rect.top(); y <= rect.bottom(); ++y)
        for (int x = rect.left(); x <= rect.right(); ++x) expected.setPixelColor(x, y, QColor(80, 100, 120));
    QImage baseline;
    QTRY_VERIFY_WITH_TIMEOUT((baseline = capture()) == expected, 10000);
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolCrop"));
    auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KisToolCrop"));
    QVERIFY2(tool, "The ordinary crop plugin must be available");
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolCrop"));
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    KConfigGroup cropSettings(KSharedConfig::openConfig(), tool->toolId());
    const auto previousCropSettings = cropSettings.entryMap();
    const auto restoreCropSettings = qScopeGuard([&] {
        for (const auto &key : {QStringLiteral("growCenter"), QStringLiteral("allowGrow"),
                               QStringLiteral("decoration"), QStringLiteral("cropType")}) {
            if (previousCropSettings.contains(key)) cropSettings.writeEntry(key, previousCropSettings[key]);
            else cropSettings.deleteEntry(key);
        }
    });
    QVERIFY(QMetaObject::invokeMethod(tool, "setGrowCenter", Qt::DirectConnection, Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(tool, "setAllowGrow", Qt::DirectConnection, Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(tool, "setCropTypeLegacy", Qt::DirectConnection, Q_ARG(int, 0)));
    auto *snapGuide = view->canvasBase()->snapGuide();
    const bool previousSnapping = snapGuide->isSnapping();
    const auto previousStrategies = snapGuide->enabledSnapStrategies();
    const int previousSnapDistance = snapGuide->snapDistance();
    const auto restoreSnapping = qScopeGuard([&] {
        snapGuide->enableSnapStrategies(previousStrategies);
        snapGuide->setSnapDistance(previousSnapDistance);
        snapGuide->enableSnapping(previousSnapping);
    });
    snapGuide->enableSnapping(false);
    QVERIFY(tool->setProperty("lockWidth", false));
    QVERIFY(tool->setProperty("lockHeight", false));
    QVERIFY(tool->setProperty("lockRatio", false));
    QVERIFY(tool->setProperty("cropX", 32));
    QVERIFY(tool->setProperty("cropY", 32));
    QVERIFY(tool->setProperty("cropWidth", 64));
    QVERIFY(tool->setProperty("cropHeight", 64));
    const auto recovery = owner->operations().recoveryData();
    const QString directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) QVERIFY(QDir().mkpath(directory));
    const auto preserveCropFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed() && !directory.isEmpty())
            capture().save(QDir(directory).filePath(QStringLiteral("crop-failure.png")));
    });
    for (int style = 0; style < 5; ++style) {
        QVERIFY(tool->setProperty("decoration", style));
        auto expectedPreview = baseline;
        expectedPreview.setDevicePixelRatio(ratio);
        QPainter painter(&expectedPreview);
        // The ordinary CPU canvas enables these hints before invoking the tool.
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                               QPainter::SmoothPixmapTransform);
        painter.setTransform(converter->flakeToWidgetTransform());
        tool->paint(painter, *view->canvasBase()->viewConverter());
        painter.end();
        expectedPreview.setDevicePixelRatio(1);
        QImage actual;
        const auto matches = [&] {
            actual = capture();
            if (actual.size() != expectedPreview.size()) return false;
            for (int y = 0; y < actual.height(); ++y) {
                for (int x = 0; x < actual.width(); ++x) {
                    const auto a = actual.pixelColor(x, y);
                    const auto e = expectedPreview.pixelColor(x, y);
                    // Display-only coverage has a two-byte rounding bound.
                    // Document, history and recovery retain exact comparisons.
                    if (qAbs(a.red() - e.red()) > 2 || qAbs(a.green() - e.green()) > 2 ||
                        qAbs(a.blue() - e.blue()) > 2 || qAbs(a.alpha() - e.alpha()) > 2) return false;
                }
            }
            return true;
        };
        const auto preserve = qScopeGuard([&] {
            if (directory.isEmpty()) return;
            actual.save(QDir(directory).filePath(QStringLiteral("crop-%1-actual.png").arg(style)));
            expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-%1-expected.png").arg(style)));
        });
        native->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(matches(), 20000);
        QCOMPARE(owner->operations().snapshot(), snapshot);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(owner->operations().recoveryData() == recovery);
    }
    const auto base = [&] {
        const auto screenshot = capture();
        QImage image(screenshot.size(), QImage::Format_RGBA8888);
        image.setDevicePixelRatio(screenshot.devicePixelRatio());
        image.setColorSpace(screenshot.colorSpace());
        image.fill(Qt::white);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const auto position = converter->widgetToImage(QPointF((x + 0.5) / ratio, (y + 0.5) / ratio));
                if (position.x() >= 0 && position.y() >= 0 && position.x() < 128 && position.y() < 128)
                    image.setPixelColor(x, y, QColor(80, 100, 120));
            }
        }
        return image;
    };
    QImage actualPreview, expectedPreview;
    const auto previewMatches = [&] {
        expectedPreview = base();
        expectedPreview.setDevicePixelRatio(ratio);
        QPainter painter(&expectedPreview);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        painter.setTransform(converter->flakeToWidgetTransform());
        tool->paint(painter, *view->canvasBase()->viewConverter());
        painter.end();
        expectedPreview.setDevicePixelRatio(1);
        actualPreview = capture();
        if (actualPreview.size() != expectedPreview.size()) return false;
        for (int y = 0; y < actualPreview.height(); ++y) {
            for (int x = 0; x < actualPreview.width(); ++x) {
                const auto actual = actualPreview.pixelColor(x, y);
                const auto expected = expectedPreview.pixelColor(x, y);
                if (qAbs(actual.red() - expected.red()) > 2 || qAbs(actual.green() - expected.green()) > 2 ||
                    qAbs(actual.blue() - expected.blue()) > 2 || qAbs(actual.alpha() - expected.alpha()) > 2) return false;
            }
        }
        return true;
    };
    const auto point = [&](QPointF position) { return converter->imageToWidget(position).toPoint(); };
    const auto safeInput = QRectF(QPointF(), native->size()).adjusted(64, 64, -64, -64);
    QVERIFY(safeInput.contains(point({16, 16})));
    QVERIFY(safeInput.contains(point({128, 108})));
    for (int stage = 0; stage < 5; ++stage) {
        if (stage == 0) {
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({64, 64}));
            QTest::mouseMove(native.data(), point({70, 68}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({70, 68}));
            QTest::qWait(50);
            QCOMPARE(tool->property("cropX").toInt(), 38);
            QCOMPARE(tool->property("cropY").toInt(), 36);
            QCOMPARE(tool->property("cropWidth").toInt(), 64);
            QCOMPARE(tool->property("cropHeight").toInt(), 64);
        } else if (stage == 1) {
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({102, 100}));
            QTest::mouseMove(native.data(), point({110, 108}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({110, 108}));
            QTest::qWait(50);
            QCOMPARE(tool->property("cropWidth").toInt(), 72);
            QCOMPARE(tool->property("cropHeight").toInt(), 72);
        } else if (stage == 2) view->canvasController()->pan(QPoint(7, -5));
        else if (stage == 3) view->canvasController()->setZoom(KoZoomMode::ZOOM_CONSTANT, 0.5);
        else view->canvasController()->rotateCanvas(23);
        const auto preserve = qScopeGuard([&] {
            if (directory.isEmpty()) return;
            actualPreview.save(QDir(directory).filePath(QStringLiteral("crop-operation-%1-actual.png").arg(stage)));
            expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-operation-%1-expected.png").arg(stage)));
        });
        native->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(previewMatches(), 20000);
        QCOMPARE(owner->operations().snapshot(), snapshot);
        QCOMPARE(owner->operations().undoCount(), 0);
        QVERIFY(owner->operations().recoveryData() == recovery);
        if (stage == 0) {
            snapGuide->enableSnapStrategies(KoSnapGuide::DocumentBoundsSnapping);
            snapGuide->setSnapDistance(10);
            snapGuide->enableSnapping(true);
            const QPoint nearRight = point({128, 100}) - QPoint(2, 0);
            QVERIFY(safeInput.contains(nearRight));
            const auto image = view->canvasBase()->image();
            const auto cursorImage = converter->widgetToImage(QPointF(nearRight));
            const auto unsnapped = image->documentToPixel(image->pixelToDocument(cursorImage));
            const auto snapped = image->documentToPixel(snapGuide->snap(image->pixelToDocument(cursorImage), Qt::NoModifier));
            QCOMPARE(snapped.x(), 128.0);
            QCOMPARE(snapped.y(), unsnapped.y());
            QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({102, 100}));
            QTest::mouseMove(native.data(), nearRight, 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, nearRight);
            QTest::qWait(50);
            QCOMPARE(tool->property("cropX").toInt(), 38);
            QCOMPARE(tool->property("cropWidth").toInt(), 90);
            QCOMPARE(tool->property("cropHeight").toInt(), 64);
            const auto preserveSnap = qScopeGuard([&] {
                if (directory.isEmpty()) return;
                actualPreview.save(QDir(directory).filePath(QStringLiteral("crop-snapped-actual.png")));
                expectedPreview.save(QDir(directory).filePath(QStringLiteral("crop-snapped-expected.png")));
            });
            native->refresh();
            QTRY_VERIFY_WITH_TIMEOUT(previewMatches(), 20000);
            QCOMPARE(owner->operations().snapshot(), snapshot);
            QCOMPARE(owner->operations().undoCount(), 0);
            QVERIFY(owner->operations().recoveryData() == recovery);
            snapGuide->enableSnapping(false);
            QVERIFY(tool->setProperty("cropWidth", 64));
        }
    }
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QCOMPARE(owner->operations().undoCount(), 0);
    QVERIFY(QMetaObject::invokeMethod(tool, "requestStrokeCancellation", Qt::DirectConnection));
    native->refresh();
    QTRY_VERIFY(capture() == base());
    const auto after = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size());
    QCOMPARE(after.code, Gpu::ExecutionCode::Success);
    QCOMPARE(after.bytes, pixels);
    QVERIFY(!lifetime->isModified());
    view->canvasController()->resetCanvasRotation();
    view->zoomManager()->zoomTo100();
    centerImage();
    QVERIFY(safeInput.contains(point({16, 16})));
    QVERIFY(safeInput.contains(point({112, 96})));
    const auto newCropOrigin = converter->widgetToImage(QPointF(point({16, 16}))).toPoint();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({16, 16}));
    QTest::mouseMove(native.data(), point({112, 96}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({112, 96}));
    QTest::qWait(50);
    QCOMPARE(tool->property("cropX").toInt(), newCropOrigin.x());
    QCOMPARE(tool->property("cropY").toInt(), newCropOrigin.y());
    QCOMPARE(tool->property("cropWidth").toInt(), 96);
    QCOMPARE(tool->property("cropHeight").toInt(), 80);
    QCOMPARE(owner->operations().snapshot(), snapshot);
    QVERIFY(QMetaObject::invokeMethod(tool, "crop", Qt::DirectConnection));
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(96, 80));
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(lifetime->undoStack()->count(), 1);
    const auto croppedPixels = QByteArray::fromHex("506478ff").repeated(96 * 80);
    const auto readCrop = [&] { return owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 96, 80), croppedPixels.size()).bytes; };
    QCOMPARE(readCrop(), croppedPixels);
    lifetime->undoStack()->undo();
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(128, 128));
    QCOMPARE(owner->operations().undoCount(), 0);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), QRect(0, 0, 128, 128), pixels.size()).bytes, pixels);
    lifetime->undoStack()->redo();
    QTRY_COMPARE(owner->operations().snapshot()->size, QSize(96, 80));
    QCOMPARE(readCrop(), croppedPixels);
    QTemporaryDir saved;
    QVERIFY(saved.isValid());
    const auto preserveSavedFailure = qScopeGuard([&] {
        if (QTest::currentTestFailed()) {
            saved.setAutoRemove(false);
            qInfo() << "Crop save failure artifacts at" << saved.path();
        }
    });
    lifetime->setFileBatchMode(true);
    const auto path = saved.filePath(QStringLiteral("crop.kra"));
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QTRY_VERIFY(!lifetime->isModified());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.size, QSize(96, 80));
    Gpu::PaintImage reopened(execution, loaded.document.image);
    QCOMPARE(reopened.readRgbaPixels(reopened.operations().activeLayer(), QRect(0, 0, 96, 80), croppedPixels.size()).bytes, croppedPixels);
    KisPropertiesConfigurationSP configuration(new KisPropertiesConfiguration);
    configuration->setProperty("alpha", true);
    configuration->setProperty("interlaced", false);
    configuration->setProperty("compression", 3);
    configuration->setProperty("saveSRGBProfile", true);
    configuration->setProperty("forceSRGB", false);
    configuration->setProperty("storeAuthor", false);
    configuration->setProperty("storeMetaData", false);
    const auto pngPath = saved.filePath(QStringLiteral("crop.png"));
    QVERIFY2(lifetime->exportDocumentSync(pngPath, QByteArrayLiteral("image/png"), configuration), qPrintable(lifetime->errorMessage()));
    const auto png = QImage(pngPath).convertToFormat(QImage::Format_RGBA8888);
    QCOMPARE(png.size(), QSize(96, 80));
    QCOMPARE(QByteArray::fromRawData(reinterpret_cast<const char*>(png.constBits()), png.sizeInBytes()), croppedPixels);
    if (!directory.isEmpty()) {
        const auto copyArtifact = [&](const QString &source, const QString &name) {
            const QString destination = QDir(directory).filePath(name);
            QFile::remove(destination);
            return QFile::copy(source, destination);
        };
        QVERIFY(copyArtifact(path, QStringLiteral("crop-edited.kra")));
        QVERIFY(copyArtifact(pngPath, QStringLiteral("crop-edited.png")));
    }
}

KISTEST_MAIN(KisGpuDocumentViewTest)
#include "KisGpuDocumentViewTest.moc"
