/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_config.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KIS_CONFIG_SIGNATURE(method, signature) \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisConfig::method)), signature>)

template<typename Enum, std::size_t Size>
void compareSequentialValues(const std::array<Enum, Size> &values)
{
    for (std::size_t index = 0; index < values.size(); ++index) {
        QCOMPARE(int(values[index]), int(index));
    }
}

template<typename Config, typename = void>
struct CanReadExportMimeTypeWithoutArgument : std::false_type {
};

template<typename Config>
struct CanReadExportMimeTypeWithoutArgument<Config,
                                            std::void_t<decltype(std::declval<const Config &>().exportMimeType())>>
    : std::true_type {
};

template<typename Config, typename = void>
struct CanReadPasteFormatWithoutArgument : std::false_type {
};

template<typename Config>
struct CanReadPasteFormatWithoutArgument<Config, std::void_t<decltype(std::declval<const Config &>().pasteFormat())>>
    : std::true_type {
};
}

class KisConfigEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inputAndSamplerValuesRemainStable();
    void canvasSurfaceValuesRemainStable();
    void sessionAndColorConfigurationValuesRemainStable();
    void selectionActionBarValuesRemainStable();
    void displayAndAssistantValuesRemainStable();
    void navigationGestureSignaturesRemainStable();
    void inputProfileAndPointerBackendSignaturesRemainStable();
    void tabletEventFilteringSignaturesRemainStable();
    void stabilizerInputSignaturesRemainStable();
    void selectionModifierSignaturesRemainStable();
    void primaryCursorAppearanceSignaturesRemainStable();
    void primaryOutlineAppearanceSignaturesRemainStable();
    void eraserCursorAppearanceSignaturesRemainStable();
    void paintingOutlineVisibilitySignaturesRemainStable();
    void selectionOutlineRenderingSignaturesRemainStable();
    void presetBrowserPresentationSignaturesRemainStable();
    void favoritePaintResourcesSignaturesRemainStable();
    void paletteAndPresetPolicySignaturesRemainStable();
    void brushHudAndPalettePresentationSignaturesRemainStable();
    void scratchpadResourceSurfaceSignaturesRemainStable();
    void canvasCheckerboardSignaturesRemainStable();
    void pixelGridSignaturesRemainStable();
    void constructionGridSignaturesRemainStable();
    void guidesPresentationSignaturesRemainStable();
    void mdiCanvasBackgroundSignaturesRemainStable();
    void workspaceModeAndPrimaryChromeSignaturesRemainStable();
    void fullscreenWindowChromeSignaturesRemainStable();
    void fullscreenWorkspaceSurfaceSignaturesRemainStable();
    void workspaceTransientSurfaceSignaturesRemainStable();
    void workspaceAuxiliaryChromeSignaturesRemainStable();
    void layerPresentationTextAndThumbnailSignaturesRemainStable();
    void layerTreePresentationSignaturesRemainStable();
    void layerPropertyDialogSignaturesRemainStable();
    void selectionSurfaceSignaturesRemainStable();
    void singleChannelPresentationSignaturesRemainStable();
    void colorManagementWorkingAndImportSignaturesRemainStable();
    void colorManagementMonitorRoutingSignaturesRemainStable();
    void colorManagementOcioSignaturesRemainStable();
    void colorManagementPrinterOutputSignaturesRemainStable();
    void colorManagementEngineAndSelectorSignaturesRemainStable();
    void animationPlaybackPolicySignaturesRemainStable();
    void animationTimelineNavigationSignaturesRemainStable();
    void animationScrubbingAndAudioSignaturesRemainStable();
    void animationCacheAndMediaBackendSignaturesRemainStable();
    void animationFrameTransferSignaturesRemainStable();
    void defaultDocumentGeometrySignaturesRemainStable();
    void defaultDocumentColorSpaceSignaturesRemainStable();
    void defaultDocumentBackgroundAndLayerSignaturesRemainStable();
    void nativeDocumentPersistenceSignaturesRemainStable();
    void documentAutosaveAndUndoHistorySignaturesRemainStable();
    void canvasNavigationAndZoomSignaturesRemainStable();
    void canvasWorkspaceStateAndSplitSignaturesRemainStable();
    void pasteWorkflowSignaturesRemainStable();
    void toolOptionSurfaceSignaturesRemainStable();
    void editingToolBehaviorSignaturesRemainStable();
    void renderingVectorAndLodPolicySignaturesRemainStable();
    void renderingTexturePipelineSignaturesRemainStable();
    void renderingProjectionBackendSignaturesRemainStable();
    void renderingDiagnosticLoggingSignaturesRemainStable();
    void renderingBackendCompatibilitySignaturesRemainStable();
};

void KisConfigEnumContractTest::inputAndSamplerValuesRemainStable()
{
    static_assert(std::is_class_v<KisConfig>);
    static_assert(!std::is_default_constructible_v<KisConfig>);

    compareSequentialValues(std::array<KisConfig::TouchPainting, 3> {{
        KisConfig::TOUCH_PAINTING_AUTO,
        KisConfig::TOUCH_PAINTING_ENABLED,
        KisConfig::TOUCH_PAINTING_DISABLED,
    }});

    ASSERT_KIS_CONFIG_SIGNATURE(touchPainting,
                                KisConfig::TouchPainting (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTouchPainting,
                                void (KisConfig::*)(KisConfig::TouchPainting) const);
    ASSERT_KIS_CONFIG_SIGNATURE(disableTouchOnCanvas, bool (KisConfig::*)() const);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().touchPainting()),
                                 KisConfig::TouchPainting>);

    compareSequentialValues(std::array<KisConfig::ColorSamplerPreviewStyle, 6> {{
        KisConfig::ColorSamplerPreviewStyle::None,
        KisConfig::ColorSamplerPreviewStyle::Circle,
        KisConfig::ColorSamplerPreviewStyle::RectangleLeft,
        KisConfig::ColorSamplerPreviewStyle::RectangleRight,
        KisConfig::ColorSamplerPreviewStyle::RectangleAbove,
        KisConfig::ColorSamplerPreviewStyle::Count,
    }});

    ASSERT_KIS_CONFIG_SIGNATURE(colorSamplerPreviewStyle,
                                KisConfig::ColorSamplerPreviewStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setColorSamplerPreviewStyle,
                                void (KisConfig::*)(KisConfig::ColorSamplerPreviewStyle));
    ASSERT_KIS_CONFIG_SIGNATURE(colorSamplerPreviewCircleDiameter,
                                int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setColorSamplerPreviewCircleDiameter,
                                void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(colorSamplerPreviewCircleThickness,
                                qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setColorSamplerPreviewCircleThickness,
                                void (KisConfig::*)(qreal));
    ASSERT_KIS_CONFIG_SIGNATURE(colorSamplerPreviewCircleOutlineEnabled,
                                bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setColorSamplerPreviewCircleOutlineEnabled,
                                void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(colorSamplerPreviewCircleExtraCirclesEnabled,
                                bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setColorSamplerPreviewCircleExtraCirclesEnabled,
                                void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().colorSamplerPreviewStyle()),
                                 KisConfig::ColorSamplerPreviewStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().colorSamplerPreviewCircleDiameter()),
                                 int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().colorSamplerPreviewCircleThickness()),
                                 qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().colorSamplerPreviewCircleOutlineEnabled()),
                                 bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().colorSamplerPreviewCircleExtraCirclesEnabled()),
                                 bool>);
}

void KisConfigEnumContractTest::canvasSurfaceValuesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(enableCanvasSurfaceColorSpaceManagement,
                                bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEnableCanvasSurfaceColorSpaceManagement,
                                void (KisConfig::*)(bool));

    compareSequentialValues(std::array<KisConfig::CanvasSurfaceMode, 5> {{
        KisConfig::CanvasSurfaceMode::Preferred,
        KisConfig::CanvasSurfaceMode::Rec709g22,
        KisConfig::CanvasSurfaceMode::Rec709g10,
        KisConfig::CanvasSurfaceMode::Rec2020pq,
        KisConfig::CanvasSurfaceMode::Unmanaged,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(canvasSurfaceColorSpaceManagementMode,
                                KisConfig::CanvasSurfaceMode (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCanvasSurfaceColorSpaceManagementMode,
                                void (KisConfig::*)(KisConfig::CanvasSurfaceMode));

    compareSequentialValues(std::array<KisConfig::CanvasSurfaceBitDepthMode, 3> {{
        KisConfig::CanvasSurfaceBitDepthMode::DepthAuto,
        KisConfig::CanvasSurfaceBitDepthMode::Depth8Bit,
        KisConfig::CanvasSurfaceBitDepthMode::Depth10Bit,
    }});

    ASSERT_KIS_CONFIG_SIGNATURE(canvasSurfaceBitDepthMode,
                                KisConfig::CanvasSurfaceBitDepthMode (*)(QSettings *, bool));
    ASSERT_KIS_CONFIG_SIGNATURE(setCanvasSurfaceBitDepthMode,
                                void (*)(QSettings *, KisConfig::CanvasSurfaceBitDepthMode));
    ASSERT_KIS_CONFIG_SIGNATURE(canvasSurfaceBitDepthMode,
                                KisConfig::CanvasSurfaceBitDepthMode (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCanvasSurfaceBitDepthMode,
                                void (KisConfig::*)(KisConfig::CanvasSurfaceBitDepthMode));
    ASSERT_KIS_CONFIG_SIGNATURE(effectiveCanvasSurfaceBitDepthMode,
                                KisConfig::CanvasSurfaceBitDepthMode (KisConfig::*)(const QSurfaceFormat &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().enableCanvasSurfaceColorSpaceManagement()),
                                 bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().canvasSurfaceColorSpaceManagementMode()),
                                 KisConfig::CanvasSurfaceMode>);
    static_assert(std::is_same_v<decltype(KisConfig::canvasSurfaceBitDepthMode(static_cast<QSettings *>(nullptr))),
                                 KisConfig::CanvasSurfaceBitDepthMode>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().canvasSurfaceBitDepthMode()),
                                 KisConfig::CanvasSurfaceBitDepthMode>);
}

void KisConfigEnumContractTest::sessionAndColorConfigurationValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::SessionOnStartup, 3> {{
        KisConfig::SOS_BlankSession,
        KisConfig::SOS_PreviousSession,
        KisConfig::SOS_ShowSessionManager,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(sessionOnStartup,
                                KisConfig::SessionOnStartup (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSessionOnStartup,
                                void (KisConfig::*)(KisConfig::SessionOnStartup));

    compareSequentialValues(std::array<KisConfig::IconsInMenu, 3> {{
        KisConfig::IIM_Default,
        KisConfig::IIM_Yes,
        KisConfig::IIM_No,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(iconsInMenu,
                                KisConfig::IconsInMenu (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setIconsInMenu,
                                void (KisConfig::*)(KisConfig::IconsInMenu));

    compareSequentialValues(std::array<KisConfig::OcioColorManagementMode, 3> {{
        KisConfig::INTERNAL,
        KisConfig::OCIO_CONFIG,
        KisConfig::OCIO_ENVIRONMENT,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(ocioColorManagementMode,
                                KisConfig::OcioColorManagementMode (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOcioColorManagementMode,
                                void (KisConfig::*)(KisConfig::OcioColorManagementMode) const);

    compareSequentialValues(std::array<KisConfig::BackgroundStyle, 3> {{
        KisConfig::RASTER_LAYER,
        KisConfig::CANVAS_COLOR,
        KisConfig::FILL_LAYER,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(defaultBackgroundStyle,
                                KisConfig::BackgroundStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultBackgroundStyle,
                                void (KisConfig::*)(KisConfig::BackgroundStyle));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().sessionOnStartup()),
                                 KisConfig::SessionOnStartup>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().iconsInMenu()),
                                 KisConfig::IconsInMenu>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().ocioColorManagementMode()),
                                 KisConfig::OcioColorManagementMode>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defaultBackgroundStyle()),
                                 KisConfig::BackgroundStyle>);
}

void KisConfigEnumContractTest::selectionActionBarValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::SelectionActionsBarBehavior, 2> {{
        KisConfig::FreeFloating,
        KisConfig::Fixed,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(selectionActionBarBehavior,
                                KisConfig::SelectionActionsBarBehavior (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSelectionActionBarBehavior,
                                void (KisConfig::*)(KisConfig::SelectionActionsBarBehavior));

    compareSequentialValues(std::array<KisConfig::SelectionActionsBarPosition, 8> {{
        KisConfig::Bottom,
        KisConfig::BottomLeft,
        KisConfig::BottomRight,
        KisConfig::Left,
        KisConfig::Right,
        KisConfig::Top,
        KisConfig::TopLeft,
        KisConfig::TopRight,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(selectionActionBarPosition,
                                KisConfig::SelectionActionsBarPosition (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSelectionActionBarPosition,
                                void (KisConfig::*)(KisConfig::SelectionActionsBarPosition));

    compareSequentialValues(std::array<KisConfig::SelectionActionsBarOrientation, 2> {{
        KisConfig::Horizontal,
        KisConfig::Vertical,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(selectionActionBarOrientation,
                                KisConfig::SelectionActionsBarOrientation (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSelectionActionBarOrientation,
                                void (KisConfig::*)(KisConfig::SelectionActionsBarOrientation));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().selectionActionBarBehavior()),
                                 KisConfig::SelectionActionsBarBehavior>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().selectionActionBarPosition()),
                                 KisConfig::SelectionActionsBarPosition>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().selectionActionBarOrientation()),
                                 KisConfig::SelectionActionsBarOrientation>);
}

void KisConfigEnumContractTest::displayAndAssistantValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::RootSurfaceFormat, 3> {{
        KisConfig::BT709_G22,
        KisConfig::BT709_G10,
        KisConfig::BT2020_PQ,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(rootSurfaceFormat,
                                KisConfig::RootSurfaceFormat (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setRootSurfaceFormat,
                                void (KisConfig::*)(KisConfig::RootSurfaceFormat));
    ASSERT_KIS_CONFIG_SIGNATURE(rootSurfaceFormat,
                                KisConfig::RootSurfaceFormat (*)(QSettings *, bool));
    ASSERT_KIS_CONFIG_SIGNATURE(setRootSurfaceFormat,
                                void (*)(QSettings *, KisConfig::RootSurfaceFormat));

    compareSequentialValues(std::array<KisConfig::LayerInfoTextStyle, 4> {{
        KisConfig::INFOTEXT_NONE,
        KisConfig::INFOTEXT_SIMPLE,
        KisConfig::INFOTEXT_BALANCED,
        KisConfig::INFOTEXT_DETAILED,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(layerInfoTextStyle,
                                KisConfig::LayerInfoTextStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLayerInfoTextStyle,
                                void (KisConfig::*)(KisConfig::LayerInfoTextStyle));

    compareSequentialValues(std::array<KisConfig::AssistantsDrawMode, 3> {{
        KisConfig::ASSISTANTS_DRAW_MODE_DIRECT,
        KisConfig::ASSISTANTS_DRAW_MODE_PIXMAP_CACHE,
        KisConfig::ASSISTANTS_DRAW_MODE_LARGE_PIXMAP_CACHE,
    }});
    ASSERT_KIS_CONFIG_SIGNATURE(assistantsDrawMode,
                                KisConfig::AssistantsDrawMode (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAssistantsDrawMode,
                                void (KisConfig::*)(KisConfig::AssistantsDrawMode));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().rootSurfaceFormat()),
                                 KisConfig::RootSurfaceFormat>);
    static_assert(std::is_same_v<decltype(KisConfig::rootSurfaceFormat(static_cast<QSettings *>(nullptr))),
                                 KisConfig::RootSurfaceFormat>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().layerInfoTextStyle()),
                                 KisConfig::LayerInfoTextStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().assistantsDrawMode()),
                                 KisConfig::AssistantsDrawMode>);
}

void KisConfigEnumContractTest::navigationGestureSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(clicklessSpacePan, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setClicklessSpacePan, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(kineticScrollingEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setKineticScrollingEnabled, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(kineticScrollingGesture, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setKineticScrollingGesture, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(kineticScrollingHiddenScrollbars, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setKineticScrollingHideScrollbars, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(kineticScrollingSensitivity, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setKineticScrollingSensitivity, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(longPressEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLongPressEnabled, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().clicklessSpacePan()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().kineticScrollingEnabled()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().kineticScrollingGesture()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().kineticScrollingHiddenScrollbars()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().kineticScrollingSensitivity()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().longPressEnabled()), bool>);
}

void KisConfigEnumContractTest::inputProfileAndPointerBackendSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(currentInputProfile, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCurrentInputProfile, void (KisConfig::*)(const QString &));
    ASSERT_KIS_CONFIG_SIGNATURE(pressureTabletCurve, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPressureTabletCurve, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(useWin8PointerInput, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseWin8PointerInput, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useWin8PointerInputNoApp, bool (*)(QSettings *, bool));
    ASSERT_KIS_CONFIG_SIGNATURE(setUseWin8PointerInputNoApp, void (*)(QSettings *, bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useRightMiddleTabletButtonWorkaround, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseRightMiddleTabletButtonWorkaround, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().currentInputProfile()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().pressureTabletCurve()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useWin8PointerInput()), bool>);
    static_assert(
        std::is_same_v<decltype(KisConfig::useWin8PointerInputNoApp(static_cast<QSettings *>(nullptr))), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().useRightMiddleTabletButtonWorkaround()), bool>);
}

void KisConfigEnumContractTest::tabletEventFilteringSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(tabletEventsDelay, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTabletEventsDelay, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(trackTabletEventLatency, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTrackTabletEventLatency, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(ignoreHighFunctionKeys, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setIgnoreHighFunctionKeys, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(shouldEatDriverShortcuts, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(testingAcceptCompressedTabletEvents, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTestingAcceptCompressedTabletEvents, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(testingCompressBrushEvents, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTestingCompressBrushEvents, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(workaroundX11SmoothPressureSteps, int (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().tabletEventsDelay()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().trackTabletEventLatency()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().ignoreHighFunctionKeys()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().shouldEatDriverShortcuts()), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().testingAcceptCompressedTabletEvents()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().testingCompressBrushEvents()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().workaroundX11SmoothPressureSteps()), int>);
}

void KisConfigEnumContractTest::stabilizerInputSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(stabilizerDelayedPaint, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setStabilizerDelayedPaint, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(stabilizerSampleSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setStabilizerSampleSize, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().stabilizerDelayedPaint()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().stabilizerSampleSize()), int>);
}

void KisConfigEnumContractTest::selectionModifierSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(switchSelectionCtrlAlt, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSwitchSelectionCtrlAlt, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().switchSelectionCtrlAlt()), bool>);
}

void KisConfigEnumContractTest::primaryCursorAppearanceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(newCursorStyle, CursorStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setNewCursorStyle, void (KisConfig::*)(CursorStyle));
    ASSERT_KIS_CONFIG_SIGNATURE(getCursorMainColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCursorMainColor, void (KisConfig::*)(const QColor &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().newCursorStyle()), CursorStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getCursorMainColor()), QColor>);
}

void KisConfigEnumContractTest::primaryOutlineAppearanceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(newOutlineStyle, OutlineStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setNewOutlineStyle, void (KisConfig::*)(OutlineStyle));
    ASSERT_KIS_CONFIG_SIGNATURE(lastUsedOutlineStyle, OutlineStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLastUsedOutlineStyle, void (KisConfig::*)(OutlineStyle));
    ASSERT_KIS_CONFIG_SIGNATURE(outlineSizeMinimum, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOutlineSizeMinimum, void (KisConfig::*)(qreal) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().newOutlineStyle()), OutlineStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().lastUsedOutlineStyle()), OutlineStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().outlineSizeMinimum()), qreal>);
}

void KisConfigEnumContractTest::eraserCursorAppearanceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(separateEraserCursor, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSeparateEraserCursor, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(eraserCursorStyle, CursorStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEraserCursorStyle, void (KisConfig::*)(CursorStyle));
    ASSERT_KIS_CONFIG_SIGNATURE(getEraserCursorMainColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEraserCursorMainColor, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(eraserOutlineStyle, OutlineStyle (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEraserOutlineStyle, void (KisConfig::*)(OutlineStyle));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().separateEraserCursor()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().eraserCursorStyle()), CursorStyle>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getEraserCursorMainColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().eraserOutlineStyle()), OutlineStyle>);
}

void KisConfigEnumContractTest::paintingOutlineVisibilitySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(showOutlineWhilePainting, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowOutlineWhilePainting, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(forceAlwaysFullSizedOutline, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setForceAlwaysFullSizedOutline, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showEraserOutlineWhilePainting, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowEraserOutlineWhilePainting, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(forceAlwaysFullSizedEraserOutline, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setForceAlwaysFullSizedEraserOutline, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showOutlineWhilePainting()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().forceAlwaysFullSizedOutline()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showEraserOutlineWhilePainting()), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().forceAlwaysFullSizedEraserOutline()), bool>);
}

void KisConfigEnumContractTest::selectionOutlineRenderingSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(antialiasSelectionOutline, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAntialiasSelectionOutline, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().antialiasSelectionOutline()), bool>);
}

void KisConfigEnumContractTest::presetBrowserPresentationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(presetChooserViewMode, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPresetChooserViewMode, void (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(presetIconSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPresetIconSize, void (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(presetStripVisible, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPresetStripVisible, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(paintopPopupDetached, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPaintopPopupDetached, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().presetChooserViewMode()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().presetIconSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().presetStripVisible()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().paintopPopupDetached()), bool>);
}

void KisConfigEnumContractTest::favoritePaintResourcesSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(favoriteCompositeOps, QStringList (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setFavoriteCompositeOps, void (KisConfig::*)(const QStringList &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(favoritePresets, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setFavoritePresets, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().favoriteCompositeOps()), QStringList>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().favoritePresets()), int>);
}

void KisConfigEnumContractTest::paletteAndPresetPolicySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(defaultPalette, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultPalette, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(forcePaletteColors, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setForcePaletteColors, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useDirtyPresets, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseDirtyPresets, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defaultPalette()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().forcePaletteColors()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useDirtyPresets()), bool>);
}

void KisConfigEnumContractTest::brushHudAndPalettePresentationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(showBrushHud, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowBrushHud, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(brushHudSetting, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setBrushHudSetting, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showPaletteBottomBar, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowPaletteBottomBar, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showBrushHud()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().brushHudSetting()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showPaletteBottomBar()), bool>);
}

void KisConfigEnumContractTest::scratchpadResourceSurfaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(scratchpadVisible, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setScratchpadVisible, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().scratchpadVisible()), bool>);
}

void KisConfigEnumContractTest::canvasCheckerboardSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(canvasBorderColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCanvasBorderColor, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(checkSize, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCheckSize, void (KisConfig::*)(qint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(checkersColor1, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCheckersColor1, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(checkersColor2, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCheckersColor2, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(scrollCheckers, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setScrollingCheckers, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().canvasBorderColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().checkSize()), qint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().checkersColor1()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().checkersColor2()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().scrollCheckers()), bool>);
}

void KisConfigEnumContractTest::pixelGridSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(pixelGridEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(enablePixelGrid, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getPixelGridColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPixelGridColor, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getPixelGridDrawingThreshold, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPixelGridDrawingThreshold, void (KisConfig::*)(qreal) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().pixelGridEnabled()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getPixelGridColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getPixelGridDrawingThreshold()), qreal>);
}

void KisConfigEnumContractTest::constructionGridSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(getDefaultGridSpacing, QPoint (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultGridSpacing, void (KisConfig::*)(QPoint));
    ASSERT_KIS_CONFIG_SIGNATURE(getGridMainStyle, quint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridMainStyle, void (KisConfig::*)(quint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getGridMainColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridMainColor, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getGridSubdivisionStyle, quint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridSubdivisionStyle, void (KisConfig::*)(quint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getGridSubdivisionColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridSubdivisionColor, void (KisConfig::*)(const QColor &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getGridIsoVerticalStyle, quint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridIsoVerticalStyle, void (KisConfig::*)(quint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getGridIsoVerticalColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGridIsoVerticalColor, void (KisConfig::*)(const QColor &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getDefaultGridSpacing()), QPoint>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridMainStyle()), quint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridMainColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridSubdivisionStyle()), quint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridSubdivisionColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridIsoVerticalStyle()), quint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getGridIsoVerticalColor()), QColor>);
}

void KisConfigEnumContractTest::guidesPresentationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(guidesLineStyle, quint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGuidesLineStyle, void (KisConfig::*)(quint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(guidesColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setGuidesColor, void (KisConfig::*)(const QColor &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().guidesLineStyle()), quint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().guidesColor()), QColor>);
}

void KisConfigEnumContractTest::mdiCanvasBackgroundSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(getMDIBackgroundColor, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setMDIBackgroundColor, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getMDIBackgroundImage, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setMDIBackgroundImage, void (KisConfig::*)(const QString &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getMDIBackgroundColor()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().getMDIBackgroundImage()), QString>);
}

void KisConfigEnumContractTest::workspaceModeAndPrimaryChromeSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(fullscreenMode, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setFullscreenMode, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showDockers, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowDockers, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showStatusBar, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowStatusBar, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().fullscreenMode()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showDockers()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showStatusBar()), bool>);
}

void KisConfigEnumContractTest::fullscreenWindowChromeSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(hideMenuFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideMenuFullscreen, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(hideStatusbarFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideStatusbarFullscreen, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(hideTitlebarFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideTitlebarFullscreen, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(hideToolbarFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideToolbarFullscreen, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideMenuFullscreen()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideStatusbarFullscreen()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideTitlebarFullscreen()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideToolbarFullscreen()), bool>);
}

void KisConfigEnumContractTest::fullscreenWorkspaceSurfaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(hideDockersFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideDockersFullscreen, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(hideScrollbarsFullscreen, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideScrollbarsFullscreen, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideDockersFullscreen()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideScrollbarsFullscreen()), bool>);
}

void KisConfigEnumContractTest::workspaceTransientSurfaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(hideScrollbars, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHideScrollbars, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(hidePopups, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHidePopups, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hideScrollbars()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().hidePopups()), bool>);
}

void KisConfigEnumContractTest::workspaceAuxiliaryChromeSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(showDockerTitleBars, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowDockerTitleBars, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showCanvasMessages, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowCanvasMessages, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(showRulers, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowRulers, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showDockerTitleBars()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showCanvasMessages()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showRulers()), bool>);
}

void KisConfigEnumContractTest::layerPresentationTextAndThumbnailSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(layerInfoTextOpacity, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLayerInfoTextOpacity, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(useInlineLayerInfoText, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseInlineLayerInfoText, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(layerThumbnailSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLayerThumbnailSize, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(layerThumbnailGenerationTimeout, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLayerThumbnailGenerationTimeout, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().layerInfoTextOpacity()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useInlineLayerInfoText()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().layerThumbnailSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().layerThumbnailGenerationTimeout()), int>);
}

void KisConfigEnumContractTest::layerTreePresentationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(layerTreeIndentation, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLayerTreeIndentation, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(useLayerSelectionCheckbox, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseLayerSelectionCheckbox, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(showRootLayer, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowRootLayer, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().layerTreeIndentation()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useLayerSelectionCheckbox()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showRootLayer()), bool>);
}

void KisConfigEnumContractTest::layerPropertyDialogSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(convertLayerColorSpaceInProperties, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setConvertLayerColorSpaceInProperties, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(showFilterGallery, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowFilterGallery, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showFilterGalleryLayerMaskDialog, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowFilterGalleryLayerMaskDialog, void (KisConfig::*)(bool) const);

    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().convertLayerColorSpaceInProperties()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showFilterGallery()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showFilterGalleryLayerMaskDialog()), bool>);
}

void KisConfigEnumContractTest::selectionSurfaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(selectionActionBar, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSelectionActionBar, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(selectionViewSizeMinimum, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSelectionViewSizeMinimum, void (KisConfig::*)(qreal) const);
    ASSERT_KIS_CONFIG_SIGNATURE(showGlobalSelection, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowGlobalSelection, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().selectionActionBar()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().selectionViewSizeMinimum()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showGlobalSelection()), bool>);
}

void KisConfigEnumContractTest::singleChannelPresentationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(showSingleChannelAsColor, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setShowSingleChannelAsColor, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().showSingleChannelAsColor()), bool>);
}

void KisConfigEnumContractTest::colorManagementWorkingAndImportSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(useDefaultColorSpace, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseDefaultColorSpace, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(workingColorSpace, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setWorkingColorSpace, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(importProfile, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setImportProfile, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(convertToImageColorspaceOnImport, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setConvertToImageColorspaceOnImport, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useDefaultColorSpace()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().workingColorSpace()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().importProfile()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().convertToImageColorspaceOnImport()), bool>);
}

void KisConfigEnumContractTest::colorManagementMonitorRoutingSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(monitorForScreen, QString (KisConfig::*)(int, const QString &, bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setMonitorForScreen, void (KisConfig::*)(int, const QString &));
    ASSERT_KIS_CONFIG_SIGNATURE(monitorProfile, QString (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setMonitorProfile, void (KisConfig::*)(int, const QString &, bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(getScreenStringIdentfier, const QString (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(monitorRenderIntent, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setRenderIntent, void (KisConfig::*)(qint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(useSystemMonitorProfile, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseSystemMonitorProfile, void (KisConfig::*)(bool) const);

    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().monitorForScreen(0, std::declval<const QString &>())),
                       QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().monitorRenderIntent()), qint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useSystemMonitorProfile()), bool>);
}

void KisConfigEnumContractTest::colorManagementOcioSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(useOcio, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseOcio, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(ocioConfiguration, KisOcioConfiguration (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOcioConfiguration, void (KisConfig::*)(const KisOcioConfiguration &));
    ASSERT_KIS_CONFIG_SIGNATURE(ocioLutEdgeSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOcioLutEdgeSize, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(ocioLockColorVisualRepresentation, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOcioLockColorVisualRepresentation, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useOcio()), bool>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().ocioConfiguration()), KisOcioConfiguration>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().ocioLutEdgeSize()), int>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().ocioLockColorVisualRepresentation()), bool>);
}

void KisConfigEnumContractTest::colorManagementPrinterOutputSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(printerColorSpace, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPrinterColorSpace, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(printerProfile, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPrinterProfile, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(useBlackPointCompensation, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseBlackPointCompensation, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().printerColorSpace()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().printerProfile()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useBlackPointCompensation()), bool>);
}

void KisConfigEnumContractTest::colorManagementEngineAndSelectorSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(allowLCMSOptimization, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAllowLCMSOptimization, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(customColorSelectorColorSpace, const KoColorSpace *(KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCustomColorSelectorColorSpace, void (KisConfig::*)(const KoColorSpace *));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().allowLCMSOptimization()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().customColorSelectorColorSpace()),
                                 const KoColorSpace *>);
}

void KisConfigEnumContractTest::animationPlaybackPolicySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(animationPlaybackBackend, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAnimationPlaybackBackend, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(animationDropFrames, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAnimationDropFrames, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(adaptivePlaybackRange, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAdaptivePlaybackRange, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().animationPlaybackBackend()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().animationDropFrames()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().adaptivePlaybackRange()), bool>);
}

void KisConfigEnumContractTest::animationTimelineNavigationSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(autoPinLayersToTimeline, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAutoPinLayersToTimeline, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(autoZoomTimelineToPlaybackRange, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAutoZoomTimelineToPlaybackRange, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(timelineZoom, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTimelineZoom, void (KisConfig::*)(qreal));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().autoPinLayersToTimeline()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().autoZoomTimelineToPlaybackRange()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().timelineZoom()), qreal>);
}

void KisConfigEnumContractTest::animationScrubbingAndAudioSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(scrubbingUpdatesDelay, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setScrubbingUpdatesDelay, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(scrubbingAudioUpdatesDelay, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setScrubbingAudioUpdatesDelay, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(audioOffsetTolerance, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAudioOffsetTolerance, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().scrubbingUpdatesDelay()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().scrubbingAudioUpdatesDelay()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().audioOffsetTolerance()), int>);
}

void KisConfigEnumContractTest::animationCacheAndMediaBackendSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(calculateAnimationCacheInBackground, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCalculateAnimationCacheInBackground, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(ffmpegLocation, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setFFMpegLocation, void (KisConfig::*)(const QString &));

    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().calculateAnimationCacheInBackground()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().ffmpegLocation()), QString>);
}

void KisConfigEnumContractTest::animationFrameTransferSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(trimFramesImport, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTrimFramesImport, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(exportMimeType, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setExportMimeType, void (KisConfig::*)(const QString &));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().trimFramesImport()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().exportMimeType(false)), QString>);
    static_assert(!CanReadExportMimeTypeWithoutArgument<KisConfig>::value);
}

void KisConfigEnumContractTest::defaultDocumentGeometrySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(defImageHeight, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defImageHeight, void (KisConfig::*)(qint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defImageResolution, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defImageResolution, void (KisConfig::*)(qreal) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defImageWidth, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defImageWidth, void (KisConfig::*)(qint32) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defImageHeight()), qint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defImageResolution()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defImageWidth()), qint32>);
}

void KisConfigEnumContractTest::defaultDocumentColorSpaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(defColorModel, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defColorModel, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defColorProfile, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defColorProfile, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(defaultColorDepth, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultColorDepth, void (KisConfig::*)(const QString &) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defColorModel()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defColorProfile()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defaultColorDepth()), QString>);
}

void KisConfigEnumContractTest::defaultDocumentBackgroundAndLayerSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(defaultBackgroundColor, QColor (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultBackgroundColor, void (KisConfig::*)(const QColor &));
    ASSERT_KIS_CONFIG_SIGNATURE(defaultBackgroundOpacity, quint8 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDefaultBackgroundOpacity, void (KisConfig::*)(quint8));
    ASSERT_KIS_CONFIG_SIGNATURE(numDefaultLayers, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setNumDefaultLayers, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defaultBackgroundColor()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().defaultBackgroundOpacity()), quint8>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().numDefaultLayers()), int>);
}

void KisConfigEnumContractTest::nativeDocumentPersistenceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(backupFile, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setBackupFile, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(compressKra, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCompressKra, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(trimKra, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setTrimKra, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useZip64, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseZip64, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().backupFile()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().compressKra()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().trimKra()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useZip64()), bool>);
}

void KisConfigEnumContractTest::documentAutosaveAndUndoHistorySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(autoSaveInterval, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAutoSaveInterval, void (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(cumulativeUndoData, KisCumulativeUndoData (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCumulativeUndoData, void (KisConfig::*)(KisCumulativeUndoData));
    ASSERT_KIS_CONFIG_SIGNATURE(setCumulativeUndoRedo, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(setUndoEnabled, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUndoStackLimit, void (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(undoEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(undoStackLimit, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(useCumulativeUndoRedo, bool (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().autoSaveInterval()), int>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisConfig &>().cumulativeUndoData()), KisCumulativeUndoData>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().undoEnabled()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().undoStackLimit()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useCumulativeUndoRedo()), bool>);
}

void KisConfigEnumContractTest::canvasNavigationAndZoomSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(rulersTrackMouse, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setRulersTrackMouse, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(scrollbarZoomEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setScrollbarZoomEnabled, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(vastScrolling, qreal (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setVastScrolling, void (KisConfig::*)(qreal) const);
    ASSERT_KIS_CONFIG_SIGNATURE(zoomHorizontal, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setZoomHorizontal, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(zoomMarginSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setZoomMarginSize, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(zoomSteps, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setZoomSteps, void (KisConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().rulersTrackMouse()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().scrollbarZoomEnabled()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().vastScrolling()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().zoomHorizontal()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().zoomMarginSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().zoomSteps()), int>);
}

void KisConfigEnumContractTest::canvasWorkspaceStateAndSplitSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(canvasState, QString (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setCanvasState, void (KisConfig::*)(const QString &) const);
    ASSERT_KIS_CONFIG_SIGNATURE(horizontalSplitLines, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setHorizontalSplitLines, void (KisConfig::*)(int) const);
    ASSERT_KIS_CONFIG_SIGNATURE(verticalSplitLines, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setVerticalSplitLines, void (KisConfig::*)(int) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().canvasState()), QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().horizontalSplitLines()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().verticalSplitLines()), int>);
}

void KisConfigEnumContractTest::pasteWorkflowSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(activateTransformToolAfterPaste, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setActivateTransformToolAfterPaste, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(pasteBehaviour, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPasteBehaviour, void (KisConfig::*)(qint32) const);
    ASSERT_KIS_CONFIG_SIGNATURE(pasteFormat, qint32 (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setPasteFormat, void (KisConfig::*)(qint32));
    ASSERT_KIS_CONFIG_SIGNATURE(renamePastedLayers, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setRenamePastedLayers, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().activateTransformToolAfterPaste()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().pasteBehaviour()), qint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().pasteFormat(false)), qint32>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().renamePastedLayers()), bool>);
    static_assert(!CanReadPasteFormatWithoutArgument<KisConfig>::value);
}

void KisConfigEnumContractTest::toolOptionSurfaceSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(sliderLabels, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setSliderLabels, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(toolOptionsInDocker, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setToolOptionsInDocker, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(toolOptionsPopupDetached, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setToolOptionsPopupDetached, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(toolbarSlider, QString (KisConfig::*)(int, bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setToolbarSlider, void (KisConfig::*)(int, const QString &));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().sliderLabels()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().toolOptionsInDocker()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().toolOptionsPopupDetached()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().toolbarSlider(0)), QString>);
}

void KisConfigEnumContractTest::editingToolBehaviorSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(antialiasCurves, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAntialiasCurves, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(autoSmoothBezierCurves, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setAutoSmoothBezierCurves, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useEraserBrushOpacity, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseEraserBrushOpacity, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(useEraserBrushSize, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseEraserBrushSize, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().antialiasCurves()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().autoSmoothBezierCurves()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useEraserBrushOpacity()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useEraserBrushSize()), bool>);
}

void KisConfigEnumContractTest::renderingVectorAndLodPolicySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(disableAVXOptimizations, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDisableAVXOptimizations, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(disableVectorOptimizations, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setDisableVectorOptimizations, void (KisConfig::*)(bool));
    ASSERT_KIS_CONFIG_SIGNATURE(levelOfDetailEnabled, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setLevelOfDetailEnabled, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().disableAVXOptimizations()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().disableVectorOptimizations()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().levelOfDetailEnabled()), bool>);
}

void KisConfigEnumContractTest::renderingTexturePipelineSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(numMipmapLevels, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(openGLFilteringMode, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setOpenGLFilteringMode, void (KisConfig::*)(int));
    ASSERT_KIS_CONFIG_SIGNATURE(openGLTextureSize, int (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(textureOverlapBorder, int (KisConfig::*)() const);
    ASSERT_KIS_CONFIG_SIGNATURE(useOpenGLTextureBuffer, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseOpenGLTextureBuffer, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().numMipmapLevels()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().openGLFilteringMode()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().openGLTextureSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useOpenGLTextureBuffer()), bool>);
}

void KisConfigEnumContractTest::renderingProjectionBackendSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(useOpenGL, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(disableOpenGL, void (KisConfig::*)() const);
    ASSERT_KIS_CONFIG_SIGNATURE(useProjections, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setUseProjections, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useOpenGL()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().useProjections()), bool>);
}

void KisConfigEnumContractTest::renderingDiagnosticLoggingSignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(enableBrushSpeedLogging, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEnableBrushSpeedLogging, void (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(enableOpenGLFramerateLogging, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(setEnableOpenGLFramerateLogging, void (KisConfig::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().enableBrushSpeedLogging()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().enableOpenGLFramerateLogging()), bool>);
}

void KisConfigEnumContractTest::renderingBackendCompatibilitySignaturesRemainStable()
{
    ASSERT_KIS_CONFIG_SIGNATURE(forceOpenGLFenceWorkaround, bool (KisConfig::*)(bool) const);
    ASSERT_KIS_CONFIG_SIGNATURE(preferXcbEglProvider, bool (KisConfig::*)(bool) const);
    static_assert(
        std::is_same_v<decltype(static_cast<bool (*)(const QSettings *, bool)>(&KisConfig::preferXcbEglProvider)),
                       bool (*)(const QSettings *, bool)>);
    ASSERT_KIS_CONFIG_SIGNATURE(setPreferXcbEglProvider, void (KisConfig::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().forceOpenGLFenceWorkaround()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisConfig &>().preferXcbEglProvider()), bool>);
    static_assert(
        std::is_same_v<decltype(KisConfig::preferXcbEglProvider(static_cast<const QSettings *>(nullptr))), bool>);
}

#undef ASSERT_KIS_CONFIG_SIGNATURE

QTEST_GUILESS_MAIN(KisConfigEnumContractTest)

#include "KisConfigEnumContractTest.moc"
