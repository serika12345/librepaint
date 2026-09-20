/*
 *  SPDX-FileCopyrightText: 2007 Adrian Page <adrian@pagenet.plus.com>
 *  SPDX-FileCopyrightText: 2023 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_opengl.h"

#include <QDebug>

#include <kis_debug.h>

QString KisOpenGL::convertOpenGLRendererToConfig(KisOpenGL::OpenGLRenderer renderer)
{
    switch (renderer) {
    case RendererNone:
        return QStringLiteral("none");
    case RendererSoftware:
        return QStringLiteral("software");
    case RendererDesktopGL:
        return QStringLiteral("desktop");
    case RendererOpenGLES:
        return QStringLiteral("angle");
    default:
        return QStringLiteral("auto");
    }
}

KisOpenGL::OpenGLRenderer KisOpenGL::convertConfigToOpenGLRenderer(QString renderer)
{
    if (renderer == "desktop") {
        return RendererDesktopGL;
    } else if (renderer == "angle") {
        return RendererOpenGLES;
    } else if (renderer == "software") {
        return RendererSoftware;
    } else if (renderer == "none") {
        return RendererNone;
    } else {
        return RendererAuto;
    }
}

KisOpenGL::OpenGLRenderer KisOpenGL::RendererConfig::rendererId() const
{
    KisOpenGL::OpenGLRenderer result = RendererAuto;

    if (format.renderableType() == QSurfaceFormat::OpenGLES &&
        angleRenderer == AngleRendererD3d11Warp) {

        result = RendererSoftware;

    } else if (format.renderableType() == QSurfaceFormat::OpenGLES) {
        // If D3D11, D3D9?, Default (which is after probing, if selected)
        // or the system specifies QT_OPENGL_ES_2
        result = RendererOpenGLES;
    } else if (format.renderableType() == QSurfaceFormat::OpenGL) {
        result = RendererDesktopGL;
    } else if (format.renderableType() == QSurfaceFormat::DefaultRenderableType &&
               angleRenderer == AngleRendererD3d11) {
        // noop
    } else {
        qWarning() << "WARNING: unsupported combination of OpenGL renderer" << ppVar(format.renderableType()) << ppVar(angleRenderer);
    }

    return result;
}
