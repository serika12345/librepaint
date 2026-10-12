/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include <QSet>
#include <exception>

namespace Krita::Image::Gpu {
namespace {
PixelReadbackOutcome failure(ExecutionCode code, const QString &diagnostic)
{
    PixelReadbackOutcome outcome;
    outcome.code = code;
    outcome.diagnostic = diagnostic;
    return outcome;
}
ExecutionCode executionCode(RenderOutcome::Code code)
{
    return code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded :
        code == RenderOutcome::InvalidVersion ? ExecutionCode::InvalidRequest : ExecutionCode::DeviceFailure;
}
}

PixelReadbackOutcome Execution::readPixels(DocumentHandle handle, const Snapshot &pinned,
                                         QRect region, const QList<LayerId> &layers, bool includeProjection, PixelReadArea area)
{
    auto &d = *m_d;
    if (!d.onThread() || !pinned || (area != PixelReadArea::Canvas && area != PixelReadArea::LayerCoordinates)) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read requires a document, valid area and the GPU thread"));
    }
    if (d.active || d.device->rhi().isRecordingFrame()) return failure(ExecutionCode::Busy, QStringLiteral("Pixel read requires submitted frames"));
    if (d.device->readbackPending()) return failure(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
    retireCompletedReadback();
    const auto document = d.documents.constFind(handle);
    if (document == d.documents.cend() || document->requested->documentId() != pinned->documentId()) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read document is no longer attached or belongs to another handle"));
    }
    constexpr qint64 limit = qint64(OperationDocument::CoordinateLimit);
    if (region.width() < 0 || region.height() < 0 ||
        region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read region is outside the supported range"));
    }
    QSet<LayerId> available, selected;
    for (const auto &layer : pinned->layers) available.insert(layer.id);
    for (const auto layer : layers) {
        if (!available.contains(layer) || selected.contains(layer)) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read layer is unavailable or repeated"));
        selected.insert(layer);
    }
    PixelReadbackOutcome outcome;
    outcome.document = pinned;
    outcome.generation = d.generation;
    outcome.region = area == PixelReadArea::Canvas ? region.intersected(QRect(QPoint(0, 0), pinned->size)) : region;
    if (outcome.region.isEmpty() || (layers.isEmpty() && !includeProjection)) {
        outcome.code = ExecutionCode::Success;
        return outcome;
    }
    if (!d.device->rhi().isFeatureSupported(QRhi::ReadBackNonUniformBuffer)) {
        return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU storage buffer readback is unavailable"));
    }
    const auto base = document->rendered;
    const auto projectedBase = document->projected;
    RenderedSnapshot rendered;
    ProjectedSnapshot projected;
    std::shared_ptr<Detail::ReadbackBatch> reads;
    QRhiResourceUpdateBatch *batch = nullptr;
    bool begun = false;
    try {
        const auto frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return failure(frame.code, frame.diagnostic);
        begun = true;
        const auto paint = d.renderer->record(*frame.commands, pinned, base);
        if (paint.code != RenderOutcome::Recorded) {
            cancelFrame();
            begun = false;
            return failure(executionCode(paint.code), paint.diagnostic);
        }
        rendered = paint.version;
        if (includeProjection) {
            const auto compose = d.renderer->compose(*frame.commands, rendered, outcome.region, projectedBase);
            if (compose.code != RenderOutcome::Recorded) {
                cancelFrame();
                begun = false;
                return failure(executionCode(compose.code), compose.diagnostic);
            }
            projected = compose.version;
        }
        QList<QRhiBuffer *> buffers;
        QHash<QRhiBuffer *, qsizetype> indices;
        QList<qsizetype> outputIndices;
        const auto append = [&](LayerId layer, const QList<TileView> &tiles) {
            for (const auto &tile : tiles) {
                if (!outcome.region.intersects(QRect(tile.coordinate * TileRenderer::TileSize,
                                                    QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                if (!indices.contains(tile.pixels)) {
                    if (buffers.size() >= d.maximumReadbackBytes / TileRenderer::TileBytes) return false;
                    indices.insert(tile.pixels, buffers.size());
                    buffers.append(tile.pixels);
                }
                outcome.tiles.append({layer, tile.coordinate, {}});
                outputIndices.append(indices.value(tile.pixels));
            }
            return true;
        };
        bool withinLimit = true;
        for (const auto layer : layers) {
            if (!append(layer, rendered->tiles(layer))) { withinLimit = false; break; }
        }
        if (withinLimit && projected) withinLimit = append(0, projected->tiles());
        if (!withinLimit) {
            cancelFrame();
            begun = false;
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("GPU pixel readback byte capacity exceeded"));
        }
        if (!buffers.isEmpty()) {
            reads = Detail::ReadbackBatch::create(*d.device, buffers.size());
            if (!reads) {
                cancelFrame();
                begun = false;
                return failure(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
            }
            d.readbackRendered = rendered;
            d.readbackProjected = projected;
            batch = d.device->rhi().nextResourceUpdateBatch();
            for (qsizetype i = 0; i < buffers.size(); ++i) {
                batch->readBackBuffer(buffers[i], 0, TileRenderer::TileBytes, reads->results[i].get());
                if (!batch->hasOptimalCapacity()) {
                    frame.commands->resourceUpdate(batch);
                    batch = d.device->rhi().nextResourceUpdateBatch();
                }
            }
            frame.commands->resourceUpdate(batch);
            batch = nullptr;
        }
        const auto submitted = submitFrame();
        begun = false;
        if (submitted.code != ExecutionCode::Success) return failure(submitted.code, submitted.diagnostic);
        if (!d.documents.contains(handle) || d.generation != outcome.generation) {
            return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read document closed or GPU generation changed"));
        }
        if (reads) {
            for (const auto &read : reads->results) {
                if (read->data.size() != TileRenderer::TileBytes) return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU pixel readback did not return a complete tile"));
            }
        }
        for (qsizetype i = 0; i < outcome.tiles.size(); ++i) outcome.tiles[i].rgba = reads->results[outputIndices[i]]->data;
        outcome.code = ExecutionCode::Success;
        return outcome;
    } catch (const std::exception &exception) {
        if (batch) batch->release();
        if (begun) cancelFrame();
        retireCompletedReadback();
        return failure(ExecutionCode::DeviceFailure, QString::fromUtf8(exception.what()));
    }
}
}
