/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionPolicy.h"

#include <QtGlobal>

namespace KisStrokePredictionPolicy
{

KisStrokePredictionPolicyReason evaluate(const KisStrokePredictionPolicyInput &input)
{
    if (!input.settingEnabled) {
        return KisStrokePredictionPolicyReason::SettingOff;
    }

    if (input.smoothingType != 0 && input.smoothingType != 1) {
        return KisStrokePredictionPolicyReason::Smoothing;
    }

    if (input.paintOpId != QStringLiteral("paintbrush")) {
        return KisStrokePredictionPolicyReason::PaintOp;
    }

    if (!qFuzzyCompare(input.opacity, 1.0)) {
        return KisStrokePredictionPolicyReason::Opacity;
    }

    if (input.flow != 1.0) {
        return KisStrokePredictionPolicyReason::Flow;
    }

    if (input.eraser) {
        return KisStrokePredictionPolicyReason::Eraser;
    }

    if (!input.compositeNormal) {
        return KisStrokePredictionPolicyReason::Composite;
    }

    if (input.assistantActive) {
        return KisStrokePredictionPolicyReason::Assistant;
    }

    return KisStrokePredictionPolicyReason::Enabled;
}

const char *reasonName(KisStrokePredictionPolicyReason reason)
{
    switch (reason) {
    case KisStrokePredictionPolicyReason::Enabled:
        return "enabled";
    case KisStrokePredictionPolicyReason::SettingOff:
        return "setting_off";
    case KisStrokePredictionPolicyReason::Smoothing:
        return "smoothing";
    case KisStrokePredictionPolicyReason::PaintOp:
        return "paintop";
    case KisStrokePredictionPolicyReason::Opacity:
        return "opacity";
    case KisStrokePredictionPolicyReason::Flow:
        return "flow";
    case KisStrokePredictionPolicyReason::Eraser:
        return "eraser";
    case KisStrokePredictionPolicyReason::Composite:
        return "composite";
    case KisStrokePredictionPolicyReason::Assistant:
        return "assistant";
    }
    return "unknown";
}

} // namespace KisStrokePredictionPolicy
