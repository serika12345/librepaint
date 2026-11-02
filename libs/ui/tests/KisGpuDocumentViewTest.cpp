/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <array>
#include "KisGpuMultibrushEditingTest.h"
#include "KisGpuRegionFillEditingTest.h"
#include <QApplication>
#include <QClipboard>
#include <kis_clipboard.h>
#include <selection/kis_selection_manager.h>
#include <QBitArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDataStream>
#include <QSpinBox>
#include <QComboBox>
#include <KisFigurePaintingOptions.h>
#include <KisAngleSelector.h>
#include <kis_slider_spin_box.h>
#include <QColorSpace>
#include <QMouseEvent>
#include <QMenuBar>
#include <QRadioButton>
#include <QDockWidget>
#include <QEnterEvent>
#include <QListView>
#include <QItemSelectionModel>
#include <QIcon>
#include <QPointer>
#include <QPushButton>
#include <QPainter>
#include <QPainterPathStroker>
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
#include <QFileInfo>
#include <QBuffer>
#include <kis_mimedata.h>
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
#include <input/ui/kis_tool_proxy.h>
#include <gpu/KisGpuCanvasWindow.h>
#include <KoToolManager.h>
#include <KoSnapGuide.h>
#include <KoCanvasResourcesIds.h>
#include <KoPattern.h>
#include <resources/KoStopGradient.h>
#include <KoToolBase.h>
#include <KoPointerEvent.h>
#include <kis_tool.h>
#include <kis_fill_painter.h>
#include <kis_selection_filters.h>
#include <commands_new/KisMergeLabeledLayersCommand.h>
#include <kis_figure_painting_stroke.h>
#include <KoResourcePaths.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOpRegistry.h>
#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <canvas/kis_display_color_converter.h>
#include <canvas/KisDisplayConfig.h>
#include <utility>
#include <document/KisDocument.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_default_bounds.h>
#include <kis_image_config.h>
#include <kis_node.h>
#include <kis_layer_utils.h>
#include <kis_meta_data_merge_strategy_registry.h>
#include <kis_paint_device.h>
#include <KisRegion.h>
#include <kis_painter.h>
#include <brushengine/kis_paintop.h>
#include <brushengine/kis_paintop_settings.h>
#include <KisRunnableStrokeJobData.h>
#include <KisRunnableStrokeJobsInterface.h>
#include <kis_paint_information.h>
#include <kis_random_source.h>
#include <kis_distance_information.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_undo_stores.h>
#include <KisGpuBlendMode.h>
#include <kis_indirect_painting_support.h>
#include <kis_properties_configuration.h>
#include <kis_selection.h>
#include <kis_selection_mask.h>
#include <kis_pixel_selection.h>
#include <kis_selection_tool_helper.h>
#include <kis_selection_options.h>
#include <kis_layer_properties_icons.h>
#include <kis_transparency_mask.h>
#include <kis_resources_snapshot.h>
#include <kis_tool_utils.h>
#include <tool/kis_tool_freehand_helper.h>
#include <kis_painting_information_builder.h>
#include <kis_smoothing_options.h>
#include <strokes/freehand_stroke.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <KisAsynchronousStrokeUpdateHelper.h>
#include <filter/kis_color_transformation_filter.h>
#include <filter/kis_filter_category_ids.h>
#include <filter/kis_filter_configuration.h>
#include <filter/kis_filter_registry.h>
#include <document/kis_filter_manager.h>
#include <KoColorTransformation.h>
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
#include <cstring>
#include <exception>
#include <memory>
#include <thread>

namespace {
// Speed-sensitive fixtures keep the delivered event clock and the existing
// input-builder settings identical on the native and GPU window paths.
class BrushPointerClock : public QObject
{
public:
    void advance() { m_time += 20; }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick)
            static_cast<QMouseEvent *>(event)->setTimestamp(m_time);
        return false;
    }
private:
    ulong m_time = 1000;
};

auto fixedBrushPointerTiming(bool enabled)
{
    const bool timestamps = KisConfig(true).readEntry("useTimestampsForBrushSpeed", false);
    const int smoothing = KisConfig(true).readEntry("speedValueSmoothing", 3);
    const int maximum = KisConfig(true).readEntry("maxAllowedSpeedValue", 30);
    if (enabled) {
        KisConfig config(false);
        config.writeEntry("useTimestampsForBrushSpeed", true);
        config.writeEntry("speedValueSmoothing", 3);
        config.writeEntry("maxAllowedSpeedValue", 30);
        KisConfigNotifier::instance()->notifyConfigChanged();
    }
    return qScopeGuard([=] {
        if (!enabled) return;
        KisConfig config(false);
        config.writeEntry("useTimestampsForBrushSpeed", timestamps);
        config.writeEntry("speedValueSmoothing", smoothing);
        config.writeEntry("maxAllowedSpeedValue", maximum);
        KisConfigNotifier::instance()->notifyConfigChanged();
    });
}

void exchangeRgbaBgra(QByteArray &bytes, int pixelBytes)
{
    const int channelBytes = pixelBytes / 4;
    for (qsizetype pixel = 0; pixel < bytes.size(); pixel += pixelBytes)
        for (int byte = 0; byte < channelBytes; ++byte)
            std::swap(bytes[pixel + byte], bytes[pixel + 2 * channelBytes + byte]);
}

QByteArray nativeClipboardMask(const KoColorSpace *cs, QByteArray source, const QByteArray &coverage, QSize size)
{
    exchangeRgbaBgra(source, cs->pixelSize());
    cs->applyAlphaU8Mask(reinterpret_cast<quint8 *>(source.data()),
                         reinterpret_cast<const quint8 *>(coverage.constData()), size.width() * size.height());
    exchangeRgbaBgra(source, cs->pixelSize());
    return source;
}

QByteArray nativeClearSelection(const KoColorSpace *cs, QByteArray source, const QByteArray &coverage, QSize size)
{
    exchangeRgbaBgra(source, cs->pixelSize());
    KisPaintDeviceSP device = new KisPaintDevice(cs);
    const QRect region(QPoint(), size);
    device->writeBytes(reinterpret_cast<const quint8 *>(source.constData()), region);
    KisSelectionSP selection = new KisSelection();
    selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(coverage.constData()), region);
    device->clearSelection(selection);
    device->readBytes(reinterpret_cast<quint8 *>(source.data()), region);
    exchangeRgbaBgra(source, cs->pixelSize());
    return source;
}

template<class ExecutionOwner, class... Arguments>
LibrePaint::Image::Gpu::PaintImageSP paintImageWithEncoding(ExecutionOwner &&execution,
    LibrePaint::Image::Gpu::RasterImage raster, Arguments &&...arguments)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (raster.colorProfile.isEmpty()) {
        const auto *cs = raster.format == Gpu::PixelFormat::Rgba16
            ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
        raster.colorProfile = cs->profile()->rawData();
    }
    return Gpu::PaintImageSP::create(std::forward<ExecutionOwner>(execution), raster,
                                    std::forward<Arguments>(arguments)...);
}
template<class ExecutionOwner, class... Arguments>
LibrePaint::Image::Gpu::PaintImageSP paintImageWithEncoding(ExecutionOwner &&execution,
    QSize size, Arguments &&...arguments)
{
    LibrePaint::Image::Gpu::RasterImage raster;
    raster.size = size;
    raster.layers = {{QStringLiteral("Paint Layer 1"), true, {}, {}}};
    return paintImageWithEncoding(std::forward<ExecutionOwner>(execution), std::move(raster),
                                   std::forward<Arguments>(arguments)...);
}

/** The example filter plugin owns this identity. The fixture supplies an
 * Invert transformation for its GPU routing contract.
 */
class GpuDocumentInvertFilter : public KisColorTransformationFilter
{
public:
    GpuDocumentInvertFilter()
        : KisColorTransformationFilter(KoID(QStringLiteral("invert"), i18n("Invert")),
                                       FiltersCategoryAdjustId, i18n("&Invert"))
    {
        setColorSpaceIndependence(FULLY_INDEPENDENT);
        setSupportsPainting(true);
        setShowConfigurationWidget(false);
        setSupportsLevelOfDetail(true);
    }
    KoColorTransformation *createTransformation(const KoColorSpace *cs, const KisFilterConfigurationSP) const override
    { return cs->createInvertTransformation(); }
};
}

class KisGpuDocumentViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        Q_INIT_RESOURCE(krita);
        const auto prefix = qEnvironmentVariable("KIS_TEST_PREFIX_PATH");
        const auto stagedActions = QDir(prefix).filePath(QStringLiteral("share/krita/actions"));
        const auto actions = !prefix.isEmpty() &&
            QFile::exists(QDir(stagedActions).filePath(QStringLiteral("krita.action"))) ? stagedActions :
            QStringLiteral(KRITA_SOURCE_DIR "/krita");
        KoResourcePaths::addAssetDir(QStringLiteral("kis_actions"), actions);
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
    void nonlinearU16CanvasTracksTheActualDisplayConfiguration();
    void brushOutlineFollowsTheOrdinaryPointerWithoutEditing();
    void cropPreviewPreservesPixelsHistoryAndCancellation();
    void canvasBackgroundFollowsPreferencesWithoutEditing_data();
    void canvasBackgroundFollowsPreferencesWithoutEditing();
    void standardAutoBrushPresetsKeepOrdinaryEditing_data();
    void standardAutoBrushPresetsKeepOrdinaryEditing();
    void quickGroupsKeepPixelsHistoryAndSaving_data();
    void quickGroupsKeepPixelsHistoryAndSaving();
    void layerMergesKeepNativeEditing_data();
    void layerMergesKeepNativeEditing();
    void stabilizedPresetsKeepOrdinaryEditing_data();
    void stabilizedPresetsKeepOrdinaryEditing();
    void stabilizerPrimitivesRetainIndependentLineStarts();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled_data();
    void freehandInputCommitsOneHistoryEntryAndCanBeCancelled();
    void brushBlendModeUsesEffectiveCompositeOpAndRejectsUnsupported();
    void paintingBackendChangesRebindTheCanvas_data();
    void paintingBackendChangesRebindTheCanvas();
    void replacementUpdatesPresentationInputHistoryAndSaving_data();
    void replacementUpdatesPresentationInputHistoryAndSaving();
    void layerPanelEditsUseOneDocumentHistory_data();
    void layerPanelEditsUseOneDocumentHistory();
    void layerPanelThumbnailsReadOnlyReducedGpuPixels();
    void rectangularSelectionLimitsPainting();
    void transparencyMaskActionsUseTheActiveGpuPaintLayer();
    void invertColorsActionUsesGpuDocumentAndDiagnosesUnsupportedFilters();
    void desaturateActionAppliesGrayscaleOnGpuDocument();
    void hsvAdjustmentActionUsesProductSettingsOnGpuDocument();
    void layerPanelShowsAndSelectsGpuTransparencyMaskChild();
    void layerPanelTogglesAndRemovesGpuTransparencyMaskChild();
    void layerPanelGpuTransparencyMaskChildSurvivesSaving();
    void maskChildFreehandStrokeEditsOnlyCoverage();
    void maskChildFreehandStrokeEditsOnlyCoverage_data();
    void maskPresetsKeepOrdinaryEditing_data();
    void maskPresetsKeepOrdinaryEditing();
    void groupMaskPresetsKeepOrdinaryEditing_data();
    void groupMaskPresetsKeepOrdinaryEditing();
    void selectionFillUsesGpuSelection();
    void nativeClipboardCoverageKeepsPrecisionAndSavedPixels_data();
    void nativeClipboardCoverageKeepsPrecisionAndSavedPixels();
    void selectionMergedCopyUsesGpuSelection();
    void selectionCopyAndCutUseGpuSelection();
    void selectionGrowAndShrinkUseGpuSelection();
    void invertedSelectionLimitsPaintingAndKra();
    void nativeInvertedSelectionsKeepEditingAndSaving_data();
    void nativeInvertedSelectionsKeepEditingAndSaving();
    void gradientsKeepNativeEditingHistoryAndKra_data();
    void gradientsKeepNativeEditingHistoryAndKra();
    void multibrushPresetsKeepNativeEditingHistoryAndKra_data();
    void multibrushPresetsKeepNativeEditingHistoryAndKra();
    void dynamicBrushPresetsKeepNativeEditingHistoryAndKra_data();
    void dynamicBrushPresetsKeepNativeEditingHistoryAndKra();
    void ellipticalSelectionLimitsPaintingAndKra();
    void ellipticalToolCreatesDocumentSelection();
    void freehandToolCreatesArbitraryMaskSelection();
    void polygonToolVerticesCreateFrozenCoverage();
    void pathToolClosedPathCreatesPolygonCoverage();
    void polygonalToolCreatesArbitraryMaskSelection();
    void generatedSelectionViewRetainsAlphaAfterClose();
    void colourSelectionToolsKeepPixelsHistoryAndKra_data();
    void colourSelectionToolsKeepPixelsHistoryAndKra();
    void shapeSelectionCombinationsKeepEditingHistoryAndKra_data();
    void shapeSelectionCombinationsKeepEditingHistoryAndKra();
    void selectionOutlineKeepsOnlyEffectiveCoverageBoundaries();
    void pathToolCreatesArbitraryMaskSelection();
    void featherActionSoftensFreehandSelection();
    void pasteClipboardImageCreatesGpuLayer_data();
    void pasteClipboardImageCreatesGpuLayer();
    void reselectRestoresGpuSelection();
    void arrowKeysNudgeGpuSelection();
    void copySelectionToNewLayerUsesGpuPixels_data();
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
    void selectionFillToolKeepsNativeEditing_data();
    void selectionFillToolKeepsNativeEditing();
    void colorFillActionsPreservePixelsHistoryAndSaving_data();
    void colorFillActionsPreservePixelsHistoryAndSaving();
private:
    KisPaintOpPresetSP m_preset;
    std::shared_ptr<LibrePaint::Image::Gpu::Execution> m_gradientExecution;
    std::shared_ptr<LibrePaint::Image::Gpu::Execution> m_freehandExecution;
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
    const auto owner = paintImageWithEncoding(execution, raster);
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
    QTest::newRow("gpu-pattern") << true << QStringLiteral("pattern")
        << QByteArray::fromHex("664422c8") << QByteArray::fromHex("543d26ff");
    QTest::newRow("gpu-pattern-opacity") << true << QStringLiteral("pattern-opacity")
        << QByteArray::fromHex("66442264") << QByteArray::fromHex("322f2cff");
    QTest::newRow("gpu-pattern-erase") << true << QStringLiteral("pattern-erase")
        << QByteArray::fromHex("00000000") << QByteArray::fromHex("11223337");
    QTest::newRow("gpu-multiply") << true << QStringLiteral("multiply")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("101103ff");
    QTest::newRow("gpu-screen") << true << QStringLiteral("screen")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f29241ff");
    QTest::newRow("gpu-darken") << true << QStringLiteral("darken")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("112211ff");
    QTest::newRow("gpu-lighten") << true << QStringLiteral("lighten")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("f18133ff");
    QTest::newRow("gpu-add") << true << QStringLiteral("add")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("ffa344ff");
    QTest::newRow("gpu-linear-dodge") << true << QStringLiteral("linear_dodge")
        << QByteArray::fromHex("f18111ff") << QByteArray::fromHex("ffa344ff");
    QTest::newRow("gpu-pattern-multiply") << true << QStringLiteral("pattern-multiply")
        << QByteArray::fromHex("664422c8") << QByteArray::fromHex("090e10ff");
    for (const auto &scenario : {"reject-pattern-blend", "reject-blend", "close-during-fill"}) {
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
        owner = paintImageWithEncoding(execution, QSize(32, 16));
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
    const bool patternScenario = scenario.startsWith(QLatin1String("pattern")) ||
        scenario.startsWith(QLatin1String("reject-pattern"));
    if (patternScenario) {
        QImage patternImage(2, 2, QImage::Format_ARGB32);
        patternImage.fill(QColor(34, 68, 102, 200));
        resources->slotPatternActivated(KoPatternSP(new KoPattern(patternImage,
            QStringLiteral("GPU test pattern"), QString())));
    }
    const bool eraseScenario = scenario == QLatin1String("erase") || scenario == QLatin1String("pattern-erase");
    resources->setEraserMode(eraseScenario);
    QString compositeOp = QStringLiteral("normal");
    if (scenario == QLatin1String("pattern-multiply"))
        compositeOp = QStringLiteral("multiply");
    else if (scenario == QLatin1String("multiply") || scenario == QLatin1String("screen") ||
             scenario == QLatin1String("darken") || scenario == QLatin1String("lighten") ||
             scenario == QLatin1String("add") || scenario == QLatin1String("linear_dodge"))
        compositeOp = scenario;
    else if (scenario.startsWith(QLatin1String("reject-")))
        compositeOp = QStringLiteral("librepaint-test-invalid-composite");
    resources->setCurrentCompositeOp(compositeOp);
    resources->resourceManager()->setResource(KoCanvasResource::GlobalAlphaLock,
        scenario == QLatin1String("alpha-lock") || eraseScenario);
    const auto count = lifetime->undoStack()->count();
    const auto index = lifetime->undoStack()->index();
    if (owner) QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    else lifetime->undoStack()->setClean();
    const auto actionName = (patternScenario ? QStringLiteral("fill_selection_pattern") :
        QStringLiteral("fill_selection_") +
            (scenario.startsWith(QLatin1String("background")) ? QStringLiteral("background_color") : QStringLiteral("foreground_color"))) +
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
        owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        owner = paintImageWithEncoding(execution, QSize(64, 64));
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
            resources->setCurrentCompositeOp(QStringLiteral("librepaint-test-invalid-composite"));
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
        if (native) QTest::keyClick(native.data(), Qt::Key_Escape);
        else QTest::keyClick(input.data(), Qt::Key_Escape);
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
        auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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

void KisGpuDocumentViewTest::quickGroupsKeepPixelsHistoryAndSaving_data()
{
    QTest::addColumn<bool>("gpu");
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("clipping");
    QTest::addColumn<bool>("outsideActive");
    for (const bool gpu : {false, true}) for (const bool words : {false, true}) for (const bool clipping : {false, true}) {
        const QByteArray row = QByteArray(gpu ? "gpu-" : "native-") + (words ? "u16-" : "u8-") + (clipping ? "clipping" : "group");
        QTest::newRow(row.constData()) << gpu << words << clipping << false;
    }
    QTest::newRow("native-u8-outside-active") << false << false << false << true;
    QTest::newRow("gpu-u8-outside-active") << true << false << false << true;
}

void KisGpuDocumentViewTest::quickGroupsKeepPixelsHistoryAndSaving()
{
    QFETCH(bool, gpu);
    QFETCH(bool, words);
    QFETCH(bool, clipping);
    QFETCH(bool, outsideActive);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 32, 16);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setAutoSaveActive(false);
    document->setFileBatchMode(true);
    QVERIFY(document->newImage(QStringLiteral("Layer groups"), area.width(), area.height(), cs,
                               KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, outsideActive ? 3 : 2, {}, 1));
    auto image = KisImageSP(document->image());
    KisNodeList originals{image->root()->at(0), image->root()->at(1)};
    originals[0]->setName(QStringLiteral("One")); originals[1]->setName(QStringLiteral("Two"));
    for (auto node : originals) {
        node->setVisible(true); node->setUserLocked(false); node->setOpacity(255);
        node->setCompositeOpId(QStringLiteral("normal"));
    }
    const QList<KoColor> colors{KoColor(Qt::red, cs), KoColor(QColor(0, 0, 255, 128), cs)};
    for (int i = 0; i < originals.size(); ++i) {
        const auto pixels = QByteArray(reinterpret_cast<const char *>(colors[i].data()), cs->pixelSize()).repeated(area.width() * area.height());
        originals[i]->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), area);
    }
    if (!gpu) image->refreshGraphAsync();
    image->waitForDone();
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get()); QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show(); window->activateWindow();
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= area.width());
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->height() >= area.height());
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(outsideActive ? image->root()->at(2) : originals.last());
    manager->slotSetSelectedNodes(originals);
    const auto allNodes = [&] {
        KisNodeList result;
        for (auto child = image->root()->firstChild(); child; child = child->nextSibling()) result.append(child);
        for (qsizetype i = 0; i < result.size(); ++i)
            for (auto child = result[i]->firstChild(); child; child = child->nextSibling()) result.append(child);
        return result;
    };
    const auto find = [&](const QUuid &uuid) {
        for (const auto &node : allNodes()) if (node->uuid() == uuid) return node;
        return KisNodeSP();
    };
    const auto selected = [&] {
        QSet<QUuid> result;
        for (const auto &node : manager->selectedNodes()) result.insert(node->uuid());
        return result;
    };
    const auto pixels = [&](const KisNodeSP &node) {
        QByteArray bytes(area.width() * area.height() * cs->pixelSize(), '\0');
        const auto device = node == image->root() ? image->projection() : node->paintDevice();
        device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        return bytes;
    };
    const auto state = [&] {
        QByteArray bytes;
        QDataStream output(&bytes, QIODevice::WriteOnly);
        for (const auto &node : allNodes()) {
            output << node->uuid() << (node->parent() == image->root() ? QUuid() : node->parent()->uuid())
                   << node->name() << QByteArray(node->metaObject()->className()) << node->x() << node->y()
                   << node->opacity() << node->visible() << node->userLocked() << node->colorLabelIndex() << node->isPinnedToTimeline();
            if (const auto layer = qobject_cast<const KisLayer *>(node.data())) {
                auto flags = layer->channelFlags();
                if (flags.isEmpty()) flags.fill(true, layer->colorSpace()->channelCount());
                output << flags;
            }
            if (const auto group = qobject_cast<const KisGroupLayer *>(node.data())) output << group->passThroughMode();
            if (qobject_cast<const KisPaintLayer *>(node.data())) {
                const auto value = node->paintDevice()->defaultPixel();
                output << QByteArray(reinterpret_cast<const char *>(value.data()), value.colorSpace()->pixelSize()) << pixels(node);
            }
        }
        return bytes;
    };
    const auto before = state();
    QTRY_COMPARE(selected(), (QSet<QUuid>{originals[0]->uuid(), originals[1]->uuid()}));
    const auto beforeProjection = pixels(image->root());
    const auto history = lifetime->undoStack()->count();
    auto *action = window->viewManager()->actionManager()->actionByName(clipping ?
        QStringLiteral("create_quick_clipping_group") : QStringLiteral("create_quick_group"));
    QVERIFY(action && action->isEnabled()); action->trigger(); image->waitForDone();
    QTRY_COMPARE(image->root()->childCount(), quint32(outsideActive ? 2 : 1));
    auto group = image->root()->lastChild();
    QVERIFY(qobject_cast<KisGroupLayer *>(group.data()));
    QTRY_COMPARE(group->childCount(), quint32(clipping ? 3 : 2));
    QCOMPARE(lifetime->undoStack()->count(), history + 1);
    QCOMPARE(group->at(0)->uuid(), originals[0]->uuid());
    QCOMPARE(group->at(1)->uuid(), originals[1]->uuid());
    QCOMPARE(pixels(image->root()), beforeProjection);
    const auto groupUuid = group->uuid();
    const auto targetUuid = clipping ? group->lastChild()->uuid() : originals[outsideActive ? 0 : 1]->uuid();
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == targetUuid);
    const QSet<QUuid> selectedAfter = clipping ? QSet<QUuid>{targetUuid} : QSet<QUuid>{originals[0]->uuid(), originals[1]->uuid()};
    QTRY_COMPARE(selected(), selectedAfter);
    if (clipping) QVERIFY(qobject_cast<KisLayer *>(group->lastChild().data())->alphaChannelDisabled());
    const auto grouped = state();
    lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), before);
    QTRY_COMPARE(selected(), (QSet<QUuid>{outsideActive ? image->root()->at(2)->uuid() : originals.last()->uuid()}));
    lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), grouped);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    int saves = 0;
    const auto saveAndOpen = [&] {
        const auto path = directory.filePath(QStringLiteral("groups-%1.kra").arg(++saves));
        if (!lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false)) return false;
        lifetime->waitForSavingToComplete();
        if (!lifetime->openPath(path, KisDocument::DontAddToRecent)) return false;
        image = lifetime->image(); image->waitForDone(); QApplication::processEvents();
        return true;
    };
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage()));
    QTRY_COMPARE(state(), grouped);
    auto target = find(targetUuid); QVERIFY(target);
    manager->slotUiActivatedNode(target); manager->slotSetSelectedNodes({target});
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == targetUuid);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    const KoColor foreground(QColor(241, 17, 129), cs);
    resources->setFGColor(foreground); resources->setOpacity(1); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    const auto beforeFill = pixels(target);
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger(); image->waitForDone(); QApplication::processEvents();
    const auto expected = QByteArray(reinterpret_cast<const char *>(foreground.data()), cs->pixelSize()).repeated(area.width() * area.height());
    QTRY_COMPARE(pixels(target), expected);
    lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(target), beforeFill);
    lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(target), expected);
    const auto filledProjection = pixels(image->root());
    if (!outsideActive) QCOMPARE(filledProjection, expected);
    const auto filled = state();
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), filled);
    group = find(groupUuid); QVERIFY(group);
    manager->slotUiActivatedNode(group); manager->slotSetSelectedNodes({group});
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == groupUuid);
    const auto ungroupHistory = lifetime->undoStack()->count();
    auto *ungroup = window->viewManager()->actionManager()->actionByName(QStringLiteral("quick_ungroup"));
    QVERIFY(ungroup); QTRY_VERIFY(ungroup->isEnabled()); ungroup->trigger(); image->waitForDone();
    QTRY_VERIFY(!find(groupUuid));
    QCOMPARE(image->root()->childCount(), quint32((clipping ? 3 : 2) + (outsideActive ? 1 : 0)));
    QCOMPARE(lifetime->undoStack()->count(), ungroupHistory + 1);
    QCOMPARE(pixels(find(targetUuid)), expected);
    const auto ungrouped = state();
    lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), filled);
    lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), ungrouped);
    QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), ungrouped);
    QCOMPARE(pixels(image->root()), filledProjection);
}

void KisGpuDocumentViewTest::layerMergesKeepNativeEditing_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("varied");
    QTest::addColumn<bool>("sparse");
    QTest::addColumn<bool>("groups");
    QTest::addColumn<int>("protection");
    const QList<QByteArray> names{"down", "selected", "layer", "image", "visible"};
    for (int mode = 0; mode < names.size(); ++mode)
        for (const bool words : {false, true}) for (const bool varied : {false, true}) {
            const QByteArray name = names[mode] + (words ? "-u16" : "-u8") + (varied ? "-varied" : "-normal");
            QTest::newRow(name.constData()) << mode << words << varied << false << false << 0;
        }
    QTest::newRow("down-u8-sparse-default") << 0 << false << false << true << false << 0;
    QTest::newRow("down-u16-sparse-default") << 0 << true << false << true << false << 0;
    for (const int mode : {1, 2, 3, 4}) for (const bool words : {false, true}) {
        const QByteArray name = names[mode] + (words ? "-u16-groups" : "-u8-groups");
        QTest::newRow(name.constData()) << mode << words << true << false << true << 0;
        const QByteArray normalName = name + "-normal";
        QTest::newRow(normalName.constData()) << mode << words << false << false << true << 0;
    }
    QTest::newRow("down-u8-public-groups") << 0 << false << false << false << true << 0;
    QTest::newRow("down-u16-public-groups") << 0 << true << false << false << true << 0;
    QTest::newRow("down-u8-public-groups-varied") << 0 << false << true << false << true << 0;
    QTest::newRow("down-u16-public-groups-varied") << 0 << true << true << false << true << 0;
    for (const int mode : {0, 1, 2}) for (const bool words : {false, true}) {
        const QByteArray name = names[mode] + (words ? "-u16-mask-default" : "-u8-mask-default");
        QTest::newRow(name.constData()) << mode << words << false << false << false << 4;
    }
    for (const int mode : {0, 1, 2}) for (const bool words : {false, true}) for (const int protection : {1, 2, 3}) {
        if (mode == 2 && protection == 1) continue;
        const QByteArray name = names[mode] + (words ? "-u16-" : "-u8-") +
            (protection == 1 ? "lower-alpha" : protection == 2 ? "upper-alpha" : "blue-protected");
        QTest::newRow(name.constData()) << mode << words << (protection != 3) << false << false << protection;
    }
}

void KisGpuDocumentViewTest::layerMergesKeepNativeEditing()
{
    QFETCH(int, mode);
    QFETCH(bool, words);
    QFETCH(bool, varied);
    QFETCH(bool, sparse);
    QFETCH(bool, groups);
    QFETCH(int, protection);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect canvas(0, 0, 32, 16), inspection(-16, -16, sparse ? 288 : 64, 48);
    struct Run {
        bool ok = false;
        QList<QByteArray> states, projections, mergedPixels;
        QList<bool> rootReplaced;
    } native, gpu;
    const auto run = [&](bool useGpu, Run &result) {
        qInfo() << "Merge editing backend" << (useGpu ? "gpu" : "native") << "mode" << mode << "groups" << groups;
        qputenv("LIBREPAINT_GPU_ENGINE", useGpu ? "1" : "0");
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false); document->setFileBatchMode(true);
        QVERIFY(document->newImage(QStringLiteral("Layer merging"), canvas.width(), canvas.height(), cs,
            KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 2, {}, 1));
        auto image = KisImageSP(document->image());
        image->setDefaultProjectionColor(KoColor(QColor(12, 34, 56, 64), cs));
        KisNodeList originals{image->root()->at(0), image->root()->at(1)};
        originals[0]->setName(QStringLiteral("Lower")); originals[1]->setName(QStringLiteral("Upper"));
        const QList<KoColor> colors{KoColor(QColor(17, 91, 203, 127), cs), KoColor(QColor(231, 19, 73, 159), cs)};
        for (int i = 0; i < originals.size(); ++i) {
            auto node = originals[i];
            node->setVisible(true); node->setUserLocked(false);
            node->setOpacity(varied ? (i ? 205 : 137) : 179);
            node->setCompositeOpId(varied ? (i ? QStringLiteral("screen") : QStringLiteral("multiply")) : QStringLiteral("normal"));
            node->setColorLabelIndex(i ? 3 : 2); node->setPinnedToTimeline(i == 0);
            if (varied) { node->setX(i ? 3 : -7); node->setY(i ? -5 : 4); }
            const QRect input = sparse ? QRect(5, 3, 1, 1) : varied ? (i ? QRect(3, -3, 24, 18) : QRect(-5, 2, 24, 18)) : canvas;
            const auto pixels = QByteArray(reinterpret_cast<const char *>(colors[i].data()), cs->pixelSize()).repeated(input.width() * input.height());
            node->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixels.constData()), input);
            if (protection == 4 && i == 1)
                node->paintDevice()->setDefaultPixel(KoColor(QColor(11, 63, 129, 127), cs));
            node->setDirty(input);
            if (sparse && i == 0) {
                node->paintDevice()->writeBytes(colors[i].data(), QRect(197, 3, 1, 1));
                node->setDirty(QRect(197, 3, 1, 1));
            }
        }
        if (protection == 1 || protection == 2)
            qobject_cast<KisLayer *>(originals[protection - 1].data())->disableAlphaChannel(true);
        else if (protection == 3) for (auto node : originals) {
            auto flags = cs->channelFlags(true, true); flags.clearBit(0);
            qobject_cast<KisLayer *>(node.data())->setChannelFlags(flags);
        }
        if (!useGpu) image->refreshGraphAsync();
        image->waitForDone();
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        window->resize(800, 600);
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        const auto close = qScopeGuard([&] {
            if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        });
        window->viewManager()->setCurrentView(view);
        window->viewManager()->canvasResourceProvider()->setPaintOpPreset(m_preset);
        window->show(); window->activateWindow();
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= canvas.width());
        auto *manager = window->viewManager()->nodeManager();
        auto *panel = window->findChild<QDockWidget *>(QStringLiteral("KisLayerBox")); QVERIFY(panel);
        auto *tree = panel->findChild<QTreeView *>(QStringLiteral("listLayers")); QVERIFY(tree);
        auto *proxy = qobject_cast<KisNodeFilterProxyModel *>(tree->model()); QVERIFY(proxy);
        QTRY_VERIFY(proxy->indexFromNode(originals[1]).isValid());
        manager->slotUiActivatedNode(originals[1]); manager->slotSetSelectedNodes({originals[1]});
        auto *addMask = window->viewManager()->actionManager()->actionByName(QStringLiteral("add_new_transparency_mask"));
        QVERIFY(addMask); QTRY_VERIFY(addMask->isEnabled()); addMask->trigger(); image->waitForDone();
        QTRY_COMPARE(originals[1]->childCount(), 1U);
        auto *mask = qobject_cast<KisTransparencyMask *>(originals[1]->firstChild().data()); QVERIFY(mask);
        const quint8 coverage = 211;
        KisSelectionSP selection = new KisSelection();
        selection->setDefaultBounds(KisDefaultBoundsBaseSP(new KisDefaultBounds(image)));
        selection->pixelSelection()->setDefaultPixel(KoColor(&coverage, KoColorSpaceRegistry::instance()->alpha8()));
        const QByteArray hole(16, '\0');
        selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(hole.constData()), QRect(8, 4, 4, 4));
        selection->updateProjection();
        if (useGpu) QVERIFY(image->writeGpuTransparencyMaskSelection(mask, selection));
        else { mask->setSelection(selection); mask->setDirty(); image->refreshGraphAsync(); }
        image->waitForDone();
        KisNodeList heads = originals;
        if (groups) {
            auto *group = window->viewManager()->actionManager()->actionByName(QStringLiteral("create_quick_group"));
            QVERIFY(group);
            if (mode == 2) {
                manager->slotUiActivatedNode(originals[1]); manager->slotSetSelectedNodes(originals);
                QTRY_VERIFY(group->isEnabled()); group->trigger(); image->waitForDone();
                QTRY_VERIFY(originals[1]->parent() && originals[1]->parent() != image->root());
                heads = {originals[1]->parent()};
                QTRY_VERIFY(proxy->indexFromNode(heads.first()).isValid());
            } else for (int i = 0; i < originals.size(); ++i) {
                manager->slotUiActivatedNode(originals[i]); manager->slotSetSelectedNodes({originals[i]});
                QTRY_VERIFY(group->isEnabled()); group->trigger(); image->waitForDone();
                QTRY_VERIFY(originals[i]->parent() && originals[i]->parent() != image->root());
                heads[i] = originals[i]->parent();
                QTRY_VERIFY(proxy->indexFromNode(heads[i]).isValid());
                heads[i]->setName(i ? QStringLiteral("Upper group") : QStringLiteral("Lower group"));
            }
        }
        manager->slotUiActivatedNode(heads.last());
        manager->slotSetSelectedNodes(mode == 1 ? heads : KisNodeList{heads.last()});
        QTRY_COMPARE(view->currentNode(), heads.last());
        const auto allNodes = [&] {
            KisNodeList nodes;
            for (auto child = image->root()->firstChild(); child; child = child->nextSibling()) nodes.append(child);
            for (qsizetype i = 0; i < nodes.size(); ++i)
                for (auto child = nodes[i]->firstChild(); child; child = child->nextSibling()) nodes.append(child);
            return nodes;
        };
        const auto pixels = [&](KisPaintDeviceSP device, const QRect &area) {
            QByteArray bytes(area.width() * area.height() * device->pixelSize(), '\0');
            device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area); return bytes;
        };
        const auto state = [&] {
            QByteArray bytes; QDataStream output(&bytes, QIODevice::WriteOnly);
            const auto background = image->defaultProjectionColor();
            output << QByteArray(reinterpret_cast<const char *>(background.data()), cs->pixelSize());
            const auto nodes = allNodes();
            for (auto node : nodes) {
                output << nodes.indexOf(node->parent()) << node->name() << QByteArray(node->metaObject()->className())
                       << node->x() << node->y() << node->opacity() << node->visible() << node->userLocked()
                       << node->colorLabelIndex() << node->isPinnedToTimeline();
                if (const auto layer = qobject_cast<KisLayer *>(node.data())) {
                    auto flags = layer->channelFlags();
                    if (flags.isEmpty()) flags.fill(true, layer->colorSpace()->channelCount());
                    output << layer->compositeOpId() << flags;
                }
                if (const auto group = qobject_cast<KisGroupLayer *>(node.data()))
                    output << group->passThroughMode() << group->projection()->extent();
                if (qobject_cast<KisPaintLayer *>(node.data()) || qobject_cast<KisTransparencyMask *>(node.data())) {
                    const auto value = node->paintDevice()->defaultPixel();
                    output << QByteArray(reinterpret_cast<const char *>(value.data()), value.colorSpace()->pixelSize())
                           << pixels(node->paintDevice(), inspection);
                }
            }
            return bytes;
        };
        auto oldRoot = image->root();
        const auto before = state(), beforeProjection = pixels(image->projection(), canvas);
        const auto history = lifetime->undoStack()->count();
        QByteArray observedRoot;
        QMetaObject::Connection rootObservation;
        const bool observeRoot = mode == 3 && !words && !varied && !groups;
        if (observeRoot) rootObservation = connect(image.data(), &KisImage::sigLayersChangedAsync, window.get(), [&] {
            if (image->root() == oldRoot) return;
            disconnect(rootObservation); observedRoot = state();
        });
        const auto disconnectRoot = qScopeGuard([&] { disconnect(rootObservation); });
        const QStringList actions{QStringLiteral("merge_layer"), QStringLiteral("merge_layer"), QStringLiteral("flatten_layer"),
                                  QStringLiteral("flatten_image"), QStringLiteral("new_from_visible")};
        auto *action = window->viewManager()->actionManager()->actionByName(actions[mode]);
        QVERIFY(action); QTRY_VERIFY(action->isEnabled());
        if (mode == 0 && groups)
            image->mergeDown(qobject_cast<KisLayer *>(heads.last().data()), KisMetaData::MergeStrategyRegistry::instance()->get("Drop"));
        else action->trigger();
        image->waitForDone();
        QTRY_COMPARE(lifetime->undoStack()->count(), history + 1);
        QTRY_VERIFY(view->currentNode() && view->currentNode() != heads.last());
        auto merged = view->currentNode();
        if (mode == 0 && groups) QVERIFY(qobject_cast<KisGroupLayer *>(merged.data()));
        else QVERIFY(qobject_cast<KisPaintLayer *>(merged.data()));
        QVERIFY(merged->uuid() != originals[0]->uuid() && merged->uuid() != originals[1]->uuid());
        const auto mergedUuid = merged->uuid();
        const auto mergedDevice = [&] { return merged->paintDevice() ? merged->paintDevice() : merged->projection(); };
        const auto capture = [&] {
            result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
            result.mergedPixels.append(pixels(mergedDevice(), inspection));
            result.rootReplaced.append(image->root() != oldRoot);
        };
        capture();
        if (mode == 3) QCOMPARE(pixels(oldRoot->projection(), canvas), beforeProjection);
        if (sparse) {
            merged->paintDevice()->setDefaultPixel(KoColor(QColor(9, 37, 71, 83), cs));
            merged->setDirty(inspection);
            if (!useGpu) image->refreshGraphAsync();
            image->waitForDone(); capture();
        }
        const auto after = state(); const auto mergedRaw = pixels(mergedDevice(), inspection);
        if (observeRoot) QTRY_COMPARE(observedRoot, after);
        lifetime->undoStack()->undo(); image->waitForDone();
        QTRY_COMPARE(state(), before); QTRY_COMPARE(pixels(image->projection(), canvas), beforeProjection);
        QCOMPARE(image->root(), oldRoot);
        QCOMPARE(pixels(mergedDevice(), inspection), mergedRaw);
        lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), after);
        QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == mergedUuid);
        capture();
        QTemporaryDir directory; QVERIFY(directory.isValid());
        int saves = 0;
        const auto saveAndOpen = [&] {
            const auto path = directory.filePath(QStringLiteral("merge-%1.kra").arg(++saves));
            if (!lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false)) return false;
            lifetime->waitForSavingToComplete();
            if (!lifetime->openPath(path, KisDocument::DontAddToRecent)) return false;
            image = lifetime->image(); image->waitForDone(); QApplication::processEvents();
            merged.clear();
            for (auto node : allNodes()) if (node->uuid() == mergedUuid) merged = node;
            return bool(merged);
        };
        QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), after);
        auto target = merged;
        if (mode == 0 && groups) {
            QVERIFY(merged->firstChild() && merged->firstChild()->paintDevice());
            target = merged->firstChild();
        }
        manager->slotUiActivatedNode(target); manager->slotSetSelectedNodes({target});
        QTRY_COMPARE(view->currentNode(), target);
        // Root identity is compared around merge/history, while KRA keeps the
        // persistent identity of the editable output layer.
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        auto *resources = window->viewManager()->canvasResourceProvider();
        resources->setPaintOpPreset(m_preset); resources->setFGColor(KoColor(QColor(29, 211, 137), cs));
        resources->setOpacity(0.5); resources->setCurrentCompositeOp(QStringLiteral("normal"));
        auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
        QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger(); image->waitForDone();
        const auto filled = state(); QVERIFY(filled != after);
        lifetime->undoStack()->undo(); image->waitForDone(); QTRY_COMPARE(state(), after);
        lifetime->undoStack()->redo(); image->waitForDone(); QTRY_COMPARE(state(), filled);
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        QVERIFY2(saveAndOpen(), qPrintable(lifetime->errorMessage())); QTRY_COMPARE(state(), filled);
        result.states.append(state()); result.projections.append(pixels(image->projection(), canvas));
        result.mergedPixels.append(pixels(mergedDevice(), inspection));
        result.ok = true;
    };
    run(false, native); QVERIFY(native.ok);
    run(true, gpu); QVERIFY(gpu.ok);
    if (!qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR")) {
        const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR") + QStringLiteral("/merge-") + QLatin1String(QTest::currentDataTag());
        QVERIFY(QDir().mkpath(root));
        for (const bool useGpu : {false, true}) {
            const auto &result = useGpu ? gpu : native;
            const auto prefix = useGpu ? QStringLiteral("gpu-") : QStringLiteral("native-");
            for (int i = 0; i < result.states.size(); ++i) {
                const QList<QPair<QString, QByteArray>> files{{QStringLiteral("state"), result.states[i]},
                    {QStringLiteral("projection"), result.projections[i]}, {QStringLiteral("layer"), result.mergedPixels[i]}};
                for (const auto &entry : files) {
                    QFile file(root + '/' + prefix + QString::number(i) + '.' + entry.first);
                    QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(entry.second), entry.second.size());
                }
            }
        }
    }
    QCOMPARE(gpu.states, native.states);
    QCOMPARE(gpu.projections, native.projections);
    QCOMPARE(gpu.mergedPixels, native.mergedPixels);
    QCOMPARE(gpu.rootReplaced, native.rootReplaced);
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
        owner = paintImageWithEncoding(execution, QSize(32, 16));
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
    const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
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
        const auto owner = paintImageWithEncoding(execution, QSize(32, 16));
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

void KisGpuDocumentViewTest::transparencyMaskActionsUseTheActiveGpuPaintLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Transparency mask actions"));
    QVERIFY(image);
    const auto node = image->root()->firstChild();
    QVERIFY(node);
    const auto layer = owner->operations().activeLayer();
    QVERIFY(image->setGpuSelectionRect(QRect(32, 32, 64, 64)));
    QSignalSpy changes(owner.data(), &Gpu::PaintImage::changed);

    QVERIFY(!owner->layerTransparencyMask(layer));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(node));
    QCOMPARE(changes.size(), 1);
    auto version = owner->operations().snapshot();
    QVERIFY(version->layers.first().transparencyMask);
    QCOMPARE(version->layers.first().transparencyMask->bounds(), QRect(32, 32, 64, 64));
    QVERIFY(image->gpuTransparencyMaskVisible(node));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::AddTransparencyMask);

    QVERIFY(image->setGpuTransparencyMaskVisible(node, false));
    QVERIFY(!owner->layerTransparencyMaskVisible(layer));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::ChangeMaskVisibility);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->layerTransparencyMaskVisible(layer));
    QVERIFY(image->gpuTransparencyMaskVisible(node));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(!owner->layerTransparencyMaskVisible(layer));

    QVERIFY(image->removeGpuTransparencyMask(node));
    QVERIFY(!owner->layerTransparencyMask(layer));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::RemoveTransparencyMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->layerTransparencyMask(layer));

    // Adding without a selection is rejected without adding history.
    QVERIFY(image->removeGpuTransparencyMask(node));
    QVERIFY(image->clearGpuSelection());
    const auto count = owner->operations().undoCount();
    QVERIFY(!image->addGpuTransparencyMaskFromSelection(node));
    QCOMPARE(owner->operations().undoCount(), count);
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
}

void KisGpuDocumentViewTest::invertColorsActionUsesGpuDocumentAndDiagnosesUnsupportedFilters()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!KisFilterRegistry::instance()->value(QStringLiteral("invert")))
        KisFilterRegistry::instance()->add(KisFilterSP(new GpuDocumentInvertFilter));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Invert Colors action")));
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
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("krita_filter_invert"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::InvertColors);
    const auto inverted = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(inverted.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    QCOMPARE(pixel(inverted.bytes, 16, 16), QByteArray::fromHex("37e1b9ff"));
    QCOMPARE(pixel(inverted.bytes, 47, 47), QByteArray::fromHex("37e1b9ff"));
    QCOMPARE(pixel(inverted.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(inverted.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes == QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("37e1b9ff"));

    // Unsupported filter identities keep their action but report a diagnostic;
    // they must not run the CPU filter against a view copy.
    QSignalSpy messages(window->viewManager(), &KisViewManager::floatingMessageRequested);
    QVERIFY(messages.isValid());
    KisFilterConfigurationSP unsupported(new KisFilterConfiguration(
        QStringLiteral("levels"), 1, KisGlobalResourcesInterface::instance()));
    const auto beforeUnsupported = owner->operations().snapshot();
    window->viewManager()->filterManager()->apply(unsupported);
    QTRY_COMPARE(messages.size(), 1);
    QVERIFY(messages.first().at(0).toString().contains(QStringLiteral("not yet available for GPU documents")));
    QCOMPARE(owner->operations().snapshot(), beforeUnsupported);
    QCOMPARE(lifetime->undoStack()->count(), count + 1);
}

void KisGpuDocumentViewTest::desaturateActionAppliesGrayscaleOnGpuDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QVERIFY(KisFilterRegistry::instance()->value(QStringLiteral("desaturate")));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Desaturate action")));
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
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("krita_filter_desaturate"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::Grayscale);
    const auto filtered = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(filtered.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    // Rec.709 luminance of (200, 30, 70) is 69 (0x45).
    QCOMPARE(pixel(filtered.bytes, 16, 16), QByteArray::fromHex("454545ff"));
    QCOMPARE(pixel(filtered.bytes, 47, 47), QByteArray::fromHex("454545ff"));
    QCOMPARE(pixel(filtered.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(filtered.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes == QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("454545ff"));
}

void KisGpuDocumentViewTest::hsvAdjustmentActionUsesProductSettingsOnGpuDocument()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QVERIFY(KisFilterRegistry::instance()->value(QStringLiteral("hsvadjustment")));
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->fillLayer(layer, {200, 30, 70}), Gpu::Result::Success);
    QCOMPARE(owner->setSelectionRect(QRect(16, 16, 32, 32)), Gpu::Result::Success);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("HSV Adjustment action")));
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
    auto *manager = window->viewManager()->nodeManager();
    const auto node = view->image()->root()->firstChild();
    QVERIFY(node);
    manager->slotUiActivatedNode(node);

    auto *action = window->viewManager()->actionManager()->actionByName(
        QStringLiteral("krita_filter_hsvadjustment"));
    QVERIFY(action);
    QVERIFY(action->isEnabled());
    const auto count = lifetime->undoStack()->count();
    QCOMPARE(owner->readRgbaPixels(layer, QRect(8, 8, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }

    // The action opens the product settings dialog and starts no CPU filter
    // stroke against the view's paint-device copy.
    QDialog *dialog = nullptr;
    QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuHsvAdjustmentDialog"))));
    QVERIFY(dialog);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    auto *hue = dialog->findChild<QSpinBox *>(QStringLiteral("hSpinBox"));
    auto *saturation = dialog->findChild<QSpinBox *>(QStringLiteral("sSpinBox"));
    auto *value = dialog->findChild<QSpinBox *>(QStringLiteral("vSpinBox"));
    auto *type = dialog->findChild<QComboBox *>(QStringLiteral("cmbType"));
    auto *colorize = dialog->findChild<QCheckBox *>(QStringLiteral("chkColorize"));
    auto *compatibility = dialog->findChild<QCheckBox *>(QStringLiteral("chkCompatibilityMode"));
    QVERIFY(hue);
    QVERIFY(saturation);
    QVERIFY(value);
    QVERIFY(type);
    QVERIFY(colorize);
    QVERIFY(compatibility);
    hue->setValue(60);
    saturation->setValue(0);
    value->setValue(0);
    type->setCurrentIndex(0);
    colorize->setChecked(false);
    compatibility->setChecked(false);
    auto *buttons = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    buttons->button(QDialogButtonBox::Ok)->click();

    QTRY_COMPARE(lifetime->undoStack()->count(), count + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()), Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(lifetime->undoStack()->undoText(), kundo2_i18n("HSV Adjustment").toString());
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    const auto filtered = owner->readRgbaPixels(layer, QRect(0, 0, 64, 64), 64 * 64 * 4);
    QCOMPARE(filtered.code, Gpu::ExecutionCode::Success);
    const auto pixel = [&](const QByteArray &bytes, int x, int y) { return bytes.mid((y * 64 + x) * 4, 4); };
    // Hue +60 on (200,30,70) is (200,160,30); the selection clips the result.
    QCOMPARE(pixel(filtered.bytes, 16, 16), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(filtered.bytes, 47, 47), QByteArray::fromHex("c8a01eff"));
    QCOMPARE(pixel(filtered.bytes, 48, 48), QByteArray::fromHex("c81e46ff"));
    QCOMPARE(pixel(filtered.bytes, 8, 8), QByteArray::fromHex("c81e46ff"));

    // The action's undo entry restores the unfiltered pixels and redo reapplies.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c8a01eff"));

    // A configuration without a `type` key follows the product default (HSL).
    // The same saturation value differs from the HSV rule asserted above.
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    KisFilterConfigurationSP defaultMode(new KisFilterConfiguration(
        QStringLiteral("hsvadjustment"), 1, KisGlobalResourcesInterface::instance()));
    defaultMode->setProperty("h", 0);
    defaultMode->setProperty("s", 50);
    defaultMode->setProperty("v", 0);
    defaultMode->setProperty("compatibilityMode", false);
    window->viewManager()->filterManager()->apply(defaultMode);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()), Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("e60036ff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes, QByteArray::fromHex("c81e46ff"));

    // HSY, YUV and Colorize are applied through the GPU path. The UI freezes
    // the document color space's luma coefficients at apply time; no CPU
    // filter stroke runs against the view's paint-device copy.
    QSignalSpy messages(window->viewManager(), &KisViewManager::floatingMessageRequested);
    QVERIFY(messages.isValid());
    const auto configuration = [](int type, int hue, int saturation, int value,
                                  bool colorize, bool compatibility = false) {
        KisFilterConfigurationSP result(new KisFilterConfiguration(
            QStringLiteral("hsvadjustment"), 1, KisGlobalResourcesInterface::instance()));
        result->setProperty("h", hue);
        result->setProperty("s", saturation);
        result->setProperty("v", value);
        result->setProperty("type", type);
        result->setProperty("colorize", colorize);
        result->setProperty("compatibilityMode", compatibility);
        return result;
    };
    const auto documentLuma = [&] {
        const QVector<qreal> coefficients = view->image()->colorSpace()->lumaCoefficients();
        return std::array<double, 3>{double(coefficients[0]), double(coefficients[1]),
                                     double(coefficients[2])};
    };
    const auto lastFilterOperation = [&] {
        return std::get<Gpu::FilterSnapshot>(
            owner->operations().snapshot()->layers.first().operations.last());
    };

    window->viewManager()->filterManager()->apply(configuration(3, 0, 50, 0, false));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("ff0a46ff"));
    {
        const auto operation = lastFilterOperation();
        QCOMPARE(operation->hsvType, Gpu::HsvAdjustmentType::Hsy);
        QVERIFY(!operation->colorize);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    window->viewManager()->filterManager()->apply(configuration(4, 30, 20, -10, false));
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::HsvAdjustment);
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("f10074ff"));
    {
        const auto operation = lastFilterOperation();
        QCOMPARE(operation->hsvType, Gpu::HsvAdjustmentType::Yuv);
        QVERIFY(!operation->colorize);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    // The product dialog's Colorize option reaches the GPU operation without
    // starting a CPU preview stroke.
    try { action->trigger(); } catch (const std::exception &error) { QFAIL(error.what()); }
    QTRY_VERIFY((dialog = window->findChild<QDialog *>(QStringLiteral("gpuHsvAdjustmentDialog"))));
    QVERIFY(dialog);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
    hue = dialog->findChild<QSpinBox *>(QStringLiteral("hSpinBox"));
    saturation = dialog->findChild<QSpinBox *>(QStringLiteral("sSpinBox"));
    value = dialog->findChild<QSpinBox *>(QStringLiteral("vSpinBox"));
    type = dialog->findChild<QComboBox *>(QStringLiteral("cmbType"));
    colorize = dialog->findChild<QCheckBox *>(QStringLiteral("chkColorize"));
    buttons = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(hue);
    QVERIFY(saturation);
    QVERIFY(value);
    QVERIFY(type);
    QVERIFY(colorize);
    QVERIFY(buttons);
    type->setCurrentIndex(0);
    colorize->setChecked(true);
    hue->setValue(120);
    saturation->setValue(80);
    value->setValue(0);
    buttons->button(QDialogButtonBox::Ok)->click();
    QTRY_COMPARE(owner->operations().historyAction(owner->operations().undoCount()),
                 Gpu::HistoryAction::HsvAdjustment);
    {
        const auto operation = lastFilterOperation();
        QVERIFY(operation->colorize);
        QCOMPARE(operation->hue, 120);
        QCOMPARE(operation->saturation, 80);
        QCOMPARE(operation->value, 0);
        QVERIFY(operation->luma == documentLuma());
    }
    QCOMPARE(owner->readRgbaPixels(layer, QRect(16, 16, 1, 1), 4).bytes,
             QByteArray::fromHex("0e7c0eff"));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());

    // Unsupported modes and values outside the selected product slider range
    // are diagnosed without changing pixels or history.
    const auto beforeRejected = owner->operations().snapshot();
    const auto beforeRejectedCount = owner->operations().undoCount();
    window->viewManager()->filterManager()->apply(configuration(5, 0, 0, 0, false));
    QTRY_COMPARE(messages.size(), 1);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("not available for GPU documents")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);

    // Colorize with the YUV mode uses the 0..100 Yellow-Blue range.
    window->viewManager()->filterManager()->apply(configuration(4, 120, 80, 0, true));
    QTRY_COMPARE(messages.size(), 2);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("outside the product slider ranges")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);

    // Out-of-range product values are rejected rather than clamped.
    window->viewManager()->filterManager()->apply(configuration(0, 200, 0, 0, false));
    QTRY_COMPARE(messages.size(), 3);
    QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("outside the product slider ranges")));
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), beforeRejectedCount);
    QVERIFY(!window->viewManager()->filterManager()->isStrokeRunning());
}

void KisGpuDocumentViewTest::layerPanelShowsAndSelectsGpuTransparencyMaskChild()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child")));
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
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);
    const auto maskNode = layer->firstChild();
    QVERIFY(!maskNode);

    // A nonuniform coverage proves the thumbnail is derived from the mask.
    QByteArray coverage(32 * 32, '\0');
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 16; ++x) coverage[y * 32 + x] = 1;
        // Keep the covered bounds square so the precise thumbnail retains both
        // selected and unselected columns.
        coverage[y * 32 + 31] = 1;
    }
    const auto selectionMask = Gpu::SelectionMask::fromCoverage(QRect(0, 0, 32, 32), coverage,
                                                                QRect(0, 0, 64, 64));
    QVERIFY(selectionMask);
    QVERIFY(image->setGpuSelectionMask(selectionMask));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));

    auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(image->isGpuTransparencyMaskView(child));

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    model->setPreferredThumnalSize(32);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    const auto index = proxy->indexFromNode(child);
    const auto thumbnail = proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>();
    QVERIFY(!thumbnail.isNull());
    QCOMPARE(thumbnail.size(), QSize(32, 32));
    QCOMPARE(qGray(thumbnail.pixel(4, 16)), 255);
    QCOMPARE(qGray(thumbnail.pixel(28, 16)), 0);

    auto *manager = window->viewManager()->nodeManager();
    QCOMPARE(view->currentNode(), layer);
    const auto undoCount = lifetime->undoStack()->count();
    manager->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);
    QCOMPARE(manager->activeNode(), child);
    QVERIFY(view->currentMask());
    QCOMPARE(view->currentMask().data(), child.data());
    QCOMPARE(lifetime->undoStack()->count(), undoCount);

    // A selected mask child routes brush input into the parent layer's retained
    // mask instead of falling back to the paint layer. One stroke adds exactly
    // one history entry and refreshes the panel child and its thumbnail.
    QVERIFY(image->clearGpuSelection());
    const auto layerId = owner->operations().activeLayer();
    const auto maskBefore = owner->layerTransparencyMask(layerId);
    QVERIFY(maskBefore);
    QCOMPARE(maskBefore->coverage(20, 20), quint8(0));
    window->viewManager()->canvasResourceProvider()->setFGColor(KoColor(Qt::white, image->colorSpace()));
    const auto brush = captureGpuBrushResources(image, child, view->canvasBase()->resourceManager());
    QCOMPARE(brush.layer, layerId);
    QVERIFY(brush.diagnostic.isEmpty());
    QVERIFY(brush.transparencyMask);
    const auto beforeThumbnail = proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>();
    const auto maskHistory = lifetime->undoStack()->count();
    const auto stroke = owner->beginTransparencyMaskStroke(layerId, brush.brush, 0);
    QVERIFY(!stroke.isNull());
    QCOMPARE(owner->appendSamples(stroke, {{{20, 20}, 1}}), Gpu::Result::Success);
    QCOMPARE(owner->commitStroke(stroke), Gpu::Result::Success);
    QCOMPARE(lifetime->undoStack()->count(), maskHistory + 1);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::PaintStroke);
    const auto paintedCoverage = owner->execution().readSelectionPixels(owner->layerTransparencyMask(layerId), {20, 20, 1, 1}, 1);
    QCOMPARE(paintedCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(paintedCoverage.bytes[0]) > 0);
    quint8 painted = 0;
    child->paintDevice()->readBytes(&painted, 20, 20, 1, 1);
    QVERIFY(painted > 0);
    QTRY_VERIFY(proxy->data(index, int(KisNodeModel::BeginThumbnailRole) + 32).value<QImage>() != beforeThumbnail);

    // The selection fill action targets the retained mask as well and leaves
    // the parent layer's pixels untouched.
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    QVERIFY(image->setGpuSelectionRect(QRect(40, 40, 8, 8)));
    QTRY_COMPARE(owner->selectionRect(), QRect(40, 40, 8, 8));
    auto *fillAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fillAction);
    QVERIFY(fillAction->isEnabled());
    fillAction->trigger();
    QTRY_COMPARE(lifetime->undoStack()->count(), maskHistory + 3);
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::FillLayer);
    const auto filledCoverage = owner->execution().readSelectionPixels(owner->layerTransparencyMask(layerId), {44, 44, 1, 1}, 1);
    QCOMPARE(filledCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(filledCoverage.bytes[0]) > 0);
    quint8 filled = 0;
    child->paintDevice()->readBytes(&filled, 44, 44, 1, 1);
    QVERIFY(filled > 0);
    const auto layerPixels = owner->readRgbaPixels(layerId, QRect(44, 44, 1, 1), 4);
    QCOMPARE(layerPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(layerPixels.bytes.at(3)), quint8(0));

    manager->slotUiActivatedNode(layer);
    QCOMPARE(view->currentNode(), layer);
    QVERIFY(!view->currentMask());
    QCOMPARE(lifetime->undoStack()->count(), maskHistory + 3);
}

void KisGpuDocumentViewTest::maskPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("figureFillStyle");
    QTest::addColumn<bool>("whiteDefault");
    QTest::addColumn<bool>("groupOwner");
    for (const auto *name : {"d)_Ink-3_Gpen.kpp", "a)_Eraser_Small.kpp", "b)_Basic-5_Size_default.kpp",
                             "b)_Basic-2_Opacity.kpp", "a)_Eraser_Circle.kpp", "b)_Basic-5_Size_Opacity.kpp", "d)_Ink-7_Brush_Rough.kpp", "y)_Screentones_Regular.kpp", "u)_Pixel_Art_Dithering.kpp", "u)_Pixel_Art.kpp", "u)_Pixel_Art_Fill.kpp",
                             "h)_Charcoal_pencil_large.kpp", "h)_Charcoal_Pencil_Medium.kpp", "h)_Charcoal_Pencil_Thin.kpp"})
        for (const bool words : {false, true}) {
            const QByteArray row = QByteArray(name) + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << qreal(-1) << int(KisFigurePaintingOptions::FillStyleNone) << false << false;
        }
    for (const bool words : {false, true})
        QTest::newRow(words ? "Gpen-low-flow-u16" : "Gpen-low-flow-u8") << QStringLiteral("d)_Ink-3_Gpen.kpp") << words << qreal(.37) << int(KisFigurePaintingOptions::FillStyleNone) << false << false;
    for (const bool words : {false, true})
        QTest::newRow(words ? "Dithering-soft-low-flow-u16" : "Dithering-soft-low-flow-u8")
            << QStringLiteral("u)_Pixel_Art_Dithering.kpp") << words << qreal(.37) << int(KisFigurePaintingOptions::FillStyleNone) << false << false;
    for (const auto *preset : {"b)_Basic-5_Size_default.kpp", "d)_Ink-7_Brush_Rough.kpp", "u)_Pixel_Art_Dithering.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray("filled-") + preset + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(preset) << words << qreal(-1) << int(KisFigurePaintingOptions::FillStyleBackgroundColor) << false << false;
        }
    for (const auto *preset : {"b)_Basic-5_Size_default.kpp", "d)_Ink-7_Brush_Rough.kpp", "u)_Pixel_Art_Dithering.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray("pattern-filled-") + preset + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(preset) << words << qreal(-1)
                << int(KisFigurePaintingOptions::FillStylePattern) << false << false;
        }


    for (const auto *preset : {"b)_Basic-5_Size_default.kpp", "d)_Ink-3_Gpen.kpp", "u)_Pixel_Art_Dithering.kpp",
                              "b)_Basic-3_Flow.kpp", "b)_Basic-4_Flow_Opacity.kpp", "d)_Ink-2_Fineliner.kpp", "c)_Pencil-1_Hard.kpp", "c)_Pencil-2.kpp", "c)_Pencil-4_Soft.kpp", "c)_Pencil-6_Quick_Shade.kpp", "c)_Pencil-3_Large_4B.kpp", "c)_Pencil-5_Tilted.kpp", "b)_Basic-6_Details.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray("white-default-") + preset + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(preset) << words << qreal(-1)
                << int(KisFigurePaintingOptions::FillStyleNone) << true << false;
        }


}

void KisGpuDocumentViewTest::groupMaskPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<qreal>("flow");
    QTest::addColumn<int>("figureFillStyle");
    QTest::addColumn<bool>("whiteDefault");
    QTest::addColumn<bool>("groupOwner");
    for (const auto *preset : {"c)_Pencil-4_Soft.kpp", "c)_Pencil-6_Quick_Shade.kpp", "a)_Eraser_Circle.kpp"})
        for (bool words : {false, true}) for (bool white : {false, true}) {
            const QByteArray row = QByteArray(white ? "group-white-" : "group-finite-") + preset + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(preset) << words << qreal(-1)
                << int(KisFigurePaintingOptions::FillStyleNone) << white << true;
        }

}

void KisGpuDocumentViewTest::groupMaskPresetsKeepOrdinaryEditing()
{
    maskPresetsKeepOrdinaryEditing();
}

void KisGpuDocumentViewTest::maskPresetsKeepOrdinaryEditing()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, presetName);
    const bool pencil = presetName.startsWith(QLatin1String("c)_Pencil-"));
    const bool imageTip = presetName == QLatin1String("b)_Basic-6_Details.kpp") || presetName == QLatin1String("c)_Pencil-4_Soft.kpp") || presetName == QLatin1String("c)_Pencil-6_Quick_Shade.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool incrementalPencil = presetName == QLatin1String("c)_Pencil-3_Large_4B.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool fineliner = presetName == QLatin1String("d)_Ink-2_Fineliner.kpp");
    const auto restoreTiming = fixedBrushPointerTiming(fineliner);
    QFETCH(bool, words);
    QFETCH(qreal, flow);
    QFETCH(int, figureFillStyle);
    QFETCH(bool, whiteDefault);
    QFETCH(bool, groupOwner);
    const bool filledFigure = figureFillStyle != KisFigurePaintingOptions::FillStyleNone;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
    QVERIFY(bundle->valid());
    auto sourcePreset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(sourcePreset);
    if (imageTip) {
        QDomDocument definition;
        QVERIFY(definition.setContent(sourcePreset->settings()->getString("brush_definition")));
        const auto file = QFileInfo(definition.documentElement().attribute("filename")).fileName();
        const auto tip = bundle->resource(QStringLiteral("brushes/") + file);
        QVERIFY(tip);
        auto source = KisGlobalResourcesInterface::instance()->source<KoResource>(ResourceType::Brushes);
        if (!source.bestMatch(tip->md5Sum(), tip->filename(), tip->name())) {
            KisResourceModel brushResources(ResourceType::Brushes);
            QVERIFY(brushResources.addResource(tip));
        }
    }

    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0) QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0) QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    if (sourcePreset->settings()->getBool("Texture/Pattern/Enabled")) {
        const auto file = QFileInfo(sourcePreset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        const auto paper = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(paper);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(paper->md5Sum(), paper->filename(), paper->name()))
            QVERIFY(patterns.addResource(paper));
        sourcePreset->settings()->setProperty("Texture/Pattern/isRandomOffsetX", false);
        sourcePreset->settings()->setProperty("Texture/Pattern/isRandomOffsetY", false);
        sourcePreset->settings()->setProperty("Texture/Pattern/OffsetX", presetName.startsWith(QLatin1String("d)")) ? 7 : 0);
        sourcePreset->settings()->setProperty("Texture/Pattern/OffsetY", presetName.startsWith(QLatin1String("d)")) ? 11 : 0);
    }
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 64, 64);
    const QByteArray initial(area.width() * area.height(), char(whiteDefault ? 255 : 128));
    QList<QByteArray> expected;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto nativeGroupSource = directory.filePath(QStringLiteral("native-group-source.kra"));
    for (const bool gpu : pencil && !incrementalPencil ? QList<bool>{true} : QList<bool>{false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        auto preset = sourcePreset->clone().dynamicCast<KisPaintOpPreset>();
        if (flow >= 0) {
            preset->settings()->setProperty("FlowValue", flow);
            if (presetName == QLatin1String("u)_Pixel_Art_Dithering.kpp")) {
                preset->settings()->setProperty("SharpnessValue", .65);
                preset->settings()->setProperty("Sharpness/alignoutline", true);
                preset->settings()->setProperty("Sharpness/softness", 45);
            }
        }
        preset->setFilename((gpu ? QStringLiteral("gpu-mask-") : QStringLiteral("cpu-mask-")) + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
        KisResourceModel presets(ResourceType::PaintOpPresets); QVERIFY(presets.addResource(preset));
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false); document->setFileBatchMode(true);
        const bool nativeGroupInput = groupOwner && gpu && QFileInfo::exists(nativeGroupSource);
        if (nativeGroupInput) {
            QVERIFY2(document->openPath(nativeGroupSource, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
        } else {
            QVERIFY(document->newImage(QStringLiteral("Ordinary mask editing"), area.width(), area.height(), cs,
                                       KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        }
        auto image = KisImageSP(document->image());
        auto layer = image->root()->firstChild(); QVERIFY(layer);
        if (groupOwner && !nativeGroupInput) {
            const KoColor color(QColor(191, 43, 97, 255), cs);
            const QByteArray pixel(reinterpret_cast<const char *>(color.data()), cs->pixelSize());
            layer->paintDevice()->writeBytes(reinterpret_cast<const quint8 *>(pixel.repeated(area.width() * area.height()).constData()), area);
            if (gpu) {
                const auto owner = image->gpuImage();
                const auto paint = owner->operations().activeLayer();
                QCOMPARE(owner->groupLayers({paint}, paint, QStringLiteral("Masked group")), Gpu::Result::Success);
                layer = image->root()->firstChild();
            } else {
                KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Masked group"), 255);
                auto paint = layer;
                QVERIFY(image->removeNode(paint)); QVERIFY(image->addNode(group)); QVERIFY(image->addNode(paint, group));
                layer = group;
            }
            QVERIFY(layer && layer->inherits("KisGroupLayer"));
            layer->setDirty(area); image->waitForDone();
        }
        if (nativeGroupInput) {
            QVERIFY(layer->inherits("KisGroupLayer"));
        } else if (gpu && whiteDefault) {
            const auto owner = image->gpuImage();
            const auto version = owner->operations().snapshot();
            const auto target = std::find_if(version->layers.cbegin(), version->layers.cend(),
                [&](const auto &value) { return value.uuid == layer->uuid(); });
            QVERIFY(target != version->layers.cend());
            QCOMPARE(owner->addTransparencyMask(target->id), Gpu::Result::Success);
        } else if (gpu) {
            const auto owner = image->gpuImage(); QVERIFY(owner);
            if (groupOwner) {
                QCOMPARE(owner->setSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, initial, area)), Gpu::Result::Success);
            } else {
                Gpu::RegionFillSettings settings; settings.seed = {16, 16}; settings.threshold = 1;
                QCOMPARE(owner->selectRegion(owner->operations().activeLayer(), settings, Gpu::SelectionCombination::Replace), Gpu::Result::Success);
                QCOMPARE(owner->combineSelectionMask(Gpu::SelectionMask::fromAlphaCoverage(area, initial, area), Gpu::SelectionCombination::Intersect), Gpu::Result::Success);
                QVERIFY(owner->selectionMask()->isGpuGenerated());
            }
            QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
            QVERIFY(image->clearGpuSelection());
        } else {
            KisSelectionSP selection = new KisSelection;
            if (whiteDefault) {
                const quint8 value = 255;
                selection->pixelSelection()->setDefaultPixel(KoColor(&value, selection->pixelSelection()->colorSpace()));
            }
            selection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), area);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(selection); QVERIFY(image->addNode(mask, layer));
            image->refreshGraphAsync(); image->waitForDone();
        }
        auto child = layer->firstChild();
        while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
        QVERIFY(child);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow()); window->resize(800, 600);
        auto *view = window->newView(document.get()); QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        BrushPointerClock pointerClock;
        QPointer<QWindow> input; bool pressed = false;
        const auto reportClose = [&](const char *stage) {
            if (!qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR"))
                qInfo() << "Mask workflow close" << gpu << stage;
        };
        const auto closeWindow = qScopeGuard([&] {
            reportClose("begin");
            if (pressed && input) { if (lifetime) lifetime->image()->requestStrokeCancellation(); QTest::mouseRelease(input, Qt::LeftButton); }
            if (lifetime) { lifetime->setModified(false); window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger(); }
            reportClose("document closed");
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
            reportClose("window destroyed");
            QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            reportClose("deferred deletion complete");
        });
        window->viewManager()->setCurrentView(view);
        for (auto *dock : window->findChildren<QDockWidget*>()) dock->hide();
        window->show(); window->activateWindow();
        auto *profiles = KisInputProfileManager::instance();
        profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                     QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
        profiles->loadProfiles();
        window->viewManager()->nodeManager()->slotUiActivatedNode(child);
        auto *resources = window->viewManager()->canvasResourceProvider();
        QImage patternImage(2, 2, QImage::Format_ARGB32);
        patternImage.setPixelColor(0, 0, QColor(34, 68, 102, 200));
        patternImage.setPixelColor(1, 0, QColor(211, 17, 93, 127));
        patternImage.setPixelColor(0, 1, QColor(71, 181, 241, 255));
        patternImage.setPixelColor(1, 1, QColor(255, 255, 255, 0));
        const KoPatternSP figurePattern(new KoPattern(patternImage, QStringLiteral("Mask pattern"), QString()));
        const auto configure = [&] {
            resources->setPaintOpPreset(preset); resources->setSize(13.75); resources->setOpacity(.7);
            resources->setGlobalAlphaLock(false); resources->setMirrorHorizontal(false); resources->setMirrorVertical(false);
            resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
            resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
            resources->setBGColor(KoColor(QColor(71, 241, 129), cs));
            if (figureFillStyle == KisFigurePaintingOptions::FillStylePattern) resources->slotPatternActivated(figurePattern);
        };
        configure();
        if (gpu) {
            const auto capture = captureGpuBrushResources(image, child, view->canvasBase()->resourceManager());
            QVERIFY2(capture.diagnostic.isEmpty(), qPrintable(capture.diagnostic));
            const qreal resourceOpacity = resources->opacity();
            if (incrementalPencil) resources->setOpacity(1);
            const auto restoreProbeOpacity = qScopeGuard([&] { if (incrementalPencil) resources->setOpacity(resourceOpacity); });
            const auto *gray = KoColorSpaceRegistry::instance()->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), QString());
            KisImageSP nativeImage = new KisImage(nullptr, 64, 64, cs, QStringLiteral("Native mask dab reference"));
            KisPaintDeviceSP temporary = new KisPaintDevice(gray);
            KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Gray dab"), 255, temporary);
            nativeImage->addNode(nativeLayer, nativeImage->root());
            if (!incrementalPencil) nativeLayer->setTemporaryTarget(temporary);
            {
                KisPainter painter(temporary);
                painter.setPaintColor(KoColor(Qt::white, gray));
                painter.setCompositeOpId(incrementalPencil ? COMPOSITE_OVER : COMPOSITE_ALPHA_DARKEN);
                painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
                KisDistanceInformation distance;
                KisPaintInformation point(QPointF(32, 32), 1);
                if (pencil || imageTip) {
                    distance = KisDistanceInformation(QPointF(32, 32), 0);
                    point.setRandomSource(KisRandomSourceSP(new KisRandomSource(17)));
                }
                painter.paintAt(point, &distance);
                bool pending;
                do {
                    QVector<KisRunnableStrokeJobData *> jobs;
                    pending = painter.paintOp()->doAsynchronousUpdate(jobs).second;
                    painter.runnableStrokeJobsInterface()->addRunnableJobs(jobs);
                } while (pending);
            }
            nativeLayer->setTemporaryTarget({}); nativeImage->waitForDone();
            QByteArray nativeGray(initial.size() * 2, '\0'), nativeAlpha(initial.size(), '\0');
            temporary->readBytes(reinterpret_cast<quint8 *>(nativeGray.data()), area);
            for (int i = 0; i < nativeAlpha.size(); ++i) nativeAlpha[i] = nativeGray[2 * i + 1];
            Gpu::RasterImage input; input.size = area.size(); input.layers = {{QStringLiteral("Mask dab")}};
            input.layers.first().transparencyMask = Gpu::SelectionMask::empty();
            Gpu::PaintImage probe(image->gpuImage()->execution(), input);
            auto white = capture.brush; white.rgba = {255, 255, 255, 255}; white.opacity = 1;
            const auto operation = probe.beginTransparencyMaskStroke(1, white, 17); QVERIFY(!operation.isNull());
            QCOMPARE(probe.appendSamples(operation, {{{32, 32}, 1}}), Gpu::Result::Success);
            QCOMPARE(probe.commitStroke(operation), Gpu::Result::Success);
            const auto actual = probe.execution().readSelectionPixels(probe.layerTransparencyMask(1), area, nativeAlpha.size());
            QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
            if (actual.bytes != nativeAlpha && imageTip && !qEnvironmentVariableIsEmpty("LIBREPAINT_GPU_RESULT_DIR")) {
                const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR") + QStringLiteral("/image-mask-") +
                    presetName + (words ? QStringLiteral("-u16") : QStringLiteral("-u8"));
                QVERIFY(QDir().mkpath(root));
                const auto save = [&](const QString &name, const QByteArray &bytes) {
                    QFile file(root + '/' + name);
                    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
                };
                QVERIFY(save(QStringLiteral("native.alpha"), nativeAlpha));
                QVERIFY(save(QStringLiteral("gpu.alpha"), actual.bytes));
                const auto &captured = *white.autoBrush->imageTips;
                qDebug() << "frozen image mask" << white.diameter << white.autoBrush->angle << captured.originalSize
                    << captured.tipCount << "drawing angle offset" << white.autoBrush->drawingAngleOffset;
                // Retain each frozen cell's result to distinguish source/selection
                // failures from sampling failures without altering the reference.
                for (int tip = 0; tip < captured.tipCount; ++tip) {
                    auto single = QSharedPointer<Gpu::ImageBrushTips>::create();
                    single->originalSize = captured.originalSize; single->tipCount = 1;
                    for (auto level : captured.levels) {
                        const auto bytes = captured.pixels.mid((level.wordOffset + tip * level.tipWordStride) * 4,
                                                               level.tipWordStride * 4);
                        level.wordOffset = single->pixels.size() / 4;
                        single->levels.append(level); single->pixels.append(bytes);
                    }
                    auto brush = white;
                    brush.autoBrush->imageTips = single;
                    Gpu::PaintImage candidate(image->gpuImage()->execution(), input);
                    const auto stroke = candidate.beginTransparencyMaskStroke(1, brush, 17);
                    QVERIFY(!stroke.isNull());
                    QCOMPARE(candidate.appendSamples(stroke, {{{32, 32}, 1}}), Gpu::Result::Success);
                    QCOMPARE(candidate.commitStroke(stroke), Gpu::Result::Success);
                    const auto pixels = candidate.execution().readSelectionPixels(candidate.layerTransparencyMask(1), area, nativeAlpha.size());
                    QCOMPARE(pixels.code, Gpu::ExecutionCode::Success);
                    int differences = 0;
                    for (int p = 0; p < nativeAlpha.size(); ++p) differences += pixels.bytes[p] != nativeAlpha[p];
                    qDebug() << "frozen tip" << tip << "native differing pixels" << differences;
                    QVERIFY(save(QStringLiteral("tip-%1.alpha").arg(tip), pixels.bytes));
                }
            }
            for (int i = 0; i < nativeAlpha.size(); ++i)
                QVERIFY2(actual.bytes[i] == nativeAlpha[i], qPrintable(QStringLiteral("Native GrayA8 dab at %1,%2: GPU %3 native %4")
                    .arg(i % area.width()).arg(i / area.width()).arg(quint8(actual.bytes[i])).arg(quint8(nativeAlpha[i]))));
        }
        const auto toolId = filledFigure ? QStringLiteral("KritaShape/KisToolEllipse") : QStringLiteral("KritaShape/KisToolBrush");
        KoToolManager::instance()->switchToolRequested(toolId);
        auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), toolId);
        QVERIFY(tool);
        if (!filledFigure) QVERIFY(tool->setProperty("smoothingType", 0));
        const auto configureFigure = [&] {
            if (!filledFigure) return;
            QComboBox *outline = nullptr, *fill = nullptr;
            for (const auto &panel : tool->optionWidgets()) {
                if (!panel) continue;
                if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbOutline"))) outline = value;
                if (auto *value = panel->findChild<QComboBox*>(QStringLiteral("cmbFill"))) fill = value;
            }
            QVERIFY(outline && fill);
            outline->setCurrentIndex(KisFigurePaintingOptions::StrokeStyleForeground);
            fill->setCurrentIndex(figureFillStyle);
        };
        configureFigure();
        const auto prepareInput = [&] {
            if (filledFigure) view->canvasBase()->snapGuide()->enableSnapping(false);
            view->canvasBase()->canvasWidget()->setFocus(); view->canvasController()->resetCanvasRotation();
            view->canvasController()->mirrorCanvas(false); view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            input = window->windowHandle();
            if (gpu) for (auto *candidate : QGuiApplication::allWindows())
                if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = candidate;
            if (fineliner && input) input->installEventFilter(&pointerClock);
        };
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= 64); prepareInput(); QTRY_VERIFY(input && input->isExposed());
        const auto pixels = [&] {
            QByteArray bytes(initial.size(), '\0');
            child->paintDevice()->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
            return bytes;
        };
        const auto parentPixels = [&] {
            QByteArray bytes(initial.size() * cs->pixelSize(), '\0');
            KisPaintDeviceSP device = layer->paintDevice();
            if (groupOwner) {
                for (auto child = layer->firstChild(); child; child = child->nextSibling())
                    if (child->inherits("KisPaintLayer")) { device = child->paintDevice(); break; }
            }
            if (!device) return QByteArray{};
            device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
            return bytes;
        };
        const auto originalParent = parentPixels(); QVERIFY(!originalParent.isEmpty());
        int state = 0;
        const auto compare = [&] {
            auto actual = pixels();
            if (parentPixels() != originalParent) return false;
            if (groupOwner) {
                QByteArray projection(initial.size() * cs->pixelSize(), '\0');
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
                actual += projection;
            }
            if (pencil && !incrementalPencil) return true; // Fixed-seed random coverage above, state invariants below.
            if (!gpu) expected.append(actual);
            else {
                const auto reference = expected.value(state++);
                if (actual != reference) {
                    const auto root = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                    if (root.isEmpty()) directory.setAutoRemove(false);
                    const QDir output(QDir(root.isEmpty() ? directory.path() : root).filePath(QString::fromLatin1(QTest::currentDataTag())));
                    if (QDir().mkpath(output.path())) {
                        for (const auto &entry : {qMakePair(QStringLiteral("native"), reference), qMakePair(QStringLiteral("gpu"), actual)}) {
                            QFile file(output.filePath(QStringLiteral("mask-state-%1-%2.alpha").arg(state - 1).arg(entry.first)));
                            if (file.open(QIODevice::WriteOnly)) file.write(entry.second);
                        }
                    }
                    for (int i = 0; i < qMin(actual.size(), reference.size()); ++i) if (actual[i] != reference[i]) {
                        qWarning() << "Mask state" << state - 1 << "at" << QPoint(i % area.width(), i / area.width())
                                   << "GPU" << quint8(actual[i]) << "native" << quint8(reference[i]);
                        break;
                    }
                    return false;
                }
            }
            return true;
        };
        const auto stroke = [&](QList<QPointF> points, bool cancel = false) {
            if (filledFigure && points.size() == 1) points = {points.first() - QPointF(4, 4), points.first() + QPointF(4, 4)};
            const auto *converter = view->canvasBase()->coordinatesConverter();
            const auto point = [&](QPointF value) {
                const auto widget = converter->imageToWidget(value).toPoint();
                return gpu ? widget : view->canvasBase()->canvasWidget()->mapTo(window.get(), widget);
            };
            const auto first = point(points.first());
            if (fineliner) pointerClock.advance();
            QTest::mouseMove(input, first);
            const auto local = converter->imageToWidget(points.first()).toPoint();
            QEnterEvent entered(local, local, view->canvasBase()->canvasWidget()->mapToGlobal(local));
            QApplication::sendEvent(gpu ? static_cast<QObject *>(input.data()) : view->canvasBase()->canvasWidget(), &entered);
            pressed = true;
            if (fineliner) pointerClock.advance();
            QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, first);
            for (const auto &value : points.mid(1)) {
                if (fineliner) pointerClock.advance();
                QTest::mouseMove(input, point(value), 5);
            }
            if (fineliner) pointerClock.advance();
            if (cancel) QTest::keyClick(input, Qt::Key_Escape);
            QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, point(points.last())); pressed = false;
            image->waitForDone(); QApplication::processEvents();
        };
        QCOMPARE(pixels(), initial);
        if (filledFigure) {
            const int historyBefore = lifetime->undoStack()->index();
            QByteArray projectionBefore(area.width() * area.height() * cs->pixelSize(), '\0');
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projectionBefore.data()), area);
            stroke({{16, 16}, {48, 48}}, true);
            QCOMPARE(lifetime->undoStack()->index(), historyBefore);
            QCOMPARE(pixels(), initial);
            QCOMPARE(parentPixels(), originalParent);
            QByteArray projectionAfter(projectionBefore.size(), '\0');
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projectionAfter.data()), area);
            QCOMPARE(projectionAfter, projectionBefore);
        }
        if (groupOwner) QVERIFY2(compare(), "Prepared group projection must apply the full initial mask");
        if (groupOwner && !gpu) {
            QVERIFY2(lifetime->saveAs(nativeGroupSource, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
            lifetime->waitForSavingToComplete();
            const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
            if (!results.isEmpty()) {
                QVERIFY(QDir().mkpath(results));
                QVERIFY(QFile::copy(nativeGroupSource, QDir(results).filePath(QStringLiteral("native-group-%1-source.kra")
                    .arg(QString::fromLatin1(QTest::currentDataTag())))));
            }
        }
        const int initialUndoIndex = lifetime->undoStack()->index();
        stroke({{24, 24}, {40, 36}});
        if (filledFigure) QTRY_COMPARE(lifetime->undoStack()->index(), initialUndoIndex + 1);
        const auto painted = pixels();
        if (gpu && painted == initial) {
            const auto owner = image->gpuImage();
            qWarning() << "Unchanged mask stroke history" << initialUndoIndex << lifetime->undoStack()->index()
                       << "active input" << owner->operations().hasActiveStroke();
            for (const auto &value : owner->operations().snapshot()->layers) {
                if (value.uuid != layer->uuid() || !value.transparencyMask) continue;
                const auto paint = value.transparencyMask->paintSource();
                if (!paint) { qWarning() << "Mask has no retained paint source"; continue; }
                for (const auto &source : paint->source->layers) for (const auto &operation : source.operations) {
                    const auto stroke = std::get_if<Gpu::PaintSnapshot>(&operation);
                    if (!stroke || !*stroke) continue;
                    qWarning() << "Retained mask stroke" << (*stroke)->randomSeed << (*stroke)->sampleCount
                               << (*stroke)->brush.diameter << (*stroke)->brush.opacity;
                    for (const auto &sample : (*stroke)->inputSamples())
                        qWarning() << "Retained mask input" << sample.position << sample.pressure << sample.timestamp;
                }
            }
        }
        QVERIFY(painted != initial);
        QVERIFY2(compare(), "Ordinary mask preset painting must match native Alpha8 pixels");
        lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), initial);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), painted);
        resources->setEraserMode(true); stroke({{32, 32}});
        const auto erased = pixels(); QVERIFY(erased != painted);
        QVERIFY2(compare(), "Ordinary mask preset erasing must match native Alpha8 pixels");
        lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), painted);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), erased);
        resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        const QRect selected(4, 4, 12, 12);
        if (gpu) QVERIFY(image->setGpuSelectionRect(selected));
        else { KisPixelSelectionSP selection = new KisPixelSelection; selection->select(selected, 255);
            KisSelectionToolHelper helper(view->canvasBase(), kundo2_i18n("Select Rectangle"));
            helper.selectPixelSelection(selection, SELECTION_REPLACE); image->waitForDone(); }
        QTRY_VERIFY(view->selection());
        QTRY_COMPARE(view->selection()->selectedExactRect(), selected);
        auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color_opacity"));
        QVERIFY(fill && fill->isEnabled()); fill->trigger(); image->waitForDone(); QApplication::processEvents();
        const auto foregroundFilled = pixels(); QVERIFY(foregroundFilled != erased);
        QVERIFY2(compare(), "Selected foreground mask fill must preserve native gray conversion");
        resources->slotPatternActivated(figurePattern);
        resources->setCurrentCompositeOp(QStringLiteral("multiply"));
        auto *patternFill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_pattern_opacity"));
        QVERIFY(patternFill && patternFill->isEnabled()); patternFill->trigger(); image->waitForDone(); QApplication::processEvents();
        const auto patternFilled = pixels(); QVERIFY(patternFilled != foregroundFilled);
        QVERIFY2(compare(), "Selected pattern mask fill must preserve native gray, alpha and blend mode");
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        // Figure painting reuses the same frozen resources and global selection.
        {
            KisFigurePaintingStroke figure(kundo2_i18n("Mask figure"), image, child, view->canvasBase()->resourceManager(),
                KisFigurePaintingOptions::StrokeStyleNone, KisFigurePaintingOptions::FillStyleBackgroundColor);
            figure.paintRect(QRectF(6, 6, 8, 8));
        }
        image->waitForDone(); QApplication::processEvents();
        const auto figureFilled = pixels(); QVERIFY(figureFilled != patternFilled);
        QVERIFY2(compare(), "Selected mask figure must preserve native gray composition");
        {
            KisFigurePaintingStroke figure(kundo2_i18n("Mask outline selection override"), image, child, view->canvasBase()->resourceManager(),
                KisFigurePaintingOptions::StrokeStyleNone, KisFigurePaintingOptions::FillStyleForegroundColor);
            figure.setSelectionOverride({}); figure.paintRect(QRectF(50, 6, 8, 8));
        }
        image->waitForDone(); QApplication::processEvents();
        QVERIFY(pixels() != figureFilled);
        QVERIFY2(compare(), "Selection-outline callers must paint across the frozen selection boundary");
        auto *deselect = window->viewManager()->actionManager()->actionByName(QStringLiteral("deselect"));
        QTRY_VERIFY(deselect && deselect->isEnabled()); deselect->trigger(); image->waitForDone();
        QTRY_VERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
        const auto saved = pixels();
        if (groupOwner) {
            auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            group->setPassThroughMode(true); image->refreshGraphAsync(); image->waitForDone();
            QCOMPARE(pixels(), saved);
            QVERIFY2(compare(), "Pass-through groups must retain the mask and use native child composition");
        }
        const auto path = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
        image = lifetime->image(); layer = image->root()->firstChild(); child = layer->firstChild();
        while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
        QVERIFY(child);
        window->viewManager()->nodeManager()->slotUiActivatedNode(child);
        QTRY_COMPARE(view->currentNode(), child);
        QTRY_VERIFY(image->isIdle());
        QCOMPARE(pixels(), saved); QVERIFY2(compare(), "KRA reload must preserve the entire edited mask");
        if (groupOwner) {
            const auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            QVERIFY(group->passThroughMode());
        }
        QTRY_VERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
        configure(); prepareInput();
        window->activateWindow(); input->requestActivate();
        KoToolManager::instance()->switchToolRequested(toolId);
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
        tool = KoToolManager::instance()->toolById(view->canvasBase(), toolId);
        QVERIFY(tool); configureFigure();
        resources->setFGColor(KoColor(whiteDefault ? Qt::black : Qt::white, cs));
        stroke({{50, 50}});
        QVERIFY2(pixels() != saved, gpu ? "GPU mask input after normal KRA Open must change coverage" : "Native mask input after normal KRA Open must change coverage");
        QVERIFY2(compare(), "Mask preset editing after KRA reload must preserve native pixels");
        const auto edited = pixels(); lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(pixels(), saved);
        lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(pixels(), edited);
        if (groupOwner) {
            auto *group = qobject_cast<KisGroupLayer *>(layer.data()); QVERIFY(group);
            group->setPassThroughMode(false); image->refreshGraphAsync(); image->waitForDone();
            QCOMPARE(pixels(), edited);
            QVERIFY2(compare(), "Returning to isolated composition must restore the retained group mask");
        }
        QVERIFY2(lifetime->saveAs(directory.filePath(gpu ? QStringLiteral("gpu-edited.kra") : QStringLiteral("cpu-edited.kra")),
            KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        if (groupOwner && gpu) {
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            const auto resetEngine = qScopeGuard([&] { qputenv("LIBREPAINT_GPU_ENGINE", "1"); });
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(directory.filePath(QStringLiteral("gpu-edited.kra")), KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto nativeImage = KisImageSP(native->image()); nativeImage->waitForDone();
            auto nativeGroup = nativeImage->root()->firstChild(); QVERIFY(nativeGroup && nativeGroup->inherits("KisGroupLayer"));
            QCOMPARE(nativeGroup->uuid(), layer->uuid());
            KisNodeSP nativeMask, nativePaint;
            for (auto node = nativeGroup->firstChild(); node; node = node->nextSibling()) {
                if (node->inherits("KisTransparencyMask")) nativeMask = node;
                else if (node->inherits("KisPaintLayer")) nativePaint = node;
            }
            QVERIFY(nativeMask && nativePaint); QCOMPARE(nativeMask->uuid(), child->uuid());
            QByteArray actualMask(initial.size(), '\0'), actualPaint(originalParent.size(), '\0');
            nativeMask->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actualMask.data()), area);
            nativePaint->paintDevice()->readBytes(reinterpret_cast<quint8 *>(actualPaint.data()), area);
            QCOMPARE(actualMask, edited); QCOMPARE(actualPaint, originalParent);
            QByteArray nativeProjection(originalParent.size(), '\0'), gpuProjection(originalParent.size(), '\0');
            nativeImage->projection()->readBytes(reinterpret_cast<quint8 *>(nativeProjection.data()), area);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(gpuProjection.data()), area);
            QCOMPARE(nativeProjection, gpuProjection);
        }
    }
}

void KisGpuDocumentViewTest::maskChildFreehandStrokeEditsOnlyCoverage_data()
{
    QTest::addColumn<int>("creation");
    QTest::addColumn<bool>("groupOwner");
    QTest::newRow("retained-finite-mask") << 0 << false;
    QTest::newRow("ordinary-white-default") << 1 << false;
    QTest::newRow("ordinary-selection-consumed") << 2 << false;
    QTest::newRow("group-retained-finite-mask") << 0 << true;
    QTest::newRow("group-ordinary-white-default") << 1 << true;
    QTest::newRow("group-ordinary-selection-consumed") << 2 << true;
}

void KisGpuDocumentViewTest::maskChildFreehandStrokeEditsOnlyCoverage()
{
    QFETCH(int, creation); QFETCH(bool, groupOwner);
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster; raster.size = {64, 64};
    raster.layers = {{QStringLiteral("Mask owner"), true, QUuid::createUuid(), {}}};
    if (groupOwner) {
        raster.layers.first().kind = Gpu::LayerKind::Group;
        Gpu::RasterLayer child{QStringLiteral("Child paint"), true, QUuid::createUuid(), {}};
        child.parentUuid = raster.layers.first().uuid; raster.layers.append(child);
    }
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    const auto layerId = owner->operations().activeLayer();
    if (creation != 1) QCOMPARE(owner->setSelectionRect(QRect(8, 8, 24, 24)), Gpu::Result::Success);
    if (creation == 0) {
        QCOMPARE(owner->addTransparencyMaskFromSelection(layerId), Gpu::Result::Success);
        QCOMPARE(owner->clearSelection(), Gpu::Result::Success);
    }

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask freehand input")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (lifetime) lifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (lifetime) {
            lifetime->setModified(false);
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
    QTest::qWait(100);
    auto image = KisImageSP(lifetime->image());
    auto layer = image->root()->firstChild();
    QVERIFY(layer);
    auto *manager = window->viewManager()->nodeManager();
    if (creation) {
        const auto before = owner->operations().undoCount();
        const auto child = manager->createNode(QStringLiteral("KisTransparencyMask"));
        QVERIFY(child && image->isGpuTransparencyMaskView(child));
        QCOMPARE(view->currentNode(), child);
        QCOMPARE(owner->operations().undoCount(), before + 1);
        QVERIFY(owner->operations().selectionRect().isEmpty());
        QVERIFY(!view->selection() || view->selection()->selectedExactRect().isEmpty());
    }
    const auto originalMask = owner->layerTransparencyMask(layerId);
    QVERIFY(originalMask);
    QCOMPARE(originalMask->coverage(44, 44), quint8(creation == 1 ? 255 : 0));
    auto child = layer->firstChild();
    while (child && !child->inherits("KisTransparencyMask")) child = child->nextSibling();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(image->isGpuTransparencyMaskView(child));
    if (!creation) manager->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);
    QCOMPARE(view->currentMask().data(), child.data());

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
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
    const auto pointAt = [&](QPointF imagePoint) {
        const auto point = converter->imageToWidget(imagePoint).toPoint();
        QTest::mouseMove(native.data(), point);
        QTest::qWait(10);
        return point;
    };
    const auto start = pointAt({40, 40});
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(start));
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    QTest::qWait(10);
    const int undoBefore = owner->operations().undoCount();
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a mask stroke" : qPrintable(errors.first().first().toString()));
    QVERIFY(owner->operations().snapshot()->layers.first().operations.isEmpty());
    const auto end = pointAt({48, 48});
    QTest::qWait(10);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, end);
    pressed = false;
    QTRY_COMPARE(owner->operations().undoCount(), undoBefore + 1);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::PaintStroke);
    const auto edited = owner->layerTransparencyMask(layerId);
    QVERIFY(edited);
    const auto editedCoverage = owner->execution().readSelectionPixels(edited, {0, 0, 64, 64}, 64 * 64);
    QCOMPARE(editedCoverage.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(editedCoverage.bytes[44 * 64 + 44]) > 0);
    QCOMPARE(quint8(editedCoverage.bytes[12 * 64 + 12]), quint8(255));
    QVERIFY(owner->operations().snapshot()->layers.first().operations.isEmpty());
    const auto layerPixels = owner->readRgbaPixels(layerId, QRect(44, 44, 1, 1), 4);
    QCOMPARE(layerPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(quint8(layerPixels.bytes.at(3)), quint8(0));
    quint8 childPixel = 0;
    child->paintDevice()->readBytes(&childPixel, 44, 44, 1, 1);
    QVERIFY(childPixel > 0);

    auto *profiles = KisInputProfileManager::instance();
    profiles->setProfileLocations({QStringLiteral(KRITA_SOURCE_DIR "/krita/data/input/kritadefault.profile")},
                                 QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/input"));
    profiles->loadProfiles();
    const auto moveId = QStringLiteral("KritaTransform/KisToolMove");
    KoToolManager::instance()->switchToolRequested(moveId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), moveId);
    auto *moveTool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), moveId));
    QVERIFY(moveTool);
    for (QWidget *panel : moveTool->optionWidgets())
        if (auto *button = panel->findChild<QRadioButton *>(QStringLiteral("radioSelectedLayer"))) button->setChecked(true);
    const auto beforeMove = owner->operations().snapshot();
    const auto beforeMoveHistory = owner->operations().undoCount();
    window->windowHandle()->requestActivate();
    window->menuBar()->setFocus();
    QTRY_VERIFY(window->menuBar()->hasFocus());
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
    pressed = true;
    QTest::mouseMove(native.data(), converter->imageToWidget(QPointF(46, 37)).toPoint(), 20);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, converter->imageToWidget(QPointF(46, 37)).toPoint());
    pressed = false;
    QVERIFY(owner->operations().hasActiveStroke());
    QVERIFY(owner->operations().preview() != beforeMove);
    QTest::keyClick(window->windowHandle(), Qt::Key_Escape);
    QTRY_VERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), beforeMove);
    QCOMPARE(owner->operations().preview(), beforeMove);
    QCOMPARE(owner->operations().undoCount(), beforeMoveHistory);
}

void KisGpuDocumentViewTest::layerPanelTogglesAndRemovesGpuTransparencyMaskChild()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child actions")));
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
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);
    QVERIFY(image->setGpuSelectionRect(QRect(8, 8, 32, 32)));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
    const auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<KisNodeModel*>(proxy->sourceModel());
    QVERIFY(model);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    const auto index = proxy->indexFromNode(child);
    auto *manager = window->viewManager()->nodeManager();
    manager->slotUiActivatedNode(child);
    manager->slotSetSelectedNodes({child});
    QCOMPARE(view->currentNode(), child);

    // The panel's visibility toggle is the mask visibility action.
    auto properties = child->sectionModelProperties();
    bool found = false;
    for (auto &property : properties) {
        if (property.id == KisLayerPropertiesIcons::visible.id()) {
            property.state = false;
            found = true;
        }
    }
    QVERIFY(found);
    const auto visibilityHistory = lifetime->undoStack()->count();
    QVERIFY(proxy->setData(index, QVariant::fromValue(properties), KisNodeModel::PropertiesRole));
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!child->visible());
    QCOMPARE(owner->operations().historyAction(owner->operations().undoCount()),
             Gpu::HistoryAction::ChangeMaskVisibility);
    QCOMPARE(lifetime->undoStack()->count(), visibilityHistory + 1);

    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(image->gpuTransparencyMaskVisible(layer));
    QVERIFY(child->visible());
    lifetime->undoStack()->redo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!child->visible());

    // Deleting the panel child removes the retained GPU mask.
    manager->slotUiActivatedNode(child);
    manager->slotSetSelectedNodes({child});
    const auto removalHistory = lifetime->undoStack()->count();
    manager->removeNode();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
    QVERIFY(!layer->firstChild());
    QCOMPARE(view->currentNode(), layer);
    QVERIFY(!view->currentMask());
    QCOMPARE(lifetime->undoStack()->count(), removalHistory + 1);

    // Undo restores both the child and the visibility it had before removal.
    lifetime->undoStack()->undo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    const auto restored = layer->firstChild();
    QVERIFY(restored);
    QVERIFY(restored->inherits("KisTransparencyMask"));
    QVERIFY(owner->operations().snapshot()->layers.first().transparencyMask);
    QVERIFY(!image->gpuTransparencyMaskVisible(layer));
    QVERIFY(!restored->visible());
    QCOMPARE(view->currentNode(), layer);

    lifetime->undoStack()->redo();
    image->waitForDone();
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    QVERIFY(!layer->firstChild());
    QVERIFY(!owner->operations().snapshot()->layers.first().transparencyMask);
}

void KisGpuDocumentViewTest::layerPanelGpuTransparencyMaskChildSurvivesSaving()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    namespace Kra = LibrePaint::ImportExport::Kra;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Mask child saving")));
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
    auto image = KisImageSP(lifetime->image());
    const auto layer = image->root()->firstChild();
    QVERIFY(layer);

    QByteArray coverage(32 * 32, '\0');
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 16; ++x) coverage[y * 32 + x] = 1;
    const auto selectionMask = Gpu::SelectionMask::fromCoverage(QRect(0, 0, 32, 32), coverage,
                                                                QRect(0, 0, 64, 64));
    QVERIFY(selectionMask);
    QVERIFY(image->setGpuSelectionMask(selectionMask));
    QVERIFY(image->addGpuTransparencyMaskFromSelection(layer));
    QVERIFY(image->setGpuTransparencyMaskVisible(layer, false));
    auto child = layer->firstChild();
    QVERIFY(child);
    QVERIFY(child->inherits("KisTransparencyMask"));
    QVERIFY(!child->visible());

    auto *panel = window->findChild<QDockWidget*>(QStringLiteral("KisLayerBox"));
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeView*>(QStringLiteral("listLayers"));
    QVERIFY(tree);
    auto *proxy = qobject_cast<KisNodeFilterProxyModel*>(tree->model());
    QVERIFY(proxy);
    QTRY_VERIFY(proxy->indexFromNode(child).isValid());
    window->viewManager()->nodeManager()->slotUiActivatedNode(child);
    QCOMPARE(view->currentNode(), child);

    const auto identity = child->uuid();
    QCOMPARE(identity, owner->operations().snapshot()->layers.first().transparencyMaskUuid);
    const auto fixed = owner->operations().snapshot();
    auto *manager = window->viewManager()->nodeManager();
    manager->setNodeName(child, QStringLiteral("Retained cutouts"));
    QCOMPARE(child->name(), QStringLiteral("Retained cutouts"));
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskName, child->name());
    QCOMPARE(layer->name(), fixed->layers.first().name);
    lifetime->undoStack()->undo();
    QCOMPARE(child->name(), fixed->layers.first().transparencyMaskName);
    lifetime->undoStack()->redo();
    QCOMPARE(child->name(), QStringLiteral("Retained cutouts"));
    child->setColorLabelIndex(4);
    QCOMPARE(child->colorLabelIndex(), 4);
    QCOMPARE(owner->operations().snapshot()->layers.first().transparencyMaskColorLabelIndex, 4);
    lifetime->undoStack()->undo();
    QCOMPARE(child->name(), fixed->layers.first().transparencyMaskName);
    QCOMPARE(child->colorLabelIndex(), 4);
    lifetime->undoStack()->redo();
    child->setPinnedToTimeline(true);
    QVERIFY(owner->operations().snapshot()->layers.first().transparencyMaskPinnedToTimeline);
    QCOMPARE(fixed->layers.first().transparencyMaskColorLabelIndex, 0);
    QCOMPARE(fixed->layers.first().transparencyMaskUuid, identity);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("mask-child.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == Kra::Status::Success, qPrintable(loaded.diagnostic));
    QVERIFY(loaded.document.image.layers.first().transparencyMask);
    QVERIFY(!loaded.document.image.layers.first().transparencyMaskVisible);
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskUuid, identity);
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskName, QStringLiteral("Retained cutouts"));
    QCOMPARE(loaded.document.image.layers.first().transparencyMaskColorLabelIndex, 4);
    QVERIFY(loaded.document.image.layers.first().transparencyMaskPinnedToTimeline);
    QCOMPARE(loaded.document.image.layers.first().transparencyMask->coverage(4, 16), quint8(255));
    QCOMPARE(loaded.document.image.layers.first().transparencyMask->coverage(28, 16), quint8(0));

    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QVERIFY(reopened);
    lifetime->setCurrentImage(KisImage::fromGpuImage(reopened, QStringLiteral("Reopened mask child")));
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == lifetime->image().data());
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    auto reopenedImage = KisImageSP(lifetime->image());
    QVERIFY(reopenedImage);
    const auto reopenedLayer = reopenedImage->root()->firstChild();
    QVERIFY(reopenedLayer);
    const auto reopenedChild = reopenedLayer->firstChild();
    QVERIFY(reopenedChild);
    QVERIFY(reopenedChild->inherits("KisTransparencyMask"));
    QVERIFY(reopenedImage->isGpuTransparencyMaskView(reopenedChild));
    QVERIFY(!reopenedChild->visible());
    QCOMPARE(reopenedChild->uuid(), identity);
    QCOMPARE(reopenedChild->name(), QStringLiteral("Retained cutouts"));
    QCOMPARE(reopenedChild->colorLabelIndex(), 4);
    QVERIFY(reopenedChild->isPinnedToTimeline());
    window->viewManager()->nodeManager()->setNodeName(reopenedChild, QStringLiteral("Edited after reload"));
    QCOMPARE(reopened->operations().snapshot()->layers.first().transparencyMaskName, QStringLiteral("Edited after reload"));
    lifetime->undoStack()->undo(); QCOMPARE(reopenedChild->name(), QStringLiteral("Retained cutouts"));
    lifetime->undoStack()->redo(); QCOMPARE(reopenedChild->name(), QStringLiteral("Edited after reload"));
    QVERIFY(!reopened->operations().snapshot()->layers.first().transparencyMaskVisible);
    quint8 selected = 0;
    quint8 unselected = 255;
    reopenedChild->paintDevice()->readBytes(&selected, 4, 16, 1, 1);
    reopenedChild->paintDevice()->readBytes(&unselected, 28, 16, 1, 1);
    QCOMPARE(selected, quint8(255));
    QCOMPARE(unselected, quint8(0));

    QTRY_VERIFY(proxy->indexFromNode(reopenedChild).isValid());
}

void KisGpuDocumentViewTest::rectangularSelectionLimitsPainting()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
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
    view->canvasController()->setPreferredCenter({imageSize.width() / 2 + 0.5, imageSize.height() / 2 + 0.5});
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
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
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

void KisGpuDocumentViewTest::nativeClipboardCoverageKeepsPrecisionAndSavedPixels_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("merged");
    QTest::addColumn<bool>("keepInside");
    QTest::addColumn<bool>("implicit");
    for (const bool words : {false, true}) for (const bool merged : {false, true})
        for (const bool keepInside : {false, true}) for (const bool implicit : {false, true}) {
            const auto name = QStringLiteral("%1-%2-%3-%4").arg(words ? QStringLiteral("u16") : QStringLiteral("u8"),
                merged ? QStringLiteral("merged") : QStringLiteral("layer"),
                keepInside ? QStringLiteral("inside") : QStringLiteral("outside")).arg(implicit ? QStringLiteral("implicit") : QStringLiteral("tiles"));
            QTest::newRow(qPrintable(name)) << words << merged << keepInside << implicit;
        }
}

void KisGpuDocumentViewTest::nativeClipboardCoverageKeepsPrecisionAndSavedPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, merged);
    QFETCH(bool, keepInside);
    QFETCH(bool, implicit);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
    const auto pixel = Gpu::rgbaBytes(color, format);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = format;
    raster.layers.append({QStringLiteral("Clipboard source"), true, QUuid::createUuid(),
                          {{{0, 0}, pixel.repeated(Gpu::RgbaTile::byteCount(format) / pixel.size()), format}}});
    if (implicit) {
        raster.layers.first().tiles.clear();
        raster.layers.first().defaultPixel = color;
    } else {
        auto &tile = raster.layers.first().tiles.first().rgba;
        const quint16 alphas[]{0, 1, quint16(words ? 32768 : 128), quint16(words ? 65535 : 255)};
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            auto value = color;
            value[3] = alphas[(x + y) % 4];
            const auto bytes = Gpu::rgbaBytes(value, format);
            std::memcpy(tile.data() + (y * Gpu::RgbaTile::Size + x) * pixel.size(), bytes.constData(), pixel.size());
        }
    }
    const auto owner = paintImageWithEncoding(execution, raster);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Native clipboard"), cs);
    const auto original = owner->operations().snapshot();
    const QUuid uuid = original->layers.first().uuid;
    GpuClipboardMask mask;
    mask.rect = QRect(16, 16, 32, 32);
    mask.keepInside = keepInside;
    mask.coverage = QImage(mask.rect.size(), QImage::Format_Alpha8);
    for (int y = 0; y < mask.coverage.height(); ++y) for (int x = 0; x < mask.coverage.width(); ++x)
        mask.coverage.scanLine(y)[x] = std::array<quint8, 6>{0, 1, 127, 128, 254, 255}[(x + 2 * y) % 6];
    const QRect captured = keepInside ? mask.rect : QRect(QPoint(), raster.size);
    const auto releaseClipboard = qScopeGuard([] { QGuiApplication::clipboard()->clear(); });
    QVERIFY(merged ? KisClipboard::instance()->setGpuProjection(image, 8 * 1024 * 1024, captured, mask) :
                     KisClipboard::instance()->setGpuLayer(image, uuid, 8 * 1024 * 1024, captured, mask));
    QCOMPARE(owner->operations().snapshot(), original);
    const auto *data = qobject_cast<const KisMimeData *>(KisClipboard::instance()->layersMimeData());
    QVERIFY(data);
    QCOMPARE(data->nodes().size(), 1);
    const auto capturedImage = data->nodes().first()->image();
    QVERIFY(capturedImage);
    QCOMPARE(capturedImage->colorSpace()->colorModelId(), cs->colorModelId());
    QCOMPARE(capturedImage->colorSpace()->colorDepthId(), cs->colorDepthId());
    QCOMPARE(capturedImage->colorSpace()->profile()->rawData(), cs->profile()->rawData());
    const auto copied = capturedImage->gpuImage();
    QVERIFY(copied);
    QCOMPARE(copied->operations().snapshot()->format, format);
    const auto source = execution->readRgbaPixels(owner->handle(), original, captured,
                                                  merged ? 0 : original->activeLayer, 8 * 1024 * 1024);
    QCOMPARE(source.code, Gpu::ExecutionCode::Success);
    QByteArray selectedness(captured.width() * captured.height(), '\0');
    for (int y = 0; y < captured.height(); ++y) for (int x = 0; x < captured.width(); ++x) {
        const QPoint point = captured.topLeft() + QPoint(x, y);
        const QPoint local = point - mask.rect.topLeft();
        const quint8 amount = mask.rect.contains(point) ? mask.coverage.constScanLine(local.y())[local.x()] : 0;
        selectedness[y * captured.width() + x] = char(keepInside ? amount : 255 - amount);
    }
    const QByteArray expected = nativeClipboardMask(cs, source.bytes, selectedness, captured.size());
    const auto actual = copied->readRgbaPixels(copied->operations().activeLayer(), QRect(QPoint(), captured.size()), expected.size());
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        const auto output = QDir(results).filePath(QStringLiteral("native-clipboard-") + QString::fromLatin1(QTest::currentDataTag()));
        QVERIFY(QDir().mkpath(output));
        for (const auto &entry : {qMakePair(QStringLiteral("expected.rgba"), expected), qMakePair(QStringLiteral("actual.rgba"), actual.bytes)}) {
            QFile artifact(QDir(output).filePath(entry.first));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(entry.second), entry.second.size());
        }
    }
    QCOMPARE(actual.bytes, expected);
    QCOMPARE(owner->fillLayerColor(owner->operations().activeLayer(), words ? Gpu::RgbaPixel{1, 257, 65534, 65535} :
                                                                          Gpu::RgbaPixel{1, 2, 254, 255}), Gpu::Result::Success);
    QCOMPARE(copied->readRgbaPixels(copied->operations().activeLayer(), QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QByteArray archive = data->data(QStringLiteral("application/zip"));
    QVERIFY(!archive.isEmpty());
    QBuffer buffer(&archive);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(buffer);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.format, format);
    QCOMPARE(loaded.document.image.size, captured.size());
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    const auto layer = reopened->operations().activeLayer();
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QCOMPARE(reopened->fillLayerColor(layer, color), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, pixel.repeated(captured.width() * captured.height()));
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(layer, QRect(QPoint(), captured.size()), expected.size()).bytes, expected);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    if (!results.isEmpty()) {
        QFile artifact(QDir(results).filePath(QStringLiteral("native-clipboard-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kra")));
        QVERIFY(artifact.open(QIODevice::WriteOnly));
        QCOMPARE(artifact.write(archive), archive.size());
    }
}

void KisGpuDocumentViewTest::selectionMergedCopyUsesGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255.0 / 255, false, false) == Gpu::Result::Success);
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
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
    QVERIFY(owner->fillLayer(owner->operations().activeLayer(), {17, 129, 241}, 255.0 / 255, false, false) == Gpu::Result::Success);
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
    KConfigGroup configuration = KSharedConfig::openConfig()->group(QStringLiteral("KisToolSelectRectangular"));
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("roundCornersX", 0);
    configuration.writeEntry("roundCornersY", 0);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
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
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == QLatin1String("KisToolSelectRectangularoption widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    const auto oldAction = options->action();
    const auto oldMode = options->mode();
    const bool oldAA = options->antiAliasSelection();
    const int oldGrow = options->growSelection(), oldFeather = options->featherSelection();
    const auto restoreOptions = qScopeGuard([&] {
        if (!options) return;
        options->setAction(oldAction); options->setMode(oldMode); options->setAntiAliasSelection(oldAA);
        options->setGrowSelection(oldGrow); options->setFeatherSelection(oldFeather);
    });
    options->setMode(PIXEL_SELECTION); options->setAction(SELECTION_REPLACE);
    options->setAntiAliasSelection(false); options->setGrowSelection(0); options->setFeatherSelection(0);
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
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Rectangle);

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
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
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

void KisGpuDocumentViewTest::nativeInvertedSelectionsKeepEditingAndSaving_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("soft");
    for (bool words : {false, true}) for (bool soft : {false, true}) {
        const QByteArray tag = QByteArray(words ? "u16" : "u8") + (soft ? "-soft-offcanvas" : "-rectangle");
        QTest::newRow(tag.constData()) << words << soft;
    }
}

void KisGpuDocumentViewTest::nativeInvertedSelectionsKeepEditingAndSaving()
{
    // Normal fill/clear actions consume native KRA's inverse selection; history
    // and reopened edits must preserve coverage and document-precision pixels.
    QFETCH(bool, words); QFETCH(bool, soft);
    const auto previous = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restore = qScopeGuard([&] {
        if (previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previous);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("native-selection.kra"));
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), 64, 64, cs, "Inverse selection");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->paintDevice()->fill(image->bounds(), KoColor(QColor(77, 113, 157, 255), cs));
        QVERIFY(image->addNode(layer));
        KisSelectionSP selection = new KisSelection;
        const quint8 selected = 255;
        selection->pixelSelection()->setDefaultPixel(KoColor(&selected, KoColorSpaceRegistry::instance()->alpha8()));
        selection->pixelSelection()->select({16, 16, 32, 32}, 0);
        if (soft) {
            const quint8 coverage[]{0, 1, 128, 254, 255};
            selection->pixelSelection()->writeBytes(coverage, {-3, 9, 5, 1});
            selection->pixelSelection()->writeBytes(coverage, {14, 20, 5, 1});
        }
        KisSelectionMaskSP mask = new KisSelectionMask(image);
        mask->setSelection(selection); mask->setActive(true);
        QVERIFY(image->addNode(mask, image->root()));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false));
        document->waitForSavingToComplete();
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            QVERIFY(QFile::copy(baseline, QDir(results).filePath(QStringLiteral("native-inverse-%1-source.kra")
                .arg(QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
    QList<QByteArray> reference;
    for (bool gpu : {false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        int stage = 0;
        for (bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone();
            QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = image->root()->firstChild();
            while (layer && !layer->inherits("KisPaintLayer")) layer = layer->nextSibling();
            QVERIFY(layer);
            std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
            auto *view = window->newView(document.get()); QVERIFY(view);
            QPointer<KisDocument> lifetime(document.release());
            const auto cleanup = qScopeGuard([&] {
                if (lifetime) {
                    lifetime->setModified(false);
                    window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
                }
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
            window->viewManager()->setCurrentView(view); window->show();
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(layer);
            QTRY_COMPARE(view->currentNode(), layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(64 * 64 * cs->pixelSize(), '\0'), projection(pixels.size(), '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), image->bounds());
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), image->bounds());
                const auto selection = image->globalSelection();
                if (!selection) return {};
                QByteArray coverage(70 * 66, '\0');
                selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), {-3, -1, 70, 66});
                return pixels + projection + coverage + char(selection->pixelSelection()->defaultPixel().opacityU8());
            };
            const auto check = [&] {
                const auto state = capture(); QVERIFY(!state.isEmpty());
                const int currentStage = stage++;
                const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!results.isEmpty()) {
                    QFile output(QDir(results).filePath(QStringLiteral("native-inverse-%1-%2-%3.bin")
                        .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? QStringLiteral("gpu") : QStringLiteral("native")).arg(currentStage)));
                    QVERIFY(output.open(QIODevice::WriteOnly)); QCOMPARE(output.write(state), state.size());
                }
                if (!gpu) reference.append(state);
                else QCOMPARE(state, reference.value(currentStage));
            };
            check();
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(m_preset);
            resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
            resources->setOpacity(1); resources->setEraserMode(false);
            resources->setCurrentCompositeOp(QStringLiteral("normal"));
            auto *action = window->viewManager()->actionManager()->actionByName(reopened
                ? QStringLiteral("clear") : QStringLiteral("fill_selection_foreground_color"));
            QVERIFY(action);
            const auto before = capture();
            action->trigger(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 1);
            const auto edited = capture(); QVERIFY(edited != before); check();
            lifetime->undoStack()->undo(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 0);
            QCOMPARE(capture(), before); check();
            lifetime->undoStack()->redo(); image->waitForDone();
            QTRY_COMPARE(lifetime->undoStack()->index(), 1);
            QCOMPARE(capture(), edited); check();
            QVERIFY(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false));
            lifetime->waitForSavingToComplete();
        }
        if (gpu) {
            // The native reader must recover the re-saved global default and
            // exact off-canvas coverage for subsequent selection editing.
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(saved, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto image = KisImageSP(native->image()); image->waitForDone();
            const auto selection = image->globalSelection(); QVERIFY(selection);
            QCOMPARE(selection->pixelSelection()->defaultPixel().opacityU8(), quint8(255));
            QByteArray coverage(70 * 66, '\0');
            selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(coverage.data()), {-3, -1, 70, 66});
            QCOMPARE(coverage, reference.last().mid(2 * 64 * 64 * cs->pixelSize(), coverage.size()));
        }
    }
}

void KisGpuDocumentViewTest::invertedSelectionLimitsPaintingAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
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
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
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
    const auto owner = paintImageWithEncoding(execution, QSize(64, 64));
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
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
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
    KConfigGroup configuration = KSharedConfig::openConfig()->group(QStringLiteral("KisToolSelectElliptical"));
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("antiAliasSelection", false);
    configuration.writeEntry("growSelection", 0);
    configuration.writeEntry("featherSelection", 0);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
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
    view->canvasController()->setPreferredCenter({imageSize.width() / 2 + 0.5, imageSize.height() / 2 + 0.5});
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
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
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

    auto *copyMerged = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_merged"));
    QVERIFY(copyMerged);
    QTRY_VERIFY(copyMerged->isEnabled());
    copyMerged->trigger();
    QTRY_VERIFY(KisClipboard::instance()->hasImage());
    const auto *mergedClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(mergedClipboard);
    const QImage merged = KisClipboard::instance()->getImageWithFallback(mergedClipboard);
    QVERIFY(!merged.isNull());
    QCOMPARE(merged.size(), mask->bounds().size());
    QCOMPARE(merged.pixelColor(80 - mask->bounds().x(), 80 - mask->bounds().y()), QColor(17, 129, 241));
    QCOMPARE(merged.pixelColor(176 - mask->bounds().x(), 176 - mask->bounds().y()).alpha(), 0);

    auto *manager = window->viewManager()->selectionManager();
    QVERIFY(manager);
    manager->copy();
    const auto *layerClipboard = QGuiApplication::clipboard()->mimeData();
    QVERIFY(layerClipboard);
    const QImage layerCopy = KisClipboard::instance()->getImageWithFallback(layerClipboard);
    QVERIFY(!layerCopy.isNull());
    QCOMPARE(layerCopy.size(), mask->bounds().size());
    QCOMPARE(layerCopy.pixelColor(80 - mask->bounds().x(), 80 - mask->bounds().y()), QColor(17, 129, 241));
    QCOMPARE(layerCopy.pixelColor(176 - mask->bounds().x(), 176 - mask->bounds().y()).alpha(), 0);

    auto *copyLayer = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_selection_to_new_layer"));
    QVERIFY(copyLayer);
    QTRY_VERIFY(copyLayer->isEnabled());
    copyLayer->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    QVERIFY(owner->operations().activeLayer() != owner->operations().snapshot()->layers.first().id);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    const QRect beforeMove = owner->selectionRect();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point({80, 80}));
    QTest::mouseMove(native.data(), point({96, 96}), 30);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({96, 96}));
    QTRY_COMPARE(owner->selectionRect(), beforeMove.translated(16, 16));
    QVERIFY(owner->selectionMask());
    QVERIFY(owner->selectionMask()->contains(96, 96));
    QVERIFY(!owner->selectionMask()->contains(beforeMove.left(), beforeMove.top()));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    QVERIFY(owner->selectionMask()->contains(80, 80));

    const auto runFilter = [&](const QString &id, int x, int y) {
        KisOperationConfigurationSP config = new KisOperationConfiguration(id);
        config->setProperty("x-radius", x);
        config->setProperty("y-radius", y);
        config->setProperty("edgeLock", false);
        window->viewManager()->actionManager()->runOperationFromConfiguration(config);
        lifetime->image()->waitForDone();
    };
    runFilter(QStringLiteral("growselection"), 2, 2);
    QTRY_VERIFY(owner->selectionMask()->contains(62, 80));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    runFilter(QStringLiteral("shrinkselection"), 2, 2);
    QTRY_VERIFY(!owner->selectionMask()->contains(64, 80));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionRect(), beforeMove);
    runFilter(QStringLiteral("invertselection"), 0, 0);
    const auto selectedness = [&](QPoint point) {
        const auto selection = lifetime->image()->globalSelection();
        return selection ? int(selection->pixelSelection()->pixel(point).opacityU8()) : -1;
    };
    QTRY_COMPARE(selectedness({80, 80}), 0);
    QCOMPARE(selectedness({176, 176}), 255);
    runFilter(QStringLiteral("growselection"), 2, 2);
    QTRY_COMPARE(selectedness({64, 80}), 255);
    QCOMPARE(selectedness({80, 80}), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    runFilter(QStringLiteral("shrinkselection"), 2, 2);
    QTRY_COMPARE(selectedness({62, 80}), 0);
    QCOMPARE(selectedness({176, 176}), 255);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    runFilter(QStringLiteral("borderselection"), 2, 2);
    QTRY_COMPARE(selectedness({62, 80}), 255);
    QCOMPARE(selectedness({80, 80}), 0);
    QCOMPARE(selectedness({200, 80}), 0);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask()->contains(80, 80));
    runFilter(QStringLiteral("borderselection"), 2, 2);
    QTRY_VERIFY(owner->selectionMask() && !owner->selectionMask()->contains(80, 80));
    QVERIFY(owner->selectionMask()->contains(64, 80));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask()->contains(80, 80));

    runFilter(QStringLiteral("shrinkselection"), 200, 200);
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->selectionMask(), mask);

    QVERIFY(lifetime->image()->clearGpuSelection());
    QTRY_VERIFY(owner->selectionRect().isEmpty());
    QVERIFY(lifetime->image()->reselectGpuSelection());
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask());
    QVERIFY(owner->selectionMask()->contains(80, 80));
    QVERIFY(!owner->selectionMask()->contains(176, 176));
    QCOMPARE(owner->selectionRect(), mask->bounds());
}

void KisGpuDocumentViewTest::polygonToolVerticesCreateFrozenCoverage()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Polygon tool coverage"));
    QVERIFY(image);

    // Document-pixel vertices as collected by the polygonal tool. The L-shape
    // separates a closed polygon from the line or triangle a truncated input
    // sequence would produce.
    const QList<QPointF> vertices{{16, 16}, {96, 16}, {96, 64}, {48, 64}, {48, 96}, {16, 96}};
    const auto expected = [](int x, int y) {
        return (x >= 16 && x < 96 && y >= 16 && y < 64) ||
            (x >= 16 && x < 48 && y >= 64 && y < 96);
    };
    const auto mask = KisSelectionToolHelper::gpuSelectionMaskFromPolygon(vertices, QSize(128, 128));
    QVERIFY(mask);
    QVERIFY(!mask->isAlpha());
    QCOMPARE(mask->bounds(), QRect(16, 16, 80, 80));
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            if (mask->contains(x, y) != expected(x, y)) {
                QFAIL(qPrintable(QStringLiteral("coverage mismatch at %1,%2").arg(x).arg(y)));
            }
        }
    }

    // The tool commits that frozen coverage as one replacement history entry.
    QVERIFY(image->setGpuSelectionMask(mask));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask() == mask);
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
    QCOMPARE(owner->operations().historyAction(1), Gpu::HistoryAction::ChangeSelection);
    QTRY_VERIFY(image->globalSelection());
    QTRY_COMPARE(image->globalSelection()->selectedExactRect(), mask->bounds());

    // It uses the same coverage path as freehand for the shared filters.
    QVERIFY(image->growGpuSelectionMask(2, 2));
    QTRY_VERIFY(owner->selectionMask()->contains(14, 16));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask() == mask);
    QVERIFY(image->invertGpuSelectionMask());
    QTRY_VERIFY(owner->selectionMask() && !owner->selectionMask()->contains(16, 16));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_VERIFY(owner->selectionMask() == mask);
}

void KisGpuDocumentViewTest::pathToolClosedPathCreatesPolygonCoverage()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Path tool coverage"));
    QVERIFY(image);

    QPainterPath path;
    path.moveTo(16, 16);
    path.lineTo(80, 16);
    path.lineTo(80, 64);
    path.lineTo(16, 64);
    path.closeSubpath();

    const QList<QPointF> vertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(path);
    QVERIFY(vertices.size() >= 3);
    const auto mask = KisSelectionToolHelper::gpuSelectionMaskFromPolygon(vertices, QSize(128, 128));
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(16, 16, 64, 48));
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            const bool expected = x >= 16 && x < 80 && y >= 16 && y < 64;
            if (mask->contains(x, y) != expected) {
                QFAIL(qPrintable(QStringLiteral("coverage mismatch at %1,%2").arg(x).arg(y)));
            }
        }
    }
    QVERIFY(image->setGpuSelectionMask(mask));
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QCOMPARE(owner->operations().undoCount(), 1);

    // A curved closed path flattens to a single polygon instead of being
    // dropped or reduced to its endpoints.
    QPainterPath curved;
    curved.moveTo(32, 96);
    curved.cubicTo(32, 32, 96, 32, 96, 96);
    curved.closeSubpath();
    const QList<QPointF> curvedVertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(curved);
    QVERIFY(curvedVertices.size() > 4);
    QVERIFY(KisSelectionToolHelper::gpuSelectionMaskFromPolygon(curvedVertices, QSize(128, 128)));

    // Several contours cannot become one polygon, so the tool reports a
    // conversion failure instead of selecting only part of the path.
    QPainterPath contours;
    contours.addRect(QRectF(8, 8, 16, 16));
    contours.addRect(QRectF(48, 48, 16, 16));
    const QList<QPointF> contourVertices = KisSelectionToolHelper::gpuSelectionPolygonVertices(contours);
    QVERIFY(contourVertices.isEmpty());
    QVERIFY(!KisSelectionToolHelper::gpuSelectionMaskFromPolygon(contourVertices, QSize(128, 128)));
}

void KisGpuDocumentViewTest::polygonalToolCreatesArbitraryMaskSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Polygon tool")));
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
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectPolygonal"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectPolygonal"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QList<QPointF> vertices{{64, 64}, {192, 64}, {192, 192}, {64, 192}};

    QTest::mouseMove(native.data(), point(vertices.first()));
    const auto start = point(vertices.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    for (const QPointF &vertex : vertices) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
    }
    // The double-click ends the polyline without adding a new corner.
    QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(64, 64, 128, 128));
    QVERIFY(mask->contains(80, 80));
    QVERIFY(mask->contains(176, 176));
    QVERIFY(!mask->contains(48, 48));
    QVERIFY(!mask->contains(208, 208));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
    QTRY_VERIFY(lifetime->image()->globalSelection());
    QTRY_COMPARE(lifetime->image()->globalSelection()->selectedExactRect(), mask->bounds());
}

void KisGpuDocumentViewTest::selectionOutlineKeepsOnlyEffectiveCoverageBoundaries()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const QRect canvas(0, 0, 64, 64);
    const auto owner = paintImageWithEncoding(execution, canvas.size());
    auto image = KisImage::fromGpuImage(owner, QStringLiteral("Selection boundary"));
    const auto ellipse = Gpu::SelectionMask::fromEllipse({8, 8, 48, 48}, canvas);
    const auto hole = Gpu::SelectionMask::fromRectangle({24, 24, 12, 16}, canvas);
    const auto hard = ellipse->combined(*hole, Gpu::SelectionCombination::Subtract, canvas);
    QVERIFY(hard);
    for (const bool alpha : {false, true}) for (const bool inverted : {false, true}) {
        QByteArray raw(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            raw[y * 64 + x] = char(hard->contains(x, y) ? (alpha ? (x < 20 ? 96 : 200) : 255) : 0);
        const auto coverage = alpha ? Gpu::SelectionMask::fromAlphaCoverage(canvas, raw, canvas) : hard;
        QByteArray effective = raw;
        if (inverted) for (char &value : effective) value = char(255 - quint8(value));
        QVERIFY(image->setGpuSelectionMask(coverage, inverted));
        const auto synchronized = [&] {
            if (!image->globalSelection()) return false;
            QByteArray actual(raw.size(), '\0');
            image->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), canvas);
            return actual == effective;
        };
        QTRY_VERIFY(synchronized());
        const auto outline = image->globalSelection()->outlineCache();
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            QCOMPARE(outline.contains(QPointF(x + 0.5, y + 0.5)), quint8(effective[y * 64 + x]) >= 128);
        QPainterPathStroker stroker;
        stroker.setWidth(1.5);
        const auto border = stroker.createStroke(outline);
        QVERIFY(border.contains(QPointF(24, 32)));
        for (int y = 2; y < 62; ++y) for (int x = 2; x < 62; ++x) {
            bool interior = true;
            for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
                interior &= quint8(effective[(y + dy) * 64 + x + dx]) >= 128;
            const QByteArray context = QStringLiteral("alpha=%1 inverted=%2 interior=(%3,%4)")
                .arg(alpha).arg(inverted).arg(x).arg(y).toLatin1();
            if (interior) QVERIFY2(!border.contains(QPointF(x + 0.5, y)), context.constData());
        }
    }
}

void KisGpuDocumentViewTest::generatedSelectionViewRetainsAlphaAfterClose()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    const QRect canvas(0, 0, 64, 64), area(16, 16, 16, 16), softArea(8, 8, 32, 32);
    const auto owner = paintImageWithEncoding(execution, canvas.size());
    const auto layer = owner->operations().activeLayer();
    QCOMPARE(owner->setSelectionRect(area), Gpu::Result::Success);
    QCOMPARE(owner->fillLayerColor(layer, {197, 43, 17, 255}), Gpu::Result::Success);
    QCOMPARE(owner->clearSelection(), Gpu::Result::Success);
    Gpu::RegionFillSettings input;
    input.seed = {20, 20}; input.threshold = 1;
    QCOMPARE(owner->selectRegion(layer, input, Gpu::SelectionCombination::Replace), Gpu::Result::Success);
    const quint8 levels[]{0, 1, 127, 128, 254, 255};
    QByteArray soft(softArea.width() * softArea.height(), '\0');
    for (int y = 0; y < softArea.height(); ++y) for (int x = 0; x < softArea.width(); ++x)
        soft[y * softArea.width() + x] = char(levels[(x + y) % 6]);
    const auto literal = Gpu::SelectionMask::fromAlphaCoverage(softArea, soft, canvas);
    QVERIFY(literal);
    QCOMPARE(owner->combineSelectionMask(literal, Gpu::SelectionCombination::Intersect), Gpu::Result::Success);
    const auto mask = owner->selectionMask();
    QVERIFY(mask && mask->isGpuGenerated());
    KisPixelSelectionSP native = new KisPixelSelection();
    QByteArray expected(canvas.width() * canvas.height(), '\0');
    for (int y = area.top(); y <= area.bottom(); ++y) for (int x = area.left(); x <= area.right(); ++x)
        expected[y * canvas.width() + x] = char(literal->coverage(x, y));
    native->writeBytes(reinterpret_cast<const quint8 *>(expected.constData()), canvas);
    const auto image = KisImage::fromGpuImage(owner, QStringLiteral("Exact Alpha8 selection view"));
    QTRY_VERIFY(image->globalSelection());
    KisPixelSelectionSP pixels = image->globalSelection()->pixelSelection();
    const auto verify = [&](const KisPixelSelectionSP &value, const QByteArray &reference) {
        QByteArray bytes(reference.size(), '\0'); value->readBytes(reinterpret_cast<quint8 *>(bytes.data()), canvas);
        return bytes == reference;
    };
    QVERIFY(verify(pixels, expected));
    QCOMPARE(pixels->selectedExactRect(), native->selectedExactRect());
    QCOMPARE(pixels->pixel({18, 17}).opacityU8(), quint8(1));
    QCOMPARE(pixels->pixel({17, 17}).opacityU8(), quint8(0));
    QVERIFY(!pixels->outlineCache().contains(QPointF(18.5, 17.5)));
    QVERIFY(pixels->outlineCache().contains(QPointF(16.5, 16.5)));
    KisPixelSelectionSP copied = new KisPixelSelection(*pixels);
    QVERIFY(verify(copied, expected));
    pixels->recalculateThumbnailImage(Qt::red); native->recalculateThumbnailImage(Qt::red);
    QCOMPARE(pixels->thumbnailImage(), native->thumbnailImage());
    const auto history = owner->operations().undoCount();
    QCOMPARE(owner->setSelectionMask(mask, true), Gpu::Result::Success);
    QTRY_VERIFY(image->globalSelection());
    // The view object can be reused by synchronization; the independent copy
    // keeps the earlier immutable mask while current selectedness is inverted.
    QByteArray inverse = expected;
    for (char &value : inverse) value = char(255 - quint8(value));
    QTRY_VERIFY(verify(image->globalSelection()->pixelSelection(), inverse));
    const auto invertedPixels = image->globalSelection()->pixelSelection();
    QVERIFY(verify(invertedPixels, inverse));
    QCOMPARE(invertedPixels->selectedExactRect(), canvas);
    QCOMPARE(owner->operations().undoCount(), history + 1);
    QVERIFY(verify(copied, expected));
    owner->close();
    QVERIFY(verify(copied, expected));
    QVERIFY(verify(invertedPixels, inverse));
    const auto replacement = Gpu::Device::createNative(diagnostic);
    QVERIFY2(replacement, qPrintable(diagnostic));
    QCOMPARE(execution->replaceDevice(replacement).code, Gpu::ExecutionCode::Success);
    QVERIFY(verify(copied, expected));
    QVERIFY(verify(invertedPixels, inverse));
}

void KisGpuDocumentViewTest::colourSelectionToolsKeepPixelsHistoryAndKra_data()
{
    QTest::addColumn<QString>("toolId");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("action");
    QTest::addColumn<QString>("scenario");
    for (const auto &tool : {QStringLiteral("KisToolSelectContiguous"), QStringLiteral("KisToolSelectSimilar")})
        for (const bool words : {false, true}) {
            for (const auto action : {SELECTION_REPLACE, SELECTION_ADD, SELECTION_SUBTRACT, SELECTION_INTERSECT, SELECTION_SYMMETRICDIFFERENCE}) {
                const QByteArray name = tool.toLatin1() + (words ? "-word-" : "-byte-") + QByteArray::number(int(action));
                QTest::newRow(name.constData()) << tool << words << int(action) << QStringLiteral("lab");
            }
            for (const auto &scenario : {QStringLiteral("hard"), QStringLiteral("all"), QStringLiteral("label"),
                                         QStringLiteral("masked"), QStringLiteral("shrink")}) {
                const QByteArray name = tool.toLatin1() + (words ? "-word-" : "-byte-") + scenario.toLatin1();
                QTest::newRow(name.constData()) << tool << words << int(SELECTION_REPLACE) << scenario;
            }
            const QByteArray gradient = tool.toLatin1() + (words ? "-word-spread-gradient" : "-byte-spread-gradient");
            QTest::newRow(gradient.constData()) << tool << words << int(SELECTION_ADD) << QStringLiteral("spread-gradient");
            for (const auto &scenario : {QStringLiteral("move"), QStringLiteral("move-clipped"), QStringLiteral("move-inverted")}) {
                const QByteArray name = tool.toLatin1() + (words ? "-word-" : "-byte-") + scenario.toLatin1();
                QTest::newRow(name.constData()) << tool << words << int(SELECTION_REPLACE) << scenario;
            }
        }
}

void KisGpuDocumentViewTest::colourSelectionToolsKeepPixelsHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, toolId);
    QFETCH(bool, words);
    QFETCH(int, action);
    QFETCH(QString, scenario);
    const bool contiguous = toolId == QLatin1String("KisToolSelectContiguous");
    const auto selectionAction = SelectionAction(action);
    const bool aa = scenario != QLatin1String("hard");
    const int threshold = aa ? 32 : 1, spread = aa ? 45 : 100;
    const int grow = scenario == QLatin1String("shrink") ? -2 : aa ? 3 : 0;
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("threshold", threshold); configuration.writeEntry("opacitySpread", spread);
    configuration.writeEntry("closeGapAmount", 0); configuration.writeEntry("useSelectionAsBoundary", false);
    configuration.writeEntry("contiguousSelectionMode", "floodFill");
    configuration.writeEntry("stopGrowingAtDarkestPixel", false); configuration.writeEntry("featherSelection", 0);
    const QRect region(0, 0, 256, 256);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int stride = Gpu::pixelByteCount(format);
    const QPoint seed(64, 64);
    KisImageSP nativeImage = new KisImage(new KisSurrogateUndoStore(), 256, 256, cs, QStringLiteral("Native colour selection"));
    KisPaintDeviceSP source = new KisPaintDevice(cs);
    const auto maximum = Gpu::maximumComponent(format);
    QByteArray sourceBytes(region.width() * region.height() * stride, '\0');
    for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
        const bool inside = ((x - 72) * (x - 72) + (y - 76) * (y - 76) < 38 * 38) ||
                            QRect(160, 40, 58, 49).contains(x, y);
        const auto pixel = Gpu::rgbaBytes(inside ? Gpu::RgbaPixel{quint16(maximum * (scenario == QLatin1String("spread-gradient") ? 96 + x % 129 : 140 + x % 21) / 255u),
            quint16(maximum * (scenario == QLatin1String("spread-gradient") ? 30 + y % 97 : 43 + y % 17) / 255u), quint16(maximum * 29u / 255u), maximum} : Gpu::RgbaPixel{0, 0, 0, maximum}, format);
        sourceBytes.replace((y * region.width() + x) * stride, stride, pixel);
    }
    auto bgra = sourceBytes; exchangeRgbaBgra(bgra, stride);
    source->writeBytes(reinterpret_cast<const quint8 *>(bgra.constData()), region);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = region.size(); raster.format = format;
    raster.layers = {{QStringLiteral("Colour source"), true, QUuid::createUuid(), {}},
                     {QStringLiteral("Selected drawing"), true, QUuid::createUuid(), {}}};
    raster.layers[0].colorLabelIndex = 1;
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->writeRgbaPixels(1, region, sourceBytes), Gpu::Result::Success);
    if (scenario == QLatin1String("masked")) {
        const QRect maskRect(32, 32, 57, 72);
        QCOMPARE(owner->setSelectionRect(maskRect), Gpu::Result::Success);
        QCOMPARE(owner->addTransparencyMaskFromSelection(1), Gpu::Result::Success);
        KisSelectionSP mask = new KisSelection(); mask->pixelSelection()->select(maskRect);
        KisPaintDeviceSP projected = new KisPaintDevice(cs);
        KisPainter(projected, mask).bitBlt(region.topLeft(), source, region);
        source = projected;
    }
    QByteArray initial(region.width() * region.height(), '\0');
    const bool inverted = selectionAction == SELECTION_INTERSECT;
    if (selectionAction == SELECTION_REPLACE) {
        for (int y = 8; y < 16; ++y) for (int x = 8; x < 16; ++x) initial[y * 256 + x] = char(255);
    } else {
        for (int y = 24; y < 160; ++y) for (int x = 24; x < 192; ++x)
            initial[y * 256 + x] = char(32 + ((x + 3 * y) % 7) * 31);
    }
    const auto previousMask = Gpu::SelectionMask::fromAlphaCoverage(region, initial, region);
    QVERIFY(previousMask);
    QCOMPARE(owner->setSelectionMask(previousMask, inverted), Gpu::Result::Success);
    if (inverted) for (char &value : initial) value = char(255 - quint8(value));
    const auto before = owner->operations().snapshot();
    const int history = owner->operations().undoCount();
    KisSelectionSP expectedSelection = new KisSelection();
    expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), region);
    KisPixelSelectionSP added = new KisPixelSelection(new KisDefaultBounds(nativeImage));
    KisFillPainter classifier;
    classifier.setWidth(256); classifier.setHeight(256);
    classifier.setFillThreshold(threshold); classifier.setOpacitySpread(spread);
    if (contiguous) {
        classifier.setAntiAlias(aa); classifier.setSizemod(grow); classifier.setUseCompositing(true);
        classifier.createFloodSelection(added, seed.x(), seed.y(), source, KisPixelSelectionSP());
    } else {
        classifier.createSimilarColorsSelection(added, source->pixel(seed), source, region, KisPixelSelectionSP());
        if (grow > 0) {
            KisGrowSelectionFilter filter(grow, grow);
            filter.process(added, added->selectedRect().adjusted(-grow, -grow, grow, grow));
        } else if (grow < 0) {
            KisShrinkSelectionFilter filter(-grow, -grow, false); filter.process(added, added->selectedRect());
        }
        if (aa) { KisAntiAliasSelectionFilter filter; filter.process(added, added->selectedRect()); }
    }
    expectedSelection->pixelSelection()->applySelection(added, selectionAction);
    auto checkedCoverage = region.united(expectedSelection->pixelSelection()->selectedExactRect());
    QByteArray expectedMask(checkedCoverage.width() * checkedCoverage.height(), '\0');
    expectedSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedMask.data()), checkedCoverage);
    const bool empty = expectedSelection->pixelSelection()->selectedExactRect().isEmpty();
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Colour selection workflow"), cs));
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
    window->resize(800, 600); window->show(); window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
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
    KoToolManager::instance()->switchToolRequested(toolId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == toolId + QLatin1String("option widget"))
            if (auto *candidate = qobject_cast<KisSelectionOptions *>(widget)) options = candidate;
    QVERIFY(options);
    options->setMode(PIXEL_SELECTION); options->setAction(selectionAction);
    options->setAntiAliasSelection(aa); options->setGrowSelection(grow); options->setFeatherSelection(0);
    options->setStopGrowingAtDarkestPixel(false);
    options->setReferenceLayers(scenario == QLatin1String("all") ? KisSelectionOptions::AllLayers :
        scenario == QLatin1String("label") ? KisSelectionOptions::ColorLabeledLayers : KisSelectionOptions::CurrentLayer);
    options->setSelectedColorLabels({1});
    auto *thresholdSlider = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderThreshold"));
    auto *spreadSlider = options->findWidgetAs<KisSliderSpinBox *>(QStringLiteral("sectionSelectionExtent/sliderSpread"));
    QVERIFY(thresholdSlider && spreadSlider);
    thresholdSlider->setValue(threshold); spreadSlider->setValue(spread);
    native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
    const auto point = converter->imageToWidget(seed).toPoint();
    QTest::mouseMove(native.data(), point);
    QEnterEvent entered(point, point, native->mapToGlobal(point)); QApplication::sendEvent(native.data(), &entered);
    QTest::mouseClick(native.data(), Qt::LeftButton, Qt::NoModifier, point);
    int selectionHistory = history + 1;
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
    auto image = KisImageSP(lifetime->image());
    QString mismatch;
    const auto checkSelection = [&](const KisImageSP &candidate) {
        if (empty) return !candidate->globalSelection();
        const auto selection = candidate->globalSelection();
        if (!selection) return false;
        QByteArray values(expectedMask.size(), '\0');
        selection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(values.data()), checkedCoverage);
        if (values != expectedMask) {
            qsizetype index = 0; while (index < values.size() && values[index] == expectedMask[index]) ++index;
            mismatch = QStringLiteral("colour selectedness %1,%2: actual%3 native%4")
                .arg(checkedCoverage.x() + index % checkedCoverage.width()).arg(checkedCoverage.y() + index / checkedCoverage.width())
                .arg(quint8(values[index])).arg(quint8(expectedMask[index]));
        }
        return values == expectedMask;
    };
    QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
    QVERIFY(empty || owner->selectionMask()->isGpuGenerated());
    if (!empty) {
        auto *pixels = image->globalSelection()->pixelSelection().data();
        QCOMPARE(pixels->colorSpace(), KoColorSpaceRegistry::instance()->alpha8());
        QCOMPARE(pixels->selectedExactRect(), expectedSelection->pixelSelection()->selectedExactRect());
        KisPixelSelectionSP retained = new KisPixelSelection(*pixels);
        QByteArray copied(expectedMask.size(), '\0'); retained->readBytes(reinterpret_cast<quint8 *>(copied.data()), checkedCoverage);
        QCOMPARE(copied, expectedMask);
        pixels->recalculateThumbnailImage(QColor(12, 94, 177, 128));
        QVERIFY(pixels->thumbnailImageValid());
        expectedSelection->pixelSelection()->recalculateThumbnailImage(QColor(12, 94, 177, 128));
        QCOMPARE(pixels->thumbnailImage(), expectedSelection->pixelSelection()->thumbnailImage());
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success); QCOMPARE(owner->operations().snapshot(), before);
    QCOMPARE(owner->redo(), Gpu::Result::Success); QTRY_VERIFY(checkSelection(image));
    if (scenario.startsWith(QLatin1String("move"))) {
        const bool inverseMove = scenario == QLatin1String("move-inverted");
        const auto adoptExpected = [&](const QByteArray &values) {
            expectedSelection = new KisSelection();
            expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), region);
            checkedCoverage = region;
            expectedMask = values;
        };
        if (inverseMove) {
            QVERIFY(image->invertGpuSelectionMask());
            ++selectionHistory;
            auto values = expectedMask;
            for (char &value : values) value = char(255 - quint8(value));
            adoptExpected(values);
            QTRY_VERIFY(checkSelection(image));
        }
        const auto expectedMoved = [&](QPoint offset) {
            QByteArray values(region.width() * region.height(), inverseMove ? char(255) : char(0));
            for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x) {
                const QPoint source(x - offset.x(), y - offset.y());
                if (checkedCoverage.contains(source))
                    values[y * region.width() + x] = expectedMask[(source.y() - checkedCoverage.y()) * checkedCoverage.width() + source.x() - checkedCoverage.x()];
            }
            return values;
        };
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectRectangular"));
        QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectRectangular"));
        native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
        const QPoint start = inverseMove ? QPoint(20, 20) : seed;
        const QPoint offset = scenario == QLatin1String("move-clipped") ? QPoint(-50, -39) : QPoint(13, 9);
        const auto sourcePoint = converter->imageToWidget(start).toPoint();
        const auto targetPoint = converter->imageToWidget(start + offset).toPoint();
        const auto moveStart = owner->operations().snapshot();
        const auto originalExpected = expectedMask;
        const auto movedExpected = expectedMoved(offset);
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        adoptExpected(movedExpected);
        QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->operations().undoCount(), selectionHistory);
        QTest::keyClick(view->canvasBase()->canvasWidget(), Qt::Key_Escape);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        adoptExpected(originalExpected);
        QTRY_VERIFY(checkSelection(image));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        adoptExpected(movedExpected);
        QTRY_VERIFY(checkSelection(image));
        auto *selectionTool = dynamic_cast<KisTool *>(KoToolManager::instance()->toolById(
            view->canvasBase(), QStringLiteral("KisToolSelectRectangular")));
        QVERIFY(selectionTool);
        selectionTool->deactivatePrimaryAction();
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        adoptExpected(originalExpected);
        QTRY_VERIFY(checkSelection(image));
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->operations().undoCount(), selectionHistory);
        selectionTool->activatePrimaryAction();
        QTest::mouseMove(native.data(), sourcePoint);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, sourcePoint);
        QTest::mouseMove(native.data(), targetPoint, 30);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, targetPoint);
        ++selectionHistory;
        QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
        adoptExpected(movedExpected);
        QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), moveStart);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QTRY_VERIFY(checkSelection(image));
        for (const auto &step : {QPair<QPoint, Qt::KeyboardModifiers>{{1, 0}, Qt::NoModifier},
                                QPair<QPoint, Qt::KeyboardModifiers>{{0, 10}, Qt::ShiftModifier}}) {
            const auto nudgedExpected = expectedMoved(step.first);
            QTest::keyClick(view->canvasBase()->canvasWidget(), step.first.x() ? Qt::Key_Right : Qt::Key_Down, step.second);
            ++selectionHistory;
            QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
            adoptExpected(nudgedExpected);
            QTRY_VERIFY2(checkSelection(image), qPrintable(mismatch));
        }
        QCOMPARE(owner->selectionInverted(), inverseMove);
    }
    QCOMPARE(owner->selectLayer(2), Gpu::Result::Success);
    QTRY_VERIFY(view->currentNode() && view->currentNode()->uuid() == raster.layers[1].uuid);
    const auto combined = owner->operations().snapshot();
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setOpacity(1); resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory + 1);
    KisPaintDeviceSP expectedPixels = new KisPaintDevice(cs);
    const auto nativeFill = [&](const KoColor &color) {
        KisPaintDeviceSP source = new KisPaintDevice(cs);
        KisFillPainter(source).fillRect(region, color);
        KisPainter painter(expectedPixels, empty ? KisSelectionSP() : expectedSelection);
        painter.bitBlt(region.topLeft(), source, region);
    };
    const auto nativeBytes = [&] {
        QByteArray bytes(region.width() * region.height() * stride, '\0');
        expectedPixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    nativeFill(resources->fgColor());
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), region, region.width() * region.height() * stride);
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    QCOMPARE(actual.bytes, nativeBytes());
    QCOMPARE(combined->selectionMask, owner->operations().snapshot()->selectionMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), combined);
    QCOMPARE(owner->redo(), Gpu::Result::Success);

    QTemporaryDir directory;
    QVERIFY(directory.isValid()); lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("combined-selection.kra"));
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    auto reopenedImage = KisImage::fromGpuImage(reopened, QStringLiteral("Reopened combined selection"), cs);
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    lifetime->setCurrentImage(reopenedImage);
    QTRY_COMPARE(view->image().data(), reopenedImage.data());
    QTRY_COMPARE(window->viewManager()->image().data(), reopenedImage.data());
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == reopenedImage.data());
    QTRY_VERIFY(checkSelection(reopenedImage));
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    resources->setFGColor(KoColor(QColor(221, 53, 19), cs));
    QTRY_VERIFY(fill->isEnabled());
    fill->trigger();
    QTRY_COMPARE(reopened->operations().undoCount(), 1);
    nativeFill(resources->fgColor());
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    const auto editedPath = directory.filePath(QStringLiteral("reedited-selection.kra"));
    QVERIFY2(lifetime->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile editedFile(editedPath); QVERIFY(editedFile.open(QIODevice::ReadOnly));
    const auto edited = LibrePaint::ImportExport::Kra::RasterCodec::read(editedFile);
    QVERIFY2(edited.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(edited.diagnostic));
    const auto editedOwner = paintImageWithEncoding(execution, edited.document.image);
    QCOMPARE(editedOwner->readRgbaPixels(editedOwner->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
}


void KisGpuDocumentViewTest::shapeSelectionCombinationsKeepEditingHistoryAndKra_data()
{
    QTest::addColumn<QString>("toolId");
    QTest::addColumn<int>("action");
    QTest::addColumn<bool>("words");
    QTest::addColumn<QString>("initialState");
    QTest::addColumn<QString>("shapeOptions");
    const QStringList tools{QStringLiteral("KisToolSelectRectangular"), QStringLiteral("KisToolSelectElliptical"),
                           QStringLiteral("KisToolSelectPolygonal"), QStringLiteral("KisToolSelectOutline"),
                           QStringLiteral("KisToolSelectPath")};
    for (const bool words : {false, true}) {
        for (const auto &tool : tools) {
            for (const auto action : {SELECTION_ADD, SELECTION_SUBTRACT, SELECTION_INTERSECT, SELECTION_SYMMETRICDIFFERENCE}) {
                const QByteArray name = (words ? "word-" : "byte-") + tool.toLatin1() + '-' + QByteArray::number(action);
                QTest::newRow(name.constData()) << tool << int(action) << words << QStringLiteral("soft") << QString();
            }
        }
    }
    for (const auto &state : {QStringLiteral("inverted"), QStringLiteral("none"), QStringLiteral("disjoint")}) {
        for (const auto action : {SELECTION_ADD, SELECTION_SUBTRACT, SELECTION_INTERSECT, SELECTION_SYMMETRICDIFFERENCE}) {
            const QByteArray name = state.toLatin1() + '-' + QByteArray::number(action);
            QTest::newRow(name.constData()) << tools.first() << int(action) << false << state << QString();
        }
    }
    // Ordinary option widgets produce frozen input coverage. The native
    // rasterizer/filter oracle protects its use by fills, history and KRA.
    const QStringList settings{QStringLiteral("aa"), QStringLiteral("grow"), QStringLiteral("shrink"),
        QStringLiteral("feather"), QStringLiteral("grow-feather"), QStringLiteral("shrink-feather"),
        QStringLiteral("feather-no-aa"), QStringLiteral("shrink-empty")};
    const QList<int> actions{SELECTION_REPLACE, SELECTION_ADD, SELECTION_SUBTRACT, SELECTION_INTERSECT,
        SELECTION_SYMMETRICDIFFERENCE, SELECTION_REPLACE, SELECTION_ADD, SELECTION_REPLACE};
    for (const bool words : {false, true}) for (const auto &tool : tools) {
        for (int i = 0; i < settings.size(); ++i) {
            const QByteArray name = (words ? "word-" : "byte-") + tool.toLatin1() + '-' + settings[i].toLatin1();
            QTest::newRow(name.constData()) << tool << actions[i] << words
                << (actions[i] == SELECTION_REPLACE ? QStringLiteral("disjoint") : QStringLiteral("soft")) << settings[i];
        }
    }
    for (const bool words : {false, true}) {
        for (const auto &tool : {tools[0], tools[1]}) {
            for (const auto &setting : {QStringLiteral("rotate"), QStringLiteral("rotate-feather"),
                                        QStringLiteral("rounded"), QStringLiteral("rounded-feather")}) {
                if (tool == tools[1] && setting.startsWith(QLatin1String("rounded"))) continue;
                const QByteArray name = (words ? "word-" : "byte-") + tool.toLatin1() + '-' + setting.toLatin1();
                QTest::newRow(name.constData()) << tool << int(SELECTION_REPLACE) << words << QStringLiteral("disjoint") << setting;
            }
        }
    }
}

void KisGpuDocumentViewTest::shapeSelectionCombinationsKeepEditingHistoryAndKra()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(QString, toolId);
    QFETCH(int, action);
    QFETCH(bool, words);
    QFETCH(QString, initialState);
    QFETCH(QString, shapeOptions);
    KConfigGroup configuration = KSharedConfig::openConfig()->group(toolId);
    const auto oldEntries = configuration.entryMap();
    const auto restoreConfiguration = qScopeGuard([&] {
        configuration.deleteGroup();
        for (auto i = oldEntries.cbegin(); i != oldEntries.cend(); ++i) configuration.writeEntry(i.key(), i.value());
    });
    configuration.writeEntry("roundCornersX", 0);
    configuration.writeEntry("roundCornersY", 0);
    const auto selectionAction = SelectionAction(action);
    const QRect region(0, 0, 256, 256);
    const QRect shape(64, 64, 128, 128);
    const QList<QPointF> vertices = shapeOptions.isEmpty() ? QList<QPointF>{{64, 64}, {192, 64}, {192, 192}, {64, 192}}
        : QList<QPointF>{{64, 64}, {192, 71}, {175, 192}, {69, 178}};
    const bool aa = !shapeOptions.isEmpty() && shapeOptions != QLatin1String("feather-no-aa");
    const int grow = shapeOptions.startsWith(QLatin1String("grow")) ? 7 :
        shapeOptions == QLatin1String("shrink-empty") ? -100 : shapeOptions.startsWith(QLatin1String("shrink")) ? -5 : 0;
    const int feather = shapeOptions.contains(QLatin1String("feather")) ? 5 : 0;
    const bool rounded = shapeOptions.startsWith(QLatin1String("rounded"));
    const bool rotated = shapeOptions.startsWith(QLatin1String("rotate"));
    const auto incoming = toolId == QLatin1String("KisToolSelectElliptical")
        ? Gpu::SelectionMask::fromEllipse(shape, region) : Gpu::SelectionMask::fromRectangle(shape, region);
    QVERIFY(incoming);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto stride = Gpu::pixelByteCount(format);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = region.size(); raster.format = format;
    raster.layers.append({QStringLiteral("Selection edit"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    QByteArray initial(region.width() * region.height(), '\0');
    if (initialState != QLatin1String("none")) {
        for (int y = 32; y < 176; ++y) for (int x = 32; x < 160; ++x)
            initial[y * region.width() + x] = char(32 + ((x + 3 * y) % 8) * 31);
        if (initialState == QLatin1String("disjoint")) {
            initial.fill('\0');
            for (int y = 8; y < 24; ++y) for (int x = 8; x < 24; ++x) initial[y * region.width() + x] = char(255);
        }
        const auto mask = Gpu::SelectionMask::fromAlphaCoverage(region, initial, region);
        QVERIFY(mask);
        QCOMPARE(owner->setSelectionMask(mask, initialState == QLatin1String("inverted")), Gpu::Result::Success);
        if (initialState == QLatin1String("inverted"))
            for (char &value : initial) value = char(255 - quint8(value));
    }
    const auto before = owner->operations().snapshot();
    const int history = owner->operations().undoCount();
    QCOMPARE(owner->markSaved(before), Gpu::Result::Success);
    KisSelectionSP expectedSelection = new KisSelection();
    expectedSelection->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(initial.constData()), region);
    if (initialState == QLatin1String("none") && selectionAction == SELECTION_SUBTRACT)
        expectedSelection->pixelSelection()->select(region);
    KisImageSP nativeImage = new KisImage(new KisSurrogateUndoStore(), region.width(), region.height(), cs,
                                        QStringLiteral("Native geometric selection"));
    KisPixelSelectionSP added = new KisPixelSelection(new KisDefaultBounds(nativeImage));
    if (shapeOptions.isEmpty()) {
        QByteArray addedBytes(initial.size(), '\0');
        for (int y = 0; y < region.height(); ++y) for (int x = 0; x < region.width(); ++x)
            addedBytes[y * region.width() + x] = char(incoming->coverage(x, y));
        added->writeBytes(reinterpret_cast<const quint8 *>(addedBytes.constData()), region);
    } else {
        QRectF inputRect(shape);
        if (rotated) {
            // Ctrl+Alt rotates the fixed-size rectangle about its drag start.
            inputRect.moveCenter(QPointF(64 + std::sqrt(2.) * 64, 64));
        }
        QPainterPath path;
        if (rounded) path.addRoundedRect(inputRect.toRect(), 18, 18);
        else if (toolId == QLatin1String("KisToolSelectRectangular")) path.addRect(inputRect.toRect());
        else if (toolId == QLatin1String("KisToolSelectElliptical")) path.addEllipse(inputRect);
        else { path.addPolygon(QPolygonF(vertices)); path.closeSubpath(); }
        if (rotated) {
            const QPointF center = toolId == QLatin1String("KisToolSelectRectangular") ? QPointF(inputRect.toRect().center()) : inputRect.center();
            QTransform transform;
            transform.translate(center.x(), center.y());
            transform.rotateRadians(-std::atan2(128., 128.));
            transform.translate(-center.x(), -center.y());
            path = transform.map(path);
        }
        KisPainter painter(added);
        painter.setPaintColor(KoColor(Qt::black, added->colorSpace()));
        painter.setAntiAliasPolygonFill(aa && feather == 0);
        painter.setFillStyle(KisPainter::FillStyleForegroundColor);
        painter.setStrokeStyle(KisPainter::StrokeStyleNone);
        painter.fillPainterPath(path);
        painter.end();
        if (grow > 0) {
            KisGrowSelectionFilter filter(grow, grow);
            filter.process(added, added->selectedRect().adjusted(-grow, -grow, grow, grow));
        } else if (grow < 0) {
            KisShrinkSelectionFilter filter(-grow, -grow, false);
            filter.process(added, added->selectedRect());
        }
        if (feather) {
            KisFeatherSelectionFilter filter(feather);
            filter.process(added, added->selectedRect().adjusted(-feather, -feather, feather, feather));
        }
    }
    expectedSelection->pixelSelection()->applySelection(added, selectionAction);
    const QRect checkedCoverage = region.united(expectedSelection->pixelSelection()->selectedExactRect());
    QByteArray expectedMask(checkedCoverage.width() * checkedCoverage.height(), '\0');
    expectedSelection->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(expectedMask.data()), checkedCoverage);
    const bool empty = expectedSelection->pixelSelection()->selectedExactRect().isEmpty();

    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Shape selection combinations"), cs));
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
    window->resize(800, 600); window->show(); window->activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
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
    KoToolManager::instance()->switchToolRequested(toolId);
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), toolId);
    QPointer<KisSelectionOptions> options;
    for (auto *widget : QApplication::allWidgets())
        if (widget->objectName() == toolId + QLatin1String("option widget"))
            if (auto *selectionOptions = qobject_cast<KisSelectionOptions *>(widget)) options = selectionOptions;
    QVERIFY(options);
    const auto oldAction = options->action();
    const auto oldMode = options->mode();
    const bool oldAA = options->antiAliasSelection();
    const int oldGrow = options->growSelection(), oldFeather = options->featherSelection();
    const auto restoreOptions = qScopeGuard([&] {
        if (!options) return;
        options->setAction(oldAction); options->setMode(oldMode); options->setAntiAliasSelection(oldAA);
        options->setGrowSelection(oldGrow); options->setFeatherSelection(oldFeather);
    });
    options->setMode(PIXEL_SELECTION); options->setAction(selectionAction);
    options->setAntiAliasSelection(aa); options->setGrowSelection(grow); options->setFeatherSelection(feather);
    QList<QPair<QPointer<QSpinBox>, int>> oldDimensions;
    QList<QPair<QPointer<QPushButton>, bool>> oldLocks;
    const auto restoreDimensions = qScopeGuard([&] {
        for (const auto &entry : oldDimensions) if (entry.first) entry.first->setValue(entry.second);
        for (const auto &entry : oldLocks) if (entry.first) entry.first->setChecked(entry.second);
    });
    if (rounded || rotated) {
        for (const auto &name : {QStringLiteral("lockWidthButton"), QStringLiteral("lockHeightButton"), QStringLiteral("lockRatioButton")}) {
            auto *button = options->findChild<QPushButton *>(name);
            QVERIFY(button);
            oldLocks.append({button, button->isChecked()});
            button->setChecked(false);
        }
        const QStringList names = rounded ? QStringList{QStringLiteral("intRoundCornersX"), QStringLiteral("intRoundCornersY")}
                                         : QStringList{QStringLiteral("intWidth"), QStringLiteral("intHeight")};
        for (const auto &name : names) {
            auto *spin = options->findChild<QSpinBox *>(name);
            QVERIFY(spin);
            oldDimensions.append({spin, spin->value()});
        }
        for (const auto &entry : oldDimensions) entry.first->setValue(rounded ? 18 : 128);
        if (rotated) {
            options->findChild<QPushButton *>(QStringLiteral("lockWidthButton"))->setChecked(true);
            options->findChild<QPushButton *>(QStringLiteral("lockHeightButton"))->setChecked(true);
        }
    }
    if (!words && toolId == QLatin1String("KisToolSelectRectangular") && shapeOptions == QLatin1String("aa")) {
        KisSelectionToolHelper helper(view->canvasBase(), kundo2_i18n("Select Rectangle"));
        QPainterPath oversized;
        oversized.addRect(QRectF(0, 0, 100000, 100000));
        QVERIFY(!helper.selectGpuSelectionPath(oversized, true, 0, 0, SELECTION_REPLACE));
        QPainterPath wide;
        wide.addRect(QRectF(0, 0, 200000, 1));
        QVERIFY(!helper.selectGpuSelectionPath(wide, true, -400, 0, SELECTION_REPLACE));
        QPainterPath valid;
        valid.addRect(shape);
        QVERIFY(!helper.selectGpuSelectionPath(valid, true, 401, 0, SELECTION_REPLACE));
        QVERIFY(!helper.selectGpuSelectionPath(valid, true, 0, 401, SELECTION_REPLACE));
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->operations().undoCount(), history);
    }
    native->requestActivate(); view->canvasBase()->canvasWidget()->setFocus();
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const auto start = point(vertices.first());
    QTest::mouseMove(native.data(), start);
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    if (toolId == QLatin1String("KisToolSelectRectangular") || toolId == QLatin1String("KisToolSelectElliptical") ||
        toolId == QLatin1String("KisToolSelectPolygonal")) {
        // The held-pointer Escape must preserve the existing selection and
        // leave the next gesture independent, including its saved pixels.
        auto selectionImage = KisImageSP(lifetime->image());
        QByteArray displayedBefore(region.width() * region.height(), '\0');
        if (selectionImage->globalSelection()) selectionImage->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(displayedBefore.data()), region);
        const auto cancelledStart = point({16, 208});
        const auto cancelledEnd = point({48, 240});
        QTest::mouseMove(native.data(), cancelledStart);
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, cancelledStart);
        QTest::mouseMove(native.data(), cancelledEnd, 20);
        QTest::keyClick(native.data(), Qt::Key_Escape);
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, cancelledEnd);
        selectionImage->waitForDone();
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->operations().undoCount(), history);
        QByteArray displayedAfter(displayedBefore.size(), '\0');
        if (selectionImage->globalSelection()) selectionImage->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(displayedAfter.data()), region);
        QCOMPARE(displayedAfter, displayedBefore);
        QTest::mouseMove(native.data(), start);
    }
    if (toolId == QLatin1String("KisToolSelectPolygonal") || toolId == QLatin1String("KisToolSelectPath")) {
        for (const auto &vertex : vertices) QTest::mouseClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    } else {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, start);
        if (toolId == QLatin1String("KisToolSelectOutline")) {
            for (qsizetype i = 1; i < vertices.size(); ++i) QTest::mouseMove(native.data(), point(vertices[i]), 20);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
        } else if (rotated) {
            QTest::keyPress(native.data(), Qt::Key_Control);
            QTest::keyPress(native.data(), Qt::Key_Alt);
            QTest::mouseMove(native.data(), point({192, 64}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::ControlModifier | Qt::AltModifier, point({192, 64}));
            QTest::keyRelease(native.data(), Qt::Key_Alt);
            QTest::keyRelease(native.data(), Qt::Key_Control);
        } else {
            QTest::mouseMove(native.data(), point({192, 192}), 30);
            QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point({192, 192}));
        }
    }
    const int selectionHistory = history + 1;
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory);
    auto image = KisImageSP(lifetime->image());
    QString selectionMismatch;
    const auto checkSelection = [&](const KisImageSP &candidate) {
        if (empty) return !candidate->globalSelection();
        if (!candidate->globalSelection()) return false;
        QByteArray actual(expectedMask.size(), '\0');
        candidate->globalSelection()->pixelSelection()->readBytes(reinterpret_cast<quint8 *>(actual.data()), checkedCoverage);
        if (actual != expectedMask) {
            qsizetype index = 0;
            while (index < actual.size() && actual[index] == expectedMask[index]) ++index;
            selectionMismatch = QStringLiteral("coverage at %1,%2: actual%3 expected%4; bounds%5,%6 %7x%8")
                .arg(checkedCoverage.x() + index % checkedCoverage.width()).arg(checkedCoverage.y() + index / checkedCoverage.width())
                .arg(quint8(actual[index])).arg(quint8(expectedMask[index]))
                .arg(owner->selectionRect().x()).arg(owner->selectionRect().y()).arg(owner->selectionRect().width()).arg(owner->selectionRect().height());
        }
        return actual == expectedMask;
    };
    QTRY_VERIFY2(checkSelection(image), qPrintable(selectionMismatch));
    const auto combined = owner->operations().snapshot();
    if (selectionHistory > history) {
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->operations().snapshot(), before);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QTRY_VERIFY(checkSelection(image));
    }

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setOpacity(1); resources->setEraserMode(false); resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    auto *fill = window->viewManager()->actionManager()->actionByName(QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill); QTRY_VERIFY(fill->isEnabled()); fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), selectionHistory + 1);
    KisPaintDeviceSP expectedPixels = new KisPaintDevice(cs);
    const auto nativeFill = [&](const KoColor &color) {
        KisPaintDeviceSP source = new KisPaintDevice(cs);
        KisFillPainter(source).fillRect(region, color);
        KisPainter painter(expectedPixels, empty ? KisSelectionSP() : expectedSelection);
        painter.bitBlt(region.topLeft(), source, region);
    };
    const auto nativeBytes = [&] {
        QByteArray bytes(region.width() * region.height() * stride, '\0');
        expectedPixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), region);
        exchangeRgbaBgra(bytes, stride);
        return bytes;
    };
    nativeFill(resources->fgColor());
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), region, region.width() * region.height() * stride);
    QCOMPARE(actual.code, Gpu::ExecutionCode::Success);
    QCOMPARE(actual.bytes, nativeBytes());
    QCOMPARE(combined->selectionMask, owner->operations().snapshot()->selectionMask);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().snapshot(), combined);
    QCOMPARE(owner->redo(), Gpu::Result::Success);

    QTemporaryDir directory;
    QVERIFY(directory.isValid()); lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("combined-selection.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    auto reopenedImage = KisImage::fromGpuImage(reopened, QStringLiteral("Reopened combined selection"), cs);
    const bool automaticDelivery = KisSynchronizedConnectionBase::isAutoModeForUnittestsEnabled();
    const auto restoreDelivery = qScopeGuard([automaticDelivery] {
        KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(automaticDelivery);
    });
    KisSynchronizedConnectionBase::setAutoModeForUnittestsEnabled(false);
    lifetime->setCurrentImage(reopenedImage);
    QTRY_COMPARE(view->image().data(), reopenedImage.data());
    QTRY_COMPARE(window->viewManager()->image().data(), reopenedImage.data());
    QTRY_VERIFY(view->currentNode() && view->currentNode()->image().data() == reopenedImage.data());
    QTRY_VERIFY(checkSelection(reopenedImage));
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    resources->setFGColor(KoColor(QColor(221, 53, 19), cs));
    QTRY_VERIFY(fill->isEnabled());
    fill->trigger();
    QTRY_COMPARE(reopened->operations().undoCount(), 1);
    nativeFill(resources->fgColor());
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, actual.bytes);
    QCOMPARE(reopened->redo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
    const auto editedPath = directory.filePath(QStringLiteral("reedited-selection.kra"));
    QVERIFY(lifetime->saveAs(editedPath, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile editedFile(editedPath); QVERIFY(editedFile.open(QIODevice::ReadOnly));
    const auto edited = LibrePaint::ImportExport::Kra::RasterCodec::read(editedFile);
    QVERIFY2(edited.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(edited.diagnostic));
    const auto editedOwner = paintImageWithEncoding(execution, edited.document.image);
    QCOMPARE(editedOwner->readRgbaPixels(editedOwner->operations().activeLayer(), region, actual.bytes.size()).bytes, nativeBytes());
}

void KisGpuDocumentViewTest::pathToolCreatesArbitraryMaskSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Path tool")));
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
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KisToolSelectPath"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KisToolSelectPath"));
    window->activateWindow();
    native->requestActivate();
    view->canvasBase()->canvasWidget()->setFocus();
    QTest::qWait(100);
    const auto point = [&](QPointF image) { return converter->imageToWidget(image).toPoint(); };
    const QList<QPointF> vertices{{64, 64}, {192, 64}, {192, 192}, {64, 192}};

    QTest::mouseMove(native.data(), point(vertices.first()));
    const auto start = point(vertices.first());
    QEnterEvent entered(start, start, native->mapToGlobal(start));
    QApplication::sendEvent(native.data(), &entered);
    for (const QPointF &vertex : vertices) {
        QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
        QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertex));
    }
    QTest::mouseDClick(native.data(), Qt::LeftButton, Qt::NoModifier, point(vertices.last()));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    const auto mask = owner->selectionMask();
    QVERIFY(mask);
    QCOMPARE(mask->bounds(), QRect(64, 64, 128, 128));
    QVERIFY(mask->contains(80, 80));
    QVERIFY(mask->contains(176, 176));
    QVERIFY(!mask->contains(48, 48));
    QVERIFY(!mask->contains(208, 208));
    QCOMPARE(owner->selectionRect(), mask->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);
}

void KisGpuDocumentViewTest::featherActionSoftensFreehandSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(256, 256));
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Feather selection")));
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

    // A freehand polygon fixes hard arbitrary coverage as the feather source.
    const QList<QPointF> polygon{{64, 64}, {192, 64}, {192, 128}, {128, 192}, {64, 192}};
    QVERIFY(lifetime->image()->setGpuSelectionFreehand(polygon));
    QTRY_COMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QVERIFY(owner->selectionMask());
    QVERIFY(!owner->selectionMask()->isAlpha());
    const QRect hardBounds = owner->selectionRect();
    QCOMPARE(hardBounds, owner->selectionMask()->bounds());
    QCOMPARE(owner->operations().undoCount(), 1);

    KisOperationConfigurationSP config = new KisOperationConfiguration(QStringLiteral("featherselection"));
    config->setProperty("radius", 6);
    window->viewManager()->actionManager()->runOperationFromConfiguration(config);
    lifetime->image()->waitForDone();
    QTRY_VERIFY(owner->selectionMask() && owner->selectionMask()->isAlpha());
    QCOMPARE(owner->selectionKind(), Gpu::SelectionKind::Freehand);
    QCOMPARE(owner->operations().undoCount(), 2);
    const auto soft = owner->selectionMask();
    QCOMPARE(soft->coverage(128, 128), 255);
    const quint8 edge = soft->coverage(hardBounds.left(), 128);
    QVERIFY(edge > 0 && edge < 255);
    QVERIFY(soft->coverage(hardBounds.left() - 4, 128) < edge);

    // The CPU display mirror carries the fractional coverage and an outline.
    QTRY_VERIFY(lifetime->image()->globalSelection());
    const auto mirror = [&] {
        QByteArray bytes(256 * 256, '\0');
        lifetime->image()->globalSelection()->pixelSelection()->readBytes(
            reinterpret_cast<quint8 *>(bytes.data()), QRect(0, 0, 256, 256));
        return bytes;
    };
    QTRY_VERIFY(!lifetime->image()->globalSelection()->pixelSelection()->outlineCache().isEmpty());
    const auto softMirror = mirror();
    QCOMPARE(quint8(softMirror[128 * 256 + 128]), 255);
    QCOMPARE(quint8(softMirror[128 * 256 + hardBounds.left()]), edge);
    QVERIFY(quint8(softMirror[128 * 256 + hardBounds.left() - 4]) > 0);

    // Filling through the soft selection keeps the fractional alpha.
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setFGColor(KoColor(QColor(17, 129, 241), KoColorSpaceRegistry::instance()->rgb8()));
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    auto *fill = window->viewManager()->actionManager()->actionByName(
        QStringLiteral("fill_selection_foreground_color"));
    QVERIFY(fill);
    fill->trigger();
    QTRY_COMPARE(owner->operations().undoCount(), 3);
    const QRect region(QPoint(), QSize(256, 256));
    const auto pixel = [&](const QByteArray &bytes, int x, int y) {
        return bytes.mid((y * 256 + x) * 4, 4);
    };
    const auto filled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(filled.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(filled.bytes, 128, 128), QByteArray::fromHex("1181f1ff"));
    QCOMPARE(quint8(pixel(filled.bytes, hardBounds.left(), 128).at(3)), edge);
    QCOMPARE(pixel(filled.bytes, 8, 8), QByteArray(4, char(0)));

    QCOMPARE(owner->undo(), Gpu::Result::Success);
    const auto unfilled = owner->readRgbaPixels(owner->operations().activeLayer(), region, 256 * 256 * 4);
    QCOMPARE(unfilled.code, Gpu::ExecutionCode::Success);
    QCOMPARE(pixel(unfilled.bytes, 128, 128), QByteArray(4, char(0)));
    QVERIFY(owner->selectionMask()->isAlpha());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QVERIFY(!owner->selectionMask()->isAlpha());
    QTRY_COMPARE(quint8(mirror()[128 * 256 + hardBounds.left()]), 255);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QVERIFY(owner->selectionMask()->isAlpha());
    QTRY_COMPARE(quint8(mirror()[128 * 256 + hardBounds.left()]), edge);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->operations().undoCount(), 3);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("feathered-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false),
             qPrintable(lifetime->errorMessage()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.selectionKind, Gpu::SelectionKind::Freehand);
    QVERIFY(loaded.document.image.selectionMask);
    QVERIFY(loaded.document.image.selectionMask->isAlpha());
    QVERIFY(*loaded.document.image.selectionMask == *soft);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QVERIFY(reopened);
    QVERIFY(reopened->selectionMask() && reopened->selectionMask()->isAlpha());
    QVERIFY(*reopened->selectionMask() == *soft);
    const auto reopenedPixels = reopened->readRgbaPixels(reopened->operations().activeLayer(), region,
                                                         256 * 256 * 4);
    QCOMPARE(reopenedPixels.code, Gpu::ExecutionCode::Success);
    QCOMPARE(reopenedPixels.bytes, filled.bytes);
}

void KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<bool>("sourceWords");
    for (const bool words : {false, true}) for (const bool sourceWords : {false, true})
        QTest::newRow(qPrintable(QStringLiteral("u%1-from-u%2").arg(words ? 16 : 8).arg(sourceWords ? 16 : 8))) << words << sourceWords;
}

void KisGpuDocumentViewTest::pasteClipboardImageCreatesGpuLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QFETCH(bool, sourceWords);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const auto format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = format;
    raster.layers.append({QStringLiteral("Paint"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Paste image"), cs));
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
    const auto profile = QColorSpace::fromIccProfile(cs->profile()->rawData());
    QVERIFY(profile.isValid());
    QImage pasted(16, 16, sourceWords ? QImage::Format_RGBA64 : QImage::Format_RGBA8888);
    if (sourceWords) pasted.fill(QColor::fromRgba64(4353, 33282, 61955, 32769));
    else pasted.fill(QColor(17, 129, 241, 191));
    pasted.setColorSpace(profile);
    const auto sourceColorSpace = sourceWords ? KoColorSpaceRegistry::instance()->rgb16(cs->profile()) :
                                                KoColorSpaceRegistry::instance()->rgb8(cs->profile());
    QByteArray nativeColor(sourceWords ? 8 : 4, Qt::Uninitialized);
    if (sourceWords) {
        const quint16 bgra[]{61955, 33282, 4353, 32769};
        std::memcpy(nativeColor.data(), bgra, sizeof(bgra));
    } else nativeColor = QByteArray::fromHex("f18111bf");
    KoColor expectedColor(reinterpret_cast<const quint8 *>(nativeColor.constData()), sourceColorSpace);
    expectedColor.convertTo(cs);
    Gpu::RgbaPixel rgba;
    if (words) {
        quint16 bgra[4];
        std::memcpy(bgra, expectedColor.data(), sizeof(bgra));
        rgba = {bgra[2], bgra[1], bgra[0], bgra[3]};
    } else {
        const auto *bgra = expectedColor.data();
        rgba = {bgra[2], bgra[1], bgra[0], bgra[3]};
    }
    if (words && sourceWords) QCOMPARE(rgba, (Gpu::RgbaPixel{4353, 33282, 61955, 32769}));
    const auto expectedPixel = Gpu::rgbaBytes(rgba, format);
    const auto clearClipboard = qScopeGuard([] { QGuiApplication::clipboard()->clear(); });
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
    for (qsizetype offset = 0; offset < pixels.bytes.size(); offset += expectedPixel.size())
        if (pixels.bytes.mid(offset, expectedPixel.size()) == expectedPixel) ++matches;
    QCOMPARE(matches, 16 * 16);
    QCOMPARE(owner->operations().snapshot()->colorProfile, cs->profile()->rawData());
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->redo(), Gpu::Result::Success);
    QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const auto path = files.filePath(QStringLiteral("pasted.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile archive(path);
    QVERIFY(archive.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(archive);
    QVERIFY(loaded.code == LibrePaint::ImportExport::Kra::Status::Success);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QCOMPARE(reopened->fillLayerColor(reopened->operations().activeLayer(), rgba), Gpu::Result::Success);
    QCOMPARE(reopened->undo(), Gpu::Result::Success);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, pixels.bytes);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);

    QImage large(128, 128, QImage::Format_RGBA8888);
    large.fill(QColor(241, 129, 17));
    large.setColorSpace(profile);
    QGuiApplication::clipboard()->setImage(large);
    manager->paste();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto cropped = owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024);
    QCOMPARE(cropped.code, Gpu::ExecutionCode::Success);
    int croppedMatches = 0;
    const auto largePixel = Gpu::rgbaBytes(words ? Gpu::RgbaPixel{61937, 33153, 4369, 65535} : Gpu::RgbaPixel{241, 129, 17, 255}, format);
    for (qsizetype offset = 0; offset < cropped.bytes.size(); offset += largePixel.size())
        if (cropped.bytes.mid(offset, largePixel.size()) == largePixel) ++croppedMatches;
    QCOMPARE(croppedMatches, 64 * 64);
}

void KisGpuDocumentViewTest::reselectRestoresGpuSelection()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
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
    const auto owner = paintImageWithEncoding(execution, QSize(128, 128));
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

void KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels_data()
{
    QTest::addColumn<bool>("words");
    QTest::newRow("u8") << false;
    QTest::newRow("u16") << true;
}

void KisGpuDocumentViewTest::copySelectionToNewLayerUsesGpuPixels()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words);
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.layers.append({QStringLiteral("Paint"), true, QUuid::createUuid(), {}});
    const auto owner = paintImageWithEncoding(execution, raster);
    const Gpu::RgbaPixel color = words ? Gpu::RgbaPixel{4353, 33282, 61955, 65535} : Gpu::RgbaPixel{17, 129, 241, 255};
    const auto colorBytes = Gpu::rgbaBytes(color, raster.format);
    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    QCOMPARE(owner->fillLayerColor(owner->operations().activeLayer(), color), Gpu::Result::Success);
    const auto saved = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(saved), Gpu::Result::Success);
    const auto baseLayer = saved->activeLayer;
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Copy selection layer"), cs));
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
    auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("copy_selection_to_new_layer"));
    QVERIFY(action);
    QTRY_VERIFY(!action->isEnabled());
    QSignalSpy selectionChanges(window->viewManager()->selectionManager(),
                                &KisSelectionManager::currentSelectionChanged);
    QVERIFY(selectionChanges.isValid());
    const QRect selected(16, 16, 32, 32);
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));
    QTRY_COMPARE(owner->selectionRect(), selected);
    QVERIFY(lifetime->image()->globalSelection());
    QVERIFY(lifetime->image()->globalSelection()->hasNonEmptyPixelSelection());
    QTRY_VERIFY(!selectionChanges.isEmpty());
    QTRY_VERIFY(action->isEnabled());
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
    const auto expectedSelection = [&](bool ellipse, bool inverted) {
        QByteArray source(64 * 64 * colorBytes.size(), '\0');
        QByteArray coverage(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            const int dx = 2 * x + 1 - 64, dy = 2 * y + 1 - 64;
            const bool inside = ellipse ? dx * dx + dy * dy <= 32 * 32 : selected.contains(x, y);
            if (inverted || selected.contains(x, y))
                std::memcpy(source.data() + (y * 64 + x) * colorBytes.size(), colorBytes.constData(), colorBytes.size());
            coverage[y * 64 + x] = inside != inverted ? char(255) : char(0);
        }
        return nativeClipboardMask(cs, source, coverage, region.size());
    };
    QCOMPARE(copiedPixels.bytes, expectedSelection(false, false));
    int copiedMatches = 0;
    for (qsizetype offset = 0; offset < copiedPixels.bytes.size(); offset += colorBytes.size())
        if (copiedPixels.bytes.mid(offset, colorBytes.size()) == colorBytes) ++copiedMatches;
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
    QCOMPARE(invertedPixels.bytes, expectedSelection(false, true));
    int invertedMatches = 0;
    for (qsizetype offset = 0; offset < invertedPixels.bytes.size(); offset += colorBytes.size())
        if (invertedPixels.bytes.mid(offset, colorBytes.size()) == colorBytes) ++invertedMatches;
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
    QCOMPARE(ellipsePixels.bytes, expectedSelection(true, false));
    const auto ellipsePixel = [&](int x, int y) { return ellipsePixels.bytes.mid((y * 64 + x) * colorBytes.size(), colorBytes.size()); };
    QCOMPARE(ellipsePixel(32, 32), colorBytes);
    auto transparentColor = color;
    transparentColor[3] = 0;
    QCOMPARE(ellipsePixel(16, 16), Gpu::rgbaBytes(transparentColor, raster.format));
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
    QCOMPARE(invertedEllipsePixels.bytes, expectedSelection(true, true));
    const auto invertedEllipsePixel = [&](int x, int y) {
        return invertedEllipsePixels.bytes.mid((y * 64 + x) * colorBytes.size(), colorBytes.size());
    };
    QCOMPARE(invertedEllipsePixel(32, 32), Gpu::rgbaBytes(transparentColor, raster.format));
    QCOMPARE(invertedEllipsePixel(16, 16), colorBytes);
    QTemporaryDir savedFiles;
    QVERIFY(savedFiles.isValid());
    const auto savedPath = savedFiles.filePath(QStringLiteral("copied-selection.kra"));
    lifetime->setFileBatchMode(true);
    QVERIFY2(lifetime->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
    lifetime->waitForSavingToComplete();
    QFile savedFile(savedPath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(savedFile);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QCOMPARE(loaded.document.image.format, raster.format);
    const auto reopened = paintImageWithEncoding(execution, loaded.document.image);
    QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, invertedEllipsePixels.bytes.size()).bytes,
             invertedEllipsePixels.bytes);
    const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!results.isEmpty()) {
        LibrePaint::Canvas::Gpu::CanvasWindow *canvas = nullptr;
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *value = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate); value && value->isVisible()) canvas = value;
        QVERIFY(canvas);
        QSignalSpy frames(canvas, &LibrePaint::Canvas::Gpu::CanvasWindow::frameSubmitted);
        const auto visible = owner->operations().snapshot();
        canvas->refresh();
        QTRY_VERIFY(std::any_of(frames.cbegin(), frames.cend(), [&](const auto &frame) {
            return frame[0].toULongLong() == owner->handle() && frame[1].toULongLong() == visible->revision();
        }));
        QVERIFY(execution->device()->finish());
        QVERIFY(QDir().mkpath(results));
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(results).filePath(
                    QStringLiteral("copied-selection-layer-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png"))));
    }
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    QByteArray alpha(selected.width() * selected.height(), '\0');
    for (int y = 0; y < selected.height(); ++y) for (int x = 0; x < selected.width(); ++x)
        alpha[y * selected.width() + x] = char(std::array<quint8, 6>{0, 1, 127, 128, 254, 255}[(x + 2 * y) % 6]);
    const auto mask = Gpu::SelectionMask::fromAlphaCoverage(selected, alpha, region);
    QVERIFY(mask);
    for (const bool inverted : {false, true}) {
        QVERIFY(lifetime->image()->setGpuSelectionMask(mask, inverted));
        QByteArray coverage(64 * 64, '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
            const quint8 value = mask->coverage(x, y);
            coverage[y * 64 + x] = char(inverted ? 255 - value : value);
        }
        QByteArray source(64 * 64 * colorBytes.size(), '\0');
        for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
            if (inverted || selected.contains(x, y))
                std::memcpy(source.data() + (y * 64 + x) * colorBytes.size(), colorBytes.constData(), colorBytes.size());
        const auto expected = nativeClipboardMask(cs, source, coverage, region.size());
        action->trigger();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->redo(), Gpu::Result::Success);
        QCOMPARE(owner->readRgbaPixels(owner->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QVERIFY(lifetime->saveAs(savedPath, KisDocument::nativeFormatMimeType(), false));
        lifetime->waitForSavingToComplete();
        QFile partialFile(savedPath);
        QVERIFY(partialFile.open(QIODevice::ReadOnly));
        const auto partial = LibrePaint::ImportExport::Kra::RasterCodec::read(partialFile);
        QVERIFY(partial.code == LibrePaint::ImportExport::Kra::Status::Success);
        const auto reopened = paintImageWithEncoding(execution, partial.document.image);
        QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(reopened->fillLayerColor(reopened->operations().activeLayer(), color), Gpu::Result::Success);
        QCOMPARE(reopened->undo(), Gpu::Result::Success);
        QCOMPARE(reopened->readRgbaPixels(reopened->operations().activeLayer(), region, 8 * 1024 * 1024).bytes, expected);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        QTRY_VERIFY(window->viewManager()->activeNode() && window->viewManager()->activeNode()->parent());
        QTRY_COMPARE(window->viewManager()->activeNode()->uuid(), saved->layers.first().uuid);
        window->viewManager()->selectionManager()->paste();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        const auto *clipboard = qobject_cast<const KisMimeData *>(KisClipboard::instance()->layersMimeData());
        QVERIFY(clipboard);
        const auto clipOwner = clipboard->nodes().first()->image()->gpuImage();
        QVERIFY(clipOwner);
        const auto clipRegion = QRect(QPoint(), clipOwner->operations().snapshot()->size);
        const auto pasted = owner->readRgbaPixels(owner->operations().activeLayer(), clipRegion, 8 * 1024 * 1024);
        QCOMPARE(pasted.bytes, clipOwner->readRgbaPixels(clipOwner->operations().activeLayer(), clipRegion, 8 * 1024 * 1024).bytes);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        auto *partialCut = window->viewManager()->actionManager()->actionByName(QStringLiteral("cut_selection_to_new_layer"));
        QVERIFY(partialCut);
        partialCut->trigger();
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
        QCOMPARE(owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024).bytes,
                 nativeClearSelection(cs, colorBytes.repeated(64 * 64), coverage, region.size()));
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QCOMPARE(owner->undo(), Gpu::Result::Success);
        QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
        QCOMPARE(owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024).bytes, colorBytes.repeated(64 * 64));
    }
    QVERIFY(lifetime->image()->setGpuSelectionRect(selected));

    auto *cutAction = window->viewManager()->actionManager()->actionByName(QStringLiteral("cut_selection_to_new_layer"));
    QVERIFY(cutAction);
    cutAction->trigger();
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 2);
    const auto basePixels = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(basePixels.code, Gpu::ExecutionCode::Success);
    const auto cutPixel = Gpu::rgbaPixel(QByteArrayView(basePixels.bytes).mid((24 * 64 + 24) * colorBytes.size(), colorBytes.size()), raster.format);
    QVERIFY(cutPixel);
    QCOMPARE(cutPixel->at(3), quint16(0));
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QCOMPARE(owner->undo(), Gpu::Result::Success);
    QTRY_COMPARE(owner->operations().snapshot()->layers.size(), 1);
    const auto restored = owner->readRgbaPixels(baseLayer, region, 8 * 1024 * 1024);
    QCOMPARE(restored.code, Gpu::ExecutionCode::Success);
    QCOMPARE(restored.bytes.mid((24 * 64 + 24) * colorBytes.size(), colorBytes.size()), colorBytes);
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

void KisGpuDocumentViewTest::stabilizedPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("failure");
    const QStringList presets{QStringLiteral("b)_Basic-5_Size_default.kpp"),
                              QStringLiteral("d)_Ink-3_Gpen.kpp"), QStringLiteral("u)_Pixel_Art_Dithering.kpp")};
    for (int mode = 0; mode < presets.size(); ++mode) for (bool words : {false, true}) {
        const QByteArray tag = presets[mode].toLatin1() + (words ? "-u16" : "-u8");
        QTest::newRow(tag.constData()) << presets[mode] << words << mode << 0;
    }
    for (const auto &entry : QList<QPair<const char *, int>>{{"cancel-pending", 1}, {"input-capacity", 2},
                                                           {"close-on-capacity", 3}})
        QTest::newRow(entry.first) << presets[1] << false << 2 << entry.second;
}

void KisGpuDocumentViewTest::stabilizedPresetsKeepOrdinaryEditing()
{
    QFETCH(QString, presetName);
    QFETCH(bool, words);
    QFETCH(int, mode);
    QFETCH(int, failure);
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    qputenv("LIBREPAINT_GPU_ENGINE", "1");
    KisConfig cfg(false);
    const bool previousDelayed = cfg.stabilizerDelayedPaint();
    cfg.setStabilizerDelayedPaint(mode == 2);
    const auto restoreDelayed = qScopeGuard([&] { KisConfig(false).setStabilizerDelayedPaint(previousDelayed); });
    const auto bundle = KisResourceStorageSP::create(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle"));
    auto preset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(preset);
    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0)
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0)
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    if (preset->settings()->getBool("Texture/Pattern/Enabled")) {
        const auto file = QFileInfo(preset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        auto pattern = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(pattern);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(pattern->md5Sum(), pattern->filename(), pattern->name()))
            QVERIFY(patterns.addResource(pattern));
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetX", false);
        preset->settings()->setProperty("Texture/Pattern/isRandomOffsetY", false);
    }
    preset->setFilename(QStringLiteral("stabilized-") + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
    KisResourceModel presets(ResourceType::PaintOpPresets);
    QVERIFY(presets.addResource(preset));
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QSize size(96, 96);
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    Gpu::RasterImage raster;
    raster.size = size;
    raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    raster.layers = {{QStringLiteral("Paint Layer 1")}};
    Gpu::Limits limits;
    if (failure >= 2) limits.samplesPerStroke = 32;
    auto owner = paintImageWithEncoding(execution, raster, limits);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Stabilized editing"), cs));
    document->setAutoSaveActive(false);
    document->setFileBatchMode(true);
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> input;
    bool pressed = false;
    const auto closeWindow = qScopeGuard([&] {
        if (pressed && input) {
            if (lifetime) lifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(input, Qt::LeftButton);
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
    resources->setPaintOpPreset(preset);
    resources->setSize(mode == 2 ? 12 : 8);
    resources->setOpacity(.7);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(tool);
    const auto configure = [&] {
        return tool->setProperty("smoothingType", 3) && tool->setProperty("smoothnessQualityMin", 6) &&
            tool->setProperty("smoothnessQualityMax", 6) &&
            tool->setProperty("useDelayDistance", mode == 1) && tool->setProperty("delayDistance", 2) &&
            tool->setProperty("finishStabilizedCurve", true) && tool->setProperty("stabilizeSensors", true);
    };
    QVERIFY(configure());
    QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
    if (failure == 3) connect(tool, &KoToolBase::statusTextChanged, window.get(), [&](const QString &message) {
        if (message.isEmpty() || !lifetime) return;
        lifetime->setModified(false);
        window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= size.width() &&
                view->canvasBase()->canvasWidget()->height() >= size.height());
    view->zoomManager()->zoomTo100();
    const auto imageSize = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
    view->canvasController()->setPreferredCenter({imageSize.width() / 2, imageSize.height() / 2});
    view->canvasBase()->canvasWidget()->setFocus();
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = canvas;
    QVERIFY(input);
    QTRY_COMPARE(owner->execution().published(owner->handle()), owner->operations().snapshot());
    QTRY_VERIFY(input && input->isExposed());
    const auto point = [&](QPointF imagePoint) { return view->canvasBase()->coordinatesConverter()->imageToWidget(imagePoint).toPoint(); };
    const auto start = point({16, 24});
    QTest::mouseMove(input, start);
    QEnterEvent entered(start, start, input->mapToGlobal(start));
    QApplication::sendEvent(input, &entered);
    KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Stabilized reference"));
    KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
    nativeImage->addNode(nativeLayer, nativeImage->root());
    const auto pixels = [&] {
        const auto value = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), size.width() * size.height() * cs->pixelSize());
        return value.bytes;
    };
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("stabilized-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(output));
    const auto empty = pixels();
    QCOMPARE(empty.size(), size.width() * size.height() * cs->pixelSize());
    for (int step = 0; step < 3; ++step) {
        QVERIFY(configure());
        resources->setSize(mode == 2 ? 12 : 8);
        resources->setOpacity(.7);
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        resources->setEraserMode(step == 1);
        const KisResourcesSnapshotSP nativeResources(new KisResourcesSnapshot(nativeImage, nativeLayer,
            view->canvasBase()->resourceManager()->canvasResourcesInterface()));
        auto nativeStroke = nativeImage->startStroke(new FreehandStrokeStrategy(nativeResources,
            new KisFreehandStrokeInfo, kundo2_i18n("Stabilized reference")));
        const auto finishNative = [&] {
            if (!nativeStroke) return;
            nativeImage->addJob(nativeStroke, new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
            nativeImage->endStroke(nativeStroke);
            nativeStroke.clear();
            nativeImage->waitForDone();
        };
        const auto finishReference = qScopeGuard(finishNative);
        const auto before = pixels();
        const int history = lifetime->undoStack()->index();
        pressed = true;
        QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, point({16, 24}));
        QVERIFY2(owner->operations().hasActiveStroke(), errors.isEmpty() ? "The stabilized press did not start a stroke" :
                 qPrintable(errors.last().first().toString()));
        // A settings edit during the stroke applies to the next stroke.
        QVERIFY(tool->setProperty("smoothingType", 0));
        QVERIFY(tool->setProperty("useDelayDistance", true));
        QVERIFY(tool->setProperty("delayDistance", 80));
        QVERIFY(tool->setProperty("finishStabilizedCurve", false));
        resources->setSize(24);
        resources->setOpacity(.2);
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        QTest::mouseMove(input, point({40, 24}), 5);
        if (failure == 1) {
            lifetime->image()->requestStrokeCancellation();
        } else if (failure >= 2) {
            QTRY_VERIFY(!owner->operations().hasActiveStroke());
        } else {
            QTest::mouseMove(input, point({56, 40}), 5);
            QTest::mouseMove(input, point({72, 40}), 5);
        }
        if (input) QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier, point({72, 40}));
        else QTest::mouseRelease(window->windowHandle(), Qt::LeftButton);
        pressed = false;
        QVERIFY(!owner->operations().hasActiveStroke());
        if (failure) {
            finishNative();
            QCOMPARE(owner->operations().undoCount(), 0);
            QCOMPARE(pixels(), empty);
            if (failure >= 2) {
                QStringList reasons;
                for (const auto &notification : errors)
                    if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
                QCOMPARE(reasons.size(), 1);
            }
            if (failure == 3) QVERIFY(!lifetime);
            QTest::qWait(50);
            QVERIFY(!owner->operations().hasActiveStroke());
            return;
        }
        QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
        const auto operation = std::get<Gpu::PaintSnapshot>(owner->operations().snapshot()->layers.first().operations.last());
        const auto samples = operation->inputSamples();
        QVERIFY(samples.size() > 2);
        QVERIFY(!operation->brush.basicSmoothing && !operation->brush.weightedSmoothing);
        QCOMPARE(operation->brush.opacity, qreal(.7));
        bool interpolated = false;
        const auto information = [](const Gpu::InputSample &sample) {
            KisPaintInformation result(sample.position, sample.pressure, sample.xTilt, sample.yTilt, sample.rotation,
                                       0, 1, sample.timestamp, sample.speed);
            result.setCanvasRotation(sample.canvasRotation);
            result.setCanvasMirroredH(sample.canvasMirroredH);
            result.setCanvasMirroredV(sample.canvasMirroredV);
            return result;
        };
        qsizetype sampleIndex = 0;
        KisPaintInformation previous;
        for (const auto command : operation->shape.pathCommands) {
            const auto point = information(samples[sampleIndex++]);
            if (command == Gpu::PathCommand::Point)
                nativeImage->addJob(nativeStroke, new FreehandStrokeStrategy::Data(0, point));
            else if (command == Gpu::PathCommand::LineTo)
                nativeImage->addJob(nativeStroke, new FreehandStrokeStrategy::Data(0, previous, point));
            else QVERIFY(command == Gpu::PathCommand::MoveTo);
            previous = point;
            interpolated |= point.pos().x() > 16 && point.pos().x() < 40;
        }
        QVERIFY(interpolated);
        finishNative();
        QByteArray expected(size.width() * size.height() * cs->pixelSize(), Qt::Uninitialized);
        nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
        exchangeRgbaBgra(expected, cs->pixelSize());
        const auto after = pixels();
        for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), after)}) {
            QFile artifact(QDir(output).filePath(QStringLiteral("step-%1-%2.rgba").arg(step).arg(entry.first)));
            QVERIFY(artifact.open(QIODevice::WriteOnly));
            QCOMPARE(artifact.write(entry.second), entry.second.size());
        }
        QFile inputArtifact(QDir(output).filePath(QStringLiteral("step-%1-input.txt").arg(step)));
        QVERIFY(inputArtifact.open(QIODevice::WriteOnly));
        for (const auto &sample : samples) {
            const auto line = QStringLiteral("%1 %2 %3 %4\n").arg(sample.position.x(), 0, 'g', 17).arg(sample.position.y(), 0, 'g', 17)
                .arg(sample.pressure, 0, 'g', 17).arg(sample.timestamp).toUtf8();
            QCOMPARE(inputArtifact.write(line), line.size());
        }
        QCOMPARE(after, expected);
        QVERIFY(after != before);
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), before);
        lifetime->undoStack()->redo();
        QCOMPARE(pixels(), after);
        const auto path = QDir(output).filePath(QStringLiteral("step-%1.kra").arg(step));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto decoded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
        QVERIFY2(decoded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(decoded.diagnostic));
        auto imported = paintImageWithEncoding(execution, decoded.document.image);
        QCOMPARE(imported->readRgbaPixels(imported->operations().activeLayer(), QRect(QPoint(), size), after.size()).bytes, after);
        if (step == 1) {
            // The next ordinary pointer stroke targets the reloaded document pixels.
            file.close();
            lifetime->waitForSavingToComplete();
            QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
            owner = lifetime->image()->gpuImage();
            QVERIFY(owner);
            window->viewManager()->nodeManager()->slotUiActivatedNode(lifetime->image()->root()->firstChild());
            resources->setPaintOpPreset(preset);
            view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            view->canvasBase()->canvasWidget()->setFocus();
            KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
            tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
            QVERIFY(tool);
            for (auto *candidate : QGuiApplication::allWindows())
                if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = canvas;
            QTRY_VERIFY(input && input->isExposed());
        }
    }
}

void KisGpuDocumentViewTest::stabilizerPrimitivesRetainIndependentLineStarts()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    QVERIFY2(execution, qPrintable(diagnostic));
    const QSize size(64, 64);
    auto owner = paintImageWithEncoding(execution, size);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Stabilizer primitives")));
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
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    const KoColor color(QColor(17, 129, 241), cs);
    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(8);
    resources->setOpacity(1);
    resources->setFGColor(color);
    resources->setCurrentCompositeOp(QStringLiteral("normal"));
    resources->setEraserMode(false);
    resources->setGlobalAlphaLock(false);
    resources->setMirrorHorizontal(false);
    resources->setMirrorVertical(false);
    class PrimitiveProbe : public KisToolFreehandHelper {
    public:
        using KisToolFreehandHelper::KisToolFreehandHelper;
        using KisToolFreehandHelper::initPaintImpl;
        using KisToolFreehandHelper::paintAt;
        using KisToolFreehandHelper::paintLine;
        int elapsedStrokeTime() const override { return 0; }
    };
    view->canvasBase()->resourceManager()->setResource(KoCanvasResource::EffectiveZoom, 1.0);
    KisPaintingInformationBuilder builder;
    PrimitiveProbe helper(&builder, view->canvasBase()->resourceManager(), kundo2_i18n("Stabilizer primitives"));
    helper.smoothingOptions()->setSmoothingType(KisSmoothingOptions::STABILIZER);
    helper.smoothingOptions()->setUseDelayDistance(false);
    helper.smoothingOptions()->setFinishStabilizedCurve(false);
    KisConfig cfg(false);
    const bool delayed = cfg.stabilizerDelayedPaint();
    cfg.setStabilizerDelayedPaint(false);
    const auto restore = qScopeGuard([&] { KisConfig(false).setStabilizerDelayedPaint(delayed); });
    const KisPaintInformation first(QPointF(16, 16)), next(QPointF(24, 24)), tap(QPointF(48, 48));
    helper.initPaintImpl(0, first, view->canvasBase()->resourceManager(), lifetime->image(),
                         lifetime->image()->root()->firstChild(), nullptr);
    QVERIFY(helper.isRunning());
    // Releasing before delayed painting starts issues the current tap first,
    // followed by the queued line from its own earlier start position.
    helper.paintAt(tap);
    helper.paintLine(first, next);
    helper.endPaint();
    QVERIFY(!helper.isRunning());
    KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Native primitives"));
    KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
    nativeImage->addNode(nativeLayer, nativeImage->root());
    KisPainter painter(nativeLayer->paintDevice());
    painter.setPaintColor(color);
    painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
    KisDistanceInformation distance;
    painter.paintAt(tap, &distance);
    painter.paintLine(first, next, &distance);
    QByteArray expected(size.width() * size.height() * 4, Qt::Uninitialized);
    nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
    exchangeRgbaBgra(expected, 4);
    const auto actual = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
    const auto output = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!output.isEmpty()) for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), actual)}) {
        QFile file(QDir(output).filePath(QStringLiteral("stabilizer-independent-line-%1.rgba").arg(entry.first)));
        QVERIFY(QDir().mkpath(output));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(entry.second), entry.second.size());
    }
    QVERIFY2(actual == expected, "A stabilizer tap must retain the independent start of its queued line");
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
    auto owner = paintImageWithEncoding(execution, QSize(64, 64), limits);
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
             QByteArray::fromHex("1181f100"));
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
        const auto imported = paintImageWithEncoding(execution, reopened.document.image);
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

void KisGpuDocumentViewTest::brushBlendModeUsesEffectiveCompositeOpAndRejectsUnsupported()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    const auto execution = std::make_shared<Gpu::Execution>(device);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    Gpu::RasterLayer paintLayer;
    paintLayer.name = QStringLiteral("Paint Layer 1");
    paintLayer.uuid = QUuid(QStringLiteral("{7187a4a8-3f6c-4d0d-9b71-2f8a2bc3d4e5}"));
    paintLayer.defaultPixel = {17, 34, 51, 255};
    raster.layers = {paintLayer};
    const auto owner = paintImageWithEncoding(execution, raster);
    QCOMPARE(owner->markSaved(owner->operations().snapshot()), Gpu::Result::Success);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("Brush blend mode")));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    QPointer<LibrePaint::Canvas::Gpu::CanvasWindow> native;
    bool pressed = false;
    const auto releaseWindow = qScopeGuard([&] {
        if (pressed && native) {
            if (lifetime) lifetime->image()->requestStrokeCancellation();
            QTest::mouseRelease(native.data(), Qt::LeftButton);
        }
        if (lifetime) {
            lifetime->setModified(false);
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

    auto *resources = window->viewManager()->canvasResourceProvider();
    resources->setPaintOpPreset(m_preset);
    resources->setSize(24);
    resources->setOpacity(1);
    resources->setEraserMode(false);
    resources->setFGColor(KoColor(QColor(221, 17, 44), KoColorSpaceRegistry::instance()->rgb8()));
    auto image = KisImageSP(lifetime->image());
    auto layerNode = image->root()->firstChild();
    QVERIFY(layerNode);
    const auto capture = [&](const QString &composite) {
        resources->setCurrentCompositeOp(composite);
        return captureGpuBrushResources(image, layerNode, view->canvasBase()->resourceManager());
    };

    const QList<QPair<QString, Gpu::LayerBlendMode>> mappings = {
        {QStringLiteral("normal"), Gpu::LayerBlendMode::Normal},
        {QStringLiteral("multiply"), Gpu::LayerBlendMode::Multiply},
        {QStringLiteral("soft_light"), Gpu::LayerBlendMode::SoftLightPhotoshop},
        {QStringLiteral("screen"), Gpu::LayerBlendMode::Screen},
        {QStringLiteral("darken"), Gpu::LayerBlendMode::Darken},
        {QStringLiteral("lighten"), Gpu::LayerBlendMode::Lighten},
        {QStringLiteral("add"), Gpu::LayerBlendMode::Addition},
        {QStringLiteral("linear_dodge"), Gpu::LayerBlendMode::LinearDodge},
        {QStringLiteral("erase"), Gpu::LayerBlendMode::Normal}
    };
    for (const auto &mapping : mappings) {
        const auto captured = capture(mapping.first);
        QVERIFY2(captured.diagnostic.isEmpty(), qPrintable(mapping.first + QStringLiteral(": ") + captured.diagnostic));
        QCOMPARE(captured.brush.blendMode, mapping.second);
        QCOMPARE(captured.brush.erase, mapping.first == QLatin1String("erase"));
    }

    // Leave eraser mode before exercising the unsupported painting blend.
    resources->setEraserMode(false);
    QVERIFY(!resources->eraserMode());
    const auto unsupported = capture(QStringLiteral("librepaint-test-invalid-composite"));
    QCOMPARE(resources->resourceManager()->resource(KoCanvasResource::CurrentEffectiveCompositeOp).toString(),
             QStringLiteral("librepaint-test-invalid-composite"));
    QVERIFY(!unsupported.diagnostic.isEmpty());
    QCOMPARE(unsupported.brush.blendMode, Gpu::LayerBlendMode::Normal);

    KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
    QTRY_COMPARE(KoToolManager::instance()->activeToolId(), QStringLiteral("KritaShape/KisToolBrush"));
    auto *brushTool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
    QVERIFY(brushTool);
    QSignalSpy errors(brushTool, &KoToolBase::statusTextChanged);
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
    const auto first = enterAt({24, 24});
    auto last = converter->imageToWidget(QPointF(40, 40)).toPoint();
    QVERIFY(view->canvasBase()->canvasWidget()->rect().contains(first));

    // A mapped mode reaches the frozen operation and paints pixels.
    resources->setCurrentCompositeOp(QStringLiteral("multiply"));
    pressed = true;
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QVERIFY2(owner->operations().hasActiveStroke(),
             errors.isEmpty() ? "The pointer press did not start a stroke" : qPrintable(errors.first().first().toString()));
    QTest::mouseMove(native.data(), last);
    QTest::qWait(10);
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, last);
    pressed = false;
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().undoCount(), 1);
    const auto operation = std::get<Gpu::PaintSnapshot>(
        owner->operations().snapshot()->layers.first().operations.last());
    QCOMPARE(operation->brush.blendMode, Gpu::LayerBlendMode::Multiply);
    const auto layerId = owner->operations().activeLayer();
    const auto painted = owner->readRgbaPixels(layerId, QRect(32, 32, 1, 1), 4);
    QCOMPARE(painted.code, Gpu::ExecutionCode::Success);
    QVERIFY(quint8(painted.bytes.at(3)) > 0);

    // Invalid resource settings must not add a history entry or draw pixels.
    resources->setCurrentCompositeOp(QStringLiteral("librepaint-test-invalid-composite"));
    const auto beforeRejected = owner->operations().snapshot();
    QTest::mousePress(native.data(), Qt::LeftButton, Qt::NoModifier, first);
    QTest::qWait(20);
    QVERIFY(!owner->operations().hasActiveStroke());
    QCOMPARE(owner->operations().snapshot(), beforeRejected);
    QCOMPARE(owner->operations().undoCount(), 1);
    QStringList reasons;
    for (const auto &notification : errors)
        if (!notification.first().toString().isEmpty()) reasons.append(notification.first().toString());
    QVERIFY(!reasons.isEmpty());
    QTest::mouseRelease(native.data(), Qt::LeftButton, Qt::NoModifier, first);

    // KRA stores the resulting blended pixels, not the blend intent.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    lifetime->setFileBatchMode(true);
    const auto path = directory.filePath(QStringLiteral("brush-blend.kra"));
    QVERIFY(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false));
    lifetime->waitForSavingToComplete();
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto loaded = LibrePaint::ImportExport::Kra::RasterCodec::read(file);
    QVERIFY2(loaded.code == LibrePaint::ImportExport::Kra::Status::Success, qPrintable(loaded.diagnostic));
    QString reopenDiagnostic;
    const auto reopenExecution = KisPart::instance()->gpuExecution(reopenDiagnostic);
    QVERIFY2(reopenExecution, qPrintable(reopenDiagnostic));
    Gpu::PaintImage reopened(reopenExecution, loaded.document.image);
    const auto reopenedPixel = reopened.readRgbaPixels(reopened.operations().activeLayer(), QRect(32, 32, 1, 1), 4);
    QCOMPARE(reopenedPixel.code, Gpu::ExecutionCode::Success);
    QCOMPARE(reopenedPixel.bytes, painted.bytes);
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
        document->setCurrentImage(KisImage::fromGpuImage(paintImageWithEncoding(execution, QSize(64, 64)), QStringLiteral("Initial image")));
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
        replacement = KisImage::fromGpuImage(paintImageWithEncoding(execution, QSize(96, 80)), QStringLiteral("Replacement image"));
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
    const auto previous = paintImageWithEncoding(execution, QSize(64, 64));
    const auto sourceOwner = paintImageWithEncoding(differentExecution ? otherExecution : execution, replaceIdentity ? QSize(96, 80) : QSize(64, 64));
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
    auto owner = paintImageWithEncoding(execution, QSize(imageLimitPan ? Gpu::RgbaTile::CoordinateLimit : 128, 128));
    const auto snapshot = owner->operations().snapshot();
    QCOMPARE(owner->markSaved(snapshot), Gpu::Result::Success);
    const auto retainedVersion = owner->operations().preview();
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
    QCOMPARE(owner->operations().preview(), retainedVersion);
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
    auto owner = paintImageWithEncoding(execution, QSize(128, 128));
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

void KisGpuDocumentViewTest::nonlinearU16CanvasTracksTheActualDisplayConfiguration()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto *registry = KoColorSpaceRegistry::instance();
    const auto raw = QColorSpace(QColorSpace::SRgb).iccProfile();
    const auto *profile = registry->createColorProfileFromData(QStringLiteral("RGBA"), QStringLiteral("U16"), raw);
    QVERIFY(profile);
    const auto *source = registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U16"), profile);
    QVERIFY(source);
    Gpu::RasterImage raster;
    raster.size = {64, 64};
    raster.format = Gpu::PixelFormat::Rgba16;
    raster.colorProfile = raw;
    Gpu::RasterLayer layer;
    layer.name = QStringLiteral("Nonlinear RGB");
    layer.defaultPixel = {0x3333, 0x6666, 0x9999, 65535};
    raster.layers.append(layer);
    auto owner = paintImageWithEncoding(execution, raster);
    std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
    document->setCurrentImage(KisImage::fromGpuImage(owner, QStringLiteral("ICC canvas"), source));
    std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
    window->resize(800, 600);
    auto *view = window->newView(document.get());
    QVERIFY(view);
    QPointer<KisDocument> lifetime(document.release());
    const auto close = qScopeGuard([&] {
        if (lifetime) {
            lifetime->setModified(false);
            if (window) {
                auto *action = window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"));
                if (action) action->trigger();
            }
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        window.reset();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    });
    window->viewManager()->setCurrentView(view);
    window->show();
    LibrePaint::Canvas::Gpu::CanvasWindow *native = nullptr;
    const auto findNative = [&] {
        for (auto *candidate : QGuiApplication::allWindows())
            if (auto *canvas = qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate))
                if (canvas->isVisible()) native = canvas;
        return bool(native);
    };
    QTRY_VERIFY(findNative());
    native->setCursor(Qt::BlankCursor);
    auto *converter = view->canvasBase()->displayColorConverter();
    const auto pixel = [&] {
        const auto frame = native->screen()->grabWindow(native->winId()).toImage();
        const auto point = view->canvasBase()->coordinatesConverter()->imageToWidgetTransform().map(QPointF(32, 32));
        return frame.pixelColor((point * native->devicePixelRatio()).toPoint());
    };
    const quint16 bgra[] = {0x9999, 0x6666, 0x3333, 65535};
    const auto expected = [&] {
        const auto config = converter->multiSurfaceDisplayConfig().canvasDisplayConfig();
        const auto *target = registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U8"), config.profile);
        quint8 result[4]{};
        if (!target || !source->convertPixelsTo(reinterpret_cast<const quint8 *>(bgra), result,
                                                target, 1, config.intent, config.conversionFlags))
            return QColor();
        return QColor(result[2], result[1], result[0], result[3]);
    };
    const auto first = expected();
    QVERIFY(first.isValid());
    native->refresh();
    QTRY_COMPARE(execution.published(owner->handle()), owner->operations().snapshot());
    QTRY_COMPARE(pixel(), first);
    auto configuration = converter->multiSurfaceDisplayConfig();
    configuration.canvasProfile = registry->p709G10Profile();
    QVERIFY(configuration.canvasProfile);
    converter->setMultiSurfaceDisplayConfig(configuration);
    const auto second = expected();
    QVERIFY(second.isValid());
    QVERIFY(second != first);
    QTRY_COMPARE(pixel(), second);
    QCOMPARE(owner->operations().snapshot()->colorProfile, raw);
    QCOMPARE(owner->operations().snapshot()->layers.first().defaultPixel, layer.defaultPixel);
}

void KisGpuDocumentViewTest::presentsEditsAndReleasesTheClosedTab()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QString diagnostic;
    const auto device = Gpu::Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    Gpu::Execution execution(device);
    auto owner = paintImageWithEncoding(execution, QSize(64, 64));
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
    auto owner = paintImageWithEncoding(execution, QSize(128, 128));
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
    const auto retainedVersion = owner->operations().preview();
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
                    // Document, history and retainedVersion retain exact comparisons.
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
        QVERIFY(owner->operations().preview() == retainedVersion);
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
        QVERIFY(owner->operations().preview() == retainedVersion);
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
            QVERIFY(owner->operations().preview() == retainedVersion);
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

void KisGpuDocumentViewTest::standardAutoBrushPresetsKeepOrdinaryEditing_data()
{
    QTest::addColumn<QString>("presetName");
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("tipCase");
    QTest::addColumn<qreal>("normalDiameter");
    QTest::addColumn<bool>("rotatedCanvas");
    for (const auto *name : {"b)_Basic-5_Size_default.kpp", "b)_Basic-2_Opacity.kpp", "a)_Eraser_Circle.kpp",
                             "b)_Basic-5_Size_Opacity.kpp", "a)_Eraser_Small.kpp", "d)_Ink-3_Gpen.kpp",
                             "b)_Basic-3_Flow.kpp", "b)_Basic-4_Flow_Opacity.kpp", "d)_Ink-2_Fineliner.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray(name) + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(40) << false;
        }
    for (const auto *name : {"b)_Basic-3_Flow.kpp", "b)_Basic-4_Flow_Opacity.kpp"})
        for (bool words : {false, true}) {
            const QByteArray row = QByteArray("constant-rotation-mirrored-canvas-") + name + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(40) << true;
        }
    for (bool words : {false, true}) {
        const auto suffix = words ? "-u16" : "-u8";
        QTest::newRow(qPrintable(QStringLiteral("ellipse-fractional") + suffix))
            << QStringLiteral("b)_Basic-5_Size_default.kpp") << words << 1 << qreal(13.75) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-rotated") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 2 << qreal(40) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-rotated-mirrored-canvas") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 2 << qreal(40) << true;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-hard-horizontal-edge") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 3 << qreal(13.75) << false;
        QTest::newRow(qPrintable(QStringLiteral("rectangle-isotropic-spacing") + suffix))
            << QStringLiteral("b)_Basic-2_Opacity.kpp") << words << 4 << qreal(40) << false;
    }
    for (bool words : {false, true})
        for (const auto *mode : {"subtract", "multiply", "darken"})
            for (bool soft : {false, true}) {
                const QByteArray row = QByteArray("texture-") + mode + (soft ? "-soft" : "-hard") + (words ? "-u16" : "-u8");
                QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
            }

    for (bool words : {false, true}) {
        for (const auto *mode : {"preset", "taper"}) {
            const QByteArray row = QByteArray("texture-") + mode + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
        }
        for (int precision = 1; precision <= 5; ++precision) {
            const QByteArray row = QByteArray("texture-precision-") + QByteArray::number(precision) + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("d)_Ink-7_Brush_Rough.kpp") << words << 0 << qreal(40) << false;
        }
    }

    for (bool words : {false, true})
        QTest::newRow(words ? "texture-screentone-u16" : "texture-screentone-u8")
            << QStringLiteral("y)_Screentones_Regular.kpp") << words << 0 << qreal(40) << false;
    for (bool words : {false, true})
        for (const auto *mode : {"preset", "taper", "sharpness-soft-aligned", "sharpness-legacy"}) {
            const QByteArray row = QByteArray("dither-") + mode + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << QStringLiteral("u)_Pixel_Art_Dithering.kpp") << words << 0 << qreal(12) << false;
        }

    const QList<QPair<QString, qreal>> gaussianPresets{
        {QStringLiteral("u)_Pixel_Art.kpp"), 1}, {QStringLiteral("u)_Pixel_Art_Fill.kpp"), 32},
        {QStringLiteral("h)_Charcoal_pencil_large.kpp"), 32}, {QStringLiteral("h)_Charcoal_Pencil_Medium.kpp"), 12},
        {QStringLiteral("h)_Charcoal_Pencil_Thin.kpp"), 6}};
    for (const auto &preset : gaussianPresets) for (bool words : {false, true}) {
        const QByteArray row = preset.first.toLatin1() + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << preset.first << words << 0 << preset.second << false;
    }

    for (const auto *name : {"c)_Pencil-1_Hard.kpp", "c)_Pencil-2.kpp"}) for (bool words : {false, true}) {
        const QByteArray row = QByteArray(name) + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << QString::fromLatin1(name) << words << 0 << qreal(10) << false;
    }

    for (const auto &preset : QList<QPair<QString, qreal>>{{QStringLiteral("c)_Pencil-4_Soft.kpp"), 16},
                                                         {QStringLiteral("c)_Pencil-6_Quick_Shade.kpp"), 60},
                                                         {QStringLiteral("c)_Pencil-3_Large_4B.kpp"), 10},
                                                         {QStringLiteral("c)_Pencil-5_Tilted.kpp"), 40},
                                                         {QStringLiteral("b)_Basic-6_Details.kpp"), 7.5}})
        for (bool words : {false, true}) {
            const QByteArray row = preset.first.toLatin1() + (words ? "-u16" : "-u8");
            QTest::newRow(row.constData()) << preset.first << words << 0 << preset.second << false;
        }

    for (bool words : {false, true}) for (const auto *mode : {"soft", "ellipse"}) {
        const QByteArray row = QByteArray("gaussian-unaliased-") + mode + (words ? "-u16" : "-u8");
        QTest::newRow(row.constData()) << QStringLiteral("u)_Pixel_Art_Fill.kpp") << words << 0 << qreal(32) << false;
    }

    for (bool words : {false, true})
        QTest::newRow(words ? "default-buildup-u16" : "default-buildup-u8")
            << QStringLiteral("c)_Pencil-3_Large_4B.kpp") << words << 0 << qreal(10) << false;

    for (bool words : {false, true})
        QTest::newRow(words ? "details-tilt-size-u16" : "details-tilt-size-u8")
            << QStringLiteral("b)_Basic-6_Details.kpp") << words << 0 << qreal(7.5) << false;

}

void KisGpuDocumentViewTest::standardAutoBrushPresetsKeepOrdinaryEditing()
{
    QFETCH(QString, presetName);
    const bool imageTip = presetName == QLatin1String("b)_Basic-6_Details.kpp") || presetName == QLatin1String("c)_Pencil-4_Soft.kpp") || presetName == QLatin1String("c)_Pencil-6_Quick_Shade.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    const bool incrementalPencil = presetName == QLatin1String("c)_Pencil-3_Large_4B.kpp") || presetName == QLatin1String("c)_Pencil-5_Tilted.kpp");
    QFETCH(bool, words);
    QFETCH(int, tipCase);
    QFETCH(qreal, normalDiameter);
    QFETCH(bool, rotatedCanvas);
    const int channelBytes = words ? 2 : 1;
    const int pixelBytes = 4 * channelBytes;
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto previousEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (previousEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", previousEngine);
    });
    const QString bundlePath = QStringLiteral(KRITA_SOURCE_DIR "/krita/data/bundles/Krita_4_Default_Resources.bundle");
    const auto bundle = KisResourceStorageSP::create(bundlePath);
    QVERIFY(bundle->valid());
    KisResourceModel brushes(ResourceType::Brushes);
    if (brushes.rowCount() == 0)
        QVERIFY(brushes.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/libs/brush/tests/data/brush.gbr"), false));
    KisResourceModel patterns(ResourceType::Patterns);
    if (patterns.rowCount() == 0)
        QVERIFY(patterns.importResourceFile(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/pattern.pat"), false));
    auto sourcePreset = bundle->resource(QStringLiteral("paintoppresets/") + presetName).dynamicCast<KisPaintOpPreset>();
    QVERIFY(sourcePreset);
    if (imageTip) {
        QDomDocument definition;
        QVERIFY(definition.setContent(sourcePreset->settings()->getString("brush_definition")));
        const auto file = QFileInfo(definition.documentElement().attribute("filename")).fileName();
        const auto tip = bundle->resource(QStringLiteral("brushes/") + file);
        QVERIFY(tip);
        auto source = KisGlobalResourcesInterface::instance()->source<KoResource>(ResourceType::Brushes);
        if (!source.bestMatch(tip->md5Sum(), tip->filename(), tip->name())) {
            KisResourceModel brushResources(ResourceType::Brushes);
            QVERIFY(brushResources.addResource(tip));
        }
    }

    const bool pencil = presetName.startsWith(QLatin1String("c)_Pencil-"));
    const bool directionalTip = pencil || presetName == QLatin1String("b)_Basic-6_Details.kpp");
    const bool fineliner = presetName == QLatin1String("d)_Ink-2_Fineliner.kpp");
    const auto restoreTiming = fixedBrushPointerTiming(fineliner);
    const bool screentone = presetName == QLatin1String("y)_Screentones_Regular.kpp");
    const bool dither = presetName == QLatin1String("u)_Pixel_Art_Dithering.kpp");
    const bool textured = sourcePreset->settings()->getBool("Texture/Pattern/Enabled");
    const QByteArray dataTag(QTest::currentDataTag());
    const bool tiltSize = dataTag.startsWith("details-tilt-size-");
    if (tiltSize) {
        // The existing brush editor combines tilt-driven size with direction
        // rotation and corner fans, including interpolated pen axes at the fan.
        auto settings = sourcePreset->settings();
        settings->setProperty("SizeSensor", QStringLiteral("<params id='declination'><curve>0,.5;1,1;</curve></params>"));
        settings->setProperty("SizeUseSameCurve", false);
    }
    if (dataTag.startsWith("default-buildup-"))
        sourcePreset->settings()->removeProperty("PaintOpAction");
    if (dataTag.startsWith("gaussian-unaliased-")) {
        // Existing brush-editor controls: keep the native Gaussian gradient
        // when sharpness is unchecked, including an elliptical rotated tip.
        sourcePreset->settings()->setProperty("PressureSharpness", false);
        if (dataTag.contains("ellipse")) {
            QDomDocument definition;
            QVERIFY(definition.setContent(sourcePreset->settings()->getString("brush_definition")));
            auto root = definition.documentElement();
            root.setAttribute(QStringLiteral("angle"), .37);
            auto mask = root.firstChildElement(QStringLiteral("MaskGenerator"));
            mask.setAttribute(QStringLiteral("ratio"), .8);
            mask.setAttribute(QStringLiteral("hfade"), .5);
            mask.setAttribute(QStringLiteral("vfade"), .5);
            sourcePreset->settings()->setProperty("brush_definition", definition.toString());
        }
    }
    if (textured) {
        const auto file = QFileInfo(sourcePreset->settings()->getString("Texture/Pattern/PatternFileName")).fileName();
        const auto paper = bundle->resource(QStringLiteral("patterns/") + file).dynamicCast<KoPattern>();
        QVERIFY(paper);
        if (!KisGlobalResourcesInterface::instance()->source<KoPattern>(ResourceType::Patterns).bestMatch(paper->md5Sum(), paper->filename(), paper->name()))
            QVERIFY(patterns.addResource(paper));
        auto settings = sourcePreset->settings();
        // Freeze native random outputs as part of the comparison's input.
        settings->setProperty("Texture/Pattern/isRandomOffsetX", false);
        settings->setProperty("Texture/Pattern/isRandomOffsetY", false);
        settings->setProperty("Texture/Pattern/OffsetX", screentone || dither ? 0 : 7);
        settings->setProperty("Texture/Pattern/OffsetY", screentone || dither ? 0 : 11);
        const QByteArray row(QTest::currentDataTag());
        if (row.startsWith("texture-subtract") || row.startsWith("texture-multiply") || row.startsWith("texture-darken")) {
            settings->setProperty("Texture/Pattern/TexturingMode", row.contains("multiply") ? 0 : row.contains("darken") ? 4 : 1);
            settings->setProperty("Texture/Pattern/UseSoftTexturing", row.contains("-soft"));
        }
        // A short fade exercises per-dab sequence and the pressure sum during
        // the ordinary strokes, including reflected and overlapping dabs.
        auto sensors = settings->getString("Texture/Strength/Sensor");
        if (row.startsWith("texture-") && !row.contains("preset")) sensors.replace(QStringLiteral("length=\"1000\""), QStringLiteral("length=\"7\""));
        if (row.contains("precision")) {
            settings->setProperty("KisPrecisionOption/AutoPrecisionEnabled", false);
            settings->setProperty("KisPrecisionOption/precisionLevel", row.split('-').value(2).toInt());
        }
        settings->setProperty("Texture/Strength/Sensor", sensors);
        if (row.contains("sharpness-soft-aligned")) {
            settings->setProperty("SharpnessValue", .65);
            settings->setProperty("SharpnessUseCurve", true);
            settings->setProperty("SharpnessSensor", QStringLiteral("<params id='pressure'><curve>0,0;1,1;</curve></params>"));
            settings->setProperty("SharpnessUseSameCurve", false);
            settings->setProperty("SharpnesscurveMode", 0);
            settings->setProperty("Sharpness/alignoutline", true);
            settings->setProperty("Sharpness/softness", 45);
        } else if (row.contains("sharpness-legacy")) {
            settings->removeProperty("SharpnessValue");
            settings->setProperty("Sharpness/factor", .65);
        }
    }

    const auto cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QSize size(128, 128);
    QList<QByteArray> expectedStates;
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto output = QDir(qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR", temporary.path())).filePath(
        QStringLiteral("ordinary-brush-%1").arg(QString::fromLatin1(QTest::currentDataTag())));
    QVERIFY(QDir().mkpath(output));
    // Random normal strokes retain their own seed. Fixed-seed native comparisons
    // below protect their pixels; this UI path protects editing and persistence.
    for (bool gpu : pencil && !incrementalPencil ? QList<bool>{true} : QList<bool>{false, true}) {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        auto preset = sourcePreset->clone().dynamicCast<KisPaintOpPreset>();
        QVERIFY(preset);
        if (tipCase) {
            QDomDocument definition;
            QVERIFY(definition.setContent(preset->settings()->getString("brush_definition")));
            auto brush = definition.documentElement();
            auto mask = brush.firstChildElement(QStringLiteral("MaskGenerator"));
            brush.setAttribute(QStringLiteral("angle"), tipCase == 1 ? .37 : .51);
            mask.setAttribute(QStringLiteral("type"), tipCase == 1 ? QStringLiteral("circle") : QStringLiteral("rect"));
            mask.setAttribute(QStringLiteral("ratio"), tipCase == 1 ? .4 : .37);
            mask.setAttribute(QStringLiteral("hfade"), tipCase == 1 ? .6 : tipCase == 3 ? 1. : .8);
            mask.setAttribute(QStringLiteral("vfade"), tipCase == 1 ? .2 : .3);
            if (tipCase == 3) mask.setAttribute(QStringLiteral("antialiasEdges"), 0);
            preset->settings()->setProperty("brush_definition", definition.toString());
            if (tipCase == 4) preset->settings()->setProperty("Spacing/Isotropic", true);
        }
        preset->setFilename((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) +
                            QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".kpp"));
        KisResourceModel presets(ResourceType::PaintOpPresets);
        QVERIFY(presets.addResource(preset));
        std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
        document->setAutoSaveActive(false);
        document->setFileBatchMode(true);
        QVERIFY(document->newImage(QStringLiteral("Ordinary brush editing"), size.width(), size.height(), cs,
                                   KoColor(QColor(0, 0, 0, 0), cs), KisConfig::CANVAS_COLOR, 1, {}, 1));
        QCOMPARE(bool(document->image()->gpuImage()), gpu);
        std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
        window->resize(800, 600);
        auto *view = window->newView(document.get());
        QVERIFY(view);
        QPointer<KisDocument> lifetime(document.release());
        BrushPointerClock pointerClock;
        QPointer<QWindow> input;
        bool pressed = false;
        const auto closeWindow = qScopeGuard([&] {
            if (pressed && input) {
                if (lifetime) lifetime->image()->requestStrokeCancellation();
                QTest::mouseRelease(input, Qt::LeftButton);
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
        resources->setPaintOpPreset(preset);
        resources->setSize(normalDiameter);
        resources->setOpacity(.7);
        resources->setGlobalAlphaLock(false);
        resources->setMirrorHorizontal(false);
        resources->setMirrorVertical(false);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        auto *tool = KoToolManager::instance()->toolById(view->canvasBase(), QStringLiteral("KritaShape/KisToolBrush"));
        QVERIFY(tool);
        QVERIFY(tool->setProperty("smoothingType", 0));
        QSignalSpy errors(tool, &KoToolBase::statusTextChanged);
        const auto prepareInput = [&] {
            view->canvasBase()->canvasWidget()->setFocus();
            view->canvasController()->resetCanvasRotation();
            view->canvasController()->mirrorCanvas(false);
            view->zoomManager()->zoomTo100();
            const auto extent = view->canvasBase()->coordinatesConverter()->imageRectInWidgetPixels().size();
            view->canvasController()->setPreferredCenter({extent.width() / 2, extent.height() / 2});
            if (rotatedCanvas) {
                view->canvasController()->rotateCanvas(23);
                view->canvasController()->mirrorCanvas(true);
            }
            input = view->canvasBase()->canvasWidget()->window()->windowHandle();
            if (gpu) for (auto *candidate : QGuiApplication::allWindows())
                if (qobject_cast<LibrePaint::Canvas::Gpu::CanvasWindow *>(candidate)) input = candidate;
            if (fineliner && input) input->installEventFilter(&pointerClock);
        };
        QTRY_VERIFY(view->canvasBase()->canvasWidget()->width() >= size.width() &&
                    view->canvasBase()->canvasWidget()->height() >= size.height());
        prepareInput();
        QTRY_VERIFY(input && input->isExposed());
        if (gpu) {
            const auto capture = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                          view->canvasBase()->resourceManager());
            QVERIFY2(capture.diagnostic.isEmpty(), qPrintable(capture.diagnostic));
            QVERIFY(capture.brush.autoBrush);
            QCOMPARE(capture.brush.autoBrush->mask, imageTip ? Gpu::AutoBrush::Mask::Image : incrementalPencil ? Gpu::AutoBrush::Mask::GaussianCircle : (presetName == QLatin1String("b)_Basic-5_Size_Opacity.kpp") ||
                      presetName == QLatin1String("b)_Basic-3_Flow.kpp") || presetName == QLatin1String("b)_Basic-4_Flow_Opacity.kpp") ||
                      presetName.startsWith(QLatin1String("h)_Charcoal_")) || pencil ||
                      presetName == QLatin1String("u)_Pixel_Art.kpp") || presetName == QLatin1String("u)_Pixel_Art_Fill.kpp")) ?
                     Gpu::AutoBrush::Mask::GaussianCircle :
                     (tipCase >= 2 || presetName == QLatin1String("a)_Eraser_Small.kpp")) ?
                     Gpu::AutoBrush::Mask::Rectangle : Gpu::AutoBrush::Mask::Circle);
            if (textured) {
                QVERIFY(capture.brush.texture);
                QCOMPARE(capture.brush.texture->offset, screentone || dither ? QPoint() : QPoint(7, 11));
                const auto frozen = *capture.brush.texture;
                auto currentPreset = resources->currentPreset();
                auto settings = currentPreset->settings();
                settings->setProperty("Texture/Pattern/isRandomOffsetX", true);
                settings->setProperty("Texture/Pattern/isRandomOffsetY", true);
                const auto randomized = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                                 view->canvasBase()->resourceManager());
                QVERIFY2(randomized.diagnostic.isEmpty(), qPrintable(randomized.diagnostic));
                QVERIFY(randomized.brush.texture);
                const auto &paper = *randomized.brush.texture;
                QVERIFY(paper.offset.x() >= 0 && paper.offset.x() < paper.size.width());
                QVERIFY(paper.offset.y() >= 0 && paper.offset.y() < paper.size.height());
                QCOMPARE(paper.alpha, frozen.alpha);
                settings->setProperty("Texture/Pattern/isRandomOffsetX", false);
                settings->setProperty("Texture/Pattern/isRandomOffsetY", false);
                const auto brightness = settings->getDouble("Texture/Pattern/Brightness");
                settings->setProperty("Texture/Pattern/Brightness", .2);
                const auto changed = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                              view->canvasBase()->resourceManager());
                QVERIFY2(changed.diagnostic.isEmpty(), qPrintable(changed.diagnostic));
                QVERIFY(changed.brush.texture && changed.brush.texture->alpha != frozen.alpha);
                QCOMPARE(*capture.brush.texture, frozen);
                settings->setProperty("Texture/Pattern/Brightness", brightness);
            }
            struct PressureProbe { qreal first, second, speed = 0; QPointF tilt; qreal tiltOffset = 0, canvasRotation = 0; bool mirroredH = false, mirroredV = false; };
            QList<PressureProbe> pressureCases{{40, .01}, {40, .05}, {40, .1}, {40, .33}, {40, .75}, {40, 1},
                                              {200, .03}, {200, .05}, {200, .06}};
            if (imageTip) {
                pressureCases.prepend({normalDiameter, 1});
                pressureCases.prepend({normalDiameter, .2});
                QVERIFY(capture.brush.autoBrush->imageTips);
            }
            if (incrementalPencil || tiltSize) {
                if (!imageTip) pressureCases.prepend({normalDiameter, 1, 0, {}});
                for (const QPointF tilt : {QPointF(35, 15), QPointF(-35, 15), QPointF(15, -35), QPointF(60, 60)})
                    pressureCases.append({normalDiameter, .6, 0, tilt, 17});
                pressureCases.append({normalDiameter, .6, 0, {35, 15}, 17, 37, true, false});
                pressureCases.append({normalDiameter, .6, 0, {35, 15}, 17, 37, true, true});
            }
            if (fineliner) for (const auto speed : {qreal(0), qreal(.2), qreal(.9), qreal(1)}) pressureCases.append({5, .6, speed});
            // This bundled size curve starts at x=.00813008. Both low-pressure
            // inputs must retain the native constant endpoint extension.
            if (dither) pressureCases.prepend({40, .004});
            if (dither) pressureCases.prepend({40, 0});
            for (const auto &input : pressureCases) {
                const auto diameter = input.first;
                const auto pressure = input.second;
                const qreal speed = input.speed;
                const QString probeName = QStringLiteral("pressure-d%1-p%2").arg(diameter).arg(pressure) +
                    (fineliner ? QStringLiteral("-s%1").arg(speed) : QString()) +
                    (incrementalPencil || tiltSize ? QStringLiteral("-tilt%1,%2-offset%3-canvas%4-%5%6").arg(input.tilt.x()).arg(input.tilt.y()).arg(input.tiltOffset).arg(input.canvasRotation).arg(input.mirroredH).arg(input.mirroredV) : QString());
                resources->setSize(diameter);
                const auto pressureCapture = captureGpuBrushResources(lifetime->image(), lifetime->image()->root()->firstChild(),
                                                                      view->canvasBase()->resourceManager());
                QVERIFY2(pressureCapture.diagnostic.isEmpty(), qPrintable(pressureCapture.diagnostic));
                const qreal endPressure = textured && (QByteArray(QTest::currentDataTag()).contains("precision") || QByteArray(QTest::currentDataTag()).contains("taper") || QByteArray(QTest::currentDataTag()).contains("sharpness")) ? std::min(pressure, qreal(.33)) : pressure;
                KisImageSP nativeImage = new KisImage(nullptr, size.width(), size.height(), cs, QStringLiteral("Native preset reference"));
                KisPaintLayerSP nativeLayer = new KisPaintLayer(nativeImage, QStringLiteral("Reference"), 255);
                nativeImage->addNode(nativeLayer, nativeImage->root());
                if (resources->currentPreset()->hasMaskingPreset()) {
                    // The production stroke combines independently spaced primary
                    // and GrayA8 painters; a single paint-op omits its secondary tip.
                    const auto opacity = resources->opacity();
                    resources->setOpacity(1);
                    const auto restoreOpacity = qScopeGuard([&] { resources->setOpacity(opacity); });
                    {
                        KisFigurePaintingStroke stroke(kundo2_i18n("Native brush reference"), nativeImage, nativeLayer,
                            view->canvasBase()->resourceManager(), KisFigurePaintingOptions::StrokeStyleForeground,
                            KisFigurePaintingOptions::FillStyleNone);
                        stroke.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure),
                                         KisPaintInformation(QPointF(104.25, 64.75), endPressure));
                    }
                    nativeImage->waitForDone();
                } else {
                    if (!incrementalPencil) nativeLayer->setTemporaryTarget(nativeLayer->paintDevice());
                    KisPainter painter(nativeLayer->paintDevice());
                    painter.setPaintColor(KoColor(QColor(17, 129, 241), cs));
                    painter.setCompositeOpId(incrementalPencil ? COMPOSITE_OVER : COMPOSITE_ALPHA_DARKEN);
                    painter.setPaintOpPreset(resources->currentPreset(), nativeLayer, nativeImage);
                    KisDistanceInformation distance;
                    if (directionalTip) {
                        distance = KisDistanceInformation(QPointF(64.25, 64.75), .37);
                        const KisRandomSourceSP random(new KisRandomSource(17));
                        KisPaintInformation first(QPointF(64.25, 64.75), pressure, input.tilt.x(), input.tilt.y(), 0, 0, 1, 0, speed);
                        KisPaintInformation second(QPointF(104.25, 64.75), endPressure, -input.tilt.x(), input.tilt.y() / 2, 0, 0, 1, 0, speed);
                        KisPaintInformation third(QPointF(66.25, 63.75), pressure / 2, input.tilt.x() / 3, -input.tilt.y(), 0, 0, 1, 0, speed);
                        for (auto *point : {&first, &second, &third}) {
                            point->setRandomSource(random); point->setTiltDirectionOffset(input.tiltOffset);
                            point->setCanvasRotation(input.canvasRotation); point->setCanvasMirroredH(input.mirroredH); point->setCanvasMirroredV(input.mirroredV);
                        }
                        painter.paintLine(first, second, &distance);
                        painter.paintLine(second, third, &distance);
                    } else if (fineliner) painter.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure, 0, 0, 0, 0, 1, 0, speed),
                                                    KisPaintInformation(QPointF(104.25, 64.75), endPressure, 0, 0, 0, 0, 1, 1, 1 - speed), &distance);
                    else if (textured) painter.paintLine(KisPaintInformation(QPointF(64.25, 64.75), pressure),
                                                   KisPaintInformation(QPointF(104.25, 64.75), endPressure), &distance);
                    else painter.paintAt(fineliner ? KisPaintInformation(QPointF(64.25, 64.75), pressure, 0, 0, 0, 0, 1, 0, speed) :
                                         KisPaintInformation(QPointF(64.25, 64.75), pressure), &distance);
                    bool pending;
                    do {
                        QVector<KisRunnableStrokeJobData *> jobs;
                        pending = painter.paintOp()->doAsynchronousUpdate(jobs).second;
                        painter.runnableStrokeJobsInterface()->addRunnableJobs(jobs);
                    } while (pending);
                }
                nativeLayer->setTemporaryTarget({});
                nativeImage->waitForDone();
                // BUILDUP's reference is the directly painted layer, including
                // RGB at zero alpha. WASH merges its source with canvas opacity.
                KisPaintDeviceSP destination = nativeLayer->paintDevice();
                if (!incrementalPencil) {
                    destination = new KisPaintDevice(cs);
                    KisPainter merge(destination);
                    merge.setOpacityF(.7);
                    for (const auto &rect : nativeLayer->paintDevice()->region().rects())
                        merge.bitBlt(rect.topLeft(), nativeLayer->paintDevice(), rect);
                }
                QByteArray expected(size.width() * size.height() * pixelBytes, Qt::Uninitialized);
                destination->readBytes(reinterpret_cast<quint8 *>(expected.data()), QRect(QPoint(), size));
                QString diagnostic;
                Gpu::RasterImage raster;
                raster.size = size;
                raster.format = words ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
                raster.colorProfile = cs->profile()->rawData();
                raster.layers.append(Gpu::RasterLayer{QStringLiteral("Pressure")});
                const auto pressureOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                const auto operation = pressureOwner->beginStroke(pressureOwner->operations().activeLayer(), pressureCapture.brush, 17);
                QVERIFY(!operation.isNull());
                QList<Gpu::InputSample> pressureSamples{{{64.25, 64.75}, pressure}};
                if (textured || fineliner || directionalTip) pressureSamples.append({{104.25, 64.75}, endPressure});
                if (directionalTip) {
                    pressureSamples.append({{66.25, 63.75}, pressure / 2});
                    for (auto &sample : pressureSamples) sample.initialDrawingAngle = .37;
                }
                if (incrementalPencil || tiltSize) {
                    pressureSamples[0].xTilt = input.tilt.x(); pressureSamples[0].yTilt = input.tilt.y();
                    pressureSamples[1].xTilt = -input.tilt.x(); pressureSamples[1].yTilt = input.tilt.y() / 2;
                    pressureSamples[2].xTilt = input.tilt.x() / 3; pressureSamples[2].yTilt = -input.tilt.y();
                    for (auto &sample : pressureSamples) {
                        sample.tiltDirectionOffset = input.tiltOffset; sample.canvasRotation = input.canvasRotation;
                        sample.canvasMirroredH = input.mirroredH; sample.canvasMirroredV = input.mirroredV;
                    }
                }
                for (auto &sample : pressureSamples) sample.speed = speed;
                if (fineliner) { pressureSamples.last().speed = 1 - speed; pressureSamples.last().timestamp = 1; }
                QCOMPARE(pressureOwner->appendSamples(operation, pressureSamples), Gpu::Result::Success);
                QCOMPARE(pressureOwner->commitStroke(operation), Gpu::Result::Success);
                const auto readback = pressureOwner->readRgbaPixels(pressureOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size());
                QVERIFY2(readback.code == Gpu::ExecutionCode::Success, qPrintable(readback.diagnostic));
                auto actual = readback.bytes;
                for (qsizetype i = 0; i < actual.size(); i += pixelBytes)
                    for (int byte = 0; byte < channelBytes; ++byte) std::swap(actual[i + byte], actual[i + 2 * channelBytes + byte]);
                for (const auto &entry : {qMakePair(QStringLiteral("native"), expected), qMakePair(QStringLiteral("gpu"), actual)}) {
                    QFile artifact(QDir(output).filePath(probeName + QLatin1Char('-') + entry.first + QStringLiteral(".bgra")));
                    QVERIFY(artifact.open(QIODevice::WriteOnly));
                    QCOMPARE(artifact.write(entry.second), entry.second.size());
                }
                if (actual != expected) {
                    if (textured) {
                        auto originalBrush = pressureCapture.brush;
                        originalBrush.texture.reset(); originalBrush.opacity = 1;
                        const auto originalOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                        const auto originalOperation = originalOwner->beginStroke(originalOwner->operations().activeLayer(), originalBrush, 17);
                        QVERIFY(!originalOperation.isNull());
                        QCOMPARE(originalOwner->appendSamples(originalOperation, {{{64.75, 64.75}, pressure}}), Gpu::Result::Success);
                        QCOMPARE(originalOwner->commitStroke(originalOperation), Gpu::Result::Success);
                        auto original = originalOwner->readRgbaPixels(originalOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
                        for (qsizetype i = 0; i < original.size(); i += pixelBytes)
                            for (int byte = 0; byte < channelBytes; ++byte) std::swap(original[i + byte], original[i + 2 * channelBytes + byte]);
                        QFile artifact(QDir(output).filePath(probeName + QStringLiteral("-original.bgra")));
                        QVERIFY(artifact.open(QIODevice::WriteOnly)); QCOMPARE(artifact.write(original), original.size());
                    }

                    QByteArray nativeTemporary(expected.size(), Qt::Uninitialized);
                    nativeLayer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(nativeTemporary.data()), QRect(QPoint(), size));
                    auto fullOpacityBrush = pressureCapture.brush;
                    fullOpacityBrush.opacity = 1;
                    const auto temporaryOwner = Gpu::PaintImageSP::create(KisPart::instance()->gpuExecution(diagnostic), raster);
                    const auto temporaryOperation = temporaryOwner->beginStroke(temporaryOwner->operations().activeLayer(), fullOpacityBrush, 17);
                    QVERIFY(!temporaryOperation.isNull());
                    QCOMPARE(temporaryOwner->appendSamples(temporaryOperation, pressureSamples), Gpu::Result::Success);
                    QCOMPARE(temporaryOwner->commitStroke(temporaryOperation), Gpu::Result::Success);
                    auto gpuTemporary = temporaryOwner->readRgbaPixels(temporaryOwner->operations().activeLayer(), QRect(QPoint(), size), expected.size()).bytes;
                    for (qsizetype i = 0; i < gpuTemporary.size(); i += pixelBytes)
                        for (int byte = 0; byte < channelBytes; ++byte) std::swap(gpuTemporary[i + byte], gpuTemporary[i + 2 * channelBytes + byte]);
                    for (const auto &entry : {qMakePair(QStringLiteral("native-temporary"), nativeTemporary),
                                              qMakePair(QStringLiteral("gpu-temporary"), gpuTemporary)}) {
                        QFile artifact(QDir(output).filePath(probeName + QLatin1Char('-') + entry.first + QStringLiteral(".bgra")));
                        QVERIFY(artifact.open(QIODevice::WriteOnly));
                        QCOMPARE(artifact.write(entry.second), entry.second.size());
                    }
                }
                QVERIFY2(actual == expected, qPrintable(QStringLiteral("Preset %1 diameter %2 pressure %3 speed %4 must keep native pixels").arg(presetName).arg(diameter).arg(pressure).arg(speed)));
            }
            resources->setSize(normalDiameter);
        }
        const auto stroke = [&](const QList<QPointF> &points) {
            const auto *converter = view->canvasBase()->coordinatesConverter();
            const auto first = converter->imageToWidget(points.first()).toPoint();
            const auto target = gpu ? first : view->canvasBase()->canvasWidget()->mapTo(window.get(), first);
            if (fineliner) pointerClock.advance();
            QTest::mouseMove(input, target);
            QEnterEvent entered(first, first, view->canvasBase()->canvasWidget()->mapToGlobal(first));
            QApplication::sendEvent(gpu ? static_cast<QObject *>(input.data()) : view->canvasBase()->canvasWidget(), &entered);
            pressed = true;
            if (fineliner) pointerClock.advance();
            QTest::mousePress(input, Qt::LeftButton, Qt::NoModifier, target);
            for (const auto &point : points.mid(1)) {
                const auto next = converter->imageToWidget(point).toPoint();
                if (fineliner) pointerClock.advance();
                QTest::mouseMove(input, gpu ? next : view->canvasBase()->canvasWidget()->mapTo(window.get(), next), 5);
            }
            const auto last = converter->imageToWidget(points.last()).toPoint();
            if (fineliner) pointerClock.advance();
            QTest::mouseRelease(input, Qt::LeftButton, Qt::NoModifier,
                                gpu ? last : view->canvasBase()->canvasWidget()->mapTo(window.get(), last));
            pressed = false;
            lifetime->image()->waitForDone();
            QApplication::processEvents();
        };
        const auto pixels = [&] {
            auto image = KisImageSP(lifetime->image());
            image->waitForDone();
            if (const auto owner = image->gpuImage()) {
                const auto value = owner->readRgbaPixels(owner->operations().activeLayer(), QRect(QPoint(), size), size.width() * size.height() * pixelBytes);
                QByteArray bgra = value.bytes;
                for (qsizetype i = 0; i < bgra.size(); i += pixelBytes)
                    for (int byte = 0; byte < channelBytes; ++byte) std::swap(bgra[i + byte], bgra[i + 2 * channelBytes + byte]);
                return bgra;
            }
            QByteArray result(size.width() * size.height() * pixelBytes, Qt::Uninitialized);
            window->viewManager()->nodeManager()->activeNode()->paintDevice()->readBytes(
                reinterpret_cast<quint8 *>(result.data()), QRect(QPoint(), size));
            return result;
        };
        int state = 0;
        const auto compareState = [&](const QString &name) {
            const auto actual = pixels();
            QFile artifact(QDir(output).filePath((gpu ? QStringLiteral("gpu-") : QStringLiteral("cpu-")) + name + QStringLiteral(".bgra")));
            if (!artifact.open(QIODevice::WriteOnly) || artifact.write(actual) != actual.size()) return false;
            if (pencil && !incrementalPencil) return true; // Seeded random comparisons above; ordinary history below.
            if (!gpu) expectedStates.append(actual);
            return !gpu || actual == expectedStates.value(state++);
        };
        stroke({{32, 32}, {80, 48}, {80, 80}});
        QTRY_COMPARE(lifetime->undoStack()->count(), 1);
        const auto painted = pixels();
        QVERIFY(painted != QByteArray(painted.size(), '\0'));
        QVERIFY2(compareState(QStringLiteral("painted")), "Ordinary preset painting must match the native engine");
        resources->setGlobalAlphaLock(true);
        resources->setFGColor(KoColor(QColor(241, 17, 129), cs));
        if (normalDiameter <= 1) stroke({{80, 56}, {80, 72}});
        else stroke({{48, 40}, {64, 48}});
        const auto protectedPixels = pixels();
        QVERIFY(protectedPixels != painted);
        for (qsizetype i = 3 * channelBytes; i < painted.size(); i += pixelBytes)
            QCOMPARE(protectedPixels.mid(i, channelBytes), painted.mid(i, channelBytes));
        QVERIFY2(compareState(QStringLiteral("alpha-locked")), "Alpha protection must preserve the native preset's color result");
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), painted);
        resources->setGlobalAlphaLock(false);
        resources->setFGColor(KoColor(QColor(17, 129, 241), cs));
        resources->setEraserMode(true);
        // One-pixel tips erase the existing vertical part of the stroke.
        if (normalDiameter <= 1) stroke({{80, 56}, {80, 72}});
        else stroke({{48, 40}, {64, 56}});
        QTRY_COMPARE(lifetime->undoStack()->count(), 2);
        const auto erased = pixels();
        QVERIFY(erased != painted);
        QVERIFY2(compareState(QStringLiteral("erased")), "Ordinary preset erasing must match the native engine");
        lifetime->undoStack()->undo();
        QCOMPARE(pixels(), painted);
        lifetime->undoStack()->redo();
        QCOMPARE(pixels(), erased);
        window->viewManager()->nodeManager()->duplicateActiveNode();
        lifetime->image()->waitForDone();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 2U);
        QCOMPARE(pixels(), erased);
        lifetime->undoStack()->undo();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 1U);
        lifetime->undoStack()->redo();
        QTRY_COMPARE(lifetime->image()->root()->childCount(), 2U);
        const auto path = QDir(output).filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("cpu.kra"));
        QVERIFY2(lifetime->saveAs(path, KisDocument::nativeFormatMimeType(), false), qPrintable(lifetime->errorMessage()));
        lifetime->waitForSavingToComplete();
        QVERIFY2(lifetime->openPath(path, KisDocument::DontAddToRecent), qPrintable(lifetime->errorMessage()));
        QCOMPARE(bool(lifetime->image()->gpuImage()), gpu);
        window->viewManager()->nodeManager()->slotUiActivatedNode(lifetime->image()->root()->firstChild());
        QCOMPARE(pixels(), erased);
        prepareInput();
        resources->setPaintOpPreset(preset);
        resources->setSize(normalDiameter);
        resources->setOpacity(.7);
        resources->setGlobalAlphaLock(false);
        resources->setMirrorHorizontal(false);
        resources->setMirrorVertical(false);
        resources->setEraserMode(false);
        resources->setCurrentCompositeOp(QStringLiteral("normal"));
        if (normalDiameter <= 1) stroke({{104, 108}, {112, 108}});
        else stroke({{108, 108}});
        QVERIFY(pixels() != erased);
        QVERIFY2(compareState(QStringLiteral("reopened-edited")), "A reopened document must retain the preset's editing result");
        resources->setMirrorHorizontal(true);
        stroke({{92, 24}, {108, 40}});
        QVERIFY2(compareState(QStringLiteral("mirrored")), "Canvas symmetry must reflect the native preset mask");
        stroke({{52, 24}, {76, 40}});
        QVERIFY2(compareState(QStringLiteral("mirror-overlap")), "Overlapping mirrored dabs must preserve the native editing result");
        QVERIFY(window->screen()->grabWindow(window->winId()).save(QDir(output).filePath(gpu ? QStringLiteral("gpu-window.png") : QStringLiteral("cpu-window.png"))));
        for (const auto &error : errors) QVERIFY2(error.first().toString().isEmpty(), qPrintable(error.first().toString()));
    }
}


void KisGpuDocumentViewTest::selectionFillToolKeepsNativeEditing_data()
{
    RegionFillEditingTest::addData();
}

void KisGpuDocumentViewTest::selectionFillToolKeepsNativeEditing()
{
    RegionFillEditingTest::run(m_preset);
}

void KisGpuDocumentViewTest::gradientsKeepNativeEditingHistoryAndKra_data()
{
    QTest::addColumn<bool>("words");
    QTest::addColumn<int>("shape");
    QTest::addColumn<int>("repeat");
    for (const bool words : {false, true}) for (int shape = 0; shape < 8; ++shape) for (int repeat = 0; repeat < 3; ++repeat) {
        const auto name = QStringLiteral("%1-shape-%2-repeat-%3").arg(words ? "u16" : "u8").arg(shape).arg(repeat).toLatin1();
        QTest::newRow(name.constData()) << words << shape << repeat;
    }
}

void KisGpuDocumentViewTest::gradientsKeepNativeEditingHistoryAndKra()
{
    // Consumer: the ordinary gradient tool. Native and GPU documents must keep
    // identical selected paint/mask pixels, projection and history through
    // resource changes, KRA reopening and another gradient operation.
    namespace Gpu = LibrePaint::Image::Gpu;
    QFETCH(bool, words); QFETCH(int, shape); QFETCH(int, repeat);
    const bool maskTarget = (shape + repeat) % 2;
    const bool groupOwner = maskTarget && shape == 3;
    const auto oldEngine = qgetenv("LIBREPAINT_GPU_ENGINE");
    const auto restoreEngine = qScopeGuard([&] {
        if (oldEngine.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE"); else qputenv("LIBREPAINT_GPU_ENGINE", oldEngine);
    });
    const auto *cs = words ? KoColorSpaceRegistry::instance()->rgb16() : KoColorSpaceRegistry::instance()->rgb8();
    const QRect area(0, 0, 32, 24);
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto baseline = directory.filePath(QStringLiteral("native-gradient.kra"));
    QUuid paintUuid, targetUuid, maskUuid;
    qputenv("LIBREPAINT_GPU_ENGINE", "0");
    {
        QScopedPointer<KisDocument> document(KisPart::instance()->createDocument());
        document->setFileBatchMode(true); document->setAutoSaveActive(false);
        KisImageSP image = new KisImage(document->createUndoStore(), area.width(), area.height(), cs, "Gradient editing");
        KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("Paint"), 255);
        layer->paintDevice()->fill(area, KoColor(QColor(77, 113, 157, 176), cs));
        KisNodeSP target = layer;
        if (groupOwner) {
            KisGroupLayerSP group = new KisGroupLayer(image, QStringLiteral("Masked group"), 255);
            QVERIFY(image->addNode(group)); QVERIFY(image->addNode(layer, group)); target = group;
        } else QVERIFY(image->addNode(layer));
        paintUuid = layer->uuid(); targetUuid = target->uuid();
        if (maskTarget) {
            KisSelectionSP coverage = new KisSelection;
            coverage->pixelSelection()->select(area, 177);
            KisTransparencyMaskSP mask = new KisTransparencyMask(image, QStringLiteral("Transparency Mask"));
            mask->setSelection(coverage); QVERIFY(image->addNode(mask, target)); maskUuid = mask->uuid();
        }
        KisSelectionSP selected = new KisSelection;
        selected->pixelSelection()->select({3, 4, 26, 17}, 211);
        selected->pixelSelection()->select({12, 10, 5, 5}, 0);
        KisSelectionMaskSP selection = new KisSelectionMask(image);
        selection->setSelection(selected); selection->setActive(true); QVERIFY(image->addNode(selection, image->root()));
        document->setCurrentImage(image); image->refreshGraphAsync(); image->waitForDone();
        QVERIFY(document->saveAs(baseline, KisDocument::nativeFormatMimeType(), false)); document->waitForSavingToComplete();
        const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
        if (!results.isEmpty()) {
            QVERIFY(QDir().mkpath(results));
            QVERIFY(QFile::copy(baseline, QDir(results).filePath(QStringLiteral("gradient-%1-source.kra").arg(QString::fromLatin1(QTest::currentDataTag())))));
        }
    }
    QList<QByteArray> reference;
    for (const bool gpu : {false, true}) {
        // Compare KRA tabs on the application's shared execution, including
        // the reopened document's attachment and retained-resource lifetime.
        if (gpu && !m_gradientExecution) {
            QString diagnostic;
            m_gradientExecution = KisPart::instance()->gpuExecution(diagnostic);
            QVERIFY2(m_gradientExecution, qPrintable(diagnostic));
        }
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
        const auto saved = directory.filePath(gpu ? QStringLiteral("gpu.kra") : QStringLiteral("native.kra"));
        int stage = 0;
        for (const bool reopened : {false, true}) {
            std::unique_ptr<KisDocument> document(KisPart::instance()->createDocument());
            document->setFileBatchMode(true); document->setAutoSaveActive(false);
            QVERIFY2(document->openPath(reopened ? saved : baseline, KisDocument::DontAddToRecent), qPrintable(document->errorMessage()));
            auto image = KisImageSP(document->image()); image->waitForDone();
            QCOMPARE(bool(image->gpuImage()), gpu);
            auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
            auto target = KisLayerUtils::findNodeByUuid(image->root(), targetUuid); QVERIFY(target);
            auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid);
            if (maskTarget) { QVERIFY(mask); QCOMPARE(mask->parent(), target); }
            std::unique_ptr<KisMainWindow> window(KisPart::instance()->createMainWindow());
            auto *view = window->newView(document.get()); QVERIFY(view);
            QPointer<KisDocument> lifetime(document.release());
            const auto cleanup = qScopeGuard([&] {
                if (lifetime) {
                    lifetime->setModified(false);
                    window->viewManager()->actionManager()->actionByName(QStringLiteral("file_close"))->trigger();
                }
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); window.reset();
                QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            });
            window->viewManager()->setCurrentView(view); window->resize(800, 600); window->show();
            QVERIFY(QTest::qWaitForWindowExposed(window->windowHandle()));
            view->canvasBase()->snapGuide()->enableSnapping(false);
            window->viewManager()->nodeManager()->slotNonUiActivatedNode(maskTarget ? mask : layer);
            QTRY_COMPARE(view->currentNode(), maskTarget ? mask : layer);
            const auto capture = [&]() -> QByteArray {
                image->waitForDone();
                QByteArray pixels(area.width() * area.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0');
                layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
                image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
                QByteArray coverage;
                if (maskTarget) {
                    coverage.resize(area.width() * area.height());
                    qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(
                        reinterpret_cast<quint8 *>(coverage.data()), area);
                }
                return pixels + projection + coverage;
            };
            const auto check = [&] {
                const auto state = capture();
                const int currentStage = stage++;
                const auto results = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
                if (!results.isEmpty()) {
                    QFile output(QDir(results).filePath(QStringLiteral("gradient-%1-%2-%3.bin")
                        .arg(QString::fromLatin1(QTest::currentDataTag()), gpu ? "gpu" : "native").arg(currentStage)));
                    QVERIFY(output.open(QIODevice::WriteOnly)); QCOMPARE(output.write(state), state.size());
                }
                if (!gpu) reference.append(state); else QCOMPARE(state, reference.value(currentStage));
            };
            check();
            auto *resources = window->viewManager()->canvasResourceProvider();
            resources->setPaintOpPreset(m_preset);
            resources->setFGColor(KoColor(reopened ? QColor(213, 49, 107) : QColor(17, 129, 241), cs));
            resources->setBGColor(KoColor(QColor(221, 17, 44), cs));
            const auto gradient = QSharedPointer<KoStopGradient>::create();
            gradient->setName(QStringLiteral("Frozen ordinary gradient"));
            gradient->setStops({{0, KoColor(Qt::black, cs), FOREGROUNDSTOP},
                {.43, KoColor(QColor(39, 213, 71, 97), cs)}, {1, KoColor(QColor(221, 17, 44, 0), cs)}});
            gradient->setValid(true); resources->slotGradientActivated(gradient);
            resources->setOpacity(.63); resources->setEraserMode(reopened && repeat == 2);
            resources->setCurrentCompositeOp(repeat == 2 ? QStringLiteral("copy") : QStringLiteral("normal"));
            if (reopened && repeat == 2) resources->setCurrentCompositeOp(QStringLiteral("erase"));
            resources->setMirrorHorizontal(false); resources->setMirrorVertical(false); resources->setGlobalAlphaLock(false);
            if (!maskTarget && shape == 2 && repeat == 2) {
                QBitArray channels(4, false); channels.setBit(0); channels.setBit(2);
                qobject_cast<KisPaintLayer *>(layer.data())->setChannelLockFlags(channels);
            }
            const auto toolId = QStringLiteral("KritaFill/KisToolGradient");
            KoToolManager::instance()->switchToolRequested(toolId);
            auto *tool = qobject_cast<KisTool *>(KoToolManager::instance()->toolById(view->canvasBase(), toolId)); QVERIFY(tool);
            // Initialize controls before assigning the retained option values.
            tool->optionWidgets();
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetShape", Qt::DirectConnection, Q_ARG(int, shape)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetRepeat", Qt::DirectConnection, Q_ARG(int, repeat)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetReverse", Qt::DirectConnection, Q_ARG(bool, repeat == 2 || (reopened && shape >= 4))));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetDither", Qt::DirectConnection, Q_ARG(bool, repeat == 1)));
            QVERIFY(QMetaObject::invokeMethod(tool, "slotSetAntiAliasThreshold", Qt::DirectConnection, Q_ARG(qreal, .2)));
            const auto before = capture(); const int history = lifetime->undoStack()->index();
            const QPointF start = reopened ? QPointF(20.75, 15.25) : QPointF(9.25, 9.75);
            const QPointF end = reopened ? QPointF(15.5, 8.125) : QPointF(16.875, 13.25);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent first(&press, view->canvasBase()->coordinatesConverter()->imageToDocument(start)); tool->beginPrimaryAction(&first);
            QMouseEvent move(QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            KoPointerEvent update(&move, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->continuePrimaryAction(&update);
            QKeyEvent cancel(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            view->canvasBase()->toolProxy()->keyPressEvent(&cancel); image->waitForDone();
            QVERIFY(cancel.isAccepted());
            QMouseEvent cancelledRelease(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            KoPointerEvent cancelledLast(&cancelledRelease, view->canvasBase()->coordinatesConverter()->imageToDocument(end));
            tool->endPrimaryAction(&cancelledLast);
            QCOMPARE(lifetime->undoStack()->index(), history); QCOMPARE(capture(), before);
            tool->beginPrimaryAction(&first); tool->continuePrimaryAction(&update);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            KoPointerEvent last(&release, view->canvasBase()->coordinatesConverter()->imageToDocument(end)); tool->endPrimaryAction(&last);
            image->waitForDone(); QTRY_COMPARE(lifetime->undoStack()->index(), history + 1);
            const auto edited = capture(); QVERIFY(edited != before); check();
            resources->setFGColor(KoColor(Qt::yellow, cs));
            gradient->setStops({{0, KoColor(Qt::red, cs)}, {1, KoColor(Qt::blue, cs)}});
            lifetime->undoStack()->undo(); image->waitForDone(); QCOMPARE(capture(), before); check();
            lifetime->undoStack()->redo(); image->waitForDone(); QCOMPARE(capture(), edited); check();
            QVERIFY(lifetime->saveAs(saved, KisDocument::nativeFormatMimeType(), false)); lifetime->waitForSavingToComplete();
        }
        if (gpu) {
            qputenv("LIBREPAINT_GPU_ENGINE", "0");
            QScopedPointer<KisDocument> native(KisPart::instance()->createDocument());
            native->setFileBatchMode(true); native->setAutoSaveActive(false);
            QVERIFY2(native->openPath(saved, KisDocument::DontAddToRecent), qPrintable(native->errorMessage()));
            auto image = KisImageSP(native->image()); image->waitForDone();
            auto layer = KisLayerUtils::findNodeByUuid(image->root(), paintUuid); QVERIFY(layer);
            QByteArray pixels(area.width() * area.height() * cs->pixelSize(), '\0'), projection(pixels.size(), '\0'), coverage;
            layer->paintDevice()->readBytes(reinterpret_cast<quint8 *>(pixels.data()), area);
            image->projection()->readBytes(reinterpret_cast<quint8 *>(projection.data()), area);
            if (maskTarget) {
                auto mask = KisLayerUtils::findNodeByUuid(image->root(), maskUuid); QVERIFY(mask);
                coverage.resize(area.width() * area.height());
                qobject_cast<KisTransparencyMask *>(mask.data())->selection()->pixelSelection()->readBytes(
                    reinterpret_cast<quint8 *>(coverage.data()), area);
            }
            QCOMPARE(pixels + projection + coverage, reference.last());
        }
    }
}

void KisGpuDocumentViewTest::multibrushPresetsKeepNativeEditingHistoryAndKra_data()
{
    MultibrushEditingTest::addData();
}

void KisGpuDocumentViewTest::multibrushPresetsKeepNativeEditingHistoryAndKra()
{
    MultibrushEditingTest::run(m_freehandExecution);
}

void KisGpuDocumentViewTest::dynamicBrushPresetsKeepNativeEditingHistoryAndKra_data()
{
    DynamicBrushEditingTest::addData();
}

void KisGpuDocumentViewTest::dynamicBrushPresetsKeepNativeEditingHistoryAndKra()
{
    DynamicBrushEditingTest::run(m_freehandExecution);
}

KISTEST_MAIN(KisGpuDocumentViewTest)
#include "KisGpuDocumentViewTest.moc"
