/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "opengl/kis_opengl.h"

#include <QTest>

#include <array>
#include <type_traits>

class KisOpenGLRendererConfigContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterModesKeepStableOrdinals();
    void rendererFlagsKeepStableBitMasks();
    void platformRendererEnumsKeepStableCodes();
    void rendererConfigDefaultsAndClassifiesValueOnlyFormats();
    void rendererConfigStringsRoundTripKnownBackends();
};

void KisOpenGLRendererConfigContractTest::filterModesKeepStableOrdinals()
{
    QCOMPARE(int(KisOpenGL::NearestFilterMode), 0);
    QCOMPARE(int(KisOpenGL::BilinearFilterMode), 1);
    QCOMPARE(int(KisOpenGL::TrilinearFilterMode), 2);
    QCOMPARE(int(KisOpenGL::HighQualityFiltering), 3);
}

void KisOpenGLRendererConfigContractTest::rendererFlagsKeepStableBitMasks()
{
    static_assert(std::is_same_v<KisOpenGL::OpenGLRenderers, QFlags<KisOpenGL::OpenGLRenderer>>);

    QCOMPARE(int(KisOpenGL::RendererNone), 0x00);
    QCOMPARE(int(KisOpenGL::RendererAuto), 0x01);
    QCOMPARE(int(KisOpenGL::RendererDesktopGL), 0x02);
    QCOMPARE(int(KisOpenGL::RendererOpenGLES), 0x04);
    QCOMPARE(int(KisOpenGL::RendererSoftware), 0x08);

    KisOpenGL::OpenGLRenderers renderers(KisOpenGL::RendererDesktopGL);
    renderers |= KisOpenGL::RendererOpenGLES;
    QCOMPARE(renderers.toInt(), 0x06);
    QVERIFY(renderers.testFlag(KisOpenGL::RendererDesktopGL));
    QVERIFY(renderers.testFlag(KisOpenGL::RendererOpenGLES));
    QVERIFY(!renderers.testFlag(KisOpenGL::RendererSoftware));
}

void KisOpenGLRendererConfigContractTest::platformRendererEnumsKeepStableCodes()
{
    QCOMPARE(int(KisOpenGL::AngleRendererDefault), 0x0000);
    QCOMPARE(int(KisOpenGL::AngleRendererD3d11), 0x0002);
    QCOMPARE(int(KisOpenGL::AngleRendererD3d9), 0x0004);
    QCOMPARE(int(KisOpenGL::AngleRendererD3d11Warp), 0x0008);

    QCOMPARE(int(KisOpenGL::XCB_GLX), 0);
    QCOMPARE(int(KisOpenGL::XCB_EGL), 1);
}

void KisOpenGLRendererConfigContractTest::rendererConfigDefaultsAndClassifiesValueOnlyFormats()
{
    static_assert(std::is_class_v<KisOpenGL>);
    static_assert(!std::is_default_constructible_v<KisOpenGL>);

    const KisOpenGL::RendererConfig defaults;
    QCOMPARE(defaults.format.renderableType(), QSurfaceFormat::DefaultRenderableType);
    QCOMPARE(defaults.angleRenderer, KisOpenGL::AngleRendererDefault);

    const auto rendererFor = [](QSurfaceFormat::RenderableType type, KisOpenGL::AngleRenderer angleRenderer) {
        KisOpenGL::RendererConfig config;
        config.format.setRenderableType(type);
        config.angleRenderer = angleRenderer;
        return config.rendererId();
    };

    QCOMPARE(rendererFor(QSurfaceFormat::DefaultRenderableType, KisOpenGL::AngleRendererD3d11),
             KisOpenGL::RendererAuto);
    QCOMPARE(rendererFor(QSurfaceFormat::OpenGL, KisOpenGL::AngleRendererD3d11Warp),
             KisOpenGL::RendererDesktopGL);
    QCOMPARE(rendererFor(QSurfaceFormat::OpenGLES, KisOpenGL::AngleRendererDefault),
             KisOpenGL::RendererOpenGLES);
    QCOMPARE(rendererFor(QSurfaceFormat::OpenGLES, KisOpenGL::AngleRendererD3d9),
             KisOpenGL::RendererOpenGLES);
    QCOMPARE(rendererFor(QSurfaceFormat::OpenGLES, KisOpenGL::AngleRendererD3d11Warp),
             KisOpenGL::RendererSoftware);
}

void KisOpenGLRendererConfigContractTest::rendererConfigStringsRoundTripKnownBackends()
{
    struct RendererConfigEntry {
        KisOpenGL::OpenGLRenderer renderer;
        const char *text;
    };

    const std::array<RendererConfigEntry, 5> entries {{
        {KisOpenGL::RendererNone, "none"},
        {KisOpenGL::RendererAuto, "auto"},
        {KisOpenGL::RendererDesktopGL, "desktop"},
        {KisOpenGL::RendererOpenGLES, "angle"},
        {KisOpenGL::RendererSoftware, "software"},
    }};

    for (const RendererConfigEntry &entry : entries) {
        const QString text = QString::fromLatin1(entry.text);
        QCOMPARE(KisOpenGL::convertOpenGLRendererToConfig(entry.renderer), text);
        QCOMPARE(KisOpenGL::convertConfigToOpenGLRenderer(text), entry.renderer);
    }

    QCOMPARE(KisOpenGL::convertOpenGLRendererToConfig(KisOpenGL::OpenGLRenderer(0x40)),
             QStringLiteral("auto"));
    QCOMPARE(KisOpenGL::convertConfigToOpenGLRenderer(QString()), KisOpenGL::RendererAuto);
    QCOMPARE(KisOpenGL::convertConfigToOpenGLRenderer(QStringLiteral("Desktop")), KisOpenGL::RendererAuto);
    QCOMPARE(KisOpenGL::convertConfigToOpenGLRenderer(QStringLiteral(" desktop ")), KisOpenGL::RendererAuto);
    QCOMPARE(KisOpenGL::convertConfigToOpenGLRenderer(QStringLiteral("vulkan")), KisOpenGL::RendererAuto);
}

QTEST_GUILESS_MAIN(KisOpenGLRendererConfigContractTest)

#include "KisOpenGLRendererConfigContractTest.moc"
