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
    KoColor foregroundColor;
    KoColor backgroundColor;
    KisSelectionSP selection;
    KisFigurePaintingOptions::StrokeStyle strokeStyle;
    KisFigurePaintingOptions::FillStyle fillStyle;
    std::optional<Gpu::Shape> shape;
    QList<Gpu::InputSample> samples;

    void setFigure(const Gpu::Shape &value, const QList<Gpu::InputSample> &input)
    {
        if (!resources.diagnostic.isEmpty()) return;
        const auto &document = owner->operations();
        const auto limits = document.limits();
        if (input.size() > limits.samplesPerStroke - samples.size() ||
            input.size() > limits.samplesPerDocument - document.snapshot()->sampleCount - samples.size()) {
            resources.diagnostic = QStringLiteral("GPU figure input exceeds the document input budget.");
            return;
        }
        if (shape) {
            const auto isPathOrLine = [](Gpu::ShapeKind kind) {
                return kind == Gpu::ShapeKind::Line || kind == Gpu::ShapeKind::Path;
            };
            if (fillStyle != KisFigurePaintingOptions::FillStyleNone ||
                !isPathOrLine(shape->kind) || !isPathOrLine(value.kind)) {
                resources.diagnostic = QStringLiteral("This GPU figure group requires separate fill or shape operations.");
                return;
            }
            if (shape->kind == Gpu::ShapeKind::Line) {
                Gpu::Shape path;
                path.kind = Gpu::ShapeKind::Path;
                path.pathCommands = {Gpu::PathCommand::MoveTo, Gpu::PathCommand::LineTo};
                shape = path;
            }
            // Each call starts an independent subpath. Keeping both endpoint
            // samples preserves sensors even when consecutive positions match.
            if (value.kind == Gpu::ShapeKind::Line)
                shape->pathCommands.append({Gpu::PathCommand::MoveTo, Gpu::PathCommand::LineTo});
            else {
                shape->pathCommands.append(value.pathCommands);
                const auto offset = samples.size();
                for (const auto &call : value.pathCalls)
                    shape->pathCalls.append({offset + call.first, offset + call.second});
            }
            samples.append(input);
            return;
        }
        shape = value;
        samples = input;
    }

    void setColor(const KoColor &value)
    {
        foregroundColor = value;
        const auto color = value.toQColor();
        resources.brush.rgba = {quint8(color.red()), quint8(color.green()), quint8(color.blue()), 255};
    }

    void finish()
    {
        if (fillStyle == KisFigurePaintingOptions::FillStylePattern)
            resources.diagnostic = QStringLiteral("GPU pattern fills require a pattern resource operation.");
        if (selection)
            resources.diagnostic = QStringLiteral("GPU figure painting requires selection coverage.");
        if (!resources.diagnostic.isEmpty()) {
            warnKrita << resources.diagnostic;
            return;
        }
        if (!shape) return;
        Gpu::FigureStyle style;
        style.outline = strokeStyle != KisFigurePaintingOptions::StrokeStyleNone;
        if (fillStyle != KisFigurePaintingOptions::FillStyleNone) {
            const auto color = (fillStyle == KisFigurePaintingOptions::FillStyleForegroundColor ?
                                foregroundColor : backgroundColor).toQColor();
            style.fill = std::array<quint8, 4>{quint8(color.red()), quint8(color.green()), quint8(color.blue()), 255};
        }
        const auto result = resources.transparencyMask
            ? owner->paintTransparencyMaskFigure(resources.layer, resources.brush, samples, *shape, style, 0)
            : owner->paintFigure(resources.layer, resources.brush, samples, *shape, style, 0);
        if (result != Gpu::Result::Success) {
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
        m_gpuStroke->foregroundColor = resourceManager->foregroundColor();
        m_gpuStroke->backgroundColor = resourceManager->backgroundColor();
        if (strokeStyle == KisFigurePaintingOptions::StrokeStyleBackground) {
            const auto foreground = m_gpuStroke->foregroundColor;
            m_gpuStroke->setColor(m_gpuStroke->backgroundColor);
            m_gpuStroke->backgroundColor = foreground;
            if (fillStyle == KisFigurePaintingOptions::FillStyleForegroundColor)
                m_gpuStroke->fillStyle = KisFigurePaintingOptions::FillStyleBackgroundColor;
            else if (fillStyle == KisFigurePaintingOptions::FillStyleBackgroundColor)
                m_gpuStroke->fillStyle = KisFigurePaintingOptions::FillStyleForegroundColor;
        }
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
        const auto &document = m_gpuStroke->owner->operations();
        const auto limits = document.limits();
        if (path.elementCount() > limits.samplesPerStroke ||
            path.elementCount() > limits.samplesPerDocument - document.snapshot()->sampleCount) {
            m_gpuStroke->resources.diagnostic = QStringLiteral("GPU path input exceeds the document input budget.");
            return;
        }
        Gpu::Shape shape;
        shape.kind = Gpu::ShapeKind::Path;
        shape.windingFill = path.fillRule() == Qt::WindingFill;
        QList<Gpu::InputSample> samples;
        samples.reserve(path.elementCount());
        for (int i = 0; i < path.elementCount(); ++i) {
            const auto element = path.elementAt(i);
            if (element.type == QPainterPath::MoveToElement || element.type == QPainterPath::LineToElement) {
                shape.pathCommands.append(element.type == QPainterPath::MoveToElement ?
                                            Gpu::PathCommand::MoveTo : Gpu::PathCommand::LineTo);
                samples.append({{element.x, element.y}, 1});
            } else if (element.type == QPainterPath::CurveToElement && path.elementCount() - i >= 3) {
                shape.pathCommands.append(Gpu::PathCommand::CubicTo);
                for (int point = 0; point < 3; ++point) {
                    const auto value = path.elementAt(i + point);
                    samples.append({{value.x, value.y}, 1});
                }
                i += 2;
            } else {
                m_gpuStroke->resources.diagnostic = QStringLiteral("GPU path commands are incomplete.");
                return;
            }
        }
        if (!samples.isEmpty()) shape.pathCalls = {{0, samples.size()}};
        m_gpuStroke->setFigure(shape, samples);
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
    if (m_gpuStroke) {
        m_gpuStroke->backgroundColor = color;
        return;
    }
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
