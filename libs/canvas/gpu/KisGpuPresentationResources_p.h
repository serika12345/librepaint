/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PRESENTATION_RESOURCES_P_H
#define KIS_GPU_PRESENTATION_RESOURCES_P_H

#include <rhi/qrhi.h>
#include <memory>

namespace Krita::Canvas::Gpu::Detail {
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
}
#endif
