/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisL1Switches.h"

#include <QTest>

// The L1 experiments switch one hypothesis at a time on the same build.
// Unset or invalid values must leave the normal behaviour untouched, and each
// variable must keep the documented range (KisL1Switches.h).
class KisL1SwitchesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        qunsetenv("LIBREPAINT_L1_MAX_FRAME_LATENCY");
        qunsetenv("LIBREPAINT_L1_SWAP_INTERVAL");
        qunsetenv("LIBREPAINT_L1_WAIT_FOR_GPU");
        qunsetenv("LIBREPAINT_L1_WINDOW_SIZE");
        qunsetenv("LIBREPAINT_L1_DISABLE_MOVE_COMPRESSION");
        qunsetenv("LIBREPAINT_L1_DISABLE_UPDATE_THROTTLE");
        qunsetenv("LIBREPAINT_L1_OPENGL_RENDERER");
    }

    void unsetVariablesKeepNormalBehaviour()
    {
        QCOMPARE(KisL1Switches::maxFrameLatency(), 0);
        QCOMPARE(KisL1Switches::swapInterval(), -1);
        QCOMPARE(KisL1Switches::waitForGpuBeforeNextFrame(), false);
        QCOMPARE(KisL1Switches::windowSize(), QSize());
        QCOMPARE(KisL1Switches::disableMoveCompression(), false);
        QCOMPARE(KisL1Switches::disableUpdateThrottle(), false);
        QCOMPARE(KisL1Switches::openGLRenderer(), QString());
    }

    void maxFrameLatencyAcceptsOnlyOneToThree()
    {
        qputenv("LIBREPAINT_L1_MAX_FRAME_LATENCY", "1");
        QCOMPARE(KisL1Switches::maxFrameLatency(), 1);
        qputenv("LIBREPAINT_L1_MAX_FRAME_LATENCY", "3");
        QCOMPARE(KisL1Switches::maxFrameLatency(), 3);
        for (const char *value : {"0", "4", "-1", "many", ""}) {
            qputenv("LIBREPAINT_L1_MAX_FRAME_LATENCY", value);
            QCOMPARE(KisL1Switches::maxFrameLatency(), 0);
        }
    }

    void swapIntervalAcceptsZeroAndOne()
    {
        qputenv("LIBREPAINT_L1_SWAP_INTERVAL", "0");
        QCOMPARE(KisL1Switches::swapInterval(), 0);
        qputenv("LIBREPAINT_L1_SWAP_INTERVAL", "1");
        QCOMPARE(KisL1Switches::swapInterval(), 1);
        for (const char *value : {"2", "-1", "on", ""}) {
            qputenv("LIBREPAINT_L1_SWAP_INTERVAL", value);
            QCOMPARE(KisL1Switches::swapInterval(), -1);
        }
    }

    void flagsAcceptTheDocumentedForms()
    {
        for (const char *value : {"1", "true", "on", "yes"}) {
            qputenv("LIBREPAINT_L1_WAIT_FOR_GPU", value);
            QCOMPARE(KisL1Switches::waitForGpuBeforeNextFrame(), true);
            qputenv("LIBREPAINT_L1_DISABLE_MOVE_COMPRESSION", value);
            QCOMPARE(KisL1Switches::disableMoveCompression(), true);
            qputenv("LIBREPAINT_L1_DISABLE_UPDATE_THROTTLE", value);
            QCOMPARE(KisL1Switches::disableUpdateThrottle(), true);
        }
        for (const char *value : {"0", "false", "off", "no", ""}) {
            qputenv("LIBREPAINT_L1_WAIT_FOR_GPU", value);
            QCOMPARE(KisL1Switches::waitForGpuBeforeNextFrame(), false);
        }
    }

    void windowSizeRequiresTwoPositiveNumbers()
    {
        qputenv("LIBREPAINT_L1_WINDOW_SIZE", "1024x768");
        QCOMPARE(KisL1Switches::windowSize(), QSize(1024, 768));
        for (const char *value : {"1024", "0x768", "1024x0", "ax768", "1024x768x2", ""}) {
            qputenv("LIBREPAINT_L1_WINDOW_SIZE", value);
            QCOMPARE(KisL1Switches::windowSize(), QSize());
        }
    }

    void openGLRendererAcceptsTheThreeConfigNames()
    {
        qputenv("LIBREPAINT_L1_OPENGL_RENDERER", " DESKTOP ");
        QCOMPARE(KisL1Switches::openGLRenderer(), QStringLiteral("desktop"));
        qputenv("LIBREPAINT_L1_OPENGL_RENDERER", "angle");
        QCOMPARE(KisL1Switches::openGLRenderer(), QStringLiteral("angle"));
        qputenv("LIBREPAINT_L1_OPENGL_RENDERER", "auto");
        QCOMPARE(KisL1Switches::openGLRenderer(), QStringLiteral("auto"));
        qputenv("LIBREPAINT_L1_OPENGL_RENDERER", "opengl");
        QCOMPARE(KisL1Switches::openGLRenderer(), QString());
    }
};

QTEST_GUILESS_MAIN(KisL1SwitchesContractTest)

#include "KisL1SwitchesContractTest.moc"
