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
 * at a time on the same build. Every variable below is read when the
 * application uses the value, so unset or an empty value keeps the normal
 * behaviour. These switches are for the L1 measurements only.
 *
 * Removal condition: when L2 (fixing the Windows delay) makes a switch the
 * default, fold it into the normal path and remove the variable. Delete the
 * others after the L1 comparison is finished.
 */
namespace KisL1Switches
{
/// H1 (frames queued ahead): 1, 2 or 3; 0 keeps the driver default.
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
}

#endif // KIS_L1_SWITCHES_H
