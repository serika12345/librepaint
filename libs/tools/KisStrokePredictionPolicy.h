/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_POLICY_H
#define KIS_STROKE_PREDICTION_POLICY_H

#include <QString>

#include <kritatools_export.h>

/**
 * State of one stroke, as far as the prediction line depends on it.
 *
 * smoothingType holds a KisSmoothingOptions::SmoothingType value as an int,
 * because libs/tools does not depend on the smoothing options object. The
 * values 0 and 1 correspond to NO_SMOOTHING and SIMPLE_SMOOTHING; this
 * evaluation has to be reviewed when the enumeration in
 * libs/tools/kis_smoothing_options.h changes.
 */
struct KisStrokePredictionPolicyInput {
    bool settingEnabled = false;   // hidden setting useStrokePrediction
    int smoothingType = 0;         // KisSmoothingOptions::SmoothingType as an int
    QString paintOpId;             // id of the current brush engine
    qreal opacity = 1.0;
    qreal flow = 1.0;
    bool eraser = false;
    bool compositeNormal = true;   // blending mode is COMPOSITE_OVER
    bool assistantActive = false;  // an assistant corrects the input points
    bool toolSupported = true;     // the tool draws where the pointer is (not Dyna or Lazy Brush)
};

enum class KisStrokePredictionPolicyReason {
    Enabled, SettingOff, Smoothing, PaintOp, Opacity, Flow, Eraser, Composite, Assistant, Tool
};

/**
 * Decides whether the display-only prediction line may be drawn for the
 * current stroke. The evaluation is pure: it depends only on the input and
 * shares no state with the canvas.
 */
namespace KisStrokePredictionPolicy {
/// Returns the first reason that applies, in the documented order.
KRITATOOLS_EXPORT KisStrokePredictionPolicyReason evaluate(const KisStrokePredictionPolicyInput &input);
/// Stable name of a reason: "enabled", "setting_off", "smoothing", "paintop",
/// "opacity", "flow", "eraser", "composite", "assistant".
KRITATOOLS_EXPORT const char *reasonName(KisStrokePredictionPolicyReason reason);
}

#endif // KIS_STROKE_PREDICTION_POLICY_H
