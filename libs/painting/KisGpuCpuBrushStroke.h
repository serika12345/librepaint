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
#include <memory>

/** Temporary native paint layer and optional sampling projection for CPU
 * freehand engines. The GPU document owns previews and history; the native
 * scheduler owns only the active stroke.
 */
class KRITAPAINTING_EXPORT KisGpuCpuBrushStroke
{
public:
    static std::unique_ptr<KisGpuCpuBrushStroke> create(KisImageSP image, KisNodeSP node, QString &diagnostic,
                                                      bool sampleProjection = false);
    ~KisGpuCpuBrushStroke();

    KisImageSP image() const;
    KisNodeSP node() const;
    bool updatePreview(KisStrokeId stroke);
    bool commit();

private:
    KisGpuCpuBrushStroke();
    bool publish(QRect area, const QByteArray &native);
    struct Private;
    std::unique_ptr<Private> m_d;
};
#endif
