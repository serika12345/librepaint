/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISGPUDOCUMENTVIEWTEST_H
#define KISGPUDOCUMENTVIEWTEST_H
#include <KisPaintingEngineTestScope.h>
#include <array>
#include "KisGpuMultibrushEditingTest.h"
#include "KisGpuRegionFillEditingTest.h"
#include "KisGpuColorFilterEditingTest.h"
#include "KisGpuFilterNodeEditingTest.h"
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

inline auto fixedBrushPointerTiming(bool enabled)
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

inline void exchangeRgbaBgra(QByteArray &bytes, int pixelBytes)
{
    const int channelBytes = pixelBytes / 4;
    for (qsizetype pixel = 0; pixel < bytes.size(); pixel += pixelBytes)
        for (int byte = 0; byte < channelBytes; ++byte)
            std::swap(bytes[pixel + byte], bytes[pixel + 2 * channelBytes + byte]);
}

inline QByteArray nativeClipboardMask(const KoColorSpace *cs, QByteArray source, const QByteArray &coverage, QSize size)
{
    exchangeRgbaBgra(source, cs->pixelSize());
    cs->applyAlphaU8Mask(reinterpret_cast<quint8 *>(source.data()),
                         reinterpret_cast<const quint8 *>(coverage.constData()), size.width() * size.height());
    exchangeRgbaBgra(source, cs->pixelSize());
    return source;
}

inline QByteArray nativeClearSelection(const KoColorSpace *cs, QByteArray source, const QByteArray &coverage, QSize size)
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
    void colorFilterActionsKeepNativeEditing_data();
    void colorFilterActionsKeepNativeEditing();
    void convolutionFiltersKeepNativeEditing_data();
    void convolutionFiltersKeepNativeEditing();
    void filterNodesKeepNativeEditing_data();
    void filterNodesKeepNativeEditing();
    void convolutionFilterNodesKeepNativeEditing_data();
    void convolutionFilterNodesKeepNativeEditing();
    void convolutionChainsKeepNativeProjection_data();
    void convolutionChainsKeepNativeProjection();
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

#endif
