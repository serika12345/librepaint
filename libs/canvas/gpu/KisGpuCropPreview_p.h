/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CROP_PREVIEW_P_H
#define KIS_GPU_CROP_PREVIEW_P_H

#include "KisGpuPresentationResources_p.h"
#include "KisGpuTilePresentation.h"
#include <vector>

namespace LibrePaint::Canvas::Gpu::Detail {
/** Prepares vector coverage and frame-owned bindings before the target changes. */
class CropPreviewRenderer
{
public:
    struct Draw
    {
        Buffer uniform;
        Buffer geometry;
        Bindings bindings;
        quint32 instances = 1;
    };
    explicit CropPreviewRenderer(Image::Gpu::DevicePtr device);
    bool prepare(const CropPreview &crop, const PresentationTarget &target,
                 std::vector<Draw> &draws, QString &diagnostic);
    void draw(WGPURenderPassEncoder pass, const std::vector<Draw> &draws) const;

private:
    Image::Gpu::DevicePtr m_device;
    Shader m_shader;
    Pipeline m_pipeline;
    Layout m_layout;
    quint32 m_format = 0;
    bool preparePipeline(quint32 format);
};
}
#endif
