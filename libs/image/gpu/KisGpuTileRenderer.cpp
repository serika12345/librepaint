/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRenderer.h"

#include <QFile>
#include <QMap>
#include <QThread>
#include <rhi/qrhi.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Krita::Image::Gpu {
namespace {
struct Budget { qsizetype limit = 0; qsizetype used = 0; };

template<class T> struct ResourceDeleter
{
    QRhi *rhi = nullptr;
    void operator()(T *resource) const
    {
        if (!resource) return;
        if (rhi->isRecordingFrame()) resource->deleteLater();
        else delete resource;
    }
};
template<class T> using Resource = std::unique_ptr<T, ResourceDeleter<T>>;
template<class T> Resource<T> own(QRhi &rhi, T *resource) { return {resource, {&rhi}}; }

struct Tile
{
    Resource<QRhiBuffer> pixels;
    std::shared_ptr<Budget> budget;
    Tile(Resource<QRhiBuffer> pixels, std::shared_ptr<Budget> budget)
        : pixels(std::move(pixels)), budget(std::move(budget))
    { this->budget->used += TileRenderer::TileBytes; }
    ~Tile() { budget->used -= TileRenderer::TileBytes; }
};
using Tiles = QMap<quint64, std::shared_ptr<Tile>>;
quint64 key(int x, int y) { return (quint64(quint32(y)) << 32) | quint32(x); }
QPoint coordinate(quint64 key) { return {int(quint32(key)), int(quint32(key >> 32))}; }

struct Segment
{
    qint32 ax, ay, bx, by;
    quint32 p0, p1, phase, length;
};
static_assert(sizeof(Segment) == 32);

quint32 integerRoot(quint64 value)
{
    quint64 root = 0, bit = quint64(1) << 62;
    while (bit > value) bit >>= 2;
    while (bit != 0) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1) + bit;
        } else root >>= 1;
        bit >>= 2;
    }
    return quint32(root);
}
qint32 fixed(qreal value) { return qint32(std::llround(value * 16)); }
quint32 pressure(qreal value) { return quint32(std::llround(value * 65535)); }

QList<Segment> geometry(const PaintOperation &operation, quint32 step)
{
    const auto input = operation.inputSamples();
    QList<InputSample> points;
    switch (operation.shape.kind) {
    case ShapeKind::Freehand:
        points = input;
        break;
    case ShapeKind::Line:
        points = {input.front(), input.back()};
        points.front().position = operation.shape.first;
        points.back().position = operation.shape.second;
        break;
    case ShapeKind::Ellipse: {
        const auto &shape = operation.shape;
        const auto center = (shape.first + shape.second) / 2;
        const qreal rx = std::abs(shape.second.x() - shape.first.x()) / 2;
        const qreal ry = std::abs(shape.second.y() - shape.first.y()) / 2;
        constexpr qreal pi = 3.14159265358979323846;
        const int count = std::clamp(int(std::ceil(pi * std::sqrt(std::max(rx, ry) / 0.125))), 16, 65536);
        const qreal cosine = std::cos(shape.rotation), sine = std::sin(shape.rotation);
        points.reserve(count + 1);
        for (int i = 0; i < count; ++i) {
            const qreal angle = qreal(i) * 2 * pi / count;
            const qreal x = rx * std::cos(angle), y = ry * std::sin(angle);
            auto point = input.front();
            point.position = center + QPointF(x * cosine - y * sine, x * sine + y * cosine);
            points.append(point);
        }
        points.append(points.front());
        break;
    }
    }
    QList<Segment> result;
    result.reserve(points.size());
    quint32 phase = 0;
    for (qsizetype i = 0; i < points.size(); ++i) {
        if (i == 0 && points.size() > 1) continue;
        const auto &a = points[i == 0 ? 0 : i - 1];
        const auto &b = points[i];
        const qint32 ax = fixed(a.position.x()), ay = fixed(a.position.y());
        const qint32 bx = fixed(b.position.x()), by = fixed(b.position.y());
        const qint64 dx = qint64(bx) - ax, dy = qint64(by) - ay;
        const quint32 length = integerRoot(quint64(dx * dx) + quint64(dy * dy));
        result.append({ax, ay, bx, by, pressure(a.pressure), pressure(b.pressure), phase, length});
        phase = (quint64(phase) + length) % step;
    }
    return result;
}

struct Parameters
{
    qint32 x, y, width, height;
    quint32 color, opacity, erase, step;
    quint32 count, diameter, reserved0 = 0, reserved1 = 0;
};
static_assert(sizeof(Parameters) == 48);

struct Job
{
    std::shared_ptr<Tile> input, output;
    Resource<QRhiBuffer> samples, parameters;
    Resource<QRhiShaderResourceBindings> bindings;
    QList<Segment> geometry;
    Parameters values;
};
}

struct RenderedVersion::Data
{
    Snapshot document;
    std::shared_ptr<Budget> owner;
    QMap<LayerId, Tiles> layers;
};

struct TileRenderer::Private
{
    QRhi &rhi;
    std::shared_ptr<Budget> budget;
    Resource<QRhiBuffer> zero;
    Resource<QRhiBuffer> layoutParameters;
    Resource<QRhiShaderResourceBindings> layout;
    Resource<QRhiComputePipeline> pipeline;
    bool zeroUploaded = false;
    QString error;

    Private(QRhi &rhi, qsizetype byteBudget)
        : rhi(rhi), budget(std::make_shared<Budget>(Budget{byteBudget})),
          zero(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, TileBytes))),
          layoutParameters(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(Parameters)))),
          layout(own(rhi, rhi.newShaderResourceBindings())), pipeline(own(rhi, rhi.newComputePipeline()))
    {
        if (!rhi.isFeatureSupported(QRhi::Compute) || !zero->create() || !layoutParameters->create()) {
            error = QStringLiteral("GPU storage buffers and compute are required");
            return;
        }
        // The pipeline's layout is independent of each tile's actual bindings.
        layout->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, layoutParameters.get()),
            QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, zero.get()),
            QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::ComputeStage, zero.get()),
            QRhiShaderResourceBinding::bufferStore(3, QRhiShaderResourceBinding::ComputeStage, zero.get())
        });
        QFile shader(QStringLiteral(":/librepaint/gpu/shaders/paint.comp.qsb"));
        if (!layout->create() || !shader.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("GPU paint resources could not be initialized");
            return;
        }
        pipeline->setShaderStage({QRhiShaderStage::Compute, QShader::fromSerialized(shader.readAll())});
        pipeline->setShaderResourceBindings(layout.get());
        if (!pipeline->create()) error = QStringLiteral("GPU paint pipeline could not be created");
    }

    std::shared_ptr<Tile> allocateTile()
    {
        auto pixels = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, TileBytes));
        if (!pixels->create()) return {};
        return std::make_shared<Tile>(std::move(pixels), budget);
    }
};

RenderedVersion::RenderedVersion(std::shared_ptr<Data> data) : m_data(std::move(data)) {}
Snapshot RenderedVersion::document() const { return m_data->document; }
QList<TileView> RenderedVersion::tiles(LayerId layer) const
{
    QList<TileView> result;
    const auto found = m_data->layers.constFind(layer);
    if (found == m_data->layers.cend()) return result;
    result.reserve(found->size());
    for (auto i = found->cbegin(); i != found->cend(); ++i) result.append({coordinate(i.key()), i.value()->pixels.get()});
    return result;
}

TileRenderer::TileRenderer(QRhi &rhi, qsizetype byteBudget)
{
    if (rhi.thread() != QThread::currentThread() || byteBudget < TileBytes) {
        throw std::invalid_argument("invalid GPU owner thread or tile budget");
    }
    m_d = std::make_unique<Private>(rhi, byteBudget);
}
TileRenderer::~TileRenderer() = default;
qsizetype TileRenderer::residentBytes() const { return m_d->budget->used; }

RenderOutcome TileRenderer::record(QRhiCommandBuffer &commands, const Snapshot &document, const RenderedSnapshot &base)
{
    auto &d = *m_d;
    if (d.rhi.thread() != QThread::currentThread() || !d.rhi.isRecordingFrame() || !document ||
        (base && (base->m_data->owner != d.budget || base->document()->documentId() != document->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU versions require their owning device and document")};
    }
    if (!d.error.isEmpty() || d.rhi.isDeviceLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    if (base && base->document() == document) return {RenderOutcome::Recorded, base, {}};
    auto data = std::make_shared<RenderedVersion::Data>();
    data->document = document;
    data->owner = d.budget;
    std::vector<Job> jobs;
    for (const auto &layer : document->layers) {
        qsizetype prefix = 0;
        if (base) {
            for (const auto &old : base->document()->layers) {
                if (old.id != layer.id || old.operations.size() > layer.operations.size()) continue;
                bool matches = true;
                for (qsizetype i = 0; i < old.operations.size(); ++i) {
                    if (old.operations[i] != layer.operations[i]) { matches = false; break; }
                }
                if (matches) {
                    data->layers[layer.id] = base->m_data->layers.value(layer.id);
                    prefix = old.operations.size();
                }
                break;
            }
        }
        auto &tiles = data->layers[layer.id];
        for (qsizetype op = prefix; op < layer.operations.size(); ++op) {
            const auto &operation = *layer.operations[op];
            const auto diameter = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * 16)));
            const auto step = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * operation.brush.spacing * 16)));
            const auto segments = geometry(operation, step);
            QMap<quint64, QList<Segment>> affected;
            const qint64 radius = (diameter + 31) / 32;
            for (const auto &segment : segments) {
                const int left = int(std::max<qint64>(0, (std::min(segment.ax, segment.bx) / 16) - radius));
                const int top = int(std::max<qint64>(0, (std::min(segment.ay, segment.by) / 16) - radius));
                const int right = int(std::min<qint64>(document->size.width() - 1, (std::max(segment.ax, segment.bx) / 16) + radius));
                const int bottom = int(std::min<qint64>(document->size.height() - 1, (std::max(segment.ay, segment.by) / 16) + radius));
                if (right < left || bottom < top) continue;
                for (int y = top / TileSize; y <= bottom / TileSize; ++y) {
                    qreal first = 0, last = 1;
                    if (segment.ay != segment.by) {
                        const qreal lower = qreal(y * TileSize - radius - 1) * 16;
                        const qreal upper = qreal((y + 1) * TileSize + radius + 1) * 16;
                        const qreal dy = qreal(segment.by) - segment.ay;
                        const qreal a = (lower - segment.ay) / dy, b = (upper - segment.ay) / dy;
                        first = std::max<qreal>(0, std::min(a, b));
                        last = std::min<qreal>(1, std::max(a, b));
                        if (first > last) continue;
                    }
                    const qreal a = (segment.ax + (qreal(segment.bx) - segment.ax) * first) / 16;
                    const qreal b = (segment.ax + (qreal(segment.bx) - segment.ax) * last) / 16;
                    const int rowLeft = std::max(left, int(std::floor(std::min(a, b) - radius - 1)));
                    const int rowRight = std::min(right, int(std::ceil(std::max(a, b) + radius + 1)));
                    for (int x = rowLeft / TileSize; x <= rowRight / TileSize; ++x) {
                        affected[key(x, y)].append(segment);
                        if (affected.size() > (d.budget->limit - d.budget->used) / TileBytes) {
                            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                        }
                    }
                }
            }
            if (affected.size() > (d.budget->limit - d.budget->used) / TileBytes) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
            }
            for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                const auto bytes = quint64(tile.value().size()) * sizeof(Segment);
                if (bytes > std::numeric_limits<quint32>::max()) {
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU operation buffer limit exceeded")};
                }
                Job job;
                job.input = tiles.value(tile.key());
                job.output = d.allocateTile();
                job.samples = own(d.rhi, d.rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, quint32(bytes)));
                job.parameters = own(d.rhi, d.rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(Parameters)));
                job.bindings = own(d.rhi, d.rhi.newShaderResourceBindings());
                if (!job.output || !job.samples->create() || !job.parameters->create()) {
                    return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
                }
                job.geometry = tile.value();
                const auto point = coordinate(tile.key()) * TileSize;
                const auto &rgba = operation.brush.rgba;
                job.values = {point.x(), point.y(), document->size.width(), document->size.height(),
                              quint32(rgba[0]) | quint32(rgba[1]) << 8 | quint32(rgba[2]) << 16 | quint32(rgba[3]) << 24,
                              operation.brush.opacity, quint32(operation.brush.erase), step, quint32(tile.value().size()), diameter};
                job.bindings->setBindings({
                    QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, job.parameters.get()),
                    QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, job.samples.get()),
                    QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::ComputeStage, job.input ? job.input->pixels.get() : d.zero.get()),
                    QRhiShaderResourceBinding::bufferStore(3, QRhiShaderResourceBinding::ComputeStage, job.output->pixels.get())
                });
                if (!job.bindings->create()) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint binding failed")};
                tiles[tile.key()] = job.output;
                jobs.push_back(std::move(job));
            }
        }
    }
    if (!d.zeroUploaded) {
        auto updates = d.rhi.nextResourceUpdateBatch();
        updates->uploadStaticBuffer(d.zero.get(), QByteArray(TileBytes, '\0'));
        commands.resourceUpdate(updates);
        d.zeroUploaded = true;
    }
    for (const auto &job : jobs) {
        auto updates = d.rhi.nextResourceUpdateBatch();
        updates->uploadStaticBuffer(job.samples.get(), job.geometry.constData());
        updates->uploadStaticBuffer(job.parameters.get(), &job.values);
        commands.beginComputePass(updates);
        commands.setComputePipeline(d.pipeline.get());
        commands.setShaderResources(job.bindings.get());
        commands.dispatch(TileSize / 8, TileSize / 8, 1);
        commands.endComputePass();
    }
    return {RenderOutcome::Recorded, RenderedSnapshot(new RenderedVersion(data)), {}};
}
}
