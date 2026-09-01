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

#undef ASSERT_KIS_CONFIG_SIGNATURE

QTEST_GUILESS_MAIN(KisConfigEnumContractTest)

#include "KisConfigEnumContractTest.moc"
