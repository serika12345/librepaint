/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_figure_painting_stroke.h>

#include <KoCanvasResourceProvider.h>

#include "KisFigurePaintingOptions.h"
#include "kis_resources_snapshot.h"
#include <kis_distance_information.h>
#include "kis_image.h"
#include "kis_painter.h"
#include <qtransform.h>
#include <strokes/KisFreehandStrokeInfo.h>
#include <strokes/freehand_stroke.h>
#include "KisAsynchronousStrokeUpdateHelper.h"
#include "kis_stroke_strategy.h"
#include "kis_types.h"
#include <gpu/KisGpuPaintImage.h>
#include <brushengine/kis_paintop_settings.h>
#include <brushengine/kis_paintop_preset.h>
#include <kis_debug.h>
#include <optional>

namespace Gpu = LibrePaint::Image::Gpu;

struct KisFigurePaintingStroke::GpuStroke
{
    KisImageSP image;
    KisNodeSP node;
    Gpu::PaintImageSP owner;
    KisGpuBrushResources resources;
    KisSelectionSP selection;
    KisFigurePaintingOptions::StrokeStyle strokeStyle;
    KisFigurePaintingOptions::FillStyle fillStyle;
    std::optional<Gpu::Shape> shape;
    QList<Gpu::InputSample> samples;

    void setFigure(const Gpu::Shape &value, const QList<Gpu::InputSample> &input)
    {
        if (shape) {
            resources.diagnostic = QStringLiteral("Multiple figures require a grouped GPU painting operation.");
            return;
        }
        shape = value;
        samples = input;
    }

    void setColor(const KoColor &value)
    {
        const auto color = value.toQColor();
        resources.brush.rgba = {quint8(color.red()), quint8(color.green()), quint8(color.blue()), quint8(color.alpha())};
    }

    void finish()
    {
        if (fillStyle != KisFigurePaintingOptions::FillStyleNone)
            resources.diagnostic = QStringLiteral("GPU figure fills require a fill operation.");
        if (selection)
            resources.diagnostic = QStringLiteral("GPU figure painting requires selection coverage.");
        if (!resources.diagnostic.isEmpty()) {
            warnKrita << resources.diagnostic;
            return;
        }
        if (!shape || strokeStyle == KisFigurePaintingOptions::StrokeStyleNone) return;
        const auto operation = owner->beginStroke(resources.layer, resources.brush, 0);
        if (operation.isNull()) {
            warnKrita << "A GPU figure stroke could not be started.";
            return;
        }
        auto result = owner->appendSamples(operation, samples);
        if (result == Gpu::Result::Success) result = owner->updateShape(operation, *shape);
        if (result == Gpu::Result::Success) result = owner->commitStroke(operation);
        if (result != Gpu::Result::Success) {
            owner->cancelStroke(operation);
            warnKrita << "A GPU figure stroke was rejected:" << int(result);
        }
    }
};


KisFigurePaintingStroke::KisFigurePaintingStroke(
    const KUndo2MagicString &name,
    KisImageWSP image,
    KisNodeSP currentNode,
    KoCanvasResourceProvider *resourceManager,
    KisFigurePaintingOptions::StrokeStyle strokeStyle,
    KisFigurePaintingOptions::FillStyle fillStyle,
    QTransform fillTransform)
{
    if (image && image->gpuImage()) {
        m_gpuStroke = std::make_unique<GpuStroke>();
        m_gpuStroke->image = image;
        m_gpuStroke->node = currentNode;
        m_gpuStroke->owner = image->gpuImage();
        m_gpuStroke->resources = captureGpuBrushResources(image, currentNode, resourceManager);
        m_gpuStroke->selection = image->globalSelection();
        m_gpuStroke->strokeStyle = strokeStyle;
        m_gpuStroke->fillStyle = fillStyle;
        if (strokeStyle == KisFigurePaintingOptions::StrokeStyleBackground)
            m_gpuStroke->setColor(resourceManager->backgroundColor());
        return;
    }
    m_strokesFacade = image.data();

    m_resources =
        new KisResourcesSnapshot(image,
                                 currentNode,
                                 resourceManager->canvasResourcesInterface());

    setupPaintStyles(m_resources, strokeStyle, fillStyle, fillTransform);

    KisFreehandStrokeInfo *strokeInfo = new KisFreehandStrokeInfo();

    KisStrokeStrategy *stroke =
        new FreehandStrokeStrategy(m_resources, strokeInfo, name);

    m_strokeId = m_strokesFacade->startStroke(stroke);
}

void KisFigurePaintingStroke::setupPaintStyles(
    KisResourcesSnapshotSP resources,
    KisFigurePaintingOptions::StrokeStyle strokeStyle,
    KisFigurePaintingOptions::FillStyle fillStyle,
    QTransform fillTransform)
{
    using namespace KisFigurePaintingOptions;

    const KoColor fgColor = resources->currentFgColor();
    const KoColor bgColor = resources->currentBgColor();

    switch (strokeStyle) {
    case StrokeStyleNone:
        resources->setStrokeStyle(KisPainter::StrokeStyleNone);
        break;
    case StrokeStyleForeground:
        resources->setStrokeStyle(KisPainter::StrokeStyleBrush);
        break;
    case StrokeStyleBackground:
        resources->setStrokeStyle(KisPainter::StrokeStyleBrush);

        resources->setFGColorOverride(bgColor);
        resources->setBGColorOverride(fgColor);

        if (fillStyle == FillStyleForegroundColor) {
            fillStyle = FillStyleBackgroundColor;
        } else if (fillStyle == FillStyleBackgroundColor) {
            fillStyle = FillStyleForegroundColor;
        }

        break;
    };

    switch (fillStyle) {
    case FillStyleForegroundColor:
        resources->setFillStyle(KisPainter::FillStyleForegroundColor);
        break;
    case FillStyleBackgroundColor:
        resources->setFillStyle(KisPainter::FillStyleBackgroundColor);
        break;
    case FillStylePattern:
        resources->setFillStyle(KisPainter::FillStylePattern);
        break;
    case FillStyleNone:
        resources->setFillStyle(KisPainter::FillStyleNone);
        break;
    }

    resources->setFillTransform(fillTransform);
}

KisFigurePaintingStroke::~KisFigurePaintingStroke()
{
    if (m_gpuStroke) {
        m_gpuStroke->finish();
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new KisAsynchronousStrokeUpdateHelper::UpdateData(true));
    m_strokesFacade->endStroke(m_strokeId);
}

void KisFigurePaintingStroke::paintLine(const KisPaintInformation &pi0,
                                        const KisPaintInformation &pi1)
{
    if (m_gpuStroke) {
        // The public line contract paints the foreground brush for None.
        if (m_gpuStroke->strokeStyle == KisFigurePaintingOptions::StrokeStyleNone)
            m_gpuStroke->strokeStyle = KisFigurePaintingOptions::StrokeStyleForeground;
        const auto sample = [](const KisPaintInformation &info) {
            return Gpu::InputSample{info.pos(), info.pressure(), info.xTilt(), info.yTilt(),
                                    info.rotation(), quint64(info.currentTime())};
        };
        m_gpuStroke->setFigure({Gpu::ShapeKind::Line, pi0.pos(), pi1.pos(), 0}, {sample(pi0), sample(pi1)});
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         pi0, pi1));
}

void KisFigurePaintingStroke::paintPolyline(const vQPointF &points)
{
    if (m_gpuStroke) {
        if (points.size() < 2) return;
        QList<Gpu::InputSample> samples;
        samples.reserve(points.size());
        for (const auto &point : points) samples.append({point, 1});
        m_gpuStroke->setFigure({Gpu::ShapeKind::Polyline, points.first(), points.last(), 0}, samples);
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::POLYLINE,
                                         points));
}

void KisFigurePaintingStroke::paintPolygon(const vQPointF &points)
{
    if (m_gpuStroke) {
        auto vertices = points;
        if (vertices.size() > 1 && vertices.first() == vertices.last()) vertices.removeLast();
        if (vertices.size() < 2) return;
        QList<Gpu::InputSample> samples;
        samples.reserve(vertices.size());
        for (const auto &point : vertices) samples.append({point, 1});
        m_gpuStroke->setFigure({Gpu::ShapeKind::Polygon, vertices.first(), vertices.last(), 0}, samples);
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::POLYGON,
                                         points));
}

void KisFigurePaintingStroke::paintRect(const QRectF &rect)
{
    if (m_gpuStroke) {
        m_gpuStroke->setFigure({Gpu::ShapeKind::Rectangle, rect.topLeft(), rect.bottomRight(), 0},
                              {{rect.topLeft(), 1}, {rect.bottomRight(), 1}});
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::RECT,
                                         rect));
}

void KisFigurePaintingStroke::paintEllipse(const QRectF &rect)
{
    if (m_gpuStroke) {
        m_gpuStroke->setFigure({Gpu::ShapeKind::Ellipse, rect.topLeft(), rect.bottomRight(), 0},
                              {{rect.topLeft(), 1}, {rect.bottomRight(), 1}});
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::ELLIPSE,
                                         rect));
}

void KisFigurePaintingStroke::paintPainterPath(const QPainterPath &path)
{
    if (m_gpuStroke) {
        m_gpuStroke->resources.diagnostic = QStringLiteral("GPU painter paths require a path operation.");
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::PAINTER_PATH,
                                         path));
}

void KisFigurePaintingStroke::setFGColorOverride(const KoColor &color)
{
    if (m_gpuStroke) {
        m_gpuStroke->setColor(color);
        return;
    }
    m_resources->setFGColorOverride(color);
}

void KisFigurePaintingStroke::setBGColorOverride(const KoColor &color)
{
    if (m_gpuStroke) return;
    m_resources->setBGColorOverride(color);
}

void KisFigurePaintingStroke::setSelectionOverride(KisSelectionSP m_selection)
{
    if (m_gpuStroke) {
        m_gpuStroke->selection = m_selection;
        return;
    }
    m_resources->setSelectionOverride(m_selection);
}

void KisFigurePaintingStroke::setBrush(const KisPaintOpPresetSP &brush)
{
    if (m_gpuStroke) {
        if (!brush || brush->paintOp().id() != QLatin1String("roundmarker"))
            m_gpuStroke->resources.diagnostic = QStringLiteral("GPU figure painting requires the circular brush.");
        else m_gpuStroke->resources.brush.diameter = brush->settings()->paintOpSize();
        return;
    }
    m_resources->setBrush(brush);
}

void KisFigurePaintingStroke::paintPainterPathQPen(const QPainterPath path,
                                                   const QPen &pen,
                                                   const KoColor &color)
{
    if (m_gpuStroke) {
        m_gpuStroke->resources.diagnostic = QStringLiteral("GPU pen paths require a pen operation.");
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::QPAINTER_PATH,
                                         path, pen, color));
}

void KisFigurePaintingStroke::paintPainterPathQPenFill(
    const QPainterPath path,
    const QPen &pen,
    const KoColor &color)
{
    if (m_gpuStroke) {
        m_gpuStroke->resources.diagnostic = QStringLiteral("GPU filled pen paths require a pen fill operation.");
        return;
    }
    m_strokesFacade->addJob(m_strokeId,
        new FreehandStrokeStrategy::Data(0,
                                         FreehandStrokeStrategy::Data::QPAINTER_PATH_FILL,
                                         path, pen, color));
}
