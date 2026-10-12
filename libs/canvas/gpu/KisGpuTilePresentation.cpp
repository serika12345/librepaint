/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTilePresentation.h"
#include <QFile>
#include <QMatrix4x4>
#include <QThread>
#include <rhi/qrhi.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Krita::Canvas::Gpu {
namespace {
template<class T> struct ResourceDeleter
{
    QRhi *rhi;
    void operator()(T *value) const
    {
        if (!value) return;
        if (rhi->isRecordingFrame()) value->deleteLater();
        else delete value;
    }
};
template<class T> using Resource = std::unique_ptr<T, ResourceDeleter<T>>;
template<class T> Resource<T> own(QRhi &rhi, T *value) { return {value, {&rhi}}; }

QShader shader(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

struct Uniforms { float matrix[16]; float clip[4]; };
struct Draw
{
    Resource<QRhiBuffer> uniform;
    Resource<QRhiShaderResourceBindings> bindings;
    Uniforms values;
};
}

struct TilePresentation::Private
{
    Image::Gpu::DevicePtr device;
    QRhi &rhi;
    qsizetype maximumTiles;
    Resource<QRhiBuffer> layoutUniform, layoutPixels;
    Resource<QRhiShaderResourceBindings> layout;
    Resource<QRhiRenderPassDescriptor> pass;
    Resource<QRhiGraphicsPipeline> pipeline;
    QShader vertex, fragment;
    QString error;

    Private(Image::Gpu::DevicePtr device, qsizetype maximumTiles)
        : device(std::move(device)), rhi(this->device->rhi()), maximumTiles(maximumTiles),
          layoutUniform(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(Uniforms)))),
          layoutPixels(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, sizeof(quint32)))),
          layout(own(rhi, rhi.newShaderResourceBindings())),
          pass(own<QRhiRenderPassDescriptor>(rhi, nullptr)), pipeline(own<QRhiGraphicsPipeline>(rhi, nullptr)),
          vertex(shader(QStringLiteral(":/librepaint/canvas/shaders/tile.vert.qsb"))),
          fragment(shader(QStringLiteral(":/librepaint/canvas/shaders/tile.frag.qsb")))
    {
        if (!vertex.isValid() || !fragment.isValid() || !layoutUniform->create() || !layoutPixels->create()) {
            error = QStringLiteral("GPU tile presentation resources could not be initialized");
            return;
        }
        layout->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, layoutUniform.get()),
            QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::FragmentStage, layoutPixels.get())
        });
        if (!layout->create()) error = QStringLiteral("GPU tile presentation layout could not be created");
    }

    bool preparePipeline(QRhiRenderTarget &target)
    {
        if (pipeline && pipeline->sampleCount() == target.sampleCount() && pass->isCompatible(target.renderPassDescriptor())) return true;
        auto nextPass = own(rhi, target.renderPassDescriptor()->newCompatibleRenderPassDescriptor());
        auto next = own(rhi, rhi.newGraphicsPipeline());
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        next->setTargetBlends({blend});
        next->setShaderStages({{QRhiShaderStage::Vertex, vertex}, {QRhiShaderStage::Fragment, fragment}});
        next->setVertexInputLayout({});
        next->setShaderResourceBindings(layout.get());
        next->setSampleCount(target.sampleCount());
        next->setRenderPassDescriptor(nextPass.get());
        if (!next->create()) return false;
        pipeline = std::move(next);
        pass = std::move(nextPass);
        return true;
    }
};

TilePresentation::TilePresentation(Image::Gpu::DevicePtr device, qsizetype maximumTiles)
{
    if (!device || device->rhi().thread() != QThread::currentThread() || maximumTiles <= 0) {
        throw std::invalid_argument("invalid GPU presentation owner or tile limit");
    }
    m_d = std::make_unique<Private>(std::move(device), maximumTiles);
}
TilePresentation::~TilePresentation() = default;

PresentationOutcome TilePresentation::record(QRhiCommandBuffer &commands,
    const Image::Gpu::ProjectedSnapshot &projection, QRhiRenderTarget &target,
    const QTransform &transform, QColor clearColor)
{
    auto &d = *m_d;
    if (d.rhi.thread() != QThread::currentThread() || !d.rhi.isRecordingFrame() ||
        commands.rhi() != &d.rhi || target.rhi() != &d.rhi || !projection ||
        !target.renderPassDescriptor() || target.pixelSize().isEmpty() ||
        !transform.isAffine() || !transform.isInvertible() || !clearColor.isValid()) {
        return {false, QStringLiteral("Presentation requires an owning GPU frame and an invertible affine transform"), 0};
    }
    for (const double value : {transform.m11(), transform.m12(), transform.m21(), transform.m22(), transform.dx(), transform.dy()}) {
        if (!std::isfinite(value) || std::abs(value) > 8388608) {
            return {false, QStringLiteral("Presentation transform is outside the supported finite range"), 0};
        }
    }
    if (!d.error.isEmpty() || d.rhi.isDeviceLost()) return {false, d.error.isEmpty() ? QStringLiteral("GPU device lost") : d.error, 0};

    const auto tiles = projection->tiles();
    const QRectF viewport(QPointF(0, 0), target.pixelSize());
    const QRectF region(projection->region());
    QList<Image::Gpu::TileView> visible;
    constexpr int size = Image::Gpu::TileRenderer::TileSize;
    for (const auto &tile : tiles) {
        if (!tile.pixels || tile.pixels->rhi() != &d.rhi) return {false, QStringLiteral("Projection belongs to another GPU"), 0};
        const QRectF tileRect(tile.coordinate * size, QSize(size, size));
        if (transform.mapRect(tileRect.intersected(region)).intersects(viewport)) visible.append(tile);
    }
    if (visible.size() > d.maximumTiles) return {false, QStringLiteral("Visible GPU tile limit exceeded"), 0};
    if (!visible.isEmpty() && !d.preparePipeline(target)) return {false, QStringLiteral("GPU presentation pipeline could not be created"), 0};

    QMatrix4x4 view;
    view.ortho(0, target.pixelSize().width(), target.pixelSize().height(), 0, -1, 1);
    view = d.rhi.clipSpaceCorrMatrix() * view * QMatrix4x4(transform);
    std::vector<Draw> draws;
    draws.reserve(visible.size());
    for (const auto &tile : visible) {
        Draw draw;
        draw.uniform = own(d.rhi, d.rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(Uniforms)));
        draw.bindings = own(d.rhi, d.rhi.newShaderResourceBindings());
        auto matrix = view;
        const QPoint origin = tile.coordinate * size;
        matrix.translate(origin.x(), origin.y());
        matrix.scale(size, size);
        std::copy(matrix.constData(), matrix.constData() + 16, draw.values.matrix);
        draw.values.clip[0] = float(region.left() - origin.x()) / size;
        draw.values.clip[1] = float(region.top() - origin.y()) / size;
        draw.values.clip[2] = float(region.left() + region.width() - origin.x()) / size;
        draw.values.clip[3] = float(region.top() + region.height() - origin.y()) / size;
        if (!draw.uniform->create()) return {false, QStringLiteral("GPU presentation uniform allocation failed"), 0};
        draw.bindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, draw.uniform.get()),
            QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::FragmentStage, tile.pixels)
        });
        if (!draw.bindings->create()) return {false, QStringLiteral("GPU presentation tile binding failed"), 0};
        draws.push_back(std::move(draw));
    }
    auto updates = d.rhi.nextResourceUpdateBatch();
    for (const auto &draw : draws) updates->uploadStaticBuffer(draw.uniform.get(), &draw.values);
    const QColor premultipliedClear = QColor::fromRgbF(clearColor.redF() * clearColor.alphaF(),
        clearColor.greenF() * clearColor.alphaF(), clearColor.blueF() * clearColor.alphaF(), clearColor.alphaF());
    commands.beginPass(&target, premultipliedClear, {1, 0}, updates);
    if (!draws.empty()) {
        commands.setGraphicsPipeline(d.pipeline.get());
        commands.setViewport({0, 0, float(target.pixelSize().width()), float(target.pixelSize().height())});
        for (const auto &draw : draws) {
            commands.setShaderResources(draw.bindings.get());
            commands.draw(6);
        }
    }
    commands.endPass();
    return {true, {}, qsizetype(draws.size())};
}
}
