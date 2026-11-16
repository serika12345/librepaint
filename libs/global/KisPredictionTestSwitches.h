/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PREDICTION_TEST_SWITCHES_H
#define KIS_PREDICTION_TEST_SWITCHES_H

#include "kritaglobal_export.h"

#include <QByteArray>

/**
 * Environment switch that tells the prediction line apart from the confirmed
 * stroke (Issue #129).
 *
 * The prediction line uses the same colour and width as the confirmed stroke,
 * so neither the eye nor the screen capture can separate them. When
 * LIBREPAINT_PREDICTION_TEST_COLOR holds six hexadecimal digits (RRGGBB), the
 * prediction line is drawn in that colour instead. The value is read once per
 * process, when the application first asks for it.
 *
 * This colour is for telling the prediction line apart in the capture only.
 * The default appearance and the main metric measurements do not use it, and
 * an unset, empty or invalid value keeps the normal drawing.
 *
 * Removal condition: when the prediction line is enabled by default and the
 * capture check no longer needs colour detection, remove this switch.
 */
namespace KisPredictionTestSwitches
{
/// Six hexadecimal digits (RRGGBB, no leading #) as 0xRRGGBB, otherwise -1.
KRITAGLOBAL_EXPORT int testColorRgb();

namespace Detail
{
/// Validate the value only; the environment is not read.
KRITAGLOBAL_EXPORT int parseTestColor(const QByteArray &value);
/// Read the environment now, without the process cache.
KRITAGLOBAL_EXPORT int readTestColor();
}
}

#endif // KIS_PREDICTION_TEST_SWITCHES_H
