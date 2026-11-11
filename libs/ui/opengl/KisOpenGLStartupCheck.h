/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_OPENGL_STARTUP_CHECK_H
#define KIS_OPENGL_STARTUP_CHECK_H

#include "kritaui_export.h"

#include <QString>
#include <QStringList>

/**
 * Startup check for the selected OpenGL renderer.
 *
 * L2 (fixing the Windows delay) makes the desktop OpenGL the default on
 * Windows and needs a way back to ANGLE when the driver only fails after the
 * context has been created. The check compiles a small shader, draws an 8x8
 * offscreen image and reads back one pixel. When it fails, the name of the
 * renderer is written to kritadisplayrc, and the next launch of
 * KisOpenGL::selectSurfaceConfig() skips that renderer and takes the next
 * candidate. A successful check removes the name again.
 *
 * See docs/perf/l2-renderer-choice.md conditions 2 and 7.
 */
namespace KisOpenGLStartupCheck
{
/// True when `rendererName` names a renderer explicitly. The accepted names
/// are the ones OpenGLRenderer converts: "desktop", "angle", "software" and
/// "none". "auto", an empty value and unknown names (including values that
/// only differ in case or spacing) are not explicit.
KRITAUI_EXPORT bool isExplicitRendererName(const QString &rendererName);

/// True when the effective renderer choice is explicit and the failure marks
/// must be ignored. `configHasRendererName` is false when kritadisplayrc has
/// no OpenGLRenderer key; the platform default then applies (for example
/// "angle" on Windows) and the marks are still honored. A L1 renderer switch
/// value of "auto" is not explicit either.
KRITAUI_EXPORT bool
isUserForcedRenderer(const QString &configRendererName, bool configHasRendererName, const QString &l1RendererName);

/// Run the first frame check on the current OpenGL context. Returns false
/// when the check fails; `failureReason` then says which step failed (it is
/// logged by the caller). The result is written to `configFilePath`, or to
/// kritadisplayrc in the generic configuration location when it is empty.
KRITAUI_EXPORT bool verifyFirstFrameOnCurrentContext(const QString &rendererName,
                                                    QString *failureReason = nullptr,
                                                    const QString &configFilePath = QString());

/// The renderer names (the values of OpenGLRenderer: "desktop", "angle" and
/// "software") whose first frame check failed in an earlier launch.
KRITAUI_EXPORT QStringList failedRendererNames(const QString &configFilePath = QString());

/// Remove `rendererName` from the failure mark. Called after a successful
/// check, so a renderer that works again is not skipped.
KRITAUI_EXPORT void clearFailedRenderer(const QString &rendererName,
                                       const QString &configFilePath = QString());

/// The candidates that keep their place in `candidateNames`. Renderers whose
/// first frame check failed are left out. When every candidate has failed,
/// the whole list is returned: trying them again is the only way a repaired
/// driver can come back, and the QPainter canvas stays the last resort.
/// With honorFailureMarks = false the caller has an explicit user choice
/// (kritadisplayrc OpenGLRenderer or the L1 renderer switch) and the marks
/// are ignored.
KRITAUI_EXPORT QStringList renderersToTry(const QStringList &candidateNames,
                                         bool honorFailureMarks = true,
                                         const QString &configFilePath = QString());

/// The environment variable of the contract test. It forces the check to
/// fail, so the test can observe the mark and the skipped renderer without a
/// broken graphics driver.
///
/// Removal condition: this seam exists only until the first frame check has
/// been confirmed on Intel, AMD and NVIDIA Windows machines
/// (docs/perf/l2-renderer-choice.md condition 3). Remove the variable, its
/// read and the contract test case then.
KRITAUI_EXPORT bool isForcedFailureForTesting();

/// Write a failure mark by hand. The check uses it; the contract test uses it
/// to build a mark for renderers that are not under test.
KRITAUI_EXPORT void recordFailedRenderer(const QString &rendererName,
                                        const QString &configFilePath = QString());
}

#endif // KIS_OPENGL_STARTUP_CHECK_H
