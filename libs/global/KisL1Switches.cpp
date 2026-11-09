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
int maxFrameLatency()
{
    if (!isSet("LIBREPAINT_L1_MAX_FRAME_LATENCY")) return 0;
    const int value = integerValue("LIBREPAINT_L1_MAX_FRAME_LATENCY", 0);
    return value >= 1 && value <= 3 ? value : 0;
}

int swapInterval()
{
    if (!isSet("LIBREPAINT_L1_SWAP_INTERVAL")) return -1;
    const int value = integerValue("LIBREPAINT_L1_SWAP_INTERVAL", -1);
    return value == 0 || value == 1 ? value : -1;
}

bool waitForGpuBeforeNextFrame()
{
    return flagValue("LIBREPAINT_L1_WAIT_FOR_GPU");
}

QSize windowSize()
{
    return sizeValue("LIBREPAINT_L1_WINDOW_SIZE");
}

bool disableMoveCompression()
{
    return flagValue("LIBREPAINT_L1_DISABLE_MOVE_COMPRESSION");
}

bool disableUpdateThrottle()
{
    return flagValue("LIBREPAINT_L1_DISABLE_UPDATE_THROTTLE");
}

QString openGLRenderer()
{
    const QString value = QString::fromLatin1(qgetenv("LIBREPAINT_L1_OPENGL_RENDERER")).trimmed().toLower();
    if (value == "desktop" || value == "angle" || value == "auto") return value;
    return QString();
}
}
