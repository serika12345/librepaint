/*
 *  kis_tool_ellipse.cc - part of Krayon
 *
 *  SPDX-FileCopyrightText: 2000 John Califf <jcaliff@compuzone.net>
 *  SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2004 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2004 Clarence Dang <dang@kde.org>
 *  SPDX-FileCopyrightText: 2009 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_ellipse.h"
#include "KoFlakeTypes.h"
#include "kis_cursor.h"
#include "kis_resources_snapshot.h"
#include "kis_tool_ellipse_base.h"
#include "kis_tool_shape.h"
#include "kundo2magicstring.h"
#include <KoCanvasBase.h>
#include <KoCanvasResourceProvider.h>
#include <KoCanvasResourcesIds.h>
#include <kis_painting_information_builder.h>
#include <kis_tool_freehand_helper.h>
#include <gpu/KisGpuPaintImage.h>
#include <KoShapeStroke.h>

#include <application/ui/workspace/KisViewManager.h>
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <KoBasicShapeFactory.h>
#include <kis_figure_painting_stroke.h>
#include <kis_image.h>
#include <qmath.h>
#include <qobjectdefs.h>
#include <qpainterpath.h>
#include <QtGlobal>

namespace {
class KisToolEllipseHelper final : public KisToolFreehandHelper
{
public:
    using KisToolFreehandHelper::KisToolFreehandHelper;

    void paint(KisImageWSP image, KisNodeSP node, const QRectF &rect, qreal rotation)
    {
        namespace Gpu = LibrePaint::Image::Gpu;
        const auto owner = image->gpuImage();
        initPaintImpl(0, KisPaintInformation(rect.topLeft()), resourceManager(), image, node, image.data());
        if (!isRunning()) return;
        const auto result = owner->updateShape(owner->operations().activeOperation(),
                                               {Gpu::ShapeKind::Ellipse, rect.topLeft(), rect.bottomRight(), rotation});
        if (result != Gpu::Result::Success) {
            cancelPaint();
            Q_EMIT requestErrorMessage(i18n("The ellipse could not be drawn on this GPU paint layer."));
            return;
        }
        endPaint();
    }
};
}

KisToolEllipse::KisToolEllipse(KoCanvasBase * canvas)
        : KisToolEllipseBase(canvas, KisToolEllipseBase::PAINT, KisCursor::loadWithSize("tool_ellipse_cursor.svg", 32, 32, 6, 6))
{
    setObjectName("tool_ellipse");
    setSupportOutline(true);
    setIsOpacityPresetMode(true);

    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas);

    connect(kritaCanvas->viewManager()->canvasResourceProvider(), SIGNAL(sigEffectiveCompositeOpChanged()), SLOT(resetCursorStyle()));
}

KisToolEllipse::~KisToolEllipse()
{
}

void KisToolEllipse::resetCursorStyle()
{
    if (isEraser() && (nodePaintAbility() == NodePaintAbility::PAINT)) {
        useCursor(KisCursor::loadWithSize("tool_ellipse_eraser_cursor.svg", 32, 32, 6, 6));
    } else {
        KisToolEllipseBase::resetCursorStyle();
    }

    overrideCursorIfNotEditable();
}

void KisToolEllipse::finishRect(const QRectF& rect, qreal roundCornersX, qreal roundCornersY)
{
    Q_UNUSED(roundCornersX);
    Q_UNUSED(roundCornersY);

    if (rect.isEmpty())
        return;

    const KisToolShape::ShapeAddInfo info =
        shouldAddShape(currentNode());

    if (!info.shouldAddShape && image()->gpuImage()) {
        if (fillStyle() != KisFigurePaintingOptions::FillStyleNone) {
            Q_EMIT statusTextChanged(i18n("Filling an ellipse is not yet supported for this GPU document."));
            return;
        }
        if (strokeStyle() == KisFigurePaintingOptions::StrokeStyleNone) return;
        auto *source = canvas()->resourceManager();
        KoCanvasResourceProvider resources;
        for (int key : {KoCanvasResource::CurrentPaintOpPreset, KoCanvasResource::CurrentEffectiveCompositeOp,
                        KoCanvasResource::Size, KoCanvasResource::Opacity, KoCanvasResource::MirrorHorizontal,
                        KoCanvasResource::MirrorVertical, KoCanvasResource::GlobalAlphaLock, KoCanvasResource::EraserMode}) {
            resources.setResource(key, source->resource(key));
        }
        resources.setForegroundColor(strokeStyle() == KisFigurePaintingOptions::StrokeStyleBackground ?
                                     source->backgroundColor() : source->foregroundColor());
        KisPaintingInformationBuilder builder;
        KisToolEllipseHelper helper(&builder, &resources, kundo2_i18n("Draw Ellipse"));
        connect(&helper, &KisToolFreehandHelper::requestErrorMessage, this, &KoToolBase::statusTextChanged);
        helper.paint(image(), currentNode(), rect, getRotationAngle());
        return;
    }
    if (!info.shouldAddShape) {
        KisFigurePaintingStroke stroke(kundo2_i18n("Draw Ellipse"),
                                       image(),
                                       currentNode(),
                                       canvas()->resourceManager(),
                                       strokeStyle(),
                                       fillStyle(),
                                       fillTransform());
        QPainterPath path;
        path.addEllipse(rect);
        getRotatedPath(path, rect.center(), getRotationAngle());
        stroke.paintPainterPath(path);
    } else {
        KisResourcesSnapshot resources(image(),
                                       currentNode(),
                                       canvas()->resourceManager()->canvasResourcesInterface());
        QRectF r = convertToPt(rect);
        KoShape* shape = KoBasicShapeFactory::createEllipse(r);
        shape->rotate(qRadiansToDegrees(getRotationAngle()));
        KoShapeStrokeSP border(new KoShapeStroke(currentStrokeWidth(), resources.currentFgColor().toQColor()));
        shape->setStroke(border);

        info.markAsSelectionShapeIfNeeded(shape);

        addShape(shape);
    }
}

bool KisToolEllipse::supportsPaintingAssistants() const
{
    return true;
}
