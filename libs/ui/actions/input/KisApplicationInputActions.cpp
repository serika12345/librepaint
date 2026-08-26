/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisApplicationInputActions.h"

#include <input/ui/KisPopupWidgetAction.h>
#include <input/ui/kis_alternate_invocation_action.h>
#include <input/ui/kis_change_primary_setting_action.h>
#include <input/ui/kis_tool_invocation_action.h>
#include <input/ui/kis_input_manager.h>

#include <canvas/kis_canvas2.h>

#include "KisTouchGestureAction.h"
#include "kis_change_frame_action.h"
#include "kis_gamma_exposure_action.h"
#include "kis_pan_action.h"
#include "kis_rotate_canvas_action.h"
#include "kis_select_layer_action.h"
#include "kis_zoom_action.h"
#include "kis_zoom_and_rotate_action.h"

QList<KisAbstractInputAction *> createApplicationInputActions()
{
    return {
        new KisToolInvocationAction(),
        new KisAlternateInvocationAction(),
        new KisChangePrimarySettingAction(),
        new KisPanAction(),
        new KisRotateCanvasAction(),
        new KisZoomAction(),
        new KisPopupWidgetAction(),
        new KisSelectLayerAction(),
        new KisGammaExposureAction(),
        new KisChangeFrameAction(),
        new KisZoomAndRotateAction(),
        new KisTouchGestureAction(),
    };
}

KisCanvas2 *applicationInputCanvas(const KisInputManager *inputManager)
{
    return qobject_cast<KisCanvas2 *>(inputManager->canvas());
}
