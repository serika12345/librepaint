/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <application/kis_config.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include <array>
#include <utility>

// Compatibility requirement: Existing user kritarc and kritadisplayrc settings depend on the stored KisConfig mode values and names.

namespace
{

template<typename Enum, std::size_t Size>
void comparePersistedIntegerValues(const std::array<Enum, Size> &values)
{
    for (std::size_t index = 0; index < values.size(); ++index) {
        QCOMPARE(int(values[index]), int(index));
    }
}

class ScopedConfigEntry final
{
public:
    ScopedConfigEntry(KConfigGroup group, QString key)
        : m_group(std::move(group))
        , m_key(std::move(key))
        , m_hadEntry(m_group.hasKey(m_key))
        , m_originalValue(m_group.readEntry(m_key, QString()))
    {
    }

    ~ScopedConfigEntry()
    {
        if (m_hadEntry) {
            m_group.writeEntry(m_key, m_originalValue);
        } else {
            m_group.deleteEntry(m_key);
        }
        m_group.sync();
    }

private:
    KConfigGroup m_group;
    QString m_key;
    bool m_hadEntry;
    QString m_originalValue;
};

} // namespace

class KisConfigCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void persistedIntegerModesRemainCompatible();
    void canvasSurfaceModeNamesRoundTrip();
    void displayFormatNamesRoundTrip();
};

void KisConfigCompatibilityTest::persistedIntegerModesRemainCompatible()
{
    /*
     * Consumer: Existing user kritarc settings and the input, canvas, and startup components that read them.
     * Operation: Decodes integer mode values written by earlier LibrePaint versions.
     * Observable result: Each stored value continues to select the same input, startup, color, selection, layer, and assistant mode.
     * Failure impact: Upgrading LibrePaint can reinterpret a user's saved preference and change the application's behavior.
     */
    comparePersistedIntegerValues(std::array<KisConfig::TouchPainting, 3> {{
        KisConfig::TOUCH_PAINTING_AUTO,
        KisConfig::TOUCH_PAINTING_ENABLED,
        KisConfig::TOUCH_PAINTING_DISABLED,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::ColorSamplerPreviewStyle, 5> {{
        KisConfig::ColorSamplerPreviewStyle::None,
        KisConfig::ColorSamplerPreviewStyle::Circle,
        KisConfig::ColorSamplerPreviewStyle::RectangleLeft,
        KisConfig::ColorSamplerPreviewStyle::RectangleRight,
        KisConfig::ColorSamplerPreviewStyle::RectangleAbove,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::SessionOnStartup, 3> {{
        KisConfig::SOS_BlankSession,
        KisConfig::SOS_PreviousSession,
        KisConfig::SOS_ShowSessionManager,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::IconsInMenu, 3> {{
        KisConfig::IIM_Default,
        KisConfig::IIM_Yes,
        KisConfig::IIM_No,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::OcioColorManagementMode, 3> {{
        KisConfig::INTERNAL,
        KisConfig::OCIO_CONFIG,
        KisConfig::OCIO_ENVIRONMENT,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::BackgroundStyle, 3> {{
        KisConfig::RASTER_LAYER,
        KisConfig::CANVAS_COLOR,
        KisConfig::FILL_LAYER,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::SelectionActionsBarBehavior, 2> {{
        KisConfig::FreeFloating,
        KisConfig::Fixed,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::SelectionActionsBarPosition, 8> {{
        KisConfig::Bottom,
        KisConfig::BottomLeft,
        KisConfig::BottomRight,
        KisConfig::Left,
        KisConfig::Right,
        KisConfig::Top,
        KisConfig::TopLeft,
        KisConfig::TopRight,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::SelectionActionsBarOrientation, 2> {{
        KisConfig::Horizontal,
        KisConfig::Vertical,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::LayerInfoTextStyle, 4> {{
        KisConfig::INFOTEXT_NONE,
        KisConfig::INFOTEXT_SIMPLE,
        KisConfig::INFOTEXT_BALANCED,
        KisConfig::INFOTEXT_DETAILED,
    }});

    comparePersistedIntegerValues(std::array<KisConfig::AssistantsDrawMode, 3> {{
        KisConfig::ASSISTANTS_DRAW_MODE_DIRECT,
        KisConfig::ASSISTANTS_DRAW_MODE_PIXMAP_CACHE,
        KisConfig::ASSISTANTS_DRAW_MODE_LARGE_PIXMAP_CACHE,
    }});
}

void KisConfigCompatibilityTest::canvasSurfaceModeNamesRoundTrip()
{
    /*
     * Consumer: The canvas surface manager reading existing user kritarc settings.
     * Operation: Saves and reloads each named canvas surface color-management mode.
     * Observable result: The stored name resolves to the same selected surface mode after restart.
     * Failure impact: A user's canvas can reopen with a different color-management mode.
     */
    const std::array<std::pair<KisConfig::CanvasSurfaceMode, QString>, 5> modes{{
        {KisConfig::CanvasSurfaceMode::Preferred, QStringLiteral("preferred")},
        {KisConfig::CanvasSurfaceMode::Rec709g22, QStringLiteral("rec709g22")},
        {KisConfig::CanvasSurfaceMode::Rec709g10, QStringLiteral("rec709g10")},
        {KisConfig::CanvasSurfaceMode::Rec2020pq, QStringLiteral("rec2020pq")},
        {KisConfig::CanvasSurfaceMode::Unmanaged, QStringLiteral("unmanaged")},
    }};

    KConfigGroup configGroup = KSharedConfig::openConfig()->group(QString());
    ScopedConfigEntry restoreEntry(configGroup, QStringLiteral("canvasSurfaceColorSpaceManagementMode"));

    for (const auto &mode : modes) {
        {
            KisConfig config(false);
            config.setCanvasSurfaceColorSpaceManagementMode(mode.first);
        }
        QCOMPARE(configGroup.readEntry(QStringLiteral("canvasSurfaceColorSpaceManagementMode"), QString()), mode.second);

        configGroup.writeEntry(QStringLiteral("canvasSurfaceColorSpaceManagementMode"), mode.second);
        configGroup.sync();

        KisConfig config(true);
        QCOMPARE(int(config.canvasSurfaceColorSpaceManagementMode()), int(mode.first));
    }
}

void KisConfigCompatibilityTest::displayFormatNamesRoundTrip()
{
    /*
     * Consumer: The OpenGL configuration reader using existing user kritadisplayrc settings.
     * Operation: Saves and reloads canvas bit-depth and root surface format names.
     * Observable result: The stored names resolve to the same display format modes after restart.
     * Failure impact: A user's display can reopen with an unintended bit depth or color format.
     */
    const std::array<std::pair<KisConfig::CanvasSurfaceBitDepthMode, QString>, 3> bitDepthModes{{
        {KisConfig::CanvasSurfaceBitDepthMode::DepthAuto, QStringLiteral("auto")},
        {KisConfig::CanvasSurfaceBitDepthMode::Depth8Bit, QStringLiteral("8bit")},
        {KisConfig::CanvasSurfaceBitDepthMode::Depth10Bit, QStringLiteral("10bit")},
    }};
    const std::array<std::pair<KisConfig::RootSurfaceFormat, QString>, 3> rootSurfaceFormats{{
        {KisConfig::BT709_G22, QStringLiteral("bt709-g22")},
        {KisConfig::BT709_G10, QStringLiteral("bt709-g10")},
        {KisConfig::BT2020_PQ, QStringLiteral("bt2020-pq")},
    }};

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings displayConfig(directory.filePath(QStringLiteral("kritadisplayrc")), QSettings::IniFormat);

    for (const auto &mode : bitDepthModes) {
        KisConfig::setCanvasSurfaceBitDepthMode(&displayConfig, mode.first);
        QCOMPARE(displayConfig.value(QStringLiteral("canvasSurfaceBitDepthMode")).toString(), mode.second);
        QCOMPARE(int(KisConfig::canvasSurfaceBitDepthMode(&displayConfig)), int(mode.first));
    }

    for (const auto &format : rootSurfaceFormats) {
        KisConfig::setRootSurfaceFormat(&displayConfig, format.first);
        QCOMPARE(displayConfig.value(QStringLiteral("rootSurfaceFormat")).toString(), format.second);
        QCOMPARE(int(KisConfig::rootSurfaceFormat(&displayConfig)), int(format.first));
    }
}

QTEST_GUILESS_MAIN(KisConfigCompatibilityTest)

#include "KisConfigCompatibilityTest.moc"
