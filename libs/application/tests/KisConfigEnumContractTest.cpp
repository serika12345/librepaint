/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_config.h>

#include <QTest>

#include <array>

namespace
{
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
    compareSequentialValues(std::array<KisConfig::TouchPainting, 3> {{
        KisConfig::TOUCH_PAINTING_AUTO,
        KisConfig::TOUCH_PAINTING_ENABLED,
        KisConfig::TOUCH_PAINTING_DISABLED,
    }});
    compareSequentialValues(std::array<KisConfig::ColorSamplerPreviewStyle, 6> {{
        KisConfig::ColorSamplerPreviewStyle::None,
        KisConfig::ColorSamplerPreviewStyle::Circle,
        KisConfig::ColorSamplerPreviewStyle::RectangleLeft,
        KisConfig::ColorSamplerPreviewStyle::RectangleRight,
        KisConfig::ColorSamplerPreviewStyle::RectangleAbove,
        KisConfig::ColorSamplerPreviewStyle::Count,
    }});
}

void KisConfigEnumContractTest::canvasSurfaceValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::CanvasSurfaceMode, 5> {{
        KisConfig::CanvasSurfaceMode::Preferred,
        KisConfig::CanvasSurfaceMode::Rec709g22,
        KisConfig::CanvasSurfaceMode::Rec709g10,
        KisConfig::CanvasSurfaceMode::Rec2020pq,
        KisConfig::CanvasSurfaceMode::Unmanaged,
    }});
    compareSequentialValues(std::array<KisConfig::CanvasSurfaceBitDepthMode, 3> {{
        KisConfig::CanvasSurfaceBitDepthMode::DepthAuto,
        KisConfig::CanvasSurfaceBitDepthMode::Depth8Bit,
        KisConfig::CanvasSurfaceBitDepthMode::Depth10Bit,
    }});
}

void KisConfigEnumContractTest::sessionAndColorConfigurationValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::SessionOnStartup, 3> {{
        KisConfig::SOS_BlankSession,
        KisConfig::SOS_PreviousSession,
        KisConfig::SOS_ShowSessionManager,
    }});
    compareSequentialValues(std::array<KisConfig::IconsInMenu, 3> {{
        KisConfig::IIM_Default,
        KisConfig::IIM_Yes,
        KisConfig::IIM_No,
    }});
    compareSequentialValues(std::array<KisConfig::OcioColorManagementMode, 3> {{
        KisConfig::INTERNAL,
        KisConfig::OCIO_CONFIG,
        KisConfig::OCIO_ENVIRONMENT,
    }});
    compareSequentialValues(std::array<KisConfig::BackgroundStyle, 3> {{
        KisConfig::RASTER_LAYER,
        KisConfig::CANVAS_COLOR,
        KisConfig::FILL_LAYER,
    }});
}

void KisConfigEnumContractTest::selectionActionBarValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::SelectionActionsBarBehavior, 2> {{
        KisConfig::FreeFloating,
        KisConfig::Fixed,
    }});
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
    compareSequentialValues(std::array<KisConfig::SelectionActionsBarOrientation, 2> {{
        KisConfig::Horizontal,
        KisConfig::Vertical,
    }});
}

void KisConfigEnumContractTest::displayAndAssistantValuesRemainStable()
{
    compareSequentialValues(std::array<KisConfig::RootSurfaceFormat, 3> {{
        KisConfig::BT709_G22,
        KisConfig::BT709_G10,
        KisConfig::BT2020_PQ,
    }});
    compareSequentialValues(std::array<KisConfig::LayerInfoTextStyle, 4> {{
        KisConfig::INFOTEXT_NONE,
        KisConfig::INFOTEXT_SIMPLE,
        KisConfig::INFOTEXT_BALANCED,
        KisConfig::INFOTEXT_DETAILED,
    }});
    compareSequentialValues(std::array<KisConfig::AssistantsDrawMode, 3> {{
        KisConfig::ASSISTANTS_DRAW_MODE_DIRECT,
        KisConfig::ASSISTANTS_DRAW_MODE_PIXMAP_CACHE,
        KisConfig::ASSISTANTS_DRAW_MODE_LARGE_PIXMAP_CACHE,
    }});
}

QTEST_GUILESS_MAIN(KisConfigEnumContractTest)

#include "KisConfigEnumContractTest.moc"
