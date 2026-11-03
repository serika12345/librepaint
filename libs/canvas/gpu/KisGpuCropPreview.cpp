/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCropPreview_p.h"
#include "KisGpuViewTransform_p.h"
#include <QFile>
#include <QPainterPathStroker>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace LibrePaint::Canvas::Gpu::Detail {
namespace {
struct Float4 { float x, y, z, w = 0; };
static_assert(sizeof(Float4) == 16);
struct Uniforms
{
    float bounds[4];
    float size[2];
    quint32 count;
    quint32 mode = 0;
    float color[4];
    float clip[8]{};
};
static_assert(sizeof(Uniforms) == 80);
bool finitePoint(QPointF point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y()) &&
        std::abs(point.x()) <= 2147483648.0 && std::abs(point.y()) <= 2147483648.0;
}
bool finiteRect(QRectF rect)
{
    return finitePoint(rect.topLeft()) && finitePoint(rect.bottomRight());
}

// Keep the original directed edges. Splitting only at viewport boundaries
// bounds each upload while preserving winding through overlapping handles.
bool outlineEdges(const QPainterPath &path, QSize viewport, std::vector<Float4> &rows, bool outlineGrid)
{
    const auto outlinePoint = [&](QPointF point) {
        return outlineGrid ? QPointF(qRound64(point.x() * 64) / 64.0, qRound64(point.y() * 64) / 64.0) : point;
    };
    for (const auto &polygon : path.toSubpathPolygons()) {
        for (qsizetype i = 1; i < polygon.size(); ++i) {
            const QLineF edge(outlinePoint(polygon[i - 1]), outlinePoint(polygon[i]));
            if (edge.dy() == 0) continue;
            const qreal top = std::max(qreal(0), std::min(edge.y1(), edge.y2()));
            const qreal bottom = std::min(qreal(viewport.height()), std::max(edge.y1(), edge.y2()));
            if (bottom <= top) continue;
            QList<qreal> heights{top, bottom};
            if (edge.dx() != 0) {
                for (const qreal x : {qreal(0), qreal(viewport.width())}) {
                    const qreal y = edge.y1() + (x - edge.x1()) * edge.dy() / edge.dx();
                    if (y > top && y < bottom) heights.append(y);
                }
            }
            std::sort(heights.begin(), heights.end());
            heights.erase(std::unique(heights.begin(), heights.end()), heights.end());
            const auto x = [&](qreal y) {
                return float(std::clamp(edge.x1() + (y - edge.y1()) * edge.dx() / edge.dy(),
                                        qreal(0), qreal(viewport.width())));
            };
            for (qsizetype band = 1; band < heights.size(); ++band) {
                if (rows.size() >= 512) return false;
                const qreal first = heights[band - 1], last = heights[band];
                rows.push_back({x(first), x(last), float(first), float(edge.dy() > 0 ? last - first : first - last)});
            }
        }
    }
    return true;
}
}
CropPreviewRenderer::CropPreviewRenderer(Image::Gpu::DevicePtr device) : m_device(std::move(device)) {}

bool CropPreviewRenderer::preparePipeline(quint32 format)
{
    if (!m_shader) {
        QFile file(QStringLiteral(":/librepaint/canvas/shaders/crop_preview.wgsl"));
        if (!file.open(QIODevice::ReadOnly)) return false;
        const auto source = file.readAll();
        WGPUShaderSourceWGSL wgsl{};
        wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgsl.code = {source.constData(), size_t(source.size())};
        WGPUShaderModuleDescriptor descriptor{};
        descriptor.nextInChain = &wgsl.chain;
        m_shader.reset(wgpuDeviceCreateShaderModule(m_device->nativeDevice(), &descriptor));
        if (!m_shader) return false;
    }
    if (m_pipeline && m_format == format) return true;
    WGPUBlendState blend{};
    blend.color = {WGPUBlendOperation_Add, WGPUBlendFactor_One, WGPUBlendFactor_OneMinusSrcAlpha};
    blend.alpha = blend.color;
    WGPUColorTargetState color{};
    color.format = WGPUTextureFormat(format);
    color.blend = &blend;
    color.writeMask = WGPUColorWriteMask_All;
    WGPUFragmentState fragment{};
    fragment.module = m_shader.get();
    fragment.entryPoint = {"fragmentMain", WGPU_STRLEN};
    fragment.targetCount = 1;
    fragment.targets = &color;
    WGPURenderPipelineDescriptor descriptor{};
    descriptor.vertex.module = m_shader.get();
    descriptor.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
    descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    descriptor.primitive.frontFace = WGPUFrontFace_CCW;
    descriptor.primitive.cullMode = WGPUCullMode_None;
    descriptor.multisample.count = 1;
    descriptor.multisample.mask = 0xffffffff;
    descriptor.fragment = &fragment;
    Pipeline next(wgpuDeviceCreateRenderPipeline(m_device->nativeDevice(), &descriptor));
    if (!next || !m_device->diagnostic().isEmpty()) return false;
    Layout layout(wgpuRenderPipelineGetBindGroupLayout(next.get(), 0));
    if (!layout) return false;
    m_pipeline = std::move(next);
    m_layout = std::move(layout);
    m_format = format;
    return true;
}

bool CropPreviewRenderer::prepare(const CropPreview &crop, const PresentationTarget &target,
                                 std::vector<Draw> &draws, QString &diagnostic)
{
    if (!finiteRect(crop.image) || !finiteRect(crop.crop) || !validDocumentTransform(crop.toTarget) ||
        !crop.shade.isValid() || !crop.handleFill.isValid() || !crop.stroke.isValid() ||
        !std::isfinite(crop.thickness) || crop.thickness <= 0 || crop.thickness > 256 ||
        crop.handles.elementCount() > 40 || crop.guides.size() > 8) {
        diagnostic = QStringLiteral("GPU crop preview requires bounded finite crop geometry, display colors and a positive cosmetic width");
        return false;
    }
    for (int i = 0; i < crop.handles.elementCount(); ++i) {
        const auto element = crop.handles.elementAt(i);
        if (!finitePoint({element.x, element.y}) || element.type == QPainterPath::CurveToElement ||
            element.type == QPainterPath::CurveToDataElement) {
            diagnostic = QStringLiteral("GPU crop handles require straight finite rectangular contours");
            return false;
        }
    }
    for (const auto &line : crop.guides) {
        if (!finitePoint(line.p1()) || !finitePoint(line.p2())) {
            diagnostic = QStringLiteral("GPU crop guides require finite bounded endpoints");
            return false;
        }
    }
    if (!preparePipeline(target.format)) {
        diagnostic = QStringLiteral("GPU crop preview pipeline could not be prepared: %1").arg(m_device->diagnostic());
        return false;
    }
    const auto uniforms = [&](QRectF bounds, QColor color, bool clip) {
        const auto rgba = qPremultiply(color.rgba());
        Uniforms values{{float(bounds.left()), float(bounds.top()), float(bounds.right()), float(bounds.bottom())},
                        {float(target.size.width()), float(target.size.height())}, 0, 0,
                        {float(qRed(rgba)), float(qGreen(rgba)), float(qBlue(rgba)), float(qAlpha(rgba))}};
        if (clip) {
            if (crop.toTarget.type() <= QTransform::TxScale) {
                const auto first = crop.toTarget.map(crop.crop.topLeft()).toPoint();
                const auto second = crop.toTarget.map(crop.crop.bottomRight()).toPoint();
                values.mode = 2;
                values.clip[0] = std::min(first.x(), second.x());
                values.clip[1] = std::min(first.y(), second.y());
                values.clip[2] = std::max(first.x(), second.x());
                values.clip[3] = std::max(first.y(), second.y());
            } else {
                values.mode = 4;
                const QPointF points[]{crop.crop.topLeft(), crop.crop.topRight(), crop.crop.bottomRight(), crop.crop.bottomLeft()};
                for (int i = 0; i < 4; ++i) {
                    const auto point = crop.toTarget.map(points[i]);
                    values.clip[2 * i] = float(qRound64(point.x() * 64) / 64.0);
                    values.clip[2 * i + 1] = float(qRound64(point.y() * 64) / 64.0);
                }
            }
        }
        return values;
    };
    const auto upload = [&](const std::vector<Float4> &rows, const Uniforms &values, quint32 instances) {
        Draw draw;
        draw.instances = instances;
        WGPUBufferDescriptor buffer{};
        buffer.size = sizeof(values);
        buffer.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        draw.uniform.reset(wgpuDeviceCreateBuffer(m_device->nativeDevice(), &buffer));
        buffer.size = rows.size() * sizeof(Float4);
        buffer.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
        draw.geometry.reset(wgpuDeviceCreateBuffer(m_device->nativeDevice(), &buffer));
        if (!draw.uniform || !draw.geometry) return false;
        WGPUBindGroupEntry entries[2]{};
        entries[0].binding = 0;
        entries[0].buffer = draw.uniform.get();
        entries[0].size = sizeof(values);
        entries[1].binding = 1;
        entries[1].buffer = draw.geometry.get();
        entries[1].size = buffer.size;
        WGPUBindGroupDescriptor bindings{};
        bindings.layout = m_layout.get();
        bindings.entryCount = 2;
        bindings.entries = entries;
        draw.bindings.reset(wgpuDeviceCreateBindGroup(m_device->nativeDevice(), &bindings));
        if (!draw.bindings || !m_device->diagnostic().isEmpty()) return false;
        wgpuQueueWriteBuffer(m_device->queue(), draw.uniform.get(), 0, &values, sizeof(values));
        wgpuQueueWriteBuffer(m_device->queue(), draw.geometry.get(), 0, rows.data(), buffer.size);
        draws.push_back(std::move(draw));
        return true;
    };
    const auto append = [&](const QPainterPath &path, QColor color, bool clip = false, bool linePrimitive = false) {
        if (color.alpha() == 0) return true;
        std::vector<Float4> rows;
        if (!outlineEdges(path, target.size, rows, !linePrimitive)) return false;
        if (rows.empty()) return true;
        const auto bounds = path.boundingRect().intersected(QRectF(QPointF(), target.size)).adjusted(-1, -1, 1, 1);
        auto values = uniforms(bounds, color, clip);
        values.count = quint32(rows.size());
        if (!linePrimitive) values.mode |= 32;
        if (path.fillRule() == Qt::OddEvenFill) values.mode |= 8;
        if (linePrimitive) values.mode |= 16;
        return upload(rows, values, 1);
    };
    const auto appendThin = [&](const QList<QLineF> &lines, bool caps, bool clip) {
        if (crop.stroke.alpha() == 0) return true;
        std::vector<Float4> rows;
        for (const auto &line : lines) {
            QPointF first = line.p1(), second = line.p2();
            bool visible = true;
            // Bound vector coordinates before fixed-point line interpolation.
            for (int axis = 0; axis < 2 && visible; ++axis) {
                const auto coord = [&](QPointF p) { return axis ? p.y() : p.x(); };
                const qreal minimum = -1, maximum = (axis ? target.size.height() : target.size.width()) + 1;
                for (int end = 0; end < 2; ++end) {
                    auto &point = end ? second : first;
                    const auto other = end ? first : second;
                    const qreal value = coord(point), last = coord(other);
                    if (value < minimum || value > maximum) {
                        const qreal boundary = value < minimum ? minimum : maximum;
                        if ((value < minimum && last <= minimum) || (value > maximum && last >= maximum)) { visible = false; break; }
                        point += (other - point) * ((boundary - value) / (last - value));
                        if (axis) point.setY(boundary); else point.setX(boundary);
                    }
                }
            }
            if (!visible) continue;
            qint32 x1 = qint32(first.x() * 64), y1 = qint32(first.y() * 64);
            qint32 x2 = qint32(second.x() * 64), y2 = qint32(second.y() * 64);
            const bool vertical = std::abs(x2 - x1) < std::abs(y2 - y1);
            if (vertical) { std::swap(x1, y1); std::swap(x2, y2); }
            if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
            if (x1 == x2) continue;
            const qint64 increment = qint64(y2 - y1) * 65536 / (x2 - x1);
            qint64 origin = qint64(y1 - 32) * 1024 - (((x1 & 63) - 32) * increment >> 6);
            if (caps) { x1 -= 32; x2 += 32; origin -= increment >> 1; }
            const qint32 metadata[]{x1, x2, qint32(origin), qint32(increment)};
            Float4 packed;
            std::memcpy(&packed, metadata, sizeof(packed));
            rows.push_back(packed);
            rows.push_back({float(std::min(first.x(), second.x()) - 2), float(std::min(first.y(), second.y()) - 2),
                            float(std::max(first.x(), second.x()) + 2), float(std::max(first.y(), second.y()) + 2)});
            const qint32 orientation[]{vertical ? 1 : 0, 0, 0, 0};
            std::memcpy(&packed, orientation, sizeof(packed));
            rows.push_back(packed);
        }
        if (rows.empty()) return true;
        auto values = uniforms({}, crop.stroke, clip);
        values.mode |= 1;
        values.count = quint32(rows.size() / 3);
        return upload(rows, values, values.count);
    };
    QPainterPath shade;
    shade.addRect(crop.image);
    shade.addRect(crop.crop);
    const auto handles = crop.toTarget.map(crop.handles);
    bool ok = append(crop.toTarget.map(shade), crop.shade) && append(handles, crop.handleFill);
    if (crop.thickness <= 1) {
        QList<QLineF> edges;
        for (const auto &polygon : handles.toSubpathPolygons())
            for (qsizetype i = 1; i < polygon.size(); ++i) edges.append(QLineF(polygon[i - 1], polygon[i]));
        QList<QLineF> guides;
        for (const auto &line : crop.guides) guides.append(crop.toTarget.map(line));
        ok = ok && appendThin(edges, false, false) && appendThin(guides, true, true);
    } else {
        QPainterPathStroker stroker;
        stroker.setWidth(crop.thickness);
        stroker.setCapStyle(Qt::SquareCap);
        stroker.setJoinStyle(Qt::BevelJoin);
        ok = ok && append(stroker.createStroke(handles), crop.stroke);
        for (const auto &line : crop.guides) {
            const auto mapped = crop.toTarget.map(line);
            const qreal length = mapped.length();
            if (length == 0) continue;
            const qreal relativeWidth = crop.thickness / length;
            const QPointF along = (0.5 * relativeWidth) * (mapped.p2() - mapped.p1());
            QPointF across(-along.y(), along.x());
            QPointF first = mapped.p1() - along, second = mapped.p2() + along;
            const bool linePrimitive = crop.toTarget.type() <= QTransform::TxRotate;
            if (linePrimitive) {
                QRect rasterClip(QPoint(), target.size);
                if (crop.toTarget.type() <= QTransform::TxScale) {
                    const auto a = crop.toTarget.map(crop.crop.topLeft()).toPoint();
                    const auto b = crop.toTarget.map(crop.crop.bottomRight()).toPoint();
                    rasterClip &= QRect(std::min(a.x(), b.x()), std::min(a.y(), b.y()),
                                        std::abs(b.x() - a.x()), std::abs(b.y() - a.y()));
                } else {
                    const QPolygonF clip{crop.toTarget.map(crop.crop.topLeft()), crop.toTarget.map(crop.crop.topRight()),
                                         crop.toTarget.map(crop.crop.bottomRight()), crop.toTarget.map(crop.crop.bottomLeft())};
                    const QPolygonF viewport(QRectF(QPointF(), target.size));
                    rasterClip &= clip.intersected(viewport).boundingRect().toAlignedRect();
                }
                if (rasterClip.isEmpty()) continue;
                // The line rasterizer clips the capped centerline before
                // snapping its corners, including at the viewport boundary.
                const QPointF extent = QPointF(std::abs(mapped.dy()), std::abs(mapped.dx())) * relativeWidth * 0.5;
                const QRectF bounds(QPointF(rasterClip.topLeft()) - extent,
                                    QPointF(rasterClip.bottomRight() + QPoint(1, 1)) + extent);
                bool visible = true;
                for (int axis = 0; axis < 2 && visible; ++axis) {
                    const auto coordinate = [&](QPointF p) { return axis ? p.y() : p.x(); };
                    const qreal minimum = axis ? bounds.top() : bounds.left();
                    const qreal maximum = axis ? bounds.bottom() : bounds.right();
                    for (int end = 0; end < 2; ++end) {
                        auto &point = end ? second : first;
                        const auto other = end ? first : second;
                        const qreal value = coordinate(point), last = coordinate(other);
                        if (value < minimum || value > maximum) {
                            const qreal boundary = value < minimum ? minimum : maximum;
                            if ((value < minimum && last <= minimum) || (value > maximum && last >= maximum)) { visible = false; break; }
                            if (axis) {
                                point.rx() += (point.x() - other.x()) / (point.y() - other.y()) * (boundary - point.y());
                                point.setY(boundary);
                            } else {
                                point.ry() += (point.y() - other.y()) / (point.x() - other.x()) * (boundary - point.x());
                                point.setX(boundary);
                            }
                        }
                    }
                }
                if (!visible || first == second) continue;
                if (first.y() > second.y()) std::swap(first, second);
                const auto delta = second - first;
                const auto originalDelta = mapped.p1() - mapped.p2();
                const qreal adjustedWidth = relativeWidth * std::sqrt(QPointF::dotProduct(originalDelta, originalDelta) /
                                                                     QPointF::dotProduct(delta, delta));
                across = QPointF(-delta.y(), delta.x()) * (0.5 * adjustedWidth);
            }
            QPolygonF corners{first - across, second - across, second + across, first + across};
            if (linePrimitive && std::abs(second.x() - first.x()) >= 1.0 / 64 && std::abs(second.y() - first.y()) >= 1.0 / 64)
                for (auto &point : corners)
                    point = QPointF(std::floor(point.x() * 64) / 64, std::floor(point.y() * 64) / 64);
            QPainterPath path;
            path.setFillRule(Qt::WindingFill);
            path.addPolygon(corners);
            path.closeSubpath();
            ok = ok && append(path, crop.stroke, true, linePrimitive);
        }
    }
    if (!ok) diagnostic = QStringLiteral("GPU crop preview vector capacity or resource allocation failed: %1").arg(m_device->diagnostic());
    return ok;
}
void CropPreviewRenderer::draw(WGPURenderPassEncoder pass, const std::vector<Draw> &draws) const
{
    if (draws.empty()) return;
    wgpuRenderPassEncoderSetPipeline(pass, m_pipeline.get());
    for (const auto &draw : draws) {
        wgpuRenderPassEncoderSetBindGroup(pass, 0, draw.bindings.get(), 0, nullptr);
        wgpuRenderPassEncoderDraw(pass, 6, draw.instances, 0, 0);
    }
}
}
