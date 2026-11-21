/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IOS_PREDICTED_TOUCHES_H
#define KIS_IOS_PREDICTED_TOUCHES_H

/// Attaches a passive gesture recognizer to the Qt view and feeds the Apple
/// Pencil predicted touches into KisPlatformPredictionBuffer::instance().
void installKisIOSPredictedTouches(void *nativeView);

#endif
