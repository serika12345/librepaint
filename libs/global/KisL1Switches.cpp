/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisL1Switches.h"

#include <QByteArray>
#include <QtGlobal>

namespace
{
bool isSet(const char *name)
{
    return qEnvironmentVariableIsSet(name);
}

int integerValue(const char *name, int fallback)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return ok ? value : fallback;
}

bool flagValue(const char *name)
{
    const QByteArray value = qgetenv(name).trimmed().toLower();
    return value == "1" || value == "true" || value == "on" || value == "yes";
}

QSize sizeValue(const char *name)
{
    const QByteArray value = qgetenv(name).trimmed();
    const int separator = value.indexOf('x');
    if (separator <= 0) return QSize();

    bool widthOk = false;
    bool heightOk = false;
    const int width = value.left(separator).toInt(&widthOk);
    const int height = value.mid(separator + 1).toInt(&heightOk);
    if (!widthOk || !heightOk || width <= 0 || height <= 0) return QSize();
    return QSize(width, height);
}
}

namespace KisL1Switches
{
namespace Detail
{
int readMaxFrameLatency()
{
    if (!isSet("LIBREPAINT_L1_MAX_FRAME_LATENCY")) return 0;
    const int value = integerValue("LIBREPAINT_L1_MAX_FRAME_LATENCY", 0);
    return value >= 1 && value <= 3 ? value : 0;
}

int resolveMaxFrameLatency(int switchedValue, bool useLowLatencyByDefault)
{
    if (switchedValue > 0) return switchedValue;
    return useLowLatencyByDefault ? 1 : 0;
}

int readSwapInterval()
{
    if (!isSet("LIBREPAINT_L1_SWAP_INTERVAL")) return -1;
    const int value = integerValue("LIBREPAINT_L1_SWAP_INTERVAL", -1);
    return value == 0 || value == 1 ? value : -1;
}

bool readWaitForGpuBeforeNextFrame()
{
    return flagValue("LIBREPAINT_L1_WAIT_FOR_GPU");
}

QSize readWindowSize()
{
    return sizeValue("LIBREPAINT_L1_WINDOW_SIZE");
}

bool readDisableMoveCompression()
{
    return flagValue("LIBREPAINT_L1_DISABLE_MOVE_COMPRESSION");
}

bool readDisableUpdateThrottle()
{
    return flagValue("LIBREPAINT_L1_DISABLE_UPDATE_THROTTLE");
}

QString readOpenGLRenderer()
{
    const QString value = QString::fromLatin1(qgetenv("LIBREPAINT_L1_OPENGL_RENDERER")).trimmed().toLower();
    if (value == "desktop" || value == "angle" || value == "auto") return value;
    return QString();
}

int readTextureBuffers()
{
    const QByteArray value = qgetenv("LIBREPAINT_L1_TEXTURE_BUFFERS").trimmed().toLower();
    if (value == "off" || value == "0" || value == "false" || value == "no") return 0;
    if (value == "on" || value == "1" || value == "true" || value == "yes") return 1;
    return -1;
}

int readWidgetsRhi()
{
    if (!isSet("LIBREPAINT_L1_WIDGETS_RHI")) return -1;
    const int value = integerValue("LIBREPAINT_L1_WIDGETS_RHI", -1);
    return value == 0 || value == 1 ? value : -1;
}
}

// The pen and frame paths call these accessors for every event and every
// frame. Reading the environment there would add the environment lock and a
// string allocation to the measured path, so each value is read once per
// process and kept in the function-local static. The test seam is
// Detail::read*(), which reads the environment again on every call.
int maxFrameLatency()
{
    static const int value = Detail::readMaxFrameLatency();
    return value;
}

int swapInterval()
{
    static const int value = Detail::readSwapInterval();
    return value;
}

bool waitForGpuBeforeNextFrame()
{
    static const bool value = Detail::readWaitForGpuBeforeNextFrame();
    return value;
}

QSize windowSize()
{
    static const QSize value = Detail::readWindowSize();
    return value;
}

bool disableMoveCompression()
{
    static const bool value = Detail::readDisableMoveCompression();
    return value;
}

bool disableUpdateThrottle()
{
    static const bool value = Detail::readDisableUpdateThrottle();
    return value;
}

QString openGLRenderer()
{
    static const QString value = Detail::readOpenGLRenderer();
    return value;
}

int textureBuffers()
{
    static const int value = Detail::readTextureBuffers();
    return value;
}

int widgetsRhi()
{
    static const int value = Detail::readWidgetsRhi();
    return value;
}
}
