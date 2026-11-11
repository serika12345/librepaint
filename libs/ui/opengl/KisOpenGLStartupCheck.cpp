/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisOpenGLStartupCheck.h"

#include <QByteArray>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QSettings>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QtGlobal>

namespace
{
const char kFailedRenderersKey[] = "OpenGLFirstFrameFailedRenderers";
const char kForceFailureVariable[] = "LIBREPAINT_TEST_FAIL_OPENGL_FIRST_FRAME";

// The check draws an 8x8 image and reads the middle pixel. The shader writes
// 0.25, 0.5, 0.75 and 1.0, which is 64, 128, 191 and 255 in 8 bits.
constexpr int kProbeSize = 8;
constexpr int kProbePixelX = 4;
constexpr int kProbePixelY = 4;
constexpr int kExpectedRed = 64;
constexpr int kExpectedGreen = 128;
constexpr int kExpectedBlue = 191;
constexpr int kExpectedAlpha = 255;
constexpr int kColorTolerance = 3;

QStringList knownRendererNames()
{
    return {QStringLiteral("desktop"), QStringLiteral("angle"), QStringLiteral("software")};
}

QStringList normalizedRendererNames(const QStringList &names)
{
    QStringList result;
    const QStringList known = knownRendererNames();
    for (const QString &name : names) {
        const QString normalized = name.trimmed().toLower();
        if (known.contains(normalized) && !result.contains(normalized)) {
            result << normalized;
        }
    }
    return result;
}

QString resolvedConfigFilePath(const QString &configFilePath)
{
    if (!configFilePath.isEmpty()) return configFilePath;
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/kritadisplayrc");
}

QStringList readFailedRendererNames(const QString &configFilePath)
{
    QSettings settings(resolvedConfigFilePath(configFilePath), QSettings::IniFormat);
    return normalizedRendererNames(settings.value(kFailedRenderersKey).toStringList());
}

void writeFailedRendererNames(const QStringList &names, const QString &configFilePath)
{
    QSettings settings(resolvedConfigFilePath(configFilePath), QSettings::IniFormat);
    const QStringList normalized = normalizedRendererNames(names);
    if (normalized.isEmpty()) {
        settings.remove(kFailedRenderersKey);
    } else {
        settings.setValue(kFailedRenderersKey, normalized);
    }
    settings.sync();
}

QString shaderVersionDirective(const QOpenGLContext *context)
{
    if (context->isOpenGLES()) {
        return QStringLiteral("#version 300 es\n");
    }
    // The Windows desktop context is requested with a compatibility profile,
    // and macOS gives a core profile. The profile token in the shader has to
    // match the context, or some drivers refuse to compile it.
    if (context->format().profile() == QSurfaceFormat::CoreProfile) {
        return QStringLiteral("#version 330 core\n");
    }
    return QStringLiteral("#version 330 compatibility\n");
}

QString pixelDifference(const unsigned char *pixel)
{
    return QStringLiteral("read back %1,%2,%3,%4 (expected %5,%6,%7,%8)")
        .arg(pixel[0]).arg(pixel[1]).arg(pixel[2]).arg(pixel[3])
        .arg(kExpectedRed).arg(kExpectedGreen).arg(kExpectedBlue).arg(kExpectedAlpha);
}
}

namespace KisOpenGLStartupCheck
{

bool isForcedFailureForTesting()
{
    return qEnvironmentVariableIsSet(kForceFailureVariable);
}

bool isExplicitRendererName(const QString &rendererName)
{
    // Match KisOpenGL::convertConfigToOpenGLRenderer() exactly. A value it
    // does not convert (including "auto") leaves the choice to the automatic
    // selection, so the failure marks must still be honored.
    return rendererName == QStringLiteral("desktop") || rendererName == QStringLiteral("angle")
        || rendererName == QStringLiteral("software") || rendererName == QStringLiteral("none");
}

bool isUserForcedRenderer(const QString &configRendererName, bool configHasRendererName, const QString &l1RendererName)
{
    return (configHasRendererName && isExplicitRendererName(configRendererName))
        || isExplicitRendererName(l1RendererName);
}

QStringList failedRendererNames(const QString &configFilePath)
{
    return readFailedRendererNames(configFilePath);
}

void recordFailedRenderer(const QString &rendererName, const QString &configFilePath)
{
    QStringList names = readFailedRendererNames(configFilePath);
    const QString normalized = rendererName.trimmed().toLower();
    if (!knownRendererNames().contains(normalized) || names.contains(normalized)) return;
    names << normalized;
    writeFailedRendererNames(names, configFilePath);
}

void clearFailedRenderer(const QString &rendererName, const QString &configFilePath)
{
    QStringList names = readFailedRendererNames(configFilePath);
    const QString normalized = rendererName.trimmed().toLower();
    if (!names.contains(normalized)) return;
    names.removeAll(normalized);
    writeFailedRendererNames(names, configFilePath);
}

QStringList renderersToTry(const QStringList &candidateNames,
                           bool honorFailureMarks,
                           const QString &configFilePath)
{
    if (!honorFailureMarks) return candidateNames;

    const QStringList failed = readFailedRendererNames(configFilePath);
    QStringList result;
    for (const QString &name : candidateNames) {
        if (!failed.contains(name)) result << name;
    }

    // Every candidate failed at some point. Trying them all again is the only
    // way a driver update can restore OpenGL; otherwise the application would
    // stay on the QPainter canvas forever.
    return result.isEmpty() ? candidateNames : result;
}

bool verifyFirstFrameOnCurrentContext(const QString &rendererName,
                                      QString *failureReason,
                                      const QString &configFilePath)
{
    auto fail = [&](const QString &reason) {
        if (failureReason) *failureReason = reason;
        recordFailedRenderer(rendererName, configFilePath);
        return false;
    };

    if (isForcedFailureForTesting()) {
        return fail(QStringLiteral("forced by %1").arg(QString::fromLatin1(kForceFailureVariable)));
    }

    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context || !context->isValid()) {
        return fail(QStringLiteral("there is no current OpenGL context"));
    }

    QOpenGLFunctions *f = context->functions();
    if (!f) {
        return fail(QStringLiteral("the context has no OpenGL functions"));
    }
    f->initializeOpenGLFunctions();

    while (f->glGetError() != GL_NO_ERROR) {
        // Clear a stale error so the check reports its own failure.
    }

    const QByteArray version = shaderVersionDirective(context).toLatin1();

    QOpenGLShaderProgram program;
    const QByteArray vertexSource = version + QByteArrayLiteral(
        "void main() {\n"
        "    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
        "    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);\n"
        "}\n");

    QByteArray fragmentSource = version;
    if (context->isOpenGLES()) {
        fragmentSource += QByteArrayLiteral("precision highp float;\n");
    }
    fragmentSource += QByteArrayLiteral(
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = vec4(0.25, 0.5, 0.75, 1.0);\n"
        "}\n");

    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        return fail(QStringLiteral("vertex shader did not compile: %1").arg(program.log().trimmed()));
    }
    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        return fail(QStringLiteral("fragment shader did not compile: %1").arg(program.log().trimmed()));
    }
    if (!program.link()) {
        return fail(QStringLiteral("shader program did not link: %1").arg(program.log().trimmed()));
    }

    QOpenGLFramebufferObject framebuffer(kProbeSize, kProbeSize);
    if (!framebuffer.isValid()) {
        return fail(QStringLiteral("could not create an %1x%1 framebuffer").arg(kProbeSize));
    }
    if (!framebuffer.bind()) {
        return fail(QStringLiteral("could not bind the offscreen framebuffer"));
    }

    const bool needsVertexArray =
        context->isOpenGLES() || context->format().profile() == QSurfaceFormat::CoreProfile;
    QOpenGLVertexArrayObject vertexArray;
    if (needsVertexArray && !vertexArray.create()) {
        framebuffer.release();
        return fail(QStringLiteral("could not create a vertex array object"));
    }
    if (vertexArray.isCreated()) {
        vertexArray.bind();
    }

    f->glViewport(0, 0, kProbeSize, kProbeSize);
    f->glDisable(GL_BLEND);
    f->glDisable(GL_DEPTH_TEST);
    f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    f->glClear(GL_COLOR_BUFFER_BIT);

    if (!program.bind()) {
        framebuffer.release();
        return fail(QStringLiteral("could not bind the shader program"));
    }
    f->glDrawArrays(GL_TRIANGLES, 0, 3);
    program.release();

    GLenum error = f->glGetError();
    if (error != GL_NO_ERROR) {
        framebuffer.release();
        return fail(QStringLiteral("drawing into the offscreen framebuffer failed: 0x%1")
                        .arg(error, 0, 16));
    }

    unsigned char pixel[4] = {0, 0, 0, 0};
    f->glReadPixels(kProbePixelX, kProbePixelY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    error = f->glGetError();
    if (error != GL_NO_ERROR) {
        framebuffer.release();
        return fail(QStringLiteral("reading the offscreen framebuffer failed: 0x%1")
                        .arg(error, 0, 16));
    }

    if (vertexArray.isCreated()) {
        vertexArray.release();
    }
    framebuffer.release();

    const bool pixelIsExpected =
        qAbs(int(pixel[0]) - kExpectedRed) <= kColorTolerance &&
        qAbs(int(pixel[1]) - kExpectedGreen) <= kColorTolerance &&
        qAbs(int(pixel[2]) - kExpectedBlue) <= kColorTolerance &&
        qAbs(int(pixel[3]) - kExpectedAlpha) <= kColorTolerance;
    if (!pixelIsExpected) {
        return fail(QStringLiteral("the offscreen image was wrong: %1").arg(pixelDifference(pixel)));
    }

    clearFailedRenderer(rendererName, configFilePath);
    if (failureReason) failureReason->clear();
    return true;
}

QString rendererSelectionLogLine(const QString &chosenRenderer,
                                 const QStringList &candidateNames,
                                 const QStringList &skippedNames)
{
    const QString chosen = chosenRenderer.isEmpty() ? QStringLiteral("none") : chosenRenderer;
    const QString candidates =
        candidateNames.isEmpty() ? QStringLiteral("none") : candidateNames.join(QLatin1Char(','));
    const QString skipped =
        skippedNames.isEmpty() ? QStringLiteral("none") : skippedNames.join(QLatin1Char(','));
    return QStringLiteral("OpenGL renderer selection: chosen=%1 candidates=%2 skipped=%3")
        .arg(chosen, candidates, skipped);
}

QString firstFrameCheckLogLine(const QString &rendererName, bool passed, const QString &failureReason)
{
    if (passed) {
        return QStringLiteral("OpenGL first frame check: renderer=%1 result=passed").arg(rendererName);
    }
    return QStringLiteral("OpenGL first frame check: renderer=%1 result=failed reason=%2")
        .arg(rendererName, failureReason);
}

}
