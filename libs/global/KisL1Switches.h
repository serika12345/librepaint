/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KIS_L1_SWITCHES_H
#define KIS_L1_SWITCHES_H

#include "kritaglobal_export.h"

#include <QSize>
#include <QString>

/**
 * Environment switches for the L1 experiments of Issue #108.
 *
 * L1 (the cause of the about 50 ms delay on Windows) compares one hypothesis
 * at a time on the same build. Every variable below is read once per process,
 * when the application first asks for the value, so unset or an empty value
 * keeps the normal behaviour. The pen and frame paths call the accessors
 * repeatedly; the environment is not read there. These switches are for the
 * L1 measurements only.
 *
 * Removal condition: when L2 (fixing the Windows delay) makes a switch the
 * default, fold it into the normal path and remove the variable. Delete the
 * others after the L1 comparison is finished.
 */
namespace KisL1Switches
{
/// H1 (frames queued ahead): 1, 2 or 3; 0 keeps the normal default, which
/// is 1 on the ANGLE path unless the hidden kritarc option turns it off.
KRITAGLOBAL_EXPORT int maxFrameLatency();
/// H3 (vertical sync): 0 or 1; -1 keeps the normal surface setting.
KRITAGLOBAL_EXPORT int swapInterval();
/// H2 (waiting for display handoff): wait for the previous frame before paint.
KRITAGLOBAL_EXPORT bool waitForGpuBeforeNextFrame();
/// H4 (GPU load of one frame): window size; an invalid size keeps the normal one.
KRITAGLOBAL_EXPORT QSize windowSize();
/// H5 (hover event decimation): dispatch every movable event immediately.
KRITAGLOBAL_EXPORT bool disableMoveCompression();
/// H6 (frame update throttle): do not defer paint until the last frame completes.
KRITAGLOBAL_EXPORT bool disableUpdateThrottle();
/// H7 (no ANGLE): "desktop", "angle" or "auto"; empty keeps the kritarc value.
KRITAGLOBAL_EXPORT QString openGLRenderer();
/// L2 CPU cause: texture buffer (PBO) uploads; -1 keeps the renderer default,
/// 0 disables them (the ANGLE default) and 1 enables them on every renderer.
KRITAGLOBAL_EXPORT int textureBuffers();
/// GPU-memory churn: reuse a texture level when a full-tile update has the
/// same size, instead of allocating it again with glTexImage2D.
KRITAGLOBAL_EXPORT bool reuseTextureStorage();
/// GPU-object churn: reuse the frame fence object and only renew the GLsync.
KRITAGLOBAL_EXPORT bool reuseGlSync();
/// Context switches: skip makeCurrent() when the widget context is already
/// current, and do not switch back in that case.
KRITAGLOBAL_EXPORT bool skipRedundantMakeCurrent();
/// Color selector: do not upload or paint its palette while measuring. The
/// widget keeps its last texture.
KRITAGLOBAL_EXPORT bool disableColorSelectorGl();

namespace Detail
{
/// Read and validate the environment now, without the process cache. The
/// accessors above keep the first result; the contract test uses these
/// functions to check every accepted form in one process.
KRITAGLOBAL_EXPORT int readMaxFrameLatency();
/// Combine the L1 switch with the normal default: a switch value wins,
/// otherwise the ANGLE path asks for one frame unless it is disabled.
KRITAGLOBAL_EXPORT int resolveMaxFrameLatency(int switchedValue,
                                             bool useLowLatencyByDefault);
KRITAGLOBAL_EXPORT int readSwapInterval();
KRITAGLOBAL_EXPORT bool readWaitForGpuBeforeNextFrame();
KRITAGLOBAL_EXPORT QSize readWindowSize();
KRITAGLOBAL_EXPORT bool readDisableMoveCompression();
KRITAGLOBAL_EXPORT bool readDisableUpdateThrottle();
KRITAGLOBAL_EXPORT QString readOpenGLRenderer();
KRITAGLOBAL_EXPORT int readTextureBuffers();
KRITAGLOBAL_EXPORT bool readReuseTextureStorage();
KRITAGLOBAL_EXPORT bool readReuseGlSync();
KRITAGLOBAL_EXPORT bool readSkipRedundantMakeCurrent();
KRITAGLOBAL_EXPORT bool readDisableColorSelectorGl();
}
}

#endif // KIS_L1_SWITCHES_H
