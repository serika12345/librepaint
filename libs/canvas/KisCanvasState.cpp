/*
 *  SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisCanvasState.h"
#include "KoZoomState.h"

bool KisCanvasState::operator==(const KisCanvasState &other) const {
    return qFuzzyCompare(zoom, other.zoom) &&
           qFuzzyCompare(effectiveZoom, other.effectiveZoom) &&
           zoomMode == other.zoomMode &&
           qFuzzyCompare(rotation, other.rotation) &&
           mirrorHorizontally == other.mirrorHorizontally &&
           mirrorVertically == other.mirrorVertically &&
           documentOffset == other.documentOffset &&
           documentOffsetF == other.documentOffsetF &&
           viewportOffsetF == other.viewportOffsetF &&
           minimumOffset == other.minimumOffset &&
           maximumOffset == other.maximumOffset &&
           canvasSize == other.canvasSize &&
           qFuzzyCompare(minimumZoom, other.minimumZoom) &&
           qFuzzyCompare(maximumZoom, other.maximumZoom) &&
           imageRectInWidgetPixels == other.imageRectInWidgetPixels;
}

KoZoomState KisCanvasState::zoomState() const {
    KoZoomState state;
    state.mode = zoomMode;
    state.zoom = zoom;
    state.minZoom = minimumZoom;
    state.maxZoom = maximumZoom;
    return state;
}
