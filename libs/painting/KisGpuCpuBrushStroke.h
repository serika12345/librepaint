/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CPU_BRUSH_STROKE_H
#define KIS_GPU_CPU_BRUSH_STROKE_H

#include <kritapainting_export.h>
#include <kis_types.h>
#include <QString>
#include <QRect>
#include <QByteArray>
#include <gpu/KisGpuOperationDocument.h>
#include <memory>

/** Temporary native paint layer or Alpha8 mask and optional sampling projection
 * for CPU brush engines and pixel tools. The GPU document owns previews and history; the
 * native scheduler owns only the active stroke.
 */
class KRITAPAINTING_EXPORT KisGpuCpuBrushStroke
{
public:
    /** Smudge adds live dabs to a captured source; clone reads an updating native projection. */
    enum class Projection { None, Captured, Updating };
    static std::unique_ptr<KisGpuCpuBrushStroke> create(KisImageSP image, KisNodeSP node, QString &diagnostic,
                                                      Projection projection = Projection::None, KisNodeSP samplingNode = {},
                                                      LibrePaint::Image::Gpu::HistoryAction action = LibrePaint::Image::Gpu::HistoryAction::PaintStroke);
    /** Prepare native sampling and remap a cloned preset's source node. */
    static std::unique_ptr<KisGpuCpuBrushStroke> createForPreset(KisImageSP image, KisNodeSP node,
                                                               KisPaintOpPresetSP &preset, QString &diagnostic);
    ~KisGpuCpuBrushStroke();

    KisImageSP image() const;
    KisNodeSP node() const;
    KisNodeSP samplingNode() const;
    bool updatePreview(KisStrokeId stroke);
    bool commit();

private:
    KisGpuCpuBrushStroke();
    bool publish(QRect area, const QByteArray &native);
    struct Private;
    std::unique_ptr<Private> m_d;
};
#endif
