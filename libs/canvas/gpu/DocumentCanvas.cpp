/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "DocumentCanvas.h"
#include <algorithm>
#include <stdexcept>

namespace Krita::Canvas {
namespace {
using Store = KisGpuTileStore;
bool sameLayers(const QVector<Store::Layer> &first, const QVector<Store::Layer> &second)
{
    return first.size() == second.size() && std::equal(first.begin(), first.end(), second.begin(),
        [](const auto &a, const auto &b) {
            return a.pixels == b.pixels && a.mask == b.mask && a.opacity == b.opacity && a.operation == b.operation;
        });
}
}

DocumentCanvas::DocumentCanvas(KisGpuDevice &device, std::shared_ptr<Store> store,
    std::shared_ptr<KisGpuLayerDocument> document, QWindow &window, QRect bounds)
    : m_device(device), m_store(std::move(store)), m_document(std::move(document))
{
    if (!m_store || !m_document || !m_device.available() || !m_store->deviceAvailable()
        || m_document->state() == KisGpuLayerDocument::State::Restoring
        || m_document->state() == KisGpuLayerDocument::State::Suspended
        || m_document->state() == KisGpuLayerDocument::State::Failed)
        throw std::invalid_argument("GPU canvas requires an available document and store");
    m_projection = std::make_unique<KisGpuLayerProjection>(*m_store, bounds);
    m_acceptedLayers = m_document->projectionLayers();
    if (m_projection->setLayers(m_acceptedLayers) != Store::Error::None)
        throw std::runtime_error("Cannot prepare GPU document projection");
    m_surface = std::make_unique<SurfaceRenderer>(m_device, window);
}

DocumentCanvas::~DocumentCanvas() = default;

void DocumentCanvas::setView(const GpuRenderer::View &view)
{
    if (view.canvasToTarget != m_view.canvasToTarget || view.backgroundRgba != m_view.backgroundRgba
        || view.sampling != m_view.sampling || view.displayLookup != m_view.displayLookup) {
        m_view = view;
        m_repaint = true;
    }
}

SurfaceRenderer::Statistics DocumentCanvas::statistics() const { return m_surface->statistics(); }

DocumentCanvas::Result DocumentCanvas::poll()
{
    if (!m_device.available()) { m_gpuError = Store::Error::DeviceLost; return Result::DeviceLost; }
    m_document->poll();
    m_projection->poll();
    m_surface->poll();
    if (m_document->state() == KisGpuLayerDocument::State::Suspended
        || m_document->state() == KisGpuLayerDocument::State::Restoring) return Result::Busy;
    if (m_projection->status() == Store::Status::Failed) return Result::GpuRejected;
    if (m_projection->status() == Store::Status::Succeeded) m_projectionReady = true;

    const auto published = m_projection->snapshot();
    if (m_projectionReady && (!m_image.texture() || (!m_repaint && !(published.pixels == m_imageProjection.pixels)))) {
        if (!m_surface->statistics().rendering.pendingFrames) {
            auto image = m_store->textureSnapshot(published.pixels, m_projection->bounds(), std::move(m_image));
            m_gpuError = image.error;
            if (m_gpuError != Store::Error::None) return Result::GpuRejected;
            m_image = std::move(image);
            m_imageProjection = published;
            m_repaint = true;
            m_store->poll();
        }
    }

    bool updated = false;
    if (m_image.texture() && m_image.completion.status() == Store::Status::Failed) return Result::GpuRejected;
    if (m_repaint && m_image.texture() && m_image.completion.status() == Store::Status::Succeeded) {
        auto frame = m_surface->present(m_image, m_view);
        m_surfaceError = frame.error;
        if (frame.accepted()) {
            m_frame = std::move(frame);
            m_displayed = m_imageProjection;
            m_repaint = false;
            updated = true;
        } else if (frame.error == SurfaceRenderer::Error::WindowUnavailable) return Result::WindowUnavailable;
        else if (frame.error != SurfaceRenderer::Error::QueueFull && frame.error != SurfaceRenderer::Error::ResizePending)
            return frame.error == SurfaceRenderer::Error::DeviceLost ? Result::DeviceLost : Result::GpuRejected;
    }

    const auto layers = m_document->projectionLayers();
    if (m_projection->status() != Store::Status::Pending && !sameLayers(layers, m_acceptedLayers)) {
        m_gpuError = m_projection->setLayers(layers);
        if (m_gpuError != Store::Error::None) return Result::GpuRejected;
        m_acceptedLayers = layers;
    }
    if (updated) return Result::Updated;
    return m_repaint || m_projection->status() == Store::Status::Pending ? Result::Busy : Result::Idle;
}
}
