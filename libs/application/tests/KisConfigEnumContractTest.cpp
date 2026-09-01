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

#undef ASSERT_KIS_CONFIG_SIGNATURE

QTEST_GUILESS_MAIN(KisConfigEnumContractTest)

#include "KisConfigEnumContractTest.moc"
