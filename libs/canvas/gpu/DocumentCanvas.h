/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_DOCUMENT_CANVAS_H
#define LIBREPAINT_GPU_DOCUMENT_CANVAS_H
#include "SurfaceRenderer.h"
#include "KisGpuLayerDocument.h"
#include "KisGpuLayerProjection.h"

namespace Krita::Canvas {
/** One document's projection, display image and native window presentation.
 * Device and prepared/exposed window outlive this owner. Calls and destruction
 * use the GUI/submitting thread. Recreate after shared GPU replacement.
 */
class DocumentCanvas
{
public:
    enum class Result { Idle, Updated, Busy, GpuRejected, WindowUnavailable, DeviceLost };
    /** Owners must belong to this device. Unavailable resources or allocation failure throws. */
    DocumentCanvas(KisGpuDevice &device, std::shared_ptr<KisGpuTileStore> store,
                   std::shared_ptr<KisGpuLayerDocument> document, QWindow &window, QRect bounds);
    ~DocumentCanvas();
    DocumentCanvas(const DocumentCanvas &) = delete;
    DocumentCanvas &operator=(const DocumentCanvas &) = delete;
    void setView(const GpuRenderer::View &view);
    /** Exposure and native window recreation repaint the retained image. */
    void requestRepaint() { m_repaint = true; }
    /** Nonblocking collection; one projection, image copy and bounded presentation can be pending. */
    Result poll();
    SurfaceRenderer::Frame lastFrame() const { return m_frame; }
    /** Immutable projection and layers associated with the last accepted display frame. */
    KisGpuLayerProjection::Snapshot displayedSnapshot() const { return m_displayed; }
    SurfaceRenderer::Statistics statistics() const;
    KisGpuTileStore::Error lastGpuError() const { return m_gpuError; }
    SurfaceRenderer::Error lastSurfaceError() const { return m_surfaceError; }
private:
    KisGpuDevice &m_device;
    std::shared_ptr<KisGpuTileStore> m_store;
    std::shared_ptr<KisGpuLayerDocument> m_document;
    std::unique_ptr<KisGpuLayerProjection> m_projection;
    std::unique_ptr<SurfaceRenderer> m_surface;
    QVector<KisGpuTileStore::Layer> m_acceptedLayers;
    KisGpuTileStore::TextureSnapshot m_image;
    KisGpuLayerProjection::Snapshot m_imageProjection, m_displayed;
    SurfaceRenderer::Frame m_frame;
    GpuRenderer::View m_view;
    bool m_projectionReady = false, m_repaint = true;
    KisGpuTileStore::Error m_gpuError = KisGpuTileStore::Error::None;
    SurfaceRenderer::Error m_surfaceError = SurfaceRenderer::Error::None;
};
}
#endif
