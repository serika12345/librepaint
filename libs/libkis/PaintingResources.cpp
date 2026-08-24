/*
 *  SPDX-FileCopyrightText: 2020 Scott Petrovic <scottpetrovic@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "PaintingResources.h"

#include "workspace/KisView.h"
#include "application/KisPart.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "workspace/KisViewManager.h"
#include "workspace/KisMainWindow.h"
#include "kis_image.h"
#include <KisFigurePaintingOptions.h>


const QStringList StrokeStyle = {
    "None",             // 0 = KisFigurePaintingOptions::StrokeStyleNone
    "ForegroundColor",  //     KisFigurePaintingOptions::StrokeStyleForeground
    "BackgroundColor"   //     KisFigurePaintingOptions::StrokeStyleBackground
};

const QStringList FillStyle = {
    "None",             // 0 = KisFigurePaintingOptions::FillStyleNone
    "ForegroundColor",  //     KisFigurePaintingOptions::FillStyleForegroundColor
    "BackgroundColor",  //     KisFigurePaintingOptions::FillStyleBackgroundColor
    "Pattern"           //     KisFigurePaintingOptions::FillStylePattern
};

KisFigurePaintingStroke PaintingResources::createFigurePaintingStroke(
    KisImageWSP image,
    KisNodeSP node,
    const QString strokeStyleString,
    const QString fillStyleString)
{
    // need to grab the resource provider
    KisView *activeView = KisPart::instance()->currentMainwindow()->activeView();
    KoCanvasResourceProvider *resourceManager = activeView->viewManager()->canvasResourceProvider()->resourceManager();

    int strokeIndex = StrokeStyle.indexOf(strokeStyleString);
    if (strokeIndex == -1) {
        dbgScript << "Script tried to paint with invalid strokeStyle" << strokeStyleString << ", ignoring and using" << defaultStrokeStyle << ".";
        strokeIndex = StrokeStyle.indexOf(defaultStrokeStyle);
        if (strokeIndex == -1) {
            warnScript << "PaintingResources::createFigurePaintingStroke(): defaultStrokeStyle" << defaultStrokeStyle << "is invalid!";
            strokeIndex = 1;
        }
    }
    KisFigurePaintingOptions::StrokeStyle strokeStyle =
        static_cast<KisFigurePaintingOptions::StrokeStyle>(strokeIndex);

    int fillIndex = FillStyle.indexOf(fillStyleString);
    if (fillIndex == -1) {
        dbgScript << "Script tried to paint with invalid fillStyle" << fillStyleString << ", ignoring and using" << defaultFillStyle << ".";
        fillIndex = FillStyle.indexOf(defaultFillStyle);
        if (fillIndex == -1) {
            warnScript << "PaintingResources::createFigurePaintingStroke(): defaultFillStyle" << defaultFillStyle << " is invalid!";
            fillIndex = 0;
        }
    }
    KisFigurePaintingOptions::FillStyle fillStyle =
        static_cast<KisFigurePaintingOptions::FillStyle>(fillIndex);

    const KUndo2MagicString name = kundo2_i18n("Scripted Brush Stroke");
    return KisFigurePaintingStroke(
        name,
        image,
        node, resourceManager,
        strokeStyle,
        fillStyle
    );
}
